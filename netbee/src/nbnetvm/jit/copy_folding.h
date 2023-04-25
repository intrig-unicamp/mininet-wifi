/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __COPY_FOLDING_H__
#define __COPY_FOLDING_H__

#include <set>
#include <vector>
#include <map>
#include <list>
#include <iostream>
#include <algorithm>
#include "basicblock.h"
#include "cfg.h"
#include "cfg_liveness.h"
//#include "cfg_printer.h"
#include "ssa_graph.h"
#include "irnode.h"
//#include "jit_internals.h"
#include "function_interface.h"
#include "bitvectorset.h"

using std::min;

namespace jit {

inline static void free_vector(std::vector<nbBitVector*>& bv)
{
	std::vector<nbBitVector*>::iterator i;
	for(i = bv.begin(); i != bv.end(); i++)
	{
		if(*i)
			delete *i;
	}
}


template<typename _CFG>
class Interference_Map {
  public:
	typedef _CFG CFG;
	typedef typename CFG::IRType::RegType RegType;
	typedef std::vector< nbBitVector* > map_t;
	
	Interference_Map(CFG &c);
	~Interference_Map();
	
	const map_t& get_interference();
	
	void print(std::ostream &);
	bool are_interfering(const RegType& a, const RegType& b);
	RegisterManager& get_manager() { return rm; }

  private:
	CFG &cfg;
	static const uint32_t liveness_space = 5;
	Register_Mapping<CFG> mapper;
	RegisterManager& rm;
	
	map_t* interf;
	
	void run();
};

template<typename _CFG>
class Fold_Copies: public nvmJitFunctionI {
  public:
	typedef _CFG CFG;
	typedef typename CFG::IRType::RegType RegType;
	typedef typename CFG::BBType BBType;
	
	struct CheckCompatible {
		virtual ~CheckCompatible() {};
		
		virtual bool operator()(RegType &a, RegType &b) {
//			std::cout << "###CHECK " << a << ", " << b << ": ";
//			std::cout << (a.get_model()->get_name() == b.get_model()->get_name()) << '\n';
//			return a.get_model()->get_name() == b.get_model()->get_name();
			return true;
		}
	};

  private:
	CFG &cfg;
	CheckCompatible *checker;
	

  public:
	Fold_Copies(CFG &c);
	Fold_Copies(CFG &c, CheckCompatible *);
	~Fold_Copies();
	
	bool run();
#ifdef ENABLE_COMPILER_PROFILING
			public:
				static uint32_t numModifiedNodes;
				static void incModifiedNodes();
				static uint32_t getModifiedNodes();
#endif
};
#ifdef ENABLE_COMPILER_PROFILING
	template <class _CFG>
		uint32_t Fold_Copies<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void Fold_Copies<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t Fold_Copies<_CFG>::getModifiedNodes() { return numModifiedNodes; }
#endif

template<typename _CFG>
class Dr_Lanugos_Magical_Device {
  public:
	typedef _CFG CFG;
	typedef typename CFG::IRType::RegType RegType;

  private:
	CFG &cfg;
	SSA_Graph<_CFG> graph;
	RegisterInstance r;
	
	void process_node(typename CFG::IRType *, bool = false);
//	void process_node(typename CFG::IRType *);

  public:
	Dr_Lanugos_Magical_Device(CFG &c);
	~Dr_Lanugos_Magical_Device();
	
	void run();
};

template<typename _CFG>
Dr_Lanugos_Magical_Device<_CFG>::Dr_Lanugos_Magical_Device(CFG &c) : cfg(c), graph(c), r(RegType::invalid)
{
	graph.dump_usage_count();

	return;
}

template<typename _CFG>
Dr_Lanugos_Magical_Device<_CFG>::~Dr_Lanugos_Magical_Device()
{
	// Nop
	return;
}

template<typename _CFG>
void Dr_Lanugos_Magical_Device<_CFG>::run() 
{
	typedef typename _CFG::BBType BBType;
	// Scan each BB, order not relevant (liveness information suffices)
	typename std::list<BBType* >::iterator i;
	std::list<BBType *> *bblist(cfg.getBBList());

	for(i = bblist->begin(); i != bblist->end(); ++i) {
		// Scan each node in a BB, in reverse order
		BBType *bb(*i);
		
		std::list<typename CFG::IRType *>& nodes(bb->getCode());
		
		typename std::list<typename CFG::IRType *>::iterator ni;
		
		for(ni = nodes.begin(); ni != nodes.end(); ++ni) {
//			std::cout << "nodo radice: " << **ni << '\n';
			process_node((*ni)->getKid(0), true);
			process_node((*ni)->getKid(1), true);
#if 0
			typename std::set<RegType> defs((*ni)->getDefs());
			if(!(*ni)->getOwnReg() || defs.find(*((*ni)->getOwnReg())) == defs.end()) {
				process_node((*ni)->getKid(0));
				process_node((*ni)->getKid(1));
			} else {
				RegType r(*((*ni)->getOwnReg()));
		
				process_node((*ni)->getKid(0), r);
				process_node((*ni)->getKid(1), r);
			}
#endif
		}		
	}
	
	return;
}

template<typename _CFG>
void Dr_Lanugos_Magical_Device<_CFG>::process_node(typename _CFG::IRType *n, bool root)
{
	if(!n)
		return;
		
//	std::cout << "esamino " << *n << '\n';
	
	if(r == RegType::invalid) {
		typename std::set<RegType> defs(n->getDefs());
		if(/*root || */ !n->getOwnReg() || defs.find(*(n->getOwnReg())) == defs.end()) {
//			std::cout << "passo oltre al nodo " << *n << '\n';
			process_node(n->getKid(0));
			process_node(n->getKid(1));
		} else {
			if(r == RegType::invalid && n->getOwnReg() && graph.count_uses(*(n->getOwnReg())) == 1) {
				r = *(n->getOwnReg());
//				std::cout << "inizio ad usare come default il registro " << r << '\n';
			}

			process_node(n->getKid(0));
			process_node(n->getKid(1));
		}
	} else {
//		std::cout << "scendo col registro " << r << " in " << *n << '\n';

		typename std::set<RegType> defs(n->getDefs());
		if(defs.find(*(n->getOwnReg())) != defs.end()) {
			if(graph.count_uses(*(n->getOwnReg())) == 1) {
//				std::cout << "e sovrascrivo\n";
				n->setDefReg(r);
			}
		}

		process_node(n->getKid(0));
		process_node(n->getKid(1));
	}
	
	return;

}

#if 0
template<typename _CFG>
void Dr_Lanugos_Magical_Device<_CFG>::process_node(typename CFG::IRType *n, RegType r)
{
	if(!n)
		return;
		
	
	return;
}
#endif



template<typename _CFG>
Interference_Map<_CFG>::Interference_Map(CFG &c) : cfg(c), mapper(cfg, liveness_space, std::set<uint32_t>()), rm(mapper.get_manager())
{
	// Nop
	return;
}

template<typename _CFG>
Interference_Map<_CFG>::~Interference_Map()
{
	if(interf)
	{
		free_vector(*interf);
		delete interf;
	}
}

template<typename _CFG>
void Interference_Map<_CFG>::print(std::ostream &os)
{
	
	for(uint32_t w = 0; w != interf->size(); ++w) {
		os << rm[ RegType(5, w) ] << " | ";

		nbBitVector* v = (*interf)[w];

		if (v)
		{
			for(nbBitVector::iterator i = v->begin(); i != v->end(); ++i)
			{
				os << rm[ RegType(5, *i) ] << ' ';
			}
		}
			
		os << '\n';
	}
	
	return;
}

template<typename _CFG>
const typename jit::Interference_Map<_CFG>::map_t&
Interference_Map<_CFG>::get_interference()
{
	mapper.define_new_names();
	interf = new std::vector< nbBitVector* >(rm.getCount());
	run();
	return *interf;
}

template<typename _CFG>
void Interference_Map<_CFG>::run() 
{
	/* Precondition: the CFG is in a 'almost-SSA form'. This means that every variable is defined at most
	 * once in each BB and that the only variables defined more than once are those that are phi-defined.
	 * When the exit-from-SSA algorithm is run the CFG is left in the correct form.
	 */

	typedef std::set<RegType> set_t;
	typedef typename set_t::iterator set_it_t;
	typedef typename _CFG::IRType IRType;
	typedef typename _CFG::BBType BBType;

	jit::Liveness<_CFG> liveness(cfg);
	liveness.run();

	mapper.define_new_names();

	map_t& interference = *interf;

	// Scan each BB, order not relevant (liveness information suffices)
	typename std::list< BBType* >::iterator i;
	std::list<BBType*> *bblist(cfg.getBBList());

	for(i = bblist->begin(); i != bblist->end(); ++i) {
		// Scan each node in a BB, in reverse order
		BBType *bb(*i);

		// We start having every live out variable alive
		nbBitVector live(rm.getCount());

		//initialize initial live set
		{
			set_t liveOut(liveness.get_LiveOutSet(bb));
			set_it_t li;

			for(li = liveOut.begin(); li != liveOut.end(); li++ )
			{
				live.Set(rm.getName(*li).get_model()->get_name());
			}
		}

		std::list<IRType *>& nodes(bb->getCode());
		typename std::list<IRType *>::iterator ni;
		nodes.reverse();
		
		for(ni = nodes.begin(); ni != nodes.end(); ++ni) {
			typename IRType::ContType instr_list((*ni)->get_reverse_instruction_list());
			typename IRType::ContType::iterator ii;

			for(ii = instr_list.begin(); ii != instr_list.end(); ++ii) {
				typedef nbBitVector::iterator it_t;

				/* Rules:
				* 1) If a variable is not live out, then its last usage is the first encountered.
				* 2) If a variable is not live in, then its definition marks the place it is born.
				* 3) Two variables used in the same node interfere.
				* 4) A variable defined in a node does not interefere with variables used in the same node.
				*/
				// THE COMMENT ABOVE IS NO LONGER RELEVANT
				
				set_t uses((*ii)->getUses()), defs((*ii)->getDefs());

				for(it_t j = live.begin(); j != live.end(); ++j) {
					
					uint32_t name = *j;

					nbBitVector *interf = interference[name];
					if(interf == NULL)
					{
						interf = new nbBitVector(rm.getCount());
						interference[name] = interf;
					}

					*interf += live;

					interf->Reset(name);
				}

				// We've seen for sure the last use of every used variable (rule 1), so from now on it is live.
				//live.insert(uses.begin(), uses.end());

				set_it_t li;

				for(li = uses.begin(); li != uses.end(); li++ )
				{
					live.Set(rm.getName(*li).get_model()->get_name());
				}

				// First of all, the variables defined here are killed (by rule 2), so from now on they are not live any longer				
				for(li = defs.begin(); li != defs.end(); li++ )
				{
					live.Reset(rm.getName(*li).get_model()->get_name());
				}

			}
		}
		nodes.reverse();
	}

	delete bblist;

	return;
}

template<typename _CFG>
bool Interference_Map<_CFG>::are_interfering(const RegType& a, const RegType& b)
{
	nbBitVector* interference = (*interf)[rm.getName(a).get_model()->get_name()];
	if(interference == NULL)
		return false;

	return interference->Test(rm.getName(b).get_model()->get_name());
}


//template<typename _CFG>
//typename Fold_Copies<_CFG>::CheckCompatible Fold_Copies<_CFG>::default_checker;

template<typename _CFG>
Fold_Copies<_CFG>::Fold_Copies(CFG &c) : nvmJitFunctionI("Fold copies"),  cfg(c), checker(new CheckCompatible())
{
	// Nop
	return;
}

template<typename _CFG>
Fold_Copies<_CFG>::Fold_Copies(CFG &c, CheckCompatible *ch) : nvmJitFunctionI("Fold copies"),  cfg(c), checker(ch)
{
	// Nop
	return;
}

template<typename _CFG>
Fold_Copies<_CFG>::~Fold_Copies()
{
	delete checker;
	return;
}

template<typename _CFG>
bool Fold_Copies<_CFG>::run()
{

	// ----- First pass - build the interference graph
	Interference_Map<CFG> im(cfg);
	const typename Interference_Map<CFG>::map_t& interference = im.get_interference();
	RegisterManager& rm = im.get_manager();
	
	// DEBUG: print the interference graph
#ifdef _DEBUG_SSA
	std::cout << "Risultati prima fase - grafo interferenze:\n";
	im.print(std::cout);
#endif
	
	typename std::list<BBType* >::iterator i;
	std::list<BBType *> *bblist(cfg.getBBList());

	// ----- Second pass - scan for copies
	std::vector<nbBitVector*> aliases(rm.getCount());

	for(i = bblist->begin(); i != bblist->end(); ++i) {
		// Scan each node in a BB, order is not relevant
		BBType *bb(*i);
		
		std::list<typename CFG::IRType *>& nodes(bb->getCode());
		typename std::list<typename CFG::IRType *>::iterator ni;
		
		for(ni = nodes.begin(); ni != nodes.end(); ++ni) {
			typename CFG::IRType::ContType instr_list((*ni)->get_reverse_instruction_list());
			typename CFG::IRType::ContType::iterator ii;

			for(ii = instr_list.begin(); ii != instr_list.end(); ++ii) {
				std::set<RegType> uses((*ii)->getUses()), defs((*ii)->getDefs());

				std::list<std::pair<RegType, RegType> > copies((*ii)->getCopiedPair());
				typename std::list<std::pair<RegType, RegType> >::iterator j;

				//					assert(copies.size() != 0 && "Copy without registers being used/defined!");

				for(j = copies.begin(); j != copies.end(); ++j) {
					// If the two do not interfere, then one is an alias of the other
					if(!im.are_interfering(j->first, j->second) &&
							!im.are_interfering(j->second, j->first) &&
							(*checker)(j->first, j->second)) {
						uint32_t first  = rm.getName(j->first).get_model()->get_name();
						uint32_t second = rm.getName(j->second).get_model()->get_name();

						nbBitVector* alias;

						alias = aliases[first];
						if(alias == NULL)
						{
							alias = new nbBitVector(rm.getCount());
							aliases[first] = alias;
						}
						alias->Set(second);

						alias = aliases[second];
						if(alias == NULL)
						{
							alias = new nbBitVector(rm.getCount());
							aliases[second] = alias;
						}
						alias->Set(first);
					} 
				}
			}
		}
	}	
	
	// ----- Now a closure on the aliases set is performed, and not-interfering ones are merged
	std::vector<nbBitVector*> rewrite_set (rm.getCount()+1);
	std::vector<RegType> rewrite(rm.getCount()+1);
	//std::map<RegType, RegType> rewrite;
	//
	
	for(uint32_t mi = 0; mi != rm.getCount(); ++mi) {
		// For every alias set, merge the names, merge the interferences and find the new name
		
		// Avoid creating a rewrite set for this register if it is already in another register
		// rewrite set
		if(rewrite[mi] != RegType::invalid)
			continue;
		
		nbBitVector current_aliases(rm.getCount());
		nbBitVector current_interfs(rm.getCount());

		std::list< nbBitVector* > work_list;

		if(aliases[mi])
			work_list.push_back(aliases[mi]);

		current_aliases.Set(mi);
		if(interference[mi])
			current_interfs = *interference[mi];
		
		while(work_list.size()) {
			nbBitVector* c(work_list.front());
			work_list.pop_front();
		
			if(c->empty())
				continue;

			// C is a set of registers that are copy-defined with the register 'name'
			// Within this set we can merge everything that does not interfere
			for(nbBitVector::iterator ci = c->begin(); ci != c->end(); ++ci) {
				if(current_interfs.Test(*ci))
					continue;
					
				// We have a merge candidate.
				if(current_aliases.Test(*ci))
					continue;
				
				// If it is already in another set, don't touch it!
				if(rewrite[*ci] != RegType::invalid)
					continue;
							
				// Ok, it was not already in the set, so we add it
				current_aliases.Set(*ci);
				
				// And we add its interferences as well
				if(interference[*ci])
					current_interfs += *interference[*ci];
				
				// This register's aliases make good candidates. Insert them into the worklist
				if(aliases[*ci])
					work_list.push_back(aliases[*ci]);
			}
		}
		
		uint32_t name = *(current_aliases.begin());
		
		// Fill the rewrite_set table
		nbBitVector* r_set = rewrite_set[name];
		if(!r_set)
		{
			r_set = new nbBitVector(rm.getCount());
			rewrite_set[name] = r_set;
		}
		*r_set += current_aliases;
		
		// Fill the rewrite table
		assert(!current_aliases.empty());

		RegType reg( rm[ RegType(5, *current_aliases.begin()) ] );
		for(nbBitVector::iterator it = current_aliases.begin(); it != current_aliases.end(); ++it) {
			rewrite[*it] = reg;
		}
	}

	// ----- Third pass - rename
	for(i = bblist->begin(); i != bblist->end(); ++i) {
		// Scan each node in a BB, order is not relevant
		BBType *bb(*i);
		
		std::list<typename CFG::IRType *>& nodes(bb->getCode());
		typename std::list<typename CFG::IRType *>::iterator ni;
		
		for(ni = nodes.begin(); ni != nodes.end(); ++ni) {
			typename CFG::IRType::ContType instr_list((*ni)->get_reverse_instruction_list());
			typename CFG::IRType::ContType::iterator ii;

			for(ii = instr_list.begin(); ii != instr_list.end(); ++ii) {
				// Rewrite uses first
				std::set<typename CFG::IRType::RegType> uses((*ii)->getUses());
				typename std::set<typename CFG::IRType::RegType>::iterator j;
				
				for(j = uses.begin(); j != uses.end(); ++j) {
					uint32_t j_name = rm.getName(*j).get_model()->get_name();
					if(rewrite[j_name] != RegType::invalid) {
						(*ii)->rewrite_use(*j, rewrite[j_name]);
					}
				}
			
				// Then rewrite eventual definitions
				RegType *r((*ii)->getOwnReg());

				if(r)
				{
					uint32_t r_name = rm.getName(*r).get_model()->get_name();

					if(rewrite[r_name] != RegType::invalid) {
						*r = rewrite[r_name];
					}
				}
			}
		}
	}

	free_vector(aliases);
	free_vector(rewrite_set);
	delete bblist;

	return true;
}


//	Liveness<IRType> liveness;
} /* namespace jit */

#endif
