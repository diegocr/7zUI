/* 7zDecode.c */

#include "7zDecode.h"
#include "LzmaDecode.h"
#include "7zUtils.h" // SNPrintf, PrintError
#include "7zDebug.h"

/*****************************************************************************/

STATIC CMethodID k_Copy		= { { 0x0 }, 1 };
STATIC CMethodID k_LZMA		= { { 0x3, 0x1, 0x1 }, 3 };
STATIC CMethodID k_PPMD		= { { 0x3, 0x4, 0x1 }, 3 };
STATIC CMethodID k_BCJ_X86	= { { 0x3, 0x3, 0x1, 0x3 }, 4 };
STATIC CMethodID k_BCJ2		= { { 0x3, 0x3, 0x1, 0x1B }, 4 };
STATIC CMethodID k_Deflate	= { { 0x4, 0x1, 0x8 }, 3 };
STATIC CMethodID k_BZip2	= { { 0x4, 0x2, 0x2 }, 3 };
STATIC CMethodID k_7zAES	= { { 0x6, 0xF1, 0x07, 0x01 }, 4 };

VOID ReportUnsuportedMethod( CCoderInfo *coder )
{
	UBYTE msg[128], * which = NULL;
	
	static const struct {
		CMethodID * mid;
		STRPTR ident;
	} mids[] = {
		{ &k_PPMD     ,	"PPMD"    },
		{ &k_BCJ_X86  ,	"BCJx86"  },
		{ &k_BCJ2     ,	"BCJ2"    },
		{ &k_Deflate  ,	"Deflate" },
		{ &k_BZip2    ,	"BZip2"   },
		{ &k_7zAES    ,	"7zAES"   },
		{ NULL, NULL },
	}, *ptr=mids;
	
	while(ptr->ident != NULL)
	{
		if(AreMethodsEqual(&coder->MethodID, ptr->mid))
		{
			which = ptr->ident;
			break;
		}
		ptr++;
	}
	
	if( which != NULL )
	{
		SNPrintf( msg, sizeof(msg)-1,
			"No decoder found for 7z-%s", which );
	}
	else CopyMem("unknown archive format", msg, sizeof(msg)-1);
	
	PrintError( msg );
}

/*****************************************************************************/

#ifdef _LZMA_IN_CB

typedef struct _CLzmaInCallbackImp
{
  ILzmaInCallback InCallback;
  ISzInStream *InStream;
  size_t Size;
} CLzmaInCallbackImp;

int LzmaReadImp(void *object, const unsigned char **buffer, SizeT *size)
{
  CLzmaInCallbackImp *cb = (CLzmaInCallbackImp *)object;
  size_t processedSize;
  SZ_RESULT res;
  *size = 0;
  res = cb->InStream->Read((void *)cb->InStream, (void **)buffer, cb->Size, &processedSize);
  *size = (SizeT)processedSize;
  if (processedSize > cb->Size)
    return (int)SZE_FAIL;
  cb->Size -= processedSize;
  if (res == SZ_OK)
    return 0;
  return (int)res;
}

#endif

/*****************************************************************************/

SZ_RESULT SzDecode(const CFileSize *packSizes, const CFolder *folder,
    #ifdef _LZMA_IN_CB
    ISzInStream *inStream,
    #else
    const Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, 
    size_t *outSizeProcessed, ISzAlloc *allocMain)
{
  UInt32 si;
  size_t inSize = 0;
  CCoderInfo *coder;
  //DBG_VALUE(folder->NumPackStreams);
  //DBG_VALUE(folder->NumCoders);
  if (folder->NumPackStreams != 1)
  {
    PrintError("archive has unsupported number of packet streams");
    return SZE_NOTIMPL;
  }
  if (folder->NumCoders != 1)
  {
    PrintError("archive has unsupported number of coders");
    return SZE_NOTIMPL;
  }
  coder = folder->Coders;
  *outSizeProcessed = 0;

  for (si = 0; si < folder->NumPackStreams; si++)
    inSize += (size_t)packSizes[si];

  if (AreMethodsEqual(&coder->MethodID, &k_Copy))
  {
    size_t i;
    if (inSize != outSize)
      return SZE_DATA_ERROR;
    #ifdef _LZMA_IN_CB
    for (i = 0; i < inSize;)
    {
      size_t j;
      Byte *inBuffer;
      size_t bufferSize;
      RINOK(inStream->Read((void *)inStream,  (void **)&inBuffer, inSize - i, &bufferSize));
      if (bufferSize == 0)
        return SZE_DATA_ERROR;
      if (bufferSize > inSize - i)
        return SZE_FAIL;
      *outSizeProcessed += bufferSize;
      for (j = 0; j < bufferSize && i < inSize; j++, i++)
        outBuffer[i] = inBuffer[j];
    }
    #else
    #if 0
    for (i = 0; i < inSize; i++)
      outBuffer[i] = inBuffer[i];
    #else
    	CopyMem((APTR) inBuffer,(APTR) outBuffer, inSize );
    #endif
    *outSizeProcessed = inSize;
    #endif
    return SZ_OK;
  }

  if (AreMethodsEqual(&coder->MethodID, &k_LZMA))
  {
    #ifdef _LZMA_IN_CB
    CLzmaInCallbackImp lzmaCallback;
    #else
    SizeT inProcessed;
    #endif

    CLzmaDecoderState state;  /* it's about 24-80 bytes structure, if int is 32-bit */
    int result;
    SizeT outSizeProcessedLoc;

    #ifdef _LZMA_IN_CB
    lzmaCallback.Size = inSize;
    lzmaCallback.InStream = inStream;
    lzmaCallback.InCallback.Read = LzmaReadImp;
    #endif

    if (LzmaDecodeProperties(&state.Properties, coder->Properties.Items, 
        coder->Properties.Capacity) != LZMA_RESULT_OK)
      return SZE_FAIL;

    state.Probs = (CProb *)allocMain->Alloc(LzmaGetNumProbs(&state.Properties) * sizeof(CProb));
    if (state.Probs == 0)
      return SZE_OUTOFMEMORY;

    #ifdef _LZMA_OUT_READ
    if (state.Properties.DictionarySize == 0)
      state.Dictionary = 0;
    else
    {
      state.Dictionary = (unsigned char *)allocMain->Alloc(state.Properties.DictionarySize);
      if (state.Dictionary == 0)
      {
        allocMain->Free(state.Probs);
        return SZE_OUTOFMEMORY;
      }
    }
    LzmaDecoderInit(&state);
    #endif

    result = LzmaDecode(&state,
        #ifdef _LZMA_IN_CB
        &lzmaCallback.InCallback,
        #else
        inBuffer, (SizeT)inSize, &inProcessed,
        #endif
        outBuffer, (SizeT)outSize, &outSizeProcessedLoc);
    *outSizeProcessed = (size_t)outSizeProcessedLoc;
    allocMain->Free(state.Probs);
    #ifdef _LZMA_OUT_READ
    allocMain->Free(state.Dictionary);
    #endif
    if (result == LZMA_RESULT_DATA_ERROR)
      return SZE_DATA_ERROR;
    if (result != LZMA_RESULT_OK)
      return SZE_FAIL;
    return SZ_OK;
  }
  
  ReportUnsuportedMethod( coder );
  return SZE_NOTIMPL;
}
