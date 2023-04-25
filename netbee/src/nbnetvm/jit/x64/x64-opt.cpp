/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x64-opt.h"

#include "irnode.h"

#define IS_VALID_32BIT_CMP(j) (*j)->getOpcode() == X64_CMP && \
	(*j)->Operands[0].Type == MEM_OP && \
	(*j)->Operands[1].Type == IMM_OP && \
	(MEM_OPERAND((*j)->Operands[0].Op)->Flags & BASE && \
	!(MEM_OPERAND((*j)->Operands[0].Op)->Flags & INDEX))

/**
* This class contains optimizations applied after the instruction selection step.
*/

namespace jit{
	namespace x64{

		typedef CFG<x64Instruction>::IRType IR;
		typedef CFG<x64Instruction>::BBType BBType;
		typedef std::list<BBType* >::iterator _BBIt;
		typedef std::list<x64Instruction*>::iterator stmtIt;    

		/**
		* Optimize 2 consecutive basic blocks which compare consecutive 32bit values and jump to the same BB
		* we currently support only jne jumps
		* (ex ip.src == 10.0.0.1 && ip.dst == 10.0.0.2)
		* cmp            dword ptr[r1.4.0 + 0x1a], 0x1402a8c0
		* jne            lbl2                               
		* --------------
		* cmp            dword ptr[r1.4.0 + 0x1e], 0x102a8c0
		* jne            lbl2   
		*/
		void optimize_consecutive_32bit_compare(CFG<x64Instruction> &cfg){
			std::list<BBType* > *BBlist;
			BBlist = cfg.getBBList();
			bool foundFirst = false;
			x64RegOpnd base;
			uint32_t displ;
			uint64_t value;
			BBType* firstBB;

			for(_BBIt i = BBlist->begin(); i != BBlist->end(); i++)
			{
				std::list<x64Instruction*> &codelist = (*i)->getCode();

				stmtIt j = codelist.begin();
				if (j != codelist.end() && IS_VALID_32BIT_CMP(j))
				{
					if (!foundFirst)
					{
						base = MEM_OPERAND((*j)->Operands[0].Op)->Base;
						displ = MEM_OPERAND((*j)->Operands[0].Op)->Displ;
						value = IMM_OPERAND((*j)->Operands[1].Op)->Value;
						firstBB = (*i);
					}
					else
					{
						if (base == MEM_OPERAND((*j)->Operands[0].Op)->Base &&
							displ == MEM_OPERAND((*j)->Operands[0].Op)->Displ - 4)
						{
								value = value;
								value += (IMM_OPERAND((*j)->Operands[1].Op)->Value << 32);
								// if we'll found a jmp we found a valid block
						}
						else
						{
							base = MEM_OPERAND((*j)->Operands[0].Op)->Base;
							displ = MEM_OPERAND((*j)->Operands[0].Op)->Displ;
							value = IMM_OPERAND((*j)->Operands[1].Op)->Value;
							firstBB = (*i);
							foundFirst = false;
						}
					}
					j++;
					//FIXME: check for same destination BB
					if (j != codelist.end() && OP_ONLY((*j)->getOpcode()) == X64_J && CC_ONLY((*j)->getOpcode()) == NE)
					{
						if (foundFirst)
						{
							firstBB->getCode().clear();

							//64bit compare using mmu
							x64RegOpnd tmp(X64_NEW_VIRT_REG);
							x64_Asm_Op_Imm_To_Reg(firstBB->getCode(), X64_MOV, value, tmp, x64_QWORD);
							x64_Asm_Op_Reg_To_Mem_Base(firstBB->getCode(), X64_CMP, tmp, base, displ, x64_QWORD);
							x64_Asm_J_Label(firstBB->getCode(), CC_ONLY((*j)->getOpcode()), LBL_OPERAND((*j)->Operands[0].Op)->Label);

							for(stmtIt x = codelist.begin(); x != codelist.end();)
							{
								delete *x;
								x = codelist.erase(x);
							}

							//finding destionation if jump and remove from next BB
							std::list< CFG<x64Instruction>::GraphNode* > successors(firstBB->getSuccessors());

							uint16_t id = successors.back()->NodeInfo->getId();
							if (id != (*i)->getId()){
								(*i)->getSuccessors().remove(cfg.getBBById(id)->getNodePtr());
							}

							id = successors.front()->NodeInfo->getId();
							if (id != (*i)->getId())
							{
								(*i)->getSuccessors().remove(cfg.getBBById(id)->getNodePtr());
							}

							foundFirst = false;
						}
						else
						{
							foundFirst = true;
						}
					}
					else
					{
						foundFirst = false;
					}
				}
			}
			delete BBlist;
		}

		void x64Optimizations::run()
		{
			optimize_consecutive_32bit_compare(_cfg);
		}
	}
}