/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "cfgwriter.h"
#include "errors.h"
#include "dump.h"



void CFGWriter::VisitBBEdge(PFLBasicBlock *from, PFLBasicBlock *to)
{
	if (DumpNodesPass)
		return;

	m_Stream << from->StartLabel->Name << "->" << to->StartLabel->Name;
	if (from->Kind != BB_CFG_ENTRY)
	{
		//CodeList *bbCode = &from->Code;
		std::list<MIRONode*> *bbCode = from->getMIRNodeCode();
		nbASSERT(bbCode->size() > 0, "basic block code should not be empty");
		StmtMIRONode *last = dynamic_cast<StmtMIRONode*>(*bbCode->rbegin());
		assert(last != NULL && "Dynamic cast failed");
		if (last->Kind == STMT_JUMP)
		{
			JumpMIRONode *jump = dynamic_cast<JumpMIRONode*>(last);
			//if it's a conditional branch:
			if (jump->FalseBranch)
			{
				string col, lab;
				PFLBasicBlock *falseBB = m_CFG->LookUpLabel(jump->FalseBranch->Name);
				nbASSERT(falseBB != NULL, "basic block should exist");
				PFLBasicBlock *trueBB = m_CFG->LookUpLabel(jump->TrueBranch->Name);
				nbASSERT(trueBB != NULL, "basic block should exist");
				if (to == falseBB)
				{
					col = "red";
					lab = "F";
				}
				else if (to == trueBB)
				{
					col = "green";
					lab = "T";
				}
				else
				{
					col = "black";
					lab = "NK";
				}
				m_Stream << "[color=" << col << ", label=" << lab << "]";
			}
		}
	}
	
	m_Stream << ";" << endl;
}

void CFGWriter::VisitBasicBlock(PFLBasicBlock *bb, PFLBasicBlock *comingFrom)
{
	if (!DumpNodesPass)
		return;

	if (bb->Kind != BB_CFG_ENTRY && bb->Kind != BB_CFG_EXIT)
	{
		//nbASSERT(bb->getMIRNodeCode()->size() > 0, "Empty Basic Block!");
	}
	//if (bb->Valid)
	{
		m_Stream << bb->StartLabel->Name;
		if (bb->getMIRNodeCode()->size() == 0 || GraphOnly)
		{
			m_Stream << "[fontname=\"Courier\",shape=\"ellipse\",label=\"" << bb->StartLabel->Name << "\"];" << endl;
		}
		else
		{
			m_Stream << "[fontname=\"Courier\",shape=\"rect\",label=\"BB ID:" << bb->getId() << "\\l";
			CodeWriter cw(m_Stream, "\\l", &num_Statement);
			if (DumpNetIL)
				cw.DumpNetIL(bb->getMIRNodeCode());
			else
				cw.DumpCode(bb->getMIRNodeCode());
			m_Stream << "\"];" << endl;
		}
	}

	return;
}

void CFGWriter::DumpCFG(PFLCFG &cfg, bool graphOnly, bool netIL)
{
	m_CFG = &cfg;
	GraphOnly = graphOnly;
	DumpNetIL = netIL;
	CFGVisitor cfgVisitor(*this);
	num_Statement = 0;

	m_Stream << "Digraph CFG {" << endl << endl;

	m_Stream << "// EDGES" << endl;
	DumpNodesPass = false;
	cfgVisitor.VisitPreOrder(cfg);
	DumpNodesPass = true;
	m_Stream << "// NODES" << endl;
	cfgVisitor.VisitPreOrder(cfg);
	m_Stream << "}" << endl;
	m_CFG = NULL;
#ifdef _ENABLE_PFLC_PROFILING
	std::cout << "Numero di statements: " << num_Statement << std::endl;
#endif
}

