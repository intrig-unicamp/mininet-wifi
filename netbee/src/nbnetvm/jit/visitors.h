/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef VISITORS_H
#define VISITORS_H

#include "digraph.h"

template<class IR>
class PrintVisitor : public DiGraph<IR>::IGraphVisitor
{

	typedef typename DiGraph<IR>::GraphNode GraphNode;
	typedef typename DiGraph<IR>::GraphEdge GraphEdge;

	private:
	std::ostream& os;

	public:

	PrintVisitor(std::ostream& os) : os(os)
	{}

	uint32_t ExecuteOnEdge(const GraphNode& visitedNode,
			const GraphEdge& edge,
			bool lastEdge)
	{
		os << "from " << edge.From.NodeInfo << ", to " << edge.To.NodeInfo << std::endl;
		return 1;
	}

	uint32_t ExecuteOnNodeFrom(const GraphNode& visitedNode,
			const GraphNode* comingFrom)
	{
		os << visitedNode.NodeInfo << std::endl;

		//std::list<GraphNode *>& succ = visitedNode.getSuccessors();

		//copy(succ.begin(), succ.end(), std::ostream_iterator<GraphNode *>(os, "\t"));

		return 1;
	}
};

template<class IR>
class PrintRecursive: public DiGraph<IR>::IGraphRecursiveVisitor
{
	private:
	int deepness;
	std::ostream& os;

	public:
	typedef typename DiGraph<IR>::GraphNode node_t;
	typedef typename DiGraph<IR>::IGraphRecursiveVisitor visitor_t;

	PrintRecursive(node_t& node, int deepness, std::ostream& os)
		: visitor_t(node), deepness(deepness), os(os) {}

	~PrintRecursive() {}

	visitor_t *newVisitor(node_t& node)
	{
		return new PrintRecursive(node, deepness + 1, os);
	}

	void action()
	{
		os << "Deepness: " << deepness << ", " << *visitor_t::node.NodeInfo << std::endl;
	}
	
};
#endif
