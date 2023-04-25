/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "octeon-regalloc.h"

using namespace jit;
using namespace octeon;
using namespace std;

octeonRegSpiller::octeonRegSpiller(jit::CFG<octeon_Insn>& cfg)
:
	_cfg(cfg),
	numSpilledRegs(0)
{} 

bool octeonRegSpiller::isSpilled(const RegType& reg)
{
	return reg.is_spill_register();
}

uint32_t octeonRegSpiller::getNumSpilledReg() const
{ 
	return numSpilledRegs; 
}

set< octeonRegSpiller::RegType > octeonRegSpiller::spillRegisters(set< RegType >& tospillRegs)
{
	set< RegType > res;
	return res;
}

uint32_t octeonRegSpiller::getTotalSpillCost() const
{
	assert(1 == 0 && "Not yet implemeted");
	return 0;
}
