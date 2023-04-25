/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "symbols.h"
#include "filtersubgraph.h"
#include <iostream>


#include <set>

using namespace std;


class ProtoGraphShaper
{

	FilterSubGraph	&m_Graph;

	bool			m_LevelsOk;

	EncapGraph::GraphNode *GetMaxPredecessor(EncapGraph::GraphNode &node);

	EncapGraph::GraphNode *GetMinSuccessor(EncapGraph::GraphNode &node);

	void DepthWalk(EncapGraph::GraphNode &node);

	void AssignLevels(void);



public:

	ProtoGraphShaper(FilterSubGraph &graph)
		:m_Graph(graph), m_LevelsOk(false){}

	void RemoveAllTunnels(void);

	void RemoveTargetTunnels(void);

};

