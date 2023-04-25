/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "protographwriter.h"
#include "errors.h"
#include "dump.h"



int32 ProtoGraphWriter::ExecuteOnEdge(const EncapGraph::GraphNode &visitedNode, const EncapGraph::GraphEdge &edge, bool lastEdge)
{
	if (m_SubGraph)
	{
		if (!m_SubGraph->HasEdge(edge.From, edge.To))
			return 0;
	}
	if (m_MaxLevel != 0 && (edge.From.NodeInfo->Level > m_MaxLevel || edge.To.NodeInfo->Level > m_MaxLevel))
		return 0;

	if (!m_NodesPass)
		m_Stream << "Node" << edge.From.NodeInfo->ID << "->Node" << edge.To.NodeInfo->ID << ";" << endl;
	return 0;
}

int32 ProtoGraphWriter::ExecuteOnNodeFrom(const EncapGraph::GraphNode &visitedNode, const EncapGraph::GraphNode *comingFrom)
{
	if (m_SubGraph)
	{
		if (!m_SubGraph->HasNode(visitedNode))
			return 0;
	}

	if (m_MaxLevel != 0 && visitedNode.NodeInfo->Level > m_MaxLevel)
		return 0;

	if (m_NodesPass)
	{
		m_Stream << "Node" << visitedNode.NodeInfo->ID << "[label=\"" << visitedNode.NodeInfo->Name;
		if (m_Levels)
			m_Stream << "\\n" << visitedNode.NodeInfo->Level;
		m_Stream << "\"];" << endl;
	}
	return 0;
}




void ProtoGraphWriter::DumpProtoGraph(EncapGraph &protograph, bool levels, uint32 maxlevel)
{
	m_MaxLevel = maxlevel;
	m_Levels = levels;
	m_StartNode = &protograph.GetStartProtoNode();
	m_Stream << "Digraph CFG {" << endl << endl;
	m_Stream << "// EDGES" << endl;
	m_NodesPass = false;
	protograph.PreOrder(*m_StartNode, *this);
	m_Stream << "// NODES" << endl;
	m_NodesPass = true;
	protograph.PreOrder(*m_StartNode, *this);
	m_Stream << "}" << endl;
}

void ProtoGraphWriter::DumpSubProtoGraph(EncapGraph &protograph, EncapGraph::SubGraph &subgraph, bool levels)
{
	m_SubGraph = &subgraph;
	// maxlevel has no meaning when dumping a subgraph
	DumpProtoGraph(protograph, levels, 0);
	m_SubGraph = NULL;
}

