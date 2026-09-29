//////////////////////////////////////////////////////////////////////////////////
//	Crash / assertion stack logger.												//
//																				//
//	Captures the call stack for fatal errors (STL debug assertions such as		//
//	"vector subscript out of range", plus access violations and other SEH		//
//	exceptions) and writes a symbolized backtrace to crashlog.txt next to the	//
//	executable. This lets us find the exact faulting function after the fact,	//
//	without having to reproduce the crash under a live debugger.				//
//////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef OPENSTORY_LAN_LOG
extern "C" void openstory_diagnostics_checkpoint(const char *phase, unsigned frame = 0);
#else
inline void openstory_diagnostics_checkpoint(const char *, unsigned = 0) {}
#endif

namespace ms
{
	// Install the crash handlers. Call once, as early as possible in main().
	void install_crash_logger();
}
