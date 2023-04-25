/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef IL_TRANSLATOR_H
#define IL_TRANSLATOR_H

/*!
 * \file iltranslator.h
 * \brief this file contains the definition of the class performing the translation from Netil to MIRNode
 */

#include "cfg.h"
#include "mirnode.h"
#include "bytecode_analyse.h"
#include "jit_internals.h"

#include <list>
#include <stack>
#include <algorithm>
#include <iostream>

extern int32_t useJIT_flag;

namespace jit {

	//! extends list to add push and pop operation, overrides copy constructor
	class IRStack : public std::list<MIRNode *>
	{
		public:

		//!constructor
		IRStack(): std::list<MIRNode*>() {}

		/*!
		 * \brief copy constructor
		 *
		 * calls copy on every MIRNode in the copied stack
		 */
		IRStack(const IRStack& stack) : std::list<MIRNode*> ()
		{
			for(const_iterator i = stack.begin(); i != stack.end(); i++)
				push_back((*i)->copy());
		}

		/*!
		 * \brief get the top of the stack
		 * \return a pointer to the MIRNode on the top of the stack
		 */
		MIRNode *top()
		{
			return back();
		}

		/*!
		 * \brief pops an element from the top of the stack
		 * \return a pointer to the element popped
		 */
		MIRNode *pop()
		{
			MIRNode *res = top();
			pop_back();
			return res;
		}

		/*!
		 * \brief push an element on the top of the stack
		 * \param element the element to push
		 */
		void push(MIRNode *element)
		{
			push_back(element);
		}

		friend std::ostream& operator<<(std::ostream& os, IRStack& irlist);
	};

	//! the class that performs the translation from netIL to intermediate representation
	class ILTranslator : public CFG<MIRNode>::IGraphRecursiveVisitor
	{

		public:

			typedef CFG<MIRNode>::GraphNode node_t; //!< type of the node of the graph
			typedef BasicBlock<MIRNode> basic_block_t; //!< type of the basic block
			//typedef std::list<MIRNode*> IRList;
			//typedef std::stack<MIRNode*, IRList> IRStack;

			//! Translate the bytecode into trees
			static void translate(CFG<MIRNode>& cfg, nvmJitByteCodeInfo* BCInfo, TargetOptions * options);
			static void translate(CFG<MIRNode>& cfg, nvmJitByteCodeInfo *BCInfo, uint16_t start, TargetOptions* options);

			//!constructor
			ILTranslator(node_t& node, IRStack& irList, nvmJitByteCodeInfo* BCInfo, TargetOptions * opt) :
				IGraphRecursiveVisitor(node),
				irStack(irList),
				BCInfo(BCInfo)
				{
					options = opt;
				}


			//!action performed on every basic block
			void action();

			//!create a new visitor and copy the stack for a new basic block
			IGraphRecursiveVisitor* newVisitor(node_t& next_node);

			//!destructor
			~ILTranslator() {}

		private:

			//!constructor
			ILTranslator(node_t& node, nvmJitByteCodeInfo* BCInfo, TargetOptions * opt) :
				IGraphRecursiveVisitor(node),
				BCInfo(BCInfo)
				{
					options = opt;				
				}

			//!create a new MIRNode from bytecode
			void New_Instruction(uint32_t pc, basic_block_t& BB);

			//!transform things left on the stack in STREG
			void StoreRegisters(basic_block_t& BB);

			//! handle MCMP
			void mcmpOp(uint32_t pc, basic_block_t& BB);
			//! handle check LOAD e STORE
			void loadstoreOp(uint32_t pc, basic_block_t& BB);
			//! handle a packet operation
			void pktOp(uint32_t pc, basic_block_t& BB);
			//! handle a stack operation
			void stackOp(uint32_t pc, basic_block_t& BB);
			//! handle a coprocessor operation
			void copOp(uint32_t pc, basic_block_t& BB);
			//! handle a local load or store operation
			void localOp(uint32_t pc, basic_block_t& BB);
			//!handle a jump operation
			void jumpOp(uint32_t pc, basic_block_t& BB);
			//!handle a pattern matching operation
			void ptnnmtcOp(uint32_t pc, basic_block_t& BB);
			//!handle operations where the second operand is on top of the stack (e.g. SUB, shifts, mem stores, etc.)
			void secondOpOnTop(uint32_t pc, basic_block_t& BB);
			//!handle every other type of operation
			void defaultCase(uint32_t pc, basic_block_t& BB);



			//! create a SWITCH MIRNode
			SwitchMIRNode * jumpSwitch(uint32_t pc, basic_block_t& BBId);

			//!
			MIRNode * type_Mem( uint8_t opcode, basic_block_t& BB, MIRNode *opcheck, MIRNode *dim);
			//! pop an operand from the current stack
			MIRNode * getOperand(MIRNode *node, basic_block_t& BB);
			//! add a new instruction to the current BB
			void addNewInstruction(MIRNode *result, basic_block_t& BB);
			//! create a Jump MIRNode
			JumpMIRNode * jumpNoTargets(uint32_t pc, basic_block_t& BBId);
			//! create a Jump MIRNode
			JumpMIRNode * jumpOneTarget(uint32_t pc, basic_block_t& BBId);
			//! create a Jump MIRNode
			JumpMIRNode * jumpTwoTargets(uint32_t pc, basic_block_t& BBId);

			IRStack irStack; //!<current stack

			TargetOptions * options;

			nvmJitByteCodeInfo* BCInfo; //!<the bytecode analyzed
	};

}
#endif
