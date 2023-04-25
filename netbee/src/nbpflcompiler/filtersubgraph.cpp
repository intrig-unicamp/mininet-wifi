/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "filtersubgraph.h"
#include "errors.h"
#include <algorithm>

EncapGraph::GraphNode &FilterSubGraph::AddNode(EncapGraph::GraphNode &origNode)
{
	NodeMap_t::iterator i = m_NodeMap.find(&origNode);
	if (i == m_NodeMap.end())
	{
		//cout << "Creating new node map for node: " << origNode.NodeInfo->ToString() << endl;
		EncapGraph::GraphNode *newNode = &m_Graph.AddNode(origNode.NodeInfo);
		pair<EncapGraph::GraphNode*, EncapGraph::GraphNode*> p = make_pair<EncapGraph::GraphNode*, EncapGraph::GraphNode*>(&origNode, newNode);
		m_NodeMap.insert(p);
		return *newNode;
	}

	//cout << "Node " << origNode.NodeInfo->ToString() << " already inserted" << endl;
	return *i->second;

}


EncapGraph::GraphNode *FilterSubGraph::GetNodeMap(EncapGraph::GraphNode &origNode)
{
	NodeMap_t::iterator i = m_NodeMap.find(&origNode);
	if (i == m_NodeMap.end())
		return NULL;
	return i->second;
}



FilterSubGraph::FilterSubGraph(EncapGraph &protograph, NodeList_t &targetNodes)
:m_StartNode(0), m_Matrix(0)
{
	protograph.SortRevPostorder(targetNodes);
	for (EncapGraph::SortedIterator i = protograph.FirstNodeSorted();
	     i != protograph.LastNodeSorted(); i++)
	{
		//cout << "Extracting node: " << (*i)->NodeInfo->ToString() << " from postorder list" << endl;
		EncapGraph::GraphNode &newNode = AddNode(**i);
		if ((*i) == &(protograph.GetStartProtoNode()))
		{
			m_StartNode = &newNode;
		}
		else if (find(targetNodes.begin(), targetNodes.end(), (*i)) != targetNodes.end())
		{
			m_TargetNodes.push_back(&newNode);
			m_TargetNodes.sort();
			m_TargetNodes.unique();
		}
		list<EncapGraph::GraphNode*> &predecessors = (*i)->GetPredecessors();
		for (list<EncapGraph::GraphNode*>::iterator p = predecessors.begin();
		     p != predecessors.end(); p++)
		{
			EncapGraph::GraphNode &newPredecessor = AddNode(**p);
			//cout << "Adding edge: " << newPredecessor.NodeInfo->ToString() << " -> " << newNode.NodeInfo->ToString() << endl;

			if ( newNode.NodeInfo->VerifySectionSupported )
			{
#ifdef VISIT_NEXT_PROTO_DEFINED_ONLY
				if ( newNode.NodeInfo->IsSupported && newNode.NodeInfo->IsDefined )
#endif
					m_Graph.AddEdge(newPredecessor, newNode);
			}
		}
	}

	if (m_StartNode == NULL || m_TargetNodes.size() == 0)
		throw ErrorInfo(ERR_PDL_ERROR, string("There is not a valid path between the startproto and all the target protocols"));

	m_Matrix = new EncapGraph::SubGraph(m_Graph);
	if (m_Matrix == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR,  "MEMORY ALLOCATION FAILURE");

	for (EncapGraph::NodeIterator k = m_Graph.FirstNode();
	     k != m_Graph.LastNode(); k++)
	{
		EncapGraph::GraphNode &node = **k;
		m_Matrix->AddNode(node);
		list<EncapGraph::GraphNode*> successors = node.GetSuccessors();
		for (list<EncapGraph::GraphNode*>::iterator s = successors.begin();
		     s != successors.end(); s++)
			m_Matrix->AddEdge(node, **s);
	}

}


FilterSubGraph::~FilterSubGraph()
{
	if (m_Matrix)
		delete(m_Matrix);

	//destroy all the code-lists associated to protocol nodes
	for (ProtoCodeMap_t::iterator i = m_ProtoCodeMap.begin();
	     i != m_ProtoCodeMap.end(); i++)
		delete i->second;
}

bool FilterSubGraph::HasEdge(EncapGraph::GraphNode &from, EncapGraph::GraphNode &to)
{

	return m_Matrix->HasEdge(from, to);
}

bool FilterSubGraph::HasNode(EncapGraph::GraphNode &node)
{
	return m_Matrix->HasNode(node);
}


void FilterSubGraph::DeleteEdge(EncapGraph::GraphNode &from, EncapGraph::GraphNode &to)
{
	m_Graph.DeleteEdge(from, to);
	m_Matrix->DeleteEdge(from, to);
}

void FilterSubGraph::DeleteNode(EncapGraph::GraphNode &node)
{
	m_Matrix->DeleteNode(node);
	m_Graph.DeleteNode(node);
}


bool FilterSubGraph::IsTargetNode(EncapGraph::GraphNode *node)
{
	return (find(m_TargetNodes.begin(), m_TargetNodes.end(), node) != m_TargetNodes.end());
}



CodeList *FilterSubGraph::NewProtoCodeList(EncapGraph::GraphNode &node)
{
	ProtoCodeMap_t::iterator i = m_ProtoCodeMap.find(&node);
	nbASSERT(i == m_ProtoCodeMap.end(), "the node already owns a code-list");

	CodeList *hirCode = new CodeList(OWNS_STATEMENTS);

	pair<EncapGraph::GraphNode*, CodeList*> p = make_pair<EncapGraph::GraphNode*, CodeList*>(&node, hirCode);
	m_ProtoCodeMap.insert(p);

	return hirCode;
}


bool FilterSubGraph::RemoveUnsupportedNodes(void)
{
	EncapGraph &subgraph = m_Graph;
	bool result = false;

	for (EncapGraph::NodeIterator n = subgraph.FirstNode(); n != subgraph.LastNode(); n++)
	{
		if (!(*n)->NodeInfo->IsSupported && !IsTargetNode(*n))
			DeleteNode(**n);
	}
	list<EncapGraph::GraphEdge> edges2delete;
	NodeList_t::iterator t = m_TargetNodes.begin();
	for (; t != m_TargetNodes.end(); t++)
	{
		EncapGraph::GraphNode &targetNode = *(*t);

		if (!targetNode.NodeInfo->IsSupported)
		{
			result = true;

			list<EncapGraph::GraphNode*> &successors = targetNode.GetSuccessors();
			for (list<EncapGraph::GraphNode*>::iterator i = successors.begin();
			     i != successors.end(); i++)
			{
				EncapGraph::GraphEdge e(targetNode, (**i));
				edges2delete.push_back(e);
			}
		}

	}
	for (list<EncapGraph::GraphEdge>::iterator e = edges2delete.begin();
	     e != edges2delete.end(); e++)
		DeleteEdge((*e).From, (*e).To);

	RemoveUnconnectedNodes();
	return result;
}


void FilterSubGraph::RemoveUnconnectedNodes(void)
{
	bool change = true;

	EncapGraph::NodeIterator n = m_Graph.FirstNode();
	while (change)
	{
		change = false;
		list<EncapGraph::GraphNode*> nodes2delete;
		for (; n != m_Graph.LastNode(); n++)
		{
			if (*n == m_StartNode || IsTargetNode(*n))
				continue;

			list<EncapGraph::GraphNode*> &successors = (*n)->GetSuccessors();
			list<EncapGraph::GraphNode*> &predecessors = (*n)->GetPredecessors();
			if (successors.empty() || predecessors.empty())
			{
				nodes2delete.push_back(*n);
				change = true;
			}
		}
		for (list<EncapGraph::GraphNode*>::iterator i = nodes2delete.begin();
		     i != nodes2delete.end(); i++)
			DeleteNode(**i);
	}

}


bool FilterSubGraph::IsConnected(void)
{
	bool foundStart = false;
	m_Graph.SortRevPostorder(m_TargetNodes);

	for (EncapGraph::SortedIterator i = m_Graph.FirstNodeSorted();
	     i != m_Graph.LastNodeSorted(); i++)
	{
		if ((*i) == m_StartNode)
			foundStart = true;
	}

	return foundStart;
}


