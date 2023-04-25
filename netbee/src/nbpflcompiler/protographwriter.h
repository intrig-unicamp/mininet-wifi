/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "symbols.h"
#include "encapgraph.h"
#include <iostream>
using namespace std;


class ProtoGraphWriter: public EncapGraph::IGraphVisitor
{
private:
	ostream					&m_Stream;
	bool					m_NodesPass;
	uint32					m_MaxLevel;
	bool					m_Levels;
	EncapGraph::SubGraph	*m_SubGraph;
	EncapGraph::GraphNode *m_StartNode;

	virtual int32 ExecuteOnEdge(const EncapGraph::GraphNode &visitedNode, const EncapGraph::GraphEdge &edge, bool lastEdge);

	virtual int32 ExecuteOnNodeFrom(const EncapGraph::GraphNode &visitedNode, const EncapGraph::GraphNode *comingFrom);

public:

	ProtoGraphWriter(ostream &stream)
		:m_Stream(stream), m_NodesPass(0), m_MaxLevel(0), m_SubGraph(0), m_StartNode(0){}

	void DumpProtoGraph(EncapGraph &protograph, bool levels = false, uint32 maxlevel = 0);
	void DumpSubProtoGraph(EncapGraph &protograph, EncapGraph::SubGraph &subgraph, bool levels = false);
};

