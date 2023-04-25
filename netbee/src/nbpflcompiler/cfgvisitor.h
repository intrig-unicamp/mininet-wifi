/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "pflcfg.h"


class ICFGVisitorHandler
{
public:
	virtual void VisitBasicBlock(PFLBasicBlock *bb, PFLBasicBlock *comingFrom) = 0;
	virtual void VisitBBEdge(PFLBasicBlock *from, PFLBasicBlock *to) = 0;
};


class CFGVisitor: public PFLCFG::IGraphVisitor
{


	ICFGVisitorHandler	&m_Handler;
	
		
	virtual int32 ExecuteOnEdge(const PFLCFG::GraphNode &visitedNode, const PFLCFG::GraphEdge &edge, bool lastEdge);

	virtual int32 ExecuteOnNodeFrom(const PFLCFG::GraphNode &visitedNode, const PFLCFG::GraphNode *comingFrom);

public:

	CFGVisitor(ICFGVisitorHandler &visitorHandler)
		:m_Handler(visitorHandler){}
	
	void VisitPreOrder(PFLCFG &cfg);
	void VisitPostOrder(PFLCFG &cfg);
	void VisitRevPreOrder(PFLCFG &cfg);
	void VisitRevPostOrder(PFLCFG &cfg);
};

