/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _X64_SWITCH_LOWERING
#define _X64_SWITCH_LOWERING

#include "x64-asm.h"
#include "switch_lowering.h"

namespace jit {
namespace x64{

	class x64SwitchHelper : public SwitchHelper<x64Instruction>
	{
		public:

		x64SwitchHelper(BasicBlock<IR>& bb, SwitchMIRNode& insn);
		void emit_jcmp_eq(uint32_t case_value, uint32_t jt, uint32_t jf);
		void emit_jcmp_l(uint32_t case_value, uint32_t jt, uint32_t jf);
		void emit_jcmp_g(uint32_t case_value, uint32_t jt, uint32_t jf);
		void emit_jmp_to_table_entry(Window& w);
		void emit_start_table_entry_code(uint32_t name);
		void emit_table_entry(uint32_t name, bool defTarget = false);
		void emit_jmp_to_table_entry(uint32_t min);
		void emit_jump_table_entry(uint32_t target);

		void emit_binary_jump(uint32_t case_value, std::string& my_name, std::string& target_name);
		void emit_binary_end_jump(uint32_t case_value, uint32_t jt, uint32_t jf, std::string& my_name);

		private:
		x64Instruction::RegType original_reg;
		x64Instruction::RegType tmp_reg;

		std::map< uint32_t, Px64Instruction> entries; 
		std::map< std::string, Px64Instruction> bin_tree;
	};

} //namespace x64
} //namespace jit

#endif
