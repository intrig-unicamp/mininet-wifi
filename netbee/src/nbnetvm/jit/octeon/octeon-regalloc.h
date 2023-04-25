/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef OCTEON_REGALLOC_H
#define OCTEON_REGALLOC_H

#include "gc_regalloc.h"
#include "octeon-asm.h"

namespace jit {
namespace octeon{

	class octeonRegSpiller : public IRegSpiller<jit::CFG<octeon_Insn> >
	{
			private:
				typedef octeon_Insn IR;
				typedef IR::RegType RegType;
				typedef std::set<RegType> set_t;

				jit::CFG<IR>& _cfg;
				
				uint32_t numSpilledRegs;

				bool isSpilled(const RegType& reg);
				std::set<RegType> spillRegisters(std::set<RegType>& tospillRegs);

			public:
				octeonRegSpiller(jit::CFG<IR> &cfg);
				uint32_t getNumSpilledReg() const;

				uint32_t getTotalSpillCost() const;
	};

} //namespace octeon
}//namespace jit

#endif
