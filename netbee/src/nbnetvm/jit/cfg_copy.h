/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef CFG_COPY_H
#define CFG_COPY_H
/*!
 * \file cfg_copy.h
 * \brief this file contains the definition of a class that copies the structure of a CFG
 */

#include "digraph.h"
#include "basicblock.h"
#include "cfg.h"
#include "cfg_builder.h"
#include "../int_structs.h"

namespace jit {
	/*!
	 * \brief A class for copying the structure of a CFG
	 */
	template<class fromIR, class toIR>
	class CFGCopy : public CFGBuilder<toIR>, public DiGraph< BasicBlock<fromIR>* >::IGraphVisitor
	{
		public:
		typedef typename DiGraph< BasicBlock<fromIR>* >::GraphNode GraphNode; //!<type of the node type of the source CFG
		typedef typename DiGraph< BasicBlock<fromIR>* >::GraphEdge GraphEdge; //!<type of the edge type of the source CFG

		/*!
		 * \brief constructor
		 * \param fromCfg the source CFG
		 * \param toCfg the copy CFG
		 */
		CFGCopy(CFG<fromIR>& fromCfg, CFG<toIR>& toCfg): CFGBuilder<toIR>(toCfg), fromCfg(fromCfg) {}
		//! the member function that implements the copy
		void buildCFG();

		//!method called by the visit
		int32_t ExecuteOnEdge(const GraphNode& visitedNode,const GraphEdge& edge, bool lastEdge);
		//!method called by the visit
		int32_t ExecuteOnNodeFrom(const GraphNode& visitedNode, const GraphNode* comingFrom);

		private:
		CFG<fromIR>& fromCfg; //!< the target CFG
	};

	/*!
	 * Simply use the Preorder visit algorithm exported from digraph
	 */
	template<class fromIR, class toIR>
	void CFGCopy<fromIR, toIR>::buildCFG()
	{
		GraphNode* entry = fromCfg.getEntryNode();
		this->fromCfg.PreOrder(*entry, *this);
	}

	/*!
	 * Function called on every edge of the source CFG. Creates the same edge in the target CFG
	 */
	template<class fromIR, class toIR>
	int32_t CFGCopy<fromIR, toIR>::ExecuteOnEdge(const GraphNode& visitedNode,const GraphEdge& edge, bool lastEdge)
	{
		uint16_t from = edge.From.NodeInfo->getId();
		uint16_t to = edge.To.NodeInfo->getId();
		this->addEdge(from, to);
		return 1;
	}

	/*!
	 * Function called on every basic block of the source CFG. Adds the same basic block in the target CFG
	 */
	template<class fromIR, class toIR>
	int32_t CFGCopy<fromIR, toIR>::ExecuteOnNodeFrom(const GraphNode& visitedNode, const GraphNode* comingFrom)
	{
		BasicBlock<toIR> *new_bb;
		new_bb = this->addBasicBlock(visitedNode.NodeInfo->getId());

		//---------------------------------[PV]
		//Needed to forward loop information to the new cfg
		uint16_t *loop_level;
		loop_level = visitedNode.NodeInfo->template getProperty<uint16_t*>("loopLevel");
		new_bb->setProperty("loopLevel", loop_level);
		//-------------------------------

		// Propagate the BB handler tag.
		Taggable *old_bb;
		old_bb = visitedNode.NodeInfo;		
		new_bb->template setProperty<nvmPEHandler *>("handler", old_bb->template getProperty<nvmPEHandler *>("handler"));

		return 1;
	}
}

#endif
