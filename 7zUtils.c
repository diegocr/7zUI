
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include "7zMain.h"
#include "7zUtils.h"
#include "7zDebug.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDI_compiler.h>
/****************************************************************************/

void PrintError(char * msg)
{
	PrintFault(IoErr(), "IoErr");
	
	if( G->CliUsage )
	{
		Printf("\n +++ Error: %s\n", (long) msg );
	}
	else
	{
		STATIC UBYTE okm[] = "Ok";
		static struct IntuiText body = { 0,0,0, 15,5, NULL, NULL, NULL };
		static struct IntuiText   ok = { 0,0,0,  6,3, NULL, okm, NULL };
		
		if(((struct Process *)FindTask(NULL))->pr_WindowPtr != (APTR)-1L)
		{
			body.IText = (UBYTE *)msg;
			AutoRequest(NULL,&body,NULL,&ok,0,0,640,72);
		}
	}
}

void PrintErrorFmt(const char * fmt, ...)
{
	va_list args;
	char buf[1024];
	
	va_start (args, fmt);
	VSNPrintf( buf, sizeof(buf), fmt, args );
	va_end(args);
	
	PrintError( buf );
}

/****************************************************************************/

BOOL SwitchCurrentdir( STRPTR path, BPTR * where, BPTR * old )
{
	/* set the CurrentDir before extracting */
	
	if((*where) == 0)
	{
		// the function wasn't used previously, lock then
		
		(*where) = Lock( path, SHARED_LOCK );
		
		if(!(*where)) {
			PrintError("cannot lock selected drawer !?");
			return FALSE;
		}
		
		(*old) = CurrentDir((*where));
	}
	else
	{
		CurrentDir((*old));
		UnLock((*where));
	}
	
	return TRUE;
}

/****************************************************************************/

int MakeDir( char * fullpath )
{
	char subdir[4096];
	char *sep, *xpath=(char *)fullpath;
	int rc = 0;
	
	sep = (char *) fullpath;
	sep = strchr(sep, '/');
	
	while( sep )
	{
		BPTR dlock;
		int len;
		
		len = sep - xpath;
		CopyMem( xpath, subdir, len);
		subdir[len] = 0;
		
		if((dlock = CreateDir((STRPTR) subdir )))
			UnLock( dlock );
		else
		{
			if((rc = IoErr()) == ERROR_OBJECT_EXISTS)
			{
				dlock = Lock((STRPTR) subdir, SHARED_LOCK );
				
				if( !dlock )
				{
					/* this can't happend!, I think.. */
					rc = -1;
				}
				else
				{
					struct FileInfoBlock fib;
					
					if(Examine(dlock,&fib) == DOSFALSE)
					{
						rc = IoErr();
					}
					else
					{
						if(fib.fib_DirEntryType > 0)
							rc = 0;
						
						#ifdef DEBUG
						if((rc != 0) || fib.fib_DirEntryType == ST_SOFTLINK)
						{
							DBG("\aDirectory Name exists and %spoint to a file !!!!\n", ((fib.fib_DirEntryType == ST_SOFTLINK) ? "MAY ":""));
						}
						#endif
					}
					
					UnLock( dlock );
				}
			}
			
			if(rc != 0)
				break;
		}
		
		sep = strchr(sep+1, '/');
	}
	
	return(rc);
}

/***************************************************************************/

struct RawDoFmtStream {
	
	STRPTR Buffer;
	LONG Size;
};

static void RawDoFmtChar( REG(d0,UBYTE c), REG(a3,struct RawDoFmtStream *s))
{
	if(s->Size > 0)
	{
		*(s->Buffer)++ = c;
		 (s->Size)--;
		
		if(s->Size == 1)
		{
			*(s->Buffer)	= '\0';
			  s->Size	= 0;
		}
	}
}

LONG VSNPrintf(STRPTR outbuf, LONG size, CONST_STRPTR fmt, va_list args)
{
	long rc = 0;
	
	if((size > (long)sizeof(char)) && (outbuf != NULL))
	{
		struct RawDoFmtStream s;
		
		s.Buffer = outbuf;
		s.Size	 = size;
		
		RawDoFmt( fmt, (APTR)args, (void (*)())RawDoFmtChar, (APTR)&s);
		
		if((rc = ( size - s.Size )) != size)
			--rc;
	}
	
	return(rc);
}

LONG SNPrintf( STRPTR outbuf, LONG size, CONST_STRPTR fmt, ... )
{
	va_list args;
	long rc;
	
	va_start (args, fmt);
	rc = VSNPrintf( outbuf, size, fmt, args );
	va_end(args);
	
	return(rc);
}

/****************************************************************************/

/****************************************************************************/
/****************************************************************************/

size_t strlen(const char *string)
{ const char *s=string;
  
  if(!(string && *string))
  	return 0;
  
  do;while(*s++); return ~(string-s);
}

/****************************************************************************/

char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!*s++)
      { s = (char *)0; break; }
  return (char *)s;
}

/****************************************************************************/
