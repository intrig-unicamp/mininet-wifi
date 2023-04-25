/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef MEM_TRANSLATOR_H
#define MEM_TRANSLATOR_H

/*!
 * \file mem_translator.h
 * \brief this file contains declaration of the class which linearize the memory accesses
 */

#include "cfg.h"
#include "mirnode.h"
#include "registers.h"
#include "jit_internals.h"

namespace jit {

	//!this class linearize the memory accesses in a CFG
	class MemTranslator: public CFG<MIRNode>::IGraphVisitor	, public nvmJitFunctionI
	{
		private:
			CFG<jit::MIRNode>& cfg; //!<the CFG containing the code to linearize
			RegisterModel *mem_register; //!<the register used to store the temporary offset

		public:
			/*!
			 * \brief constructor
			 * \param cfg the CFG to linearize
			 */
			MemTranslator(CFG<MIRNode>& _cfg);

			typedef CFG<MIRNode>::node_t GraphNode; //!<type of a CFG node
			typedef CFG<MIRNode>::edge_t GraphEdge; //!<type of a CFG edge
			//!the main interface to this class
			static void translate(CFG<jit::MIRNode>& cfg);
			

			int32_t ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge);
			
			int32_t ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom);

			//!operation called on a node
			MIRNode *operator()(MIRNode *node);

			bool run();
	};

}
#endif
