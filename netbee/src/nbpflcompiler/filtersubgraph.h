/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "defs.h"
#include "symbols.h"
#include "encapgraph.h"
#include "errors.h"
#include "digraph.h"
#include "statements.h"
#include <map>

using namespace std;




typedef list<EncapGraph::GraphNode*> NodeList_t;


/*!
	\brief This class represents a subgraph of the protocol encapsulation graph
*/



class FilterSubGraph
{
	typedef map<EncapGraph::GraphNode*, EncapGraph::GraphNode*> NodeMap_t;
	typedef std::map <EncapGraph::GraphNode*, CodeList*> ProtoCodeMap_t;

	EncapGraph					m_Graph;
	EncapGraph::GraphNode		*m_StartNode;
	NodeList_t					m_TargetNodes;

	NodeMap_t					m_NodeMap;
	ProtoCodeMap_t				m_ProtoCodeMap;
	EncapGraph::SubGraph		*m_Matrix;

	EncapGraph::GraphNode &AddNode(EncapGraph::GraphNode &origNode);

	void DeleteEdge(EncapGraph::GraphNode &from, EncapGraph::GraphNode &to);

	void DeleteNode(EncapGraph::GraphNode &node);

public:

	FilterSubGraph(EncapGraph &protograph, NodeList_t &targetNodes);

	~FilterSubGraph();

	EncapGraph::GraphNode	&GetStartNode(void)
	{
		return *m_StartNode;
	}


	NodeList_t	&GetTargetNodes(void)
	{
		return m_TargetNodes;
	}


	EncapGraph &GetGraph(void)
	{
		return m_Graph;
	}

	bool IsTargetNode(EncapGraph::GraphNode *node);


	EncapGraph::GraphNode *GetNodeMap(EncapGraph::GraphNode &origNode);

	bool HasEdge(EncapGraph::GraphNode &from, EncapGraph::GraphNode &to);

	bool HasNode(EncapGraph::GraphNode &node);

	bool RemoveUnsupportedNodes(void);

	void RemoveUnconnectedNodes(void);

	bool IsConnected(void);

	CodeList *NewProtoCodeList(EncapGraph::GraphNode &node);

};

