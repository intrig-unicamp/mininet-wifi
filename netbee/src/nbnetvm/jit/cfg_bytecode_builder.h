/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once


/*!
 * \file cfg_bytecode_builder.h
 * \brief declaration of class to build a CFG from a bytecode
 */
#include "../opcodes.h"
#include "cfg_builder.h"

namespace jit {
	/*!
	 * \brief class to build a CFG from a netil Bytecode
	 */
	template <class IR>
		class ByteCode2CFG : public CFGBuilder<IR>
	{
		public:
			/*!
			 * \brief constructor
			 * \param cfg the CFG to build
			 * \param BCInfo the analyzed bytecode from which extract the CFG
			 */
			ByteCode2CFG(CFG<IR>& cfg, nvmJitByteCodeInfo* BCInfo): CFGBuilder<IR>(cfg), BCInfo(BCInfo),
				handler(0) {}
				
			ByteCode2CFG(CFG<IR>& cfg, nvmJitByteCodeInfo* BCInfo, nvmPEHandler *peh): CFGBuilder<IR>(cfg),
				BCInfo(BCInfo),	handler(peh) {}

			//! the function that implements the building
			void buildCFG();

		private:

			//! add the edges introduced by an instruction
			void setInsnTargets(BasicBlock<IR> *from, InstructionInfo *BCInfoArray, int pc);
			//! add an edges from a basic block to a instruction
			void addTarget(BasicBlock<IR> *from, InstructionInfo *BCInfoArray, int targetpc);
			
			//! label the BB with its PEHandler structure pointer
			void labelBB(BasicBlock<IR> *bb);
			void labelBB(uint16_t bb);
			
			nvmJitByteCodeInfo* BCInfo; //!<The analyzed bytecode from which extract the CFG structure
			nvmPEHandler *handler;
	};

	/*!
	 * \param bb Basic block to label
	 */
	template<class IR>
		void ByteCode2CFG<IR>::labelBB(BasicBlock<IR> *bb)
		{
			bb->template setProperty<nvmPEHandler *>("handler", handler);
#ifdef _DEBUG_CFG_BUILD
			std::cout << "Setto la property \"handler\" al BB: " << bb->getId() << std::endl;
#endif
			return;
		}

	/*!
	 * \param bb Basic block to label
	 */		
	template<class IR>
		void ByteCode2CFG<IR>::labelBB(uint16_t bb)
		{
			BasicBlock<IR> *BB = this->cfg.getBBById(bb);
			labelBB(BB);
			return;
		}

	/*!
	 * \param from the basic block from which we are jumping
	 * \param BCInfoArray the array of analyzed netil instructions
	 * \param targetpc the program counter of the target instruction
	 */
	template<class IR>
		void ByteCode2CFG<IR>::addTarget(BasicBlock<IR> *from, InstructionInfo *BCInfoArray, int targetpc)
		{
			NETVM_ASSERT(from != NULL, __FUNCTION__"NULL basic block pointer");
			NETVM_ASSERT(BCInfoArray != NULL, __FUNCTION__"NULL BCInfoArray pointer");

			uint16_t target = BCInfoArray[targetpc].BasicBlockId;
			this->addEdge(from->getId(), target);
			labelBB(target);
			labelBB(from);
		}

	/*!
	 * \param from pointer to the basic block from which we are jumping
	 * \param BCInfoArray pointer to the array of the analyzed instruction
	 * \param pc program counter of the jump instruction
	 *
	 * Extracts from the InstructionInfo the target of the current instruction. <br>
	 * Adds the edge from this instruction BB to the target BB. <br> 
	 */
	template<class IR>
		void ByteCode2CFG<IR>::setInsnTargets(BasicBlock<IR> *from, InstructionInfo *BCInfoArray, int pc)
		{
			NETVM_ASSERT(from != NULL, __FUNCTION__"NULL basic block pointer");
			NETVM_ASSERT(BCInfoArray != NULL, __FUNCTION__"NULL BCInfoArray pointer");

			int targetpc;

			if (BCInfoArray[pc].Opcode == SWITCH)
			{
				targetpc = BCInfoArray[pc].SwInfo->DefTarget;
				addTarget(from, BCInfoArray, targetpc);

				for(uint32_t i = 0; i < BCInfoArray[pc].SwInfo->NumCases; i++)
				{
					targetpc = BCInfoArray[pc].SwInfo->CaseTargets[i];
					addTarget(from, BCInfoArray, targetpc);
				}
			} 
			else if (nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_RET_INSN))
			{
                          this->addEdge(from, this->cfg.getExitBB());
				labelBB(from);
			}
			else
			{
				switch (BCInfoArray[pc].NumSuccessors)
				{
					case JMP_OUT_EDGES:
						if (nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_BR_INSN))
							targetpc = BCInfoArray[pc].Arguments[0];//Branch target is the first argument of instruction
						else
							targetpc = pc + 1;                      //if instruction is not a branch the next node begins with next instruction

						addTarget(from, BCInfoArray, targetpc);
						break;

					case BRANCH_OUT_EDGES:
						targetpc = BCInfoArray[pc].Arguments[0];//Branch target is the first argument of instruction
						addTarget(from, BCInfoArray, targetpc);

						targetpc = pc + 1;                      //if instruction is not a branch the next node begins with next instruction
						addTarget(from, BCInfoArray, targetpc);
						break;

				}
			}

		}

	/*!
	 * For every instruction in the bytecode array if this instruction is flagged as a BBLeader 
	 * adds a new BasicBlock to CFG and sets is in order number.<br>
	 * For every instruction flagged as BBEnd adds the edges and targets from that instruction
	 */
	template<class IR>
		void ByteCode2CFG<IR>::buildCFG()
		{
			int pc;
			InstructionInfo *BCInfoArray = BCInfo->BCInfoArray;
			int numInsn = BCInfo->NumInstructions;
			uint16_t currentBBId = 0;
			
			// Start from where we left last time - which is 0, if this CFG is new.
			int inOrder = this->cfg.BBListSize()-2;
#ifdef _DEBUG_CFG_BUILD
			std::clog << "Initial inOrder: " << inOrder << '\n';
#endif

			this->cfg.getEntryBB()->setInOrder(inOrder);  
			int newBB(this->cfg.BBListSize()-2+1);
			
#ifdef _DEBUG_CFG_BUILD
			std::clog << "New basic block: " << newBB << '\n';
#endif
			
			handler->start_bb = newBB;

			this->addEdge(BasicBlock<IR>::ENTRY_BB, newBB);
			labelBB(newBB);

			for(pc = 0; pc<numInsn; pc++)
			{
				if (nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_BB_LEADER))
				{
					inOrder++;

					currentBBId = BCInfoArray[pc].BasicBlockId;
#ifdef _DEBUG_CFG_BUILD
					std::clog << "Adding basic block " << currentBBId << '\n';
#endif
					
					BasicBlock<IR> *BB = this->addBasicBlock(currentBBId);
					labelBB(BB);

					BB->setInOrder(inOrder);
					//cfg->inOrder.push_back(BB);
					BB->setStartpc(pc);
					//std::cout << "Leader New CFG" << std::endl << *cfg;
				}

				if(nvmFLAG_ISSET(BCInfoArray[pc].Flags, FLAG_BB_END))
				{
					BasicBlock<IR> *BB = this->cfg.getBBById(currentBBId);
					labelBB(BB);

					BB->setEndpc(pc);
					setInsnTargets(BB, BCInfoArray, pc);
					//std::cout << "End New CFG" << std::endl << *cfg;
				}
			}

			this->cfg.getExitBB()->setInOrder(++inOrder);
			//cfg->inOrder.push_back(cfg->exitNode);

		}
}

