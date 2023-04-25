/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

#include "gc_regalloc.h"
#include "x64-asm.h"

namespace jit{
	namespace x64{


		class x64RegSpiller: public IRegSpiller<jit::CFG<x64Instruction> >
		{
			private:
				typedef x64Instruction IR;
				typedef IR::RegType RegType;
				typedef std::set<RegType> set_t;

				jit::CFG<IR> &_cfg;
				
				uint32_t numSpilledRegs;
			public:
				x64RegSpiller(jit::CFG<IR> &cfg): _cfg(cfg) { numSpilledRegs = 0; };

				std::set<x64Instruction::RegType> spillRegisters(set_t&);

				bool isSpilled(const RegType &);

				uint32_t getNumSpilledReg() const;

				uint32_t getTotalSpillCost() const;
		};

	}	/* x64 */
}	/* JIT */

