/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef _COPY_PROPAGATION_H
#define _COPY_PROPAGATION_H
#include "cfg.h"
#include "irnode.h"
#include "optimization_step.h"

namespace jit {
	namespace opt{
		
		template <class _CFG>
			class CopyPropagation: public OptimizationStep<_CFG>
		{
			typedef typename _CFG::IRType IRType;
			typedef typename IRType::RegType RegType;
			typedef typename IRType::LoadType LoadType;
			public:
				CopyPropagation(_CFG & cfg);
				void start(bool &changed);
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
		uint32_t CopyPropagation<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void CopyPropagation<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t CopyPropagation<_CFG>::getModifiedNodes() { return numModifiedNodes; }
#endif

		template <class _CFG>
		struct FillRegMap
		{
			typedef typename _CFG::IRType IRType;
			typedef typename IRType::RegType RegType;
			typedef typename IRType::LoadType LoadType;

			typedef std::map<RegType, RegType> reg_map_t;
			reg_map_t &_map;
			FillRegMap(reg_map_t &map): _map(map) {};

			void operator()(IRType *node)
			{
				assert(node != NULL );
				if(node->isStore() && node->getKid(0)->isLoad())
				{
					_map[node->getDefReg()] = ((LoadType*)node->getKid(0))->getUsed();
#ifdef _DEBUG_OPTIMIZER
					std::cout << "Aggiunto alla mappa[" << node->getDefReg() << "] il registro " << ((LoadType*)node->getKid(0))->getUsed() << std::endl;
#endif
				}
			}
		};

		template <class _CFG>
		struct RewriteRegister
		{
			typedef typename _CFG::IRType IRType;
			typedef typename IRType::RegType RegType;
			typedef typename IRType::LoadType LoadType;

			typedef std::map<RegType, RegType> reg_map_t;
			typedef typename reg_map_t::iterator reg_map_it_t;
			typedef typename IRType::IRNodeIterator node_it_t;
			bool &_changed;
			reg_map_t _map;

			RewriteRegister(bool &changed, reg_map_t &map): _changed(changed), _map(map) {};
			void operator()(IRType *node)
			{
				for(node_it_t i = node->nodeBegin(); i != node->nodeEnd(); i++)
				{
					if(i->isLoad())
					{
						LoadType *ldnode = dynamic_cast<LoadType*>(*i);
						RegType usedreg = ldnode->getUsed();
						reg_map_it_t j = _map.find(usedreg);
						if(j != _map.end()) // register has an alias
						{
#ifdef _DEBUG_OPTIMIZER
							std::cout << "Riscrivo il nodo: ";
							node->printNode(std::cout, true);
							std::cout << std::endl;
#endif
							_changed = true;	
							ldnode->setUsed((*j).second);
#ifdef _DEBUG_OPTIMIZER
							node->printNode(std::cout, true);
							std::cout << std::endl;
#endif
#ifdef ENABLE_COMPILER_PROFILING
							CopyPropagation<_CFG>::incModifiedNodes();
#endif
						}
					}
				}
			}
		};

		template <class _CFG>
		void transitiveClosure(std::map<typename _CFG::IRType::RegType, typename _CFG::IRType::RegType> &map)
		{
			typedef typename _CFG::IRType IRType;
			typedef typename IRType::RegType RegType;
			typedef typename IRType::LoadType LoadType;

			typedef std::map<RegType, RegType> reg_map_t;
			typedef typename reg_map_t::iterator reg_map_it_t;

			RegType tempReg;
			reg_map_it_t j, k;

			for(reg_map_it_t i = map.begin(); i != map.end(); i++)
			{
				j = map.find( (*i).second );
				while( j != map.end())
				{
					map[ (*i).first ] = (*j).second;
					j = map.find( (*j).second );
				}
#ifdef _DEBUG_OPTIMIZER
				std::cout << "L'alias del registro " << (*i).first << "e': " << map[(*i).first] << std::endl;
#endif
			}
		}


		template <class _CFG>
		CopyPropagation<_CFG>::CopyPropagation(_CFG &cfg): OptimizationStep<_CFG>(cfg) {};

		template <class _CFG>
		void CopyPropagation<_CFG>::start(bool &changed)
		{
			std::map<RegType, RegType> reg_map;	

			//fill the map
			FillRegMap<_CFG> frm(reg_map);
			visitCfgNodes(this->_cfg, &frm);		

			//chiusura della mappa
			transitiveClosure<_CFG>(reg_map);
			//constant substitution
			RewriteRegister<_CFG> rr(changed, reg_map);
			visitCfgNodes(this->_cfg, &rr);
		}

		template <class _CFG>
			bool CopyPropagation<_CFG>::run()
			{
				bool dummy = false;
				start(dummy);
				return dummy;
			}
	} /* OPT */
} /* JIT */

#endif
