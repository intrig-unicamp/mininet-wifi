/****************************************************************************************/
/*                                                                           			*/
/* Copyright notice: please read file license.txt in the NetBee root folder. 			*/
/*                                                                           			*/
/****************************************************************************************/

/****************************************************************************************/
/*																		    			*/
/* class developed by Ivano Cerrato - wed nov 30 2011 - thu jul 5 2012 - fri jul 6 2012 */
/*																		    			*/
/****************************************************************************************/

#pragma once


#include "defs.h"
#include "symbols.h"
#include <math.h>
#include <iostream>
#include <set>

#ifdef ENABLE_PEG_AND_xpFSA_PROFILING
	#include <nbee_profiler.h>
#endif

#define INF 65535

using namespace std;


class AssignProtocolLayer
{

	EncapGraph	&m_Graph;
	bool assigned;
	map<double,set<SymbolProto*> > pl; //for each layer, contains the set of the name of protocols of that layer
	set<double> layers;
	double currentRequired;

#ifdef ENABLE_PEG_AND_xpFSA_PROFILING	
	uint64_t TicksBefore;
	uint64_t TicksAfter;
	uint64_t MeasureCost;
#endif	
		
	void Initialize(void)
	{	
		EncapGraph::NodeIterator i = m_Graph.FirstNode();
		for (; i != m_Graph.LastNode(); i++)
		{
			if((*i)->NodeInfo->ID!=0) //the node is not the startproto
				(*i)->NodeInfo->Level = INF;
			else
				(*i)->NodeInfo->Level = 1;
		} 
	
	}
		
	EncapGraph::GraphNode *GetMinSuccessor(EncapGraph::GraphNode &node)
	{
		EncapGraph::GraphNode *successor = NULL;
		double minLevel = INF;
		list<EncapGraph::GraphNode*> &successors = node.GetSuccessors();
		list<EncapGraph::GraphNode*>::iterator s = successors.begin();
		for (; s != successors.end(); s++)
		{
			if(*s == &node) //we must no consider this case
				continue;
			if ((*s)->NodeInfo->Level < minLevel)
			{
				minLevel = (*s)->NodeInfo->Level;
				successor = *s;
			}
		}
		return successor;
	}
	
	EncapGraph::GraphNode *GetMaxPredecessor(EncapGraph::GraphNode &node)
	{	
		EncapGraph::GraphNode *predecessor = NULL;
		double maxLevel = 0;
		list<EncapGraph::GraphNode*> &predecessors = node.GetPredecessors();
		list<EncapGraph::GraphNode*>::iterator p = predecessors.begin();
		for (; p != predecessors.end(); p++)
		{
			if(*p == &node) //we must no consider this case
				continue;
			if ((*p)->NodeInfo->Level >= maxLevel)
			{
				maxLevel = (*p)->NodeInfo->Level;
				predecessor = *p;
			}
		}
		return predecessor;
	}
	
	void DepthWalk(EncapGraph::GraphNode &node)
	{
		if (node.Visited)
			return;

		node.Visited = true;

		double nodeLevel = node.NodeInfo->Level;

		EncapGraph::GraphNode *minSuccessor = GetMinSuccessor(node);
		
		double nextLevel = (minSuccessor!=NULL) ? minSuccessor->NodeInfo->Level : INF;

		if (nextLevel <= nodeLevel)
		{
			EncapGraph::GraphNode *maxPredecessor = GetMaxPredecessor(node);
			nbASSERT(maxPredecessor != NULL, "maxPredecessor should not be NULL");
			double prevLevel = maxPredecessor->NodeInfo->Level;
			if (prevLevel < nextLevel)
			{
				double old = node.NodeInfo->Level;
				node.NodeInfo->Level = prevLevel + ((nextLevel - prevLevel) / 2);
								
				ChangeInformation(node.NodeInfo, old, node.NodeInfo->Level);
			}
		}

		double level = ceil(node.NodeInfo->Level + 1);
		list<EncapGraph::GraphNode*> &successors = node.GetSuccessors();
		list<EncapGraph::GraphNode*>::iterator i = successors.begin();
		for (; i != successors.end(); i++)
		{
			if (level < (*i)->NodeInfo->Level)
			{
				double old = (*i)->NodeInfo->Level;
				(*i)->NodeInfo->Level = level;
				
				ChangeInformation((*i)->NodeInfo, old, (*i)->NodeInfo->Level);				
			}
		}
		list<EncapGraph::GraphNode*>::iterator ss = successors.begin();
		for (; ss != successors.end(); ss++)
			DepthWalk(**ss);

	}
	
	void ChangeInformation(SymbolProto *protocol, double oldL, double newL)
	{
		//remove the old info - if needed
		map<double,set<SymbolProto*> >::iterator element = pl.find(oldL);
		set<SymbolProto*> s;
		if(element != pl.end())
		{
			s = (*element).second;
		
			set<SymbolProto*>::iterator n = s.begin();
			for(; n != s.end(); n++)
			{
				if((*n) == protocol)
					break;
			}
			if(n!=s.end())
				s.erase(n);		
			pl.erase(oldL);		
			pl.insert(make_pair<double,set<SymbolProto*> >(oldL,s));
		}
				
		//store the new info
		map<double,set<SymbolProto*> >:: iterator element2 = pl.find(newL);
		set<SymbolProto*> s2; 
		if(element2 != pl.end())
			s2 = (*element2).second;
		s2.insert(protocol);
		pl.erase(newL);
		pl.insert(make_pair<double,set<SymbolProto*> >(newL,s2));
	}
	
	/*
	*	This method stores in a set, all the assigned layers
	*/
	void StoreTheLayers(void)
	{
		EncapGraph::NodeIterator i = m_Graph.FirstNode();
		for (; i != m_Graph.LastNode(); i++)
			layers.insert((*i)->NodeInfo->Level);
	}
	
	/*
	*	Assigns to currentRequired, the next layer stored in the set layers
	*/
	void UpdateCurrentRequired()
	{
		set<double>::iterator i = layers.begin();
		for (; i != layers.end(); i++)
		{
			if((*i) == currentRequired)
				break;
		}
		i++;
		if(i != layers.end())
			currentRequired = (*i);
		else
			currentRequired = -1;
	}

	/*
	*	Given the currentRequired level, this method returns the related protocols
	*/
	set<SymbolProto*> GetProtocolsOfLayer()
	{		
		if(currentRequired==1)
		{
			set<SymbolProto*> aux;
			aux.insert((m_Graph.GetStartProtoNode()).NodeInfo);
			return aux;
		}
		
		map<double,set<SymbolProto*> >::iterator it = pl.find(currentRequired);
		set<SymbolProto*> s;
		
		if(it == pl.end())
			return s;
		
		s = (*it).second;
		return (*(pl.find(currentRequired))).second;
	}

public:

	AssignProtocolLayer(EncapGraph &graph)
		:m_Graph(graph),assigned(false),currentRequired(-1){}
	
	//assign a layer to each protocol of the database	
	void Classify(void)
	{
		if(!assigned)
		{
#ifdef ENABLE_PEG_AND_xpFSA_PROFILING		
		MeasureCost= nbProfilerGetMeasureCost();
		TicksBefore = nbProfilerGetTime();
#endif
			Initialize();		
			m_Graph.ResetVisited();
			DepthWalk(m_Graph.GetStartProtoNode());
#ifdef ENABLE_PEG_AND_xpFSA_PROFILING		
			TicksAfter = nbProfilerGetTime();
#endif
			StoreTheLayers();
			assigned = true;
		}
	}
	
	void StartRequiringProtocolsFrom(double layer)
	{
		nbASSERT(assigned,"You must call \"Classify()\" before this method!");
		currentRequired = layer;
	}	

	set<SymbolProto*> GetNext()
	{
		nbASSERT(assigned,"You must call \"Classify()\" and \"StartRequiringProtocolsFrom(double layer)\" before this method!");

		set<SymbolProto*> protocols;
		if(currentRequired<0)
			return protocols;	
		
		protocols = GetProtocolsOfLayer();		
		UpdateCurrentRequired();
		
		return protocols;
	}
	
#ifdef ENABLE_PEG_AND_xpFSA_PROFILING
	//Written the 3rd december 2012 by Ivano Cerrato
	void PrintLayers(ofstream &pegAndXPFSAProfiling)
	{
		nbASSERT(assigned,"You must call \"Classify()\" before this method!");
		unsigned int counter = 0;
		stringstream ss;
		EncapGraph::NodeIterator i = m_Graph.FirstNode();
		for (; i != m_Graph.LastNode(); i++)
		{
			if((*i)->NodeInfo->Level == INF)
				continue;
				
			pegAndXPFSAProfiling <<"\t\tProtocol: " << (*i)->NodeInfo->Name << " - level: " << (*i)->NodeInfo->Level << endl;
			
			list<EncapGraph::GraphNode*> &successors = (*i)->GetSuccessors();
			for(list<EncapGraph::GraphNode*>::iterator next = successors.begin(); next != successors.end(); next++)
			{
				if((*i)->NodeInfo->Level >= (*next)->NodeInfo->Level)
				{
					ss << "\t\t" << (*i)->NodeInfo->Name << "(" << (*i)->NodeInfo->Level << ")" << " --> " << (*next)->NodeInfo->Name << "(" << (*next)->NodeInfo->Level << ")" << endl;
					counter++;
				}
			}
		}
		pegAndXPFSAProfiling <<"\tEdges toward a protocol of a lower/equal layer: " << counter << endl;
		pegAndXPFSAProfiling << ss.str();
		pegAndXPFSAProfiling.flush();
		
		pegAndXPFSAProfiling <<"\tThe classification algorithm required " << (TicksAfter - TicksBefore - MeasureCost) << " ticks." << endl;
	
	}		
#endif

};

