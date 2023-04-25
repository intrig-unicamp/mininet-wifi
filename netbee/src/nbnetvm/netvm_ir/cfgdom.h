#ifndef GRAPH_NUMBERER_H
# define GRAPH_NUMBERER_H

/*!
 * \file cfgDom.h
 * \brief this file contains definition for the class that computes the dominance for a digraph
 */

#include "cfg.h"
#include "function_interface.h"
#include <assert.h>
//#include "mirnode.h"

namespace jit{
	/*!
	 * \brief class to number the node in reverse post-order
	 */
	template<class _CFG>
		class GraphNumberer: public _CFG::IGraphVisitor
	{
		private:
			int postorder_num; //!<actual reverse post order number
			typedef typename _CFG::node_t GraphNode; //!<type of a graph node
			typedef typename _CFG::edge_t GraphEdge; //!<type of a graph edge

		public:

			/*!
			 * \brief constructor
			 * \param initPosition first number to assign
			 */
			GraphNumberer(uint32_t initPosition = 0):postorder_num(initPosition) {};

			/*!
			 * \brief does nothing
			 */
			uint32_t ExecuteOnEdge(const GraphNode &visitedNode,const  GraphEdge &edge, bool lastEdge) 
			{
				return 0; //nvmJitSUCCESS;		
			}


			/*!
			 * \brief numbering action
			 * 
			 * It sets the postorder_num as "postOrderPosition" property in the node. Increments the postorder_num
			 */
			uint32_t ExecuteOnNodeFrom(const GraphNode &visitedNode, const GraphNode *comingFrom) 
			{
				visitedNode.NodeInfo->template setProperty<uint32_t>("postOrderPosition", postorder_num);
				//std::cout << "BB ID: " << visitedNode.NodeInfo->getId() << "Postorder: " << postorder_num << std::endl; 
				postorder_num++;
				return 0; //nvmJitSUCCESS;
			}
	};

	/*!
	 * \brief function to compare to basic block using their post order number
	 * \param BB1 pointer to a basic block
	 * \param BB2 pointer to a basic block
	 * \return true if BB1 comes before BB2 in post order number
	 */
	template<class _CFG>
		bool cmpPostOrder(typename _CFG::BBType* BB1, typename _CFG::BBType* BB2)
		{
			uint32_t pos1, pos2;
			pos1 = (uint32_t)BB1->getProperty("postOrderPosition");
			pos2 = (uint32_t)BB2->getProperty("postOrderPosition");
			
			return pos1 < pos2;

			return false;
		}

	/*!
	 * \brief class that simply compares two basic blocks' Ids
	 */
	template<class _CFG>
		struct BBIDCmp
		{
			/*!
			 * \brief function to compare to basic block using their post order number
			 * \param BB1 pointer to a basic block
			 * \param BB2 pointer to a basic block
			 * \return true if BB1 comes before BB2 in post order number
			 */
		bool operator()(typename _CFG::BBType* BB1, typename _CFG::BBType* BB2)
		{
			return BB1->getId() < BB2->getId();
		}
		};

	template<class _CFG>
	class IDom_IterationFunctor
	{
		private:
		bool *changed;
		
		public:
		typedef typename _CFG::BBType BBType;
		typedef typename _CFG::IRType IR;
		typedef typename BBType::node_t node_t;
		typedef typename std::list<node_t *>::iterator predIterator;

		IDom_IterationFunctor(bool *changed): changed(changed) {}

		BBType* IntersectDomSets(BBType *BBi, BBType *BBj)
		{
			BBType *curs1, *curs2;

			curs1 = BBi;
			curs2 = BBj;

			uint32_t curs1RPON, curs2RPON;
			BBType *IDom1, *IDom2;

			curs1RPON = (curs1->template getProperty<uint32_t>("postOrderPosition"));
			curs2RPON = (curs2->template getProperty<uint32_t>("postOrderPosition"));
			IDom1 = curs1->template getProperty<BBType*>("IDOM");
			IDom2 = curs2->template getProperty<BBType*>("IDOM");

			while(curs1RPON != curs2RPON && IDom1 && IDom2)
			{
				while( curs1RPON > curs2RPON )
				{
					curs1 = IDom1;
					curs1RPON = (curs1->template getProperty<uint32_t>("postOrderPosition"));
					IDom1 = curs1->template getProperty<BBType*>("IDOM");
				}
				while( curs2RPON > curs1RPON)
				{
					curs2 = IDom2;
					curs2RPON = (curs2->template getProperty<uint32_t>("postOrderPosition"));
					IDom2 = curs2->template getProperty<BBType*>("IDOM");
				}
			}
			
			// Fix for nested loops, don't know if it works or not...
			if(!curs1->template getProperty<BBType *>("IDOM"))
				return curs2;
	
			return curs1;
		}
		void operator()(node_t*  Node)
		{
			BBType* BB = Node->NodeInfo;
			std::list<node_t *> tmplist;
			predIterator i;
			BBType *current, *tmpIDom;
			tmplist = BB->getPredecessors();

			if(tmplist.size() == 0)
				return;
			i = tmplist.begin();

			tmpIDom = (*i)->NodeInfo;
			i++;
	
			while(i != tmplist.end())
			{
				current = (*i)->NodeInfo;
				tmpIDom = IntersectDomSets(tmpIDom, current);
				i++;
			}

			if(BB->template getProperty<BBType*>("IDOM") == tmpIDom)
				*changed = false;
			else
			{
				BB->setProperty("IDOM", tmpIDom);
				*changed = true;
			}
			return;
		}
	};

	template<class _CFG>
		struct DFIterationStep
		{
			typedef typename _CFG::BBType BBType;
			typedef typename _CFG::IRType IR;
			typedef typename _CFG::node_t node_t;
			void operator()(node_t *node)
			{
				typedef typename BBType::node_t node_t;
				typedef typename std::list<node_t *>::iterator predIterator;

				BBType* BB = node->NodeInfo;
				std::list<node_t *> predList = BB->getPredecessors();
				if(predList.size() <=1)
					return;

				for(predIterator i = predList.begin(); i != predList.end(); i++)
				{
					BBType *currNode, *BBIDom;

					BBIDom = BB->template getProperty<BBType*>("IDOM");
					currNode = (*i)->NodeInfo;
					while( currNode != BBIDom)
					{
						std::set< BBType*, BBIDCmp<_CFG> > *domfrontier;
						if( (domfrontier =currNode->template getProperty<std::set< BBType*, BBIDCmp<_CFG> > *>("DomFrontier")) == NULL)
						{
							domfrontier = new std::set< BBType*, BBIDCmp<_CFG> >;
							currNode->setProperty("DomFrontier", domfrontier);
						}
						domfrontier->insert(BB);
						currNode = currNode->template getProperty<BBType*>("IDOM");
						if (currNode == NULL)
							break;
					}
				}
			}
		};

	template<class _CFG>
		struct DomTreeIterationStep
		{

		typedef typename _CFG::node_t node_t;
		typedef typename _CFG::BBType BBType;
		typedef typename _CFG::IRType IR;
		void operator()(node_t * node)
		{
			BBType *BB = node->NodeInfo;
			typedef typename BBType::node_t node_t;
			typedef BBType* BasicBlockP_t;
			typedef typename std::list<node_t *>::iterator predIterator;
			BasicBlockP_t iDom = NULL;
			std::set<BasicBlockP_t, BBIDCmp<_CFG> > *domSuccessors;

			iDom = BB->template getProperty<BasicBlockP_t>("IDOM");
			if(iDom == NULL)
				return;
			domSuccessors = iDom->template getProperty<std::set<BasicBlockP_t, BBIDCmp<_CFG> >*>("DomSuccessors");
			if(domSuccessors == NULL)
			{
				domSuccessors = new std::set<BasicBlockP_t, BBIDCmp<_CFG> >;
				iDom->setProperty("DomSuccessors", domSuccessors);
			}
			domSuccessors->insert(BB);
			return;
		}
		};

	/*!
	 * \brief this function computes the dominance of a control fow graph
	 * \param cfg The control flow graph on which we run the algorithm
	 * 
	 * This function finds every BB's direct dominator. And every BB's domination frontier
	 */
	template<class _CFG>
		void computeDominance(_CFG &cfg)
		{
			typedef typename _CFG::BBType BBType;
			typedef typename _CFG::IRType IR;
			typedef typename std::list< BBType* >::iterator listIterator;
			typedef typename DiGraph< BBType* >::SortedIterator sortedIt;
//			GraphNumberer<jit::MIRNode_t> g;
//			cfg.BackPostOrder(*cfg.getExitNode(), g);
			cfg.SortRevPostorder_alternate(*cfg.getEntryNode());
			uint32_t counter = 0;
			sortedIt i = cfg.FirstNodeSorted();
			for( ; i != cfg.LastNodeSorted(); i++)
			{
				BBType* BBiter;
				BBiter = (*i)->NodeInfo;
				//std::cout << "setting postOrderPosition to " << counter << " for " << BBiter->getId() << '\n';
				BBiter->template setProperty<uint32_t>("postOrderPosition", counter);
				counter++;
			}

/*
			std::list< BasicBlock<IR>* > *BBPostOrderlist;

			BBPostOrderlist = cfg.getBBList();

			BBPostOrderlist->sort(cmpPostOrder<IR>);	
*/
			// idom loop
			bool changed = true;
			while(changed)
			{
				changed = false;
				IDom_IterationFunctor<_CFG> func(&changed);
				std::for_each(cfg.FirstNodeSorted(), cfg.LastNodeSorted(), func);
			}

			std::for_each(cfg.FirstNodeSorted(), cfg.LastNodeSorted(), DFIterationStep<_CFG>());

			std::for_each(cfg.FirstNodeSorted(), cfg.LastNodeSorted(), DomTreeIterationStep<_CFG>());
		}

	template<class _CFG>
		class ComputeDominance: public nvmJitFunctionI
	{
		private:
			_CFG &_cfg;
		public:
			ComputeDominance(_CFG &cfg): nvmJitFunctionI("Compute dominance"), _cfg(cfg) {};
			bool run() { computeDominance(_cfg); return true; }
	};
}
#endif
