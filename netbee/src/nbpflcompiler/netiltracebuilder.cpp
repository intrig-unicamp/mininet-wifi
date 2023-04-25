/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "netiltracebuilder.h"
#include "errors.h"
#include "dump.h"
#include "tree.h"

void NetILTraceBuilder::InvertBranch(JumpMIRONode *brStmt)
{
	nbASSERT(brStmt->FalseBranch != NULL, "jump should be a conditional branch");

	switch(brStmt->getKid(0)->getOpcode())
	{
		case JCMPEQ:
			brStmt->getKid(0)->setOpcode(JCMPNEQ);
			break;
		case JCMPNEQ:
			brStmt->getKid(0)->setOpcode(JCMPEQ);
			break;
		case JCMPG:
			brStmt->getKid(0)->setOpcode(JCMPLE);
			break;
		case JCMPGE:
			brStmt->getKid(0)->setOpcode(JCMPL);
			break;
		case JCMPL:
			brStmt->getKid(0)->setOpcode(JCMPGE);
			break;
		case JCMPLE:
			brStmt->getKid(0)->setOpcode(JCMPG);
			break;
		case JCMPG_S:
			brStmt->getKid(0)->setOpcode(JCMPLE_S);
			break;
		case JCMPGE_S:		
			brStmt->getKid(0)->setOpcode(JCMPL_S);
			break;
		case JCMPL_S:		
			brStmt->getKid(0)->setOpcode(JCMPGE_S);
			break;
		case JCMPLE_S:		
			brStmt->getKid(0)->setOpcode(JCMPG_S);
			break;
		case JFLDEQ:
			brStmt->getKid(0)->setOpcode(JFLDNEQ);
			break;
		case JFLDNEQ:
			brStmt->getKid(0)->setOpcode(JFLDEQ);
			break;
		case JFLDGT:
			brStmt->getKid(0)->setOpcode(JFLDLT);
			break;
		case JFLDLT:
			brStmt->getKid(0)->setOpcode(JFLDGT);
			break;
		default:
			//nbASSERT(false, "Invert Branch: CANNOT BE HERE");
			std::cout << "Jump senza opcode!!!!!!!" << std::endl;
			break;
	}

	//swap the target labels
	SymbolLabel *tmp = brStmt->FalseBranch;
	brStmt->FalseBranch = brStmt->TrueBranch;
	brStmt->TrueBranch = tmp;
}


void NetILTraceBuilder::VisitBasicBlock(BBType *bb, BBType *comingFrom)
{
	if (m_ValidationPass)
	{
		bb->Valid = true;
		return;
	}

	if (!bb->Valid)
		return;

	if (bb->Kind != BB_CFG_ENTRY)
	{
		//CodeList *bbCode = &bb->Code;
		std::list<MIRONode*> bbCode = bb->getCode();
		nbASSERT(bbCode.size() > 0, "basic block code should not be empty");
		StmtMIRONode *last = dynamic_cast<StmtMIRONode*>(*bbCode.rbegin());
		std::cout << "BB: " << bb->getId() << "TRACE:  Esamino il nodo: ";
		last->printNode(std::cout, false);
		std::cout << std::endl;

		if (last->Kind == STMT_JUMP || last->Kind == STMT_JUMP_FIELD)
		{
			JumpMIRONode *jump = dynamic_cast<JumpMIRONode*>(last);
			//if it's a conditional branch:
			if (jump->FalseBranch)
			{
				BBType *trueBB = m_CFG->LookUpLabel(jump->TrueBranch->Name);
				nbASSERT(trueBB != NULL, "basic block should exist");
				if (comingFrom == trueBB)
					InvertBranch(jump);
			}
		}
		CodeWriter cw(m_Stream);
		cw.DumpNetIL(bb->getMIRNodeCode());
	}
}

void NetILTraceBuilder::CreateTrace(PFLCFG &cfg)
{
	m_CFG = &cfg;
	cfg.ResetValid();
	m_ValidationPass = true;
	CFGVisitor cfgVisitor(*this);
	cfgVisitor.VisitPreOrder(cfg);
	m_ValidationPass = false;
	cfgVisitor.VisitRevPostOrder(cfg);
}
