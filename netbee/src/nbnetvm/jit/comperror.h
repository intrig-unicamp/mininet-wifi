/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

/** @file comperror.h
 * \brief This file contains prototypes, definitions and data structures for
 *  the compilation errors list management
 *
 */


#ifdef __cplusplus
extern "C" {
#endif


typedef enum{
	FLAG_NO_ERROR				= 0X00000000,
	FLAG_LST_STACK_OVFLOW		= 0x00000001,
	FLAG_LST_STACK_UNDERFLOW	= 0x00000002,
	FLAG_LST_STACK_EMPTY		= 0x00000004,
	FLAG_LST_LOCAL_UNREF		= 0x00000008,
	FLAG_LST_LOCAL_OUTOB		= 0x00000010,
	FLAG_OP_NOT_DEF				= 0x00000100,
	FLAG_BC_FALLOUT				= 0x00000200,
	FLAG_INVALID_ARGS_SZ		= 0x00000400,
	FLAG_INVALID_BR_TARGET		= 0x00000800,
	FLAG_STACK_MERGE			= 0x00001000,
	FLAG_OP_NOT_IMPLEMENTED		= 0x00002000,
	FLAG_INIT_OP_IN_OTHER_SEG	= 0x00004000,
	FLAG_END_OF_SEG_WO_RET		= 0x00008000,
	FLAG_WRONG_NUM_INSTR		= 0x00010000,
	FLAG_ANALYSIS_ERROR			= 0x10000000
}AnalysisErrorFlags;


#define WARNING_FLAGS	(FLAG_STACK_MERGE)

struct InstructionInfo;

typedef struct InstructionInfo InstructionInfo;

/*!
  \brief
 */

typedef struct _ANALYSIS_ERROR
{
	uint32_t	ErrFlags;
	InstructionInfo *InsnInfo;
	uint32_t	Byte;
	uint32_t	StackDepth;
	uint32_t	BasicBlock;
	struct _ANALYSIS_ERROR *next;
}AnalysisError;


typedef struct _ERROR_LIST
{
	AnalysisError *Head;
	AnalysisError *Tail;
} ErrorList;


/*
*	PROTOTYPES
*/

ErrorList * nvmJitNew_Error_List(void);
void Analysis_Error(ErrorList * errList, uint32_t errFlags, InstructionInfo *insnInfo, uint32_t byte, uint32_t stackDepth, uint32_t BB);

void nvmJitDump_Analysis_Errors(char *dstString, uint32_t size, ErrorList * errList);
void nvmJitFree_Error_List(ErrorList * errList);

#ifdef __cplusplus
}
#endif
