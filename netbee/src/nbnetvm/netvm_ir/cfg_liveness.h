#ifndef CFG_LIVENESS
#define CFG_LIVENESS

/*!
 * \file cfg_liveness.h
 * \brief This file contains the definition of the routines used to do liveness analysis on a control flow graph
 */

#include "irnode.h"
#include "cfg.h"
#include "register_mapping.h"
#include <algorithm>
#include <set>
//#include "cfg_printer.h"

namespace jit {

	//! a class to compute liveness infomation for a cfg
	template<class _CFG>
		class Liveness
		{
			typedef struct liveness_info LivenessInfo;
			typedef nbBitVector set_t;

			struct liveness_info
			{
				set_t UEVarSet;
				set_t LiveInSet;
				set_t LiveOutSet;
				set_t VarKillSet;

				liveness_info(uint32_t num_regs);
			};

			public:
			typedef _CFG cfg_t;
			typedef typename cfg_t::IRType IR;
			typedef typename IR::RegType RegType;
			typedef std::set< RegType > reg_set_t;
			typedef typename cfg_t::BBType BBType;

			//!constructor
			Liveness(cfg_t& cfg);

			//!destructor
			~Liveness();

			//!the liveness calculation main procedure
			void run();

			//!return a livein set for a bb
			reg_set_t get_LiveInSet(BBType* bb);
			//!return a livein set for a bb
			reg_set_t get_LiveInSet(uint16_t bb);

			//!return a liveout set for a bb
			reg_set_t get_LiveOutSet(BBType* bb);
			//!return a liveout set for a bb
			reg_set_t get_LiveOutSet(uint16_t bb);

			template<typename T>
				friend std::ostream& operator<< (std::ostream& os, const jit::Liveness<T>& liveness);

			private:

			//!compute UEvarset and VarKill set for a bb
			void compute_UEVarSet(BBType* bb);
			//!perform a step in the liveness computation
			bool compute_VarSets(BBType* bb);

			static const uint32_t liveness_space = 4; //!space where the registers of the cfg are mapped

			cfg_t& cfg; //!<cfg to exam
			Register_Mapping<cfg_t> mapper; //!<class to map the register to a dense space
			RegisterManager& rm; //!<class which holds information for the mapping

			std::map<uint16_t, liveness_info* > bb_liveness; //!<map which holds liveness information indexed by bb id

		};

	/*!
	 * \brief a simple struct to call compute_UEVarSet on every node of a CFG
	 */
	template<class IR>
		struct compute_UEVarSet
		{

			typedef typename CFG<IR>::node_t node_t;
			typedef typename IR::RegType RegType;
			//		RegisterManager<RegType>& manager;

			compute_UEVarSet(/* RegisterManager<RegType>& manager */) /* : manager(manager) */ {}

			void operator()(node_t* node)
			{
				node->NodeInfo->compute_UEVarSets(/* manager */);
			}
		};

	/*!
	 * \brief This function computes the live set of each basic block of a CFG
	 * \param cfg the control flow graph
	 *
	 * First we compute every BB's UEVarSet and LiveKillSet. The we iterate on every BB in 
	 * reverse post order and solve the liveness analysis equation till we find no changes.
	 */
//	template<class IR>
//		void cfg_computeLiveSet(CFG<IR>& cfg)
//		{
//			typedef typename CFG<IR>::SortedIterator iterator_t;
//
//			cfg.SortPostorder(*cfg.getEntryNode());
//
//			for(iterator_t i = cfg.FirstNodeSorted(); i != cfg.LastNodeSorted(); i++)
//			{
//				(*i)->NodeInfo->reset_Liveness();
//			}
//
//			std::for_each(cfg.FirstNodeSorted(), cfg.LastNodeSorted(), compute_UEVarSet<IR>(/* cfg.manager */));
//
//			bool changed = true;
//			while(changed)
//			{
//				changed = false;
//				for(iterator_t i = cfg.FirstNodeSorted(); i != cfg.LastNodeSorted(); ++i)
//				{
//					changed = (*i)->NodeInfo->compute_VarSets() || changed;
//
//				}
//			}
//		}

	template<typename T>
		std::ostream& operator<<(std::ostream& os, const jit::Liveness<T>& liveness)
		{
			typename std::map<uint16_t, typename jit::Liveness<T>::liveness_info* >::const_iterator bb;

			for (bb = liveness.bb_liveness.begin(); bb != liveness.bb_liveness.end(); bb++)
			{
				typedef typename jit::Liveness<T>::set_t::iterator iterator_t; 

				os << "BB ID: " << bb->first << std::endl;

				os << "UEVarSet" << std::endl;
				for(iterator_t i = bb->second->UEVarSet.begin(); i != bb->second->UEVarSet.end(); i++)
				{
					os << liveness.rm[RegisterInstance(liveness.liveness_space, *i)] << ",";
				}
				os << std::endl;

				os << "VarKillSet" << std::endl;
				for(iterator_t i = bb->second->VarKillSet.begin(); i != bb->second->VarKillSet.end(); i++)
				{
					os << liveness.rm[RegisterInstance(liveness.liveness_space, *i)] << ",";
				}
				os << std::endl;

				os << "LiveInSet" << std::endl;
				for(iterator_t i = bb->second->LiveInSet.begin(); i != bb->second->LiveInSet.end(); i++)
				{
					os << liveness.rm[RegisterInstance(liveness.liveness_space, *i)] << ",";
				}
				os << std::endl;

				os << "LiveOutSet" << std::endl;
				for(iterator_t i = bb->second->LiveOutSet.begin(); i != bb->second->LiveOutSet.end(); i++)
				{
					os << liveness.rm[RegisterInstance(liveness.liveness_space, *i)] << ",";
				}
				os << std::endl;
				os << std::endl;
			}

			return os;
		}

} //namespace jit

	template<class _CFG>
std::set< typename jit::Liveness<_CFG>::RegType> jit::Liveness<_CFG>::get_LiveInSet(typename jit::Liveness<_CFG>::BBType* bb)
{
	assert(bb != NULL);
	return get_LiveInSet(bb->getId());
}

	template<typename _IR>
std::set< typename jit::Liveness<_IR>::RegType> jit::Liveness<_IR>::get_LiveInSet(uint16_t bb)
{
	liveness_info* linfo = bb_liveness[bb];
	assert(linfo != NULL);

	reg_set_t res;

	typedef typename set_t::iterator iterator_t; 
	
	for(iterator_t i = linfo->LiveInSet.begin(); i != linfo->LiveInSet.end(); i++)
	{
		res.insert(rm[RegisterInstance(liveness_space, *i)]);
	}

	return res;
}

	template<typename _CFG>
std::set< typename jit::Liveness<_CFG>::RegType> jit::Liveness<_CFG>::get_LiveOutSet(typename jit::Liveness<_CFG>::BBType* bb)
{
	assert(bb != NULL);
	return get_LiveOutSet(bb->getId());
}

	template<typename _IR>
std::set< typename jit::Liveness<_IR>::RegType> jit::Liveness<_IR>::get_LiveOutSet(uint16_t bb)
{
	liveness_info* linfo = bb_liveness[bb];
	assert(linfo != NULL);

	reg_set_t res;

	typedef typename set_t::iterator iterator_t; 

	for(iterator_t i = linfo->LiveOutSet.begin(); i != linfo->LiveOutSet.end(); i++)
	{
		res.insert(rm[RegisterInstance(liveness_space, *i)]);
	}

	return res;
}

	template<typename _IR>
jit::Liveness<_IR>::liveness_info::liveness_info(uint32_t num_regs)
	:
		UEVarSet(num_regs),
		LiveInSet(num_regs),
		LiveOutSet(num_regs),
		VarKillSet(num_regs)
{ }

	template<typename _CFG>
void jit::Liveness<_CFG>::compute_UEVarSet(typename jit::Liveness<_CFG>::BBType* bb)
{

	//std::cout << "Compute UE Var Set of BB " << bb->getId() << endl;
	liveness_info *linfo = new liveness_info(rm.getCount());
	typename BBType::IRStmtNodeIterator i;

	for (i = bb->codeBegin(); i != bb->codeEnd(); i++)
	{
		typename IR::IRNodeIterator j((*i)->nodeBegin());
		for(; j != (*i)->nodeEnd(); j++) {

			reg_set_t defs= (*j)->getDefs();
			reg_set_t uses= (*j)->getUses();

			typename reg_set_t::iterator r;

#ifdef _DEBUG_LIVENESS
			std::cout << "Usi di ";
			(*j)->printNode(std::cout, true);
			std::cout << '\n';
#endif

			for(r = uses.begin(); r != uses.end(); r++)
			{
#ifdef _DEBUG_LIVENESS
				std::cout << *r << std::endl;
#endif
				uint32_t index = rm.getName(*r).get_model()->get_name();

				if(!linfo->VarKillSet.Test(index))
				{
					linfo->UEVarSet.Set(index);
				}
			}

#ifdef _DEBUG_LIVENESS
			std::cout << "Definizioni di ";
			(*j)->printNode(std::cout, true);
			std::cout << '\n';
#endif
			for(r = defs.begin(); r != defs.end(); r++)
			{
#ifdef _DEBUG_LIVENESS
				std::cout << *r << std::endl;
#endif
				uint32_t index = rm.getName(*r).get_model()->get_name();
				linfo->VarKillSet.Set(index);
			}
		}
	}

	//std::cerr << "Inserisco i sets del BB: " << bb->getId() << std::endl;
	bb_liveness[bb->getId()] = linfo;
	return;
};

	template<typename _CFG>
bool jit::Liveness<_CFG>::compute_VarSets(typename jit::Liveness<_CFG>::BBType* bb)
{
	liveness_info* linfo = bb_liveness[bb->getId()];
	assert(linfo != NULL);

	set_t before = linfo->LiveOutSet;

	typedef typename BBType::BBIterator bb_iterator;

	for(bb_iterator succ = bb->succBegin(); succ != bb->succEnd(); succ++)
	{
		liveness_info* succ_info = bb_liveness[(*succ)->getId()];
		assert(succ_info != NULL);

		set_t disjoint = succ_info->LiveOutSet - succ_info->VarKillSet;
		succ_info->LiveInSet = succ_info->UEVarSet + disjoint;
		linfo->LiveOutSet += succ_info->LiveInSet;
	}

	return before != linfo->LiveOutSet;
}

	template<typename _CFG>
void jit::Liveness<_CFG>::run()
{
	mapper.define_new_names();

#ifdef _DEBUG_LIVENESS
	std::cerr << "!!!!!!!!!!!!!!1MAPPING DONE!!!!!!!!!!!!!!!!!!\n";
	std::cerr << "number of registers " << rm.getCount() << std::endl;
	std::cerr << "number of bb " << cfg.BBListSize() << std::endl;
#endif

	typedef typename _CFG::SortedIterator sorted_iterator;

	cfg.SortPostorder(*cfg.getEntryNode());

	for(sorted_iterator i = cfg.FirstNodeSorted(); i != cfg.LastNodeSorted(); i++)
	{
		BBType* bb((*i)->NodeInfo);
		assert(bb != NULL);

		compute_UEVarSet(bb);
	}

	//std::cerr << "UE done\n";

	bool changed = true;
	while(changed)
	{
		changed = false;
		for(sorted_iterator i = cfg.FirstNodeSorted(); i != cfg.LastNodeSorted(); ++i)
		{
			BBType* bb((*i)->NodeInfo);
			assert(bb != NULL);

			changed |= compute_VarSets(bb);
		}
	}
}

template<typename _IR>
jit::Liveness<_IR>::~Liveness()
{
	typename std::map<uint16_t, liveness_info*>::iterator i;

	for(i = bb_liveness.begin(); i != bb_liveness.end(); i++)
	{
		delete i->second;
	}

	//reset space and map the cfg
	RegisterModel::reset(liveness_space);
}

	template<typename _CFG>
jit::Liveness<_CFG>::Liveness(_CFG& cfg)
	:
		cfg(cfg),
		mapper(cfg, liveness_space, std::set<uint32_t>()),
		rm(mapper.get_manager())
{ }
#endif
