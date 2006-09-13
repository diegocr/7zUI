#ifndef __7ZMAIN_H
#define __7ZMAIN_H

#include <exec/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct GlobalData
{
	BOOL WbStartup;
	BOOL CliUsage;
	
	struct {
		Object * app;
		Object * win;
		Object * list;
		Object * arc;
		Object * gauge;
	} mui;
	
};

GLOBAL struct GlobalData * G;

GLOBAL int _7zArchive( STRPTR filename, int mode );
GLOBAL VOID ShowProgramInfo( VOID );

#ifdef __cplusplus
}
#endif
#endif /* __7ZMAIN_H */
