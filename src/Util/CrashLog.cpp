//////////////////////////////////////////////////////////////////////////////////
//	Crash / assertion stack logger. See CrashLog.h.								//
//////////////////////////////////////////////////////////////////////////////////
#include "CrashLog.h"

// For PLATFORM_MACOS, which selects the backtrace branch below. Nothing in the
// build passes -DPLATFORM_*, so the header has to be included to see it.
#include "../../platform/shared/PlatformConfig.h"

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

// macOS only, deliberately. The code below is plain POSIX and would work on
// Linux too, but <execinfo.h> is a glibc extension that musl does not ship, and
// nobody here can build or test that -- so Linux and iOS keep the no-op stub
// they had. Widening this to Linux is a one-line change once someone can build it.
#elif defined(PLATFORM_MACOS)

// POSIX counterpart of the Windows logger above. Deliberately minimal: a
// signal handler that writes a raw backtrace to crashlog.txt (same filename and
// same cwd as the Windows path) and to stderr, then lets the default action
// run so the OS still produces its own report / core file.
//
// Everything in the handler is restricted to async-signal-safe calls: a
// pre-opened file descriptor, write(), and backtrace_symbols_fd() -- which,
// unlike backtrace_symbols(), writes straight to an fd instead of allocating.
// backtrace() itself can lazily initialize on first use, so install() warms it
// up while the process is still healthy.
//
// Symbol names come from the dynamic symbol table, so file/line resolution
// needs `atos -o OpenStory <addr>`; the addresses in the
// log are enough for that.

#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <exception>

namespace ms
{
	namespace
	{
		constexpr int MAX_FRAMES = 64;

		// Opened up front: open() in a handler after heap corruption is a much
		// worse bet than holding a descriptor for the life of the process.
		int g_log_fd = -1;
		volatile sig_atomic_t g_logged = 0;

		void write_str(int fd, const char* s)
		{
			if (fd < 0 || s == nullptr)
				return;

			size_t len = std::strlen(s);

			while (len > 0)
			{
				ssize_t n = ::write(fd, s, len);

				if (n <= 0)
					return;

				s += n;
				len -= static_cast<size_t>(n);
			}
		}

		// No snprintf in a signal handler; render the small integers by hand.
		void write_int(int fd, long value)
		{
			char buf[24];
			int i = static_cast<int>(sizeof(buf));
			bool negative = value < 0;
			unsigned long v = negative
				? static_cast<unsigned long>(-(value + 1)) + 1u
				: static_cast<unsigned long>(value);

			buf[--i] = '\0';

			do
			{
				buf[--i] = static_cast<char>('0' + (v % 10u));
				v /= 10u;
			}
			while (v != 0 && i > 1);

			if (negative && i > 0)
				buf[--i] = '-';

			write_str(fd, buf + i);
		}

		void write_report(int fd, const char* label, int sig, void* addr)
		{
			write_str(fd, "\n=== ");
			write_str(fd, label);
			write_str(fd, " ===\nsignal: ");
			write_int(fd, sig);

			const char* name = (sig > 0 && sig < NSIG) ? ::strsignal(sig) : nullptr;

			if (name)
			{
				write_str(fd, " (");
				write_str(fd, name);
				write_str(fd, ")");
			}

			// Printed for every signal report, including a null address: a
			// null fault address is the single most informative value there is.
			if (sig != 0)
			{
				write_str(fd, "\nfault address: 0x");

				// Hex, most significant nibble first.
				unsigned long long a = reinterpret_cast<unsigned long long>(addr);
				char hex[17];
				hex[16] = '\0';

				for (int i = 15; i >= 0; i--)
				{
					hex[i] = "0123456789abcdef"[a & 0xFull];
					a >>= 4;
				}

				write_str(fd, hex);
			}

			write_str(fd, "\n--- stack ---\n");

			void* frames[MAX_FRAMES];
			int captured = ::backtrace(frames, MAX_FRAMES);

			if (captured > 0)
				::backtrace_symbols_fd(frames, captured, fd);

			write_str(fd, "\n");
		}

		void signal_handler(int sig, siginfo_t* info, void*)
		{
			// Only the first fatal signal writes; a fault inside the handler
			// must not recurse into it.
			if (g_logged)
				::_exit(3);

			g_logged = 1;

			void* addr = info ? info->si_addr : nullptr;

			write_report(g_log_fd, "FATAL SIGNAL", sig, addr);
			write_report(STDERR_FILENO, "FATAL SIGNAL", sig, addr);
			write_str(STDERR_FILENO, "[CrashLog] captured to crashlog.txt\n");

			// Restore the default action and re-raise, so the OS still writes
			// its own crash report and the exit status reflects the signal.
			struct sigaction dfl;
			std::memset(&dfl, 0, sizeof(dfl));
			dfl.sa_handler = SIG_DFL;
			sigemptyset(&dfl.sa_mask);
			::sigaction(sig, &dfl, nullptr);
			::raise(sig);
		}

		void terminate_handler()
		{
			if (!g_logged)
			{
				g_logged = 1;

				write_report(g_log_fd, "UNCAUGHT EXCEPTION / TERMINATE", 0, nullptr);
				write_report(STDERR_FILENO, "UNCAUGHT EXCEPTION / TERMINATE", 0, nullptr);
				write_str(STDERR_FILENO, "[CrashLog] captured to crashlog.txt\n");
			}

			std::abort();
		}
	}

	void install_crash_logger()
	{
		// cwd is the wz/ working dir at runtime, matching the Windows path.
		g_log_fd = ::open("crashlog.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

		// Warm up the unwinder (and its lazy dyld lookups) while the process
		// is still healthy, so the handler does not have to do it.
		void* warmup[4];
		(void)::backtrace(warmup, 4);

		// A stack-overflow SIGSEGV leaves no room on the faulting stack, so run
		// the handler on its own. Leaked by design: it has to outlive main.
		static const size_t altsize = SIGSTKSZ < 65536 ? 65536 : static_cast<size_t>(SIGSTKSZ);
		stack_t altstack;
		std::memset(&altstack, 0, sizeof(altstack));
		altstack.ss_sp = std::malloc(altsize);
		altstack.ss_size = altsize;
		altstack.ss_flags = 0;

		if (altstack.ss_sp != nullptr)
			::sigaltstack(&altstack, nullptr);

		struct sigaction sa;
		std::memset(&sa, 0, sizeof(sa));
		sa.sa_sigaction = signal_handler;
		sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
		sigemptyset(&sa.sa_mask);

		const int signals[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };

		for (int sig : signals)
			::sigaction(sig, &sa, nullptr);

		std::set_terminate(terminate_handler);
	}
}

#else

namespace ms
{
	void install_crash_logger() {}
}

#endif
