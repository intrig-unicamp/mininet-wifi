#include "pfl_trace_builder.h"
#include "tree.h"


PFLTraceBuilder::PFLTraceBuilder(PFLCFG & cfg): jit::TraceBuilder<PFLCFG>(cfg)
{
	//nop
};

void PFLTraceBuilder::handle_no_succ_bb(PFLTraceBuilder::bb_t *bb)
{
	assert(bb != NULL);
	return;
}

void PFLTraceBuilder::handle_one_succ_bb(PFLTraceBuilder::bb_t *bb)
{

	assert(bb != NULL);

	//	std::cout << "TRACE: Esamino il BB: " << bb->getId() << std::endl;

	bb_t* next = bb->getProperty< bb_t* >(next_prop_name);
	std::list< PFLCFG::GraphNode* > successors(bb->getSuccessors());

	bb_t *target = successors.front()->NodeInfo;



	if(bb->getCode().size() == 0 || ! bb->getCode().back()->isJump())
	{
		//empty BB should be eliminated from the cfg!!!
		//for now emit a jump to the target if necessary

		if(cfg.getExitBB() == target)
			return;

		if(!next || next->getId() != target->getId())
		{
			//			std::cerr << "BB ID: " << bb->getId() << "Code Size = 0" << std::endl;

			//assert(1 == 0 && "This should not happen");
			JumpMIRONode *new_j = new JumpMIRONode(bb->getId(), cfg.getBBById(target->getId())->getStartLabel());
			//bb->getCode().pop_back();
			new_j->setOpcode(JUMPW);
			bb->getCode().push_back(new_j);

		}
	}
	else
	{
		MIRONode* insn = bb->getCode().back();

		//std::cout << "Ultima istruzione: ";
		//insn->printNode(std::cout, true);
		//std::cout << std::endl;

		uint16_t opcode;
		if(insn->getKid(0))
			opcode = insn->getKid(0)->getOpcode();
		else
			opcode = insn->getOpcode();

		//if not followed by next bb in emission
		if(!next || next->getId() != target->getId())
		{
			//if has not explicit jump
			if(opcode != JUMP && opcode != JUMPW)
			{
				//add the jump
				JumpMIRONode *new_j = new JumpMIRONode(bb->getId(), cfg.getBBById(target->getId())->getStartLabel());
				//bb->getCode().pop_back();
				new_j->setOpcode(JUMPW);
				bb->getCode().push_back(new_j);
			}
		}
		else
		{
			//we can remove last useless jump
			if(opcode == JUMP || opcode == JUMPW)
			{
				bb->getCode().pop_back();
				delete insn;
			}
		}
	}
	return;
}

void PFLTraceBuilder::handle_two_succ_bb(PFLTraceBuilder::bb_t *bb)
{
	assert(bb != NULL);

	//std::cout << "TRACE: Esamino il BB: " << bb->getId() << std::endl;
	bb_t* next = bb->getProperty< bb_t* >(jit::TraceBuilder<PFLCFG>::next_prop_name);
	std::list< PFLCFG::GraphNode* > successors(bb->getSuccessors());

	MIRONode* insn = bb->getCode().back();


	//std::cout << "Ultima istruzione: ";
	//insn->printNode(std::cout, true);
	//std::cout << std::endl;

	if( ((StmtMIRONode*)insn)->Kind != STMT_JUMP && ((StmtMIRONode*)insn)->Kind != STMT_JUMP_FIELD)
		return;

	//uint16_t jt = LBL_OPERAND(insn->Operands[0].Op)->Label;
	uint16_t jt = ((JumpMIRONode*)insn)->getTrueTarget();
	successors.remove(this->cfg.getBBById(jt)->getNode());
	uint16_t jf = successors.front()->NodeInfo->getId();


	if(!next || (next->getId() != jt && next->getId() != jf))
	{
		//x86_Asm_JMP_Label(bb->getCode(), jf);
		JumpMIRONode *new_j = new JumpMIRONode(bb->getId(), cfg.getBBById(jf)->getStartLabel());
		//bb->getCode().pop_back();
		new_j->setOpcode(JUMPW);
		bb->getCode().push_back(new_j);
		return;
	}

	MIRONode *jump_node = insn->getKid(0);
	if (jump_node->getOpcode() == JUMPW)
		return;

	if (jt == next->getId())
	{
		//std::cout << "Vecchio nodo: " << std::endl;
		//insn->printNode(std::cout,false);
		//std::cout << std::endl;

		((JumpMIRONode*)insn)->swapTargets();
		//FIXME
		//assert(1 == 0 && "Not implemented yet");
		switch(jump_node->getOpcode())
		{
		case JCMPEQ:
			jump_node->setOpcode(JCMPNEQ);
			break;
		case JCMPNEQ:
			jump_node->setOpcode(JCMPEQ);
			break;
		case JCMPG:
			jump_node->setOpcode(JCMPLE);
			break;
		case JCMPGE:
			jump_node->setOpcode(JCMPL);
			break;
		case JCMPL:
			jump_node->setOpcode(JCMPGE);
			break;
		case JCMPLE:
			jump_node->setOpcode(JCMPG);
			break;
		case JCMPG_S:
			jump_node->setOpcode(JCMPLE_S);
			break;
		case JCMPGE_S:
			jump_node->setOpcode(JCMPL_S);
			break;
		case JCMPL_S:
			jump_node->setOpcode(JCMPGE_S);
			break;
		case JCMPLE_S:
			jump_node->setOpcode(JCMPG_S);
			break;
		case JFLDEQ:
			jump_node->setOpcode(JFLDNEQ);
			break;
		case JFLDNEQ:
			jump_node->setOpcode(JFLDEQ);
			break;
		case JFLDGT:
			jump_node->setOpcode(JFLDLT);
			break;
		case JFLDLT:
			jump_node->setOpcode(JFLDGT);
			break;
		default:
			break;
		}
		//std::cout << "Nuovo nodo: " << std::endl;
		//insn->printNode(std::cout,false);
		//std::cout << std::endl;
	}

	//else bb is followed by is false target so it's correct
	return;
}
