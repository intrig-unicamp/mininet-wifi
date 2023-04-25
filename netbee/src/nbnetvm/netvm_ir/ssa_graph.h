#ifndef __SSA_GRAPH_H__
#define __SSA_GRAPH_H__

#include <map>
#include <list>
#include <set>

#include "cfg.h"
#include "basicblock.h"

namespace jit {

template<typename _CFG>
class SSA_Graph {
  public:
	typedef _CFG CFG;
	typedef typename CFG::IRType IRType;
	typedef typename CFG::RegType RegType;
	typedef typename _CFG::BBType BBType;

  private:
	CFG &cfg;
	std::map<RegType, IRType *> loci;
	std::map<RegType, uint32_t> usage_count;
	BBType *bb;
	
  public:
	SSA_Graph(CFG &cfg);
	SSA_Graph(CFG &cfg, BBType &bb);
	~SSA_Graph();
	
	void dump_usage_count();

	std::pair<BBType *, IRType *> get_def(const RegType &);
	uint32_t count_uses(const RegType &);

}; /* class SSA_Graph */


template<typename _CFG>
SSA_Graph<_CFG>::SSA_Graph(CFG &c, BBType &basicblock) : cfg(c), bb(&basicblock)
{
	// Fill the definition loci map
	std::list<IRType *> &codeList(bb->getCode());
	typename std::list<IRType *>::iterator j;
		
	for(j = codeList.begin(); j != codeList.end(); ++j) {
		typename IRType::IRNodeIterator k((*j)->nodeBegin());
		for(; k != (*j)->nodeEnd(); k++) {
			std::set<RegType> defs((*k)->getDefs());
			typename std::set<RegType>::iterator l;
			for(l = defs.begin(); l != defs.end(); ++l) {
//				assert(loci.find(*l) == loci.end() && "Double definition!");
				if(loci.find(*l) != loci.end()) {
//					std::cout << "WARNING: MULTIPLE DEFINITIONS\n";
				}

				loci[*l] = *k;
#ifdef _DEBUG_SSA
					std::cout << "SSAGraph: aggiungo il nodo alla mappa: " << std::endl;
					(*k)->printNode(std::cout, true);
					std::cout << std::endl;
#endif
			}
								
			std::set<RegType> uses((*k)->getUses());
			for(l = uses.begin(); l != uses.end(); ++l) {
				if(!defs.size() || *((*k)->getOwnReg()) != *l)
					++usage_count[*l];
			}
		}
	}

	return;
}

template<typename _CFG>
SSA_Graph<_CFG>::SSA_Graph(CFG &c) : cfg(c), bb(0)
{
	// Fill the definition loci map
	std::list<BBType* >* bbList(cfg.getBBList());
	typename std::list<BBType *>::iterator i;
	
	for(i = bbList->begin(); i != bbList->end(); ++i) {
		std::list<IRType *> &codeList((*i)->getCode());
		typename std::list<IRType *>::iterator j;
		
		for(j = codeList.begin(); j != codeList.end(); ++j) {
			typename IRType::IRNodeIterator k((*j)->nodeBegin());
			for(; k != (*j)->nodeEnd(); k++) {
				std::set<RegType> defs((*k)->getDefs());
				typename std::set<RegType>::iterator l;
				for(l = defs.begin(); l != defs.end(); ++l) {
					assert(loci.find(*l) == loci.end() && "Double definition!");
					if(loci.find(*l) != loci.end()) {
						std::cout << "WARNING: MULTIPLE DEFINITIONS\n";
					}

					loci[*l] = *k;

#ifdef _DEBUG_SSA
					std::cout << "SSAGraph: aggiungo il nodo alla mappa: " << std::endl;
					(*k)->printNode(std::cout, true);
					std::cout << std::endl;
#endif
				}
								
				std::set<RegType> uses((*k)->getUses());
				for(l = uses.begin(); l != uses.end(); ++l) {
					if(!defs.size() || *((*k)->getOwnReg()) != *l)
						++usage_count[*l];
				}
			}
		}
	}

	delete bbList;
}

template<typename _CFG>
SSA_Graph<_CFG>::~SSA_Graph()
{
	// Nop
	return;
}

template<typename _CFG>
std::pair<typename SSA_Graph<_CFG>::BBType *, typename SSA_Graph<_CFG>::IRType *>
	SSA_Graph<_CFG>::get_def(const typename SSA_Graph<_CFG>::RegType &r)
{
	std::pair<BBType *, IRType *> t((BBType*)NULL, (IRType*)NULL);
	typename std::map<RegType, IRType *>::iterator i;
	
	if((i = loci.find(r)) != loci.end()) {
		t.second = i->second;
		t.first  = cfg.getBBById(i->second->belongsTo());
	}
	
	return t;
}

template<typename _CFG>
void SSA_Graph<_CFG>::dump_usage_count()
{
	typename std::map<RegType, uint32_t>::iterator i;
	for(i = usage_count.begin(); i != usage_count.end(); ++i) {
#ifdef _DEBUG_SSA
		std::cout << i->first << " | " << i->second << '\n';
#endif
	}
	
	return;
}

template<typename _CFG>
uint32_t SSA_Graph<_CFG>::count_uses(const RegType &r)
{
	return usage_count[r];
}

} /* namespace
 jit */

#endif /* __SSA_GRAPH_H__ */
