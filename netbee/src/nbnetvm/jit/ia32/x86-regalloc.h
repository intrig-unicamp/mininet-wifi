/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef X86_REGALLOC_H
#define X86_REGALLOC_H
#include "gc_regalloc.h"
#include "x86-asm.h"

namespace jit{
	namespace ia32{


		class x86RegSpiller: public IRegSpiller<jit::CFG<x86Instruction> >
		{
			private:
				typedef x86Instruction IR;
				typedef IR::RegType RegType;
				typedef std::set<RegType> set_t;

				jit::CFG<IR> &_cfg;
				
				uint32_t numSpilledRegs;
			public:
				x86RegSpiller(jit::CFG<IR> &cfg): _cfg(cfg) { numSpilledRegs = 0; };

				std::set<x86Instruction::RegType> spillRegisters(set_t&);

				bool isSpilled(const RegType &);

				uint32_t getNumSpilledReg() const { 
				  #ifdef CODE_PROFILING
	    printf("Registred spilled: %d to stack\n", numSpilledRegs);
	#endif 
	return numSpilledRegs ; };

				uint32_t getTotalSpillCost() const;
		};

	}	/* IA32 */
}	/* JIT */
#endif
