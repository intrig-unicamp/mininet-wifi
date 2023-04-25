/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




/** @file bytecode_analyse.c
 * \brief This file contains the functions that perform bytecode analysis and verification of a NetVM segment
 *
 */

#include <stdlib.h>


#include <stdio.h>

#ifndef WIN32
#include <string.h>
#endif

#include "bytecode_analyse.h"
#include "netvmjitglobals.h"
#include "../opcodes.h"			// NetVM opcodes definitions
#include "codetable.h"
#include "bytecode_segments.h"
#include "comperror.h"
#include "../../nbee/globals/debug.h"
#include <iostream>



//!Macro that set the stack pointer to its initial value (different between .init and .push or .pull segments)
#define SET_INITIAL_STACK(SP, N)	(SP = N)	//set stack pointer to N


//!Number of elements that an instruction pushes to or pops from the stack
enum{
	NO_STACK_ELEMENTS		= 0,
	ONE_STACK_ELEMENT		= 1,
	TWO_STACK_ELEMENTS		= 2,
	THREE_STACK_ELEMENTS	= 3
};

//!Macro that updtates PC to point to the next instruction in the bytecode:
#define NEXT_INSTR(PC)		(pc + GetInstructionLen(bytecode[PC]))



#define MAX(A,B) ( ( (A) > (B) ) ? (A) : (B) )




/*!
  \brief Instruction Indexes
 */


typedef struct InstrIndexes
{
	uint32_t *InstrIndexesArray;	//!< Array of the instruction indexes
	uint32_t NumInstructions;		//!< Number of NetIL instructions in the segment
	uint32_t SegmentLength;		//!< Bytecode Segment Length

} InstrIndexes;





InstrIndexes * New_Instr_Indexes(nvmByteCodeSegment * segment, ErrorList * errList);
void Free_Instr_Indexes(InstrIndexes * InsIndexes);




/*!
  \brief  adds to the current stack depth numOfElements stack elements as a PUSH-like
	  operation would do. Returns the eventual stack overflow tested on StackLimit

  \param  sp:				pointer to the current stack depth
  \param  StackLimit:		maximum stack length
  \param  numElements:		number of elements to be PUSHed onto the stack

  \return local/stack access flags
*/


uint32_t Stack_PUSH(int32_t *sp, int32_t StackLimit, uint32_t numElements)
{

	(*sp) += numElements;

	if ((*sp) >= StackLimit)
		return FLAG_LST_STACK_OVFLOW;

	return FLAG_NO_ERROR;

}




/*!
  \brief  subtract to the current stack depth numOfElements stack elements as a POP-like
	  operation would do. Returns the eventual stack underflow or pop from empty stack errors

  \param  sp:				pointer to the current stack depth
  \param  consumedElements:	number of elements to be POPed from the stack

  \return local/stack access flags
*/


uint32_t Stack_POP(int32_t *sp, uint32_t consumedElements)
{

	if ((*sp) == 0)
		return FLAG_LST_STACK_EMPTY;

	if ((*sp) < (int32_t)consumedElements){

		return FLAG_LST_STACK_UNDERFLOW;
	}
	(*sp) -= consumedElements;

	return FLAG_NO_ERROR;	// OK
}




/*!
  \brief  increment the current stack depth as a DUP-like operation would do. Returns the eventual
	  dup from empty stack or stack overflow errors

  \param  sp:				pointer to the current stack depth
  \param  StackLimit:		maximum stack length

  \return local/stack access flags
*/


uint32_t Stack_DUP(int32_t *sp, int32_t StackLimit)
{
	if ((*sp) == 0)
		return FLAG_LST_STACK_EMPTY;

	(*sp) += ONE_STACK_ELEMENT;

	if ((*sp) >= StackLimit)
		return FLAG_LST_STACK_OVFLOW;

	return FLAG_NO_ERROR;	// OK
}




/*!
  \brief  Simulates a stack POP and a locals definition reference. Returns the result of the POP and the
	  eventual local's index out of boundaries error

  \param  LocalsRefs:		The locals references array
  \param  locIndex:			Accessed local's index
  \param  localsSize:		Size of the Locals Area
  \param  sp:				pointer to the current stack depth

  \return local/stack access flags

 */


uint32_t Local_ST(uint32_t *LocalsRefs, uint32_t locIndex, uint16_t localsSize, int32_t *sp)
{
	uint32_t result = 0;

	result = Stack_POP(sp, ONE_STACK_ELEMENT);

	if (locIndex >= localsSize)
	{
		result |= FLAG_LST_LOCAL_OUTOB;
		return result;
	}

	if (result == 0)
		LocalsRefs[locIndex] = TRUE;

	return result;

}




/*!
  \brief  Simulates a stack PUSH and a locals use reference. Returns the result of the PUSH, the
	  eventual local's index out of boundaries, or the Unreferenced Local error

  \param  LocalsRefs:		The locals references array
  \param  locIndex:			Accessed local's index
  \param  localsSize:		Size of the Locals Area
  \param  sp:				Pointer to the current stack depth
  \param  stackLimit:		The maximum stack length

  \return local/stack access flags

 */


uint32_t Local_LD(uint32_t *LocalsRefs, uint32_t locIndex, uint16_t localsSize, int32_t *sp, int32_t stackLimit)
{
	uint32_t result = 0;

	result = Stack_PUSH(sp, stackLimit, ONE_STACK_ELEMENT);

	if (locIndex >= localsSize)
	{
		result |= FLAG_LST_LOCAL_OUTOB;
		return result;
	}

	return result;
}




/*!
  \brief  Verifies the effective locals area utilization scanning the Locals References array

  \param  LocalsRefs:		The locals references array
  \param  localsSize:		Size of the Locals Area

  \return the number of locals effectively used in the bytecode
*/


uint16_t Get_Locals_Use(uint32_t *LocalsRefs, uint16_t localsSize)
{
	uint16_t result = 0, i;

	if (LocalsRefs == NULL)
		return nvmJitSUCCESS;

	for (i = 0; i < localsSize; i++)
	{
		if (LocalsRefs[i])
			result++;
	}

	return result;

}



/*!
  \brief  update the current InstructionInfo stack depth to currStackDepth

  \param  BCInfoArray:		pointer to the InstructionInfo array
  \param  pc:				current instruction index
  \param  currStackDepth:	the current stack depth


  \return NOTHING
*/


void Update_Instr_Stack_Depth(InstructionInfo * BCInfoArray, uint32_t pc, int32_t currStackDepth)
{
#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Update_Instr_Stack_Depth");
#endif

	BCInfoArray[pc].StackDepth = MAX(BCInfoArray[pc].StackDepth,currStackDepth);

}





/*!
  \brief  Initialize the Instruction Indexes struct

  \param  segment:			pointer to the NetVM bytecode segment descriptor
  \param  errList:			pointer to the compilation errors list
  \return a pointer to the newly allocated InstrIndexes struct
*/


InstrIndexes * New_Instr_Indexes(nvmByteCodeSegment * segment, ErrorList * errList)
{
	uint8_t *bytecode;
	uint8_t opcode;
	uint32_t pc, insnNum, segmentLen;
	InstrIndexes * InstrIndexesStruct;
	uint32_t	*InstrIndexesArray;
	//uint32_t ncases = 0;

	bytecode = segment->ByteCode;
	segmentLen = segment->SegmentSize;

	if (segmentLen == 0){
		return NULL;
	}

	InstrIndexesArray = (uint32_t *) CALLOC(segment->SegmentSize, sizeof(uint32_t));
	if (InstrIndexesArray == NULL){
		return NULL;
	}

	InstrIndexesStruct = (InstrIndexes *) CALLOC(1, sizeof(InstrIndexes));
	if (InstrIndexesStruct == NULL){
		free(InstrIndexesArray);
		free(InstrIndexesStruct);
		return NULL;
	}

	//Instruction Indexes array initialization:
	pc = 0;
	insnNum = 0;

	while(pc < segmentLen){
		opcode = bytecode[pc];
		if (strcmp(nvmOpCodeTable[opcode].CodeName, OPCODE_NAME_INVALID) == 0){
			Analysis_Error(errList, FLAG_OP_NOT_DEF, NULL, pc, 0, 0);
			free(InstrIndexesArray);
			free(InstrIndexesStruct);
			return NULL;
		}

		InstrIndexesArray[pc] = insnNum;	// Instruction Start index
		pc += GetInstructionLen(&bytecode[pc]);

		insnNum++;
	}

	// BYTECODE_FALLOUT_TEST:
	if (pc != segmentLen){
		Analysis_Error(errList, FLAG_BC_FALLOUT, NULL, pc, 0, 0);
		free(InstrIndexesArray);
		free(InstrIndexesStruct);
		return NULL;
	}


	if (insnNum == 0){
		free(InstrIndexesArray);
		free(InstrIndexesStruct);
		return NULL;
	}

	InstrIndexesStruct->InstrIndexesArray = InstrIndexesArray;
	InstrIndexesStruct->NumInstructions = insnNum;
	InstrIndexesStruct->SegmentLength = segmentLen;


	return InstrIndexesStruct;
}


uint32_t Translate_BC_Index(InstrIndexes * InstrIndexesStruct, uint32_t BCPC, uint32_t *isValid, ErrorList * errList)
{
	if (BCPC > InstrIndexesStruct->SegmentLength){
		(*isValid) = FALSE;
		Analysis_Error(errList, FLAG_INVALID_BR_TARGET, NULL, BCPC, 0, 0);
		return nvmJitSUCCESS;
	}
	if ((InstrIndexesStruct->InstrIndexesArray[BCPC] == 0) && (BCPC != 0)){
		(*isValid) = FALSE;
		Analysis_Error(errList, FLAG_INVALID_BR_TARGET, NULL, BCPC, 0, 0);
		return nvmJitSUCCESS;
	}

	*isValid = TRUE;


	return InstrIndexesStruct->InstrIndexesArray[BCPC];
}

int32_t Create_ByteCode_Info_Array(nvmJitByteCodeInfo * BCInfo, InstrIndexes * InstrIndexesStruct, ErrorList * errList)
{
	uint32_t BCPC, BCInfoPC;		// Bytecode program counter, BCInfo program counter
	uint8_t opcode;				//current instruction opcode
	uint8_t *bytecode;				//bytecode array
	uint32_t segmentLen;			//code length
	InstructionInfo * BCInfoArray;			//Array of InstructionInfo
	uint32_t numInstructions, btarget;
	uint32_t isValid = FALSE;
	SwInsnInfo *swInfo = NULL;
	uint32_t i = 0;
	uint32_t insnLen = 0;


	//std::cout << "Create Bytecode Array" << std::endl;

	if (InstrIndexesStruct == NULL || BCInfo == NULL){
		return nvmJitFAILURE;
	}

	if (BCInfo->Segment->SegmentSize == 0 || InstrIndexesStruct->NumInstructions == 0){
		return nvmJitFAILURE;
	}
	BCInfoArray = BCInfo->BCInfoArray;
	bytecode = BCInfo->Segment->ByteCode;
	numInstructions = InstrIndexesStruct->NumInstructions;
	segmentLen = InstrIndexesStruct->SegmentLength;
	BCPC = 0;
	BCInfoPC = 0;

	while ((BCPC < segmentLen) && (BCInfoPC < numInstructions)){
		opcode = bytecode[BCPC];


		BCInfoArray[BCInfoPC].Opcode = opcode;
		BCInfoArray[BCInfoPC].SwInfo = NULL;
		switch(opcode){
			case JUMP: case CALL:
				nvmFLAG_SET(BCInfoArray[BCInfoPC].Flags, FLAG_BR_INSN);
				//SET_BR_INSN(BCInfoArray[BCInfoPC].Flags);
				btarget = Translate_BC_Index(InstrIndexesStruct, SHORT_JMP_TARGET(BCPC), &isValid, errList);
				if (!isValid)
					return nvmJitFAILURE;
				BCInfoArray[BCInfoPC].Arguments[0] = btarget;
				BCInfoArray[BCInfoPC].NumArgs = 1;

				break;
			case JUMPW:	case CALLW:
				nvmFLAG_SET(BCInfoArray[BCInfoPC].Flags, FLAG_BR_INSN);
				//SET_BR_INSN(BCInfoArray[BCInfoPC].Flags);
				btarget = Translate_BC_Index(InstrIndexesStruct, LONG_JMP_TARGET(BCPC), &isValid, errList);
				if (!isValid)
					return nvmJitFAILURE;
				BCInfoArray[BCInfoPC].Arguments[0] = btarget;
				BCInfoArray[BCInfoPC].NumArgs = 1;

				break;

			case JEQ: case JNE:
/*
			case BJFLDEQ: case BJFLDNEQ: case BJFLDLT: case BJFLDLE:
			case BJFLDGT: case BJFLDGE: case SJFLDEQ: case SJFLDNEQ:
			case SJFLDLT: case SJFLDLE: case SJFLDGT: case SJFLDGE:
			case IJFLDEQ: case IJFLDNEQ: case IJFLDLT: case IJFLDLE:
			case IJFLDGT: case IJFLDGE:
			case BMJFLDEQ: case SMJFLDEQ: case IMJFLDEQ:
*/
			case JCMPEQ: case JCMPNEQ: case JCMPG: case JCMPGE:
			case JCMPL: case JCMPLE: case JCMPG_S: case JCMPGE_S:
			case JCMPL_S: case JCMPLE_S:
			case JFLDEQ: case JFLDNEQ: case JFLDLT: case JFLDGT:

				nvmFLAG_SET(BCInfoArray[BCInfoPC].Flags, FLAG_BR_INSN);
				//SET_BR_INSN(BCInfoArray[BCInfoPC].Flags);
				BCInfoArray[BCInfoPC].Arguments[0] = Translate_BC_Index(InstrIndexesStruct, LONG_JMP_TARGET(BCPC), &isValid, errList);
				if (!isValid)
					return nvmJitFAILURE;
				BCInfoArray[BCInfoPC].Arguments[1] = Translate_BC_Index(InstrIndexesStruct, BCPC + GetInstructionLen(&bytecode[BCPC]), &isValid, errList);
				if (!isValid)
					return nvmJitFAILURE;
				BCInfoArray[BCInfoPC].NumArgs = 2;

				break;
			case SWITCH:
				nvmFLAG_SET(BCInfoArray[BCInfoPC].Flags, FLAG_SW_INSN);
				swInfo = (SwInsnInfo*)malloc(sizeof(SwInsnInfo));
				if (swInfo == NULL)
					return nvmJitFAILURE;
				swInfo->NumCases = LONG_ARG(BCPC + 4);
				swInfo->CaseTargets = (uint32_t*)calloc(swInfo->NumCases, 4);
				swInfo->Values		= (uint32_t*)calloc(swInfo->NumCases, sizeof(uint32_t));
				if (swInfo->CaseTargets == NULL)
					return nvmJitFAILURE;
				insnLen = GetInstructionLen(&bytecode[BCPC]);
				swInfo->DefTarget = Translate_BC_Index(InstrIndexesStruct, (*(int32_t*)&bytecode[BCPC + 1]) + insnLen + BCPC, &isValid, errList);
				for (i = 0; i < swInfo->NumCases; i++)
				{
					//swInfo->Values[i] = (uint32_t)bytecode[BCPC + 4 + 1 + 8*i];	// ???
					swInfo->Values[i] = *(uint32_t*)&bytecode[BCPC + 1 + 8 + 8*i];
					swInfo->CaseTargets[i] = Translate_BC_Index(InstrIndexesStruct, *(int32_t*)&bytecode[BCPC + 1 + 12 + 8*i] + insnLen + BCPC, &isValid, errList);
				}
				BCInfoArray[BCInfoPC].NumArgs = 0;
				BCInfoArray[BCInfoPC].SwInfo = swInfo;
				break;
			default:
				switch (nvmOpCodeTable[opcode].ArgLen){
					case T_NO_ARG_INST:
						BCInfoArray[BCInfoPC].NumArgs = 0;
						break;
					case T_1BYTE_ARG_INST:
						BCInfoArray[BCInfoPC].NumArgs = 1;
						BCInfoArray[BCInfoPC].Arguments[0] = BYTE_ARG(BCPC);
						break;
					case T_1INT_ARG_INST:
						BCInfoArray[BCInfoPC].NumArgs = 1;
						BCInfoArray[BCInfoPC].Arguments[0] = LONG_ARG(BCPC);
						break;
					case T_2INT_ARG_INST:
						BCInfoArray[BCInfoPC].NumArgs = 2;
						BCInfoArray[BCInfoPC].Arguments[0] = LONG_ARG(BCPC);
						BCInfoArray[BCInfoPC].Arguments[1] = LONG_ARG(BCPC + 4);
						break;
					default:	//NOTHING
						Analysis_Error(errList, FLAG_INVALID_ARGS_SZ, NULL, BCPC, 0, 0);
						return nvmJitFAILURE;
						break;
				}
				break;
		}

		BCInfoArray[BCInfoPC].BCpc = BCPC;
		std::map<uint32_t, uint32_t>::iterator k = BCInfo->Segment->Insn2LineMap.find(BCPC);
		if (k != BCInfo->Segment->Insn2LineMap.end())
		{
			BCInfoArray[BCInfoPC].SrcLine = k->second;
			//std::cout << "Map: " << BCPC << " --> " << k->second << std::endl;
		}
		else
		{
			BCInfoArray[BCInfoPC].SrcLine = 0;
		}
		BCInfoPC ++;

		BCPC += GetInstructionLen(&bytecode[BCPC]);
	}

	if (BCInfoArray[BCInfoPC-1].Opcode != EXIT && \
		BCInfoArray[BCInfoPC-1].Opcode != RET &&  \
		BCInfoArray[BCInfoPC-1].Opcode != SNDPKT && \
		(!nvmFLAG_ISSET(BCInfoArray[BCInfoPC - 1].Flags, FLAG_BR_INSN)) && \
		(!nvmFLAG_ISSET(BCInfoArray[BCInfoPC - 1].Flags, FLAG_SW_INSN))
		){
		Analysis_Error(errList, FLAG_END_OF_SEG_WO_RET, NULL, BCPC, 0, 0);
		return nvmJitFAILURE;
	}

	if (BCPC != segmentLen){
		Analysis_Error(errList, FLAG_WRONG_NUM_INSTR, NULL, BCPC, 0, 0);
		return nvmJitFAILURE;
	}

	return nvmJitSUCCESS;
}

/*!
  \brief  creates and initializes a nvmJitByteCodeInfo structure given its corresponding NetVM
	  bytecode segment descriptor

  \param  segment:			pointer to the NetVM bytecode segment descriptor
  \param  errList:			pointer to the compilation errors list

  \return pointer to the new allocated nvmJitByteCodeInfo structure
*/


nvmJitByteCodeInfo * New_ByteCode_Info(nvmByteCodeSegment * segment, ErrorList * errList)
{
	InstructionInfo * BCInfoArray;
	nvmJitByteCodeInfo * BCInfo;
	InstrIndexes * InstrIndexesStruct;
	uint32_t numInstructions;

	if (segment == NULL){
		return NULL;
	}

	if (segment->SegmentSize == 0){
		return NULL;
	}

	if (strcmp(segment->Name,".ports") == 0){
		return NULL;
	}

	InstrIndexesStruct = New_Instr_Indexes(segment, errList);
	if (InstrIndexesStruct == NULL){
		return NULL;
	}
	numInstructions = InstrIndexesStruct->NumInstructions;


	BCInfoArray = (InstructionInfo *) CALLOC(numInstructions, sizeof(InstructionInfo));
	if (BCInfoArray == NULL){
		Free_Instr_Indexes(InstrIndexesStruct);
		return NULL;
	}

	BCInfo = (nvmJitByteCodeInfo *) CALLOC(1, sizeof(nvmJitByteCodeInfo));
	if (BCInfo == NULL){
		Free_Instr_Indexes(InstrIndexesStruct);
		free(BCInfoArray);
		return NULL;
	}

//	BCInfo->StackSize = segment->MaxStackSize;
//	BCInfo->LocalsSize = segment->LocalsSize;
	BCInfo->BCInfoArray = BCInfoArray;
	BCInfo->MaxStackDepth = 0;
	BCInfo->LocalsUse = 0;
	BCInfo->NumBasicBlocks = 0;
	BCInfo->Segment = segment;
	BCInfo->NumInstructions = numInstructions;
	BCInfo->LocalsReferences = NULL;
	BCInfo->AnalysisFlags = 0;

	if (Create_ByteCode_Info_Array(BCInfo, InstrIndexesStruct, errList) < 0){
		BCInfo->AnalysisFlags = errList->Tail->ErrFlags;
		Free_Instr_Indexes(InstrIndexesStruct);
		//nvmJitFree_ByteCode_Info(BCInfo);
		return BCInfo;
	}

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "BCInfo Created");
#endif

	Free_Instr_Indexes(InstrIndexesStruct);
	return BCInfo;
}




/*!
  \brief  Frees memory allocated for the nvmJitByteCodeInfo Structure

  \param  BCInfo:			pointer to the nvmJitByteCodeInfo struct

  \return Nothing
*/


void nvmJitFree_ByteCode_Info(nvmJitByteCodeInfo * BCInfo)
{
	if (BCInfo != NULL){
		if (BCInfo->LocalsReferences != NULL)
			free(BCInfo->LocalsReferences);
		if (BCInfo->BCInfoArray != NULL)
			free(BCInfo->BCInfoArray);
		free(BCInfo);
	}
}

void Free_Instr_Indexes(InstrIndexes * InsIndexes)
{
	if (InsIndexes != NULL){
		if (InsIndexes->InstrIndexesArray != NULL)
			free(InsIndexes->InstrIndexesArray);
		free(InsIndexes);
	}
}



/*!
  \brief  sets a new Basic Block leader. Used to create a new basic block following a RET
	  or a branch/jump Instruction

  \param  BCInfoArray:		pointer to the InstructionInfo array
  \param  pc:				current instruction index
  \param  currStackDepth:	the current stack depth
  \param  BBCount:			pointer to the Basic Block counter

  \return NOTHING
*/


void Set_Basic_Block_Leader(InstructionInfo * BCInfoArray, uint32_t pc, int32_t currStackDepth, uint16_t *BBCount)
{

	if (!nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_BB_LEADER)){
		(*BBCount)++;
		BCInfoArray[pc].BasicBlockId = (*BBCount);

	}

	nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_BB_LEADER);
	if (!nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_VISITED)){

	}


	BCInfoArray[pc].StackDepth = MAX(BCInfoArray[pc].StackDepth,currStackDepth);


}




/*!
  \brief  sets the end of a Basic Block updating the InstructionInfo element's numSuccessors.
	  Used when we encounter a branch/jump instruction or a RET

  \param  BCInfoArray:		pointer to the InstructionInfo array
  \param  pc:				current instruction index
  \param  currStackDepth:	the current stack depth
  \param  consumedElements: number of stack elements elements consumed by the instruction
  \param  numSuccessors:		the numer of edges coming out from the basic block:
							(0 if RET, 1 if JUMP, 2 if BRANCH)
  \return NOTHING
*/


void Set_End_Of_Basic_Block(InstructionInfo * BCInfoArray, uint32_t pc, int32_t currStackDepth, uint16_t consumedElements, uint16_t numSuccessors)
{

	nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_BB_END);
	BCInfoArray[pc].NumSuccessors = numSuccessors;
	BCInfoArray[pc].StackDepth = currStackDepth;
	BCInfoArray[pc].StackAfter =(uint32_t) (currStackDepth - consumedElements);

}




/*!
  \brief  split an existing Basic Block in 2

  \param  BCInfoArray:		pointer to the InstructionInfo array
  \param  pc:				current instruction index

  \return NOTHING
*/


void Split_Basic_Block(InstructionInfo * BCInfoArray, uint32_t pc)
{

	if (pc > 0){
		BCInfoArray[pc - 1].NumSuccessors++;
		nvmFLAG_SET(BCInfoArray[pc - 1].Flags, FLAG_BB_END);
	}

	BCInfoArray[pc].NumPredecessors++;
}


/*!
  \brief  updates the first InstructionInfo element adding the first Basic Block information

  \param  BCInfoArray:		pointer to the InstructionInfo array
  \param  pc:				current instruction index
  \param  initStackDepth:	the initial stack depth
  \param  BBCount:			pointer to the Basic Block counter

  \return NOTHING
*/


void Add_First_Basic_Block(InstructionInfo * BCInfoArray, uint32_t pc, int32_t initStackDepth, uint16_t *BBCount)
{

	BCInfoArray[pc].NumPredecessors=0;
	BCInfoArray[pc].NumSuccessors=0;
	Set_Basic_Block_Leader(BCInfoArray, pc, initStackDepth, BBCount);

}




/*!
  \brief  adds a Basic Block information backward in the code

  \param  BCInfoArray:		pointer to the InstructionInfo array
  \param  pc:				current instruction index
  \param  currStackDepth:	the current stack depth
  \param  BBCount:			pointer to the Basic Block counter

  \return TRUE if there are incoming edges with different stack depths FALSE otherwise
*/


uint32_t Add_BW_Basic_Block(InstructionInfo * BCInfoArray, uint32_t pc, int32_t currStackDepth, uint16_t *BBCount)
{

	uint32_t diffInStackDepth=FALSE;


	if (!nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_BB_LEADER)){  // Split the basic block
		Split_Basic_Block(BCInfoArray, pc);
	}


	if ((currStackDepth != BCInfoArray[pc].StackDepth) && nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_VISITED)){
		nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_STACK_MERGE_ERR);
		diffInStackDepth=TRUE;
	}


	Set_Basic_Block_Leader(BCInfoArray, pc, currStackDepth, BBCount);
	BCInfoArray[pc].NumPredecessors++;


	return diffInStackDepth;
}




/*!
  \brief  adds a Basic Block information forward in the code

  \param  BCInfoArray:		pointer to the InstructionInfo array
  \param  pc:				current instruction index
  \param  currStackDepth:	the current stack depth
  \param  BBCount:			pointer to the Basic Block counter

  \return TRUE if there are incoming edges with different stack depths
		  FALSE otherwise
*/


uint32_t Add_FW_Basic_Block(InstructionInfo * BCInfoArray, uint32_t pc, int32_t currStackDepth, uint16_t *BBCount)
{
	uint32_t diffInStackDepth=FALSE;


	if (!nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_BB_LEADER)){  // Split the basic block
		Split_Basic_Block(BCInfoArray, pc);
	}

	if ((currStackDepth != BCInfoArray[pc].StackDepth) && nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_VISITED)){
		nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_STACK_MERGE_ERR);
		diffInStackDepth = TRUE;
	}


	Set_Basic_Block_Leader(BCInfoArray, pc, currStackDepth, BBCount);
	BCInfoArray[pc].NumPredecessors++;

	return diffInStackDepth;
}




/*!
  \brief  creates Basic Blocks because of a JUMP-like instruction. If the branch points to the
	  next instruction marks the current instr for successive deletion

  \param  BCInfo:			pointer to a nvmJitByteCodeInfo structure
  \param  pc:				current instruction index
  \param  branchTarget:		index target of the branch
  \param  currStackDepth:	the current stack depth
  \param  BBCount:			pointer to the Basic Block counter

  \return TRUE if creates Basic Blocks with different stack depth from incoming edges
	      FALSE otherwise
*/


uint32_t Manage_JUMP(nvmJitByteCodeInfo * BCInfo, uint32_t pc, uint32_t branchTarget, int32_t currStackDepth, uint16_t *BBCount)
{
	InstructionInfo * BCInfoArray;

	uint32_t diffInStackDepth=FALSE;

	BCInfoArray = BCInfo->BCInfoArray ;

	nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_BR_INSN);// current is a branch/jmp instruction

	if (branchTarget == pc + 1){// JMP to next instruction
		diffInStackDepth = Add_FW_Basic_Block(BCInfoArray, branchTarget, currStackDepth, BBCount);
	}
	else{
		if (branchTarget <= pc)	// JMP Backward in the code
			diffInStackDepth = Add_BW_Basic_Block(BCInfoArray, branchTarget, currStackDepth, BBCount);
		else					// JMP Forward in the code
			diffInStackDepth = Add_FW_Basic_Block(BCInfoArray, branchTarget, currStackDepth, BBCount);
		// Next Instruction is BB Leader
		if ((pc + 1) < BCInfo->NumInstructions)
			Set_Basic_Block_Leader(BCInfoArray, pc + 1, 0, BBCount);
	}

	Set_End_Of_Basic_Block(BCInfoArray,pc,currStackDepth, NO_STACK_ELEMENTS, JMP_OUT_EDGES);

	return diffInStackDepth;
}




/*!
  \brief  creates Basic Blocks because of a Branch-like instruction. If the branch points to the
	  next instruction marks the current instr for successive deletion

  \param  BCInfo:			pointer to a nvmJitByteCodeInfo structure
  \param  pc:				current instruction index
  \param  branchTarget:		index target of the branch
  \param  currStackDepth:	the current stack depth
  \param  consumedElements:	the number of stack elements consumed by the branch instr
  \param  BBCount:			pointer to the Basic Block counter

  \return TRUE if creates Basic Blocks with different stack depth from incoming edges
	      FALSE otherwise
*/


uint32_t Manage_BRANCH(nvmJitByteCodeInfo * BCInfo, uint32_t pc, uint32_t branchTarget, int32_t currStackDepth, uint16_t consumedElements, uint16_t *BBCount)
{
	InstructionInfo * BCInfoArray;
	uint32_t numInsns = BCInfo->NumInstructions;
	uint32_t diffInStackDepth=FALSE;


	BCInfoArray = BCInfo->BCInfoArray ;

	nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_BR_INSN);	// current is a branch/jmp instruction

	if (branchTarget == pc + 1){// BRANCH to next instruction = 2 * JUMP next
		diffInStackDepth = Add_FW_Basic_Block(BCInfoArray, branchTarget,(uint16_t) (currStackDepth - consumedElements), BBCount);
		diffInStackDepth |= Add_FW_Basic_Block(BCInfoArray, branchTarget,(uint16_t) (currStackDepth - consumedElements), BBCount);
	}
	else{
		if (branchTarget <= pc)	// BRANCH Backward in the code
			diffInStackDepth = Add_BW_Basic_Block(BCInfoArray, branchTarget, (uint32_t) (currStackDepth - consumedElements), BBCount);
		else					// BRANCH Forward in the code
			diffInStackDepth = Add_FW_Basic_Block(BCInfoArray, branchTarget, (uint32_t) (currStackDepth - consumedElements), BBCount);
		if ((pc+1) < numInsns)
		{
			//Next statement is Basic Block Leader with 1 more incoming edge:
			diffInStackDepth |= Add_FW_Basic_Block(BCInfoArray, pc + 1,(uint16_t) (currStackDepth - consumedElements),
			BBCount);
		}
	}

	Set_End_Of_Basic_Block(BCInfoArray,pc,currStackDepth, consumedElements, BRANCH_OUT_EDGES);

	return diffInStackDepth;
}



nvmJitByteCodeInfo * nvmJitAnalyse_ByteCode_Segment_Ex(nvmByteCodeSegment * segment, ErrorList * errList,
	uint16_t *BBCount)
{
	uint32_t pc;					//Instr index
	int32_t sp;						//current stack depth tracking

//	uint16_t BBCount; 				//Basic Block Counter
	int32_t maxStackDepth;			//Maximum Reached Stack Depth

	nvmJitByteCodeInfo * BCInfo;			//nvmJitByteCodeInfo structure that
									//will retain the results of the bytecode
									//analysis

	InstructionInfo * BCInfoArray;			//Array of InstructionInfo
	uint32_t numInstructions;		//Number of Instructions

	uint32_t *LocalsReferences = NULL;	//Locals Refs Array
	int32_t StackLimit = 0;			//Operands Stack Size
	uint16_t LocalsSize = 0;		//Locals Region Size

	uint32_t i;

	uint32_t xchBufUse = FALSE, datMemUse = FALSE, shdMemUse = FALSE;

	uint32_t diffStackDepth = FALSE;
	uint32_t result = 0;

	if (segment == NULL){
		return NULL;
	}

	if (segment->SegmentSize == 0){
		return NULL;
	}

	if (segment->SegmentType == PORT_SEGMENT){
		return NULL;
	}

	BCInfo = New_ByteCode_Info(segment, errList);

	if (BCInfo == NULL){
		return NULL;
	}


	if (segment->SegmentType == INIT_SEGMENT){
		SET_INITIAL_STACK(sp, 0);	// .init segment
	}
	else
	{
		SET_INITIAL_STACK(sp, 1);	// the calling port is on the stack
	}

	StackLimit = segment->MaxStackSize;
	LocalsSize = segment->LocalsSize;

	if (LocalsSize != 0){

		//Allocate the Locals References Array
		LocalsReferences = (uint32_t *) CALLOC(LocalsSize, sizeof(uint32_t));

		if (LocalsReferences == NULL){
			return NULL;
		}
	}
	else{
		LocalsReferences = NULL;
	}

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "ByteCode Analysis Step");
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "----------------------");
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Segment Name: %s\n", segment->Name);
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Locals Size: %d", LocalsSize);
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Max Stack Size: %d", StackLimit);
#endif

	BCInfoArray = BCInfo->BCInfoArray;
	numInstructions = BCInfo->NumInstructions;
	pc = 0;
	maxStackDepth = sp;

	//first instruction is BB leader:
	Add_First_Basic_Block(BCInfoArray, pc, sp, BBCount);

	while (pc < numInstructions)
	{
		switch(BCInfoArray[pc].Opcode)
		{

			case RET:
			case SNDPKT:

				nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_RET_INSN);
				// only END OF BB
				Set_End_Of_Basic_Block(BCInfoArray,pc,sp, NO_STACK_ELEMENTS, RET_OUT_EDGES);

				//Next statement is Basic Block Leader (if in the segment)
				if ((pc + 1) < numInstructions){
//					Set_Basic_Block_Leader(BCInfoArray, pc + 1, 0, &BBCount);
					Set_Basic_Block_Leader(BCInfoArray, pc + 1, 0, BBCount);
					sp = BCInfoArray[pc + 1].StackDepth;
				}

				break;


			// JUMPS NO STACK OPERATIONS

			case JUMP: case JUMPW:

//				diffStackDepth = Manage_JUMP(BCInfo, pc, BCInfoArray[pc].Arguments[0],sp, &BBCount);
				diffStackDepth = Manage_JUMP(BCInfo, pc, BCInfoArray[pc].Arguments[0],sp, BBCount);

				if (diffStackDepth){
					nvmFLAG_SET(BCInfo->AnalysisFlags, FLAG_STACK_MERGE);
//					Analysis_Error(errList, FLAG_STACK_MERGE, BCInfoArray[pc].Arguments[0], sp, BBCount);
					Analysis_Error(errList, FLAG_STACK_MERGE, &BCInfoArray[BCInfoArray[pc].Arguments[0]], 0, sp, *BBCount);
				}

				if ((pc + 1) < numInstructions){
					sp = BCInfoArray[pc + 1].StackDepth;
				}
				break;

			//BRANCHES	STACK TRANSITION: value ==> ...

			case JEQ: case JNE:
/*
			case BJFLDEQ: case BJFLDNEQ:
			case BJFLDLT: case BJFLDLE:
			case BJFLDGT: case BJFLDGE:

			case SJFLDEQ: case SJFLDNEQ:
			case SJFLDLT: case SJFLDLE:
			case SJFLDGT: case SJFLDGE:

			case IJFLDEQ: case IJFLDNEQ:
			case IJFLDLT: case IJFLDLE:
			case IJFLDGT: case IJFLDGE:
*/
//				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].Arguments[0], sp, ONE_STACK_ELEMENT, &BBCount);
				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].Arguments[0], sp, ONE_STACK_ELEMENT, BBCount);


				if (diffStackDepth){
					nvmFLAG_SET(BCInfo->AnalysisFlags, FLAG_STACK_MERGE);
//					Analysis_Error(errList, FLAG_STACK_MERGE, BCInfoArray[pc].Arguments[0], sp, BBCount);
					Analysis_Error(errList, FLAG_STACK_MERGE, &BCInfoArray[BCInfoArray[pc].Arguments[0]], 0, sp, *BBCount);
				}

				result |= Stack_POP(&sp, ONE_STACK_ELEMENT);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}

				BCInfoArray[pc].StackAfter = sp;

				break;

			case SWITCH:

//				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].SwInfo->DefTarget, sp, ONE_STACK_ELEMENT, &BBCount);
				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].SwInfo->DefTarget, sp, ONE_STACK_ELEMENT, BBCount);

				if (diffStackDepth){
					nvmFLAG_SET(BCInfo->AnalysisFlags, FLAG_STACK_MERGE);
//					Analysis_Error(errList, FLAG_STACK_MERGE, BCInfoArray[pc].Arguments[0], sp, BBCount);
					Analysis_Error(errList, FLAG_STACK_MERGE, &BCInfoArray[BCInfoArray[pc].Arguments[0]], 0, sp, *BBCount);
				}
				for ( i = 0; i < BCInfoArray[pc].SwInfo->NumCases; i++ )
				{
//					diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].SwInfo->CaseTargets[i], sp, ONE_STACK_ELEMENT, &BBCount);
					diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].SwInfo->CaseTargets[i], sp, ONE_STACK_ELEMENT, BBCount);
					//diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].SwInfo->CaseTargets[i], sp, NO_STACK_ELEMENTS, &BBCount);

					if (diffStackDepth){
						nvmFLAG_SET(BCInfo->AnalysisFlags, FLAG_STACK_MERGE);
//						Analysis_Error(errList, FLAG_STACK_MERGE, BCInfoArray[pc].Arguments[0], sp, BBCount);
						Analysis_Error(errList, FLAG_STACK_MERGE, &BCInfoArray[BCInfoArray[pc].Arguments[0]], 0, sp, *BBCount);
					}
				}

				result |= Stack_POP(&sp, ONE_STACK_ELEMENT);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}

				BCInfoArray[pc].StackAfter = sp;

				break;

			//BRANCHES	STACK TRANSITION: value, value ==> ...

			case JCMPEQ: case JCMPNEQ: case JCMPG:
			case JCMPGE: case JCMPL: case JCMPLE:
			case JCMPG_S: case JCMPGE_S:
			case JCMPL_S: case JCMPLE_S:
			/*
			case BMJFLDEQ: case SMJFLDEQ: case IMJFLDEQ:
			*/


//				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].Arguments[0], sp, TWO_STACK_ELEMENTS, &BBCount);
				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].Arguments[0], sp, TWO_STACK_ELEMENTS, BBCount);

				if (diffStackDepth){
					nvmFLAG_SET(BCInfo->AnalysisFlags, FLAG_STACK_MERGE);
//					Analysis_Error(errList, FLAG_STACK_MERGE, BCInfoArray[pc].Arguments[0], sp, BBCount);
					Analysis_Error(errList, FLAG_STACK_MERGE, &BCInfoArray[BCInfoArray[pc].Arguments[0]], 0, sp, *BBCount);
				}

				result |= Stack_POP(&sp, TWO_STACK_ELEMENTS);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}

				BCInfoArray[pc].StackAfter = sp;
				break;

			//field comparisons	STACK TRANSITION: value, value, value ==> ...
			case JFLDEQ: case JFLDNEQ: case JFLDLT: case JFLDGT:

//				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].Arguments[0], sp, THREE_STACK_ELEMENTS, &BBCount);
				diffStackDepth = Manage_BRANCH(BCInfo, pc, BCInfoArray[pc].Arguments[0], sp, THREE_STACK_ELEMENTS, BBCount);

				if (diffStackDepth){
					nvmFLAG_SET(BCInfo->AnalysisFlags, FLAG_STACK_MERGE);
//					Analysis_Error(errList, FLAG_STACK_MERGE, BCInfoArray[pc].Arguments[0], sp, BBCount);
					Analysis_Error(errList, FLAG_STACK_MERGE, &BCInfoArray[BCInfoArray[pc].Arguments[0]], 0, sp, *BBCount);
				}

				result |= Stack_POP(&sp, THREE_STACK_ELEMENTS);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}

				BCInfoArray[pc].StackAfter = sp;
				break;
			// PUSHes	STACK TRANSITION: ... ==> value
			case PUSH:
			case CONST_1: case CONST_2:
			case CONST_0: case CONST__1:
//			case TSTAMP: case IRND:
			case TSTAMP_S: case TSTAMP_US: case IRND:
			case PBL:
			case COPIN: case COPINIT:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_PUSH(&sp, StackLimit, ONE_STACK_ELEMENT);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// LOCALS LOAD	STACK TRANSITION: ... ==> value
			case LOCLD:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Local_LD(LocalsReferences, BCInfoArray[pc].Arguments[0], LocalsSize, &sp, StackLimit);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;


			// DUP		STACK TRANSITION: value ==> value, value
			case DUP:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_DUP(&sp, StackLimit);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// PUSH2	STACK TRANSITION: ... ==> value, value
			case PUSH2:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_PUSH(&sp, StackLimit, TWO_STACK_ELEMENTS);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;


			//( STACK TRANSITION: value ==> ... )
			case POP: case SLEEP: case COPOUT:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_POP(&sp, ONE_STACK_ELEMENT);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// LOCALS STORE	STACK TRANSITION: value ==> ...
			case LOCST:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Local_ST(LocalsReferences, BCInfoArray[pc].Arguments[0], LocalsSize, &sp);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			//( STACK TRANSITION: value, value ==> value )

			// BIT CLEAR /SET /TEST:
			case SETBIT: case CLEARBIT: case FLIPBIT:
			case TESTBIT: case TESTNBIT:

			// SHIFTs and ROTs:
			case SHL: case SHR: case USHR:
			case ROTL: case ROTR:

			// ARITHMETIC & LOGIC OPs:
			case ADD: case ADDSOV: case ADDUOV:
			case SUB: case SUBSOV: case SUBUOV:
			case IMUL: case IMULSOV: case IMULUOV:
			case OR: case AND: case XOR: case MOD:

			// PACKET SCAN
			case PSCANB: case PSCANW: case PSCANDW:

			// COMPARISON
			case CMP: case CMP_S:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_POP(&sp, TWO_STACK_ELEMENTS);

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_PUSH(&sp, StackLimit, ONE_STACK_ELEMENT);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			//( STACK TRANSITION: value ==> value )

			// LOADs
			case PBLDS: case PBLDU: case PSLDS: case PSLDU: case PILD:
			case DBLDS: case DBLDU: case DSLDS: case DSLDU: case DILD:
			case SBLDS: case SBLDU: case SSLDS: case SSLDU: case SILD:
			case ISBLD: case ISSLD: case ISSBLD: case ISSSLD: case ISSILD:
			case NEG: case NOT: case BPLOAD_IH:


			// (1 POP -> 1 PUSH)
			case IINC_1: case IDEC_1:
			case IESWAP: case CLZ:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_POP(&sp, ONE_STACK_ELEMENT);

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_PUSH(&sp, StackLimit, ONE_STACK_ELEMENT);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// TODO OM: inc e dec...
/*
			case IINC_2: case IINC_3: case IINC_4:
			case IDEC_2: case IDEC_3: case IDEC_4:
				 break;
*/
			//( STACK TRANSITION: value, value ==> ... )
			// STOREs			(2 POP)
			case DBSTR: case DSSTR: case DISTR:
			case PBSTR: case PSSTR: case PISTR:
			case SBSTR: case SSSTR: case SISTR:
			case IBSTR: case ISSTR: case IISTR:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_POP(&sp, TWO_STACK_ELEMENTS);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// MASKED COMP	STACK TRANSITION: value, value, value ==> value )
			case MCMP:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_POP(&sp, THREE_STACK_ELEMENTS);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_PUSH(&sp, StackLimit, ONE_STACK_ELEMENT);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// POP_I			(i POPs)
			case POP_I:
				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_POP(&sp, BCInfoArray[pc].Arguments[0]);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// SWAP		STACK TRANSITION: value, value ==> value, value)
			case SWAP:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_POP(&sp, TWO_STACK_ELEMENTS);

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result |= Stack_PUSH(&sp, StackLimit, TWO_STACK_ELEMENTS);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result,&BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// MEMCOPY	 STACK TRANSITION: value, value, value ==> ...)
			case DPMCPY: case PDMCPY: case DSMCPY:
			case SDMCPY: case SPMCPY: case PSMCPY:

				Update_Instr_Stack_Depth(BCInfoArray, pc, sp);
				result = Stack_POP(&sp, THREE_STACK_ELEMENTS);
				if (result != 0){
					BCInfo->AnalysisFlags |= result;
//					Analysis_Error(errList, result, pc, sp, BBCount);
					Analysis_Error(errList, result, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			// INIT INSTRUCTIONS
			case SSMSIZE:
			//case LOADE: case STOREE:	//exceptions not implemented

				if (strcmp(segment->Name, ".init") != 0){
					BCInfo->AnalysisFlags |= FLAG_INIT_OP_IN_OTHER_SEG;
//					Analysis_Error(errList, FLAG_INIT_OP_IN_OTHER_SEG, pc, sp, BBCount);
					Analysis_Error(errList, FLAG_INIT_OP_IN_OTHER_SEG, &BCInfoArray[pc], 0, sp, *BBCount);
				}
				BCInfoArray[pc].StackAfter = sp;
				break;

			//Packet Instructions  STACK TRANSITION: ... ==> ...)
			case RCVPKT:
			case DSNDPKT:
			case DELEXBUF:
			case COPRUN:
			case COPPKTOUT:
			case INFOCLR:
			case NOP:	// NOTHING
				break;

			default:	//TODO: analyse Bytecode: OTHER instr

#ifdef ENABLE_NETVM_LOGGING
				logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Opcode not implemented, PC= %d, Name= %s",
					pc, nvmOpCodeTable[BCInfoArray[pc].Opcode].CodeName);
#endif

				BCInfo->AnalysisFlags |= FLAG_OP_NOT_IMPLEMENTED;
				Analysis_Error(errList, FLAG_OP_NOT_IMPLEMENTED, &BCInfoArray[pc], 0, sp, *BBCount);
				break;
		}


		// Test for memory usage:

		switch (BCInfoArray[pc].Opcode){
			/*
			case BJFLDEQ: case BJFLDNEQ: case BJFLDLT: case BJFLDLE:
			case BJFLDGT: case BJFLDGE: case SJFLDEQ: case SJFLDNEQ:
			case SJFLDLT: case SJFLDLE: case SJFLDGT: case SJFLDGE:
			case IJFLDEQ: case IJFLDNEQ: case IJFLDLT: case IJFLDLE:
			case IJFLDGT: case IJFLDGE:
			*/

			case JFLDEQ: case JFLDNEQ: case JFLDLT: case JFLDGT:
			case BPLOAD_IH:
			case PBLDS: case PBLDU: case PSLDS: case PSLDU: case PILD:
			case PSCANB: case PSCANW: case PSCANDW:
				xchBufUse = TRUE;
				break;


			case DBLDS: case DBLDU: case DSLDS: case DSLDU: case DILD:
				datMemUse = TRUE;
				break;

			case SBLDS: case SBLDU: case SSLDS: case SSLDU: case SILD:
				shdMemUse = TRUE;
				break;

			case DPMCPY: case PDMCPY:
				xchBufUse = TRUE;
				datMemUse = TRUE;
				break;

			case DSMCPY: case SDMCPY:
				shdMemUse = TRUE;
				datMemUse = TRUE;
				break;

			case SPMCPY: case PSMCPY:
				xchBufUse = TRUE;
				shdMemUse = TRUE;
				break;

		}

		//current instruction has been visited
		nvmFLAG_SET(BCInfoArray[pc].Flags, FLAG_VISITED);

		maxStackDepth=MAX(sp, maxStackDepth);

		if (BCInfo->AnalysisFlags != 0){

			return NULL;
		}

		pc++;

	}


	BCInfo->MaxStackDepth = maxStackDepth;
	BCInfo->NumBasicBlocks = *BBCount + 1;
	BCInfo->LocalsReferences = LocalsReferences;
	BCInfo->LocalsUse = Get_Locals_Use(LocalsReferences, LocalsSize);
	BCInfo->DatMemUse = datMemUse;
	BCInfo->ShdMemUse = shdMemUse;
	BCInfo->XchBufUse = xchBufUse;


	return BCInfo;
}





//tries to match the patterns {RET} and {POP-RET} on the bytecode
uint32_t nvmJitIs_Segment_Empty(nvmJitByteCodeInfo *BCInfo)
{

	if (BCInfo->BCInfoArray[0].Opcode == POP){
		if (BCInfo->BCInfoArray[1].Opcode == RET)
			return TRUE;
		return FALSE;
	}
	else if (BCInfo->BCInfoArray[0].Opcode == RET){
		return TRUE;
	}
	else
		return FALSE;

}
