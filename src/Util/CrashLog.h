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

namespace ms
{
	// Install the crash handlers. Call once, as early as possible in main().
	void install_crash_logger();
}
