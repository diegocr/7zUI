
#ifdef DEBUG
# include <proto/exec.h>
# include <stdarg.h>
# include <SDI_compiler.h>

#ifndef RawPutChar
#define RawPutChar(c) ({ \
  ULONG _RawPutChar_c = (c); \
  { \
  register struct Library * const __RawPutChar__bn __asm("a6") = SysBase;\
  register ULONG __RawPutChar_c __asm("d0") = (_RawPutChar_c); \
  __asm volatile ("jsr a6@(-516:W)" \
  : \
  : "r"(__RawPutChar__bn), "r"(__RawPutChar_c) \
  : "d0", "d1", "a0", "a1", "fp0", "fp1", "cc", "memory"); \
  } \
})
#endif

VOID KPutC(UBYTE ch)
{
	RawPutChar(ch);
}

VOID KPutStr(CONST_STRPTR string)
{
	UBYTE ch;
	
	while((ch = *string++))
		KPutC( ch );
}

STATIC VOID ASM RawPutC(REG(d0,UBYTE ch))
{
	KPutC(ch); 
}

VOID KPrintF(CONST_STRPTR fmt, ...)
{
	va_list args;
	
	va_start(args,fmt);
	RawDoFmt((STRPTR)fmt, args,(VOID (*)())RawPutC, NULL );
	va_end(args);
}

#endif /* DEBUG */
