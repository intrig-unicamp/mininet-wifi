/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef INSSELECTOR_H
#define INSSELECTOR_H

#include <cassert>
#include <sstream>

/*!
 * \file insselector.h
 * \brief this file contains the definition of the class that runs the Instruction selection algorithm
 */

/*!
 * \page inssel Instruction Selection
 *
 * \section burg What does burg emits?
 *
 * <p>You usually call burg with this command line:
 * <pre>NetVMBurg -C class_name -p -d header_file burg_file > cpp_file</pre>
 * and you get from your burg file two a header file and a implementation file
 * defining a new class called class_name.</p>
 *
 * <p>A valid .brg file is made of three section:
 * <ol>
 * <li>Before the first "%%" every line is included in the header file emitted</li>
 * <li>Before the second "%%" there are the rules and the emission functions</li>
 * <li>Every line after that is included in the cpp file generated</li>
 * </ol></p>
 *
 * <p>The generated class contains the method used by InsSelector reduce method. It also contains some static members
 * that are the vector of the terminal symbols and the emission function. Burg also generates a structure named
 * classname_MBState used to contains the state for every node. </p>
 *
 * \section client_req Client Requirements
 * <p>The client should define some macros in the first part of the burg file:
 * <dl>
 * <dt>MBTREE_TYPE</dt>
 * <dd>The type of the intermediate node (usually MIRNode)</dd>
 * <dt>MBTREE_LEFT</dt>
 * <dd>returns the left kid of a node</dd>
 * <dt>MBTREE_RIGHT</dt>
 * <dd>returns the right kid of a node</dd>
 * <dt>MBTREE_OP</dt>
 * <dd>returns a node's opcode</dd>
 * <dt>MBTREE_STATE</dt>
 * <dd>returns the a node's state (this must be a valid lvalue)</dd>
 * </dl>
 * It alse must defines the type IR which will be the type of the instructions emitted.
 * This definition are needed by the InsSelector class that will find them also in the class
 * emitted by the burg.</p>
 *
 * <p>For an example see inssel-ia32.cpp inssel-ia32.h generated from inssel-ia32.brg</p>
 * <p>For an esample of InsSelector invocation see x86-backend.cpp</p>
 *
 */

#include "mirnode.h"
#include "basicblock.h"
#include "cfg.h"
#include <iostream>

//#define _DEBUG_JIT_EMISSION

namespace jit {

	/*!
	 * \brief Class that runs the instruction selection algorithm
	 *
	 * Every backend uses burg to generate its instruction selection functions. Burg generates a bunch of
	 * function to label the node of a tree and to emit code associated with them. We need a general
	 * algorithm to call the exact sequence of function to complete the instruction selection.<br>
	 * Burgs put all the generated function in a class. The InsSelector template class calls this
	 * function in the right order.
	 * Now this class uses the visitors mechanism to traverse the source_cfg
	 */
	template<class S, class IR>
		class InsSelector : public CFG<MIRNode>::IGraphVisitor
		{
			public:

				typedef typename S::MBState MBState; //!<the state type
				//typedef typename S::MBTree MBTREE_TYPE;
				//typedef typename S::MBCostData MBCOST_DATA;
				typedef typename S::MBEmitFunc MBEmitFunc; //!<the emit function type

				typedef CFG<MIRNode> source_cfg_t; //!<the type of the source CFG
				typedef CFG<IR> target_cfg_t; //!<the type of the target CFG
				typedef typename CFG<MIRNode>::iterator_t node_iterator_t; //!< iterator on the basic block of the source CFG
				typedef typename BasicBlock<MIRNode>::IRStmtNodeIterator code_iterator_t; //!<iterator on the code of the source CFG
				typedef typename BasicBlock<IR>::IRStmtNodeIterator target_code_iterator_t;
				typedef typename CFG<MIRNode>::GraphNode GraphNode;
				typedef typename CFG<MIRNode>::GraphEdge GraphEdge;

				/*!
				 * \brief constructor
				 * \param selector The class created by burg
				 * \param source_cfg The source CFG of MIRNode
				 * \param target_cfg The target CFG of the backend instruction type
				 */
				InsSelector(S& selector, source_cfg_t& source_cfg, target_cfg_t& target_cfg, int goal = 0)
					: selector(selector), source_cfg(source_cfg), target_cfg(target_cfg), goal(goal) {}


				virtual ~InsSelector() {};

				//! the interface function to call to start the instruction selection process
				virtual void instruction_selection(int goal);

				int32_t ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge);
				virtual int32_t ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom);

			private:


				//! function which does the tree labeling and emission
				void reduce(MIRNode* tree, BasicBlock<IR>& bb, int goal);

				S& selector; //!<the selector provided by the backend
				source_cfg_t& source_cfg; //!<the intermediate CFG
				target_cfg_t& target_cfg; //!<the low level CFG to generate
				int goal; //!<goal of the instruction selection

		};

	/*!
	 * \param tree The node to reduce
	 * \param bb the basic block fo the target CFG to which add the instructions emitted
	 * \param goal the start non terminal of burg
	 *
	 * This recursive function does the tree labelling using the netvm_burg_label function.
	 * Then it calls the emission functions associated with the rules recognized.
	 */

	template<class S, class IR>
		void InsSelector<S, IR>::reduce(MIRNode* tree, BasicBlock<IR>& bb, int goal)
		{
			MIRNode *kids[10];
			int ern;
			uint16_t *nts;
			int i;//, n = 0;

			ern = selector.netvm_burg_rule((MBState *)tree->state, goal);
			#ifdef ENABLE_COMPILER_PROFILING
				//std::cout << "\t"<< selector.netvm_burg_rule_string[ern] << std::endl;
			#endif

			if(selector.error == INSSEL_ERROR_NTERM_NOT_FOUND)
				throw "NTERM not found";

			nts = (uint16_t*)selector.netvm_burg_nts[ern];
			selector.netvm_burg_kids(tree, ern, kids);

			if ( selector.error == INSSEL_ERROR_RULE_NOT_FOUND )
				throw "Rule not found";

			for(i = 0; nts[i]; i++)
			{
				reduce(kids[i], bb, nts[i]);
			}

			//n = (tree->getKid(0) != NULL) + (tree->getKid(1) != NULL);

			selector.netvm_burg_func[ern](target_cfg, bb, tree, NULL);
		}

	template<class S, class IR>
	int32_t InsSelector<S, IR>::ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge)
	{
		return 1;
	}

	template<class S, class IR>
	int32_t InsSelector<S, IR>::ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom)
	{
		BasicBlock<MIRNode>* bb = visitedNode.NodeInfo;
		BasicBlock<IR>* newbb = target_cfg.getBBById(bb->getId());
		assert(newbb && "newbb is null - did you remember to copy the old CFG into the new one?");
#ifdef _DEBUG_JIT_EMISSION
		std::cout << "L" << bb->getId() << std::endl;
		target_code_iterator_t start = newbb->codeBegin();
#endif
		for(code_iterator_t c = bb->codeBegin() ; c!= bb->codeEnd() ; c++)
		{
			selector.error = 0;
		#ifdef _DEBUG_JIT_EMISSION
			//std::cout << endl << "Selecting target code for tree: ";
			(*c)->printNode(cout, true);
			std::cout <<endl;
		#endif
			MBState *s = selector.netvm_burg_label(*c, NULL);

			if(selector.error == INSSEL_ERROR_ARITY)
				throw "arity error";

			if(s == NULL)
			{
				std::ostringstream msg;
				msg << "Cannot reduce tree: ";
				(*c)->printNode(msg, true);
				(*c)->printNode(std::cout, true);
				throw msg.str();
			}

#ifdef ENABLE_COMPILER_PROFILING
			//std::cout << "\nBURG STATEMENT: ";
			//(*c)->printSignature(std::cout) << std::endl;
#endif
			reduce(*c, *newbb, goal);

#ifdef _DEBUG_JIT_EMISSION
			/*
			if (start != newbb->codeBegin() && start !=newbb->codeEnd())
				start++;

			for (target_code_iterator_t ci = start; ci != newbb->codeEnd(); ci++)
			{
				cout << "*\t";
				(*ci)->printNode(cout, true);
				start = ci;
				cout <<endl;
			}
			*/
#endif
		}
#ifdef _DEBUG_JIT_EMISSION
		cout << "Target code: " <<endl;
			for (target_code_iterator_t ci = newbb->codeBegin(); ci != newbb->codeEnd(); ci++)
			{
				cout << "*\t";
				(*ci)->printNode(cout, true);
				start = ci;
				cout <<endl;
			}
		cout <<endl;
#endif
		return 1;
	}

	/*!
	 * \param goal the start non terminal
	 *
	 * This function initializes burg code. Then for every basic block in the source CFG, for every tree in
	 * the list of the basic block it calls reduce on that node.
	 */
	template<class S, class IR>
		void InsSelector<S, IR>::instruction_selection(int goal)
		{
			this->goal = goal;
			selector.netvm_burg_init();

			source_cfg.PreOrder(*source_cfg.getEntryNode(), *this);
		}
}
#endif
