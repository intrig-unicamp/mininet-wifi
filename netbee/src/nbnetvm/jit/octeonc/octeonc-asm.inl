/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

// octeoncRegType
inline jit::octeonc::octeoncRegType 
jit::octeonc::octeoncRegType::new_virt_reg()
{
	return octeoncRegType(RegisterInstance::get_new(VirtSpace));
}

inline jit::octeonc::octeonc_Insn::IRNodeIterator::value_type& 
jit::octeonc::octeonc_Insn::IRNodeIterator::operator*()
{
	return ptr;
}

// octeonc_Insn

inline std::ostream& 
jit::octeonc::octeonc_Insn::printNode(
std::ostream& os, bool inSSA) const
{
	return print(os);
}

// octeonc_GenerigInsn

inline std::ostream&
jit::octeonc::octeonc_GenerigInsn::print(std::ostream& os) const
{
    return os << insn_;
}

// octeonc_Insn::IRNodeIterator
inline jit::octeonc::octeonc_Insn::IRNodeIterator::value_type& 
jit::octeonc::octeonc_Insn::IRNodeIterator::operator->() 
{ 
	return ptr; 
}

inline jit::octeonc::octeonc_Insn::IRNodeIterator& 
jit::octeonc::octeonc_Insn::IRNodeIterator::operator++(int) 
{
	ptr = NULL; 
	return *this; 
}

inline bool 
jit::octeonc::octeonc_Insn::IRNodeIterator::operator==(
const IRNodeIterator& it) const 
{ 
	return ptr == it.ptr;
}

inline bool 
jit::octeonc::octeonc_Insn::IRNodeIterator::operator!=(
const IRNodeIterator& it) const 
{ 
	return ptr != it.ptr;
}

inline jit::octeonc::octeonc_Insn::IRNodeIterator 
jit::octeonc::octeonc_Insn::nodeBegin()
{
	return IRNodeIterator(this);
}

inline jit::octeonc::octeonc_Insn::IRNodeIterator 
jit::octeonc::octeonc_Insn::nodeEnd()
{
	return IRNodeIterator(NULL);
}
