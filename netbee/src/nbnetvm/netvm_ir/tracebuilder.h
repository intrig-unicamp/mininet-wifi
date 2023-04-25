#ifndef _TRACE_BUILDER
#define _TRACE_BUILDER

/*!
 * \file tracebuilder.h
 * \brief a class which given a cfg gives you an order of emission of the bb
 */

#include "basicblock.h"
#include "cfg.h"
#include <set>
#include <list>
#include <string>

namespace jit
{

	//!class that builds trace of bb (selects the order of emission)
	template<typename _CFG>
	class TraceBuilder 
	{
		public:
		typedef typename _CFG::IRType IR; //!<instruction type
		typedef typename _CFG::BBType bb_t; //!<basic block type
		typedef std::list< bb_t* > list_t; //!<list of basic block type
		typedef typename list_t::iterator trace_iterator_t; //!<iterator to the trace type
		typedef std::set<uint16_t> set_t; //!<set of bb id type

		/*!
		 * \brief constructor
		 * \param cfg reference to the cfg to trace
		 */
		TraceBuilder(_CFG& cfg);

		/*!
		 * \brief get an iterator to the start of the traces
		 * \return an iterator to the start of the traces
		 */
		trace_iterator_t begin();

		/*!
		 * \brief get an iterator to the end of the traces
		 * \return an iterator to the end of the traces
		 */
		trace_iterator_t end();

		//!build the traces
		void build_trace();

		virtual void handle_no_succ_bb(bb_t* bb) = 0;
		virtual void handle_one_succ_bb(bb_t* bb)= 0;
		virtual void handle_two_succ_bb(bb_t* bb)= 0;

		virtual ~TraceBuilder();

		static const std::string next_prop_name; //!<name of the property in the bb with the pointer of next bb in emission

		protected:

		//!is this basicblock already visited
		bool is_visited(bb_t* bb);

		//!add a basicblock to the traces
		void add_to_trace(bb_t* bb);
		//!start a trace with this basic block
		void beginTrace(bb_t* bb);

		_CFG& cfg; //!<cfg to trace
		list_t traces; //!<sequence of basic block in the order of the emission
		list_t* bbs; //!<list of all basic block in the cfg
		set_t visited; //!<set which holds already visited nodes
	};
}

template<typename _CFG>
jit::TraceBuilder<_CFG>::~TraceBuilder()
{
	delete bbs;
}

template<typename _CFG>
const std::string jit::TraceBuilder<_CFG>::next_prop_name("trace_next");

template<typename _CFG>
jit::TraceBuilder<_CFG>::TraceBuilder(_CFG& cfg) : cfg(cfg), bbs(cfg.getBBList())
{
}

template<typename _CFG>
typename jit::TraceBuilder<_CFG>::trace_iterator_t jit::TraceBuilder<_CFG>::begin()
{
	return traces.begin();
}

template <typename _CFG>
bool jit::TraceBuilder<_CFG>::is_visited(bb_t* bb)
{
	return visited.find(bb->getId()) != visited.end();
}

template<typename _CFG>
typename jit::TraceBuilder<_CFG>::trace_iterator_t jit::TraceBuilder<_CFG>::end()
{
	return traces.end();
}

template<typename _CFG>
void jit::TraceBuilder<_CFG>::add_to_trace(bb_t* bb)
{
	bb_t* previous = NULL;

	if(!traces.empty())
	{
		previous = traces.back();
		previous->template setProperty< bb_t* >(next_prop_name, bb);
	}
		
	//insert in the list and set traced
	traces.push_back(bb);
	visited.insert(bb->getId());
}

template<typename _CFG>
void jit::TraceBuilder<_CFG>::beginTrace(bb_t* b)
{
	while(!is_visited(b))
	{
		//typedef typename bb_t::BBIterator bb_iterator_t;
		typedef typename std::list< typename _CFG::GraphNode* > succ_list_t;
		typedef typename succ_list_t::iterator iterator_t;

		add_to_trace(b);
		
		succ_list_t& succs = b->getSuccessors();
		iterator_t succ = succs.begin();
		for(; succ != succs.end(); succ++)
		{
			b = (*succ)->NodeInfo;
			if(!is_visited(b))
				break;
		}

		if(succ == succs.end())
			return;
	}
}

template<typename _CFG>
void jit::TraceBuilder<_CFG>::build_trace()
{
	list_t * bbs_ptr = cfg.getBBList();
	list_t bbs(*bbs_ptr);
	bb_t* b = cfg.getEntryBB();
	bbs.remove(b);
	
	beginTrace(b);

	for(trace_iterator_t bb = bbs.begin(); bb != bbs.end(); bb++)
	{
		if(!is_visited(*bb))
		{
			beginTrace(*bb);
		}
	}

	#ifdef _DEBUG_TRACE_BUILDER
	std::cout << "===========================================\nTrace builder output\n";
	for(trace_iterator_t bb = traces.begin(); bb != traces.end(); bb++)
	{
		std::cout << **bb << std::endl;
	}
	#endif

	trace_iterator_t i;

	for(i = begin(); i != end(); i++)
	{
		bb_t* bb = *i;
		switch(bb->getSuccessors().size())
		{
			case 0:
				handle_no_succ_bb(bb);
				break;
			case 1:
				handle_one_succ_bb(bb);
				break;
			case 2:
				handle_two_succ_bb(bb);
			default:
				//switch
				break;
		}
	}
	 
	delete bbs_ptr;
}
#endif
