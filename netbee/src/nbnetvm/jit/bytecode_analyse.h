/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "nbnetvm.h"
#include "netvmjitglobals.h"
#include "bytecode_segments.h"
#include "comperror.h"


/** @file bytecode_analyse.h
 * \brief This file contains prototypes, definitions and data structures for analysis and verification of the NetVM bytecode
 *
 */



/*!
	\defgroup BytecodeAnalysis	NetVMJit - Bytecode Analysis and Verification
*/

/*! \addtogroup BytecodeAnalysis
	\{


  Verification and analysis are performed with two passes over the bytecodes,
  gathering information useful for next compilation steps and checking
  bytecode correctness and consistency.<br>
  Analysis is done for one NetIL bytecode segment at a time. Valid segment types are
  .init, .push and .pull.<br>
  The first pass counts the number of instructions and fills
  an array with the indexes of each instruction in the bytecode, checking the
  presence of unrecognised opcodes and testing that the last instruction does not
  fall out of the bytecode segment, i.e. that the last opcode and all of its arguments lie
  inside the segment.
  If this first pass succeeds, a nvmJitByteCodeInfo structure is allocated and initialized for extra
  info recording.<br>
  An InstructionInfo array (nvmJitByteCodeInfo.BCInfoArray), with a size proportional to the number
  of instructions found in the first pass, keeps per instruction data.
  Such structures are filled parsing all the instructions of the segment.
  Stack operations are simulated and the stack depth before and after each instruction is
  recorded into the BCInfoArray. Stack underflows, stack overflows and use of
  empty stack make the verification process to fail. Local variables loads and
  stores are simulated as well and locals indexes are tested to be within the
  locals area boundaries. The loading of a not already initialized local is
  considered an error. <br>
  The initial stack depth for .push and . pull segments is set to  1, because the
  NetVM pushes the calling port id on the stack before executing the segment's code.
  The initial stack depth for .init segment is set to 0.
  Instructions already parsed are marked as visited.<br>
  The parsing of branch and return instructions implies the subdivision of the
  code into basic blocks. The first instruction and all branch targets are
  marked as Basic Block Leaders. Branch and return instructions are marked as
  Basic Block Ends, as well as instructions that precede a basic block leader.
  When a branch or jump instruction is reached, the
  corresponding branch target is tested to be contained within the segment's
  boundaries and to point to an instruction's opcode. Then Basic Block info
  are updated into the BCInfoArray, recording for each basic block leader
  the number of incoming edges and for each basic block end the number of
  outcoming edges.<br>
  When an opcode is target of multiple branches, the stack depths at basic
  block boundaries have to be merged. The merge succeeds only if stack depths
  from all incoming edges to a basic block leader are the same, otherwise
  the verification fails.<br>
  At the end of this process we know the stack depths at the beginning of all
  instructions, the maximum stack depth reached, the number of the locals
  effectively referenced and the number of basic blocks. As a help for the
  generation of the IR, the memory use is tracked down. All those information
  are useful for the construction of the Control Flow Graph and  for the other steps
  of the compilation process.

*/





//!Instruction Info flags: give details on NetIL instructions found in the bytecode
typedef enum{
	FLAG_BB_LEADER			= 0x01,	//!< Instruction is Basic Block Leader
	FLAG_BB_END				= 0x02,	//!< Instruction is Basic Block END
	FLAG_STACK_MERGE_ERR	= 0x04,	//!< Stack merging Error at Basic Block Start
	FLAG_SW_INSN			= 0x08,
	FLAG_MARK_FOR_DEL		= 0x10,	//!< Instruction is marked for deletion --> NOT CURRENTLY USED
	FLAG_RET_INSN			= 0x20,	//!< Ret Instruction
	FLAG_BR_INSN			= 0x40,	//!< Branch instruction
	FLAG_VISITED			= 0x80	//!< Instruction already visited
} InstructionInfoFlags;



//!Number of Basic Block edges that will come out from an instruction (for RET, Branch, or Jump instructions)
typedef enum{
	RET_OUT_EDGES			= 0,	//!< No outcoming edges
	JMP_OUT_EDGES			= 1,	//!< One outcoming edge
	BRANCH_OUT_EDGES		= 2		//!< Two outcoming edges
}OutEdgesEnum;



typedef struct SwInsnInfo
{
	uint32_t	NumCases;
	uint32_t	*CaseTargets;
	uint32_t	DefTarget;
	uint32_t   *Values;
}SwInsnInfo;

/*!
  \brief Instruction Information
 */


typedef struct InstructionInfo
{
	uint8_t			Opcode;				//!< Opcode of the instruction
	uint8_t			NumArgs;			//!< Number of arguments of the instruction
	uint32_t			Arguments[2];		//!< 1st Argument, or Target of the true path for a branch instruction
	uint16_t			NumPredecessors;	//!< number of incoming (backward) edges (if BB Leader)
	uint16_t			NumSuccessors;		//!< number of outcoming (forward) edges (if BB End)
	int32_t				StackDepth;			//!< stack depth before the execution of the instruction
	uint32_t			BCpc;				//!< Corresponding ByteCode's index
	uint32_t			SrcLine;			//!< Corresponding source line
	uint32_t			 Flags;				//!< Flags for instruction details
	uint32_t			StackAfter;			//!< Stack depth after the instruction
	//BasicBlock *		BBPtr;				//!< Pointer to the corresponding BasicBlock structure
	uint16_t			BasicBlockId;		//!< Id of the corresponding Basic Block
	SwInsnInfo			*SwInfo;			//!< Info for the SWITCH instruction
} InstructionInfo;



/*!
  \brief ByteCode Information
 */


typedef struct nvmJitByteCodeInfo
{
	InstructionInfo *	BCInfoArray;		//!< Pointer to an array of InstructionInfo elements
	uint32_t			NumInstructions;	//!< Number of instructions in the bytecode
	int32_t				MaxStackDepth;		//!< Maximum stack depth reached
	uint32_t			LocalsUse;			//!< Number of locals effectively used
	uint32_t			NumBasicBlocks;		//!< Number of basic blocks
	nvmByteCodeSegment	*Segment;			//!< Pointer to the bytecode segment descriptor
	uint32_t				*LocalsReferences;	//!< Locals References Array
	uint32_t				XchBufUse;			//!< Exchange Buffer Use
	uint32_t				DatMemUse;			//!< Data Memory Use
	uint32_t				ShdMemUse;			//!< Shared Memory Use
	uint32_t			AnalysisFlags;		//!< Details on the analysis result

} nvmJitByteCodeInfo;




/*!
  \brief Verifies and analyses a bytecode segment creating a nvmJitByteCodeInfo structure

  \param segment	pointer to a bytecode segment descriptor

  \param errList	pointer to the error list that evenutally will contain detailed error output

  \return a pointer to a nvmJitByteCodeInfo structure that will contain analysis results

  This function is performs the first compilation step. Only if it succeeds, the compilation process can
  go on with the other steps

*/

nvmJitByteCodeInfo * nvmJitAnalyse_ByteCode_Segment(nvmByteCodeSegment * segment, ErrorList * errList);

/*!
  \brief Verifies and analyses a bytecode segment creating a nvmJitByteCodeInfo structure

  \param segment	pointer to a bytecode segment descriptor

  \param errList	pointer to the error list that evenutally will contain detailed error output

  \param BBCount	pointer to a uint16_t value that holds the number the first BB will assume.

  \return a pointer to a nvmJitByteCodeInfo structure that will contain analysis results

  This function is performs the first compilation step. Only if it succeeds, the compilation process can
  go on with the other steps

*/

nvmJitByteCodeInfo * nvmJitAnalyse_ByteCode_Segment_Ex(nvmByteCodeSegment * segment, ErrorList * errList,
	uint16_t *BBCount);



/*!
  \brief Frees the memory allocated to a nvmJitByteCodeInfo structure

  \param BCInfo	pointer to a nvmJitByteCodeInfo structure

  \return NOTHING

*/

void nvmJitFree_ByteCode_Info(nvmJitByteCodeInfo * BCInfo);



uint32_t nvmJitIs_Segment_Empty(nvmJitByteCodeInfo *BCInfo);

/*!
	\}
*/

