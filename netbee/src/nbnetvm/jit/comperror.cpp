/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




/** @file comperror.c
 * \brief This file contains the functions for compilation error tracking facilities
 *
 */



#include <stdlib.h>

#include <stdio.h>
#include <string.h>

#include <bytecode_analyse.h>
#include "netvmjitglobals.h"
#include "../../nbee/globals/debug.h"
#include "comperror.h"
#include <sstream>


ErrorList * nvmJitNew_Error_List(void)
{
	ErrorList * errList;

	errList = (ErrorList*)CALLOC(1, sizeof (ErrorList));
	if (errList == NULL){
		return NULL;
	}

	errList->Head = NULL;
	errList->Tail = NULL;

	return errList;
}




void nvmJitFree_Error_List(ErrorList * errList)
{
	AnalysisError *err, *next;

	if (errList == NULL)
		return;

	err = errList->Head;
	while (err != NULL){
		next = err->next;
		free(err);
		err = next;
	}

	free(errList);
}



void Analysis_Error(ErrorList * errList, uint32_t errFlags, InstructionInfo *insnInfo, uint32_t byte, uint32_t stackDepth, uint32_t BB)
{
	AnalysisError * err;

	NETVM_ASSERT(errList != NULL, " Null Argument");

	err = (AnalysisError*)CALLOC(1, sizeof(AnalysisError));
	NETVM_ASSERT(err != NULL, " Memory Allocation Failure");

	err->ErrFlags = errFlags;
	err->InsnInfo = insnInfo;
	err->Byte = byte;
	err->StackDepth = stackDepth;
	err->BasicBlock = BB;
	err->next = NULL;

	if (errList->Head == NULL || errList->Tail == NULL){
		errList->Head = err;
		errList->Tail = err;
	}
	else{
		errList->Tail->next = err;
		errList->Tail = err;
	}

}

std::string DumpSrcLine(AnalysisError *err)
{
	std::stringstream ss;
	if (err->InsnInfo == NULL)
	{
		ss << "byte " << err->Byte;
	}
	else if (err->InsnInfo->SrcLine == 0)
	{
		ss << "byte " << err->InsnInfo->BCpc;
	}
	else
		ss << "source line " << err->InsnInfo->SrcLine;
	return ss.str();
}

void nvmJitDump_Analysis_Errors(char *dstString, uint32_t size, ErrorList * errList)
{
	AnalysisError * err;
	std::stringstream ss;
	err = errList->Head;

	while (err != NULL){
		if (err->ErrFlags & FLAG_WRONG_NUM_INSTR)
			ss << "Wrong number of instructions in segment" << std::endl;
		else if (err->ErrFlags & FLAG_END_OF_SEG_WO_RET)
			ss << "Unexpected end of segment without RET or Branch at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_STACK_MERGE)
			ss << "Cannot merge stack depths in basic blocks at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_OP_NOT_DEF)
			ss << "OpCode not defined at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_BC_FALLOUT)
			ss << "Reached bytecode end parsing the last instruction" << std::endl;
		else if (err->ErrFlags & FLAG_INVALID_ARGS_SZ)
			ss << "Invalid argument at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_INVALID_BR_TARGET)
			ss << "Invalid branch target at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_LST_STACK_OVFLOW)
			ss << "Stack overflow at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_LST_STACK_UNDERFLOW)
			ss << "Stack underflow at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_LST_STACK_EMPTY)
			ss << "Empty stack at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_LST_LOCAL_UNREF)
			ss << "Access to uninitialized local at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_LST_LOCAL_OUTOB)
			ss << "Local's index out of boundaries at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_OP_NOT_IMPLEMENTED)
			ss << "Opcode not implemented at " << DumpSrcLine(err) << std::endl;
		else if (err->ErrFlags & FLAG_INIT_OP_IN_OTHER_SEG)
			ss << "Initialization opcode in other segment at " << DumpSrcLine(err) << std::endl;
		err = err->next;
	}
	snprintf(dstString, size-1, "%s", ss.str().c_str());
}
