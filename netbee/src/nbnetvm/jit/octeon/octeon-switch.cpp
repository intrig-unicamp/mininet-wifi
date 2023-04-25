/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "octeon-switch.h"
#include <sstream>

using namespace jit;
using namespace octeon;
using namespace std;

octeon_Switch_Helper::octeon_Switch_Helper(CFG<octeon_Insn>& cfg_p, BasicBlock<IR>& bb, SwitchMIRNode& insn)
	: 
		SwitchHelper<octeon_Insn>(bb, insn),
		original_reg(insn.getKid(0)->getDefReg()),
		cfg(dynamic_cast<octeonCFG*>(&cfg_p)),
		jump_table(NULL)
{
//	cfg = dynamic_cast<octeonCFG*>(&cfg);
	assert(cfg != NULL);
}

void octeon_Switch_Helper::emit_jcmp_l(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	octeonRegType tmp(octeonRegType::new_virt_reg());
	octeonRegType zero(octeonRegType::mach_reg(ZERO));

	bb.getCode().push_back( new octeonAlu3op(OCTEON_DADDI, tmp, zero, case_value));
	bb.getCode().push_back( new octeonAlu3op(OCTEON_SUB, tmp, original_reg, tmp));
	bb.getCode().push_back( new octeonBranchCmp1Op(OCTEON_BLTZ, tmp, jt, jf));
}

void octeon_Switch_Helper::emit_jcmp_g(uint32_t case_value, uint32_t jt, uint32_t jf)
{
	octeonRegType tmp(octeonRegType::new_virt_reg());
	octeonRegType zero(octeonRegType::mach_reg(ZERO));

	bb.getCode().push_back( new octeonAlu3op(OCTEON_DADDI, tmp, zero, case_value));
	bb.getCode().push_back( new octeonAlu3op(OCTEON_SUB, tmp, original_reg, tmp));
	bb.getCode().push_back( new octeonBranchCmp1Op(OCTEON_BGTZ, tmp, jt, jf));
}

void octeon_Switch_Helper::emit_jmp_to_table_entry(uint32_t min)
{
	ostringstream name;
	name << cfg->getName() << "." << cfg->getJumpTables().size();
	octeonRegType tmp(octeonRegType::new_virt_reg());
	octeonRegType zero(octeonRegType::mach_reg(ZERO));
	string s_name(name.str());

	jump_table = new octeonJumpTable(s_name);
	cfg->addJumpTable(jump_table);

	bb.getCode().push_back( new octeonAlu3op(OCTEON_DADDI, tmp, zero, min));
	bb.getCode().push_back( new octeonAlu3op(OCTEON_SUB, tmp, original_reg, tmp));
	bb.getCode().push_back( new octeonAlu3op(OCTEON_SLL, tmp, tmp, 3));
	bb.getCode().push_back( new octeonBranchJumpTable(tmp, s_name));
}

void octeon_Switch_Helper::emit_table_entry(uint32_t name, bool defTarget)
{
}

void octeon_Switch_Helper::emit_jump_table_entry(uint32_t target)
{
	jump_table->addTarget(target);
}

string octeon_Switch_Helper::make_name(string& name) //(uint32_t name, bool end)
{
	ostringstream res;
	res << ".L" << bb.getId() << "." <<name;
	return res.str();
}

void octeon_Switch_Helper::emit_binary_jump(uint32_t case_value, std::string& my_name, std::string& target_name)
{
	string lname(make_name(my_name));
	string tname(make_name(target_name));

	bb.getCode().push_back( new octeonBinaryTreeBranch(original_reg, case_value, lname, tname));
}

void octeon_Switch_Helper::emit_binary_end_jump(uint32_t case_value, uint32_t jt, uint32_t jf, string& my_name)
{
	string name(make_name(my_name));
	octeonRegType tmp(octeonRegType::new_virt_reg());
	octeonRegType zero(octeonRegType::mach_reg(ZERO));

	bb.getCode().push_back( new octeonLabel(name));
	bb.getCode().push_back( new octeonAlu3op(OCTEON_DADDI, tmp, zero, case_value));
	bb.getCode().push_back( new octeonBranchCmp2Op(OCTEON_BEQ, original_reg, tmp, jt, jf));
	bb.getCode().push_back( new octeonBranchLblOp(OCTEON_J, jf));
}

void octeon_SwitchEmitter::run()
{
	cases_set c(insn.TargetsBegin(), insn.TargetsEnd());
	CaseCompLess less;
	sort(c.begin(), c.end(), less);

	uint32_t size = c.size();
	uint32_t density = size * 100 / (c[size - 1].first - c[0].first);

	if(density >= 40)
	{
		emit_jump_table(c);
	}
	else
		binary_switch(c);
}
