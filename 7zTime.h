#ifndef __7ZTIME_H
#define __7ZTIME_H

#include <exec/types.h>
#include <dos/dos.h>

#define UTOF	252460800

typedef struct {
	ULONG dwHighDateTime;
	ULONG dwLowDateTime;
} CArchiveFileTime;

GLOBAL VOID IntToDateStamp(ULONG secs, struct DateStamp * ds );
GLOBAL STRPTR IntToDateStr(ULONG secs,STRPTR buffer,ULONG bufferLen,BOOL wday);
GLOBAL VOID UnixTimeToFileTime(ULONG unixTime, CArchiveFileTime * fileTime );
GLOBAL BOOL FileTimeToUnixTime(CArchiveFileTime * fileTime, ULONG *unixTime);


#endif /* __7ZTIME_H */
