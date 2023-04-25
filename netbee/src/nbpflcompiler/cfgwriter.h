/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "cfgvisitor.h"
#include <iostream>

using namespace std;




class CFGWriter: public ICFGVisitorHandler
{
private:
	ostream		&m_Stream;
	bool		DumpNodesPass;
	bool		GraphOnly;
	bool		DumpNetIL;
	PFLCFG			*m_CFG;

	uint32_t 	num_Statement;

	virtual void VisitBasicBlock(PFLBasicBlock *bb, PFLBasicBlock *comingFrom = 0);
	
	virtual void VisitBBEdge(PFLBasicBlock *from, PFLBasicBlock *to);

public:

	CFGWriter(ostream &stream)
		:m_Stream(stream), DumpNodesPass(0), GraphOnly(0), DumpNetIL(0), num_Statement(0) {}

	void DumpCFG(PFLCFG &cfg, bool graphOnly, bool netIL);

	
};

