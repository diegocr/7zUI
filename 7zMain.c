/* 
7zMain.c
Test application for 7z Decoder
LZMA SDK 4.43 Copyright (c) 1999-2006 Igor Pavlov (2006-06-04)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#include <proto/intuition.h>

#include "7zCrc.h"
#include "7zIn.h"
#include "7zExtract.h"
#include "7zMain.h"
#include "7zUtils.h"
#include "7zUI.h"

struct Library * DOSBase = NULL, * IntuitionBase = NULL;
struct GlobalData * G = NULL;
GLOBAL APTR _WBenchMsg;

typedef struct _CFileInStream
{
  ISzInStream InStream;
  BPTR File;
} CFileInStream;

/****************************************************************************/

#define TEMPLATE	"ARCHIVE/A,TO/K,LIST=L/S,TEST=T/S,EXTRACT=E/S"
#define ARCHIVE_ARG	((STRPTR)arg[0])
#define TOFOLDER	((STRPTR)arg[1])
#define LIST_ARG	arg[2]
#define TEST_ARG	arg[3]
#define EXTRACT_ARG	arg[4]

int main( VOID )
{
	LONG arg[5] = {0};
	struct RDArgs *args = NULL;
	int rc = 10;
	
	if((G = (struct GlobalData*)AllocMem(sizeof(struct GlobalData),MEMF_PUBLIC|MEMF_CLEAR)))
	{
		/* Initialize 'GlobalData' */
		
		G->WbStartup = ((_WBenchMsg != NULL) ? TRUE : FALSE);
		
		if((IntuitionBase = OpenLibrary("intuition.library", 0)))
		{
			if((DOSBase = OpenLibrary("dos.library", 0)))
			{
				args = ReadArgs( TEMPLATE, (long*)arg, NULL);
				if(!args)
				{
					if((rc = _7zUI ( )) != 0 )
					{
						/* failed creating MUI App */
						
						if( G->WbStartup )
							DisplayBeep(NULL);
						
						PrintFault(IoErr(),0L);
					}
				}
				else
				{
					int mode = 1;
					BPTR WhereToExtract = 0, 
						OldCurrentdir = 0;
					
					G->CliUsage = TRUE;
					
					if( TOFOLDER )
					{
						MakeDir( TOFOLDER );
						
						mode=SwitchCurrentdir(TOFOLDER,
							&WhereToExtract, 
							&OldCurrentdir );
					}
					
					if( mode )
					{
						mode = LIST_ARG ? 1 : TEST_ARG ? 2:3;
						
						rc = _7zArchive( ARCHIVE_ARG, mode );
						
						if( TOFOLDER )
						  SwitchCurrentdir(NULL,
							&WhereToExtract,
							&OldCurrentdir);
					}
					
					FreeArgs( args );
				}
				
				CloseLibrary( DOSBase );
			}
			
			CloseLibrary( IntuitionBase );
		}
		
		FreeMem( G, sizeof(struct GlobalData));
	}
	
	return rc;
}
void __main(void){}

/****************************************************************************/
/****************************************************************************/

#ifdef _LZMA_IN_CB

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];

SZ_RESULT SzFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc;
  if (maxRequiredSize > kBufferSize)
    maxRequiredSize = kBufferSize;
  processedSizeLoc = Read(s->File, g_Buffer, maxRequiredSize );
  *buffer = g_Buffer;
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#else

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = Read(s->File, buffer, size );
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#endif

/****************************************************************************/

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;
  LONG res = Seek(s->File, (long)pos, OFFSET_BEGINING);
  if (res != -1)
    return SZ_OK;
  return SZE_FAIL;
}

/****************************************************************************/

STATIC STRPTR SzErrorString( SZ_RESULT errno )
{
	switch( errno )
	{
		case SZE_DATA_ERROR:return "Data Error";
		case SZE_OUTOFMEMORY:return "Cannot allocate memory";
		case SZE_CRC_ERROR:return "CRC Error";
		case SZE_NOTIMPL:return "unimplemented decoder instruction";
		case SZE_FAIL:return "failed assertion";
		case SZE_ARCHIVE_ERROR:return "Archive Error";
		default:break;
	}
	return "unknown error";
}

/****************************************************************************/
/**
 * ::7zArchive read and process a .7z archive
 * 
 * @filename: 7z archive to load
 * @mode: 1 = LIST, 2 = TEST, 3 = EXTRACT
 */

int _7zArchive( STRPTR filename, int mode )
{
  CFileInStream archiveStream;
  CArchiveDatabaseEx db;
  SZ_RESULT res;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;
  
  archiveStream.File = Open( filename, MODE_OLDFILE);
  if (!archiveStream.File)
  {
    PrintError("Unable to open archive!");
    goto done;
  }

  archiveStream.InStream.zRead = SzFileReadImp;
  archiveStream.InStream.zSeek = SzFileSeekImp;

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;

  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  InitCrcTable();
  SzArDbExInit(&db);
  res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);
  if (res == SZ_OK)
  {
    if( G->CliUsage )
    {
      ShowProgramInfo ( );
      Printf("\n%s archive %s\n",
        (long)((mode == 1) ? "Listing":((mode == 2) ? "Testing":"Extracting")),
        (long) FilePart( filename ));
    }
    
    if ( mode == 1 ) // LIST mode
    {
      UInt32 i;
      
      if( G->CliUsage )
      {
        Printf("\n%7s%9s%14s%7s%8s\n",(long)"Date",(long)"Time",(long)"Size",(long)"CRC",(long)"Name");
        for( i = 0; i < 60 ; i++ )
          PutStr("-");
        PutStr("\n");
      }
      
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        CFileItem *f = db.Database.Files + i;
        
        if( G->CliUsage )
        {
          UBYTE date[20];
          
          ItemTime( f, date, sizeof(date), FALSE );
          
          Printf("%s%11ld  %08lx  %s\n", (long)date, (long)f->Size,
          	f->IsFileCRCDefined ? f->FileCRC:0, (long)f->Name);
        }
        else
          NListInsert((APTR) f );
      }
    }
    else
    {
      UInt32 i;

      // if you need cache, use these 3 variables.
      // if you use external function, you can make these variable as static.
      UInt32 blockIndex = 0xFFFFFFFF; // it can have any value before first call (if outBuffer = 0) 
      Byte *outBuffer = 0; // it must be 0 before first call for each new archive. 
      size_t outBufferSize = 0;  // it can have any value before first call (if outBuffer = 0) 
      
      if( G->CliUsage )
        PutStr("\n");
      
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        size_t offset;
        size_t outSizeProcessed;
        CFileItem *f = db.Database.Files + i;
        
        if( G->CliUsage )
        {
          #if 0
          if (f->IsDirectory)
            PutStr("Directory");
          else
            Printf("%12s",(long)((mode == 2) ? "Testing":"Extracting"));
          Printf(" %s", (long)f->Name);
          #else
          if (!f->IsDirectory)
            Printf("%12s %s",(long)((mode == 2) ? "Testing":"Extracting"), (long)f->Name);
          #endif
        }
        else
        {
          STATIC UBYTE msg[256];
          
          SNPrintf( msg, sizeof(msg)-1, "%12s %s",
            (mode == 2) ? "Testing":"Extracting", f->Name );
          
          GaugeUpdate( msg, i*100/db.Database.NumFiles );
        }
        if (f->IsDirectory)
        {
        //  if( G->CliUsage )
        //    PutStr("\n");
          continue;
        }
        res = SzExtract(&archiveStream.InStream, &db, i, 
            &blockIndex, &outBuffer, &outBufferSize, 
            &offset, &outSizeProcessed, 
            &allocImp, &allocTempImp);
        if (res != SZ_OK)
          break;
        if ( mode == 3 ) // EXTRACT mode
        {
          BPTR outputHandle;
          UInt32 processedSize;
          
          MakeDir( f->Name ); // creates ALL Directories pointing to this file
          
          outputHandle = Open( f->Name, MODE_NEWFILE );
          if (outputHandle == 0)
          {
            PrintError("Unable to open output file!");
            res = SZE_FAIL;
            break;
          }
          processedSize = Write(outputHandle, outBuffer + offset, outSizeProcessed);
          if (processedSize != outSizeProcessed)
          {
            PrintError("Cannot write to output file!");
            res = SZE_FAIL;
            break;
          }
          Close(outputHandle);
          SetFileTimeToFile( f );
        }
        if( G->CliUsage )
          PutStr("\n");
      }
      allocImp.Free(outBuffer);
    }
  }
  SzArDbExFree(&db, allocImp.Free);

  Close(archiveStream.File);
  if (res == SZ_OK)
  {
    if( G->CliUsage )
      Printf("\n%s\n", (long)"Everything is Ok");
    else
      GaugeUpdate("Everything is Ok", 0 );
    return 0;
  }
#if 0
  if (res == SZE_OUTOFMEMORY)
    PrintError("can not allocate memory");
  else if( G->CliUsage )
    Printf("\nERROR #%ld\n", res);
#else
  PrintError(SzErrorString( res ));
#endif
  if( ! G->CliUsage )
    GaugeUpdate("error %ld procesing archive", res );
    
done:
  return 1;
}
