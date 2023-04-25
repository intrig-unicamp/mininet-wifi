/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __CFG_EDGE_SPLITTER_H__
#define __CFG_EDGE_SPLITTER_H__

#include "cfg_builder.h"
#include "jit_internals.h"
#include <list>

namespace jit {
/*!
 * \brief simple functor-like class to split a CFG's critical edges
 *
 * For the exit-from-SSA algorithm it is necessary to split critical edges,
 * that is, there must be no edge in the CFG that goes from a BB that has more than
 * one successor to a BB that has more than one predecessor.<br>
 * This algorithm splits such edges by inserting a new, empty BB in the middle.
 */
template<class IR>
class CFGEdgeSplitter : private CFGBuilder<IR>, public nvmJitFunctionI {
  public: 			
	//!split edges that go from a BB that has more than one successor to a BB that has more than one predecessor
	bool run();
	
	virtual void buildCFG() {run(); return;};
			
	CFGEdgeSplitter(CFG<IR>& cfg);
	virtual ~CFGEdgeSplitter();
	
};

template<typename IR>
CFGEdgeSplitter<IR>::~CFGEdgeSplitter()
{
	// Nop
	return;
}

template<typename IR>
CFGEdgeSplitter<IR>::CFGEdgeSplitter(CFG<IR>& cfg) : CFGBuilder<IR>(cfg), nvmJitFunctionI("Edge splitter")
{
	// Nop
	return;
}

/*!
 * Split critical edges inserting a new, empty BB to bridge the 2 ends of the edge.
 * This is needed for the exit-from-SSA algorithm to work.
 *
 */
template<class IR>
bool CFGEdgeSplitter<IR>::run()
{
	if(CFGBuilder<IR>::cfg.BBListSize() < 2) {
		// No BBs, no party
		return true;
	}
	
	//std::cout << "Edge Splitting" << std::endl;
	
	std::list<BasicBlock<IR> *> *bblist(CFGBuilder<IR>::cfg.getBBList());

	// Get the highest BB number (skip the -1 one)
//	uint16_t highest((++(bblist->rbegin()))->first);
//	++highest;
	// FIXME: assumes that the BB numbering is contiguous
	uint16_t highest(CFGBuilder<IR>::cfg.BBListSize() - 1);

	// Iterate over every BB - order is not relevant
	typename CFG<IR>::NodeIterator i(CFGBuilder<IR>::cfg.FirstNode());
			
	for(; i != CFGBuilder<IR>::cfg.LastNode(); ++i) {
		BasicBlock<IR> *bb((*i)->NodeInfo);
		
		////std::cout << "iterating over BB " << bb->getId() << endl;
		// does the BB have more than one successor?
		if(bb->getSuccessors().size() > 1) {
			// iterate over successors
			std::list<typename BasicBlock<IR>::node_t *> l = bb->getSuccessors();
			typename std::list<typename BasicBlock<IR>::node_t *>::iterator li;
			
			//std::cout << "iterating over successors of BB " << bb->getId() << endl;
			for(li = l.begin(); li != l.end(); ++li) {
				// Does the successor has more than one predecessor?
				BasicBlock<IR> *ss((*li)->NodeInfo);
				//std::cout << "    successor " << ss->getId();
				if(ss->getPredecessors().size() > 1) {
					//std::cout << " has more than one predecessor" << endl;
					
					/* Tell the last instruction of the predecessor the new
					 * destination BB ID */
					if(bb->getCode().rbegin() != bb->getCode().rend())
						(*(bb->getCode().rbegin()))->rewrite_destination(ss->getId(), highest);

					//std::cout << "    deleting edge " << bb->getId() << "->" << ss->getId() << endl;
					
					// delete edge (current, successor)
					CFGBuilder<IR>::cfg.DeleteEdge(**i, **li);

					//std::cout << "    adding edge " << bb->getId() << "->" << highest << endl;
					// insert new edge (current, new)
					this->addEdge(bb->getId(), highest);
					//std::cout << "    adding edge " << highest << "->" << ss->getId() << endl;
					// insert new edge (new, successor)
					this->addEdge(highest, ss->getId());

					BasicBlock<IR> *newBB = CFGBuilder<IR>::cfg.getBBById(highest);
					assert(bb->template getProperty<nvmPEHandler*>("handler") == ss->template getProperty<nvmPEHandler*>("handler"));
#ifdef _DEBUG_CFG_BUILD
					//std::cout << "Setto la proprieta' \"handler\" per il BB: " << newBB->getId() << std::endl;
#endif
					newBB->setProperty("handler", bb->template getProperty<nvmPEHandler*>("handler"));
							
					++highest;
				}
				//std::cout << endl;
			}
		}
	}
	
	delete bblist;
							
	return true;
}


} /* namespace jit */

#endif /* __CFG_EDGE_SPLITTER__ */
