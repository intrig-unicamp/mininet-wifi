#ifndef __CFG_EDGE_SPLITTER_H__
#define __CFG_EDGE_SPLITTER_H__

#include "basicblock.h"
#include <list>



/*!
 * \brief simple functor-like class to split a CFG's critical edges
 *
 * For the exit-from-SSA algorithm it is necessary to split critical edges,
 * that is, there must be no edge in the CFG that goes from a BB that has more than
 * one successor to a BB that has more than one predecessor.<br>
 * This algorithm splits such edges by inserting a new, empty BB in the middle.
 */
template<class IR>
class CFGEdgeSplitter {
	private:
		PFLCFG &_cfg;
  public: 			
	//!split edges that go from a BB that has more than one successor to a BB that has more than one predecessor
	bool run();
	
	virtual void buildCFG() {run(); return;};
			
	CFGEdgeSplitter(PFLCFG& cfg);
	virtual ~CFGEdgeSplitter();
	//void addEdge(uint16_t, uint16_t);	
	//void addEdge(PFLBasicBlock *from, PFLBasicBlock *to);	
};

template<typename IR>
CFGEdgeSplitter<IR>::~CFGEdgeSplitter()
{
	// Nop
	return;
}

template<typename IR>
CFGEdgeSplitter<IR>::CFGEdgeSplitter(PFLCFG& cfg) : _cfg(cfg)
{
	// Nop
	return;
}


#if 0
template<typename IR>
void CFGEdgeSplitter<IR>::addEdge(PFLBasicBlock *from, PFLBasicBlock *to)
{
	_cfg.AddEdge(*from->getNode(), *to->getNode());
}



template<typename IR>
void CFGEdgeSplitter<IR>::addEdge(uint16_t from, uint16_t to)
{
	PFLBasicBlock *newfrom = _cfg.getBBById(from);
	assert(newfrom);
/*
	if(!newfrom)
	{
		newfrom = new PFLBasicBlock(from);
		_cfg.setBBInMap(from, newfrom);
		newfrom->setNode(&_cfg.AddNode(newfrom));
	}
*/

	PFLBasicBlock *newto = _cfg.getBBById(to);
	assert(newto);

/*
	if(!newto)
	{
		newto = new PFLBasicBlock(to);
		_cfg.setBBInMap(to, newto);
		newto->setNode(&_cfg.AddNode(newto));
	}
*/	
	_cfg.AddEdge(*newfrom->getNode(), *newto->getNode());
	//std::cout << "    adding edge " << newfrom->getId() << "->" << newto->getId() << endl;
}
#endif

/*!
 * Split critical edges inserting a new, empty BB to bridge the 2 ends of the edge.
 * This is needed for the exit-from-SSA algorithm to work.
 *
 */
template<class IR>
bool CFGEdgeSplitter<IR>::run()
{
	if(_cfg.BBListSize() < 2) {
		// No BBs, no party
		return true;
	}
	//std::cout << "Edge Splitting" << std::endl;
	std::list<PFLBasicBlock *> *bblist(_cfg.getBBList());
	// Iterate over every BB - order is not relevant

	for(std::list<PFLBasicBlock *>::iterator i = bblist->begin(); i != bblist->end(); ++i)
	{
		PFLBasicBlock *bb = (*i);
		//std::cout << "iterating over BB " << bb->getId() << " " << bb->getStartLabel()->Name << endl;
		
		// does the BB have more than one successor?
		if(bb->getSuccessors().size() > 1) {
		//std::cout << "iterating over successors of BB " << bb->getId() << " " << bb->getStartLabel()->Name << endl;
			// iterate over successors
			std::list<PFLBasicBlock::node_t *> l = bb->getSuccessors();
			typename std::list<PFLBasicBlock::node_t *>::iterator li;
					
			for(li = l.begin(); li != l.end(); ++li) {
				// Does the successor has more than one predecessor?
				PFLBasicBlock *ss((*li)->NodeInfo);
				//std::cout << "    successor " << ss->getId();		
				if(ss->getPredecessors().size() > 1) {
					//std::cout << " has more than one predecessor" << endl;
//					//std::cout << bb->getId() << " -> " << highest << " -> " << ss->getId()
//						<< '\n';
					//std::cout << "    deleting edge " << bb->getId() << "->" << ss->getId() << endl;
					// delete edge (current, successor)
					_cfg.DeleteEdge(*(*i)->getNode(), **li);
					
					
					// insert new edge (current, new)
					PFLBasicBlock *newbb = _cfg.NewBasicBlock(BB_CODE, NULL);
					LabelMIRONode *lblStmt = new LabelMIRONode();
					MIRONode *lblnode = new MIRONode(IR_LABEL, newbb->getStartLabel());
					lblStmt->setKid(lblnode, 0);
					newbb->getMIRNodeCode()->push_back(lblStmt);
					_cfg.AddEdge(*(bb->getNode()), *(newbb->getNode()));
					JumpMIRONode *newjump = new JumpMIRONode(ss->getId(), ss->getStartLabel()); 		
					newjump->setOpcode(JUMPW);
					newbb->getCode().push_back(newjump);
					
					// insert new edge (new, successor)
					_cfg.AddEdge(*(newbb->getNode()), *(ss->getNode()));
										/* Tell the last instruction of the predecessor the new
					 * destination BB ID */
					if(bb->getCode().rbegin() != bb->getCode().rend())
						(*(bb->getCode().rbegin()))->rewrite_destination(ss->getId(), newbb->getId());
				}
				//std::cout << endl;
			}
		}
	}
	
	//delete bblist;
							
	return true;
}



#endif /* __CFG_EDGE_SPLITTER__ */
