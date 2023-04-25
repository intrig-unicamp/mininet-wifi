/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "protographshaper.h"
#include "protographwriter.h"
#include "errors.h"
#include <math.h>
#include <float.h>
#include <iostream>
#include <fstream>

using namespace std;


EncapGraph::GraphNode *ProtoGraphShaper::GetMaxPredecessor(EncapGraph::GraphNode &node)
{
	EncapGraph::GraphNode *predecessor = NULL;
	double maxLevel = 0;
	list<EncapGraph::GraphNode*> &predecessors = node.GetPredecessors();
	list<EncapGraph::GraphNode*>::iterator p = predecessors.begin();
	for (; p != predecessors.end(); p++)
	{
		if (!(*p)->Visited || *p == &node)
			continue;
		if ((*p)->NodeInfo->Level >= maxLevel)
		{
			maxLevel = (*p)->NodeInfo->Level;
			predecessor = *p;
		}
	}
	return predecessor;
}

EncapGraph::GraphNode *ProtoGraphShaper::GetMinSuccessor(EncapGraph::GraphNode &node)
{
	EncapGraph::GraphNode *successor = NULL;
	double minLevel = DBL_MAX;
	list<EncapGraph::GraphNode*> &successors = node.GetSuccessors();
	list<EncapGraph::GraphNode*>::iterator s = successors.begin();
	for (; s != successors.end(); s++)
	{
		if (m_Graph.HasEdge(**s, node))
			continue;

		if ((*s)->NodeInfo->Level < minLevel)
		{
			minLevel = (*s)->NodeInfo->Level;
			successor = *s;
		}
	}
	return successor;
}

void ProtoGraphShaper::DepthWalk(EncapGraph::GraphNode &node)
{

	if (node.Visited)
		return;

	node.Visited = true;

	double nodeLevel = node.NodeInfo->Level;

	//cout << "Visiting node " << node.NodeInfo->ToString() << " level: " << nodeLevel << endl;

	double nextLevel = DBL_MAX;
	EncapGraph::GraphNode *minSuccessor = GetMinSuccessor(node);
	if (minSuccessor)
		nextLevel = minSuccessor->NodeInfo->Level;

	//cout << "Minimum successors level: " << nextLevel << endl;

	if (nextLevel <= nodeLevel)
	{
		EncapGraph::GraphNode *maxPredecessor = GetMaxPredecessor(node);
		nbASSERT(maxPredecessor != NULL, "maxPredecessor should not be NULL");
		double prevLevel = maxPredecessor->NodeInfo->Level;
		//cout << "Max predecessors level: " << prevLevel << endl;
		if (prevLevel < nextLevel)
			node.NodeInfo->Level = prevLevel + ((nextLevel - prevLevel) / 2);
		//cout << "Rewriting level for node " << node.NodeInfo->ToString();
		//cout << ". Changing from " << nodeLevel << " to " << node.NodeInfo->Level << endl;
	}

	double level = ceil(node.NodeInfo->Level + 1);
	list<EncapGraph::GraphNode*> &successors = node.GetSuccessors();
	list<EncapGraph::GraphNode*>::iterator i = successors.begin();
	for (; i != successors.end(); i++)
	{
		if (level < (*i)->NodeInfo->Level)
		{
			//cout << "Assigning level " << level << " to node " << (*i)->NodeInfo->Name << endl;
			(*i)->NodeInfo->Level = level;
		}
	}
	list<EncapGraph::GraphNode*>::iterator ss = successors.begin();

	for (; ss != successors.end(); ss++)
		DepthWalk(**ss);

}

void ProtoGraphShaper::AssignLevels(void)
{
	EncapGraph &g = m_Graph.GetGraph();
	EncapGraph::NodeIterator i = g.FirstNode();
	for (; i != g.LastNode(); i++)
		(*i)->NodeInfo->Level = DBL_MAX; //set each level to infinite

	g.ResetVisited();
	EncapGraph::GraphNode &startNode = m_Graph.GetStartNode();
	startNode.NodeInfo->Level = 1;
	DepthWalk(startNode);

	m_LevelsOk = true;

}



void ProtoGraphShaper::RemoveAllTunnels(void)
{
	if (!m_LevelsOk)
		AssignLevels();

	//EncapGraph::GraphNode &targetNode = m_Graph.GetTargetNode();
	EncapGraph &g = m_Graph.GetGraph();

	EncapGraph::NodeIterator i = g.FirstNode();
	for (; i != g.LastNode(); i++)
	{
		EncapGraph::GraphNode *node = *i;
		list<EncapGraph::GraphEdge> edges;
		list<EncapGraph::GraphNode*> &successors = node->GetSuccessors();
		list<EncapGraph::GraphNode*>::iterator s = successors.begin();
		for(; s != successors.end(); s++)
		{
			if ((*s)->NodeInfo->Level <= node->NodeInfo->Level)
			{
				//cout << "edge " << node->NodeInfo->ToString() << " (level " << node->NodeInfo->Level << ") -> ";
				cout << (*s)->NodeInfo->ToString() << " (level " << (*s)->NodeInfo->Level << " is being deleted" << endl;
				EncapGraph::GraphEdge edge(*node, **s);
				edges.push_back(edge);
			}
		}
		list<EncapGraph::GraphEdge>::iterator e = edges.begin();
		for(; e != edges.end(); e++)
			g.DeleteEdge((*e).From, (*e).To);
	}

	m_Graph.RemoveUnconnectedNodes();
}


void ProtoGraphShaper::RemoveTargetTunnels(void)
{
	if (!m_LevelsOk)
		AssignLevels();

	NodeList_t &targetNodes = m_Graph.GetTargetNodes();
	EncapGraph &g = m_Graph.GetGraph();

	list<EncapGraph::GraphEdge> edges;

	NodeList_t::iterator t = targetNodes.begin();

	for (; t != targetNodes.end(); t++)
	{
		EncapGraph::GraphNode &targetNode = *(*t);
		list<EncapGraph::GraphNode*> &successors = targetNode.GetSuccessors();
		list<EncapGraph::GraphNode*>::iterator s = successors.begin();
		//Remove all the edges exiting from every target node
		for (; s != successors.end(); s++)
		{
			//cout << "edge " << targetNode.NodeInfo->ToString() << " (level " << targetNode.NodeInfo->Level << ") -> ";
			//cout << (*s)->NodeInfo->ToString() << " (level " << (*s)->NodeInfo->Level << " is being deleted" << endl;
			EncapGraph::GraphEdge edge(targetNode, **s);
			edges.push_back(edge);
		}

	}

	list<EncapGraph::GraphEdge>::iterator e = edges.begin();
	for(; e != edges.end(); e++)
		g.DeleteEdge((*e).From, (*e).To);

	NodeList_t::iterator k = targetNodes.begin();
	for (; k != targetNodes.end(); k++)
	{
		EncapGraph::GraphNode &targetNode = *(*k);
		EncapGraph::NodeIterator i = g.FirstNode();
		for (; i != g.LastNode(); i++)
		{
			list<EncapGraph::GraphNode*> nodes;

			if ((*i)->NodeInfo->Level >= targetNode.NodeInfo->Level && (*i) != &targetNode)
			{
				//cout << "node " << (*i)->NodeInfo->ToString() << " (level " << (*i)->NodeInfo->Level << " is being deleted" << endl;
				nodes.push_back(*i);
			}
			list<EncapGraph::GraphNode*>::iterator n = nodes.begin();
			for(; n != nodes.end(); n++)
				g.DeleteNode(**n);
		}
	}
	m_Graph.RemoveUnconnectedNodes();

}

