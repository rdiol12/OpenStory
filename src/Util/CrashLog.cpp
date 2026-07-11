//////////////////////////////////////////////////////////////////////////////////
//	Crash / assertion stack logger. See CrashLog.h.								//
//////////////////////////////////////////////////////////////////////////////////
#include "CrashLog.h"

#ifdef _WIN32

#include <windows.h>
#include <dbghelp.h>
#include <crtdbg.h>

#include <cstdio>
#include <cstdlib>

#pragma comment(lib, "dbghelp.lib")

namespace ms
{
	namespace
	{
		// Guard so we only ever write one crash log and terminate once, even
		// if both the CRT report hook and the SEH filter fire for the same
		// failure.
		volatile LONG g_logged = 0;

		void write_stack(FILE* out, CONTEXT* context)
		{
			HANDLE process = GetCurrentProcess();
			HANDLE thread = GetCurrentThread();

			SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
			SymInitialize(process, nullptr, TRUE);

			void* frames[62];
			USHORT captured = 0;

			// If we have a CONTEXT (SEH path) walk from the faulting frame.
			// Otherwise (assert path) capture the current call stack.
			if (context == nullptr)
			{
				captured = CaptureStackBackTrace(0, 62, frames, nullptr);

				for (USHORT i = 0; i < captured; i++)
				{
					DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);

					char symbuf[sizeof(SYMBOL_INFO) + 256];
					SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(symbuf);
					sym->SizeOfStruct = sizeof(SYMBOL_INFO);
					sym->MaxNameLen = 255;

					DWORD64 disp = 0;
					IMAGEHLP_LINE64 line;
					line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
					DWORD linedisp = 0;

					std::fprintf(out, "  [%02u] ", i);

					if (SymFromAddr(process, addr, &disp, sym))
						std::fprintf(out, "%s +0x%llx", sym->Name, static_cast<unsigned long long>(disp));
					else
						std::fprintf(out, "0x%llx", static_cast<unsigned long long>(addr));

					if (SymGetLineFromAddr64(process, addr, &linedisp, &line))
						std::fprintf(out, "  (%s:%lu)", line.FileName, line.LineNumber);

					std::fprintf(out, "\n");
				}
			}
			else
			{
				STACKFRAME64 sf;
				ZeroMemory(&sf, sizeof(sf));

				DWORD machine = IMAGE_FILE_MACHINE_AMD64;
				sf.AddrPC.Offset = context->Rip;
				sf.AddrPC.Mode = AddrModeFlat;
				sf.AddrFrame.Offset = context->Rbp;
				sf.AddrFrame.Mode = AddrModeFlat;
				sf.AddrStack.Offset = context->Rsp;
				sf.AddrStack.Mode = AddrModeFlat;

				for (int i = 0; i < 62; i++)
				{
					if (!StackWalk64(machine, process, thread, &sf, context,
						nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
						break;

					if (sf.AddrPC.Offset == 0)
						break;

					DWORD64 addr = sf.AddrPC.Offset;

					char symbuf[sizeof(SYMBOL_INFO) + 256];
					SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(symbuf);
					sym->SizeOfStruct = sizeof(SYMBOL_INFO);
					sym->MaxNameLen = 255;

					DWORD64 disp = 0;
					IMAGEHLP_LINE64 line;
					line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
					DWORD linedisp = 0;

					std::fprintf(out, "  [%02d] ", i);

					if (SymFromAddr(process, addr, &disp, sym))
						std::fprintf(out, "%s +0x%llx", sym->Name, static_cast<unsigned long long>(disp));
					else
						std::fprintf(out, "0x%llx", static_cast<unsigned long long>(addr));

					if (SymGetLineFromAddr64(process, addr, &linedisp, &line))
						std::fprintf(out, "  (%s:%lu)", line.FileName, line.LineNumber);

					std::fprintf(out, "\n");
				}
			}

			std::fflush(out);
		}

		FILE* open_log()
		{
			FILE* out = nullptr;
			// cwd is the wz/ working dir at runtime; crashlog.txt lands there.
			fopen_s(&out, "crashlog.txt", "w");

			if (!out)
				out = stderr;

			return out;
		}

		int __cdecl report_hook(int reportType, char* message, int* returnValue)
		{
			if (returnValue)
				*returnValue = 0;

			// Only care about assertions (this catches the STL "vector
			// subscript out of range" / iterator debug checks).
			if (reportType != _CRT_ASSERT && reportType != _CRT_ERROR)
				return FALSE;

			if (InterlockedExchange(&g_logged, 1) != 0)
				return TRUE; // already logged by another handler

			FILE* out = open_log();
			std::fprintf(out, "=== CRT ASSERT / STL CHECK ===\n");
			if (message)
				std::fprintf(out, "message: %s\n", message);
			std::fprintf(out, "--- stack ---\n");
			write_stack(out, nullptr);
			if (out != stderr)
				std::fclose(out);

			std::fprintf(stderr, "\n[CrashLog] Assertion captured to crashlog.txt\n");
			std::fflush(stderr);

			// Terminate cleanly; the log is already flushed. Returning would
			// otherwise continue into undefined out-of-bounds behaviour.
			_exit(3);

			return TRUE;
		}

		LONG WINAPI seh_filter(EXCEPTION_POINTERS* info)
		{
			if (InterlockedExchange(&g_logged, 1) != 0)
				return EXCEPTION_EXECUTE_HANDLER;

			FILE* out = open_log();
			std::fprintf(out, "=== UNHANDLED EXCEPTION ===\n");
			std::fprintf(out, "code: 0x%08lx  address: 0x%llx\n",
				info->ExceptionRecord->ExceptionCode,
				reinterpret_cast<unsigned long long>(info->ExceptionRecord->ExceptionAddress));
			std::fprintf(out, "--- stack ---\n");
			write_stack(out, info->ContextRecord);
			if (out != stderr)
				std::fclose(out);

			std::fprintf(stderr, "\n[CrashLog] Exception captured to crashlog.txt\n");
			std::fflush(stderr);

			_exit(3);

			return EXCEPTION_EXECUTE_HANDLER;
		}
	}

	void install_crash_logger()
	{
		// Route asserts through our hook and suppress the blocking dialog.
		_CrtSetReportHook(report_hook);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

		SetUnhandledExceptionFilter(seh_filter);
	}
}

#else

namespace ms
{
	void install_crash_logger() {}
}

#endif
