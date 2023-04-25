/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file mirnode.cpp
 * \brief definition of the classes MIRNode
 */
#include "mirnode.h"
#include "cfg_ssa.h" // needed for SSA MASKS
#include "application.h"
#include <cassert>
#include <iostream>
using std::cout;

namespace jit {

	/*!
	 * \param opcode the opcode of this node
	 * \param BBId the id of the BB we belong to
	 * \param defReg the virtual register defined by this node
	 * \param inlist true if this node is in a BB list
	 */
	MIRNode::MIRNode(OpCodeType opcode, uint32_t BBId, RegType defReg, bool inlist)
		: jit::TableIRNode<RegType , OpCodeType>(opcode, BBId), numRefs(0), defReg(defReg), isNotValid(false), isLinked(false), isDepend(false), depend(NULL), linked(NULL), inlist(inlist)
	{
		kids[0] = kids[1] = NULL;
	}

	/*!
	 * calls copy()
	 */
	MIRNode::MIRNode(const MIRNode& n) : jit::TableIRNode<RegType , OpCodeType >( n.getOpcode(), n.belongsTo()), defReg(n.defReg),  isNotValid(false), isLinked(false), isDepend(false), depend(NULL), linked(NULL), inlist(n.inlist)
	{
		int i;
		for(i=0; i<2; i++)
		{
			if(n.getKid(i) != NULL)
				kids[i] = n.getKid(i)->copy();
			else
				kids[i] = NULL;
		}
	}

	void MIRNode::rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id)
	{
		assert(1 == 0 && "rewrite_destination called on a non-jump IR node");
		return;
	}

	void MIRNode::rewrite_use(RegType, RegType)
	{
		// Nop by default
		return;
	}

	/*!
	 * also deletes this node's kids
	 */
	MIRNode::~MIRNode()
	{
		if(isLinked == true)
		{

			linked->isNotValid = true;
			linked->setDepend(NULL);
			linked->setIsDepend(false);
			
		}
		if( isDepend == true)
		{

			depend->setLinked(NULL);
			depend->setIsLinked(false);

		}

		delete kids[0];
		kids[0] = NULL;
		delete kids[1];
		kids[1] = NULL;
	}

	std::ostream& MIRNode::printSignature(std::ostream &os)
	{
		os << nvmOpCodeTable[getOpcode()].CodeName;

		os << " ( ";

		for(unsigned i = 0; i < 2; ++i) {
			if(kids[i]) {
				if(i)
					os << ", ";

				kids[i]->printSignature(os);
			}
		}

		os << ")";

		return os;
	}

	std::ostream& MIRNode::printNode(std::ostream &os, bool SSAForm = false)
	{
		os << nvmOpCodeTable[getOpcode()].CodeName ;

		operand_size_t size(getProperty<operand_size_t>("size"));
		if(size) {
			os << "<";
			switch(size) {
				case byte8:
					os << " 8";
					break;

				case word16:
					os << "16";
					break;

				case dword32:
					os << "32";
					break;

				default:
					os << "??";
			}
			os << "> ";
		}

		if(Get_Defined_Count(getOpcode()) == ONE_DEF) {
			os << " [";
			if(SSAForm) {
				operator<<(os, defReg);
			} else {
				operator<<(os, *(defReg.get_model()));
			}
			os << "]";
		}

		os << " ( ";

		for(unsigned i = 0; i < 2; ++i) {
			if(kids[i]) {
				if(i)
					os << ", ";

				kids[i]->printNode(os, SSAForm);
			}
		}

		os << ")";

		return os;
	}

	/*!
	 * only the registers written by a STREG are returned
	 * FIXME: FALSE. That's _not_ _nearly_ _enough_. Moreover, is this for this node only or
	 * for the entire subtree?
	 */
	std::set<MIRNode::RegType> MIRNode::getDefs()
	{
		std::set<RegType> res;

		if(Get_Defined_Count(getOpcode()) == ONE_DEF) {
			res.insert(defReg);
		}

/*
		for(unsigned i(0); i < 2; ++i) {
			if(kids[i]) {
				std::set<RegType> t(kids[i]->getDefs()), p(res);
				res.clear();
				set_union(p.begin(), p.end(), t.begin(), t.end(),
					std::insert_iterator<std::set<RegType> >(res, res.begin()));
			}
		}
*/

		return res;
	}

	std::set<MIRNode::RegType> MIRNode::getAllDefs()
	{
//		std::cout << "chiamo getalldefs per ";
//		this->printNode(std::cout, true);
//		std::cout << '\n';

		std::set<RegType> res(getDefs());

//		std::cout << "Def locali:\n";
//		std::set<RegType>::iterator w;
//		for(w = res.begin(); w != res.end(); ++ w) {
//			std::cout << *w << '\n';
//		}

		for(unsigned i(0); i < 2; ++i) {
			if(kids[i]) {
//				std::cout << "scendo nel figlio " << i << '\n';
				std::set<RegType> t(kids[i]->getAllDefs()), p(res);
				res.clear();
				set_union(p.begin(), p.end(), t.begin(), t.end(),
					std::insert_iterator<std::set<RegType> >(res, res.begin()));
			}
		}


		return res;
	}


	/*!
	 * only the register read by a LDReg are returned
	 * FIXME: Same comments as for getDefs...
	 */
	std::set<MIRNode::RegType> MIRNode::getUses()
	{
		std::set<RegType> res;

		// Add to the set the registers directly used by this instruction
		switch(nvmOpCodeTable[getOpcode()].Consts & (OP_COUNT_MASK)) {
			case TWO_OPS:
				if(kids[1]->getOwnReg()) {
					res.insert(*(kids[1]->getOwnReg()));
				}
				// fall-through
			case ONE_OP:
				if(kids[0]->getOwnReg()) {
					res.insert(*(kids[0]->getOwnReg()));
				}
//				if(Get_Defined_Count(kids[0]->getOpcode()) == ONE_DEF) {
//					res.insert(kids[0]->getDefReg());
//				}
				break;

			default:
				;
		}

#if 0
		for(int i=2; i > 0; --i)
		{
			if (kids[i-1]) {
				std::set<RegType> def = kids[i-1]->getUses();
				set_union(res.begin(), res.end(), def.begin(), def.end(), std::insert_iterator< std::set<RegType> >(res, res.begin()));
			}
		}
#endif

		return res;
	}

	std::set<MIRNode::RegType> MIRNode::getAllUses()
	{
		std::set<RegType> res(getUses());

#if 0
		// Add to the set the registers directly used by this instruction
		switch(nvmOpCodeTable[getOpcode()].Consts & (OP_COUNT_MASK)) {
			case TWO_OPS:
				if(Get_Defined_Count(kids[1]->getOpcode()) == ONE_DEF) {
					res.insert(kids[1]->getDefReg());
				}
				// fall-through
			case ONE_OP:
				if(Get_Defined_Count(kids[0]->getOpcode()) == ONE_DEF) {
					res.insert(kids[0]->getDefReg());
				}
				break;

			default:
				;
		}
#endif

		for(int i=2; i > 0; --i)
		{
			if (kids[i-1]) {
				std::set<RegType> def = kids[i-1]->getAllUses();
				set_union(res.begin(), res.end(), def.begin(), def.end(), std::insert_iterator< std::set<RegType> >(res, res.begin()));
			}
		}

		return res;
	}


	std::list<std::pair<MIRNode::RegType, MIRNode::RegType> > MIRNode::getCopiedPair()
	{
		std::list<std::pair<RegType, RegType> > copylist;

		if(getOpcode() == STREG && getKid(0) && getKid(0)->getOwnReg())
//		if(getOpcode() == STREG && getKid(0) && getKid(0)->getOpcode() == LDREG && getKid(0)->getOwnReg())
		{
			copylist.push_back(std::pair<RegType,RegType>(getDefReg(), *(getKid(0)->getOwnReg())));
		}
		return copylist;
	}

	MIRNode::ConstType * MIRNode::make_const(uint16_t bbID, MIRNode::ValueType const_value, MIRNode::RegType defReg)
	{
		return new ConstNode(bbID, const_value, defReg);
	}

	MIRNode::StoreType * MIRNode::make_store(uint16_t bbID, MIRNode *kid, MIRNode::RegType defReg)
	{
		return MIRNode::unOp(STREG, bbID, kid, defReg);
	}

	MIRNode::LoadType *MIRNode::make_load(uint16_t bbID, MIRNode::RegType loadReg)
	{
		return new LDRegNode(bbID, loadReg);
	}

	bool MIRNode::isConst()
	{
		return false;
	}

	bool MIRNode::isStore()
	{
		return getOpcode() == STREG;
	}


	bool MIRNode::isLoad()
	{
		return getOpcode() == LDREG;
	}

	/*!
	 * return the node defined by this ldreg
	 */
	std::set<MIRNode::RegType> LDRegNode::getUses()
	{
		std::set<RegType> res;
		res.insert(defReg);
		return res;
	}

	std::set<LDRegNode::RegType> LDRegNode::getDefs()
	{
		return std::set<RegType>();
	}

	void LDRegNode::rewrite_use(RegType old, RegType new_reg)
	{
		assert(old == defReg && "Tried to rewrite the wrong register in a LDREG node!");
		defReg = new_reg;
		return;
	}

	void LDRegNode::set_preferred_size(operand_size_t size)
	{
		switch(size) {
			case byte8:
				OpCode = LDREG8;
				break;
			case word16:
				OpCode = LDREG16;
				break;
			case dword32:
				OpCode = LDREG;
				break;
			case size_unknown:
				break;
			default:
				assert(1 == 0 && "operand_size_t out of bounds");
		}

		return;
	}

	MIRNode *MIRNode::make_copy(uint16_t id, RegType src, RegType dst)
	{
		LDRegNode *ldreg(new LDRegNode(id, src));
		MIRNode *streg(unOp(STREG, id, ldreg, dst));
		return streg;
	}

	/*!
	 * \param opcode the opcode of this node
	 * \param BBId the id of the BB we belong to
	 * \param kid this node left kid
	 * \param defReg the virtual register defined by this node
	 * \return the new created node
	 */
	MIRNode* MIRNode::unOp(uint8_t opcode, uint32_t BBId, MIRNode *kid, RegType defReg)
	{
		MIRNode* newNode;

		newNode = new MIRNode(opcode, BBId, defReg);
		newNode->setKid(kid, 0);
		return newNode;
	}

	/*!
	 * \param opcode the opcode of this node
	 * \param BBId the id of the BB we belong to
	 * \param kid1 this node left kid
	 * \param kid2 this node right kid
	 * \param defReg the virtual register defined by this node
	 * \return the new created node
	 */
	MIRNode* MIRNode::binOp(uint8_t opcode, uint16_t BBId, MIRNode *kid1, MIRNode *kid2, MIRNode::RegType defReg)
	{
		MIRNode* newNode;

		newNode = new MIRNode(opcode, BBId, defReg);
		newNode->setKid(kid1, 0);
		newNode->setKid(kid2, 1);
		return newNode;
	}

	/*!
	 * \param opcode the opcode of this node
	 * \param BBId the id of the BB we belong to
	 * \param defReg the virtual register defined by this node
	 * \return the new created node
	 */
	MIRNode* MIRNode::leafNode(uint8_t opcode, uint32_t BBId, MIRNode::RegType defReg)
	{
		MIRNode* newNode;

		switch(opcode) {
			case LDREG:
				newNode = new LDRegNode(BBId, defReg);
				break;
			default:
				newNode = new MIRNode(opcode, BBId, defReg);
		}

		return newNode;
	}

	/*
	 * \brief print operation
	 *
	 * class printNode()
	 */
	std::ostream& operator<<(std::ostream& os, jit::MIRNode& node)
	{
		node.printNode(os);
		return os;
	}

	std::ostream& JumpMIRNode::printNode(std::ostream &os, bool SSAForm)
	{
		MIRNode::printNode(os, SSAForm);
		if(this->OpCode == JUMP || this->OpCode == JUMPW)
			os << "-> " << jt << std::endl;
		else
		{
			if(this->OpCode != RET)
				os << "TRUE: " << jt << ", FALSE: " << jf <<std::endl;
		}
		return os;
	}

	void JumpMIRNode::rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id)
	{
		if(jt == old_bb_id)
			jt = new_bb_id;
		if(jf == old_bb_id)
			jf = new_bb_id;
		return;
	}

#if 0
	std::ostream& operator<<(std::ostream& os, SSARegPrinter& rp)
	{
		os << SSA_PREFIX(rp._reg) << "." << SSA_SUFFIX(rp._reg);
		return os;
	}
#endif

	std::ostream& SwitchMIRNode::printNode(std::ostream &os, bool SSAForm)
	{
		MIRNode::printNode(os, SSAForm);
		for(targets_iterator i = TargetsBegin(); i != TargetsEnd(); i++)
			os << "CASE: " << i->first << " TARGET: " << i->second << std::endl;
		os << "DEFAULT: " << sw_info.defTarget << '\n';
		return os;
	}

	void SwitchMIRNode::rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id)
	{
		targets_iterator i;
		for(i = TargetsBegin(); i != TargetsEnd(); ++i) {
			if(i->second == old_bb_id)
				i->second = new_bb_id;
		}

		if(sw_info.defTarget == old_bb_id)
			sw_info.defTarget = new_bb_id;

		return;
	}

	std::ostream& PhiMIRNode::printNode(std::ostream &os, bool SSAForm )
	{
		os << "PHI (";
//		SSARegPrinter rp(defReg);
//		os << rp;

		if(SSAForm)
			operator<<(os, defReg);
		else
			operator<<(os, *(defReg.get_model()));

		os << " <-- ";
		for(params_iterator i = paramsBegin(); i!=paramsEnd(); i++)
		{
			if(i != paramsBegin())
				os << ", ";
//			SSARegPrinter rp(*i);
//			os << rp;
			if(SSAForm) {
				operator<<(os, i->second);
			} else {
				operator<<(os, *((i->second).get_model()));
			}

			os << " from " << i->first;

		}
		os << ")" << std::endl;

		return os;
	}

#if 0
	std::ostream& operator<<(std::ostream& os, CodePrinterSSA& cp){
		cp._node.printNode(os, true);
		return os;
	}
#endif

	PhiMIRNode *MIRNode::make_phi(uint16_t bb, RegType reg) {
		return new PhiMIRNode(bb, reg);
	}

	std::set<MIRNode::RegType> PhiMIRNode::getUses()
	{
		typedef PhiMIRNode::params_iterator parsIt;
		std::set<RegType> usesSet;

		for(parsIt i = paramsBegin(); i != paramsEnd(); i++)
			usesSet.insert(i->second);
		return usesSet;
	}

	MIRNode::OpCodeType MIRNode::getOpcode() const
	{
		return OpCode;
	}

	void MIRNode::setOpcode(OpCodeType op_code)
	{
		OpCode = op_code;
	}

	MIRNode::ContType MIRNode::get_reverse_instruction_list()
	{
		ordered_list.clear();
		sortPostOrder();
		ContType t(ordered_list);
		t.reverse();
		return t;
	}

	uint32_t MIRNode::get_defined_count() {
		return Get_Defined_Count(getOpcode()) == ONE_DEF ? 1 : 0;
	}

	bool MIRNode::isCopy() const {
#if 0
		cout << "Controllo copia: " << nvmOpCodeTable[getOpcode()].CodeName;
		if(getOpcode() != STREG) {
			cout << " xx ";
		}
		if(kids[0]) {
			cout << " figlio: " << nvmOpCodeTable[kids[0]->getOpcode()].CodeName;
			if(kids[0]->getOwnReg()) {
				cout << ", registro: " << *(kids[0]->getOwnReg());
			}
		}

		if(!(getOpcode() == STREG && (kids[0] != 0) && (kids[0]->getOwnReg() != 0)))
			cout << " NO";
		else
			cout << " OK";

		cout << '\n';
#endif

#if 1
		return (getOpcode() == STREG) && (kids[0] != 0) && ((kids[0]->getOpcode() == LDREG) != 0);
#endif

//		return getOpcode() == STREG && kids[0] && kids[0]->getDefReg() != RegisterInstance::invalid;
	}
	/*Interface for reassociation */

	bool MIRNode::isOpMemPacket()
	{
		switch(getOpcode())
		{
			case PBLDS:
			case PBLDU:
			case PSLDS:
			case PSLDU:
			case PILD:
			case PBSTR:
			case PSSTR:
			case PISTR:
				return true;
				break;
			default:
				return false;
				break;
		}
		return false;
	}

	bool MIRNode::isOpMemData()
	{
		switch(getOpcode())
		{
			case DBLDS:
			case DBLDU:
			case DSLDS:
			case DSLDU:
			case DILD:
			case DBSTR:
			case DSSTR:
			case DISTR:
				return true;
				break;
			default:
				return false;
				break;
		}
		return false;
	}

	bool MIRNode::isMemLoad()
	{
		switch(getOpcode())
		{
			case PBLDS:
			case PBLDU:
			case PSLDS:
			case PSLDU:
			case PILD:
			case DBLDS:
			case DBLDU:
			case DSLDS:
			case DSLDU:
			case DILD:
			case SBLDS:
			case SBLDU:
			case SSLDS:
			case SSLDU:
			case SILD:
			case ISBLD:
			case ISSLD:
			case ISSBLD:
			case ISSSLD:
			case ISSILD:
				return true;
				break;
			default:
				return false;
				break;
		}
		return false;
	}

	bool MIRNode::isMemStore()
	{
		switch(getOpcode())
		{
			case DBSTR:
			case DSSTR:
			case DISTR:
			case PBSTR:
			case PSSTR:
			case PISTR:
			case SBSTR:
			case SSSTR:
			case SISTR:
			case IBSTR:
			case ISSTR:
			case IISTR:
				return true;
				break;
			default:
				return false;
				break;
		}
		return false;
	}

	uint8_t MIRNode::num_kids(2);

	/*END Interface for reassociation */

	void ConstNode::set_preferred_size(operand_size_t size) {
		switch(size) {
			case byte8:
				OpCode = CNST8;
				break;
			case word16:
				OpCode = CNST16;
				break;
			case dword32:
				OpCode = CNST;
				break;
			default:
				assert(1 == 0 && "operand_size_t out of range");
		}
		return;
	}

Nodelet::Nodelet(uint16_t bb_id, RegType def, std::set<RegType> u) :
	MIRNode(NOP, bb_id, def), uses(u), original(def)
{
	std::set<RegType>::iterator i;
	for(i = uses.begin(); i != uses.end(); ++i) {
		use_mapping[*i] = *i;
		reverse[*i] = *i;
	}
	return;
}

#if 0
void Nodelet::setDefReg(RegType r)
{


	return;
}
#endif

std::ostream& Nodelet::printNode(std::ostream &os, bool SSAForm)
{
	os << "nodelet ";

	if(defReg != RegisterInstance::invalid) {
		os << '[' << defReg << "] ";
	}

	if(uses.size()) {
		os << '(';
		std::set<RegType>::iterator i;
		for(i = uses.begin(); i != uses.end(); ++i) {
			os << *i << ' ';
		}
		os << ')';
	}

	return os;
}

Nodelet::Nodelet(const Nodelet &n) : MIRNode(n), uses(n.uses)
{
	// Nop
	return;
}

Nodelet::~Nodelet()
{
	// Nop
	return;
}

void Nodelet::set_preferred_size(operand_size_t size)
{
#if 1
	if(defReg != RegisterInstance::invalid) {
		defReg.setProperty<operand_size_t>("size", size);
	}
#endif

	std::set<RegType>::iterator i;
	std::set<RegType> shadow;
	for(i = uses.begin(); i != uses.end(); ++i) {
		RegisterInstance t(*i);
		t.setProperty<operand_size_t>("size", size);
		shadow.insert(t);
	}

	uses = shadow;

	return;
}

void Nodelet::rewrite_use(RegType old, RegType new_reg)
{
	if(uses.find(old) != uses.end()) {
		RegType t(reverse[old]);
		reverse[new_reg] = reverse[old];
		use_mapping[t] = new_reg;

		uses.erase(old);
		uses.insert(new_reg);
	}
	return;
}

std::set<Nodelet::RegType> Nodelet::getUses()
{
	return uses;
}

std::set<Nodelet::RegType> Nodelet::getDefs()
{
	std::set<RegType> defs;

	if(defReg != RegType::invalid)
		defs.insert(defReg);

	return defs;
}

Nodelet *Nodelet::copy()
{
	return new Nodelet(*this);
}


CopMIRNode::CopMIRNode(uint8_t opcode, uint16_t BBId, uint32_t cId, uint32_t opId, nvmCoprocessorState *cop) :
	MIRNode(opcode, BBId), coproId(cId), coproOp(opId), state(cop),
	base(Application::getApp(BBId).getCoprocessorBaseRegister(coproId)),
	space(Application::getApp(BBId).getCoprocessorRegSpace())
{

	if(opcode == COPPKTOUT)
		return;

	// Build the uses/defs set
	assert(cop != 0 && "Coprocessor not found!");
	std::set<RegType> uses, defs, empty;

	for(uint32_t i(0); i < cop->n_regs; ++i) {
		if(cop->reg_flags[i] & COPREG_READ /* COPREG_WRITE */) {
		//	std::cout << "CopMIRNode: definisco il registro " << i << '\n';
			RegisterInstance r(space, base + i);
			reg_mapping[i] = r;
			reverse_mapping[r] = i;
			defs.insert(r);
		}
		if(cop->reg_flags[i] & COPREG_WRITE /* COPREG_READ */) {
		//	std::cout << "CopMIRNode: uso il registro " << i << '\n';
			RegisterInstance r(space, base + i);
			reg_mapping[i] = r;
			reverse_mapping[r] = i;
			uses.insert(r);
		}
	}

	// Create the first child
	Nodelet *use_node(new Nodelet(BBId, RegisterInstance::invalid, uses));
	Nodelet *chain_head(use_node);

	std::set<RegType>::iterator i;
	for(i = defs.begin(); i != defs.end(); ++i) {
		Nodelet *t(new Nodelet(BBId, *i, empty));
		t->setKid(chain_head, 0);
		chain_head = t;
	}

	setKid(chain_head, 0);

	return;
}

CopMIRNode::CopMIRNode(const CopMIRNode& n) :
	MIRNode(n), coproId(n.coproId), coproOp(n.coproOp), coproInitOffset(n.coproInitOffset),
	reverse_mapping(n.reverse_mapping), reg_mapping(n.reg_mapping)
{
	// Nop
	return;
}

CopMIRNode::~CopMIRNode()
{
	// Nop
	return;
}

CopMIRNode *CopMIRNode::copy()
{
	CopMIRNode *other = new CopMIRNode(*this);
	return other;
}

RegisterInstance CopMIRNode::getCoproReg(uint32_t which_one, bool is_use)
{
	if(is_use) {
		Nodelet *n(dynamic_cast<Nodelet *>(kids[0]));
//		uint32_t base(Application::getApp().getCoprocessorBaseRegister(coproId));
//		uint32_t space(Application::getApp().getCoprocessorRegSpace());

		// Fetch the uses map
		while(n->getKid(0))
			n = dynamic_cast<Nodelet *>(n->getKid(0));

		return n->getUseMap(RegisterInstance(space, base + which_one));
	}

	Nodelet *n(dynamic_cast<Nodelet *>(kids[0]));
//	uint32_t base(Application::getApp().getCoprocessorBaseRegister(coproId));
	while(n) {
		if(n->getOriginal().get_model()->get_name() - base == which_one)
			return n->getDefReg();
		n = dynamic_cast<Nodelet *>(n->getKid(0));
	}

	return RegisterInstance::invalid;
}

#if 0
void CopMIRNode::rewrite_use(RegType old, RegType new_reg)
{
	if(uses.find(old) != uses.end()) {
		uint32_t reg_no(reverse_mapping[old]);

		cout << "Aggiorno il mapping di " << reg_no << " da " << old << " a " << new_reg << '\n';

		reg_mapping[reg_no] = new_reg;
		reverse_mapping[new_reg] = reg_no;
	}

	return;
}
#endif

bool CopMIRNode::emission_mode(false);

void CopMIRNode::unset_for_emission()
{
	emission_mode = false;
	return;
}

void CopMIRNode::set_for_emission()
{
	emission_mode = true;
	return;
}

nvmCoprocessorState *CopMIRNode::get_state()
{
	return state;
}

uint32_t CopMIRNode::get_base() const
{
	return base;
}

uint32_t CopMIRNode::get_space() const
{
	return space;
}

bool CopMIRNode::isUsedReg(uint32_t which_one) const
{
	nvmCoprocessorState *cop(state);
	assert(cop != 0 && "Coprocessor not found!");
	assert(which_one < cop->n_regs && "Coprocessor out of bounds!");

	return !!(cop->reg_flags[which_one] & COPREG_WRITE);
}

bool CopMIRNode::isDefdReg(uint32_t which_one) const
{
	nvmCoprocessorState *cop(state);
	assert(cop != 0 && "Coprocessor not found!");
	assert(which_one < cop->n_regs && "Coprocessor out of bounds!");

	return !!(cop->reg_flags[which_one] & COPREG_READ);
}

uint32_t CopMIRNode::regNo() const
{
	nvmCoprocessorState *cop(state);
	assert(cop != 0 && "Coprocessor not found!");

	return cop->n_regs;
}

std::ostream& CopMIRNode::printNode(std::ostream &os, bool SSAForm)
{
	os << nvmOpCodeTable[getOpcode()].CodeName ;

	if(OpCode == COPPKTOUT)
	{
		return os << "()" << std::endl;
	}

	operand_size_t size(getProperty<operand_size_t>("size"));
	if(size) {
		os << "<";
		switch(size) {
			case byte8:
				os << " 8";
				break;

			case word16:
				os << "16";
				break;

			case dword32:
				os << "32";
				break;

			default:
				os << "??";
		}
		os << "> ";
	}



	std::set<RegType> defs(getAllDefs());
	if(defs.size()) {
		os << '[';


		nvmCoprocessorState *cop(state);
		assert(cop != 0 && "Coprocessor not found!");

		for(uint32_t i(0); i < cop->n_regs; ++i)
			if(cop->reg_flags[i] & COPREG_READ /* COPREG_WRITE */) {
				cout << i << " -> " << getCoproReg(i, false) << ' ';
			}

#if 0
		std::set<RegType>::iterator i;
		for(i = defs.begin(); i != defs.end(); ++i) {
			if(SSAForm) {
				operator<<(os, *i);
			} else {
				operator<<(os, *(i->get_model()));
			}
			os << " -> " << reverse_mapping[*i];
			os << ' ';
		}
#endif
		os << ']';
	}

	os << " ( ";

#if 0
	std::set<RegType> uses(getAllUses());
	std::set<RegType>::iterator i;
	for(i = uses.begin(); i != uses.end(); ++i) {
		if(SSAForm) {
			operator<<(os, *i);
		} else {
			operator<<(os, *(i->get_model()));
		}
		os << " -> " << reverse_mapping[*i];
		os << ' ';
	}
#endif

	nvmCoprocessorState *cop(state);
	assert(cop != 0 && "Coprocessor not found!");

	for(uint32_t i(0); i < cop->n_regs; ++i)
		if(cop->reg_flags[i] & COPREG_WRITE) {
			cout << i << " -> " << getCoproReg(i, true) << ' ';
		}

	os << ")";

	return os;
}

} /* namespace jit */
