/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file bytecode_segments.h
 * \brief This file contains prototypes and definition of routines to fill the bytecode segment info structure
 */

#pragma once


#include <nbnetvm.h>
#include <netvm_bytecode.h>
#include "../assembler/hashtable.h"
#include <map>


//!\brief Macros for bytecode pointers and arguments

//!Macro that gets a LONG given its address in memory
#define PTR_TO_LONG(ADDR)	(*(int32_t *)(ADDR))

//!Macro that gets the BYTE argument of a NetIL Instruction in the bytecode
#define BYTE_ARG(PC)	(*(int8_t *)(bytecode + PC + 1))	//!< the code at bytecode[pc+1] is treated as a char argument

//!Macro that gets the SHORT argument of a NetIL Instruction in the bytecode
#define SHORT_ARG(PC)	(*(int16_t *)(bytecode + PC + 1))	//!< the 2 codes at bytecode[pc+1] are treated as a short int argument

//!Macro that gets the LONG argument of a NetIL Instruction in the bytecode
#define LONG_ARG(PC)	(*(int32_t *)(bytecode + PC + 1))	//!< the 4 codes at bytecode[pc+1] are treated as a long int argument


/*!\brief Macros for absolute branch targets
	*
	* (+3) and (+5) are because the address argument of a branch is relative to the next
	* opcode, so for the translation to an absolute address in the bytecode we have to do
	* this kind of adjustment
*/
//!Macro that gets the index in the bytecode of a short jump or branch target
#define SHORT_JMP_TARGET(PC)	(BYTE_ARG(PC) + PC + 3)
//!Macro that gets the index in the bytecode of a long jump or branch target
#define LONG_JMP_TARGET(PC)		(LONG_ARG(PC) + PC + 5)


/* Code segments */
#define SEGMENT_HEADER_LEN	8		//!< 12 bytes, see below
#define MAX_STACK_SIZE_OFFS	0		//!< displacement in the bytecode of the Max Stack Size
#define LOCALS_SIZE_OFFS	4		//!< displacement in the bytecode of the Locals Size

#define EXTRACT_BC_SIGNATURE(dst,src)	do{strncpy( (dst) , (src) , 4 ); (dst)[4]='\0';}while(0)


#ifdef __cplusplus
extern "C" {
#endif



//!Bytecode Segment Types
typedef enum
{
	PULL_SEGMENT,
	PUSH_SEGMENT,
	INIT_SEGMENT,
	PORT_SEGMENT
} SegmentTypeEnum;


/*!
  \brief Descriptor of a NetVM bytecode Segment: {.init, .push, .pull, .ports}
*/
typedef struct nvmByteCodeSegment
{

	char						*Name;			//!< The name of the segment
	char						*PEName;
	char						*PEFileName;
	uint8_t					*ByteCode;		//!< Pointer to the bytecode of the segment
	uint32_t					SegmentSize;	//!< Size of the segment
	uint32_t					LocalsSize;		//!< Locals Region maximum Size
	uint32_t					MaxStackSize;	//!< Maximum Stack Size
	SegmentTypeEnum				SegmentType;	//!< Type of the segment.
	std::map<uint32_t,uint32_t>	Insn2LineMap;	//!< Instruction to line mapping
} nvmByteCodeSegment;



/*!
  \brief Bytecode Segments Information
*/

typedef struct nvmByteCodeSegmentsInfo
{
	nvmByteCodeSegment	PushSegment;//!< The Push Segment descriptor
	nvmByteCodeSegment	PullSegment;//!< The Pull Segment descriptor
	nvmByteCodeSegment	InitSegment;//!< The Init Segment descriptor
	nvmByteCodeSegment	PortSegment;//!< The Port Segment descriptor
} nvmByteCodeSegmentsInfo;



/*!
  \brief  Fills a nvmByteCodeSegmentsInfo structure with bytecode segments information

  \param  segmentsInfo		pointer to a nvmByteCodeSegmentsInfo structure

  \param  bytecode			pointer to a nvmByteCode structure containing the bytecode

  \return nvmSUCCESS if it succeeds, nvmFAILURE if it fails
*/

int32_t nvmJitFill_Segments_Info(nvmByteCodeSegmentsInfo *segmentsInfo, nvmByteCode *bytecode, char *errbuf);


#ifdef __cplusplus
}
#endif
