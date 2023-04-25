#ifndef _IR_NODE_REG_RENAMER_H
#define _IR_NODE_REG_RENAMER_H

#include "cfgvisitor.h"
#include "pflcfg.h"
#include "registers.h"


class IRNodeRegRenamer : public ICFGVisitorHandler
{
	
	public:
	void VisitBasicBlock(PFLBasicBlock *bb, PFLBasicBlock *comingFrom);
	void VisitBBEdge(PFLBasicBlock *from, PFLBasicBlock *to);
	

	IRNodeRegRenamer(PFLCFG &cfg);

	bool run();

	private:
		PFLCFG &_cfg;
		jit::RegisterInstance def_reg;
		void visitNode(MIRONode *node);
};

#endif
