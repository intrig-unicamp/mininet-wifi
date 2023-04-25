/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __OP_SIZE_H__
#define __OP_SIZE_H__

#include "cfg.h"
#include "mirnode.h"
#include "basicblock.h"
#include "registers.h"
#include "digraph.h"

#include <map>

namespace jit {

class Infer_Size {
  public:
	typedef jit::CFG<MIRNode> CFG;
	typedef jit::BasicBlock<CFG::IRType> BasicBlock;
	typedef std::map<RegisterInstance, operand_size_t> regsize_map_t;

  private:
	CFG &cfg;

	/* annotate annotates every IRNode with the size of the result that
	 * is computed by that node or that is held in that node */
	void annotate();
	
	/* adapt modifies each and every node containing a constant (CNST) and
	 * every node containing a LDRREG so that it is explicited the size of that constant/register.
	 * This helps a lot while writing the BURG instruction file.
	 * If revert_modifications is not zero then the modifications are undone (i.e., CNST is put back
	 * everywhere it was, and the same thing happens to LDREG).
	 */
	void adapt();
	
	regsize_map_t map;

	void Find_Live_Range_Size(BasicBlock *bb);
	void Find_Insn_Live_Range_Size(MIRNode * insn);
	void Set_Live_Range_Size(BasicBlock *bb);
	void Set_Insn_Live_Range_Size(MIRNode *insn);

	operand_size_t get_constant_size(uint32_t cnst, bool has_sign = false);

	void propagate_size(MIRNode *, operand_size_t);

	void Touch_Constants(BasicBlock *bb);
	void Touch_Insn_Constants(MIRNode *insn);
	void Propagate(BasicBlock *BB);

	
	class visitor_find_live_range_size : public CFG::IGraphVisitor
	{
		private:
			CFG& cfg; //!<the CFG containing the code to linearize
			Infer_Size &is;

		public:
			/*!
			 * \brief constructor
			 * \param cfg the CFG to annotate
			 */
			visitor_find_live_range_size(CFG& _cfg, Infer_Size &i) : cfg(_cfg), is(i) {}

			typedef CFG::node_t GraphNode; //!<type of a CFG node
			typedef CFG::edge_t GraphEdge; //!<type of a CFG edge
			
			int32_t ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge)
			{
				return nvmJitSUCCESS;
			};
			
			int32_t ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom) {
				BasicBlock *BB = visitedNode.NodeInfo;
//				std::for_each(BB->codeBegin(), BB->codeEnd(), *this);
				(*this)(BB);
				return nvmJitSUCCESS;
			};

			//!operation called on a node
			bool operator()(BasicBlock *bb);
	};

	class visitor_set_live_range_size : public CFG::IGraphVisitor
	{
		private:
			CFG &cfg; //!<the CFG containing the code to linearize
			Infer_Size &is;

		public:
			/*!
			 * \brief constructor
			 * \param cfg the CFG to linearize
			 */
			visitor_set_live_range_size(CFG &_cfg, Infer_Size &i) : cfg(_cfg), is(i) {}

			typedef CFG::node_t GraphNode; //!<type of a CFG node
			typedef CFG::edge_t GraphEdge; //!<type of a CFG edge
			
			int32_t ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge)
			{
				return nvmJitSUCCESS;
			};
			
			int32_t ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom) {
				BasicBlock *BB = visitedNode.NodeInfo;
//				std::for_each(BB->codeBegin(), BB->codeEnd(), *this);
				(*this)(BB);
				return nvmJitSUCCESS;
			};

			//!operation called on a node
			bool operator()(BasicBlock *bb);
	};
	
	class visitor_propagate : public CFG::IGraphVisitor {
	  private:
		CFG &cfg; //!<the CFG containing the code to linearize
		Infer_Size &is;

	  public:
		/*!
		 * \brief constructor
		 * \param cfg the CFG to linearize
		 */
		visitor_propagate(CFG &_cfg, Infer_Size &i) : cfg(_cfg), is(i) {}

		typedef CFG::node_t GraphNode; //!<type of a CFG node
		typedef CFG::edge_t GraphEdge; //!<type of a CFG edge
			
		int32_t ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge)
		{
			return nvmJitSUCCESS;
		};
			
		int32_t ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom) {
			BasicBlock *BB = visitedNode.NodeInfo;
//			std::for_each(BB->codeBegin(), BB->codeEnd(), *this);
			(*this)(BB);
			return nvmJitSUCCESS;
		};

		//!operation called on a node
		bool operator()(BasicBlock *bb);
		
	};
	
	friend class visitor_find_live_range_size;
	friend class visitor_set_live_range_size;

  public:
	Infer_Size(CFG &c);
	~Infer_Size();
	
	void run();
	
	regsize_map_t *getMap();

}; /* class Infer_Size */

} /* namespace jit */

#endif /* __OP_SIZE_H__ */
