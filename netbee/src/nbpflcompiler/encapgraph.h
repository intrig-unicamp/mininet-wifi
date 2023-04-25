/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "defs.h"
#include "symbols.h"
#include "digraph.h"
#include <map>



typedef DiGraph<SymbolProto*> ProtoGraph; //!< DiGraph template specialization with nodes holding information on a single protocol


/*! \brief this class implements the Protocol Encapsulation Graph

 	This class inherits from the template specialization DiGraph<SymbolProto*>, so the nodes of the graph
 	represent a single protocol defined in the NetPDL database
*/
class EncapGraph: public ProtoGraph
{

public:

	typedef ProtoGraph::GraphNode GraphNode;
	typedef ProtoGraph::GraphEdge GraphEdge;
	typedef ProtoGraph::NodeIterator NodeIterator;

private:

	typedef std::map <uint32, ProtoGraph::GraphNode*> ProtoGraphMap_t;


	ProtoGraphMap_t	m_ProtoGraphMap;		//!< Protocol Id to GraphNode associative map
	/*!
	 	\brief graph node containing the startproto protocol

	 	This corresponds to the ONLY source node of the protocol encapsulation graph
	*/
	GraphNode	*m_StartNode;

public:

	/*!
		\brief constructor
	 */
	EncapGraph();

	/*!
	 	\brief returns the graph node containing the startproto protocol
	 	\return a reference to the startproto graph node
	*/
	ProtoGraph::GraphNode &GetStartProtoNode(void);

	/*!
	 	\brief set the startproto node
	 	\param proto pointer to the protocol symbol relative to the startproto protocol
	*/
	void SetStartProtoNode(SymbolProto *proto);


	/*!
	 	\brief set the startproto node
	 	\param proto pointer to a graph node containing the startproto protocol
	*/
	void SetStartProtoNode(GraphNode *node);


	 /*!
	 	\brief add a Node to the graph

	 	This method overloads the AddNode method of the DiGraph class

	 	\param proto pointer to the symbol of the protocol to add
	 	\return the added graph node
	 */
	GraphNode &AddNode(SymbolProto* &proto);


	/*!
	 	\brief returns the node relative to a protocol
	 	\param proto pointer to the symbol of the protocol to add
	 	\return the node corresponding to the specified protocol
	 */
	GraphNode *GetNode(SymbolProto *proto);


	bool RemoveUnsupportedNodes(void);

	void RemoveUnconnectedNodes(void);

	bool IsConnected(void);

};


