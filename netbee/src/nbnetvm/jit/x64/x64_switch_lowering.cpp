/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x64_switch_lowering.h"

using namespace jit;
using namespace x64;

x64SwitchHelper::x64SwitchHelper(BasicBlock<x64Instruction>& bb, SwitchMIRNode& insn)
	: 
		SwitchHelper<x64Instruction>(bb, insn),
		original_reg(insn.getKid(0)->getDefReg()),
		tmp_reg(X64_NEW_VIRT_REG)
{ } 

void x64SwitchHelper::emit_jcmp_l(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_CMP, case_value, original_reg, x64_DWORD);
	x64_Asm_J_Label(bb.getCode(), L, jt);
}

void x64SwitchHelper::emit_jcmp_g(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_CMP, case_value, original_reg, x64_DWORD);
	x64_Asm_J_Label(bb.getCode(), G, jt);
}

void x64SwitchHelper::emit_jcmp_eq(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_CMP, case_value, original_reg, x64_DWORD);
	x64_Asm_J_Label(bb.getCode(), E, jt);
	x64_Asm_JMP_Label(bb.getCode(), jf);
}

/**
 * Emit jump to the destination saved in the jump table (case value getted from original_reg)
 * @param min the value of the first entry in the jumptable (ex. if min = 5 and case = 6, it will jump to address specified in the second entry of the jumptable)
 */
void x64SwitchHelper::emit_jmp_to_table_entry(uint32_t min)
{
	int32_t displ = -min*8;
	x64_Asm_Load_Current_Memory_Location(bb.getCode(), tmp_reg);
	x64_Asm_Op_Mem_Index_To_Reg(bb.getCode(), X64_LEA, tmp_reg, original_reg, displ, 3, tmp_reg, x64_QWORD);
	
	// we need to calcolate offset from loadCurrentMemory instruction to first Table Entry
	// 10 bytes (mov/loadCurrentAddress) + 4/5/8 bytes (lea, displ = 0, displ is 8bit, displ is 32bit) + 7 byte jmp
	int32_t offset;
	if (displ == 0)
	    offset = 21;
	else if (displ >= -128 && displ <= 127)
	    offset = 22;
	else
	    offset = 25;

	x64_Asm_JMP_Indirect(bb.getCode(),tmp_reg, offset, x64_QWORD);
}

void x64SwitchHelper::emit_jump_table_entry(uint32_t target)
{
	x64_Asm_Switch_Table_Entry(bb.getCode(), target);
}

void x64SwitchHelper::emit_jmp_to_table_entry(Window& w)
{
	x64_Asm_Op_Reg_To_Reg(bb.getCode(), X64_MOV, original_reg, tmp_reg, x64_DWORD);
	x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_AND, w.mask, tmp_reg, x64_DWORD);
	
	//implicit multiply for 8
	uint32_t shift = w.r;
	if(shift > 3)
	{
		x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_SHR, shift - 3, tmp_reg, x64_QWORD);
	}
	else if (shift < 3)
	{
		x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_SHL, 3 - shift, tmp_reg, x64_QWORD);
	}
	//else shift == 3 do nothing
	x64Instruction::RegType base_reg = X64_NEW_VIRT_REG;
	x64_Asm_Load_Current_Memory_Location(bb.getCode(), base_reg);
	x64_Asm_Op_Reg_To_Reg(bb.getCode(), X64_ADD, base_reg, tmp_reg, x64_QWORD);
	x64_Asm_JMP_Indirect(bb.getCode(), tmp_reg, 10 + 3 + 7, x64_QWORD);
}

void x64SwitchHelper::emit_start_table_entry_code(uint32_t name)
{
	Px64Instruction entry = entries[name];
	x64_Asm_Switch_Table_Entry_Start(bb.getCode(), entry);
}

void x64SwitchHelper::emit_binary_end_jump(uint32_t case_value, uint32_t jt, uint32_t jf, std::string& my_name)
{
#ifdef _DEBUG_X64_BACKEND
	cout << "end jump case " << case_value << "\t jt:" << jt << "\t jf:" << jf << "\n";
#endif
	x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_CMP, case_value, original_reg, x64_DWORD);
	Px64Instruction node = x64_Asm_J_Label(bb.getCode(), E, jt);
	x64_Asm_JMP_Label(bb.getCode(), jf);

	Px64Instruction father = bin_tree[my_name];

	if(father)
	{
		father->binary_switch_jump = true;
		node->switch_entry = father;
	}
}

void x64SwitchHelper::emit_binary_jump(uint32_t case_value, std::string& my_name, std::string& target_name)
{
#ifdef _DEBUG_X64_BACKEND
	cout << "bin jump case " << case_value << "\t jt: -" << "\t jf:" << "\n";
#endif
	x64_Asm_Op_Imm_To_Reg(bb.getCode(), X64_CMP, case_value, original_reg, x64_DWORD);
	Px64Instruction node = x64_Asm_J_Label(bb.getCode(), GE, 0);

	bin_tree[target_name] = node;

	Px64Instruction father = bin_tree[my_name];
	if(father)
	{
		father->binary_switch_jump = true;
		node->switch_entry = father;
	}
}

void x64SwitchHelper::emit_table_entry(uint32_t name, bool defTarget)
{
	if(defTarget)
	{
		x64_Asm_Switch_Table_Entry(bb.getCode(), insn.getDefaultTarget());
		return;
	}

	entries[name] = x64_Asm_Switch_Table_Entry(bb.getCode());
}
