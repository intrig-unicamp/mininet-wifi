/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "encapgraph.h"
#include "../nbee/globals/debug.h"



EncapGraph::EncapGraph()
	:m_StartNode(0){}


EncapGraph::GraphNode &EncapGraph::GetStartProtoNode(void)
{
	////cerr << __func__ << endl;
	nbASSERT(m_StartNode, "The startproto node has not been set");
	return *m_StartNode;
}
	

void EncapGraph::SetStartProtoNode(SymbolProto *proto)
{
	////cerr << __func__ << endl;
	EncapGraph::GraphNode *node = GetNode(proto);
	nbASSERT(node, "node is NULL");
	
	SetStartProtoNode(node);
}



void EncapGraph::SetStartProtoNode(EncapGraph::GraphNode *node)
{
	////cerr << __func__ << endl;
	nbASSERT(node, "node cannot be NULL");
	m_StartNode = node;
}
 
 

EncapGraph::GraphNode &EncapGraph::AddNode(SymbolProto* &proto)
{
	////cerr << __func__ << " " << proto->Name << endl;
	
	EncapGraph::GraphNode *node = GetNode(proto);
	if (node)
		return (*node);
	
	////cerr << " inserting protocol node" << endl;
	node = &(ProtoGraph::AddNode(proto));
	m_ProtoGraphMap.insert(std::make_pair<uint32, EncapGraph::GraphNode*>(proto->ID, node));
	
	return *node;
}



EncapGraph::GraphNode *EncapGraph::GetNode(SymbolProto *proto)
{
	////cerr << __func__ << " " << proto->Name << endl;
	
	nbASSERT(proto, "proto cannot be NULL");
	
	ProtoGraphMap_t::iterator i = m_ProtoGraphMap.find(proto->ID);
	if (i == m_ProtoGraphMap.end())
	{
		////cerr << " proto not found" << endl;
		return NULL;
	}
	
	////cerr << "proto found " << (*i).second << endl;
	
	return (*i).second;
}


bool EncapGraph::RemoveUnsupportedNodes(void)
{
	bool result = false;
	EncapGraph::NodeIterator n = this->FirstNode();
	for (; n != this->LastNode(); n++)
	{
		if (!(*n)->NodeInfo->IsSupported)
			DeleteNode(**n);
	}

	RemoveUnconnectedNodes();

	return result;
}


void EncapGraph::RemoveUnconnectedNodes(void)
{	
	
	//cerr << __func__ << endl;
	bool change = true;
	//uint32 c = 0;
	
	//while some change takes place...
	while (change)
	{
		//cerr << "RUN " << c++ << endl;
		change = false;
		list<EncapGraph::GraphNode*> nodes2delete;
		
		//	iterate over all the nodes of the graph searching for those with no predecessors
		// 	in order to delete them from the graph
		EncapGraph::NodeIterator n = this->FirstNode();
		for (; n != this->LastNode(); n++)
		{
			//only the startnode can have no predecessors, so we skip it
			if (*n != m_StartNode)
			{
				list<EncapGraph::GraphNode*> &predecessors = (*n)->GetPredecessors();
				if (predecessors.empty())
				{
					nodes2delete.push_back(*n);
					change = true;
					//cerr << "removing node " << (*n)->NodeInfo->Name << endl;
				}
			}
		}
		//actually delete the nodes
		list<EncapGraph::GraphNode*>::iterator i = nodes2delete.begin();
		for (; i != nodes2delete.end(); i++)
		{
			//cerr << "actually deleting node " << (*i)->NodeInfo->Name << endl;
			DeleteNode(**i);
		}
		//cerr << change << endl;
	}

}


bool EncapGraph::IsConnected(void)
{
	bool foundStart = false;
	EncapGraph::NodeIterator n = this->FirstNode();
	for (; n != this->LastNode(); n++)
	{
		if ((*n) == m_StartNode)
			continue;
		
		this->SortRevPostorder(*(*n));
		
		EncapGraph::SortedIterator i = this->FirstNodeSorted();
	
		for (; i != this->LastNodeSorted(); i++)
		{
			if ((*i) == m_StartNode)
				foundStart = true;
		}
	}
	return foundStart;
}

#if 0
bool EncapGraph::SetLinkLayerProto(SymbolProto *proto)
{
	EncapGraph::GraphNode &startProtoNode = GetStartProtoNode();
	EncapGraph::GraphNode *protoNode = GetNode(proto);
	nbASSERT(protoNode, "no node associated to this protocol");
	
	list<EncapGraph::GraphNode*> &successors = startProtoNode.GetSuccessors();
	list<EncapGraph::GraphNode*>::iterator i = successors.begin();
	
	list<EncapGraph::GraphNode*> nodes2delete;
	for(i; i != successors.end(); i++)
	{
		if ((*i) != protoNode)
			nodes2delete.push_back(*i);
	}
	
	if (nodes2delete.size() < successors.size())
	{
		list<EncapGraph::GraphNode*>::iterator j = nodes2delete.begin();
		for(j; j != nodes2delete.end(); j++)
			this->DeleteNode(**j);
	}
		
}
#endif

