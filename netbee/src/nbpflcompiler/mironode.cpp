#include "mironode.h"
#include "pflcfg.h"
#include "dump.h"
#include "tree.h"



PFLCFG* MIRONode::_cfg(NULL);

MIRONode::~MIRONode()
{
	if(kids[0])
	{
		delete kids[0];
		kids[0] = NULL;
	}
	if(kids[1])
	{
		delete kids[1];
		kids[1] = NULL;
	}
	if(kids[2])
	{
		delete kids[2];
		kids[2] = NULL;
	}
	//if(Sym)
	//{
	//	//delete Sym;
	//	Sym = NULL;
	//}
}

MIRONode *MIRONode::getKid(uint8_t pos) const
{
	if(pos <=2)
		return kids[pos];
	return NULL;
};

void MIRONode::setKid(MIRONode *node, uint8_t pos)
{
	if(pos <=2)
		kids[pos] = node;
}

void MIRONode::deleteKid(uint8_t pos)
{
	assert(pos <= 2);
	delete kids[pos];
}

MIRONode *MIRONode::unlinkKid(uint8_t pos)
{
	assert(pos <= 2);
	MIRONode *kid = kids[pos];
	kid->removeRef();
	kids[pos] = NULL;
	return kid;
}

void MIRONode::swapKids()
{
	MIRONode *tmp;
	tmp = kids[0];
	kids[0] = kids[1];
	kids[1] = tmp;
};

bool MIRONode::IsBoolean(void)
{
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	if (OpCode > mir_first_op)
		return false;
	return (GET_OP_RTYPE(OpCode) == IR_TYPE_BOOL);
}

bool MIRONode::IsInteger(void)
{
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	if (OpCode > mir_first_op)
		return true;
	return (GET_OP_RTYPE(OpCode) == IR_TYPE_INT);
}

bool MIRONode::IsString(void)
{
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	if (OpCode > mir_first_op)
		return false;
	return (GET_OP_RTYPE(OpCode) == IR_TYPE_STR);
}


bool MIRONode::IsTerOp(void)
{
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	if (OpCode > mir_first_op)
		return (kids[0] != NULL && kids[1] != NULL && kids[2] != NULL);
	return (GET_OP_FLAGS(OpCode) == IR_TEROP);
}


bool MIRONode::IsBinOp(void)
{
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	if (OpCode > mir_first_op)
		return (kids[0] != NULL && kids[1] != NULL && kids[2] == NULL);
	return (GET_OP_FLAGS(OpCode) == IR_BINOP);
}


bool MIRONode::IsUnOp(void)
{
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	if (OpCode > mir_first_op)
		return (kids[0] != NULL && kids[1] == NULL && kids[2] == NULL);
	return (GET_OP_FLAGS(OpCode) == IR_UNOP);
}


bool MIRONode::IsTerm(void)
{	
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");	
	
	if (OpCode > mir_first_op)
		return (kids[0] == NULL && kids[1] == NULL && kids[2] == NULL);
	return (GET_OP_FLAGS(OpCode) == IR_TERM);
}

bool MIRONode::isCopy() const
{
	//assert(1 == 0 && "Not yet implemented");
	return getOpcode() == (LOCST) && getKid(0) && getKid(0)->getOpcode() == (LOCLD);
}

bool MIRONode::isJump() const
{
	switch(getOpcode())
	{
		case (JCMPEQ):
		case (JCMPNEQ):
		case (JCMPG):
		case (JCMPGE):
		case (JCMPL):
		case (JCMPLE):
		case (JCMPG_S):
		case (JCMPGE_S):
		case (JCMPL_S):
		case (JCMPLE_S):
		case (JFLDEQ):
		case (JFLDNEQ):
		case (JFLDLT):
		case (JFLDGT):
		case (CMP_S):
		case (JEQ):
		case (JNE):
		case (JUMP):
		case (JUMPW):
		case (SWITCH):
		case (CALL):
		case (CALLW):
		case (RET):
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

void MIRONode::rewrite_destination(uint16 old_bb_id, uint16 new_bb)
{
	assert(1 == 0 && "Rewrite destination called on a non jump node");
}

void MIRONode::rewrite_use(RegType old, RegType new_reg)
{
	//NOP
	//assert(1 == 0 && "Not yet implemented");
	if(getOpcode() == LOCLD)
	{
		assert(old == ((SymbolTemp*)Sym)->LocalReg && "Tried to rewrite the wrong register in a LDREG node");
		((SymbolTemp*)Sym)->LocalReg = new_reg;
	}
	return;
}

void MIRONode::setSymbol(Symbol *sym)
{
	if(sym)
		Sym = sym->copy();
	else
		Sym = NULL;
}

MIRONode::RegType MIRONode::getDefReg()
{
	if(Sym && Sym->SymKind == SYM_TEMP)
		return ((SymbolTemp*)Sym)->LocalReg; 
	else
		return RegType::invalid;
}

void MIRONode::setDefReg(RegType reg)
{ 
	if(Sym && Sym->SymKind != SYM_TEMP)
		return;

	if(Sym == NULL)
	{
		std::string name("");
		Sym = new SymbolTemp(0, name);
	}
	((SymbolTemp*)Sym)->LocalReg = reg; 
};

MIRONode::RegType * MIRONode::getOwnReg()
{ 
	if(!Sym)
	{
		return NULL;
		// "Nodo senza simbolo!" << std::endl;
		printNode(std::cout, true);
		assert(1 == 0 && "Symbol annot be NULL");
	}

	if(Sym && Sym->SymKind == SYM_TEMP)
	{
		if( ((SymbolTemp*)Sym)->LocalReg != RegType::invalid )
			return &((SymbolTemp*)Sym)->LocalReg;
		return NULL;
	}
	
	return NULL;
}


std::set<MIRONode::RegType> MIRONode::getDefs()
{
	std::set<RegType> res;
	//if(OpCode == LOCST)
	if(OpCode == LOCLD)
		return res;
	if(Sym && Sym->SymKind == SYM_TEMP)
		if( ((SymbolTemp*)Sym)->LocalReg != RegType::invalid)
			res.insert( ((SymbolTemp*)Sym)->LocalReg);
	return res;
}

std::set<MIRONode::RegType> MIRONode::getAllDefs()
{
	std::set<RegType> res(getDefs());

	for(int i(0); i < 3; i++)
	{
		if(getKid(i)){
			std::set<RegType> t(kids[i]->getAllDefs()), p(res);
			res.clear();
			set_union(p.begin(), p.end(), t.begin(), t.end(),
					std::insert_iterator<std::set<RegType> >(res, res.begin()));
		}
	}
	return res;
}

std::set<MIRONode::RegType> MIRONode::getUses()
{
	std::set<RegType> res;
	if(OpCode == LOCLD)
		res.insert( ((SymbolTemp*)Sym)->LocalReg);
	else
	{
		for(int i = 0; i < 3; i++)
		{
			if(getKid(i) && getKid(i)->getOwnReg())
				res.insert(*getKid(i)->getOwnReg());
		}
	}
	return res;
}

std::set<MIRONode::RegType> MIRONode::getAllUses()
{
	std::set<RegType> res(getUses());

	for(int i=2; i >= 0; --i)
	{
		if(kids[i])
		{
			std::set<RegType> use = kids[i]->getAllUses();
			set_union(res.begin(), res.end(), use.begin(), use.end(),
					std::insert_iterator<std::set<RegType> >(res, res.begin()));
		}
	}
	return res;
}

std::list<std::pair<MIRONode::RegType,MIRONode::RegType> > MIRONode::getCopiedPair()
{
	//std::cout << "CopiedPair: considero lo statement "; 
	//printNode(std::cout, false);
	//std::cout << std::endl;
	std::list<std::pair<RegType,RegType> > res;

	
	MIRONode *tree = getTree();

	if(!tree)
		return res;
	if(tree->isStore() && tree->getKid(0) && tree->getKid(0)->isLoad())
	{
		RegType dst = tree->getDefReg();
		RegType src = tree->getKid(0)->getDefReg();
		res.push_back(std::pair<RegType,RegType>(dst, src));
	}
	return res;
}

/*

bool BaseNode::IsStmt(void)
{
	return (GET_OP_FLAGS(Op) == IR_STMT);
}
*/
MIRONode::ConstType * MIRONode::make_const(uint16_t bbId, MIRONode::ValueType const_value, MIRONode::RegType def_reg)
{
	MIRONode *const_node = new MIRONode(PUSH, const_value); 
	const_node->setBBId(bbId);
	return const_node;
}

MIRONode::StoreType * MIRONode::make_store(uint16_t bbID, MIRONode *kid, MIRONode::RegType defReg)
{
	MIRONode *new_store = new MIRONode(LOCST, kid, new SymbolTemp(defReg));
	new_store->setBBId(bbID);
	StmtMIRONode *new_stmt = new StmtMIRONode(STMT_GEN, new_store);
	new_stmt->setBBId(bbID);
	return new_stmt;
}

MIRONode::LoadType * MIRONode::make_load(uint16_t bbID, MIRONode::RegType defReg)
{
	MIRONode *new_load = new MIRONode(LOCLD, new SymbolTemp(defReg));
	return new_load;
}

std::set<MIRONode::RegType> StmtMIRONode::getDefs()
{
	std::set<RegType> res;
	//if(getKid(0))
	//	return getKid(0)->getDefs();
	return res;
}

std::set<MIRONode::RegType> StmtMIRONode::getAllUses()
{
	std::set<RegType> res;

	if(getKid(0))
	{
		std::set<RegType> use = getKid(0)->getAllUses();
		set_union(res.begin(), res.end(), use.begin(), use.end(),
				std::insert_iterator<std::set<RegType> >(res, res.begin()));
	}
	return res;
}

MIRONode::RegType * StmtMIRONode::getOwnReg()
{ 
	//assert(1 == 0 && "Not implemented"); 
	if(getKid(0))
		return getKid(0)->getOwnReg();
	
	return NULL;
}

bool MIRONode::isConst(void)
{
	//nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	if (OpCode > mir_first_op)
		return (OpCode ==PUSH);
	return (GET_OP(OpCode) == OP_CONST);
}

bool MIRONode::isStore()
{
	return (getOpcode() == LOCST);
}

bool MIRONode::isLoad()
{
	return (getOpcode() == LOCLD);
}

MIRONode::RegType MIRONode::getUsed()
{
	if(isLoad())
		return *getOwnReg();
	else
		return RegType::invalid;
}

void MIRONode::setUsed(MIRONode::RegType reg)
{
	if(isLoad())
		setDefReg(reg);
	return;
}

/*Interface for reassociation */

bool MIRONode::isOpMemPacket()
{
	switch(getOpcode())
	{
		case (PBLDS):
		case (PBLDU):
		case (PSLDS):
		case (PSLDU):
		case (PILD):
		case (PBSTR):
		case (PSSTR):
		case (PISTR):
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

bool MIRONode::isOpMemData()
{
	switch(getOpcode())
	{
		case (DBLDS):
		case (DBLDU):
		case (DSLDS):
		case (DSLDU):
		case (DILD):
		case (DBSTR):
		case (DSSTR):
		case (DISTR):
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

bool MIRONode::isMemLoad()
{
	switch(getOpcode())
	{
		case (PBLDS):
		case (PBLDU):
		case (PSLDS):
		case (PSLDU):
		case (PILD):
		case (DBLDS):
		case (DBLDU):
		case (DSLDS):
		case (DSLDU):
		case (DILD):
		case (SBLDS):
		case (SBLDU):
		case (SSLDS):
		case (SSLDU):
		case (SILD):
		case (ISBLD): 
		case (ISSLD): 
		case (ISSBLD):
		case (ISSSLD):
		case (ISSILD):
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

bool MIRONode::isMemStore()
{
	switch(getOpcode())
	{
		case (DBSTR):
		case (DSSTR):
		case (DISTR):
		case (PBSTR):
		case (PSSTR):
		case (PISTR):
		case (SBSTR):
		case (SSSTR):
		case (SISTR):
		case (IBSTR):
		case (ISSTR):
		case (IISTR):
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

uint8_t MIRONode::num_kids = 3;
/*END Interface for reassociation */

MIRONode::ValueType MIRONode::getValue()
{
	return Value;
}

uint32 MIRONode::GetConstVal(void)
{
	nbASSERT(OpCode > mir_first_op,"MIRNode needs a MIROpcode");

	nbASSERT(isConst(), "node must be constant");
	if (OpCode < HIR_LAST_OP)
		return ((SymbolIntConst *)Sym)->Value;
	
	return Value;
}


MIRONode *MIRONode::copy()
{
	MIRONode *newnode = new MIRONode(getOpcode(), Sym);
	for(int i = 0; i < 3; i++)
		if(kids[i])
			newnode->setKid(kids[i]->copy(), i);
	//newnode->setDefReg(defReg);
	newnode->setValue(Value);
	return newnode;
}

MIRONode::PhiType *MIRONode::make_phi(uint16_t bb, RegType reg)
{
	//assert(1 == 0 && "Not yet implemented");
	PhiMIRONode *newnode = new PhiMIRONode(bb, reg);
	return newnode;
}

MIRONode::CopyType *MIRONode::make_copy(uint16_t bbid, RegType src, RegType dst)
{
	//assert(1 == 0 && "Not yet implemented");
	MIRONode *srcnode = new MIRONode(LOCLD, new SymbolTemp(src));
	srcnode->setBBId(bbid);
	MIRONode *dstnode = new MIRONode(LOCST, srcnode, new SymbolTemp(dst));

	GenMIRONode *stmtnode = new GenMIRONode(dstnode);

	return stmtnode;
}

MIRONode::ContType MIRONode::get_reverse_instruction_list()
{
	ordered_list.clear();
	sortPostOrder();
	ContType t(ordered_list);
	t.reverse();
	return t;
}

MIRONode *StmtMIRONode::copy() 
{
	return new StmtMIRONode(Kind, getKid(0)->copy() );
}

MIRONode *LabelMIRONode::copy() 
{
	return new LabelMIRONode(); 
}

MIRONode *CommentMIRONode::copy() 
{ 
	return new CommentMIRONode(Comment);
}


MIRONode *GenMIRONode::copy() { 
	GenMIRONode *newnode = new GenMIRONode();
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode;
}

JumpMIRONode::JumpMIRONode(uint32_t opcode, uint16_t bb, 
			uint32_t jt, 
			uint32_t jf,
			RegType defReg,
			MIRONode * param1,
			MIRONode* param2)
				:StmtMIRONode(STMT_JUMP, NULL)
{
	setBBId(bb);
	if(jf == 0)
	{
	TrueBranch = getCFG()->getBBById(jt)->getStartLabel();
	setOpcode(opcode);
	FalseBranch = NULL;
	}
	else
	{
		TrueBranch = getCFG()->getBBById(jt)->getStartLabel();
		nbASSERT(getCFG()->getBBById(jf)!=NULL,"-_-");
		FalseBranch = getCFG()->getBBById(jf)->getStartLabel();
		MIRONode *kid = new MIRONode(opcode, param1->copy(), param2->copy());
		this->setKid(kid, 0);
	}
}

MIRONode *JumpMIRONode::copy() {
	JumpMIRONode *newnode = new JumpMIRONode(belongsTo(), TrueBranch, FalseBranch);
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode; 
}

void JumpMIRONode::rewrite_destination(uint16 old_bb_id, uint16 new_bb_id)
{
	//assert(1 == 0 && "Not yet implemented!!!!!!");
	assert(_cfg != NULL && "Cfg not setted");
	SymbolLabel *oldl = _cfg->getBBById(old_bb_id)->getStartLabel();	
	SymbolLabel *newl = _cfg->getBBById(new_bb_id)->getStartLabel();
	assert( (TrueBranch == oldl) || ((FalseBranch == oldl) && "No matching label"));

	if(TrueBranch == oldl)
		TrueBranch = newl;
	if(FalseBranch == oldl)
		FalseBranch = newl;
		
}

uint16_t JumpMIRONode::getTrueTarget()
{
	return getCFG()->LookUpLabel(TrueBranch->Name)->getId();
}

uint16_t JumpMIRONode::getFalseTarget()
{
	if(FalseBranch)
		return getCFG()->LookUpLabel(FalseBranch->Name)->getId();
	else
		return 0;
}

uint16_t JumpMIRONode::getOpcode() const
{
	if(getKid(0))
		return getKid(0)->getOpcode();
	else
		return this->OpCode;
}

SwitchMIRONode::targets_iterator SwitchMIRONode::TargetsBegin()
{
	typedef std::list<MIRONode*>::iterator l_it;
	_target_list.clear();
	for(l_it i = Cases.begin(); i != Cases.end(); i++)
	{
		uint32_t case_value = (*i)->getKid(0)->getValue();
		CaseMIRONode *case_node = dynamic_cast<CaseMIRONode*>(*i);
		uint32_t target = getCFG()->LookUpLabel(case_node->getTarget()->Name)->getId();
		_target_list.push_back(std::make_pair<uint32_t, uint32_t>(case_value, target));
	}
	return _target_list.begin();
}

SwitchMIRONode::targets_iterator SwitchMIRONode::TargetsEnd()
{
	return _target_list.end();
}


void SwitchMIRONode::removeCase(uint32_t case_n)
{
	typedef std::list<MIRONode*>::iterator l_it;

	for(l_it i = Cases.begin(); i != Cases.end(); )
	{
		uint32_t case_value = (*i)->getKid(0)->getValue();
		if(case_value == case_n)
		{
			//this case has to be removed
			i = Cases.erase(i);
			NumCases--;
		}
		else
			i++;
	}
}

uint32_t SwitchMIRONode::getDefaultTarget()
{
	assert(Default != NULL);
	SymbolLabel *target = Default->getTarget();
	assert(target != NULL);
	PFLCFG *_cfg = getCFG();
	assert(_cfg != NULL);
	PFLBasicBlock *bb = _cfg->LookUpLabel(target->Name);
	assert(bb != NULL);
	
	return bb->getId();
}

SwitchMIRONode::~SwitchMIRONode()
{
	delete SwExit;
	delete Default;
	typedef std::list<MIRONode*>::iterator l_it;

	for(l_it i = Cases.begin(); i != Cases.end(); i++)
	{
		MIRONode * case_node = *i;
		delete case_node;
	}
}

void JumpMIRONode::swapTargets()
{
	SymbolLabel *tmp = TrueBranch;
	TrueBranch = FalseBranch;
	FalseBranch = tmp;
}

MIRONode *JumpFieldMIRONode::copy()
{
	JumpMIRONode *newnode = new JumpMIRONode(belongsTo(), TrueBranch, FalseBranch);
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode;
}

MIRONode *CaseMIRONode::copy()
{
	CaseMIRONode *newnode = new CaseMIRONode(0, this->Target);
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode;
}

MIRONode *SwitchMIRONode::copy() {
	std::cerr << "Copy Switch not implemetned"; 
	assert(1==0); 
	return NULL;
}

void SwitchMIRONode::rewrite_destination(uint16 oldbb, uint16 newbb)
{
	//assert(1 == 0 && "Function not yet implemented. Sorry");
	assert(_cfg != NULL && "CFG not setted");
	typedef std::list<MIRONode*>::iterator it_t;
	
	//std::cout << "Rewriting Switch targets: (BBID: " << this->belongsTo() << std::endl;
	
	PFLBasicBlock * old_bb = _cfg->getBBById(oldbb);
	assert(old_bb != NULL && "BB cannot be NULL");
	SymbolLabel *oldl = old_bb->getStartLabel();	
	PFLBasicBlock * new_bb = _cfg->getBBById(newbb);
	assert(new_bb != NULL && "BB cannot be NULL");
	SymbolLabel *newl = new_bb->getStartLabel();
//	bool found = false;
	if(Default->getTarget() == oldl)
	{
//		found = true;
		Default->setTarget(newl);
		//std::cout << "===> Changed default " << oldl->Name << " - " << oldbb << " into " << newl->Name << " - " << newbb <<  std::endl;
	}
	for(it_t i = Cases.begin(); i != Cases.end(); i++)
	{
		if( ((CaseMIRONode*)*i)->getTarget() == oldl)
		{
//			found = true;
			((CaseMIRONode*)*i)->setTarget(newl); 
			//std::cout << "===> Changed case " << oldl->Name << " - " << oldbb << " into " << newl->Name << " - " << newbb <<  std::endl;
		}
	}
	/*
	if (!found)
		std::cout << "===> No change" << std::endl;
	*/
	//assert(found == true && "No match");
}

//---------------------------------------------------
//PRINTER METODS
//--------------------------------------------------

std::ostream& MIRONode::printNode(std::ostream &os, bool SSAForm)
{
	CodeWriter::DumpOpCode_s(this->getOpcode(), os);
	if(Sym && Sym->SymKind == SYM_TEMP && getOwnReg() != NULL)
		os << "[" << *getOwnReg() << "]"; 
		//os << "[" << ((SymbolTemp*)Sym)->LocalReg << "]";
	//else
	//	os << "[" << defReg << "]";
		
	os << "(";
	if (this->isConst())
	{
		os << this->Value;
	}
	else
	{	
		for(unsigned int i = 0; i < 3; i++)
		{
			if(kids[i])
			{
				if(i)
					os << ", ";
				kids[i]->printNode(os, SSAForm);
			}
		}
	}
	os << " ) ";
	return os;
};


std::ostream& StmtMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "StmtMIRONode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& BlockMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "BlockMIRONode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& LabelMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "LabelMIRONode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& CommentMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "CommentMIRONode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& GenMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "GenMIRONode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& JumpMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	os << "JumpMIRONode";
	if(getKid(0))
	{
		os << "(";
		getKid(0)->printNode(os, SSAForm) << ")";
	}
	os << "TRUE: " << TrueBranch->Name;
	if(FalseBranch)
		os << ", FALSE: " << FalseBranch->Name;
	os << std::endl;
	
	return os;
}

std::ostream& JumpFieldMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "JumpFieldMIRONode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& CaseMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "CaseMIRONode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	SymbolLabel *target = this->Target;
	assert(target != NULL);
	PFLBasicBlock *bb = _cfg->LookUpLabel(target->Name);
	assert(bb != NULL);
	std::cout << " -> " << target->Name << " BBID: " << bb->getId();	
	return os;
}

std::ostream& SwitchMIRONode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "SwitchMIRONode [" << NumCases << "](";
	getKid(0)->printNode(std::cout, true);
	std::cout << std::endl;
	for(std::list<MIRONode*>::iterator i = Cases.begin(); i != Cases.end(); i++)
	{
		std::cout << "	";
		(*i)->printNode(std::cout, SSAForm);
		std::cout << std::endl;
	}
	std::cout << "Default: ";
	Default->printNode(std::cout, SSAForm);
	std::cout << std::endl;
	return os;
}

std::ostream& PhiMIRONode::printNode(std::ostream& os, bool ssa_form)
{
	os << "Phi node" << "[" << getDefReg() << "] (";
	PhiMIRONode::params_iterator i = paramsBegin();
	while(i != paramsEnd())
	{
		os << i->second << ", ";
		i++;
	}
	os << ")";
	return os;
}

PhiMIRONode::PhiMIRONode(uint32_t BBId, RegType defReg)
	: StmtMIRONode(STMT_PHI) {
		BBid = BBId;
		setDefReg(defReg);
	};

PhiMIRONode::PhiMIRONode(const PhiMIRONode& n)
	: StmtMIRONode(STMT_PHI) {
		BBid = n.BBid;		
	};


std::set<MIRONode::RegType> PhiMIRONode::getUses()
{
	std::set<RegType> reg_set;
	//assert(1 == 0 && "Not yet implemented");
	for(params_iterator i = params.begin(); i != params.end(); i++)
		reg_set.insert((*i).second);
	return reg_set;
}

std::set<MIRONode::RegType> PhiMIRONode::getDefs()
{
	std::set<RegType> reg_set;
	reg_set.insert(getDefReg());
	return reg_set;
}

std::set<MIRONode::RegType> PhiMIRONode::getAllUses()
{
	std::set<RegType> reg_set(getUses());
	return reg_set;
}

bool GenMIRONode::isStore()
{
	return getKid(0)->isStore();
}

bool GenMIRONode::isCopy() const
{
	return getKid(0)->isCopy();
}

MIRONode::ValueType GenMIRONode::getValue()
{
	return getKid(0)->getValue();
}

std::ostream& operator<<(std::ostream& os, MIRONode& node)
{
	node.printNode(os, true);
	return os;
}

std::set<MIRONode::RegType> SwitchMIRONode::getDefs()
{
	std::set<RegType> res;
	return res;
}
