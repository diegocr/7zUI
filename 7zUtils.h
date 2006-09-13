#ifndef __7ZUTILS_H
#define __7ZUTILS_H

#include <stdarg.h>
#include <exec/types.h>

GLOBAL void PrintError(char * msg);
GLOBAL void PrintErrorFmt(const char * fmt, ...);

GLOBAL BOOL SwitchCurrentdir( STRPTR path, BPTR * where, BPTR * old );
GLOBAL int MakeDir( char * fullpath );

GLOBAL LONG VSNPrintf(STRPTR outbuf, LONG size, CONST_STRPTR fmt, va_list args);
GLOBAL LONG SNPrintf( STRPTR outbuf, LONG size, CONST_STRPTR fmt, ... );

#endif /* __7ZUTILS_H */
