/* There is no include guard here - by purpose. This file can be included
 * after system includes that redefine the assert() macro.
 * The actual problem for which this file got separated from utilities.h
 * happens in the sat-solver code, when local include files include
 * Solver.h which in turn includes the system's assert.h. */

#include "error.h" /* for prt_error() */

#ifndef STRINGIFY
#define STR(x) #x
#define STRINGIFY(x) STR(x)
#endif /* STRINGIFY */

#define FILELINE __FILE__ ":" STRINGIFY(__LINE__)

#ifdef _WIN32
#define DEBUG_TRAP (*((volatile int*) 0x0) = 42)
#else
#define DEBUG_TRAP __builtin_trap()
#endif

#define assert(ex, ...) {                                                   \
	if (!(ex)) {                                                             \
		prt_error("Fatal error: \nAssertion (" #ex ") failed at " FILELINE ": " __VA_ARGS__);  \
		prt_error("\n");                                                \
		DEBUG_TRAP;  /* leave stack trace in debugger */                      \
	}                                                                        \
}
