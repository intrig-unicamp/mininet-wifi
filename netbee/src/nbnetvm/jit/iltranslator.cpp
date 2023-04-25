/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file iltranslator.cpp
 * \brief This file contains the definition of the routines used to translate netil bytecode in MIRNode
 */
#include "iltranslator.h"
#include "cfg_printer.h"
#include "registers.h"
#include "application.h"
#include <algorithm>

using namespace std;

int32_t useJIT_flag;

namespace jit {

	std::ostream& operator<<(std::ostream& os, IRStack& irlist)
	{
		typedef IRStack::iterator iterator_t;
		for(iterator_t i = irlist.begin(); i!=irlist.end(); i++)
		{
			os << **i << std::endl;
		}
		return os;
	}

	/*!
	 * \param cfg the cfg to fill with the generated code
	 * \param BCInfo the analysed bytecode
	 * \param start basic block form which the visit starts
	 *
	 * This is the interface funtion to call to start the translation process.<br>
	 * The translation is done by a IGraphRecursiveVisitor. This function create the
	 * load.port instruction in the EntryBB and then start the visit.
	 * It is possible to specifiy the start basic block. The load.port instruction will be
	 * emitted in this first basic block
	 */
	void ILTranslator::translate(CFG<MIRNode>& cfg, nvmJitByteCodeInfo *BCInfo, uint16_t start, TargetOptions* options)
	{

		basic_block_t* entry = cfg.getBBById(start);
//		node_t* entryNode = cfg.getEntryNode();

		//basic_block_t* first = *entry->succBegin();
		//RegisterManager<MIRNode::RegType>& manager = cfg.manager;
		//for(uint32_t i = 0; i < 1 + BCInfo->MaxStackDepth + BCInfo->Segment->LocalsSize; i++)
		//	manager.addNewRegister(i);

		// FIXME: THIS IS NOT SUPPOSED TO BE A REGISTER!!!
		MIRNode *port = MIRNode::leafNode(LDPORT, entry->getId());
#if 0
		port->setDefReg(1);
		entry->addNewInstruction(port);
#endif

		// Now we need to transform the BB Id into a node pointer...
		std::list<node_t *>& successors (cfg.getEntryBB()->getSuccessors());
		std::list<node_t *>::iterator i;
		node_t *node(0);

		for(i = successors.begin(); i != successors.end(); ++i) {
			if((*i)->NodeInfo->getId() == start) {
				node = (*i);
				break;
			}
		}

		assert(node != 0);

		ILTranslator visitor(*node, BCInfo, options);
		visitor.irStack.push(port);
		visitor();

		return;
	}



	/*!
	 * \param cfg the cfg to fill with the generated code
	 * \param BCInfo the analysed bytecode
	 *
	 * This is the interface funtion to call to start the translation process.<br>
	 * The translation is done by a IGraphRecursiveVisitor. This function create the
	 * load.port instruction in the EntryBB and then start the visit.
	 */
	void ILTranslator::translate(CFG<MIRNode>& cfg, nvmJitByteCodeInfo* BCInfo, TargetOptions* options)
	{
		std::clog << "Started translating the CFG\n";

		basic_block_t* entry = cfg.getEntryBB();
		node_t* entryNode = cfg.getEntryNode();

		//basic_block_t* first = *entry->succBegin();
		//RegisterManager<MIRNode::RegType>& manager = cfg.manager;
		//for(uint32_t i = 0; i < 1 + BCInfo->MaxStackDepth + BCInfo->Segment->LocalsSize; i++)
		//	manager.addNewRegister(i);

		// FIXME: THIS IS NOT SUPPOSED TO BE A REGISTER!!!
		MIRNode *port = MIRNode::leafNode(LDPORT, entry->getId());
#if 0
		port->setDefReg(1);
		entry->addNewInstruction(port);
#endif

		ILTranslator visitor(*entryNode, BCInfo, options);
		visitor.irStack.push(port);
		visitor();
	}

	/*
	 * Action done on every BB. For every instruction in the bytecode which belongs to
	 * this BB we add this instruction to the BB's list. Finally we call
	 * StoreRegister()
	 */
	void ILTranslator::action()
	{
		basic_block_t *BB = node.NodeInfo;

#ifdef _DEBUG_CFG_BUILD
		std::clog << "Translating basic bloc " << BB->getId() << '\n';
#endif

		for(uint32_t pc = BB->getStartPc(); pc <= BB->getEndPc(); pc++)
		{
			New_Instruction(pc, *BB);
		}

		StoreRegisters(*BB);
	}

	//! a functor that trasform data left on the stack in STREG
	struct transform_in_streg
	{
		ILTranslator::basic_block_t& BB;//!<the basic block to transform
		uint32_t &stack_depth;

		//!constructor
		transform_in_streg(ILTranslator::basic_block_t& BB, uint32_t &sd)
			: BB(BB), stack_depth(sd) {}

		/*!
		 * \brief transform a node in store register if needed
		 * \param node a pointer to the node to exam
		 *
		 * If the node is an expression left on the stack we transfor it in a store STREG and
		 * also replace it in the BB list
		 */
		MIRNode* operator()(MIRNode *node)
		{
			if(node->isStmt())
				return node;

			MIRNode* result;

			result = MIRNode::unOp(STREG, node->belongsTo(), node);
//			result->setDefReg(node->getDefReg());
			result->setDefReg(RegisterInstance(0, stack_depth--, 0));

			//std::cout << "\ncodice prima di replace\n" << BBPrinter<MIRNode>(BB);
			replace(BB.codeBegin(), BB.codeEnd(), node, result);
			//std::cout << "\ncodice dopo replace\n" << BBPrinter<MIRNode>(BB);

			return result;
		}
	};

	//!returns true if node is an Expression
	static bool is_Exp(const MIRNode *node)
	{
		return !node->isStmt();
	}

	/*!
	 * \param BB the BB on which run the algorithm
	 *
	 * First calls transform_in_streg on every instruction left on the stack.
	 * Then removes the expression MIRNodes which are in the basic block list.
	 */
	void ILTranslator::StoreRegisters(basic_block_t& BB)
	{
		//std::cout <<  "\nbefore storeReg\n" << jit::BBPrinter<MIRNode>(BB);
		//std::cout << "\nstack before storeReg\n" << irStack;

		uint32_t stack_depth(irStack.size());

		transform(irStack.rbegin(), irStack.rend(), irStack.rbegin(), transform_in_streg(BB, stack_depth));
		BB.eraseCode(remove_if(BB.codeBegin(), BB.codeEnd(), is_Exp), BB.codeEnd());

		//std::cout <<  "\nafter storeReg\n" << jit::BBPrinter<MIRNode>(BB);
		//std::cout << "\nstack after storeReg\n" << irStack;

	}

	ILTranslator::IGraphRecursiveVisitor* ILTranslator::newVisitor(ILTranslator::node_t& next_node)
	{
		return new jit::ILTranslator(next_node, irStack, BCInfo, options);
	}

	/*
	 * \param pc the instruction offset in the bytecode
	 * \param BB the basic block of the new instruction
	 *
	 * Simply checks the type of instruction and calls the relative helper function.
	 */
	void ILTranslator::New_Instruction(uint32_t pc,
			basic_block_t& BB)
	{

		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;

		//	MIRNode *result;

		switch( nvmOpCodeTable[opcode].Consts & OPCODE_MASK )
		{
			case OP_STACK:
				stackOp(pc, BB);
				break;
			case OP_LOC:
				localOp(pc, BB);
				break;
			case OP_JUMP:
				jumpOp(pc, BB);
				break;
			case OP_COP:
				copOp(pc, BB);
				break;
			case OP_PKT:
				pktOp(pc, BB);
				break;
			case OP_PTNMTC:
				ptnnmtcOp(pc, BB);
				break;
			case OP_LOAD:
			case OP_STORE:

				if(useJIT_flag)
				{
					if (nvmFLAG_ISSET(options->Flags, nvmDO_BCHECK ) )
					{
						loadstoreOp(pc, BB);
					break;
					}
				
					defaultCase(pc, BB);

				} else {

					defaultCase(pc, BB);
				}

				break;
			default:
				switch(opcode)
				{
					case MCMP:
						mcmpOp(pc, BB);
					break;
					case SUB:
					case SUBSOV:
					case SUBUOV:
					case MOD:
					case SHL:
					case SHR:
					case USHR:
					case ROTL:
					case ROTR:
					case CMP:
					case JCMPLE:
					case JCMPGE:
					case JCMPL:
					case JCMPG:
						secondOpOnTop(pc, BB);
						break;
					default:
						defaultCase(pc, BB);
					break;
				}
				break;
		}

		if(Get_Instruction_Type(opcode) == OP_TYPE_STMT)
			StoreRegisters(BB);

	}


	void ILTranslator::secondOpOnTop(uint32_t pc, basic_block_t& BB)
	{
		//uint32_t;
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		MIRNode *result;
		uint8_t opcode = instInfo.Opcode;
		RegisterInstance defReg(RegisterInstance::invalid);

		NETVM_ASSERT(Get_Operators_Count(opcode) == TWO_OPS, "Operator should have 2 operands");

		MIRNode *op1, *op2;
		op2 = getOperand(irStack.pop(), BB);
		op1 = getOperand(irStack.pop(), BB);

		result = MIRNode::binOp(opcode, BB.getId(), op1, op2);

		switch( Get_Defined_Count(opcode) )
		{
			case NO_DEF:
				defReg = RegisterInstance::invalid;
				break;
			case ONE_DEF:
				if( Is_Memory_Operation(opcode) )
				{

					if ( (nvmOpCodeTable[opcode].Consts & OPCODE_MASK) == OP_LOAD )
					{
						defReg = RegisterInstance(0, irStack.size() + 1, 0);
					}
					else
					{
						defReg = RegisterInstance(RegisterModel::get_new(0));
					}
				}
				else
				{
					// XXX we always have the same old bugs...
					defReg = RegisterInstance(0, irStack.size() + 1, 0);
					// XXX and we never remember how they are supposed to be fixed...
	//					defReg = RegisterInstance(RegisterModel::get_new(0));
				}

				break;

			default:
				break;
		}

		//if(defReg != 0)
		//	result->setDefReg(defReg);
		result->setDefReg(defReg);

		if( Get_Instruction_Type(opcode) == OP_TYPE_STMT )
		{
			addNewInstruction(result, BB);
		}
		else
		{
			addNewInstruction(result, BB);
			irStack.push(result);
		}
		return;
	}

	void ILTranslator::mcmpOp(uint32_t pc, basic_block_t& BB)
	{
          // unused
          // InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		MIRNode *result, *kid1, *kid2, *op1, *op2, *mask, *mask_dup;

		op2 = getOperand(irStack.pop(), BB);
		op1 = getOperand(irStack.pop(), BB);
		mask = getOperand(irStack.pop(), BB);
		mask_dup = mask->copy();

		kid1 = MIRNode::binOp(AND, BB.getId(), op1, mask, op1->getDefReg());
		kid2 = MIRNode::binOp(AND, BB.getId(), op2, mask_dup, op2->getDefReg());
		RegisterInstance defReg(0, irStack.size() + 1, 0);
		result = MIRNode::binOp(CMP, BB.getId(), kid1, kid2, defReg);
		irStack.push(result);
		addNewInstruction(result, BB);

	}

	void ILTranslator::copOp(uint32_t pc, basic_block_t& BB)
	{

		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		uint32_t copro_reg(0);
		uint32_t copro_id(instInfo.Arguments[0]);

		switch(opcode) {
			case COPIN:
			case COPOUT:
				copro_reg = instInfo.Arguments[1];
				break;
			default:
				;
		}

		// Get the correct coprocessor state
		nvmCoprocessorState *cop_state(Application::getApp(BB).getCoprocessor(copro_id));

		switch(opcode) {
			case COPIN:
				/* Read a value from a coprocessor register */
				{
					LDRegNode *ld(new LDRegNode(BB.getId(),
						RegisterInstance(
							Application::getApp(BB).getCoprocessorRegSpace(),
							Application::getApp(BB).getCoprocessorBaseRegister(copro_id)+copro_reg
						)
					));
					irStack.push(ld);
					addNewInstruction(ld, BB);
				}
				break;

			case COPOUT:
				/* Write the current top of the stack value into the register */
				{
					RegisterInstance dst(Application::getApp(BB).getCoprocessorRegSpace(),
						Application::getApp(BB).getCoprocessorBaseRegister(copro_id)+copro_reg);

					MIRNode *streg(MIRNode::unOp(STREG, BB.getId(), getOperand(irStack.pop(), BB), dst));

					addNewInstruction(streg, BB);
				}
				break;

			case COPRUN:
				{
					CopMIRNode *cop(new CopMIRNode(opcode, BB.getId(), copro_id, instInfo.Arguments[1], cop_state));
					addNewInstruction(cop, BB);
				}
				break;

			case COPINIT:
				{
					RegisterInstance defReg(0, irStack.size() + 1, 0);
					CopMIRNode *cop(new CopMIRNode(opcode, BB.getId(), copro_id, instInfo.Arguments[1], cop_state));
					cop->setDefReg(defReg);
					cop->setcoproInitOffset(instInfo.Arguments[1]);
					irStack.push(cop);
					addNewInstruction(cop, BB);
				}
				break;

			case COPPKTOUT:
				{
					CopMIRNode *cop(new CopMIRNode(opcode, BB.getId(), copro_id, 0, cop_state));
					addNewInstruction(cop, BB);
				}
				break;

			default:
				;
		}


#if 0
		CopMIRNode *result = new CopMIRNode(opcode, BB.getId(), instInfo.Arguments[0]);

		switch (Get_Defined_Count(opcode))
		{
			case NO_DEF:
				break;

			case ONE_DEF:
				result->setDefReg(RegisterInstance(0, irStack.size() + 1, 0));
				break;

			default:
				break;
		}

		switch (Get_Operators_Count(opcode))
		{
			case NO_OPS:
				break;
			case ONE_OP:
				result->setKid(getOperand(irStack.pop(), BB), 0);
				break;
		}

		if (opcode == COPIN || opcode == COPOUT)
			result->setcoproReg(instInfo.Arguments[1]);

		if (opcode == COPRUN)
			result->setcoproOp(instInfo.Arguments[1]);

		if (opcode == COPINIT)
			result->setcoproInitOffset(instInfo.Arguments[1]);

		if (opcode == COPIN)
			irStack.push(result);

			addNewInstruction(result, BB);
#endif

		return;
	}

	void ILTranslator::pktOp(uint32_t pc, basic_block_t& BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		MIRNode *result;
		if( opcode == SNDPKT)
		{
			result = new SndPktNode(BB.getId(), instInfo.Arguments[0]);
			addNewInstruction(result, BB);
		}
		else
			if( Get_Operators_Count(opcode) == ONE_OP )
			{
//				result = MIRNode::leafNode(opcode, BB.getId(),
//					RegisterInstance(0, instInfo.Arguments[0], 0));
				std::cout << "gestisco una " << nvmOpCodeTable[opcode].CodeName << '\n';
				result = MIRNode::leafNode(opcode, BB.getId(),
					RegisterInstance(RegisterModel::get_new(0)));
				addNewInstruction(result, BB);
			}

		return;
	}

	void ILTranslator::stackOp(uint32_t pc, basic_block_t& BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		MIRNode *result;
		switch(nvmOpCodeTable[opcode].Consts & STK_MASK)
		{
			case STK_PUSH:
				result = new ConstNode(BB.getId(), instInfo.Arguments[0], RegisterInstance(0, irStack.size()+1, 0));
				//push
				irStack.push(result);
				//add newInstr
				addNewInstruction(result, BB);
				break;

			case STK_POP:
				{
					delete irStack.pop();
				}
				break;
			case STK_DUP:
				{
					MIRNode *tmp,*store;
					tmp = getOperand(irStack.pop(), BB);

//					std::cout << "Dup: dim. stack: " << irStack.size() << '\n';

					store = MIRNode::unOp(STREG, BB.getId(), tmp, RegisterInstance(0, irStack.size()+1, 0));
					addNewInstruction(store, BB);

					tmp = new LDRegNode(BB.getId(), RegisterInstance(0, irStack.size()+1, 0));

					irStack.push(tmp);
					addNewInstruction(tmp, BB);

					tmp = tmp->copy();
					irStack.push(tmp);
					addNewInstruction(tmp, BB);
				}
				break;
			default:
				defaultCase(pc, BB);
		}
	}

	/* LOC* instructions work on the register namespace 2 */
	void ILTranslator::localOp(uint32_t pc, basic_block_t& BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		MIRNode *result = NULL;
		switch(nvmOpCodeTable[opcode].Consts & LOC_MASK)
		{
			case LOC_LOCLD:
				{
					result = MIRNode::leafNode(LDREG, BB.getId(), MIRNode::RegType(2, instInfo.Arguments[0]));
					irStack.push(result);
					addNewInstruction(result, BB);
					break;
				}
			case LOC_LOCST:
				{
					MIRNode *t = irStack.pop();
//					cout << "Nodo per locstore: " << *t << '\n';

					t = getOperand(t, BB);
//					cout << "Nodo dopo getOperand: " << *t << '\n';

					result = MIRNode::unOp(STREG, BB.getId(), t, MIRNode::RegType(2, instInfo.Arguments[0]));
					addNewInstruction(result, BB);
					break;
				}
		}
		return;
	}

	void ILTranslator::jumpOp(uint32_t pc, basic_block_t& BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		MIRNode *result = NULL;

		switch ( nvmOpCodeTable[opcode].Consts & JMPTGT_MASK )
		{
			case JMPTGT_NO_TARGETS:
				result = jumpNoTargets(pc, BB);
				break;
			case JMPTGT_ONE_TARGET:
				result = jumpOneTarget(pc, BB);
				break;
			case JMPTGT_TWO_TARGETS:
				result = jumpTwoTargets(pc, BB);
				break;
			case JMPTGT_SWITCH:
				result = jumpSwitch(pc, BB);
		}

		addNewInstruction(result, BB);
	}

	void ILTranslator::ptnnmtcOp(uint32_t pc, basic_block_t &BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		JumpMIRNode *result;
		uint8_t opcode = instInfo.Opcode;
		MIRNode *length, *nop, *op1, *op2;

		length = getOperand(irStack.pop(), BB);
		op2 = getOperand(irStack.pop(), BB);
		op1 = getOperand(irStack.pop(), BB);

		// Boundcheck
		if( nvmFLAG_ISSET(options->Flags, nvmDO_BCHECK ) )
		{
			MIRNode *off, *dim, *check;
			dim = length->copy();
			off = op1->copy();
			check = MIRNode::binOp(PCHECK, BB.getId(), off, dim);
			addNewInstruction(check, BB);

			dim = length->copy();
			off = op2->copy();
			check = MIRNode::binOp(PCHECK, BB.getId(), off, dim);
			addNewInstruction(check, BB);
		}		// end boundcheck

		nop = MIRNode::binOp(NOP, BB.getId(), op1, op2);
		result = new JumpMIRNode(opcode, BB.getId(),
						BCInfo->BCInfoArray[instInfo.Arguments[0]].BasicBlockId,
						BCInfo->BCInfoArray[instInfo.Arguments[1]].BasicBlockId,
						MIRNode::RegType::invalid,
						length, nop);
		addNewInstruction(result, BB);
	}

	void ILTranslator::loadstoreOp(uint32_t pc, basic_block_t& BB)
	{

		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		MIRNode *result = NULL;
		MIRNode *check = NULL;
		MIRNode *op1 = NULL;
		MIRNode *op2 = NULL;
		MIRNode *offset_check = NULL;
		MIRNode *dim = NULL;
		uint8_t opcode = instInfo.Opcode;
		RegisterInstance defReg(RegisterInstance::invalid);
		RegisterInstance defReg2(RegisterInstance::invalid);

		switch( Get_Operators_Count(opcode) )
		{
			
			case ONE_OP:
				op1 = getOperand(irStack.pop(), BB);
				result = MIRNode::unOp(opcode, BB.getId(), op1 );
				break;
			case TWO_OPS:
				op1 = getOperand(irStack.pop(), BB);
				op2 = getOperand(irStack.pop(), BB);
				result = MIRNode::binOp(opcode, BB.getId(), op1, op2);
				break;
		}

		offset_check = op1->copy();

		if ( (nvmOpCodeTable[opcode].Consts & DIM_MASK) == DIM_8 )
		{
			dim = new ConstNode(BB.getId(), 1, RegisterInstance(0, irStack.size()+1, 0));
			check = type_Mem(opcode, BB, offset_check, dim);
		}
		else if ( (nvmOpCodeTable[opcode].Consts & DIM_MASK) == DIM_16 )
		{
			dim = new ConstNode(BB.getId(), 2, RegisterInstance(0, irStack.size()+1, 0));
			check = type_Mem(opcode, BB, offset_check, dim);
		}
		else 
		{
			dim = new ConstNode(BB.getId(), 4, RegisterInstance(0, irStack.size()+1, 0));
			check = type_Mem(opcode, BB, offset_check, dim);
		}

			result->setIsLinked(true);
			result->setLinked(check);
			check->setIsDepend(true);
			check->setDepend(result);
			addNewInstruction(check, BB);
	
			//std::cout << "nodo linkato " <<  (*(result->getLinked())) << "in " << BB.getId() << std::endl;
			//std::cout << "result " << (*result) << "  flag " <<  ((result->getIsNotValid())) << std::endl;

	

		switch( Get_Defined_Count(opcode) )
		{
			case NO_DEF:
				defReg = RegisterInstance::invalid;
				break;
			case ONE_DEF:
				defReg = RegisterInstance(0, irStack.size() + 1, 0);
				break;
			default:
				break;
		}

		//if(defReg != 0)
		//	result->setDefReg(defReg);
		result->setDefReg(defReg);


		if( Get_Instruction_Type(opcode) == OP_TYPE_STMT )
		{
			addNewInstruction(result, BB);
		}
		else
		{
			addNewInstruction(result, BB);
			irStack.push(result);
		}
		return;
	}

	MIRNode * ILTranslator::type_Mem( uint8_t opcode, basic_block_t& BB, MIRNode *offset_check, MIRNode *dim)
	{
		MIRNode *check = NULL;

		if ( (nvmOpCodeTable[opcode].Consts & MEM_MASK) == MEM_PACKET )
		{
			check = MIRNode::binOp(PCHECK, BB.getId(), offset_check, dim);
		}
		else if ( (nvmOpCodeTable[opcode].Consts & MEM_MASK) == MEM_INFO )
		{
			check = MIRNode::binOp(ICHECK, BB.getId(), offset_check, dim);
		}
		else if ( (nvmOpCodeTable[opcode].Consts & MEM_MASK) == MEM_DATA )
		{	
			check = MIRNode::binOp(DCHECK, BB.getId(), offset_check, dim);
		}
	
		return check;

	}

	void ILTranslator::defaultCase(uint32_t pc, basic_block_t& BB)
	{
		//uint32_t;
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		MIRNode *result = NULL;
		uint8_t opcode = instInfo.Opcode;
		RegisterInstance defReg(RegisterInstance::invalid);

		switch( Get_Operators_Count(opcode) )
		{
			case NO_OPS:
				result = MIRNode::unOp(opcode, BB.getId(), NULL);
				break;
			case ONE_OP:
				result = MIRNode::unOp(opcode, BB.getId(), getOperand(irStack.pop(), BB));
				break;
			case TWO_OPS:
				MIRNode *op1, *op2;
				op1 = getOperand(irStack.pop(), BB);
				op2 = getOperand(irStack.pop(), BB);
				result = MIRNode::binOp(opcode, BB.getId(), op1, op2);
				break;
		}

		switch( Get_Defined_Count(opcode) )
		{
			case NO_DEF:
				defReg = RegisterInstance::invalid;
				break;
			case ONE_DEF:
				if( Is_Memory_Operation(opcode) )
				{

					if ( (nvmOpCodeTable[opcode].Consts & OPCODE_MASK) == OP_LOAD )
					{
						defReg = RegisterInstance(0, irStack.size() + 1, 0);
					}
					else
					{
						defReg = RegisterInstance(RegisterModel::get_new(0));
					}
				}
				else
				{
					// XXX we always have the same old bugs...
					defReg = RegisterInstance(0, irStack.size() + 1, 0);
					// XXX and we never remember how they are supposed to be fixed...
//					defReg = RegisterInstance(RegisterModel::get_new(0));
				}

				break;

			default:
				break;
		}

		//if(defReg != 0)
		//	result->setDefReg(defReg);
		result->setDefReg(defReg);

		if( Get_Instruction_Type(opcode) == OP_TYPE_STMT )
		{
			addNewInstruction(result, BB);
		}
		else
		{
			addNewInstruction(result, BB);
			irStack.push(result);
		}
		return;
	}

	void ILTranslator::addNewInstruction(MIRNode *result, basic_block_t& BB)
	{
		assert(result != 0);
		BB.addNewInstruction(result);
		result->inlist = true;
	}

	MIRNode* ILTranslator::getOperand(MIRNode *node, basic_block_t& BB)
	{
		if(node->inlist)
		{
			node->inlist = false;
			BB.eraseCode(find(BB.codeBegin(), BB.codeEnd(),node));
		}

		if(node->getOpcode() == STREG)
		{
			MIRNode *newNode = new LDRegNode(BB.getId(), node->getDefReg());
			return newNode;
		}
		return node;
	}

	JumpMIRNode* ILTranslator::jumpNoTargets(uint32_t pc, basic_block_t& BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		JumpMIRNode *result = NULL;
		switch(Get_Operators_Count(opcode) )
		{
			case NO_OPS:
				result = new JumpMIRNode(opcode,BB.getId(), //BBId
						0, 0,
						MIRNode::RegType::invalid,
						NULL, NULL);
				break;
			case ONE_OP:
				result = new JumpMIRNode(opcode, BB.getId(),
						0, 0,
						MIRNode::RegType::invalid,
						getOperand(irStack.pop(), BB), NULL);
				break;
			case TWO_OPS:
				MIRNode *op1, *op2;
				op1 = getOperand(irStack.pop(), BB);
				op2 = getOperand(irStack.pop(), BB);
				result = new JumpMIRNode(opcode, BB.getId(),
						0, 0,
						MIRNode::RegType::invalid,
						op1, op2);
				break;
		}
		return result;
	}

	JumpMIRNode * ILTranslator::jumpOneTarget(uint32_t pc, basic_block_t& BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		JumpMIRNode *result = NULL;

		switch(Get_Operators_Count(opcode) )
		{
			case NO_OPS:
				result = new JumpMIRNode(opcode, BB.getId(), //BBId
						BCInfo->BCInfoArray[instInfo.Arguments[0]].BasicBlockId, 0,
						MIRNode::RegType::invalid,
						NULL, NULL); //jf
				break;
			case ONE_OP:
				result = new JumpMIRNode(opcode, BB.getId(),
						BCInfo->BCInfoArray[instInfo.Arguments[0]].BasicBlockId, 0,
						MIRNode::RegType::invalid,
						getOperand(irStack.pop(), BB), NULL);
				break;
			case TWO_OPS:
				MIRNode *op1, *op2;
				op2 = getOperand(irStack.pop(), BB);
				op1 = getOperand(irStack.pop(), BB);
				result = new JumpMIRNode(opcode, BB.getId(),
						BCInfo->BCInfoArray[instInfo.Arguments[0]].BasicBlockId, 0,
						MIRNode::RegType::invalid,
						op1, op2);
				break;
		}
		return result;
	}

	JumpMIRNode * ILTranslator::jumpTwoTargets(uint32_t pc, basic_block_t& BB)
	{
		InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		uint8_t opcode = instInfo.Opcode;
		JumpMIRNode *result = NULL;
		switch( Get_Operators_Count(opcode) )
		{
			case NO_OPS:
				result = new JumpMIRNode(opcode, BB.getId(),
						BCInfo->BCInfoArray[instInfo.Arguments[0]].BasicBlockId,
						BCInfo->BCInfoArray[instInfo.Arguments[1]].BasicBlockId,
						MIRNode::RegType::invalid,
						NULL, NULL);
				break;
			case ONE_OP:
				result = new JumpMIRNode(opcode, BB.getId(),
						BCInfo->BCInfoArray[instInfo.Arguments[0]].BasicBlockId,
						BCInfo->BCInfoArray[instInfo.Arguments[1]].BasicBlockId,
						MIRNode::RegType::invalid,
						getOperand(irStack.pop(), BB), NULL);
				break;
			case TWO_OPS:
				MIRNode *op1, *op2;
				op2 = getOperand(irStack.pop(), BB);
				op1 = getOperand(irStack.pop(), BB);
				result = new JumpMIRNode(opcode, BB.getId(),
						BCInfo->BCInfoArray[instInfo.Arguments[0]].BasicBlockId,
						BCInfo->BCInfoArray[instInfo.Arguments[1]].BasicBlockId,
						MIRNode::RegType::invalid,
						op1, op2);
				break;
		}
		return result;
	}


	SwitchMIRNode * ILTranslator::jumpSwitch(uint32_t pc, basic_block_t& BB)
	{
          // unused
          // InstructionInfo instInfo = BCInfo->BCInfoArray[pc];
		SwitchMIRNode *result;

		result = new SwitchMIRNode(BB.getId(), BCInfo->BCInfoArray[pc].SwInfo->NumCases, getOperand(irStack.pop(), BB));
		result->addDefaultTarget(BCInfo->BCInfoArray[BCInfo->BCInfoArray[pc].SwInfo->DefTarget].BasicBlockId);

		for(uint32_t i=0; i < BCInfo->BCInfoArray[pc].SwInfo->NumCases; i++)
		{
			result->addTarget(i,
					BCInfo->BCInfoArray[pc].SwInfo->Values[i],
					BCInfo->BCInfoArray[BCInfo->BCInfoArray[pc].SwInfo->CaseTargets[i]].BasicBlockId);
		}
		return result;
	}

}
