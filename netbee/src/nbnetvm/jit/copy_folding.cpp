/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "copy_folding.h"


			//}

			////if this is a jump we do not reassociate the register
			////unless nobody else uses it 
			//if(n->isJump())
			//{
			//	RegType comp_reg(kid->getDefReg());

			//	std::set<RegType> liveout(liveness.get_LiveOutSet(bb));

			//	if(liveout.find(comp_reg) != liveout.end())
			//		continue;
