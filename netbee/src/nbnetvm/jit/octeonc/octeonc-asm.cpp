/*!
 * \file octeonc-asm.cpp
 * \brief this file contains definition of routines working on octeonc instruction
 */
#include "octeonc-asm.h"
#include "application.h"
#include "int_structs.h"
#include "gc_regalloc.h"
#include <sstream>
#include <string>
#include <algorithm>

using namespace jit;
using namespace octeonc;
using namespace std;

const string jit::octeonc::count_prop_name("instruction_count");

// octeoncRegType
const uint32_t jit::octeonc::octeoncRegType::VirtSpace = 1;

octeoncRegType::octeoncRegType(const RegisterInstance& reg):
	RegisterInstance(reg)
{}

octeoncRegType::octeoncRegType(
    uint32_t space, uint32_t name, uint32_t version):
	RegisterInstance(space, name, version)
{}

ostream& jit::octeonc::operator<<(
    ostream& os, const octeoncRegType& reg)
{
    ostringstream oss;
    oss << (const RegisterInstance&) reg;
    string s2(oss.str());

    replace(s2.begin(), s2.end(), '.', '_');
	return os << s2;
}

// octeoncOperand

ostream& jit::octeonc::operator<<(
    ostream& os, const octeoncOperand& op)
{
    return op.print(os);
}

// octeonRegOperand

octeoncRegOperand::octeoncRegOperand(octeoncRegType reg)
: reg_(reg)
{}

ostream& octeoncRegOperand::print(ostream& os) const {
    return os << reg_;
}

// octeoncLitOperand

octeoncLitOperand::octeoncLitOperand(int32_t val)
: val_(val)
{}

ostream& octeoncLitOperand::print(ostream& os) const {
    return os << val_;
}

// octeonCFG
octeoncCFG::octeoncCFG(const std::string& name)
:
	CFG<octeonc_Insn>(name)
{ }

// octeonc_Insn
octeonc_Insn::octeonc_Insn() : IRNode<octeoncRegType>()
{ }

ostream& octeonc_Insn::print(ostream& os) const
{
	return os << "what???";
}

ostream& jit::octeonc::operator<<(
ostream& os, const octeonc_Insn& insn)
{
	return insn.print(os);
}

set<octeoncRegType> octeonc_Insn::getUses()
{
	return set<octeoncRegType>();
}

set<octeoncRegType> octeonc_Insn::getDefs()
{
	return set<octeoncRegType>();
}

// octeonc_GenerigInsn
octeonc_GenerigInsn::octeonc_GenerigInsn(const string& insn)
: insn_(insn)
{}

// octeonc_Unaryexpr

octeonc_Unaryexpr::octeonc_Unaryexpr(
MIRNode* insn, octeoncOperand* dst, octeoncOperand* src)
: op_(get_expr_op(insn)), dst_(dst), src_(src)
{}

ostream& octeonc_Unaryexpr::print(ostream& os) const {
    os << *dst_ << " = " << op_ << *src_ << "\n;";
    return os;
}

const char*
octeonc_Unaryexpr::get_expr_op(MIRNode* insn) {
	assert(insn != NULL);

	switch(insn->getOpcode())
	{
		case IINC_1:
            return "++";
			break;
		case IDEC_1:
            return "--";
			break;
		case NEG:
            return "-";
			break;
		case NOT:
            return "^";
			break;
		default:
			assert(1 == 0 && "alu opcode invalid");
			;
	}

	return "";
}

// octeonc_Binaryexpr

octeonc_Binaryexpr::octeonc_Binaryexpr(
MIRNode* insn, octeoncOperand* dst, 
octeoncOperand* src1, octeoncOperand* src2)
: op_(get_expr_op(insn)), dst_(dst), 
  src1_(src1), src2_(src2)
{}

const char*
octeonc_Binaryexpr::get_expr_op(MIRNode* insn) {
	assert(insn != NULL);

	switch(insn->getOpcode())
	{
		case ADD:
		case ADDUOV:
            return "+";
			break;
		case SUB:
		case SUBUOV:
			return "-";
			break;
		case AND:
            return "&";
			break;
		case OR:
			return "|";
			break;
		case SHR:
            return ">>";
			break;
		case USHR:
            return ">>";
			break;
        case MOD:
            return "%";
            break;
		case SHL:
			return "<<";
			break;
        case IMUL:
            return "*";
            break;
       // TODO
       // case IDIV:
       //     return "/";
       //     break;
		default:
			assert(1 == 0 && "alu opcode invalid");
			;
	}

	return "";
}

ostream& octeonc_Binaryexpr::print(ostream& os) const
{
    //os << "int32_t " << *dst_ << " = "
    os << *dst_ << " = "
       << *src1_ << " " << op_ << " " << *src2_ << ";";
    return os;
}

// octeonc_Jump
octeonc_Jump::octeonc_Jump(
    JumpMIRNode* insn, uint32_t jt, uint32_t jf, 
    octeoncOperand* src1, octeoncOperand* src2)
: op_(get_cond_op(insn)), jt_(jt), jf_(jf), src1_(src1), src2_(src2)
{}

const char*
octeonc_Jump::get_cond_op(JumpMIRNode* insn) {
	assert(insn != NULL);
	switch(insn->getOpcode()) {
		case JNE:
		case JCMPNEQ:
		case JFLDNEQ:
			return "!=";
			break;
		case JCMPGE:
			return ">=";
			break;
		case JCMPG:
		case JFLDGT:
			return ">";
			break;
		case JCMPL:
		case JFLDLT:
            return "<";
			break;
		case JCMPLE:
            return "<=";
			break;
		case JCMPEQ:
		case JEQ:
		case JFLDEQ:
			return "==";
			break;
		default:
			assert(1 == 0 && "jump opcode invalid");
			;
	}
	// Never reached
	return "";
}

ostream& octeonc_Jump::print(ostream& os) const {
    os << "if (" << *src1_ << " " << op_ << " " << *src2_ << ")\n";
    os << "\t\tgoto L" << jt_ << ";\n";
    os << "\telse\n\t\tgoto L" << jf_ << ";\n";
    return os;
}

// octeonc_MemOp

octeonc_MemOp::octeonc_MemOp(MIRNode* insn)
: memtype_(get_memtype(insn)), datatype_(get_datatype(insn))
{
    ostringstream os;
    nvmNetPE* pe = 
        Application::getApp(1).getCurrentPEHandler()->OwnerPE;
    os << pe->Name << "_data";
    data_name_ = os.str();
}

octeonc_MemOp::octeonc_DataType octeonc_MemOp::get_datatype(MIRNode *insn) {
    switch (insn->getOpcode()) {
        case DBLDS:
        case ISSBLD:
        case PBLDS:
            return INT8;

        case DBLDU:
        case ISBLD:
        case PBLDU:
        case DBSTR:
        case IBSTR:
        case PBSTR:
            return UINT8;

        case DSLDS:
        case ISSSLD:
        case PSLDS:
            return INT16;

        case DSLDU:
        case ISSLD:
        case PSLDU:
        case DSSTR:
        case ISSTR:
        case PSSTR:
            return UINT16;

        case DILD:
        case ISSILD:
        case PILD:
        case DISTR:
        case IISTR:
        case PISTR:
            return INT32;
    }
    return INVALID_DATATYPE;
}

octeonc_MemOp::octeonc_MemType octeonc_MemOp::get_memtype(MIRNode *insn) {
    switch (insn->getOpcode()) {
        case DBLDS:
        case DBLDU:
        case DBSTR:
        case DILD:
        case DISTR:
        case DSLDS:
        case DSLDU:
        case DSSTR:
            return DATA;

        case IBSTR:
        case IISTR:
        case ISBLD:
        case ISSBLD:
        case ISSILD:
        case ISSLD:
        case ISSSLD:
        case ISSTR:
            return INFO;

        case PBLDS:
        case PBLDU:
        case PBSTR:
        case PILD:
        case PISTR:
        case PSLDS:
        case PSLDU:
        case PSSTR:
            return PKT;
    }
    return INVALID_TYPE;
}

const char* octeonc_MemOp::get_base() const {
    switch (memtype_) {
        case PKT:
            return "(*exbuf)->PacketBuffer";
            break;
        case INFO:  
            return "(*exbuf)->InfoData";
            break;
        case DATA:
            return data_name_.c_str();
            break;
        case INVALID_TYPE:
        default:
            assert(1 == 0 && "wrong memory type");
            break;
    }
    return "";
}

const char* octeonc_MemOp::get_type() const {
    switch(datatype_) {
        case INT8:
            return "int8_t";
            break;
        case INT16:
            return "int16_t";
            break;
        case INT32:
            return "int32_t";
            break;
        case UINT8:
            return "uint8_t";
            break;
        case UINT16:
            return "uint16_t";
            break;
        case UINT32:
            return "uint32_t";
            break;
        default:
            assert(1 == 0 && "wrong data type");
            ;
    }
    return "";
}

// octeonc_Load
octeonc_Load::octeonc_Load(
        MIRNode* insn, octeoncOperand* offset, octeoncOperand* dst):
    octeonc_MemOp(insn), offset_(offset), dst_(dst)
{}

ostream& octeonc_Load::print(std::ostream& os) const {
    // os << "int32_t "<< *dst_ << " = (int32_t)("  //assing

    os << *dst_ << " = (int32_t)*("  //assing
       << "("<< get_type() << "*)" //cast
       << "(" << get_base() << " + " << *offset_ << ")" // offset
       << ");\n";
    return os;
}

// octeonc_Store
octeonc_Store::octeonc_Store(
        MIRNode* insn, octeoncOperand* offset, octeoncOperand* src):
    octeonc_MemOp(insn), offset_(offset), src_(src)
{}

ostream& octeonc_Store::print(std::ostream& os) const {

    os << "*(" << get_type() << "*)("              //cast
       << get_base() << " + " << *offset_ << ") = " // offset
       << "("<< get_type() << ")" << *src_ << ";\n";
    return os;
}

// octeon_Ins::IRNodeIterator
octeonc_Insn::IRNodeIterator::IRNodeIterator(octeonc_Insn* insn)
: ptr(insn) 
{}

octeonc_Insn::IRNodeIterator::IRNodeIterator(const IRNodeIterator &it)
: ptr(it.ptr) 
{}

