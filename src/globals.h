#ifndef WIN_C_HTTP_GLOBALS
#define WIN_C_HTTP_GLOBALS

#define DEBUG

#ifdef DEBUG
#define DEBUGPRINT(formatstr, ...) printf(formatstr, __VA_ARGS__)
#else
#define DEBUGPRINT(formatstr,...)
#endif

#endif
