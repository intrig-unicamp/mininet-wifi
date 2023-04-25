/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file digraph.h
 * \brief This file contains the definition of the template class di graph
 */

#pragma once


#include "bitmatrix.h"
#include <list>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <map>
/*!
  \brief This template class implements a generic directed graph, which nodes are objects of type T

*/

template<class T>
class DiGraph
{

	public:

		//forward declarations for friend clauses
		class GraphNode;
		class GraphEdge;
		class IGraphVisitor;
		class IGraphRecursiveVisitor;
		class SubGraph;
		class NodeIterator;
		class SortedIterator;

		friend class NodeIterator;
		friend class SortedIterator;
		friend class GraphNode;
		friend class GraphEdge;
		friend class IGraphVisitor;
		friend class IGraphRecursiveVisitor;
		friend class SubGraph;

		//! Iterator type for a list of GraphNode
		typedef typename std::list<GraphNode*>::iterator	list_iterator_t;

		typedef class IGraphVisitor IGraphVisitor;

		//! This nested class defines a graph node
		class GraphNode
		{
			friend class DiGraph;
			friend class NodeIterator;
			friend class SortedIterator;
			private:
				std::list<GraphNode*>	Predecessors;	//!<List of predecessor nodes
				std::list<GraphNode*>	Successors;	//!<List of successor nodes
				//bool			Valid;		//!<if false the node has been removed
			public:
				uint32_t		Id;		//!<Id of the node
				bool			Visited;	//!<Flag used by the Depth First Search methods and FindAllPaths mathod
				T			NodeInfo;	//!<Object of class T wrapped by the graph node
				bool			OnPath;		//!<Flag used by the FindAllPaths method //[icerrato]

			/*!
			  \brief Constructor
			  \param nodeInfo		reference to the object that must be associated to this node
			  \param id			id of the node (internally managed by the DiGraph class)
			  */
			GraphNode(T	&nodeInfo, uint32_t id)
				:Id(id), Visited(0), NodeInfo(nodeInfo){}

			/*!
			 * \brief Return a list with predecessors of this node in the graph
			 * \return list of predecessors
			 */
			std::list<GraphNode*> &GetPredecessors(void)
			{
				return Predecessors;
			}

			/*!
			 * \brief Return a list with successors of this node in the graph
			 * \return list of successors
			 */
			std::list<GraphNode*> &GetSuccessors(void)
			{
				return Successors;
			}

		};

		struct NodeLess
		{
			/*!
			 * \brief compare function
			 * \param x reference to first node
			 * \param y reference to second node
			 * \return true if x comes before y ???
			 */
			bool operator()(const GraphNode *x, const GraphNode *y)
			{
				if (x->Id >= y->Id)
					return false;
				if (x->Id >= y->Id)
					return false;
				return true;
			}
		};

		struct NodeEqu
		{
			/*!
			 * \brief compare function
			 * \param x reference to first node
			 * \param y reference to second node
			 * \return true if x comes before y ???
			 */
			bool operator()(const GraphNode *x, const GraphNode *y)
			{
				if (x->Id != y->Id)
					return false;

				return true;
			}
		};
		/*!
		  \brief This nested class defines a directed graph edge
		  */
		class GraphEdge
		{
			friend class DiGraph;
			public:
			GraphNode	&From;	//!<Graph node the edge comes from
			GraphNode	&To;	//!<Graph node the edge points to

			/*!
			  \brief Constructor
			  \param from		reference to the graph node the edge comes from
			  \param to		reference to the graph node the edge points to
			  */
			GraphEdge(GraphNode &from, GraphNode &to)
				:From(from), To(to){}
		};

		/*!
		 * \brief nested struct to compare edges
		 */
		struct EdgeComp
		{
			/*!
			 * \brief compare function
			 * \param x reference to first edge
			 * \param y reference to second edge
			 * \return true if x comes before y ???  
			 */
			bool operator()(const GraphEdge &x, const GraphEdge &y)
			{				
				if (x.From.Id >= y.From.Id)
					return false;
				if (x.To.Id >= y.To.Id)
					return false;
				return true;
				
				/*if((x.From.Id==y.From.Id)&&(x.To.Id==y.To.Id))
					return true;
				return false;*/
			}
		};

		class SubGraph
		{
			friend class DiGraph;
			private:
			DiGraph			*m_Graph;
			nbBitMatrix		m_AdjMatrix;
			nbBitVector		m_Nodes;
			uint32_t		m_NumNodes;

			public:
			SubGraph(DiGraph &graph)
				:m_Graph(&graph), m_AdjMatrix(m_Graph->m_NodeCount, m_Graph->m_NodeCount),
				m_Nodes(m_Graph->m_NodeCount), m_NumNodes(m_Graph->m_NodeCount){}


			uint32_t CountSuccessors(const GraphNode &node)
			{
				nbASSERT(OwnsNode(node), "The node does not belong to the subgraph!");
				uint32_t n = 0;
				for (uint32_t i = 0; i < m_AdjMatrix.GetNCols(); i++)
				{
					if (m_AdjMatrix(node.Id, i))
						n++;
				}
				return n;
			}

			uint32_t CountPredecessors(const GraphNode &node)
			{
				nbASSERT(OwnsNode(node), "The node does not belong to the subgraph!");
				uint32_t n = 0;
				for (uint32_t i = 0; i < m_AdjMatrix.GetNCols(); i++)
				{
					if (m_AdjMatrix(i, node.Id))
						n++;
				}
				return n;
			}


			void AddNode(const GraphNode &node)
			{
				m_Nodes[node.Id] = true;
			}

			void DeleteNode(const GraphNode &node)
			{
				m_Nodes[node.Id] = false;
				for (uint32_t i = 0; i < m_NumNodes; i++)
					m_AdjMatrix(i, node.Id).Clear();
				for (uint32_t j = 0; j < m_NumNodes; j++)
					m_AdjMatrix(node.Id, j).Clear();
			}

			void AddEdge(const GraphNode &from, const GraphNode &to)
			{
				AddNode(from);
				AddNode(to);
				m_AdjMatrix(from.Id, to.Id).Set();
			}

			void DeleteEdge(const GraphNode &from, const GraphNode &to)
			{
				m_AdjMatrix(from.Id, to.Id).Clear();
			}

			bool HasNode(const GraphNode &node)
			{
				return m_Nodes[node.Id];
			}


			bool HasEdge(const GraphNode &from, const GraphNode &to)
			{
				return m_AdjMatrix(from.Id, to.Id);
			}

			string ToString(void)
			{
				return m_AdjMatrix.ToString();
			}

		};

		/*!
		 * \brief class that recursively visits the graph
		 *
		 * You should inherit this class and implement the method action which performs the action on the nodes
		 * and the method newVisitor to create a new visitor for the child node and passing state in the recursive
		 * visit
		 */
		class IGraphRecursiveVisitor
		{
			protected:
				GraphNode& node; //!< the node we are on

			public:

				/*!\brief constructor
				 * \param node The node we are on
				 */
				IGraphRecursiveVisitor(GraphNode& node): node(node) {}
				/*!\brief function with the action to do on the node*/
				virtual void action() = 0;
				//!\brief function called when we are going to visit a new node
				virtual IGraphRecursiveVisitor* newVisitor(GraphNode &node) = 0;
				//!\brief desctructor
				virtual ~IGraphRecursiveVisitor() {};

				/**
				 * \brief function that implements recursive visit
				 *
				 * if we have not already visited this node, first call action() then
				 * we call visit on every successors of ours
				 */
				void operator()(void)
				{
					typedef typename std::list<GraphNode *>::iterator iterator_t;

					if(!node.Visited){
						action();
						node.Visited = true;
						for (iterator_t i = node.GetSuccessors().begin(); i!=node.GetSuccessors().end(); i++)
						{
							visit(*i);
						}
					}
				}

			private:
				/**
				 *  \brief called before visiting a new node
				 *  \param node The node we are going to visit
				 *
				 *  Call newVisitor with the new node, then call the new visitor operator() and deletes the new visitor
				 */
				void visit(GraphNode *node)
				{
					IGraphRecursiveVisitor *rv;
					rv = newVisitor(*node);
					(*rv)();
					delete rv;
				}
		};

		/*!
		  \brief This abstract class defines the interface for a generic graph visitor.
		  Its member function ExecuteOnEdge is called by the DFS (depth first search) methods when an
		  edge of the graph is traversed forward or backward.
		  */

		class IGraphVisitor
		{
			friend class DiGraph;
			public:

			virtual ~IGraphVisitor() {}


			/*!
			  \brief this function is called by the DFS (depth first search) methods when an
			  edge of the graph is traversed forward or backward.

			  \param	visitedNode	reference to the currently visited graph node
			  \param	edge			reference to the graph edge being traversed
			  \param 	lastEdge  true if this is the last edge for this node
			  \return nothing
			  */

			virtual int32_t ExecuteOnEdge(const GraphNode &visitedNode, const GraphEdge &edge, bool lastEdge) = 0;

			/*!
			  \brief this function is called by the DFS (depth first search) methods when an
			  edge of the graph is traversed forward or backward.

			  \param	visitedNode	reference to the currently visited graph node
			  \return nothing
			  */

			//virtual uint32_t ExecuteOnNodeFrom(const GraphNode &visitedNode) = 0;


			/*!
			  \brief this function is called by the DFS (depth first search) methods when an
			  edge of the graph is traversed forward or backward.

			  \param	visitedNode	reference to the currently visited graph node

			  \param	comingFrom reference to the node visited just before the current one

			  \return nothing
			  */

			virtual int32_t ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom) = 0;


		};


		/*!
		 * \brief This class implements an Iterator on graph nodes
		 *
		 * You can optain such an iterator by calling FirstNode() or LastNode() on a digraph.
		 * Then you can use them with standard algorithm to iterate on nodes in a casual order
		 */
		class NodeIterator
		{
			DiGraph<T>& _dg; //!< Graph on which we iterate
			uint32_t		_i; //!< index in vector of nodes

		public:

			typedef typename DiGraph<T>::GraphNode* value_type;  //!< type of the value of this iterator
			typedef ptrdiff_t difference_type; 					 //!< type of the difference between to elements
			typedef typename DiGraph<T>::GraphNode** pointer; 	 //!< type of a pointer to the value
			typedef typename DiGraph<T>::GraphNode*& reference;  //!< type of a refernce to a value
			typedef std::forward_iterator_tag iterator_category; //!< category of this iterator

			/*!
			 * \brief contructor of a NodeIterator
			 */
			NodeIterator(DiGraph<T> &graph)
				:_dg(graph), _i(0)
			{
				//scan for the first valid node
				while (_dg.m_NodeList[_i] == NULL && _i < _dg.m_NodeList.size())
					_i++;
				//printf("New NodeIterator: %u\n", _i);

			}

			/*!
			 * \brief copy contructor
			 */
			NodeIterator(const NodeIterator &i)
				:_dg(i._dg), _i(i._i) {}

			/// To create the "end sentinel" NodeIterator:
			NodeIterator(DiGraph<T>& graph, bool)
				:_dg(graph), _i(_dg.m_NodeList.size())
			{
				//printf("New NodeIterator sentinel: %u\n", _i);
			}


			GraphNode*& current() const
			{
				//printf("NodeIterator Current: %u\n", _i);
				assert(_dg.m_NodeList[_i] != NULL);
				return _dg.m_NodeList[_i];
			}

			GraphNode*& operator*() const
			{
				//printf("NodeIterator Dereference: %u\n", _i);
				assert(_dg.m_NodeList[_i] != NULL);
				return _dg.m_NodeList[_i];
			}

			NodeIterator& operator++()
			{ // Prefix
				//printf("++NodeIterator: %u", _i);
				assert(_i < _dg.m_NodeList.size() && "NodeIterator moved out of range");
				_i++;
				if (_i == _dg.m_NodeList.size())
					return *this;
				//scan for the first valid node
				while (_i < _dg.m_NodeList.size())
				{
					if (_dg.m_NodeList[_i] == NULL)
						_i++;
					else
						break;

				}
				assert(_i <= _dg.m_NodeList.size() && "NodeIterator moved out of range");
				//assert(_dg.m_NodeList[_i] != NULL);
				//printf("-> %u - size: %u\n", _i, _dg.m_NodeList.size());
				return *this;
			}

			NodeIterator& operator++(int)
			{ // Postfix
				//printf("++NodeIterator: %u", _i);
				assert(_i < _dg.m_NodeList.size() && "NodeIterator moved out of range");
				_i++;
				if (_i == _dg.m_NodeList.size())
					return *this;
				//scan for the first valid node
				while (_i < _dg.m_NodeList.size())
				{
					if (_dg.m_NodeList[_i] == NULL)
						_i++;
					else
						break;

				}
				assert(_i <= _dg.m_NodeList.size() && "NodeIterator moved out of range");
				//assert(_dg.m_NodeList[_i] != NULL);
				//printf("-> %u - size: %u\n", _i, _dg.m_NodeList.size());
				return *this;
			}
#if 0
			NodeIterator& operator+=(uint32_t amount)
			{
				assert((_i + amount) < _dg.m_NodeList.size() && "NodeIterator moved out of range");
				_i += amount;
				return *this;
			}
#endif

			// To see if you're at the end:
			bool operator==(const NodeIterator& rv) const
			{
				//printf("NodeIterator %u == %u\n", _i, rv._i);
				return _i == rv._i;
			}

			bool operator!=(const NodeIterator& rv) const
			{
				//printf("NodeIterator %u != %u\n", _i, rv._i);
				return _i != rv._i;
			}

		};


		/*!
		 * \brief This class implements an Iterator on graph nodes to iterate in a sorted way
		 *
		 * You should first call a function like Sort<something> to sort the digraph.
		 * You can get such an iterator by calling FirstNodeSorted() or LastNodeSorted().
		 * Then you can use it to iterate on a digraph with an order.
		 */

		class SortedIterator
		{
			DiGraph<T>& _dg;
			uint32_t		_i;
			public:
				typedef typename DiGraph<T>::GraphNode* value_type;
				typedef ptrdiff_t difference_type;
				typedef typename DiGraph<T>::GraphNode** pointer;
				typedef typename DiGraph<T>::GraphNode*& reference;
				typedef std::forward_iterator_tag iterator_category;

			/*!
			 * \brief constructor
			 */
			SortedIterator(DiGraph<T> &graph)
				:_dg(graph), _i(0) {}

			/*!
			 * \brief copy constructor
			 */
			SortedIterator(const SortedIterator &i)
				:_dg(i._dg), _i(i._i) {}

			// To create the "end sentinel" NodeIterator:
			SortedIterator(DiGraph<T>& graph, bool)
				:_dg(graph), _i(_dg.m_SortedNodes.size()) {}


			GraphNode*& operator*() const
			{
				return _dg.m_SortedNodes[_i];
			}

			SortedIterator& operator++()
			{ // Prefix
				assert(_i < _dg.m_SortedNodes.size() && "SortedIterator moved out of range");
				_i++;
				return *this;
			}

			SortedIterator& operator++(int)
			{ // Postfix
				assert(_i < _dg.m_SortedNodes.size() && "SortedIterator moved out of range");
				_i++;
				return *this;
			}

			SortedIterator& operator+=(uint32_t amount)
			{
				assert((_i + amount) < _dg.m_SortedNodes.size() && "SortedIterator moved out of range");
				_i += amount;
				return *this;
			}

			// To see if you're at the end:
			bool operator==(const SortedIterator& rv) const
			{
				return _i == rv._i;
			}

			bool operator!=(const SortedIterator& rv) const
			{
				return _i != rv._i;
			}

		};
		
		/*!

		 * \brief This class implements an Iterator on graph nodes to iterate in a sorted way
		 *
		 * You should first call a function like Sort<something> to sort the digraph.
		 * You can get such an iterator by calling FirstNodeSorted() or LastNodeSorted().
		 * Then you can use it to iterate on a digraph with an order.
		 */

		class OnPathIterator //[icerrato]
		{
			DiGraph<T>& _dg;
			uint32_t		_i;
			public:
				typedef typename DiGraph<T>::GraphNode* value_type;
				typedef ptrdiff_t difference_type;
				typedef typename DiGraph<T>::GraphNode** pointer;
				typedef typename DiGraph<T>::GraphNode*& reference;
				typedef std::forward_iterator_tag iterator_category;

			/*!
			 * \brief constructor
			 */
			OnPathIterator(DiGraph<T> &graph)
				:_dg(graph), _i(0) {}

			/*!
			 * \brief copy constructor
			 */
			OnPathIterator(const OnPathIterator &i)
				:_dg(i._dg), _i(i._i) {}

			// To create the "end sentinel" NodeIterator:
			OnPathIterator(DiGraph<T>& graph, bool)
				:_dg(graph), _i(_dg.m_PathNodeList.size()) {}


			GraphNode*& operator*() const
			{
				return _dg.m_PathNodeList[_i];
			}

			OnPathIterator& operator++()
			{ // Prefix
				assert(_i < _dg.m_PathNodeList.size() && "Iterator moved out of range");
				_i++;
				return *this;
			}

			OnPathIterator& operator++(int)
			{ // Postfix
				assert(_i < _dg.m_PathNodeList.size() && "Iterator moved out of range");
				_i++;
				return *this;
			}

			OnPathIterator& operator+=(uint32_t amount)
			{
				assert((_i + amount) < _dg.m_PathNodeList.size() && "Iterator moved out of range");
				_i += amount;
				return *this;
			}

			// To see if you're at the end:
			bool operator==(const OnPathIterator& rv) const
			{
				return _i == rv._i;
			}

			bool operator!=(const OnPathIterator& rv) const
			{
				return _i != rv._i;
			}

		};


	protected:
		uint32_t				m_NodeCount;		//!<Number of nodes in the graph
		std::vector<GraphNode*>			m_NodeList;		//!<List of all the nodes in the graph
		std::vector<GraphNode*>			m_SortedNodes;  	//!<List of the nodes in the graph sorted
		std::set<GraphEdge, EdgeComp>		m_BackEdges; 		//!<Set of the edge
		bool					m_Sorted; 		//!< true if this graph is sorted
		//SubGraph				*m_SubGraph;

		void ForwardDFSPre(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor);
		void ForwardDFSPost(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor);
		void BackwardDFSPre(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor);
		void BackwardDFSPost(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor);
		void ForwardDepthWalk(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor);

		void DFSPreorder(GraphNode &node);
		void DFSPostorder(GraphNode &node);
		void DFSReversePreorder(GraphNode &node);
		bool DFSReversePostorder(GraphNode *node, GraphNode *root_for_loops=NULL);


	public:

		DiGraph()
			:m_NodeCount(0), m_Sorted(0){}

		virtual ~DiGraph()
		{
			uint32_t size = m_NodeList.size();
			for (uint32_t i = 0; i < size; i++)
				delete m_NodeList[i];
		}

		/*!
		  \brief	this function resets the visited flag for all the nodes in the graph

		  \return nothing
		  */
		void ResetVisited(void)
		{
			uint32_t size = m_NodeList.size();

			for (uint32_t i = 0; i < size; i++)
			{
				if (m_NodeList[i] != NULL)
					m_NodeList[i]->Visited = false;
			}
		}
		
		/*!
		 * \brief  this function free all memory allocated from the graph
		 * \return nothing
		 */
		void Clear(void)
		{
			uint32_t size = m_NodeList.size();
			for (uint32_t i = 0; i < size; i++)
				delete m_NodeList[i];
			m_NodeCount = 0;
			m_BackEdges.clear();
			m_NodeList.clear();
			m_SortedNodes.clear();

		}

		/*!
		 * \brief add a Node to the graph
		 * \param nodeInfo the new node content
		 * \return the added node
		 */
		GraphNode &AddNode(T &nodeInfo)
		{
			typedef typename DiGraph<T>::GraphNode node_t;
			node_t* newNode = new node_t(nodeInfo, m_NodeCount);
			m_NodeList.push_back(newNode);
			//m_NodeList[m_NodeCount]->Valid = true;
			return *m_NodeList[m_NodeCount++];
		}

		/*!
		 * \brief add an Edge to the graph
		 * \param from the start of the edge
		 * \param to the end of the edge
		 */
		void AddEdge(GraphNode &from, GraphNode &to)
		{
			NodeLess less;
			NodeEqu equ;
			from.Successors.push_back(&to);
			from.Successors.sort(less);
			from.Successors.unique(equ);
			to.Predecessors.push_back(&from);
			to.Successors.sort(less);
			to.Predecessors.unique(equ);
		}

		/*!
		 * \brief deletes a node from the graph
		 * \param node the node to delete
		 * \return nothing
		 */
		void DeleteNode(GraphNode &node)
		{
			list_iterator_t p = node.Predecessors.begin();
			for (; p != node.Predecessors.end(); p++)
				//DeleteEdge(**p, node);
				(*p)->Successors.remove(&node);
			list_iterator_t s = node.Successors.begin();
			for (; s != node.Successors.end(); s++)
				//DeleteEdge(node, **s);
				(*s)->Predecessors.remove(&node);
			assert(node.Id < m_NodeList.size() &&	 "wrong node id");

			GraphNode *to_delete = m_NodeList[node.Id];
			m_NodeList[node.Id] = NULL;
			delete to_delete;
		}

		/*!
		 * \brief deletes an Edge to the graph
		 * \param from the start of the edge
		 * \param to the end of the edge
		 */
		void DeleteEdge(GraphNode &from, GraphNode &to)
		{
			from.Successors.remove(&to);
			to.Predecessors.remove(&from);
		}

		/*!
		 * \brief visits the graph in pre-order
		 * \param node starting node
		 * \param visitor the visitor to call
		 */
		void PreOrder(GraphNode &node, IGraphVisitor &visitor)
		{
			ResetVisited();
			ForwardDFSPre(node, NULL, visitor);
		}

		/*!
		 * \brief visits the graph in post-order
		 * \param node starting node
		 * \param visitor the visitor to call
		 */
		void PostOrder(GraphNode &node, IGraphVisitor &visitor)
		{
			ResetVisited();
			ForwardDFSPost(node, NULL, visitor);
		}

		/*!
		 * \brief visits the graph in reverse pre-order
		 * \param node starting node
		 * \param visitor the visitor to call
		 */
		void BackPreOrder(GraphNode &node, IGraphVisitor &visitor)
		{
			ResetVisited();
			BackwardDFSPre(node, NULL, visitor);
		}

		/*!
		 * \brief visits the graph in reverse post-order
		 * \param node starting node
		 * \param visitor the visitor to call
		 */
		void BackPostOrder(GraphNode &node, IGraphVisitor &visitor)
		{
			ResetVisited();
			BackwardDFSPost(node, NULL, visitor);
		}


		/*!
		 * \brief visits the graph using depth first order
		 * \param node starting node
		 * \param visitor the visitor to call
		 */
		void ForwDepthWalk(GraphNode &node, IGraphVisitor &visitor)
		{
			ResetVisited();
			ForwardDepthWalk(node, NULL, visitor);
		}

		/*!
		 * \brief get an iterator to the start of the nodes list
		 * \return an iterator to the start of the nodes list
		 */
		NodeIterator FirstNode()
		{
			return NodeIterator(*this);
		}


		/*!
		 * \brief get an iterator to the end of the nodes list
		 * \return an iterator to the end of the nodes list
		 */
		NodeIterator LastNode()
		{
			return NodeIterator(*this, true);
		}


		/*!
		 * \brief get an iterator to the start of the sorted nodes list
		 * \return an iterator to the start of the sorted nodes list
		 */
		SortedIterator FirstNodeSorted()
		{
			return SortedIterator(*this);
		}


		/*!
		 * \brief get an iterator to the end of the sorted nodes list
		 * \return an iterator to the end of the sorted nodes list
		 */
		SortedIterator LastNodeSorted()
		{
			return SortedIterator(*this, true);
		}


		/*!
		 * \brief to get the number of nodes in the graph
		 * \return the number of the nodes in the graph
		 */
		uint32_t GetNodeCount()
		{
			return m_NodeCount;
		}

		/*!
		 * \brief sort a graph in pre-order to get a SortedIterator
		 * \param startnode the node to start the visit from
		 */
		void SortPreorder(GraphNode &startnode)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			DFSPreorder(startnode);
			m_Sorted = true;
		}


		/*!
		 * \brief sort a graph in pre-order to get a SortedIterator
		 * \param startnodes a list of the nodes to start the visit from
		 */
		void SortPreorder(std::list<GraphNode*> &startnodes)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			typename std::list<GraphNode*>::iterator i = startnodes.begin();
			for (; i != startnodes.end(); i++)
				DFSPreorder(*(*i));
			m_Sorted = true;
		}


		/*!
		 * \brief sort a graph in post-order to get a SortedIterator
		 * \param startnode the node to start the visit from
		 */
		void SortPostorder(GraphNode &startnode)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			DFSPostorder(startnode);
			m_Sorted = true;
		}


		/*!
		 * \brief sort a graph in post-order to get a SortedIterator
		 * \param startnodes a list of the nodes to start the visit from
		 */
		void SortPostorder(std::list<GraphNode*> &startnodes)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			typename std::list<GraphNode*>::iterator i = startnodes.begin();
			for (; i != startnodes.end(); i++)
				DFSPostorder(*(*i));
			m_Sorted = true;
		}

		/*!
		 * \brief sort a graph in reverse pre-order to get a SortedIterator
		 * \param startnode the node to start the visit from
		 */
		void SortRevPreorder(GraphNode &startnode)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			DFSReversePreorder(startnode);
			m_Sorted = true;
		}


		/*!
		 * \brief sort a graph in reverse pre-order to get a SortedIterator
		 * \param startnodes a list of the nodes to start the visit from
		 */
		void SortRevPreorder(std::list<GraphNode*> &startnodes)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			typename std::list<GraphNode*>::iterator i = startnodes.begin();
			for (; i != startnodes.end(); i++)
				DFSReversePreorder(*(*i));
			m_Sorted = true;
		}

		/*!
		 * \brief sort a graph in reverse post-order to get a SortedIterator
		 * \param startnode the node to start the visit from
                 * \return true if a loop containing startnode was found
		 */
		bool SortRevPostorder(GraphNode &startnode)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			const bool to_ret = DFSReversePostorder(&startnode, &startnode);
			m_Sorted = true;
                        return to_ret;
		}


		/*!
		 * \brief sort a graph in reverse post-order to get a SortedIterator
		 * \param startnodes a list of the nodes to start the visit from
		 */
		void SortRevPostorder(std::list<GraphNode*> &startnodes)
		{
			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			typename std::list<GraphNode*>::iterator i = startnodes.begin();
			for (; i != startnodes.end(); i++)
				DFSReversePostorder(*i);
			m_Sorted = true;
		}

		void SortRevPostorder_alternate(GraphNode &startnode)
		{


			ResetVisited();
			m_SortedNodes.clear();
			m_BackEdges.clear();
			DFSPostorder(startnode);
			std::reverse(m_SortedNodes.begin(), m_SortedNodes.end());
			m_Sorted = true;
		}

		bool IsBackEdge(GraphEdge &edge)
		{
			typedef typename std::set<GraphEdge, EdgeComp>::iterator iterator_t;
			iterator_t i = m_BackEdges.find(edge);
			if (i == m_BackEdges.end())
				return false;
			return true;
		}
		
		
		std::set<GraphEdge, EdgeComp> get_back_edges() { return m_BackEdges;};
		
		
};


	template<class T>
void DiGraph<T>::ForwardDFSPre(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor)
{

	if (node.Visited)
		return;

	visitor.ExecuteOnNodeFrom(node, comingFrom);

	node.Visited = true;

	list_iterator_t p = node.Successors.begin();

	while (p != node.Successors.end())
	{
		GraphNode &successor(**p);
		GraphEdge edge(node, successor);
		p++;
		bool last = (p == node.Successors.end());
		visitor.ExecuteOnEdge(node, edge, last);

		ForwardDFSPre(successor, &node, visitor);
	}
}



	template<class T>
void DiGraph<T>::ForwardDFSPost(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor)
{

	if (node.Visited)
		return;

	node.Visited = true;

	list_iterator_t p = node.Successors.begin();

	while (p != node.Successors.end())
	{
		GraphNode &successor(**p);
		GraphEdge edge(node, successor);
		p++;
		bool last = (p == node.Successors.end());

		ForwardDFSPost(successor, &node, visitor);

		visitor.ExecuteOnEdge(node, edge, last);


	}

	visitor.ExecuteOnNodeFrom(node, comingFrom);
}


	template<class T>
void DiGraph<T>::BackwardDFSPre(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor)
{

	if (node.Visited)
		return;

	node.Visited = true;
	visitor.ExecuteOnNodeFrom(node, comingFrom);
	list_iterator_t p = node.Predecessors.begin();

	while (p != node.Predecessors.end())
	{
		GraphNode &predecessor(**p);
		GraphEdge edge(predecessor, node);
		p++;
		bool last = (p == node.Predecessors.end());
		visitor.ExecuteOnEdge(node, edge, last);

		BackwardDFSPre(predecessor, &node, visitor);
	}
}


	template<class T>
void DiGraph<T>::BackwardDFSPost(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor)
{

	if (node.Visited)
		return;

	node.Visited = true;


	list_iterator_t p = node.Predecessors.begin();

	while (p != node.Predecessors.end())
	{
		GraphNode &predecessor(**p);
		GraphEdge edge(predecessor, node);
		p++;
		bool last = (p == node.Predecessors.end());
		BackwardDFSPost(predecessor, &node, visitor);

		visitor.ExecuteOnEdge(node, edge, last);
	}

	visitor.ExecuteOnNodeFrom(node, comingFrom);
}

	template<class T>
void DiGraph<T>::ForwardDepthWalk(GraphNode &node, GraphNode *comingFrom, IGraphVisitor &visitor)
{
	if (node.Visited)
		return;

	node.Visited = true;

	list_iterator_t p = node.Successors.begin();

	while (p != node.Successors.end())
	{
		visitor.ExecuteOnNodeFrom(**p, comingFrom);
		GraphEdge edge(node, **p);
		visitor.ExecuteOnEdge(node, edge, false);
		p++;
	}

	list_iterator_t s = node.Successors.begin();

	while (s != node.Successors.end())
	{
		ForwardDepthWalk(**s, &node, visitor);
		s++;
	}
}



	template<class T>
void DiGraph<T>::DFSPreorder(GraphNode &node)
{
	if (node.Visited)
		return;

	m_SortedNodes.push_back(&node);

	node.Visited = true;

	list_iterator_t p = node.Successors.begin();

	for(; p != node.Successors.end(); p++)
	{
		GraphNode &successor(**p);
		if (successor.Visited)
		{
			GraphEdge edge(node, successor);
			m_BackEdges.insert(edge);
		}
		else
		{
			DFSPreorder(successor);
		}
	}
}



	template<class T>
void DiGraph<T>::DFSPostorder(GraphNode &node)
{
	if (node.Visited)
		return;

	node.Visited = true;

	list_iterator_t p = node.Successors.begin();

	for(; p != node.Successors.end(); p++)
	{
		GraphNode &successor(**p);
		if (successor.Visited)
		{
			GraphEdge edge(node, successor);
			m_BackEdges.insert(edge);
		}
		else
		{
			DFSPostorder(successor);
		}
	}

	m_SortedNodes.push_back(&node);
}


	template<class T>
void DiGraph<T>::DFSReversePreorder(GraphNode &node)
{

	if (node.Visited)
		return;

	m_SortedNodes.push_back(&node);

	node.Visited = true;

	list_iterator_t p = node.Predecessors.begin();

	for(; p != node.Predecessors.end(); p++)
	{
		GraphNode &predecessor(**p);
		if (predecessor.Visited)
		{
			GraphEdge edge(predecessor, node);
			m_BackEdges.insert(edge);
		}
		else
		{
			DFSReversePreorder(predecessor);
		}
	}
}

// returns true if a loop that includes root_for_loops (optional parameter) was found
template<class T>
bool DiGraph<T>::DFSReversePostorder(GraphNode *node, GraphNode *root_for_loops)
{
	//std::cout << "Entering node " << node.NodeInfo << std::endl;
	if (node->Visited)
          return (root_for_loops != NULL && node == root_for_loops);

	node->Visited = true;

	list_iterator_t p = node->Predecessors.begin();

        bool loop_found = false;
	for(; p != node->Predecessors.end(); p++)
	{
		GraphNode &predecessor(**p);
		if (predecessor.Visited)
		{
			GraphEdge edge(predecessor, *node);
			//std::cout << "Back edge: " << predecessor.NodeInfo << " -> " << node.NodeInfo << std::endl;
			m_BackEdges.insert(edge);
		}

		loop_found = DFSReversePostorder(&predecessor, root_for_loops) || loop_found;
	}

	//std::cout << "Visiting node " << node.NodeInfo << std::endl;
	m_SortedNodes.push_back(node);
        return loop_found;
}
