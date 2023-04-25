/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file codetable.c
 * \brief This file contains the functions for the opcode table management
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "codetable.h"
#include <../opcodes.h>


OpCodeDescriptor nvmOpCodes[OPCODE_TABLE_LEN] =
{
	#define nvmOPCODE(id, name, pars, code, consts, desc)  {name, pars, code, consts},
		#include "../opcodes.txt"
		#include "../newopcodes.txt"
	#undef nvmOPCODE
};

uint32_t OpcodeTableInited = 0;

OpCodeDescriptor nvmOpCodeTable[OPCODE_TABLE_LEN] =
{
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
	{OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0}, {OPCODE_NAME_INVALID, 0, 0, 0},
};



void nvmInitOpcodeTable(void)
{
	uint32_t i = 0;
	uint8_t opcode = 0;
	for (i = 0; i < OPCODE_TABLE_LEN; i++){
		if(nvmOpCodes[i].CodeName[0] != 0)
		{
			opcode = nvmOpCodes[i].Opcode;
			nvmOpCodeTable[opcode].Opcode = nvmOpCodes[i].Opcode;
			nvmOpCodeTable[opcode].ArgLen = nvmOpCodes[i].ArgLen;
			strcpy(nvmOpCodeTable[opcode].CodeName, nvmOpCodes[i].CodeName);
			nvmOpCodeTable[opcode].Consts = nvmOpCodes[i].Consts;
		}
#if 0
		// XXX: sanity check per roba che viene dopo.
		/* If there are any operators that trigger this condition then they will NOT be correctly
		 * treated during the memory linearization stage because we DO NOT KNOW which one of their
		 * kids holds the/an address and which one is NOT.
		 */
		assert(!(Is_Memory_Operation(nvmOpCodes[i].Opcode) && Get_Operators_Count(nvmOpCodes[i].Opcode) == TWO_OPS &&
			((nvmOpCodes[i].Consts & OP_STORE) != OP_STORE)));
#endif
	}
	OpcodeTableInited = 1;

}


char *GetInstructionName(uint8_t opcode)
{
	return nvmOpCodeTable[opcode].CodeName;
}

uint32_t GetInstructionArgLen(uint8_t opcode)
{
	return nvmOpCodeTable[opcode].ArgLen;
}


uint32_t GetInstructionLen(uint8_t *bytecode)
{
	uint32_t len = 0;
	uint32_t ncases = 0;
	uint8_t opcode = *bytecode;

	if (*bytecode != SWITCH)
	{
		len = nvmOpCodeTable[opcode].ArgLen + 1;
	}
	else
	{
		len += 5; //opcode + default target
		ncases = *(uint32_t*)&bytecode[len];
		len += 4 + 8 * ncases;
	}
	//printf("opcode = %s (0x%x), len = %d\n", nvmOpCodeTable[opcode].CodeName, opcode, len);
	return len;
}

uint32_t Get_Instruction_Type(uint8_t opcode)
{
	return nvmOpCodeTable[opcode].Consts & OPTYPE_MASK;
}

int	Is_Memory_Operation(uint8_t opcode)
{
	uint32_t op = nvmOpCodeTable[opcode].Consts & OPCODE_MASK;
	if ( (op == OP_LOAD) || (op == OP_STORE) || (op == OP_LLOAD) || (op == OP_LSTORE) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

uint32_t	Get_Operators_Count(uint8_t opcode)
{
	return nvmOpCodeTable[opcode].Consts & OP_COUNT_MASK;
}

uint32_t	Get_Defined_Count(uint8_t opcode)
{
	return nvmOpCodeTable[opcode].Consts & OP_DEF_MASK;
}
