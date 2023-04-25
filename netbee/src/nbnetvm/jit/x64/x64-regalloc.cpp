/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x64-regalloc.h"
#include "gc_regalloc.h"
#include <algorithm>
#include <iterator>

//#define ENABLE_XMM_SPILLING 1

// If we use xmm spilling first 16 entry get spilled in xmm registers
#ifdef ENABLE_XMM_SPILLING
  #define SPILLING_OFFSET 16
#else
  #define SPILLING_OFFSET 0
#endif

using namespace std;
using namespace jit;
using namespace x64;

std::set<x64Instruction::RegType> jit::x64::x64RegSpiller::spillRegisters(std::set<x64Instruction::RegType> &tospillRegs)
{
	typedef list<BasicBlock<IR>* >::iterator _BBIt;
	typedef list<IR*>::iterator code_it;
	typedef map<x64Instruction::RegType, int32_t> map_t;

#ifdef _DEBUG_SPILL
	{
	cout << "Prima di Spill " << endl;
	CodePrint<x64Instruction> codeprinter(_cfg);
	cout << codeprinter;
	}
#endif
	map_t offsets_map;

	set_t newregisters;
	set_t intersection;
	code_it tmp;

	for(set_t::iterator i = tospillRegs.begin(); i != tospillRegs.end(); i++)
		offsets_map[*i] = ++numSpilledRegs;

	std::list<BasicBlock<IR>* > *BBlist;

	BBlist = _cfg.getBBList();		
	for(_BBIt i = BBlist->begin(); i != BBlist->end(); i++)
	{
//		cout << "BB: " << **i << endl;
		std::list<IR*> &codelist = (*i)->getCode();
		for(code_it instruction = codelist.begin(); instruction != codelist.end(); instruction++)
		{
			set_t usesregs = (*instruction)->getUses();
			set_t defregs = (*instruction)->getDefs();
			map<x64Instruction::RegType, x64Instruction::RegType *> help_map;
			// USED REGISTERS
			tmp = instruction;
			intersection.clear();
			set_intersection(usesregs.begin(), usesregs.end(),
					tospillRegs.begin(), tospillRegs.end(),
					insert_iterator<set_t>(intersection, intersection.begin()));
			if(intersection.size() > 0)
			{
				for(set_t::iterator k = intersection.begin(); k != intersection.end(); k++)
				{
					RegType *newreg;// = new RegType(X64_NEW_SPILL_REG);
					if(help_map.count(*k) > 0)
						newreg = help_map[*k];
					else
					{
						newreg = new RegType(X64_NEW_SPILL_REG);
						help_map[*k] = newreg;
						newregisters.insert(*newreg);
					}
#ifdef _DEBUG_SPILL
					cout << "Spillo il registro usato: " << *k << " nuovo nome: " << *newreg << "displacement: " << offsets_map[*k] << endl;
#endif
					(*instruction)->rewrite_Reg(*k, newreg);
					x64Instruction * newload;
					
					#ifdef ENABLE_XMM_SPILLING
					  if (offsets_map[*k] <= SPILLING_OFFSET){
					    newload = x64_Asm_New_Op(X64_MOVQ);
					    x64_Asm_Append_RegOp(newload, *newreg, x64_QWORD);
					    x64_Asm_Append_XMMOp(newload, (jit::RegisterInstance((uint32_t)MACH_SPACE, (uint32_t)(offsets_map[*k] - 1), 0)));
					    x64_Asm_Append_Comment(newload,"reg spilling: load from xmm reg");
					  }else{
					#endif
					    newload = x64_Asm_New_Op(X64_MOV);
					    x64_Asm_Append_RegOp(newload, *newreg, x64_QWORD);
					    x64_Asm_Append_MemOp(newload, X64_MACH_REG(RBP), RegisterInstance(), (SPILLING_OFFSET - offsets_map[*k])*8, 0, x64_QWORD, (x64MemFlags) (BASE | DISPL) );
					    x64_Asm_Append_Comment(newload,"reg spilling: load from mem");
					#ifdef ENABLE_XMM_SPILLING
					  }
					#endif
					tmp = codelist.insert(tmp, newload); // iterator pointing to the new instruction
				}
			}

			// DEFINED REGISTERS
			tmp = instruction;
			intersection.clear();
			set_intersection(defregs.begin(), defregs.end(), 
					tospillRegs.begin(), tospillRegs.end(), 
					insert_iterator<set_t>(intersection, intersection.begin()) );
			if(intersection.size() > 0)	// the node defines some registers that have to be spilled
			{
				for(set_t::iterator k = intersection.begin(); k != intersection.end(); k++)
				{
					RegType *newreg; 
					if( help_map.count(*k) > 0)
						newreg = help_map[*k];
					else
					{
						newreg = new RegType(X64_NEW_SPILL_REG);
						help_map[*k] = newreg;
						newregisters.insert(*newreg);
					}
#ifdef _DEBUG_SPILL
					cout << "Spillo il registro definito: " << *k << " nuovo nome: " << *newreg << "displacement: " << offsets_map[*k] << endl;
#endif
					(*instruction)->rewrite_Reg(*k, newreg);	
					x64Instruction * newStore;

					#ifdef ENABLE_XMM_SPILLING
					  if (offsets_map[*k] <= SPILLING_OFFSET){
					    newStore = x64_Asm_New_Op(X64_MOVQ);
					    x64_Asm_Append_XMMOp(newStore, (jit::RegisterInstance((uint32_t)MACH_SPACE, (uint32_t)(offsets_map[*k] - 1), 0)));
					    x64_Asm_Append_RegOp(newStore, *newreg, x64_QWORD);
					    x64_Asm_Append_Comment(newStore,"reg spilling: store in  xmm reg");
					  }else{
					#endif
					    newStore = x64_Asm_New_Op(X64_MOV);
					    x64_Asm_Append_MemOp(newStore, X64_MACH_REG(RBP), RegisterInstance(),  ((SPILLING_OFFSET - offsets_map[*k])*8), 0, x64_QWORD, (x64MemFlags) (BASE | DISPL) );
					    x64_Asm_Append_RegOp(newStore, *newreg, x64_QWORD);
					    x64_Asm_Append_Comment(newStore,"reg spilling: store in  mem");
					#ifdef ENABLE_XMM_SPILLING
					  }
					#endif
					
					tmp = codelist.insert(++tmp, newStore);
				}
			}
		} // end loop on instruction
	}

#ifdef _DEBUG_SPILL
	{
	cout << "Dopo Spill " << endl;
	CodePrint<x64Instruction> codeprinter(_cfg);
	cout << codeprinter;
	}
#endif
	return newregisters;
}


bool x64RegSpiller::isSpilled(const RegType &reg)
{
	if(reg.get_model()->get_space() == SPILL_SPACE)
		return true;
	return false;
}

uint32_t x64RegSpiller::getNumSpilledReg() const {
  	
    #ifdef ENABLE_XMM_SPILLING
	if (numSpilledRegs <= SPILLING_OFFSET){ 
	  #ifdef CODE_PROFILING
	    printf("Registred spilled: %d to xmm; 0 to stack\n", numSpilledRegs);
	  #endif
	  return 0;
	}else{
	  #ifdef CODE_PROFILING
	    printf("Registred spilled: %d to xmm; %d to stack\n", SPILLING_OFFSET, numSpilledRegs - SPILLING_OFFSET);
	  #endif
	  return numSpilledRegs - SPILLING_OFFSET;
	}
    #else
	#ifdef CODE_PROFILING
	    printf("Registred spilled: 0 to xmm; %d to stack\n", numSpilledRegs);
	#endif
	return numSpilledRegs;
    #endif
}

uint32_t x64RegSpiller::getTotalSpillCost() const
{
	assert(1 == 0 && "Not yet implemented");
	return 0;
}
