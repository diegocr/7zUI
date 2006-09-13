/**
 * $Id: startup.c,v 0.1 2006/07/06 23:39:43 diegocr Exp $
 */

#define __NOLIBBASE__
#include <proto/exec.h>
#include <dos/dosextens.h>

/***************************************************************************/

#define STACK_SIZE	262144

GLOBAL int __swap_stack_and_call(struct StackSwapStruct * stk,APTR function);
GLOBAL int main ( VOID );

struct Library * SysBase = NULL;
struct WBStartup * _WBenchMsg = NULL;

/***************************************************************************/

int __startup_function_which_calls_main_without_arguments ( void )
{
	unsigned int stack_size, __stack_size = STACK_SIZE ;
	struct Process * this_process;
	struct StackSwapStruct *stk;
	APTR old_window_pointer;
	struct Task *this_task;
	APTR new_stack;
	int ret = 1;
	
	SysBase = *(struct Library **) 4L;
	
	this_process = (struct Process *)(this_task = FindTask(NULL));
	
	if(!this_process->pr_CLI)
	{
		struct MsgPort * mp = &this_process->pr_MsgPort;
		
		WaitPort(mp);
		
		_WBenchMsg = (struct WBStartup *)GetMsg(mp);
	}
	else	_WBenchMsg = NULL;
	
	old_window_pointer = this_process->pr_WindowPtr;
	
	__stack_size += ((ULONG)this_task->tc_SPUpper-(ULONG)this_task->tc_SPLower);
	
	/* Make the stack size a multiple of 32 bytes. */
	stack_size = 32 + ((__stack_size + 31UL) & ~31UL);
	
	/* Allocate the stack swapping data structure
	   and the stack space separately. */
	stk = (struct StackSwapStruct *) AllocVec( sizeof(*stk), MEMF_PUBLIC|MEMF_ANY );
	if(stk != NULL)
	{
		new_stack = AllocMem(stack_size,MEMF_PUBLIC|MEMF_ANY);
		if(new_stack != NULL)
		{
			/* Fill in the lower and upper bounds, then
			   take care of the stack pointer itself. */
			
			stk->stk_Lower	= new_stack;
			stk->stk_Upper	= (ULONG)(new_stack)+stack_size;
			stk->stk_Pointer= (APTR)(stk->stk_Upper - 32);
			
			ret = __swap_stack_and_call(stk,(APTR)main);
			
			FreeMem(new_stack, stack_size);
		}
		
		FreeVec(stk);
	}
	
	this_process->pr_WindowPtr = old_window_pointer;
	
	if(_WBenchMsg != NULL)
	{
		Forbid();
		
		ReplyMsg((struct Message *)_WBenchMsg);
	}
	
	return ret;
}

