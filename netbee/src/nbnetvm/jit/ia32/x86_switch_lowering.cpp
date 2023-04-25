/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x86_switch_lowering.h"

using namespace jit;
using namespace ia32;

x86SwitchHelper::x86SwitchHelper(BasicBlock<x86Instruction>& bb, SwitchMIRNode& insn)
	: 
		SwitchHelper<x86Instruction>(bb, insn),
		original_reg(insn.getKid(0)->getDefReg()),
		tmp_reg(X86_NEW_VIRT_REG)
{ } 

void x86SwitchHelper::emit_jcmp_l(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_CMP, case_value, original_reg, x86_DWORD);
	x86_Asm_J_Label(bb.getCode(), L, jt);
}

void x86SwitchHelper::emit_jcmp_g(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_CMP, case_value, original_reg, x86_DWORD);
	x86_Asm_J_Label(bb.getCode(), G, jt);
}

void x86SwitchHelper::emit_jcmp_eq(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_CMP, case_value, original_reg, x86_DWORD);
	x86_Asm_J_Label(bb.getCode(), E, jt);
	x86_Asm_JMP_Label(bb.getCode(), jf);
}

void x86SwitchHelper::emit_jmp_to_table_entry(uint32_t min)
{
	x86_Asm_Op_Reg_To_Reg(bb.getCode(), X86_MOV, original_reg, tmp_reg, x86_DWORD);
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_ADD, -min, tmp_reg, x86_DWORD);
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_SHL, 2, tmp_reg, x86_DWORD);

	x86_Asm_JMP_Table_Entry(bb.getCode(), tmp_reg, 0, x86_DWORD);
}

void x86SwitchHelper::emit_jump_table_entry(uint32_t target)
{
	x86_Asm_Switch_Table_Entry(bb.getCode(), target);
}

void x86SwitchHelper::emit_jmp_to_table_entry(Window& w)
{
	x86_Asm_Op_Reg_To_Reg(bb.getCode(), X86_MOV, original_reg, tmp_reg, x86_DWORD);
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_AND, w.mask, tmp_reg, x86_DWORD);
	
	//implicit multiply for 4
	uint32_t shift = w.r;
	if(shift > 2)
	{
		x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_SHR, shift - 2, tmp_reg, x86_DWORD);
	}
	else if (shift < 2)
	{
		x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_SHL, 2 - shift, tmp_reg, x86_DWORD);
	}
	//else shift == 2 do nothing

	x86_Asm_JMP_Table_Entry(bb.getCode(), tmp_reg, 0, x86_DWORD);
}

void x86SwitchHelper::emit_start_table_entry_code(uint32_t name)
{
	Px86Instruction entry = entries[name];
	x86_Asm_Switch_Table_Entry_Start(bb.getCode(), entry);
}

void x86SwitchHelper::emit_binary_end_jump(uint32_t case_value, uint32_t jt, uint32_t jf, std::string& my_name)
{
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_CMP, case_value, original_reg, x86_DWORD);
	Px86Instruction node = x86_Asm_J_Label(bb.getCode(), E, jt);
	x86_Asm_JMP_Label(bb.getCode(), jf);

	Px86Instruction father = bin_tree[my_name];

	if(father)
	{
		father->table_jump = true;
		node->switch_entry = father;
	}
}

void x86SwitchHelper::emit_binary_jump(uint32_t case_value, std::string& my_name, std::string& target_name)
{
	x86_Asm_Op_Imm_To_Reg(bb.getCode(), X86_CMP, case_value, original_reg, x86_DWORD);
	Px86Instruction node = x86_Asm_J_Label(bb.getCode(), GE, 0);

	bin_tree[target_name] = node;

	Px86Instruction father = bin_tree[my_name];
	if(father)
	{
		father->table_jump = true;
		node->switch_entry = father;
	}
}

void x86SwitchHelper::emit_table_entry(uint32_t name, bool defTarget)
{
	if(defTarget)
	{
		x86_Asm_Switch_Table_Entry(bb.getCode(), insn.getDefaultTarget());
		return;
	}

	entries[name] = x86_Asm_Switch_Table_Entry(bb.getCode());
}
