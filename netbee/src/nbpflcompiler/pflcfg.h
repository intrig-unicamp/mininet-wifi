/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "digraph.h"
#include "symbols.h"
#include "statements.h"
#include "mironode.h"
#include <iostream>
#include <fstream>
#include "basicblock.h"
#include "cfg.h"


using namespace std;


enum BBKind
{
	BB_CFG_ENTRY,
	BB_CFG_EXIT,
	BB_CODE
};


struct PFLBasicBlock: public jit::BasicBlock<MIRONode>
{
	typedef DiGraph<PFLBasicBlock*>::GraphNode node_t;
	typedef PFLBasicBlock ThisType;

	BBKind			Kind;
	SymbolLabel		*StartLabel;
	bool			Valid;
	node_t 			*nodePtr;
	CodeList		Code;
	std::list<MIRONode*> *MIRONodeCode;

	PFLBasicBlock(BBKind kind, SymbolLabel *startLbl = 0, uint16 id = 0)
		: jit::BasicBlock<MIRONode>(id), Kind(kind), StartLabel(startLbl), Valid(true), nodePtr(NULL), Code(), MIRONodeCode(NULL) {
			MIRONodeCode = new std::list<MIRONode*>;
		}

	PFLBasicBlock(uint16_t id)
		: jit::BasicBlock<MIRONode>(id), Kind(BB_CODE), StartLabel(0), Valid(true), nodePtr(NULL), Code(), MIRONodeCode(NULL) 
	{
			MIRONodeCode = new std::list<MIRONode*>;
	}


	~PFLBasicBlock();

	node_t * getNode() { return dynamic_cast<node_t*>(nodePtr); };
	void setNode(node_t *node) { nodePtr = node; };

	void setCode(std::list<MIRONode*> *list) { MIRONodeCode = list; }
	std::list<MIRONode*> * getMIRNodeCode() { return MIRONodeCode; }

	void addHeadInstruction(MIRONode *node);
	void addTailInstruction(MIRONode *node);

	std::list<node_t *>& getSuccessors() { return getNode()->GetSuccessors(); }
	std::list<node_t *>& getPredecessors() { return getNode()->GetPredecessors(); }

	std::list<MIRONode*>& getCode() { return *MIRONodeCode; }
	
	SymbolLabel * getStartLabel() { return StartLabel;}

	void add_copy_tail(RegType src, RegType dst);
	void add_copy_head(RegType src, RegType dst);

	/*!
	 * \brief get an iterator for the code list
	 * \return an iterator at the start of the code list
	 */
	IRStmtNodeIterator codeBegin() { return MIRONodeCode->begin(); }

	/*!
	 * \brief get an iterator for the code list
	 * \return an iterator at the end of the code list
	 */
	IRStmtNodeIterator codeEnd() { return MIRONodeCode->end(); }

	/*!
	 * \brief deletes code from this BB list
	 * \param start iterator to the first element to delete
	 * \return iterator to the element after the last element deleted
	 */
	IRStmtNodeIterator eraseCode(IRStmtNodeIterator start) { return MIRONodeCode->erase(start); }

	/*!
	 * \brief deletes code from this BB list
	 * \param start iterator to the point from which to start deleting
	 * \param end iterator to the last element to delete
	 * \return iterator to the element after the last element deleted
	 */
	IRStmtNodeIterator eraseCode(IRStmtNodeIterator start, IRStmtNodeIterator end) { return MIRONodeCode->erase(start, end); }


	//! Return real statement number
	uint32_t getCodeSize(); 

	MIRONode * getLastStatement();


	class BBIterator
	{
		public:
			typedef std::list<node_t *>::iterator node_iterator_t; ///<node iterato type

			typedef ThisType* value_type; ///<type returned from this iterator
			typedef ptrdiff_t difference_type; ///<type of the difference between two iterators
			typedef ThisType** pointer; ///<type of pointer to a value returned from this iterator
			typedef ThisType*& reference; ///<type of a reference to a value returned from this iterator
			typedef std::forward_iterator_tag iterator_category; ///<category of this iterator

		private:
			node_iterator_t node_iterator; ///<iterator to the undelying list

		public:
			BBIterator(node_iterator_t node_iterator): node_iterator(node_iterator) {}

			BBIterator& operator=(const BBIterator &bbit) 
			{
				this->node_iterator = bbit.node_iterator;
				return *this;
			};

			ThisType*& operator*()
			{
				return (*node_iterator)->NodeInfo;
			}

			ThisType** operator->()
			{
				return &(*node_iterator)->NodeInfo;
			}

			BBIterator& operator++()
			{
				node_iterator++;
				return *this;
			}

			BBIterator& operator--()
			{
				node_iterator--;
				return *this;
			}

			bool operator==(BBIterator bb)
			{
				return bb.node_iterator == node_iterator;
			}

			bool operator!=(BBIterator bb)
			{
				return bb.node_iterator != node_iterator;
			}

			BBIterator& operator++(int)
			{
				node_iterator++;
				return *this;
			}

			BBIterator& operator--(int)
			{
				node_iterator--;
				return *this;
			}

	}; /* class BBIterator */
	/*!
	 * \brief get an iterator for the successors list
	 * \return an iterator to the start of the successors list
	 */
	BBIterator succBegin() { return BBIterator(nodePtr->GetSuccessors().begin()); }

	/*!
	 * \brief get an iterator for the successors list
	 * \return an iterator to the end of the successors list
	 */
	BBIterator succEnd() { return BBIterator(nodePtr->GetSuccessors().end()); }

	/*!
	 * \brief get an iterator for the predecessors list
	 * \return an iterator to the start of the predecessors list
	 */
	BBIterator precBegin() { return BBIterator(nodePtr->GetPredecessors().begin()); }

	/*!
	 * \brief get an iterator for the predecessors list
	 * \return an iterator to the end of the predecessors list
	 */
	BBIterator precEnd() { return BBIterator(nodePtr->GetPredecessors().end()); }
};


typedef StrSymbolTable<PFLBasicBlock*> LabelMap_t;

class CFGBuilder; // forward decl
class CFGVisitor;

class PFLCFG: public jit::CFG<MIRONode, PFLBasicBlock>
{
private:
	friend class CFGBuilder;
	friend class CFGVisitor;

	struct MacroBlock
	{
		string				Caption;
		list<BBType*>	BasicBlocks;
	};

	typedef list<MacroBlock*> MacroBlockList_t;

	SymbolLabel			EntryLbl;
	SymbolLabel			ExitLbl;
	LabelMap_t			m_BBLabelMap;
	uint32				m_BBCount;

	MacroBlock			*m_CurrMacroBlock;
	MacroBlockList_t	MacroBlocks;

	
public:
	BBType *NewBasicBlock(BBKind kind, SymbolLabel *startLbl);

	
	PFLCFG();

	virtual ~PFLCFG();
	
	BBType *NewBasicBlock(SymbolLabel *startLbl);
	void EnterMacroBlock(void);
	void ExitMacroBlock(void);
	bool StoreLabelMapping(string label, BBType *bb);
	BBType *LookUpLabel(string label);
	void RemoveDeadNodes(void);
	void ResetValid(void);

	uint32 GetBBCount(void)
	{
		return m_BBCount;
	}

	/*
	void GetSuccessors(BasicBlock *bb, list<BasicBlock*> &successors)
	{
		list<CFG_t::GraphNode*> &succ = bb->GraphNode->GetSuccessors();
		for (list<CFG_t::GraphNode*>::iterator i = succ.begin(); i != succ.end(); i++)
			successors.push_back((*i)->NodeInfo);
	}

	void GetPredecessors(BasicBlock *bb, list<BasicBlock*> &predecessors)
	{
		list<CFG_t::GraphNode*> &pred = bb->GraphNode->GetPredecessors();
		for (list<CFG_t::GraphNode*>::iterator i = pred.begin(); i != pred.end(); i++)
			predecessors.push_back((*i)->NodeInfo);
	}
	*/

	/*
	void GetBBList(list<BasicBlock*> &bbList)
	{
		typedef CFG_t::NodeIterator iterator_t;
		bbList.clear();
		iterator_t  n = Graph.FirstNode();
		while (n != Graph.LastNode())
		{
			bbList.push_back(n.current()->NodeInfo);
			n++;
		}
	}
	*/
};


class CFGBuilder
{
	//BB means "Basic Block"

	typedef PFLCFG::BBType BBType;

	PFLCFG			&m_CFG;
	BBType			*m_CurrBB;
	bool			IsFirstBB;			

	void VisitStatement(StmtBase *stmt);
	void FoundLabel(StmtLabel *stmtLabel);
	void FoundJump(StmtJump *stmtJump);
	void FoundSwitch(StmtSwitch *stmtSwitch);
	void ManageJumpTarget(SymbolLabel *target);
	void ManageNoLabelBB(StmtBase *stmt);
	void AddEdge(BBType *from, BBType *to);
	void RemEdge(BBType *from, BBType *to);
	void ReproduceEdges(BBType *bb);
	void RemAllOutEdges(BBType *bb);
	void BuildCFG(CodeList &codeList);
public:

	//contructor
	CFGBuilder(PFLCFG &cfg)
		:m_CFG(cfg), m_CurrBB(0), IsFirstBB(true){}

	void Build(CodeList &codeList);
	
	bool AddEdge(SymbolLabel *from, SymbolLabel *to);
	bool RemEdge(SymbolLabel *from, SymbolLabel *to);
};



