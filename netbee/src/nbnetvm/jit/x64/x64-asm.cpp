/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/** @file x64-asm.cpp
 * \brief This file contains functions for the creation of x64-assembly instruction sequences
 *
 */

#include "x64-asm.h"
#include "x64-emit.h"
#include "registers.h"
#include "netvmjitglobals.h"

#include "../../../nbee/globals/debug.h"
#include <iomanip>

using namespace jit;
using namespace x64;
using namespace std;

namespace jit{
	namespace x64{

		x64OpDescr x64OpDescriptions[] =
		{

#define X64_ASM(code, mnemonic, flags, defs, uses, emitfunction, description)	{mnemonic, x64OpDescrFlags(flags), defs, uses, emitfunction},
#include "x64-asm.def"
#undef X64_ASM

		};


		char *x64RegNames[] =
		{
			"al", "cl", "dl", "bl", "ah", "ch",	"dh", "bh",
			"r8l", "r9l", "r10l", "r11l", "r12l", "r13l",	"r14l", "r15l",
			"ax", "cx",	"dx", "bx", "sp", "bp",	"si", "di",
			"r8w", "r9w", "r10w", "r11w", "r12w", "r13w",	"r14w", "r15w",
			"eax","ecx","edx","ebx","esp","ebp","esi","edi",
			"r8d", "r9d", "r10d", "r11d", "r12d", "r13d",	"r14d", "r15d",
			"rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
			"r8", "r9", "r10", "r11", "r12", "r13",	"r14", "r15",
		};


		char *x64CC[] =
		{
			"o",		//Overflow
			"no",		//No Overflow
			"b",		//Below
			"ae",		//Above Or Equal = Not Below
			"e",		//Equal
			"ne",		//Not Equal
			"be",		//Below Or Equal
			"a",		//Above
			"s",		//Sign
			"ns",		//Not Sign
			"p",		//Parity
			"np",		//Not Parity
			"l",		//Less Than
			"ge",		//Greater Or Equal = Not Less Than
			"le",		//Less Or Equal
			"g"			//Greater = Not Less Or Equal
		};

		static void printRegister(std::ostream& os, x64RegOpnd reg, uint32_t size);

		static int32_t x64_Asm_Append_Operand(Px64Instruction insn, void *operand);
		int32_t x64_Asm_Append_ImmOp(Px64Instruction insn, uint64_t immVal);
		int32_t x64_Asm_Append_ImmOp64(Px64Instruction insn, uint64_t immVal);
		int32_t x64_Asm_Append_LblOp(Px64Instruction insn, uint32_t label);

		static Px64Instruction x64_Asm_Enqueue_Insn(Px64InsnSequence x64InsnSeq, uint16_t code);

	} //namespace x64
} //namespace jit

Px64Instruction jit::x64::x64_Asm_New_Op(x64OpCodesEnum code)
{
	NETVM_ASSERT(OP_ONLY(code) <= X64_COMMENT, "Trying to create an invalid x64 instruction!");

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Creating new x64 instruction: %s", x64OpDescriptions[OP_ONLY(code)].Name);
#endif

	return new x64Instruction(code);
}

int32_t jit::x64::x64_Asm_Append_Operand(Px64Instruction insn, void *operand)
{
	if (insn == NULL || operand == NULL)
		return nvmJitFAILURE;

	if (insn->NOperands >= 2){
		NETVM_ASSERT(1 == 0, "Error Appending operand to x64 instruction");
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands++].Op = operand;

	return nvmJitSUCCESS;
}

int32_t jit::x64::x64_Asm_Append_RegOp(Px64Instruction insn, x64RegOpnd reg, x64OpndSz size)
{
	if (x64_Asm_Append_Operand(insn, new x64RegOpnd(reg)) == nvmJitFAILURE){
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = REG_OP;
	insn->Operands[insn->NOperands - 1].Size = size;

	return nvmJitSUCCESS;

}

int32_t jit::x64::x64_Asm_Append_XMMOp(Px64Instruction insn, x64RegOpnd reg)
{
	if (x64_Asm_Append_Operand(insn, new x64RegOpnd(reg)) == nvmJitFAILURE){
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = XMM_OP;
	insn->Operands[insn->NOperands - 1].Size = x64_QWORD;

	return nvmJitSUCCESS;

}

int32_t jit::x64::x64_Asm_Append_ImmOp(Px64Instruction insn, uint64_t immVal)
{
	Px64ImmOpnd	immOp;

	immOp = new x64ImmOpnd();
	if (immOp == NULL)
		return nvmJitFAILURE;

	immOp->Value = immVal;

	if (x64_Asm_Append_Operand(insn, immOp) < 0){
		delete immOp;
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = IMM_OP;
	insn->Operands[insn->NOperands - 1].Size = x64_DWORD;

	return nvmJitSUCCESS;

}

int32_t jit::x64::x64_Asm_Append_ImmOp64(Px64Instruction insn, uint64_t immVal)
{
	Px64ImmOpnd	immOp;

	immOp = new x64ImmOpnd();
	if (immOp == NULL)
		return nvmJitFAILURE;

	immOp->Value = immVal;

	if (x64_Asm_Append_Operand(insn, immOp) < 0){
		delete immOp;
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = IMM_OP;
	insn->Operands[insn->NOperands - 1].Size = x64_QWORD;

	return nvmJitSUCCESS;

}


int32_t jit::x64::x64_Asm_Append_LblOp(Px64Instruction insn, uint32_t label)
{
	Px64LblOpnd	lblOp;

	lblOp = new x64LblOpnd();
	if (lblOp == NULL)
		return nvmJitFAILURE;

	lblOp->Label = label;

	if (x64_Asm_Append_Operand(insn, lblOp) < 0){
		delete lblOp;
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = LBL_OP;
	insn->Operands[insn->NOperands - 1].Size = x64_DWORD;

	return nvmJitSUCCESS;
}

int32_t jit::x64::x64_Asm_Append_MemOp(Px64Instruction insn, x64RegOpnd base, x64RegOpnd index, uint64_t displ, uint8_t scale, x64OpndSz size, x64MemFlags memFlags)
{
	Px64MemOpnd	memOp;

	memOp =  new x64MemOpnd();
	if (memOp == NULL)
		return nvmJitFAILURE;

	memOp->Base = base;
	memOp->Index = index;
	memOp->Displ = displ;
	memOp->Scale = scale;
	memOp->Flags = memFlags;

	if (x64_Asm_Append_Operand(insn, memOp) < 0){
		delete memOp;
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = MEM_OP;
	insn->Operands[insn->NOperands - 1].Size = size;
	return nvmJitSUCCESS;

}


void jit::x64::x64_Asm_Append_Comment(Px64Instruction insn, const char *comment)
{
	_snprintf(insn->Comment, X64_COMMENT_LEN - 1, "%s", comment);
}

Px64Instruction jit::x64::x64_Asm_Enqueue_Insn(Px64InsnSequence x64InsnSeq, uint16_t code)
{
	Px64Instruction instruction;

	instruction = x64_Asm_New_Op((x64OpCodesEnum)code);
	if (instruction == NULL){
		NETVM_ASSERT(1 == 0, "Error creating x64 instruction");
		return NULL;
	}

	x64InsnSeq.push_back(instruction);
	return instruction;
}


Px64Instruction jit::x64::x64_Asm_Comment(Px64InsnSequence x64InsnSeq, const char *comment)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64InsnSeq, X64_COMMENT);
	if (x64Insn == NULL)
		return NULL;

	x64_Asm_Append_Comment(x64Insn, comment);
	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;

	return x64Insn;
}


Px64Instruction jit::x64::x64_Asm_Op_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL){
		return NULL;
	}
	if (x64_Asm_Append_RegOp(x64Insn, srcReg, opSize) < 0 )
		return NULL;

	return x64Insn;
}


Px64Instruction jit::x64::x64_Asm_Op_Imm(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_ImmOp(x64Insn, immVal) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op_Mem_Base(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, uint32_t displ, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;

	if (x64_Asm_Append_MemOp(x64Insn, baseReg, RegisterInstance(), displ, 0, opSize, x64MemFlags(BASE | DISPL)) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op_Mem_Index(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, x64RegOpnd indexReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, indexReg, 0, 0, opSize, x64MemFlags(BASE | INDEX)) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op_Reg_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64RegOpnd dstReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;
	
	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL){
		return NULL;
	}
	
	if (x64_Asm_Append_RegOp(x64Insn, dstReg, opSize) < 0 )
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, srcReg, opSize) < 0 )
		return NULL;

	return x64Insn;
}


Px64Instruction jit::x64::x64_Asm_Op_Imm_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint64_t immVal, x64RegOpnd dstReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, dstReg, opSize) < 0 )
		return NULL;
	if (opSize == x64_BYTE){
		if (x64_Asm_Append_ImmOp(x64Insn, (uint8_t)immVal) < 0 )
			return NULL;
	}
	else if (opSize == x64_WORD){
		if (x64_Asm_Append_ImmOp(x64Insn, (uint16_t)immVal) < 0 )
			return NULL;
	}
	else if (opSize == x64_DWORD){
		if (x64_Asm_Append_ImmOp(x64Insn, (uint32_t)immVal) < 0 )
			return NULL;
	}
	else{
		if (x64_Asm_Append_ImmOp(x64Insn, immVal) < 0 )
			return NULL;
	}

	return x64Insn;
}


Px64Instruction jit::x64::x64_Asm_Op_Mem_Base_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, uint32_t displ, x64RegOpnd dstReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, dstReg, opSize) != nvmJitSUCCESS)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, RegisterInstance(), displ, 0, opSize, x64MemFlags(BASE | DISPL)) != nvmJitSUCCESS)
		return NULL;

	return x64Insn;
}


Px64Instruction jit::x64::x64_Asm_Op_Reg_To_Mem_Base(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64RegOpnd baseReg, uint32_t displ, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, RegisterInstance(), displ, 0, opSize, x64MemFlags(BASE | DISPL)) != nvmJitSUCCESS )
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, srcReg, opSize) != nvmJitSUCCESS )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op_Mem_Index_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, x64RegOpnd indexReg, uint32_t displ, uint8_t scale, x64RegOpnd dstReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, dstReg, opSize) < 0 )
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, indexReg, displ, scale, opSize, x64MemFlags(BASE | INDEX)) < 0 )
		return NULL;

	return x64Insn;
}


Px64Instruction jit::x64::x64_Asm_Op_Reg_To_Mem_Index(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64RegOpnd baseReg, x64RegOpnd indexReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, indexReg, 0, 0, opSize, x64MemFlags(BASE | INDEX)) < 0 )
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, srcReg, opSize) < 0 )
		return NULL;

	return x64Insn;
}



Px64Instruction jit::x64::x64_Asm_Op_Imm_To_Mem_Base(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal, x64RegOpnd baseReg, uint32_t displ, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, RegisterInstance(), displ, 0, opSize, x64MemFlags(BASE | DISPL)) < 0 )
		return NULL;
	if (x64_Asm_Append_ImmOp(x64Insn, immVal) < 0 )
		return NULL;

	return x64Insn;
}



Px64Instruction jit::x64::x64_Asm_Op_Imm_To_Mem_Index(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal, x64RegOpnd baseReg, x64RegOpnd indexReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, indexReg, 0, 0, opSize, x64MemFlags(BASE | INDEX)) < 0 )
		return NULL;
	if (x64_Asm_Append_ImmOp(x64Insn, immVal) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op_Mem_Displ_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t displ, x64RegOpnd dstReg, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, dstReg, opSize) < 0 )
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, RegisterInstance(), RegisterInstance(), displ, 0, opSize, DISPL) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op_Reg_To_Mem_Displ(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, uint64_t displ, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, RegisterInstance(), RegisterInstance(), displ, 0, opSize, DISPL) < 0 )
		return NULL;
	if (x64_Asm_Append_RegOp(x64Insn, srcReg, opSize) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Op_Imm_To_Mem_Displ(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal, uint64_t displ, x64OpndSz opSize)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, x64OpCode);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, RegisterInstance(), RegisterInstance(), displ, 0, opSize, DISPL) < 0 )
		return NULL;
	if (x64_Asm_Append_ImmOp(x64Insn, immVal) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Load_Current_Memory_Location(Px64InsnSequence x64CodeSeq, x64RegOpnd dstReg)
{
	Px64Instruction x64Insn = x64_Asm_Op_Imm_To_Reg(x64CodeSeq, X64_MOV, 0, dstReg, x64_QWORD);
	x64_Asm_Append_Comment(x64Insn, "Load current istruction addres");
	x64Insn->load_current_address = true;
	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_JMP_Indirect(Px64InsnSequence x64CodeSeq, x64RegOpnd baseReg, uint32_t displ, x64OpndSz size)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, X64_JMP);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_MemOp(x64Insn, baseReg, RegisterInstance(), displ, 0, size, x64MemFlags(DISPL|BASE)) < 0 )
		return NULL;
	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_JMP_Label(Px64InsnSequence x64CodeSeq, uint32_t basicBlockLabel)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, X64_JMP);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_LblOp(x64Insn, basicBlockLabel) < 0 )
		return NULL;

	return x64Insn;

}

Px64Instruction jit::x64::x64_Asm_J_Label(Px64InsnSequence x64CodeSeq, uint8_t condCode, uint32_t basicBlockLabel)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, OP_CC(X64_J, condCode));
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_LblOp(x64Insn, basicBlockLabel) < 0 )
		return NULL;

	return x64Insn;

}

Px64Instruction jit::x64::x64_Asm_CALL_NetVM_API(Px64InsnSequence x64CodeSeq, void *functAddress)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, X64_CALL);
	if (x64Insn == NULL)
		return NULL;
	if (x64_Asm_Append_ImmOp64(x64Insn, (uint64_t)functAddress) < 0 )
		return NULL;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Switch_Table_Entry(Px64InsnSequence x64CodeSeq, uint32_t target)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, X64_SW_TABLE_ENTRY);
	if (x64Insn == NULL)
		return NULL;

	x64Insn->switch_target = target;

	return x64Insn;
}

Px64Instruction jit::x64::x64_Asm_Switch_Table_Entry_Start(Px64InsnSequence x64CodeSeq, Px64Instruction entry)
{
	Px64Instruction x64Insn;

	x64Insn = x64_Asm_Enqueue_Insn(x64CodeSeq, X64_SW_TABLE_ENTRY_START);
	if (x64Insn == NULL)
		return NULL;

	x64Insn->switch_entry = entry;

	return x64Insn;
}

std::set<x64Instruction::RegType > jit::x64::x64Instruction::getUses()
{
	std::set<x64Instruction::RegType> res;

	int32_t i = 0, usedOpnds = 0	;


	if (OpDescr->Uses == 0 || NOperands == 0)
		return res;

	if (OpDescr->Flags & U_EAX)
		res.insert(RegisterInstance(MACH_SPACE, EAX));

	if (OpDescr->Flags & U_ECX)
		res.insert(RegisterInstance(MACH_SPACE, ECX));

	if (OpDescr->Flags & U_EDX)
		res.insert(RegisterInstance(MACH_SPACE, EDX));

	if (OpDescr->Flags & U_EDI)
		res.insert(RegisterInstance(MACH_SPACE, EDI));

	if (OpDescr->Flags & U_ESI)
		res.insert(RegisterInstance(MACH_SPACE, ESI));

/*
 	for (i = 0; i < NOperands; i++)
	{

		if (Operands[i].Type == REG_OP && i >= OpDescr->Defs)
		{
			x64RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
			res.insert(reg);
		}
		else if(Operands[i].Type == MEM_OP)
		{
			if (MEM_OPERAND(Operands[i].Op)->Flags & BASE)
			{
				x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
				res.insert(reg);
			}
			if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX)
			{
				x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
				res.insert(reg);
			}
		}
	}
*/

	for (i = NOperands - 1; i >= 0 && usedOpnds < OpDescr->Uses; i--){

		if (Operands[i].Type == REG_OP){
			x64RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
			res.insert(reg);
		}
		else if(Operands[i].Type == MEM_OP){
			if (MEM_OPERAND(Operands[i].Op)->Flags & BASE){
				x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
				res.insert(reg);
			}
			if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
				x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
				res.insert(reg);
			}
		}
		usedOpnds++;
	}

	//some operands left if they are mem operands actually use some registers
	while(i >= 0)
	{
		if(Operands[i].Type == MEM_OP){
			if (MEM_OPERAND(Operands[i].Op)->Flags & BASE){
				x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
				res.insert(reg);
			}
			if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
				x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
				res.insert(reg);
			}
		}

		i--;
	}
/*
	for (i = NOperands - 1; i >= 0 && usedOpnds < OpDescr->Uses; i--){

			if (Operands[i].Type == REG_OP){
				x64RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
				res.insert(reg);
			}
			else if(Operands[i].Type == MEM_OP){
				if (MEM_OPERAND(Operands[i].Op)->Flags & BASE){
					x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
					res.insert(reg);
				}
				if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
					x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
					res.insert(reg);
				}
			}
			usedOpnds++;
		}

		//some operands left if they are mem operands actually use some registers
		while(i >= 0)
		{
			if(Operands[i].Type == MEM_OP){
				if (MEM_OPERAND(Operands[i].Op)->Flags & BASE){
					x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
					res.insert(reg);
				}
				if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
					x64RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
					res.insert(reg);
				}
			}

			i--;
		}
*/
	return res;
}

std::set<x64Instruction::RegType> jit::x64::x64Instruction::getDefs()
{
	std::set<x64Instruction::RegType> res;

	uint32_t i;
	x64RegOpnd reg;


	if (OpDescr->Flags & M_EAX)
		res.insert(RegisterInstance(MACH_SPACE, EAX));

	if (OpDescr->Flags & M_ECX)
		res.insert(RegisterInstance(MACH_SPACE, ECX));

	if (OpDescr->Flags & M_EDX)
		res.insert(RegisterInstance(MACH_SPACE, EDX));

	if (OpDescr->Flags & M_EDI)
		res.insert(RegisterInstance(MACH_SPACE, EDI));

	if (OpDescr->Flags & M_ESI)
		res.insert(RegisterInstance(MACH_SPACE, ESI));

	if (OpDescr->Flags & M_R8)
		res.insert(RegisterInstance(MACH_SPACE, R8));

	if (OpDescr->Flags & M_R9)
		res.insert(RegisterInstance(MACH_SPACE, R9));

	if (OpDescr->Flags & M_R10)
		res.insert(RegisterInstance(MACH_SPACE, R10));

	if (OpDescr->Flags & M_R11)
		res.insert(RegisterInstance(MACH_SPACE, R11));

	for (i = 0; i < OpDescr->Defs && i < NOperands; i++)
	{
		if (Operands[i].Type == REG_OP)
		{
			x64RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
			res.insert(reg);
		}
	}

	return res;
}

bool jit::x64::x64Instruction::isCopy() const
{
	if (OpCode == (uint16_t)X64_MOV && NOperands == 2){
		if (Operands[0].Type == REG_OP && Operands[1].Type == REG_OP){
			return true;
		}
	}
	return false;
}

jit::x64::x64Instruction::RegType
jit::x64::x64Instruction::get_to() const
{
	return *REG_OPERAND(Operands[0].Op);
}

jit::x64::x64Instruction::RegType
jit::x64::x64Instruction::get_from() const
{
	return *REG_OPERAND(Operands[1].Op);
}

std::list< std::pair<RegisterInstance, RegisterInstance> > jit::x64::x64Instruction::getCopiedPair()
{
	std::list< std::pair<RegType, RegType> > res;

	if(isCopy())
	{
		res.push_back( std::pair<RegType, RegType>(get_to(), get_from()));
	}

	return res;
}

bool jit::x64::x64Instruction::has_side_effects()
{
	if(this->getOpcode() == X64_BSWAP)
		return true;
	return false;
}
void jit::x64::x64Instruction::rewrite_Reg(jit::x64::x64Instruction::RegType oldreg, jit::x64::x64Instruction::RegType *newreg)
{
//	for (int i = 0; i < OpDescr->Defs && i < NOperands; i++)
	for (int i = 0; i < NOperands; i++)
	{

		if (Operands[i].Type == REG_OP && oldreg == *REG_OPERAND(Operands[i].Op))
		{
			Operands[i].Op = newreg;
		}

		else if(Operands[i].Type == MEM_OP){

			if (MEM_OPERAND(Operands[i].Op)->Flags & BASE
					&& oldreg == MEM_OPERAND(Operands[i].Op)->Base)
			{
				MEM_OPERAND(Operands[i].Op)->Base = *newreg;
			}
			if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX
					&& oldreg == MEM_OPERAND(Operands[i].Op)->Index)
			{
				MEM_OPERAND(Operands[i].Op)->Index = *newreg;
			}
		}
	}
}

void jit::x64::printRegister(std::ostream& os, x64RegOpnd reg, uint32_t size)
{
	//os << reg.get_model()->get_name() + size * 8;
	if(X64_IS_MACH_REG(reg))
		os << x64RegNames[reg.get_model()->get_name() + size * 16];
	else
		os << "r"<< reg.get_model()->get_space() << "." << reg.get_model()->get_name() << "." << reg.version();
}


std::ostream& jit::x64::operator<<(std::ostream& os, x64Operand& op)
{
	if(op.Type == IMM_OP)
	{
		os << std::hex <<"0x" << IMM_OPERAND(op.Op)->Value <<std::dec;
	}
	else if(op.Type == LBL_OP)
	{
		os << "lbl" << ((Px64LblOpnd)op.Op)->Label;
	}
	else if(op.Type == MEM_OP)
	{
		switch (op.Size){
			case x64_BYTE:
				os << "byte ptr[";
				break;
			case x64_WORD:
				os << "word ptr[";
				break;
			case x64_DWORD:
				os <<  "dword ptr[";
				break;
			case x64_QWORD:
				os <<  "qword ptr[";
				break;
		}
		if (((Px64MemOpnd)op.Op)->Flags & BASE){
			printRegister(os, MEM_OPERAND(op.Op)->Base, x64_QWORD);
		}
		if (((Px64MemOpnd)op.Op)->Flags & INDEX){
			if (((Px64MemOpnd)op.Op)->Flags & BASE)
				os << " + ";
			printRegister(os, MEM_OPERAND(op.Op)->Index, x64_QWORD);
		}
		if (((Px64MemOpnd)op.Op)->Flags & INDEX && ((Px64MemOpnd)op.Op)->Scale != 0){
			os << " * " << hex << (1 << (int) (MEM_OPERAND(op.Op)->Scale)) << dec;
		}
		if (((Px64MemOpnd)op.Op)->Displ != 0){
			if((int64_t)MEM_OPERAND(op.Op)->Displ != 0) {
				if ((((Px64MemOpnd)op.Op)->Flags & (BASE | INDEX)))
				    os << " + ";
				os << hex << "0x" << MEM_OPERAND(op.Op)->Displ << dec;
			}
		}
		os << "]";
	}
	else if(op.Type == XMM_OP)
	{
	      os << "xmm" << (*REG_OPERAND(op.Op)).get_model()->get_name();
	}
	else
	{
		printRegister(os, *REG_OPERAND(op.Op), op.Size);
	}

	return os;
}

void jit::x64::_X64_INSTRUCTION::printNode(std::ostream& os, bool SSAform)
{
	os << *this;
}

std::ostream& jit::x64::operator<<(std::ostream& os, _X64_INSTRUCTION& insn)
{
	if(insn.getOpcode() == X64_COMMENT)
	{
		return os << "; " << insn.Comment;
	}

	{
		ostringstream name;
		name << insn.OpDescr->Name;
		name << (insn.OpDescr->Flags & NEED_CC ? x64CC[CC_ONLY(insn.getOpcode())] : "" );

		os << left << setw(15) << name.str();
	}

	{
		ostringstream operands;
		for (uint8_t i = 0; i < insn.NOperands; i++)
		{
			operands << insn.Operands[i] << (i + 1 == insn.NOperands ? "" : ", ") ;
		}
		os << left << setw(35) << operands.str();
	}

	if(insn.Comment[0] != '\0')
		os << ";" << insn.Comment;

	return os;
}


_X64_INSTRUCTION*& jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT::operator*()
{
	return ptr;
}

_X64_INSTRUCTION*& jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT::operator->()
{
	return ptr;
}

jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT& jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT::operator++(int)
{
	ptr = NULL;
	return *this;
}

bool jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT::operator==(const _X64_INSTRUCTION_IT& it) const
{
	return ptr == it.ptr;
}

bool jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT::operator!=(const _X64_INSTRUCTION_IT& it) const
{
	return ptr != it.ptr;
}

jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT jit::x64::_X64_INSTRUCTION::nodeBegin()
{
	_X64_INSTRUCTION_IT it(this);
	return it;
}


jit::x64::_X64_INSTRUCTION::_X64_INSTRUCTION_IT jit::x64::_X64_INSTRUCTION::nodeEnd()
{
	_X64_INSTRUCTION_IT it;
	return it;
}
bool jit::x64::operator==(x64Operand a, x64Operand b)
{
	if(a.Type != b.Type || a.Size != b.Size)
		return false;
	switch(a.Type)
	{
		case REG_OP:
		case XMM_OP:
			return (*((Px64RegOpnd)a.Op)) == (*((Px64RegOpnd)b.Op));
			break;
		case IMM_OP:
			return (*((Px64ImmOpnd)a.Op)) == (*((Px64ImmOpnd)b.Op));
			break;
		case MEM_OP:
			return (*((Px64MemOpnd)a.Op)) == (*((Px64MemOpnd)b.Op));
			break;
		case LBL_OP:
			return (*((Px64LblOpnd)a.Op)) == (*((Px64LblOpnd)b.Op));
			break;
		default:
			return false;
	}
	return false;

}

bool jit::x64::operator==(x64ImmOpnd a, x64ImmOpnd b)
{
	return a.Value == b.Value;
}

bool jit::x64::operator==(x64LblOpnd a, x64LblOpnd b)
{
	return a.Label == b.Label;
}

bool jit::x64::operator==(x64MemOpnd a, x64MemOpnd b)
{
	if(a.Base != b.Base
			|| a.Index != b.Index
			|| a.Displ != b.Displ
			|| a.Scale != b.Scale
			|| a.Flags != b.Flags)
		return false;
	return true;
}
