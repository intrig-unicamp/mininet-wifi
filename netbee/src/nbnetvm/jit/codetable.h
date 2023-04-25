/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

/** @file codetable.h
 * \brief This file contains the definition of the NetVM Inverse Opcodes Table management and its initialization function
 *
 *		  The Opcode Table serves to get a NetIL opcode descriptor from its numerical code
 */


#include <stdint.h>
#include "../opcode_consts.h"
#include "netvmjitglobals.h"

#ifdef __cplusplus
extern "C" {
#endif

#define T_NO_ARG_INST			0	//!< No arguments to instruction
#define T_1BYTE_ARG_INST		1	//!< 1 BYTE argument
#define T_1_SHORT_LABEL_INST	1	//!< 1 BYTE (1 byte) address
#define T_1WORD_ARG_INST		2	//!< 2 BYTE (1 word) argument
#define T_1LABEL_INST			4	//!< 4 BYTE (1 Long) address
#define T_1INT_ARG_INST			4	//!< 4 BYTE (1 Long) argument
#define T_2INT_ARG_INST			8	//!< 8 BYTE (2 Long)	argument
#define T_1_PUSH_PORT_ARG_INST	4	//!< 4 BYTE (1 Long) argument
#define T_1_PULL_PORT_ARG_INST  4	//!< 4 BYTE (1 Long) argument
#define T_2_COPRO_IO_ARG_INST	8
#define T_COPRO_INIT_INST	8
#define T_JMP_TBL_LKUP_INST		-1	//!< this corresponds to a switch statement


#define OPCODE_TABLE_LEN		256	//!< 1 BYTE per instruction => 256 entries
#define OPCODE_NAME_LEN			64	//!< Maximum length for the opcode name
#define OPCODE_NAME_INVALID		"INVALID"



/*!
  \brief Opcode Descriptor
 */

typedef struct OpCodeDescriptor
{
	char		CodeName[OPCODE_NAME_LEN];	//!< The Opcode Name
	uint8_t	ArgLen;						//!< The length in bytes of its args
	uint8_t	Opcode;						//!< The opcode
	uint32_t	Consts;						//!< The constansts of the opcode
	uint32_t	Flags;						//!< Flags
} OpCodeDescriptor;



//! The Opcode Table is an array of 256 OpcodeDescriptors for the translation from a numerical opcode to its description

extern OpCodeDescriptor nvmOpCodeTable[];
extern uint32_t OpcodeTableInited;



/*!
  \brief  Initializes the Opcode Table

		  It's the very first function to be called from the JIT

  \return Nothing
*/

void nvmInitOpcodeTable(void);
char *GetInstructionName(uint8_t opcode);
uint32_t GetInstructionLen(uint8_t *bytecode);
uint32_t GetInstructionArgLen(uint8_t opcode);
uint32_t Get_Instruction_Type(uint8_t opcode);
int32_t	Is_Memory_Operation(uint8_t opcode);
uint32_t Get_Operators_Count(uint8_t opcode);
uint32_t Get_Defined_Count(uint8_t opcode);

#ifdef __cplusplus
}
#endif

