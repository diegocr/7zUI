
#include <proto/exec.h>
#include <proto/dos.h>
#include "7zUtils.h"
#include "7zTime.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UInt32	ULONG
#define UInt64	unsigned long long

static const UInt32 kNumTimeQuantumsInSecond = 10000000;

#if 0
static const UInt64 kUnixTimeStartValue = 
	((UInt64)kNumTimeQuantumsInSecond) * 60 * 60 * 24 * 134774;
#else
# define kUnixTimeStartValue	\
	(((UInt64)kNumTimeQuantumsInSecond) * 60 * 60 * 24 * 134774)
#endif

/****************************************************************************/

VOID IntToDateStamp(ULONG secs, struct DateStamp * ds )
{
	ds->ds_Days   = (secs / 86400);
	ds->ds_Minute = ((secs % 86400) / 60);
	ds->ds_Tick   = (((secs % 86400) % 60) * TICKS_PER_SECOND);
}

/****************************************************************************/

STRPTR IntToDateStr(ULONG secs, STRPTR buffer, ULONG bufferLen, BOOL wday )
{
	UBYTE timeStr[LEN_DATSTRING];
	UBYTE dateStr[LEN_DATSTRING];
	UBYTE dayStr[LEN_DATSTRING];
	struct DateTime dt;
	
	IntToDateStamp(secs, &dt.dat_Stamp);
	dt.dat_Format  = FORMAT_DOS;
	dt.dat_Flags   = 0;
	dt.dat_StrDay  = dayStr;
	dt.dat_StrDate = dateStr;
	dt.dat_StrTime = timeStr;
	DateToStr(&dt);
	
	if( wday )
		SNPrintf( buffer, bufferLen-1, "%s, %s %s ", dayStr, dateStr, timeStr);
	else
		SNPrintf( buffer, bufferLen-1, "%s %s ", dateStr, timeStr);
	
	return buffer;
}

/****************************************************************************/

VOID UnixTimeToFileTime(UInt32 unixTime, CArchiveFileTime * fileTime )
{
  UInt64 v = kUnixTimeStartValue + ((UInt64)unixTime) * kNumTimeQuantumsInSecond;
  fileTime->dwLowDateTime = (ULONG)v;
  fileTime->dwHighDateTime = (ULONG)(v >> 32);
}

/****************************************************************************/

BOOL FileTimeToUnixTime(CArchiveFileTime * fileTime, UInt32 * unixTime )
{
  UInt64 winTime = (((UInt64)fileTime->dwHighDateTime) << 32) + fileTime->dwLowDateTime;
  if (winTime < kUnixTimeStartValue)
  {
    unixTime = 0;
    return FALSE;
  }
  winTime = (winTime - kUnixTimeStartValue) / kNumTimeQuantumsInSecond;
  if (winTime > 0xFFFFFFFF)
  {
    (*unixTime) = 0xFFFFFFFF;
    return FALSE;
  }
  (*unixTime) = (UInt32)winTime;
  return TRUE;
}

