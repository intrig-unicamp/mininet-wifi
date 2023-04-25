/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "pflcfg.h"
#include "errors.h"
#include "tree.h"
#include "cfgwriter.h"
#include "node_translation.h"

#include "dump.h"

#include <iostream>
using namespace std;


PFLBasicBlock::~PFLBasicBlock()
{
	typedef list<MIRONode*>::iterator it_t;
	for(it_t i = MIRONodeCode->begin(); i != MIRONodeCode->end(); i++)
	{
		StmtMIRONode *stmt = dynamic_cast<StmtMIRONode*>(*i);
		delete stmt;
	}

	delete MIRONodeCode;
	//if(StartLabel)
	//	delete StartLabel;
}

void PFLBasicBlock::addHeadInstruction(MIRONode *node)
{
	assert(node != NULL && "Adding a NULL node to a BasicBlock");
	//std::cout << "Aggiungo un nodo al BB: " << getId() << std::endl;
	MIRONodeCode->push_front(node);
}

void PFLBasicBlock::addTailInstruction(MIRONode *node)
{
	assert(node != NULL && "Adding a NULL node to a BasicBlock");
	MIRONodeCode->push_back(node);
}

PFLCFG::PFLCFG()
	: jit::CFG<MIRONode, PFLBasicBlock>(std::string("pflCFG"), true), EntryLbl(LBL_CODE, 0, "ENTRY"), ExitLbl(LBL_CODE, 0, "EXIT"), m_BBCount(0), m_CurrMacroBlock(0)
{
	entryNode = NewBasicBlock(BB_CFG_ENTRY, &EntryLbl);

	exitNode = NewBasicBlock(BB_CFG_EXIT, &ExitLbl);
	
}


PFLCFG::~PFLCFG()
{
	//debug
	//cout << "PFLCFG distrutto" << endl;
#if 0
	uint32_t size = this->m_NodeList.size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (this->m_NodeList[i] != NULL)
			delete this->m_NodeList[i]->NodeInfo;
	}
#endif
}


bool PFLCFG::StoreLabelMapping(string label, BBType *bb)
{
	return m_BBLabelMap.LookUp(label, bb, ENTRY_ADD);
}

void PFLCFG::EnterMacroBlock(void)
{
	MacroBlock *mb = new MacroBlock;
	if (mb == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_CurrMacroBlock = mb;
	MacroBlocks.push_back(mb);

}

void PFLCFG::ExitMacroBlock(void)
{
	m_CurrMacroBlock = NULL;
}

PFLCFG::BBType *PFLCFG::LookUpLabel(string label)
{
	BBType *bb = NULL;
	if (!m_BBLabelMap.LookUp(label, bb))
		return NULL;

	return bb;
}


PFLCFG::BBType *PFLCFG::NewBasicBlock(BBKind kind, SymbolLabel *startLbl)
{
	if (startLbl == NULL)
	{
		startLbl = new SymbolLabel(LBL_CODE, m_BBCount, string("BB_")+int2str(m_BBCount,10));
		if (startLbl == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	}
	BBType *bb = new BBType(kind, startLbl, m_BBCount++);
	if (bb == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	StoreLabelMapping(startLbl->Name, bb);

	node_t *graphNode = &AddNode(bb);
	
	bb->setNode(graphNode);
	allBB[bb->getId()] = bb;

	if (m_CurrMacroBlock)
		m_CurrMacroBlock->BasicBlocks.push_back(bb);

	return bb;
}

PFLCFG::BBType *PFLCFG::NewBasicBlock(SymbolLabel *startLbl)
{
	return NewBasicBlock(BB_CODE, startLbl);
}

void PFLCFG::RemoveDeadNodes(void)
{
	bool changed = true;
	//uint32 round = 0;
	while (changed)
	{
		NodeIterator i = FirstNode();
		changed = false;
		for (; i != LastNode(); i++)
		{
			BBType *bb = i.current()->NodeInfo;
			if (bb->Kind != BB_CFG_ENTRY && bb->Valid)
			{
				if (i.current()->GetPredecessors().empty())
				{
					changed = true;
					bb->Valid = false;
				}
			}
		}
		if (changed)
		{
			NodeIterator i = FirstNode();
			for(; i != LastNode(); i++)
			{
				BBType *bb = i.current()->NodeInfo;
				if (!bb->Valid)
				{ 
					list<node_t*> &succ = bb->getNode()->GetSuccessors();
					list<node_t*>::iterator k = succ.begin();
					for (; k != succ.end(); k++)
						DeleteEdge(*(*k), *bb->getNode());
					//delete bb;
					//Graph.DeleteNode(*i.current());
				}
			}
		}
	}
}

void PFLCFG::ResetValid(void)
{
	NodeIterator i = FirstNode();
	for (; i != LastNode(); i++)
	{
		BBType *bb = i.current()->NodeInfo;
		bb->Valid = false;
	}
}

void CFGBuilder::AddEdge(BBType *from, BBType *to)
{
	nbASSERT(from != NULL, "from cannot be NULL");
	nbASSERT(to != NULL, "to cannot be NULL");
	m_CFG.AddEdge(*from->getNode(), *to->getNode());
}

// This method was written by Ivano Cerrato - 2nd Jan 2013
void CFGBuilder::ReproduceEdges(BBType *bb)
{								
	CodeList &codeList = bb->Code;
	StmtBase *stmt = codeList.Front();
	StmtBase *last = codeList.Back();

	bool flag = true;
	while(true)
	{
		//nbASSERT(stmt != NULL, "There is a bug!");
		if(stmt == NULL) 
			break;
		switch(stmt->Kind)
		{
			case STMT_LABEL:
			case STMT_GEN:
			case STMT_BLOCK: 
			case STMT_COMMENT:
				break;
			case STMT_JUMP:
			case STMT_JUMP_FIELD:
			{
				BBType *targetBB = NULL;
				StmtJump *stmtJump = (StmtJump*)stmt;
				SymbolLabel *trueBr = stmtJump->TrueBranch;
				if (trueBr)
				{
					targetBB = m_CFG.LookUpLabel(trueBr->Name);
					if(!targetBB)
						targetBB = m_CFG.NewBasicBlock(trueBr);//creates a new basic block that starts at the branch-target label
					AddEdge(bb, targetBB);//adds an edge from the current bb to the one which starts at the current label
				}
				SymbolLabel *falseBr = stmtJump->FalseBranch; 
				if (falseBr)
				{
					targetBB = m_CFG.LookUpLabel(falseBr->Name);
					if(!targetBB)
						targetBB = m_CFG.NewBasicBlock(falseBr);//creates a new basic block that starts at the branch-target label
					AddEdge(bb, targetBB);//adds an edge from the current bb to the one which starts at the current label
				}
				break;
			}		
			case STMT_SWITCH:
			{	
				BBType *targetBB = NULL;
				StmtSwitch *stmtSwitch = (StmtSwitch*)stmt;
				StmtBase *caseSt = stmtSwitch->Cases->Front();
				while (caseSt)//for every case in the switch
				{
					StmtCase *caseStmt = (StmtCase*)caseSt;
					nbASSERT(caseStmt->Target != NULL, "target should be != NULL");
					targetBB = m_CFG.LookUpLabel(caseStmt->Target->Name);
					if(!targetBB)
						targetBB = m_CFG.NewBasicBlock(caseStmt->Target);//creates a new basic block that starts at the branch-target label
					AddEdge(bb, targetBB);//adds an edge from the current bb to the one which starts at the current label
					caseSt = caseSt->Next;
				}
				if (stmtSwitch->Default)
				{
					nbASSERT(stmtSwitch->Default->Target != NULL, "default target should be != NULL");
					targetBB = m_CFG.LookUpLabel(stmtSwitch->Default->Target->Name);
					if(!targetBB)
						targetBB = m_CFG.NewBasicBlock(stmtSwitch->Default->Target);//creates a new basic block that starts at the branch-target label
					AddEdge(bb, targetBB);//adds an edge from the current bb to the one which starts at the current label
				}			
				break;
			}
			default:
				nbASSERT(false, "CANNOT BE HERE");
				break;
		}
	
		if(!flag)
			break;

		stmt = stmt->Next;
		if(stmt==last)
			flag = false;
	}
}

void CFGBuilder::RemEdge(BBType *from, BBType *to)
{
	nbASSERT(from != NULL, "from cannot be NULL");
	nbASSERT(to != NULL, "to cannot be NULL");
	m_CFG.DeleteEdge(*from->getNode(), *to->getNode());
	
}

// This method was written by Ivano Cerrato - 2nd Jan 2013
void CFGBuilder::RemAllOutEdges(BBType *bb)
{
	nbASSERT(bb != NULL, "bb cannot be NULL");
	DiGraph<PFLBasicBlock*>::GraphNode *node = NULL;
	node = bb->getNode();
	list<DiGraph<PFLBasicBlock*>::GraphNode*> successors = node->GetSuccessors();
	for(list<DiGraph<PFLBasicBlock*>::GraphNode*>::iterator it = successors.begin(); it != successors.end(); it++)
		m_CFG.DeleteEdge(*node, **it);
	
}


bool CFGBuilder::AddEdge(SymbolLabel *from, SymbolLabel *to)
{
	BBType *bbFrom = m_CFG.LookUpLabel(from->Name);
	if (bbFrom == NULL)
		return false;
	BBType *bbTo = m_CFG.LookUpLabel(to->Name);
	if (bbTo == NULL)
		return false;
	AddEdge(bbFrom, bbTo);
	return true;
}

bool CFGBuilder::RemEdge(SymbolLabel *from, SymbolLabel *to)
{
	BBType *bbFrom = m_CFG.LookUpLabel(from->Name);
	if (bbFrom == NULL)
		return false;
	BBType *bbTo = m_CFG.LookUpLabel(to->Name);
	if (bbTo == NULL)
		return false;
	RemEdge(bbFrom, bbTo);
	return true;
}



void CFGBuilder::FoundLabel(StmtLabel *stmtLabel)
{
	BBType *bb = m_CFG.LookUpLabel(LABEL_SYMBOL(stmtLabel)->Name);

/*	uint32 flag=0;
	if(LABEL_SYMBOL(stmtLabel)->Name=="if_true_l40")
	{
		cout<<"TROVATAAAAA - TRUE"<<endl;
		flag=1;
	}

	if(LABEL_SYMBOL(stmtLabel)->Name=="if_false_l41")
	{
		cout<<"TROVATAAAAA - FALSE!!"<<endl;
		flag=1;
	}
	if(LABEL_SYMBOL(stmtLabel)->Name=="case_l44")
	{
		cout<<"TROVATAAAAA -case_44 interno"<<endl;
		flag=1;
	}

*/
	if (bb)
	{

//		if(flag==1) cout<<"BB EXISTS"<<endl;

		//The label is the target of a forward jump
		//we have found a label that is the target of a jump. So the label is the start of a basic block
		if (m_CurrBB != NULL)
		{//the istruction above the label represents the end of the current basic block
			m_CurrBB->Code.SetTail(stmtLabel->Prev);
			AddEdge(m_CurrBB, bb); //adds an edge from the current basic block to the jump-target one
		}

		//this label is the start of a new basic block
		bb->Code.SetHead(stmtLabel);
		m_CurrBB = bb; ///changes the current basic block to the new one
		
		return;
	}
/*
	else 
	{
//		if(flag==1){
			cout<<"BB DON'T EXISTS: "<<LABEL_SYMBOL(stmtLabel)->Name<<endl;
//			flag=2;}
	}
*/	   	

	if (m_CurrBB == NULL)
	{

/*		 if (flag==1 || flag==2)
		{
		cout<<"EH??CURRBBBBBBB"<<endl;
		flag=0;
		}
*/
		//First label found, the current basic block is not set, so we create it
		//creates a new basic block that starts end ends at this label
		m_CurrBB = m_CFG.NewBasicBlock(LABEL_SYMBOL(stmtLabel));
		m_CurrBB->Code.SetHead(stmtLabel);
		m_CurrBB->Code.SetTail(stmtLabel);
		if (IsFirstBB)
		{//the basic block just created is the first basic block
			AddEdge(m_CFG.getEntryBB(), m_CurrBB);//link the new bb (the one created) to the start basic block
			IsFirstBB = false;
		}
				
		return;
	}
/*	else
	{
		cout<<"m_CurrBB != NULL: "<<LABEL_SYMBOL(stmtLabel)->Name<<endl;
	}
	

	cout<<"SOMEONE GET HERE?"<<endl;
*/
	//The label is in the middle of a basic block
	m_CFG.StoreLabelMapping(LABEL_SYMBOL(stmtLabel)->Name, m_CurrBB);
	
}



void CFGBuilder::ManageJumpTarget(SymbolLabel *target)
{
	BBType *targetBB = m_CFG.LookUpLabel(target->Name);
	if (targetBB)
	{
		//there is already a bb which starts at the current label
		//Target label has already a mapping
				
		if (!targetBB->Code.Empty())
		{	//the target bb has already code
			//Target is a previously visited BB (otherwise this could be a forward branch to a label that was
			//already target of a forward branch
			if (targetBB->StartLabel->Name.compare(target->Name) != 0)
			{
				//target label is in the middle of a basic block, so we split it
				BBType *newBB = m_CFG.NewBasicBlock(target);
				StmtBase *oldTail = targetBB->Code.Back();
				targetBB->Code.SetTail(target->Code->Prev);
				newBB->Code.SetHead(target->Code);
				newBB->Code.SetTail(oldTail);
				
				//Ivano: the next three calls together moves some edges from targetBB to newBB		
				ReproduceEdges(newBB); 
				RemAllOutEdges(targetBB);
				ReproduceEdges(targetBB);

				AddEdge(m_CurrBB, newBB);
				AddEdge(targetBB, newBB);
				if (targetBB == m_CurrBB)
					m_CurrBB = newBB;
				return;
			}
		}
		AddEdge(m_CurrBB, targetBB);//adds an edge from the current bb to the one which starts at the current label
		return;
	}
	// Target label does not have a mapping
	
	targetBB = m_CFG.NewBasicBlock(target);//creates a new basic block that starts at the branch-target label
	AddEdge(m_CurrBB, targetBB);//adds an edge from the current bb to the one which starts at the current label

}

void CFGBuilder::FoundJump(StmtJump *stmtJump)
{
	nbASSERT(stmtJump->TrueBranch, "a jump stmt should have at least a true branch");
	m_CurrBB->Code.SetTail(stmtJump);//a jump is the end of a bb
	SymbolLabel *trueBr = stmtJump->TrueBranch;
	if (trueBr)
		ManageJumpTarget(trueBr);
	SymbolLabel *falseBr = stmtJump->FalseBranch; 
	if (falseBr)
		ManageJumpTarget(falseBr);
	m_CurrBB = NULL;
}



void CFGBuilder::FoundSwitch(StmtSwitch *stmtSwitch)
{
	StmtBase *caseSt = stmtSwitch->Cases->Front();
	while (caseSt)//for every case in the switch
	{
		StmtCase *caseStmt = (StmtCase*)caseSt;
		nbASSERT(caseStmt->Target != NULL, "target should be != NULL");
		ManageJumpTarget(caseStmt->Target);
		caseSt = caseSt->Next;
	}
	if (stmtSwitch->Default)
	{
		nbASSERT(stmtSwitch->Default->Target != NULL, "default target should be != NULL");
		ManageJumpTarget(stmtSwitch->Default->Target);
	}
	m_CurrBB->Code.SetTail(stmtSwitch);
	m_CurrBB = NULL;
}

//creates a new basic block
void CFGBuilder::ManageNoLabelBB(StmtBase *stmt)
{	
	//the current statement is both the head and the tail of the new basic block
	m_CurrBB = m_CFG.NewBasicBlock(NULL);
	m_CurrBB->Code.SetHead(stmt);
	m_CurrBB->Code.SetTail(stmt);
}


//creates the control flow graph
void CFGBuilder::BuildCFG(CodeList &codeList)
{

	StmtBase *first = codeList.Front();
	StmtBase *last = codeList.Back();
	StmtBase *stmt = first;
	while(stmt)//for every statement in the codeList
	{
		switch(stmt->Kind)
		{
			case STMT_LABEL:
				FoundLabel((StmtLabel*)stmt);	
				break;
			
			case STMT_GEN:
				if (m_CurrBB == NULL) //there is no basic block yet
					ManageNoLabelBB(stmt); 
				m_CurrBB->Code.SetTail(stmt);//set the statement as the end of the current basic block
				// [ds] this should be fixed...
				if (stmt->Forest->Op == RET /*|| stmt->Forest->Op == SNDPKT*/)
				{
					//the istruction is RET so this is the end of the current bb
					AddEdge(m_CurrBB, m_CFG.getExitBB());//links the current bb to the exit one
					m_CurrBB = NULL;
				}
				break;

			case STMT_JUMP:
			case STMT_JUMP_FIELD:
				if (m_CurrBB == NULL)
					ManageNoLabelBB(stmt);
				FoundJump((StmtJump*)stmt); 
				break;

			case STMT_SWITCH:
				if (m_CurrBB == NULL)
					ManageNoLabelBB(stmt);
				FoundSwitch((StmtSwitch*)stmt);
				break;

			case STMT_BLOCK: 
				m_CFG.EnterMacroBlock();
				m_CFG.m_CurrMacroBlock->Caption =((StmtBlock*)stmt)->Comment;
				BuildCFG(*((StmtBlock*)stmt)->Code); //creates a new CFG for the block
				m_CFG.ExitMacroBlock();
				break;
			case STMT_COMMENT:
				break;

			default:
				nbASSERT(false, "CANNOT BE HERE");
				break;
		}
		if (stmt == last && m_CurrBB)
			AddEdge(m_CurrBB, m_CFG.getExitBB());//adds an edge from the current bb to the exit one
		stmt = stmt->Next;
	}

}

//The CodeList contains MIR statements
void CFGBuilder::Build(CodeList &codeList)
{
	typedef PFLCFG::SortedIterator s_it_t;

	BuildCFG(codeList); //creates the control flow graph

	//patch the code-list of every basic block
	PFLCFG::NodeIterator i = m_CFG.FirstNode();
	for (; i != m_CFG.LastNode(); i++)
	{

		BBType *bb = i.current()->NodeInfo;

		if (!bb->Code.Empty())	//THIS CAUSE ISSUES IN MIR LIST! (FILTER=HTTP -> WHEN CYCLE SECOND TIME-> GET IN AND CHANGE MIR!) 
		{
			bb->Code.Front()->Prev = NULL;
			bb->Code.Back()->Next = NULL;
		}
		
		//Translate code generated into the MIRONode form
		//std::cout << "Traduco il BB: " << bb->getId() << std::endl;
		//NodeTranslator is in node_translation.h

		NodeTranslator nt(&bb->Code, bb->getId());			//mligios here mir change!
		(nt.translate(bb->getMIRNodeCode())); //generates the MIRO code

		bb->setProperty("reached", false);

	}



	m_CFG.SortPreorder(*m_CFG.getEntryNode());
	for(s_it_t j = m_CFG.FirstNodeSorted(); j != m_CFG.LastNodeSorted(); j++)
	{
		BBType *bb = (*j)->NodeInfo;
		bb->setProperty<bool>("reached", true);
	}

	PFLCFG::NodeIterator k = m_CFG.FirstNode();
	for (; k != m_CFG.LastNode(); )
	{
		BBType *bb = (*k)->NodeInfo;
		if(!bb->getProperty<bool>("reached"))
		{
			//std::cout << "Il BB: " << bb->getId() << " Non è raggiungibile" << std::endl;
			//iterate over predecessors
			m_CFG.removeBasicBlock(bb->getId());
			k++;

		}
		else
			k++;

	}
	
}

void PFLBasicBlock::add_copy_head(RegType src, RegType dst)
{
		// Ask the IR to generate a copy node
		IRNode::CopyType *n(IRNode::make_copy(getId(), src, dst));
		
		// Scroll the list of instruction until we find the last phi
		std::list<IRNode *>::iterator i, j;
		for(i = getCode().begin(); i != getCode().end() && (*i)->isPhi(); ++i)
			;
			
		getCode().insert(i, n);
	
		return;
}

void PFLBasicBlock::add_copy_tail(PFLBasicBlock::IRNode::RegType src, PFLBasicBlock::IRNode::RegType dst)
{
			// Ask the IR to generate a copy node
			IRNode::CopyType *n(IRNode::make_copy(getId(), src, dst));

			// Scroll the list of instruction until we find one that is not a jump - should take one try only
			std::list<IRNode *>::iterator i, j;

			getCode().reverse(); // Consider the list from the last element to the first
			for(i = getCode().begin(); i != getCode().end() && (*i)->isJump(); ++i)
				;

			getCode().insert(i, n);

			getCode().reverse(); // Go back to the original order

			return;
}


uint32_t PFLBasicBlock::getCodeSize()
{
	uint32_t counter = 0;
	typedef std::list<MIRONode*>::iterator l_it;
	for(IRStmtNodeIterator i = codeBegin(); i != codeEnd(); i++)
	{
		if( (*i)->isStateChanger() )	
			counter++;
	}
	return counter;
}

MIRONode *PFLBasicBlock::getLastStatement()
{
	if(MIRONodeCode->size() > 0)
		return *(--codeEnd());
	else
		return NULL;
}
