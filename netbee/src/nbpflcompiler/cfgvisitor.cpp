/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "cfgvisitor.h"
#include "errors.h"
#include "dump.h"


int32 CFGVisitor::ExecuteOnEdge(const PFLCFG::GraphNode &visitedNode, const PFLCFG::GraphEdge &edge, bool lastEdge)
{
	m_Handler.VisitBBEdge(edge.From.NodeInfo, edge.To.NodeInfo);

	return 0;
}

int32 CFGVisitor::ExecuteOnNodeFrom(const PFLCFG::GraphNode &visitedNode, const PFLCFG::GraphNode *comingFrom)
{
	if (comingFrom != NULL)
		m_Handler.VisitBasicBlock(visitedNode.NodeInfo, comingFrom->NodeInfo);

	return 0;
}




void CFGVisitor::VisitPreOrder(PFLCFG &cfg)
{
	cfg.PreOrder(*cfg.getEntryBB()->getNode(), *this);
}


void CFGVisitor::VisitPostOrder(PFLCFG &cfg)
{
	cfg.PostOrder(*cfg.getEntryBB()->getNode(), *this);
}


void CFGVisitor::VisitRevPreOrder(PFLCFG &cfg)
{
	cfg.BackPreOrder(*cfg.getExitBB()->getNode(), *this);
}


void CFGVisitor::VisitRevPostOrder(PFLCFG &cfg)
{
	cfg.BackPostOrder(*cfg.getExitBB()->getNode(), *this);
}

