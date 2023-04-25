/*!
 * \file octeon-asm.cpp
 * \brief this file contains definition of routines working on octeon instruction
 */
#include "octeon-asm.h"
#include "application.h"
#include "int_structs.h"
#include "gc_regalloc.h"
#include <sstream>

using namespace jit;
using namespace octeon;
using namespace std;


const uint32_t jit::octeon::octeonRegType::MachSpace  = 64;
const uint32_t jit::octeon::octeonRegType::SpillSpace = 65;
const uint32_t jit::octeon::octeonRegType::VirtSpace = 1;
const uint32_t jit::octeon::octeonRegType::num_mach_regs = 34;
const string    jit::octeon::count_prop_name("instruction_count");
const uint32_t jit::octeon::octeon_Insn::max_bytes_per_insn(6);
const uint32_t jit::octeon::octeon_Insn::max_near_jump_offset(1 << 16); //128k

const octeonRegs jit::octeon::octeonRegType::calle_save_regs[] = 
{
	A0, A1, A2, A3,
	S0, S1, S2, S3, S4, S5, S6, S7,
	T9,
	GP, SP, FP, RA
};

const octeonRegs jit::octeon::octeonRegType::caller_save_regs[] = 
{
	V0, V1, T0, T1, T2, T3, T4, T5, T6, T7,
	T8, T9
};

const octeonRegs jit::octeon::octeonRegType::avaible [] =
{
	S0, S1, S2, S3, S4, S5, S6, S7,
	V0, V1, T0, T1, T2, T3, T4, T5, T6, T7, A0, A1, A2, A3, 
};

const octeonRegs jit::octeon::octeonRegType::precolored [] =
{
	ZERO, AT, T8, T9, KT0, KT1, GP, SP, FP, RA, HI, LO
};

const char* jit::octeon::octeonRegType::octeonRegsNames[] =
{
	"ZERO",
	"AT",
	"V0", "V1",
	"A0", "A1", "A2", "A3",
	"T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
	"S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
	"T8", "T9",
	"KT0", "KT1",
	"GP", "SP", "FP", "RA", "HI", "LO"
};

octeonCFG::octeonCFG(const std::string& name)
:
	CFG<octeon_Insn>(name)
{
}

const octeonCFG::jump_table_list_t& octeonCFG::getJumpTables() const
{
	return jumpTables;
}

octeonCFG::~octeonCFG()
{
	jump_table_list_t::iterator i;
	for(i = jumpTables.begin(); i != jumpTables.end(); i++)
	{
		delete *i;
	}
}

void octeonCFG::addJumpTable(octeonJumpTable* jump_table)
{
	assert(jump_table != NULL);

	jumpTables.push_back(jump_table);
}

const struct octeonInsDescr jit::octeon::octeonInsDescription[] =
{
#define OCTEON_ASM(code, mnemonic, flags, defs, uses, description)	{mnemonic, flags, defs, uses},
#include "octeon-asm.def"
#undef OCTEON_ASM
};

void octeonRegType::init_machine_regs(list<octeonRegType>& machine_regs)
{
	for(uint32_t i = 0; i < sizeof(precolored); i++)
	{
		machine_regs.push_back( octeonRegType(MachSpace, (uint32_t)precolored[i]) );
	}
}

void octeonRegType::init_virtual_regs(list<octeonRegType>& virtual_regs)
{
	int latest_virt =  RegisterModel::get_latest_name(octeonRegType::VirtSpace);

	for(int32_t i = 0; i <= latest_virt; i++)
	{
		virtual_regs.push_back( octeonRegType(octeonRegType::VirtSpace, i) );
	}
}

void octeonRegType::init_colors(list<octeonRegType>& colors)
{

	for(uint32_t i = 0; i < sizeof(avaible); i++)
	{
		colors.push_back( octeonRegType(MachSpace, (uint32_t)avaible[i]) );
	}
}

void octeonRegType::rename_regs(list<octeonRegType>& virtualRegisters, GCRegAlloc<CFG<octeon_Insn> >& regAlloc)
{
	typedef list< pair<octeonRegType, octeonRegType> > list_t;
	typedef list< pair<octeonRegType, octeonRegType> >::iterator iterator_t;
	list_t registers = regAlloc.getColors();

	for(iterator_t reg = registers.begin(); reg != registers.end(); reg++)
	{
		octeonRegType& r = reg->first;
		octeonRegType& color = reg->second;
		r.get_model()->rename( color.get_model()->get_space(), color.get_model()->get_name());
	}
}

octeonRegType::octeonRegType(const RegisterInstance& reg)
:
	RegisterInstance(reg)
{}

octeonRegType::octeonRegType(uint32_t space, uint32_t name, uint32_t version)
:
	RegisterInstance(space, name, version)
{}

ostream& jit::octeon::operator<<(ostream& os, const octeonRegType& reg)
{
	if(reg.is_mach_register())
	{
		return os << "$" << reg.get_model()->get_name();//octeonRegType::octeonRegsNames[reg.get_model()->get_name()];
	}

	return os << (const RegisterInstance&) reg;
}

octeonJumpTable::octeonJumpTable(std::string& name)
:
	name(name)
{}

ostream& octeonJumpTable::print(ostream& os) const
{
	os << ".type\t" << name << ", @object" << endl;
	os << ".size\t" << name << ", " << targets.size() * 8 << endl;
	os << name << ":" << endl;

	target_list_t::const_iterator i;
	for(i = targets.begin(); i != targets.end(); i++)
	{
		os << ".dword\t" << *i << endl;
	}

	return os;
}

octeon_Insn::octeon_Insn(octeonOpcodes opcode)
:
	TableIRNode<octeonRegType, octeonOpcodes>(opcode)
{ }

ostream& octeon_Insn::print(ostream& os) const
{
	return os << octeonInsDescription[OpCode].Name;
}

bool octeon_Insn::test_flag(octeonOpDescrFlags flag) const
{
	if (octeonInsDescription[getOpcode()].Flags & (uint32_t)flag)
		return true;
	else
		return false;
}

bool octeon_Insn::isCopy() const
{
	return false;
}

//!return true if this instruction is a far (>128K) jump
bool octeon_Insn::isLongJump(CFG<octeon_Insn>& cfg, uint32_t count) const
{
	return false;
}

//!trasform this instruction to a long jump
void octeon_Insn::toLongJump()
{
	//do nothing
	return;
}

list< pair <octeon_Insn::RegType, octeon_Insn::RegType> > octeon_Insn::getCopiedPair() const
{
	list< pair<RegType, RegType> >res;
	return res;
}

octeonRegType octeon_Insn::get_to() const
{
	return octeonRegType();
}

octeonRegType octeon_Insn::get_from() const
{
	return octeonRegType();
}

octeon_Insn::IRNodeIterator::IRNodeIterator(octeon_Insn* insn)
	: 
		ptr(insn) 
{ }

octeon_Insn::IRNodeIterator::IRNodeIterator(const IRNodeIterator &it)
	: 
		ptr(it.ptr) 
{}

ostream& jit::octeon::operator<<(ostream& os, const octeon_Insn& insn)
{
	return insn.print(os);
}

octeonOperand::octeonOperand(octeonOpType type, octeonOpSize size)
	:
	type(type),
	size(size)
{}

ostream& jit::octeon::operator<<(ostream& os, const octeonOperand& operand)
{
	return operand.print(os);
}

octeonImmOp::octeonImmOp(uint32_t value)
	:
	octeonOperand(IMM_OP, OC_WORD),
	value(value)
{}

ostream& octeonImmOp::print(ostream& os) const
{
	return os << value;
}

octeonRegOp::octeonRegOp()
:
	octeonOperand(REG_OP),
	reg(octeonRegType ( RegisterInstance() ))
{}

octeonRegOp::octeonRegOp(octeonRegType& reg, octeonOpSize size)
:
	octeonOperand(REG_OP, size),
	reg(reg)
{}

ostream& octeonRegOp::print(ostream& os) const
{
	return os << reg;
}

octeonMemOp::octeonMemOp(uint32_t displ, octeonRegType& base, octeonOpSize size)
:
	octeonOperand(REG_OP,size),
	displ(displ),
	base(base)
{}

octeonMemOp::octeonMemOp(string& displ, octeonRegType& base, octeonOpSize size)
:
	octeonOperand(REG_OP,size),
	str_displ(displ),
	base(base)
{}

ostream& octeonMemOp::print(ostream& os) const
{
	if(str_displ.empty())
	{
		os << displ;
	}
	else
	{
		os << str_displ;
	}

	return os << "(" << base << ")";
}

octeonLabelOp::octeonLabelOp(uint32_t target)
:
	octeonOperand(LABEL_OP),
	label(target)
{}

ostream& octeonLabelOp::print(ostream& os) const
{
	return os << ".L" << label;
}

octeonLoad::octeonLoad(octeonOpcodes opcode, octeonRegType& base, std::string& displ, octeonRegType& dst, octeonOpSize size)
:
	octeon_Insn(opcode),
	from(displ, base, OC_DWORD),
	to(dst, size)
{}

octeonLoad::octeonLoad(octeonOpcodes opcode, octeonRegType& base, uint32_t displ, octeonRegType& dst, octeonOpSize size)
:
	octeon_Insn(opcode),
	from(displ, base, OC_DWORD),
	to(dst, size)
{}

set<octeonRegType> octeon_Insn::getUses()
{
	set<octeonRegType> res;
	return res;
}

set<octeonRegType> octeon_Insn::getDefs()
{
	set<octeonRegType> res;
	return res;
}

set<octeonRegType> octeonLoad::getUses()
{
	set<octeonRegType> res;
	res.insert(from.get_base());
	return res;
}

set<octeonRegType> octeonLoad::getDefs()
{
	set<octeonRegType> res;
	res.insert(to.get_reg());
	return res;
}

ostream& octeonLoad::print(ostream& os) const
{
	return octeon_Insn::print(os) << " " << to << ", " << from;
}

octeonStore::octeonStore(octeonOpcodes opcode, octeonRegType& base, uint32_t displ, octeonRegType& src, octeonOpSize size)
:
	octeon_Insn(opcode),
	to(displ, base, OC_DWORD),
	from(src, size)
{
}

set<octeonRegType> octeonStore::getUses()
{
	set<octeonRegType> res;
	res.insert(from.get_reg());
	res.insert(to.get_base());
	return res;
}

ostream& octeonStore::print(ostream& os) const
{
	return octeon_Insn::print(os) << " " << from << ", " << to;
}

octeonAlu3op::octeonAlu3op(
	octeonOpcodes opcode, 
	octeonRegType& dst, 
	octeonRegType& src1, 
	octeonRegType& src2, 
	octeonOpSize size)
:
	octeon_Insn(opcode),
	imm(0)
{
	assert(!test_flag(IMM));

	operands[0] = octeonRegOp(dst,size);
	operands[1] = octeonRegOp(src1, size);
	operands[2] = octeonRegOp(src2, size);
}
	
octeonAlu3op::octeonAlu3op(octeonOpcodes opcode, octeonRegType& dst, octeonRegType& src1, uint32_t imm, octeonOpSize size)
:
	octeon_Insn(opcode),
	imm(imm)
{
	assert(test_flag(IMM));

	operands[0] = octeonRegOp(dst,size);
	operands[1] = octeonRegOp(src1, size);
}

set<octeonRegType> octeonAlu3op::getUses()
{
	set<octeonRegType> res;
	res.insert(operands[1].get_reg());
	if(!test_flag(IMM))
	{
		res.insert(operands[2].get_reg());
	}
	return res;
}

set<octeonRegType> octeonAlu3op::getDefs()
{
	set<octeonRegType> res;
	res.insert(operands[0].get_reg());
	return res;
}

ostream& octeonAlu3op::print(std::ostream& os) const
{
	octeon_Insn::print(os) << " " << operands[0] << ", " << operands[1] << ", ";
	if(test_flag(IMM))
	{
		return os << imm;
	}

	return os << operands[2];
}

octeonAlu2Op::octeonAlu2Op(octeonOpcodes opcode, octeonRegType& dst, octeonRegType& src, octeonOpSize size)
:
	octeon_Insn(opcode),
	imm(0)
{
	operands[0] = octeonRegOp(dst, size);
	operands[1] = octeonRegOp(src, size);
}

octeonAlu2Op::octeonAlu2Op(octeonOpcodes opcode, octeonRegType& dst, uint32_t imm, octeonOpSize size)
:
	octeon_Insn(opcode),
	imm(imm)
{
	assert(test_flag(IMM));

	operands[0] = octeonRegOp(dst, size);
}

set<octeonRegType> octeonAlu2Op::getUses()
{
	set<octeonRegType> res;
	if(!test_flag(IMM))
	{
		res.insert(operands[1].get_reg());
	}
	return res;
}

set<octeonRegType> octeonAlu2Op::getDefs()
{
	set<octeonRegType> res;
	res.insert(operands[0].get_reg());
	return res;
}

ostream& octeonAlu2Op::print(std::ostream& os) const
{
	octeon_Insn::print(os) << " " << operands[0] << ", ";
	
	if(test_flag(IMM))
	{
		return os << imm;
	}

	return os << operands[1];
}

octeonMulDivOp::octeonMulDivOp(octeonOpcodes opcode, octeonRegType& src1, octeonRegType& src2, octeonOpSize size)
:
	octeon_Insn(opcode)
{
	operands[0] = octeonRegOp(src1, size);
	operands[1] = octeonRegOp(src2, size);
}

set<octeonRegType> octeonMulDivOp::getUses()
{
	set<octeonRegType> res;
	res.insert(operands[0].get_reg());
	res.insert(operands[1].get_reg());
	return res;
}

ostream& octeonMulDivOp::print(std::ostream& os) const
{
	return octeon_Insn::print(os) << " " << operands[0] << ", " << operands[1];
}

octeonBinaryTreeBranch::octeonBinaryTreeBranch(octeonRegType reg, uint32_t value, string& my_name, string& target_name)
:
	myname(my_name),
	target_name(target_name),
	value(value),
	reg(reg)
{
}

ostream& octeonBinaryTreeBranch::print(ostream& os) const
{
	os << myname << ":" << endl;
	os << "\tslti $1, " << reg << ", " << value << endl;
	return os << "\tbeq $1, $0, " << target_name;
}

set<octeonRegType> octeonBinaryTreeBranch::getUses()
{
	set<octeonRegType> res;
	res.insert(reg);
	return res;
}

octeonBranchJumpTable::octeonBranchJumpTable(octeonRegType& offset, std::string& table_name)
:
	octeon_Insn(OCTEON_JR),
	offset(offset),
	table_name(table_name)
{
}

set<octeonRegType> octeonBranchJumpTable::getUses()
{
	set<octeonRegType> res;
	res.insert(offset);
	return res;
}

ostream& octeonBranchJumpTable::print(ostream& os) const
{
	os << "ld $1, %got_page("<< table_name << ")($28)" << endl;
	os << "\tdaddiu $1, $1, %got_ofst("<< table_name << ")" << endl;
	//os << "\tld $1, "<< <<"($1)" << endl;
	os << "\tdaddu $1, $1, " << offset << endl;
	os << "\tld $1, 0($1)" << endl;
	os << "\tjr $1";
	return os;
}

octeonStartTicks::octeonStartTicks(const string& pe_name)
:
	pe_name(pe_name)
{}

ostream& octeonStartTicks::print(ostream& os) const
{
	os << "\trdhwr $1, $31" << endl;
	os << "\tsd    $1, 48($sp)" << endl;
	return os;
}

octeonStopTicks::octeonStopTicks(const string& pe_name, octeonRegType& reg)
:
	pe_name(pe_name),
	op(reg, OC_DWORD)
{}

set<octeonRegType> octeonStopTicks::getDefs()
{
	set<octeonRegType> res;
	res.insert(op.get_reg());
	return res;
}

ostream& octeonStopTicks::print(ostream& os) const
{
	os << "\trdhwr $1, $31" << endl;
	os << "\tld	" << op << ", 48($sp)" << endl;
	os << "\tdsub $1, $1, " << op << endl;
	os << "\tld " << op << ", %got_disp(" << pe_name << "_ticks)($28)" << endl;
	os << "\tld " << op << ", 0(" << op << ")" << endl;
	os << "\tdadd $1, $1, " << op << endl; 
	os << "\tld " << op << ", %got_disp(" << pe_name << "_ticks)($28)" << endl;
	os << "\tsd $1, 0(" << op << ")" << endl;
	return os;
}

octeonLabel::octeonLabel(const string& label)
:
	octeon_Insn(OCTEON_NOP),
	label(label)
{
}

ostream& octeonLabel::print(ostream& os) const
{
	return os << label << ":";
}

octeonBranchLblOp::octeonBranchLblOp(octeonOpcodes opcode, uint32_t target, bool long_jump)
	:
		octeon_Insn(opcode),
		target(target),
		epilogue(false),
		long_jump(long_jump)
{
	assert(test_flag(LBL));
}

octeonBranchLblOp::octeonBranchLblOp(octeonOpcodes opcode, octeonRegType& target, bool long_jump)
	:
		octeon_Insn(opcode),
		reg_target(target, OC_DWORD),
		epilogue(false),
		long_jump(long_jump)
{
	assert(!test_flag(LBL));
}

octeonBranchLblOp::octeonBranchLblOp()
:
	octeon_Insn(OCTEON_J),
	epilogue(true),
	long_jump(false)
{
}

octeonCondBranch::octeonCondBranch(
	octeonOpcodes opcode, 
	uint32_t jt, 
	uint32_t jf, 
	bool long_jump)
:
	octeonBranchLblOp(opcode, jt, long_jump),
	false_target(jf)
{
}

bool octeonBranchLblOp::isLongJump(CFG<octeon_Insn>& cfg, uint32_t count) const
{
	uint32_t offset;

	if(epilogue)
	{
		offset = (cfg.get_insn_num() - count) * max_bytes_per_insn;
	}
	else
	{
		uint16_t target = get_target();
		uint32_t target_count = cfg.getBBById(target)->getProperty<uint32_t>(count_prop_name);
		offset = (count > target_count ? count - target_count : target_count - count) * max_bytes_per_insn; 
	}

	return offset > max_near_jump_offset;
}

void octeonBranchLblOp::toLongJump()
{
	long_jump = true;
}

void octeonCondBranch::invert()
{
	//do nothing
	return;
}

octeonOpcodes octeonCondBranch::get_inverted_opcode() const
{
	octeonOpcodes opcode(OCTEON_NOP);

	switch (getOpcode())
	{
		case OCTEON_BGEZ:
			opcode = OCTEON_BLTZ;
			break;
		case OCTEON_BGTZ:
			opcode = OCTEON_BLEZ;
			break;
		case OCTEON_BLEZ:
			opcode = OCTEON_BGTZ;
			break;
		case OCTEON_BLTZ:
			opcode = OCTEON_BGEZ;
			break;
		case OCTEON_BEQ:
			opcode = OCTEON_BNE;
			break;
		case OCTEON_BNE:
			opcode = OCTEON_BEQ;
			break;
		default:
			throw "Wrong opcode in jump";
			;
	}

	return opcode;
}

string octeonBranchLblOp::long_jump_to_target(const string& target)
{
	ostringstream result;
	result << "\tld  $1,%got_page(" << target << ")($28)" << endl;
	result << "\tdaddiu  $1,$1,%got_ofst(" << target << ")" << endl;
	result << "\tjr $1" << endl;
	return result.str();
}

string octeonBranchLblOp::long_jump_to_target(const octeonLabelOp& target)
{
	ostringstream result;
	result << "\tld  $1,%got_page(" << target << ")($28)" << endl;
	result << "\tdaddiu  $1,$1,%got_ofst(" << target << ")" << endl;
	result << "\tjr $1" << endl;
	return result.str();
}

ostream& octeonBranchLblOp::print(std::ostream& os) const
{
	if(epilogue)
	{
		return (long_jump ?
				os << long_jump_to_target(".Lepilogue") :
				octeon_Insn::print(os) << " .Lepilogue");
	}

	if(test_flag(LBL))
	{
		return (long_jump ?
				os << long_jump_to_target(target) :
				octeon_Insn::print(os) << " " << target);
	}

	return octeon_Insn::print(os) << reg_target;
}

octeonBranchCmp1Op::octeonBranchCmp1Op(octeonOpcodes opcode, octeonRegType& src, uint32_t jt, uint32_t jf, octeonOpSize size)
	:
		octeonCondBranch(opcode, jt, jf),
		op(src, size)
{
}

void octeonBranchCmp1Op::invert()
{
	octeonLabelOp tmp = target;
	target = false_target;
	false_target = tmp;
	
	setOpcode( get_inverted_opcode() );
}

void octeonBranchCmp2Op::invert()
{
	octeonLabelOp tmp = target;
	target = false_target;
	false_target = tmp;
	setOpcode( get_inverted_opcode() );
}

set<octeonRegType> octeonBranchCmp1Op::getUses()
{
	set<octeonRegType> res;
	res.insert(op.get_reg());
	return res;
}

ostream& octeonBranchCmp1Op::print(std::ostream& os) const
{
	if (!long_jump)
		octeon_Insn::print(os) << " " << op << ", " << target;
	else
	{
		os << octeonInsDescription[get_inverted_opcode()].Name << " " << op << ", .L" << this << endl;
		os << long_jump_to_target(target);
		os << ".L" << this << ":" << endl;

		if( false_target.get_label() != 0 )
		{
			os << long_jump_to_target(false_target);
		}
		//else
		//{
		//	os << ".L" << this << endl;
		//	os << octeonInsDescription[OCTEON_J].Name << " " << target << endl;
		//	os << ".L" << this << ":";
		//}
	}
	
	return os;
}

octeonBranchCmp2Op::octeonBranchCmp2Op(
	octeonOpcodes opcode, 
	octeonRegType& src1, 
	octeonRegType& src2, 
	uint32_t jt,
	uint32_t jf,
	octeonOpSize size)
:
	octeonCondBranch(opcode, jt, jf)
{
	ops[0] = octeonRegOp(src1, size);
	ops[1] = octeonRegOp(src2, size);
}
	

set<octeonRegType> octeonBranchCmp2Op::getUses()
{
	set<octeonRegType> res;
	res.insert(ops[0].get_reg());
	res.insert(ops[1].get_reg());
	return res;
}

ostream& octeonBranchCmp2Op::print(std::ostream& os) const
{
	if (!long_jump)
		octeon_Insn::print(os) << " " << ops[0] << ", " << ops[1] << ", " << target;
	else
	{
		os << octeonInsDescription[get_inverted_opcode()].Name << " " << ops[0] << ", " << ops[1] << ", .L" << this << endl;

		os << long_jump_to_target(target);
		os << ".L" << this << ":" << endl;

		if( false_target.get_label() != 0 )
		{
			os << long_jump_to_target(false_target);
		}
		//else
		//{
		//	os << ".L" << this << endl;
		//	os << octeonInsDescription[OCTEON_J].Name << " " << target << endl;
		//	os << ".L" << this << ":";
		//}
	}
	
	return os;
}

octeonMoveOp::octeonMoveOp(octeonRegType& src, octeonRegType& dst, octeonOpSize size)
:
	octeon_Insn(),
	src(src, size),
	dst(dst, size)
{
	switch(dst.get_model()->get_name())
	{
		case HI:
			this->setOpcode(OCTEON_MTHI);
			break;
		case LO:
			this->setOpcode(OCTEON_MTLO);
			break;
	}

	switch(src.get_model()->get_name())
	{
		case HI:
			this->setOpcode(OCTEON_MFHI);
			break;
		case LO:
			this->setOpcode(OCTEON_MFLO);
			break;
	}

	if(getOpcode() == OCTEON_INVALID)
		setOpcode(OCTEON_MOVE);
}

set<octeonRegType> octeonMoveOp::getUses()
{
	set<octeonRegType> res;
	if(getOpcode() == OCTEON_MOVE)
		res.insert(src.get_reg());
	return res;
}

set<octeonRegType> octeonMoveOp::getDefs()
{
	set<octeonRegType> res;
	if(getOpcode() == OCTEON_MOVE)
		res.insert(dst.get_reg());
	return res;
}

ostream& octeonMoveOp::print(std::ostream& os) const
{
	octeon_Insn::print(os) << " ";

	switch(getOpcode())
	{
		case OCTEON_MTHI:
		case OCTEON_MTLO:
			os << src;
			break;
		case OCTEON_MFHI:
		case OCTEON_MFLO:
			os << dst;
			break;
		case OCTEON_MOVE:
			os << dst << ", " << src;
			break;
		default:
			assert(1 != 0);
	}

	return os;
}

list< pair <octeon_Insn::RegType, octeon_Insn::RegType> > octeonMoveOp::getCopiedPair() const
{
	list< pair <octeon_Insn::RegType, octeon_Insn::RegType> > res;

	if(isCopy())
	{
		res.push_back( make_pair(src.get_reg(), dst.get_reg()) );
	}

	return res;
}

octeonRegType octeonMoveOp::get_from() const
{
	if(isCopy())
		return src.get_reg();
	return octeonRegType();
}

octeonRegType octeonMoveOp::get_to() const
{
	if(isCopy())
		return dst.get_reg();
	return octeonRegType();
}

octeonRet::octeonRet()
:
	octeon_Insn(OCTEON_RET)
{}

ostream& octeonRet::print(ostream& os) const
{
	os << "ld $4, 0($sp)\n";
	os << "\tld $25, %call16(nvmReleaseWork)($28)\n";
	os << "\taddiu $5, $0, "<< 0 << "#id porta\n";
	os << "\tld $6, 16($sp)\n";
	os << "\tjal $25\n";
	return os << "\tnop\n";
}

ostream& octeonSendPkt::print(ostream& os) const
{
	nvmNetPE* pe = Application::getCurrentPEHandler()->OwnerPE;

	nvmPEPort* port = pe->PortTable + port_number;
	assert(port->PortFlags != 0);

	string next_func;

	if(nvmFLAG_ISSET(port->PortFlags, PORT_CONN_SOCK))
	{
		next_func = "nvmSendOut";
	}
	else if(nvmFLAG_ISSET(port->PortFlags, PORT_CONN_PE))
	{
		next_func = string(port->CtdPE->Name) + "_push";
	}
	else throw "Port connected to nothing!!!";

	os << "ld $4, 0($sp)\n";
	os << "\tld $25, %call16(" << next_func << ")($28)\n";
	os << "\taddiu $5, $0, "<< port_number << "#id porta\n";
	os << "\tld $6, 16($sp)\n";
	os << "\tjal $25\n";
	return os << "\tnop\n";
}

octeonSendPkt::octeonSendPkt(uint32_t port_number)
:
	octeon_Insn(OCTEON_SNDPKT),
	port_number(port_number)
{
}

octeonUnOp::octeonUnOp(octeonOpcodes opcode, octeonRegType& op, octeonOpSize size)
:
	octeon_Insn(opcode),
	operand(op, size)
{}

ostream& octeonUnOp::print(ostream& os) const
{
	return octeon_Insn::print(os) << " " << operand;
}

set<octeonRegType> octeonUnOp::getUses()
{
	set<octeonRegType> res;
	res.insert(operand.get_reg());
	return res;
}
