/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

inline jit::octeon::octeonOpType jit::octeon::octeonOperand::get_type() const
{
	return type;
}

inline jit::octeon::octeonOpSize jit::octeon::octeonOperand::get_size() const
{
	return size;
}

inline uint32_t jit::octeon::octeonImmOp::get_value() const
{
	return value;
}

inline uint32_t jit::octeon::octeonMemOp::get_displ() const
{
	return displ;
}

inline const std::string& jit::octeon::octeonMemOp::get_str_displ() const
{
	return str_displ;
}

inline const jit::octeon::octeonRegType& jit::octeon::octeonMemOp::get_base() const
{
	return base;
}

inline const jit::octeon::octeonRegType& jit::octeon::octeonRegOp::get_reg() const
{
	return reg;
}

inline uint32_t jit::octeon::octeonLabelOp::get_label() const
{
	return label;
}

inline jit::octeon::octeonRegType jit::octeon::octeonRegType::mach_reg(octeonRegs reg)
{
	return octeonRegType( RegisterInstance(MachSpace, (uint32_t)reg) );
}

inline jit::octeon::octeonRegType jit::octeon::octeonRegType::virt_reg(uint32_t name)
{
	return octeonRegType( RegisterInstance(VirtSpace, name) );
}
inline jit::octeon::octeonRegType jit::octeon::octeonRegType::new_virt_reg()
{
	return octeonRegType( RegisterInstance::get_new(VirtSpace) );
}

inline jit::octeon::octeonRegType jit::octeon::octeonRegType::spill_reg(uint32_t name)
{
	return octeonRegType( RegisterInstance(SpillSpace, name) );
}

inline jit::octeon::octeonRegType jit::octeon::octeonRegType::new_spill_reg()
{
	return octeonRegType( RegisterInstance::get_new(SpillSpace) );
}

inline bool jit::octeon::octeonRegType::is_spill_register() const
{
	return SpillSpace == get_model()->get_space();
}

inline bool jit::octeon::octeonRegType::is_mach_register() const
{
	return MachSpace == get_model()->get_space();
}

inline bool jit::octeon::octeonRegType::is_virt_register() const
{
	return VirtSpace == get_model()->get_space();
}

inline jit::octeon::octeon_Insn::IRNodeIterator::value_type& jit::octeon::octeon_Insn::IRNodeIterator::operator*()
{
	return ptr;
}

inline std::ostream& jit::octeon::octeon_Insn::printNode(std::ostream& os, bool inSSA) const
{
	return print(os);
}

inline jit::octeon::octeon_Insn::IRNodeIterator::value_type& jit::octeon::octeon_Insn::IRNodeIterator::operator->() 
{ 
	return ptr; 
}

inline jit::octeon::octeon_Insn::IRNodeIterator& jit::octeon::octeon_Insn::IRNodeIterator::operator++(int) 
{
	ptr = NULL; 
	return *this; 
}

inline bool jit::octeon::octeon_Insn::IRNodeIterator::operator==(const IRNodeIterator& it) const 
{ 
	return ptr == it.ptr;
}

inline bool jit::octeon::octeon_Insn::IRNodeIterator::operator!=(const IRNodeIterator& it) const 
{ 
	return ptr != it.ptr;
}

inline jit::octeon::octeon_Insn::IRNodeIterator jit::octeon::octeon_Insn::nodeBegin()
{
	return IRNodeIterator(this);
}

inline jit::octeon::octeon_Insn::IRNodeIterator jit::octeon::octeon_Insn::nodeEnd()
{
	return IRNodeIterator(NULL);
}

inline bool jit::octeon::octeon_Insn::has_side_effects()
{
	return false;
}

inline bool jit::octeon::octeonMoveOp::isCopy() const
{
	return getOpcode() == OCTEON_MOVE;
}

inline uint32_t jit::octeon::octeonBranchLblOp::get_target() const
{
	return target.get_label();
}

inline uint32_t jit::octeon::octeonCondBranch::get_false_target() const
{
	return false_target.get_label();
}

inline const std::string& jit::octeon::octeonJumpTable::getName() const
{
	return name;
}

inline void jit::octeon::octeonJumpTable::addTarget(uint16_t target)
{
	targets.push_back(target);
}
