#ifndef _OCTEON_SWITCH_H
#define _OCTEON_SWITCH_H

#include "switch_lowering.h"
#include "octeon-asm.h"

namespace jit {
	namespace octeon {

		class octeon_Switch_Helper : public SwitchHelper<octeon_Insn> {

			public:

				octeon_Switch_Helper(CFG<octeon_Insn>& cfg, BasicBlock<IR>& bb, SwitchMIRNode& insn);

				void emit_jcmp_l(uint32_t case_value, uint32_t jt, uint32_t jf);
				void emit_jcmp_g(uint32_t case_value, uint32_t jt, uint32_t jf);
				void emit_jmp_to_table_entry(uint32_t min);
				void emit_table_entry(uint32_t name, bool defTarget = false);
				void emit_jump_table_entry(uint32_t target);
				void emit_binary_jump(uint32_t case_value, std::string& my_name, std::string& target_name);
				void emit_binary_end_jump(uint32_t case_value, uint32_t jt, uint32_t jf, std::string& my_name);

				//methods of MRST not implemented for now
				void emit_jcmp_eq(uint32_t case_value, uint32_t jt, uint32_t jf) {throw "Method not implemented!!!";}
				void emit_start_table_entry_code(uint32_t name) {throw "Method not implemented!!!";}
				void emit_jmp_to_table_entry(Window& w)  {throw "Method not implemented!!!";}

			private:
				octeonRegType original_reg;
				octeonCFG* cfg;
				octeonJumpTable* jump_table;
				std::string make_name(std::string& name);
				//std::string name_to_label(uint32_t target, bool end = false);
		};

		class octeon_SwitchEmitter : public SwitchEmitter
		{
			public:

				octeon_SwitchEmitter(ISwitchHelper& helper, SwitchMIRNode& insn): SwitchEmitter(helper, insn) {}
				void run();
		};

	}
}

#endif
