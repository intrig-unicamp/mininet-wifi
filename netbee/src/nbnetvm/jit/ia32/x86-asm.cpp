/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/** @file x86-asm.cpp
 * \brief This file contains functions for the creation of x86-assembly instruction sequences
 *
 */

#include "x86-asm.h"
#include "x86-emit.h"
#include "registers.h"
#include "netvmjitglobals.h"

#include "../../../nbee/globals/debug.h"
#include <iomanip>

using namespace jit;
using namespace ia32;
using namespace std;

namespace jit{
	namespace ia32{

		x86OpDescr x86OpDescriptions[] =
		{

#define X86_ASM(code, mnemonic, flags, defs, uses, emitfunction, description)	{mnemonic, x86OpDescrFlags(flags), defs, uses, emitfunction},
#include "x86-asm.def"
#undef X86_ASM

		};


		char *x86RegNames[] =
		{
			"al", "cl", "dl", "bl", "ah", "ch",	"dh", "bh",
			"ax", "cx",	"dx", "bx", "sp", "bp",	"si", "di",
			"eax","ecx","edx","ebx","esp","ebp","esi","edi"
		};


		char *x86CC[] =
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

		static void printRegister(std::ostream& os, x86RegOpnd reg, uint32_t size);

		static int32_t x86_Asm_Append_Operand(Px86Instruction insn, void *operand);
		int32_t x86_Asm_Append_ImmOp(Px86Instruction insn, uint32_t immVal);
		int32_t x86_Asm_Append_LblOp(Px86Instruction insn, uint32_t label);

		static Px86Instruction x86_Asm_Enqueue_Insn(Px86InsnSequence x86InsnSeq, uint16_t code);

	} //namespace ia32
} //namespace jit

Px86Instruction jit::ia32::x86_Asm_New_Op(x86OpCodesEnum code)
{
	NETVM_ASSERT(OP_ONLY(code) <= X86_COMMENT, "Trying to create an invalid x86 instruction!");

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Creating new x86 instruction: %s", x86OpDescriptions[OP_ONLY(code)].Name);
#endif

	return new x86Instruction(code);
}

int32_t jit::ia32::x86_Asm_Append_Operand(Px86Instruction insn, void *operand)
{
	if (insn == NULL || operand == NULL)
		return nvmJitFAILURE;

	if (insn->NOperands >= 2){
		NETVM_ASSERT(1 == 0, "Error Appending operand to x86 instruction");
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands++].Op = operand;

	return nvmJitSUCCESS;
}

int32_t jit::ia32::x86_Asm_Append_RegOp(Px86Instruction insn, x86RegOpnd reg, x86OpndSz size)
{
	if (x86_Asm_Append_Operand(insn, new x86RegOpnd(reg)) == nvmJitFAILURE){
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = REG_OP;
	insn->Operands[insn->NOperands - 1].Size = size;

	return nvmJitSUCCESS;

}

int32_t jit::ia32::x86_Asm_Append_ImmOp(Px86Instruction insn, uint32_t immVal)
{
	Px86ImmOpnd	immOp;

	immOp = new x86ImmOpnd();
	if (immOp == NULL)
		return nvmJitFAILURE;

	immOp->Value = immVal;

	if (x86_Asm_Append_Operand(insn, immOp) < 0){
		delete immOp;
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = IMM_OP;
	insn->Operands[insn->NOperands - 1].Size = x86_DWORD;

	return nvmJitSUCCESS;

}


int32_t jit::ia32::x86_Asm_Append_LblOp(Px86Instruction insn, uint32_t label)
{
	Px86LblOpnd	lblOp;

	lblOp = new x86LblOpnd();
	if (lblOp == NULL)
		return nvmJitFAILURE;

	lblOp->Label = label;

	if (x86_Asm_Append_Operand(insn, lblOp) < 0){
		delete lblOp;
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = LBL_OP;
	insn->Operands[insn->NOperands - 1].Size = x86_DWORD;

	return nvmJitSUCCESS;
}

int32_t jit::ia32::x86_Asm_Append_MemOp(Px86Instruction insn, x86RegOpnd base, x86RegOpnd index, uint32_t displ, uint8_t scale, x86OpndSz size, x86MemFlags memFlags)
{
	Px86MemOpnd	memOp;

	memOp =  new x86MemOpnd();
	if (memOp == NULL)
		return nvmJitFAILURE;

	memOp->Base = base;
	memOp->Index = index;
	memOp->Displ = displ;
	memOp->Scale = scale;
	memOp->Flags = memFlags;

	if (x86_Asm_Append_Operand(insn, memOp) < 0)
	{
		delete memOp;
		return nvmJitFAILURE;
	}

	insn->Operands[insn->NOperands - 1].Type = MEM_OP;
	insn->Operands[insn->NOperands - 1].Size = size;
	return nvmJitSUCCESS;

}


void jit::ia32::x86_Asm_Append_Comment(Px86Instruction insn, const char *comment)
{
	_snprintf(insn->Comment, X86_COMMENT_LEN - 1, "%s", comment);
}

Px86Instruction jit::ia32::x86_Asm_Enqueue_Insn(Px86InsnSequence x86InsnSeq, uint16_t code)
{
	Px86Instruction instruction;

	instruction = x86_Asm_New_Op((x86OpCodesEnum)code);
	if (instruction == NULL)
	{
		NETVM_ASSERT(1 == 0, "Error creating x86 instruction");
		return NULL;
	}

	x86InsnSeq.push_back(instruction);
	return instruction;
}


Px86Instruction jit::ia32::x86_Asm_Comment(Px86InsnSequence x86InsnSeq, const char *comment)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86InsnSeq, X86_COMMENT);
	if (x86Insn == NULL)
		return NULL;

	x86_Asm_Append_Comment(x86Insn, comment);
	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;

	return x86Insn;
}


Px86Instruction jit::ia32::x86_Asm_Op_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL){
		return NULL;
	}
	if (x86_Asm_Append_RegOp(x86Insn, srcReg, opSize) < 0 )
		return NULL;

	return x86Insn;
}


Px86Instruction jit::ia32::x86_Asm_Op_Imm(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_ImmOp(x86Insn, immVal) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op_Mem_Base(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, uint32_t displ, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;

	if (x86_Asm_Append_MemOp(x86Insn, baseReg, RegisterInstance(), displ, 0, opSize, x86MemFlags(BASE | DISPL)) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op_Mem_Index(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, x86RegOpnd indexReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, indexReg, 0, 0, opSize, x86MemFlags(BASE | INDEX)) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op_Reg_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86RegOpnd dstReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL){
		return NULL;
	}
	if (x86_Asm_Append_RegOp(x86Insn, dstReg, opSize) < 0 )
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, srcReg, opSize) < 0 )
		return NULL;

	return x86Insn;
}


Px86Instruction jit::ia32::x86_Asm_Op_Imm_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, x86RegOpnd dstReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, dstReg, opSize) < 0 )
		return NULL;
	if (opSize == x86_BYTE){
		if (x86_Asm_Append_ImmOp(x86Insn, (uint8_t)immVal) < 0 )
			return NULL;
	}
	else if (opSize == x86_WORD){
		if (x86_Asm_Append_ImmOp(x86Insn, (uint16_t)immVal) < 0 )
			return NULL;
	}
	else{
		if (x86_Asm_Append_ImmOp(x86Insn, immVal) < 0 )
			return NULL;
	}

	return x86Insn;
}


Px86Instruction jit::ia32::x86_Asm_Op_Mem_Base_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, uint32_t displ, x86RegOpnd dstReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, dstReg, opSize) != nvmJitSUCCESS)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, RegisterInstance(), displ, 0, opSize, x86MemFlags(BASE | DISPL)) != nvmJitSUCCESS)
		return NULL;

	return x86Insn;
}


Px86Instruction jit::ia32::x86_Asm_Op_Reg_To_Mem_Base(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86RegOpnd baseReg, uint32_t displ, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, RegisterInstance(), displ, 0, opSize, x86MemFlags(BASE | DISPL)) != nvmJitSUCCESS )
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, srcReg, opSize) != nvmJitSUCCESS )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op_Mem_Index_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, x86RegOpnd indexReg, uint32_t displ, uint8_t scale, x86RegOpnd dstReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, dstReg, opSize) < 0 )
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, indexReg, displ, scale, opSize, x86MemFlags(BASE | INDEX)) < 0 )
		return NULL;

	return x86Insn;
}


Px86Instruction jit::ia32::x86_Asm_Op_Reg_To_Mem_Index(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86RegOpnd baseReg, x86RegOpnd indexReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, indexReg, 0, 0, opSize, x86MemFlags(BASE | INDEX)) < 0 )
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, srcReg, opSize) < 0 )
		return NULL;

	return x86Insn;
}



Px86Instruction jit::ia32::x86_Asm_Op_Imm_To_Mem_Base(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, x86RegOpnd baseReg, uint32_t displ, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, RegisterInstance(), displ, 0, opSize, x86MemFlags(BASE | DISPL)) < 0 )
		return NULL;
	if (x86_Asm_Append_ImmOp(x86Insn, immVal) < 0 )
		return NULL;

	return x86Insn;
}



Px86Instruction jit::ia32::x86_Asm_Op_Imm_To_Mem_Index(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, x86RegOpnd baseReg, x86RegOpnd indexReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, indexReg, 0, 0, opSize, x86MemFlags(BASE | INDEX)) < 0 )
		return NULL;
	if (x86_Asm_Append_ImmOp(x86Insn, immVal) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op_Mem_Displ_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t displ, x86RegOpnd dstReg, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, dstReg, opSize) < 0 )
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, RegisterInstance(), RegisterInstance(), displ, 0, opSize, DISPL) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op_Reg_To_Mem_Displ(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, uint32_t displ, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, RegisterInstance(), RegisterInstance(), displ, 0, opSize, DISPL) < 0 )
		return NULL;
	if (x86_Asm_Append_RegOp(x86Insn, srcReg, opSize) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Op_Imm_To_Mem_Displ(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, uint32_t displ, x86OpndSz opSize)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, x86OpCode);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, RegisterInstance(), RegisterInstance(), displ, 0, opSize, DISPL) < 0 )
		return NULL;
	if (x86_Asm_Append_ImmOp(x86Insn, immVal) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_JMP_Table_Entry(Px86InsnSequence x86CodeSeq, x86RegOpnd baseReg, uint32_t displ, x86OpndSz size)
{
	Px86Instruction x86Insn = x86_Asm_JMP_Indirect(x86CodeSeq, baseReg, displ, size);
	x86Insn->table_jump = true;
	return x86Insn;
}
Px86Instruction jit::ia32::x86_Asm_JMP_Indirect(Px86InsnSequence x86CodeSeq, x86RegOpnd baseReg, uint32_t displ, x86OpndSz size)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, X86_JMP);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_MemOp(x86Insn, baseReg, RegisterInstance(), displ, 0, size, x86MemFlags(DISPL|BASE)) < 0 )
		return NULL;
	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_JMP_Label(Px86InsnSequence x86CodeSeq, uint32_t basicBlockLabel)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, X86_JMP);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_LblOp(x86Insn, basicBlockLabel) < 0 )
		return NULL;

	return x86Insn;

}

Px86Instruction jit::ia32::x86_Asm_J_Label(Px86InsnSequence x86CodeSeq, uint8_t condCode, uint32_t basicBlockLabel)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, OP_CC(X86_J, condCode));
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_LblOp(x86Insn, basicBlockLabel) < 0 )
		return NULL;

	return x86Insn;

}

Px86Instruction jit::ia32::x86_Asm_CALL_NetVM_API(Px86InsnSequence x86CodeSeq, void *functAddress)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, X86_CALL);
	if (x86Insn == NULL)
		return NULL;
	if (x86_Asm_Append_ImmOp(x86Insn, (intptr_t)functAddress) < 0 )
		return NULL;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Switch_Table_Entry(Px86InsnSequence x86CodeSeq, uint32_t target)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, X86_SW_TABLE_ENTRY);
	if (x86Insn == NULL)
		return NULL;

	x86Insn->switch_default_target = target;

	return x86Insn;
}

Px86Instruction jit::ia32::x86_Asm_Switch_Table_Entry_Start(Px86InsnSequence x86CodeSeq, Px86Instruction entry)
{
	Px86Instruction x86Insn;

	x86Insn = x86_Asm_Enqueue_Insn(x86CodeSeq, X86_SW_TABLE_ENTRY_START);
	if (x86Insn == NULL)
		return NULL;

	x86Insn->switch_entry = entry;

	return x86Insn;
}

std::set<x86Instruction::RegType > jit::ia32::x86Instruction::getUses()
{
	std::set<x86Instruction::RegType> res;

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
			x86RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
			res.insert(reg);
		}
		else if(Operands[i].Type == MEM_OP)
		{
			if (MEM_OPERAND(Operands[i].Op)->Flags & BASE)
			{
				x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
				res.insert(reg);
			}
			if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX)
			{
				x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
				res.insert(reg);
			}
		}
	}
*/

	for (i = NOperands - 1; i >= 0 && usedOpnds < OpDescr->Uses; i--){

		if (Operands[i].Type == REG_OP){
			x86RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
			res.insert(reg);
		}
		else if(Operands[i].Type == MEM_OP){
			if (MEM_OPERAND(Operands[i].Op)->Flags & BASE){
				x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
				res.insert(reg);
			}
			if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
				x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
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
				x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
				res.insert(reg);
			}
			if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
				x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
				res.insert(reg);
			}
		}

		i--;
	}
/*
	for (i = NOperands - 1; i >= 0 && usedOpnds < OpDescr->Uses; i--){

			if (Operands[i].Type == REG_OP){
				x86RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
				res.insert(reg);
			}
			else if(Operands[i].Type == MEM_OP){
				if (MEM_OPERAND(Operands[i].Op)->Flags & BASE){
					x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
					res.insert(reg);
				}
				if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
					x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
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
					x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Base;
					res.insert(reg);
				}
				if (MEM_OPERAND(Operands[i].Op)->Flags & INDEX){
					x86RegOpnd& reg = MEM_OPERAND(Operands[i].Op)->Index;
					res.insert(reg);
				}
			}

			i--;
		}
*/
	return res;
}

std::set<x86Instruction::RegType> jit::ia32::x86Instruction::getDefs()
{
	std::set<x86Instruction::RegType> res;

	uint32_t i;
	x86RegOpnd reg;


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

	for (i = 0; i < OpDescr->Defs && i < NOperands; i++)
	{
		if (Operands[i].Type == REG_OP)
		{
			x86RegOpnd& reg = *REG_OPERAND(Operands[i].Op);
			res.insert(reg);
		}
	}

	return res;
}

bool jit::ia32::x86Instruction::isCopy() const
{
	if (OpCode == (uint16_t)X86_MOV && NOperands == 2){
		if (Operands[0].Type == REG_OP && Operands[1].Type == REG_OP){
			return true;
		}
	}
	return false;
}

jit::ia32::x86Instruction::RegType
jit::ia32::x86Instruction::get_to() const
{
	return *REG_OPERAND(Operands[0].Op);
}

jit::ia32::x86Instruction::RegType
jit::ia32::x86Instruction::get_from() const
{
	return *REG_OPERAND(Operands[1].Op);
}

std::list< std::pair<RegisterInstance, RegisterInstance> > jit::ia32::x86Instruction::getCopiedPair()
{
	std::list< std::pair<RegType, RegType> > res;

	if(isCopy())
	{
		res.push_back( std::pair<RegType, RegType>(get_to(), get_from()));
	}

	return res;
}

bool jit::ia32::x86Instruction::has_side_effects()
{
	if(this->getOpcode() == X86_BSWAP)
		return true;
	return false;
}
void jit::ia32::x86Instruction::rewrite_Reg(jit::ia32::x86Instruction::RegType oldreg, jit::ia32::x86Instruction::RegType *newreg)
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

void jit::ia32::printRegister(std::ostream& os, x86RegOpnd reg, uint32_t size)
{
	if(X86_IS_MACH_REG(reg))
		os << x86RegNames[reg.get_model()->get_name() + size * 8];
	else
		os << "r"<< reg.get_model()->get_space() << "." << reg.get_model()->get_name() << "." << reg.version();
}


std::ostream& jit::ia32::operator<<(std::ostream& os, x86Operand& op)
{
	os << left;

	if(op.Type == IMM_OP)
	{
		os << std::hex <<"0x" << IMM_OPERAND(op.Op)->Value <<std::dec;
	}
	else if(op.Type == LBL_OP)
	{
		os << "lbl" << ((Px86LblOpnd)op.Op)->Label;
	}
	else if(op.Type == MEM_OP)
	{
		switch (op.Size){
			case x86_BYTE:
				os << "byte ptr[";
				break;
			case x86_WORD:
				os << "word ptr[";
				break;
			case x86_DWORD:
				os <<  "dword ptr[";
		}
		if (((Px86MemOpnd)op.Op)->Flags & BASE){
			printRegister(os, MEM_OPERAND(op.Op)->Base, x86_DWORD);
		}
		if (((Px86MemOpnd)op.Op)->Flags & INDEX){
			if (((Px86MemOpnd)op.Op)->Flags & BASE)
				os << " + ";
			printRegister(os, MEM_OPERAND(op.Op)->Index, x86_DWORD);
		}
		if (((Px86MemOpnd)op.Op)->Flags & SCALE & INDEX){
			os << " * " << MEM_OPERAND(op.Op)->Scale;
		}
		if (((Px86MemOpnd)op.Op)->Flags & DISPL){
			if((int32_t)MEM_OPERAND(op.Op)->Displ != 0) {
				if ((((Px86MemOpnd)op.Op)->Flags & (BASE | INDEX))){
					if((int32_t)MEM_OPERAND(op.Op)->Displ > 0)
						os << " + ";
				}
				os << hex << "0x" << MEM_OPERAND(op.Op)->Displ << dec;
			}
		}
		os << "]";
	}
	else
	{
		printRegister(os, *REG_OPERAND(op.Op), op.Size);
	}

	return os;
}

void jit::ia32::_X86_INSTRUCTION::printNode(std::ostream& os, bool SSAform)
{
	os << *this;
}

std::ostream& jit::ia32::operator<<(std::ostream& os, _X86_INSTRUCTION& insn)
{
	if(insn.getOpcode() == X86_COMMENT)
	{
		return os << "; " << insn.Comment;
	}

	{
		ostringstream name;
		name << insn.OpDescr->Name;
		name << (insn.OpDescr->Flags & NEED_CC ? x86CC[CC_ONLY(insn.getOpcode())] : "" );

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


_X86_INSTRUCTION*& jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT::operator*()
{
	return ptr;
}

_X86_INSTRUCTION*& jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT::operator->()
{
	return ptr;
}

jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT& jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT::operator++(int)
{
	ptr = NULL;
	return *this;
}

bool jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT::operator==(const _X86_INSTRUCTION_IT& it) const
{
	return ptr == it.ptr;
}

bool jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT::operator!=(const _X86_INSTRUCTION_IT& it) const
{
	return ptr != it.ptr;
}

jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT jit::ia32::_X86_INSTRUCTION::nodeBegin()
{
	_X86_INSTRUCTION_IT it(this);
	return it;
}


jit::ia32::_X86_INSTRUCTION::_X86_INSTRUCTION_IT jit::ia32::_X86_INSTRUCTION::nodeEnd()
{
	_X86_INSTRUCTION_IT it;
	return it;
}
bool jit::ia32::operator==(x86Operand a, x86Operand b)
{
	if(a.Type != b.Type || a.Size != b.Size)
		return false;
	switch(a.Type)
	{
		case REG_OP:
			return (*((Px86RegOpnd)a.Op)) == (*((Px86RegOpnd)b.Op));
			break;
		case IMM_OP:
			return (*((Px86ImmOpnd)a.Op)) == (*((Px86ImmOpnd)b.Op));
			break;
		case MEM_OP:
			return (*((Px86MemOpnd)a.Op)) == (*((Px86MemOpnd)b.Op));
			break;
		case LBL_OP:
			return (*((Px86LblOpnd)a.Op)) == (*((Px86LblOpnd)b.Op));
			break;
		default:
			return false;
	}
	return false;

}

bool jit::ia32::operator==(x86ImmOpnd a, x86ImmOpnd b)
{
	return a.Value == b.Value;
}

bool jit::ia32::operator==(x86LblOpnd a, x86LblOpnd b)
{
	return a.Label == b.Label;
}

bool jit::ia32::operator==(x86MemOpnd a, x86MemOpnd b)
{
	if(a.Base != b.Base
			|| a.Index != b.Index
			|| a.Displ != b.Displ
			|| a.Scale != b.Scale
			|| a.Flags != b.Flags)
		return false;
	return true;
}
