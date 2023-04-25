#ifndef _REASSOCIATION_FIXER
#define _REASSOCIATION_FIXER

#include "function_interface.h"
#include "cfgvisitor.h"

using namespace std;

class ReassociationFixer: public nvmJitFunctionI, public ICFGVisitorHandler
{
	public:
		bool run();	
		void VisitBasicBlock(PFLBasicBlock *bb, PFLBasicBlock *from);
		void VisitBBEdge(PFLBasicBlock *from, PFLBasicBlock *to);

		ReassociationFixer(PFLCFG &cfg);

	private:
		PFLCFG &_cfg;
};

#endif
