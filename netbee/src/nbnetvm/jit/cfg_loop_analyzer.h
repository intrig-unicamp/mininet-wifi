/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

#include "cfg.h"
#include "digraph.h"
#include "jit_internals.h"

#include <stack>

namespace jit{

template <class IR>
	class LoopAnalyzer: public nvmJitFunctionI
	{
		private:
			jit::CFG<IR> &_cfg;
		public:
			LoopAnalyzer(jit::CFG<IR> &cfg): nvmJitFunctionI("Loop analyzer"),  _cfg(cfg) {}
			bool run();
		private:
			void analyzeBackEdge(typename jit::CFG<IR>::GraphEdge );
			bool isDominant(const typename jit::CFG<IR>::GraphEdge &);
	};

template <class IR>
	bool LoopAnalyzer<IR>::isDominant(const typename jit::CFG<IR>::GraphEdge &ge)
	{
		typedef BasicBlock<IR>* p_bb_t;
		typedef typename std::set<p_bb_t> set_t;

		set_t dom_set;

		dom_set.insert(ge.From.NodeInfo);
		p_bb_t iter = ge.From.NodeInfo->template getProperty<p_bb_t>("IDOM");

		while(iter != NULL)
		{
			if(*iter == *ge.To.NodeInfo)
				return true;
			iter = iter->template getProperty<p_bb_t>("IDOM");
		}
		return false;
	}

template <class IR>
	void LoopAnalyzer<IR>::analyzeBackEdge(typename jit::CFG<IR>::GraphEdge ge)
	{
		typedef typename jit::BasicBlock<IR>::BBIterator bb_it_t;
		BasicBlock<IR> *from, *to;
		from = ge.From.NodeInfo;
		to = ge.To.NodeInfo;

#ifdef _DEBUG_SPILL
		std::cout << "Analizzo l'edge da: " << *from << " a: " << *to << std::endl;
#endif
		if(!isDominant(ge))
		{
			return;
		}
		std::stack<BasicBlock<IR> *> stack;
		std::set<BasicBlock<IR> *> loop_set;
		loop_set.insert(to);
		stack.push(from);
		while(stack.size() > 0)
		{
			BasicBlock<IR> *tmp = stack.top();
			stack.pop();
			for(bb_it_t i = tmp->precBegin(); i != tmp->precEnd(); i++)
			{
				if(loop_set.find(*i) == loop_set.end())
				{
					loop_set.insert(*i);
					stack.push(*i);
				}
			}
		}

		typename std::set<BasicBlock<IR> *>::iterator i = loop_set.begin();
		for(; i != loop_set.end(); i++)
		{
			uint16_t *level;
			level = (*i)->template getProperty<uint16_t*>("loopLevel");
			assert(level != NULL);
			(*level)++;
		}
	}

template <class IR>
	bool LoopAnalyzer<IR>::run()
	{
		typedef typename jit::CFG<IR>::GraphEdge GraphEdge;
		typedef typename jit::CFG<IR>::EdgeComp EdgeComp;
		typedef typename jit::CFG<IR>::NodeIterator node_it;
		typedef std::set<GraphEdge, EdgeComp> set_t;
		typedef typename set_t::iterator set_it_t;

		//set level property to all BBs
		for(node_it i = _cfg.FirstNode(); i != _cfg.LastNode(); i++)
			(*i)->NodeInfo->setProperty("loopLevel", new uint16_t(0));
		set_t be_set(_cfg.get_back_edges());
		for(set_it_t i = be_set.begin(); i != be_set.end(); i++)
		{
			analyzeBackEdge(*i);
		}
		return true;
	}

}//end namespace

