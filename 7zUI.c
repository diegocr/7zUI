/* ***** BEGIN LICENSE BLOCK *****
 * Version: BSD License
 * 
 * Copyright (c) 2006, Diego Casorran <dcasorran@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 ** Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  
 ** Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <proto/muimaster.h>
#include <proto/asl.h>
#include <MUI/BetterString_mcc.h>
#include <MUI/NList_mcc.h>
#include <MUI/NListview_mcc.h>

#include "7zMain.h"
#include "7zItem.h"
#include "7zUtils.h"
#include "7zUI.h"
#include "7zTime.h"
#include <SDI_mui.h>

/***************************************************************************/
#define VERSION		_7zUI_VERSION

struct Library * MUIMasterBase = NULL;

STATIC CONST STRPTR ApplicationData[] = {
	"7ZUI",						/* title / base */
	"$VER: 7zUI " VERSION " (10.09.2006)",		/* Version */
	"©2006, Diego Casorran",			/* Copyright */
	"7zUI " VERSION " - 7zip unarchiving tool",	/* Description */
	NULL
};

enum Actions
{
	ID_LIST = 0x44f789,
	ID_TEST,
	ID_EXTRACT,
};

STATIC VOID NListFormat( APTR obj, CONST_STRPTR align);

/***************************************************************************/

INLINE VOID ArchiveAction( ULONG action )
{
	BPTR WhereToExtract = 0, OldCurrentdir = 0;
	STRPTR archive = (STRPTR) xget( G->mui.arc, MUIA_String_Contents);
	
	if(!(archive && *archive))
	{
		DisplayBeep(NULL);
		return;
	}
	
	SetAttrs(G->mui.app, MUIA_Application_Sleep, TRUE );
	
	if( action == ID_EXTRACT )
	{
		struct Library * AslBase;
		
		if((AslBase = OpenLibrary("asl.library", 0)))
		{
			struct FileRequester *freq;
			
			if((freq = AllocAslRequestTags(ASL_FileRequest, TAG_END)))
			{
				if(AslRequestTags(freq,
					ASLFR_TitleText, (ULONG)"Select where to extract the archive",
					ASLFR_InitialDrawer, (ULONG)"PROGDIR:",
					ASLFR_DrawersOnly, TRUE,
				TAG_DONE))
					SwitchCurrentdir( freq->fr_Drawer, 
						&WhereToExtract,&OldCurrentdir);
				
				FreeAslRequest(freq);
			}
			
			CloseLibrary( AslBase );
		}
		
		if(!WhereToExtract)
		{
			DisplayBeep(NULL);
			goto done;
		}
	}
	else if( action == ID_LIST )
		DoMethod( G->mui.list, MUIM_NList_Clear );
	
	_7zArchive(archive,(action == ID_LIST) ? 1:((action == ID_TEST) ? 2:3));
	
	if( action == ID_EXTRACT && WhereToExtract )
		SwitchCurrentdir( NULL, &WhereToExtract, &OldCurrentdir );
	
done:
	SetAttrs(G->mui.app, MUIA_Application_Sleep, FALSE);
}

/***************************************************************************/
/***************************************************************************/

struct NList_Entry
{
	UBYTE name[512];
	UBYTE size[16];
	UBYTE crc[10];
	UBYTE date[64];
};

//---------------------------------------------- NList Construct Hook ------

#define CM( src, dst )	\
	CopyMem( src, dst, sizeof( dst )-1)

#define SNP( dst, fmt... )	\
	SNPrintf( dst, sizeof(dst)-1, fmt )

HOOKPROTONHNO( ConstructFunc, APTR, struct NList_ConstructMessage * msg )
{
	struct NList_Entry * entry;
	CFileItem * item = (CFileItem * ) msg->entry;
	
	if((entry = AllocVec(sizeof(struct NList_Entry), MEMF_ANY )))
	{
		ULONG time;
		
		CM( item->Name, entry->name );
		
		if(item->IsDirectory)
			CM( "\033r\033I[6:22]", entry->size );
		else
			SNP( entry->size, "\033r%ld", item->Size );
		
		SNP(entry->crc,"%08lx",item->IsFileCRCDefined?item->FileCRC:0);
		
		time = ItemTime( item, entry->date, sizeof(entry->date), TRUE);
		
	//	Printf("%s,%ld,%lx %ld,%lx\n",
	//		item->Name, item->Size, item->FileCRC, time,time );
	}
	return (APTR) entry;
}
MakeStaticHook( ConstructHook, ConstructFunc );

//---------------------------------------------- NList Destruct Hook -------

HOOKPROTONHNO( DestructFunc, VOID, struct NList_DestructMessage * msg )
{
	struct NList_Entry * entry;
	
	if((entry = (struct NList_Entry *) msg->entry))
		FreeVec( entry );
}
MakeStaticHook( DestructHook, DestructFunc );

//---------------------------------------------- NList Display Hook --------

HOOKPROTONHNO( DisplayFunc, VOID, struct NList_DisplayMessage * msg )
{
	struct NList_Entry * entry = (struct NList_Entry *) msg->entry;
	
	if( entry == NULL )
	{
		msg->strings[0] = "Name";
		msg->strings[1] = "Size";
		msg->strings[2] = "CRC";
		msg->strings[3] = "Date";
	}
	else
	{
		msg->strings[0] = entry->name;
		msg->strings[1] = entry->size;
		msg->strings[2] = entry->crc;
		msg->strings[3] = entry->date;
	}
}
MakeStaticHook( DisplayHook, DisplayFunc );

/***************************************************************************/

int _7zUI( VOID )
{
	int rc = 1;
	Object * list, * test, * extract;
	
	if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME,MUIMASTER_VMIN)))
	{
		//PrintError("This program requires MUI, Alternativaly you can use the CLI Arguments");
		return rc;
	}
	
	G->mui.app = ApplicationObject,
		MUIA_Application_Title      , ApplicationData[0],
		MUIA_Application_Version    , ApplicationData[1],
		MUIA_Application_Copyright  , ApplicationData[2],
		MUIA_Application_Author     , ApplicationData[2] + 7,
		MUIA_Application_Description, ApplicationData[3],
		MUIA_Application_Base       , ApplicationData[0],
		
		SubWindow, G->mui.win = WindowObject,
			MUIA_Window_Title, ApplicationData[3],
			MUIA_Window_ID, MAKE_ID('7','Z','U','I'),
			WindowContents, VGroup,
				MUIA_Group_SameWidth, TRUE,
				Child, HGroup,
					Child, list = MUI_MakeObject(MUIO_Button, "List Contents"),
					Child, test = MUI_MakeObject(MUIO_Button, "Test Archive"),
					Child, extract = MUI_MakeObject(MUIO_Button, "Extract Archive"),
					Child, HVSpace,
				End,
				Child, ColGroup(2), GroupFrame,
					MUIA_Background, MUII_GroupBack,
					Child, Label2("Archive:"),
					Child, G->mui.arc = PopaslObject,
						MUIA_Popstring_String, BetterStringObject, StringFrame,
							MUIA_String_AdvanceOnCR		, TRUE,
							MUIA_String_MaxLen		, 1024,
							MUIA_ObjectID			, MAKE_ID('7','A','R','C'),
						End,
						MUIA_Popstring_Button, PopButton(MUII_PopFile),
						ASLFR_TitleText, "Select 7z archive",
						ASLFR_InitialPattern, "#?.7z",
						ASLFR_DoPatterns, TRUE,
					End,
				End,
				Child, NListviewObject,
					MUIA_NListview_Horiz_ScrollBar, MUIV_NListview_HSB_FullAuto,
					MUIA_NListview_NList, G->mui.list = NListObject,
						MUIA_NList_AutoVisible,		TRUE,
						MUIA_NList_TitleSeparator,	TRUE,
						MUIA_NList_Title,		TRUE,
						MUIA_NList_Input,		FALSE,
						MUIA_NList_DefaultObjectOnClick,TRUE,
						MUIA_NList_MinColSortable,	0,
						MUIA_NList_Imports,		MUIV_NList_Imports_All,
						MUIA_NList_Exports,		MUIV_NList_Exports_All,
						MUIA_NList_ConstructHook2,	(ULONG) &ConstructHook,
						MUIA_NList_DestructHook2,	(ULONG) &DestructHook,
						MUIA_NList_DisplayHook2,	(ULONG) &DisplayHook,
					//	MUIA_NList_CompareHook2,	(ULONG) &CompareHook,
					End,
				End,
				Child, G->mui.gauge = GaugeObject,
					GaugeFrame, 
					MUIA_FixHeightTxt, "M",
					MUIA_Gauge_Horiz, TRUE,
				End,
			End,
		End,
	End;
	
	if( ! G->mui.app ) {
		PrintError("Failed creating MUI Application!");
		goto done;
	}
	
	DoMethod(G->mui.win, MUIM_Notify,MUIA_Window_CloseRequest, TRUE,
		G->mui.app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
	
	DoMethod( list, MUIM_Notify, MUIA_Pressed, FALSE, G->mui.app, 2, MUIM_Application_ReturnID, ID_LIST );
	DoMethod( test, MUIM_Notify, MUIA_Pressed, FALSE, G->mui.app, 2, MUIM_Application_ReturnID, ID_TEST );
	DoMethod( extract, MUIM_Notify, MUIA_Pressed, FALSE, G->mui.app, 2, MUIM_Application_ReturnID, ID_EXTRACT );
	
	DoMethod( G->mui.arc, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		G->mui.app, 2, MUIM_Application_ReturnID, ID_LIST );
	
	DoMethod( G->mui.app, MUIM_Application_Load, MUIV_Application_Load_ENVARC );
	
	NListFormat(G->mui.list, "lrcl");
	SetAttrs(G->mui.win,MUIA_Window_Open,TRUE,TAG_DONE);
	
	if(xget( G->mui.win, MUIA_Window_Open))
	{
		ULONG sigs = 0, mid;
		struct Window * window;
		BOOL stop = FALSE;
		
		window = (struct Window *)xget(G->mui.win, MUIA_Window_Window);
		if(window)
			((struct Process *)FindTask(0))->pr_WindowPtr = window;
		
		do {
			mid = DoMethod(G->mui.app,MUIM_Application_NewInput,&sigs);
			
			switch( mid )
			{
				case MUIV_Application_ReturnID_Quit:
					stop = TRUE;
					break;
				
				case ID_EXTRACT:
				case ID_TEST:
				case ID_LIST:
					ArchiveAction( mid );
					break;
				
				default:
					break;
			}
			if(sigs && !stop)
			{
				sigs = Wait(sigs | SIGBREAKF_CTRL_C);
				if (sigs & SIGBREAKF_CTRL_C) break;
			}
		} while(!stop);
		
		SetAttrs(G->mui.win,MUIA_Window_Open,FALSE,TAG_DONE);
		DoMethod( G->mui.app, MUIM_Application_Save, MUIV_Application_Save_ENVARC );
		
		rc = 0;
	}
	else PrintError("Failed opening MUI Window!");
	
done:
	if(G->mui.app)
		MUI_DisposeObject(G->mui.app);
	if (MUIMasterBase)
		CloseLibrary(MUIMasterBase);
	
	return(rc);
}

/***************************************************************************/
/***************************************************************************/
//------ 7zUI related functions --------------------------------------------

VOID GaugeUpdate( STRPTR text, ULONG pos )
{
	SetAttrs( G->mui.gauge,
		MUIA_Gauge_InfoText, (ULONG) text,
		MUIA_Gauge_Current, pos,
	TAG_DONE );
}

/***************************************************************************/

VOID NListInsert( APTR entry )
{
	DoMethod( G->mui.list, MUIM_NList_InsertSingle, 
		entry, MUIV_NList_Insert_Sorted );
}

/***************************************************************************/

STATIC VOID NListFormat( APTR obj, CONST_STRPTR align)
{
	UBYTE format[1024], *a=(char *)align, *ptr=format;
	*format=0;
	
	do {
		ptr += SNPrintf( ptr, 20, "BAR W=%ld P=\033%lc,", -1, (*a));
		
	} while(*(++a));
	
	*ptr++ = 'B'; *ptr++ = 'A'; *ptr++ = 'R'; *ptr = 0;
	
 //	DBG_HEAVY("NList Format to Object 0x%08lx: \"%s\"\n", obj, format );
	
	set( obj, MUIA_NList_Format, (ULONG) format);
}

/***************************************************************************/

VOID ShowProgramInfo( VOID )
{
	Printf("\n%s, %s\nUsing LZMA SDK %s Copyright (c) 1999-2006 Igor "
		"Pavlov\n", (long)ApplicationData[3], 
			(long)ApplicationData[2],(long)_7zSDK_VERSION );
}

/***************************************************************************/

