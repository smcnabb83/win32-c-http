#ifndef WIN_C_HTTP_GLOBALS
#define WIN_C_HTTP_GLOBALS

#ifdef DEBUG
#define DEBUGPRINT(formatstr, ...) printf(formatstr, __VA_ARGS__)
#define ASSERT(expr) if(!(expr)) {*(int *)0 = 0;}
#else
#define DEBUGPRINT(formatstr,...)
#define ASSERT(expr)
#endif


#endif
