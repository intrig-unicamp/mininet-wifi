/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file cfg_builder.h
 * \brief this file contains the definition of the cfg builder interface
 */
#ifndef CFG_BUILDER_H
#define CFG_BUILDER_H

#include "../../nbee/globals/debug.h"
#include "digraph.h"
#include "cfg.h"
#include <list>

namespace jit {
	/*!
	 * \brief this is a simple interface to a cfg builder
	 *
	 * This interface should be used by classes that need to build a cfg from some data.<br>
	 * It has some methods that helps in building a new cfg
	 */
	template<class IR>
	class CFGBuilder
	{
		public: 
			//!member to override by implementers to perform the building
			virtual void buildCFG() = 0;

			//!constructor
			CFGBuilder(CFG<IR>& cfg);
			//!destructor
			virtual ~CFGBuilder(){}

		protected:
			CFG<IR>& cfg;//!<The cfg to build

			//!add a basic block to the cfg
			BasicBlock<IR> *addBasicBlock(uint16_t Id);
			//!add an edge to the cfg between to basic block
			void addEdge(uint16_t from, uint16_t to);
			//!add an edge to the cfg between to basic block
			void addEdge(BasicBlock<IR> *from, BasicBlock<IR> *to);

	};

	template<class IR>
		CFGBuilder<IR>::CFGBuilder(CFG<IR>& cfg) : cfg(cfg) { }

	/*!
	 * \param Id the id of the basic block to add
	 * \return the basic block added
	 *
	 * Add a basic block to the cfg only if not already present.
	 */
	template<class IR>
		BasicBlock<IR> *CFGBuilder<IR>::addBasicBlock(uint16_t Id)
		{
			BasicBlock<IR> *newBB = cfg.allBB[Id];

			if(!newBB)
			{
				newBB = new BasicBlock<IR>(Id);
				cfg.allBB[Id] = newBB;
				newBB->nodePtr = &cfg.AddNode(newBB);
			}

			return newBB; 
		}

	/*!
	 * \param from the id of the basic block the edge comes from
	 * \param to the id of the basic block the edge goes to
	 *
	 * Add an edge to the CFG between to basic blocks. If one of the node does not exist 
	 * a new basic block is created and added to the CFG.
	 */
	template<class IR>
		void CFGBuilder<IR>::addEdge(uint16_t from, uint16_t to)
		{
			BasicBlock<IR> *fromBB = addBasicBlock(from);
			BasicBlock<IR> *toBB = addBasicBlock(to);

			addEdge(fromBB, toBB);
		}

	/*!
	 * \param from pointer to the basic block the edge comes from
	 * \param to pointer to the basic block the edge goes to
	 *
	 * Add an edge to the CFG between to basic blocks. 
	 */
	template<class IR>
		void CFGBuilder<IR>::addEdge(BasicBlock<IR> *from, BasicBlock<IR> *to)
		{
			NETVM_ASSERT(from!=NULL, __FUNCTION__"Adding an edge to a NULL basic block");
			NETVM_ASSERT(to!=NULL, __FUNCTION__"Adding an edge to a NULL basic block");
			cfg.AddEdge(*from->nodePtr, *to->nodePtr);
		}

}
#endif
