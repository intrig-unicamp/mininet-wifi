/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x86-regalloc.h"
#include "gc_regalloc.h"
#include <algorithm>
#include <iterator>

using namespace std;
using namespace jit;
using namespace ia32;

std::set<x86Instruction::RegType> jit::ia32::x86RegSpiller::spillRegisters(std::set<x86Instruction::RegType> &tospillRegs)
{
	typedef list<BasicBlock<IR>* >::iterator _BBIt;
	typedef list<IR*>::iterator code_it;
	typedef map<x86Instruction::RegType, int32_t> map_t;

#ifdef _DEBUG_SPILL
	{
	cout << "Prima di Spill " << endl;
	CodePrint<x86Instruction> codeprinter(_cfg);
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
			map<x86Instruction::RegType, x86Instruction::RegType *> help_map;
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
					RegType *newreg;// = new RegType(X86_NEW_SPILL_REG);
					if(help_map.count(*k) > 0)
						newreg = help_map[*k];
					else
					{
						newreg = new RegType(X86_NEW_SPILL_REG);
						help_map[*k] = newreg;
						newregisters.insert(*newreg);
					}
#ifdef _DEBUG_SPILL
					cout << "Spillo il registro usato: " << *k << " nuovo nome: " << *newreg << "displacement: " << offsets_map[*k] << endl;
#endif
					(*instruction)->rewrite_Reg(*k, newreg);	
					x86Instruction * newload = x86_Asm_New_Op(X86_MOV);
					x86_Asm_Append_RegOp(newload, *newreg, x86_DWORD);
					x86_Asm_Append_MemOp(newload, X86_MACH_REG(EBP), RegisterInstance(), -offsets_map[*k]*4, 0, x86_DWORD, (x86MemFlags) (BASE | DISPL) );
					x86_Asm_Append_Comment(newload,"register spilling");
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
						newreg = new RegType(X86_NEW_SPILL_REG);
						help_map[*k] = newreg;
						newregisters.insert(*newreg);
					}
#ifdef _DEBUG_SPILL
					cout << "Spillo il registro definito: " << *k << " nuovo nome: " << *newreg << "displacement: " << offsets_map[*k] << endl;
#endif
					(*instruction)->rewrite_Reg(*k, newreg);	
					x86Instruction * newStore = x86_Asm_New_Op(X86_MOV);
					x86_Asm_Append_MemOp(newStore, X86_MACH_REG(EBP), RegisterInstance(), - (offsets_map[*k]*4), 0, x86_DWORD, (x86MemFlags) (BASE | DISPL) );
					x86_Asm_Append_RegOp(newStore, *newreg, x86_DWORD);
					x86_Asm_Append_Comment(newStore,"register spilling");
					tmp = codelist.insert(++tmp, newStore);
				}
			}
		} // end loop on instruction
	}

#ifdef _DEBUG_SPILL
	{
	cout << "Dopo Spill " << endl;
	CodePrint<x86Instruction> codeprinter(_cfg);
	cout << codeprinter;
	}
#endif
	return newregisters;
}


bool x86RegSpiller::isSpilled(const RegType &reg)
{
	if(reg.get_model()->get_space() == SPILL_SPACE)
		return true;
	return false;
}

uint32_t x86RegSpiller::getTotalSpillCost() const
{
	assert(1 == 0 && "Not yet implemented");
	return 0;
}
