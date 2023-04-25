/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*
 * \file deadcode_elimination.h This file contains declaration of DeadcodeElimination class
 */
#ifndef DEADCODE_ELIMINATION_H
#define DEADCODE_ELIMINATION_H
#include "cfg.h"
#include "optimization_step.h"
#include "function_interface.h"
#include "irnode.h"
#include <cassert>
#include <iostream>


namespace jit{
namespace opt{

	template <class _CFG>
		bool manage_phi_node(typename _CFG::IRType::PhiType *node, std::map<typename _CFG::IRType::RegType, typename _CFG::IRType * > &phi_def)
		{
			typedef typename _CFG::IRType::PhiType PhiType;
			typedef typename PhiType::params_iterator p_it;

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Itero sui parametri del nodo phi" << std::endl;
#endif
			for(p_it p = node->paramsBegin(); p != node->paramsEnd(); p++)
			{
#ifdef _DEBUG_OPTIMIZER
			std::cout << "Parametro: " << (*p).second << std::endl;
#endif
				if(phi_def.find((*p).second) != phi_def.end())
				{
					PhiType *phi_node = dynamic_cast<PhiType*>(phi_def[(*p).second]);
					assert(phi_node != NULL);
					if(!phi_node->template getProperty<bool>("useful"))
					{
						phi_node->template setProperty<bool>("useful", true); 
#ifdef _DEBUG_OPTIMIZER
						std::cout << "Il nodo ";
						phi_node->printNode(std::cout, true);
						std::cout << "è utile" << std::endl;					
#endif
						manage_phi_node<_CFG>(phi_node, phi_def);
					}
				}
			}
			return true;
		}

	template<class _CFG>
	class DeadcodeElimination2: public OptimizationStep<_CFG >, public nvmJitFunctionI
	{
		typedef _CFG CFG;
		typedef typename CFG::BBType 	BBType;
		typedef typename CFG::IRType	IR;
		public:
			DeadcodeElimination2(_CFG &cfg);
			void start(bool &changed);
			bool run();	
#ifdef ENABLE_COMPILER_PROFILING
			public:
				static uint32_t numModifiedNodes;
				static void incNumModifiedNodes();
				static uint32_t getModifiedNodes();
#endif
	};
#ifdef ENABLE_COMPILER_PROFILING
	template <class _CFG>
		uint32_t DeadcodeElimination2<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void DeadcodeElimination2<_CFG>::incNumModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t DeadcodeElimination2<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif

	template<class _CFG>
	DeadcodeElimination2<_CFG>::DeadcodeElimination2(_CFG &cfg): OptimizationStep<_CFG >(cfg) {};


	template<class _CFG>
	struct FillPhiDefTable
	{
		typedef typename _CFG::IRType IR;
		typedef typename IR::PhiType PhiType;
		typedef std::map<typename IR::RegType, IR * > PhiMapType;

		PhiMapType & _phi_def_map;
		FillPhiDefTable(PhiMapType &phi_def_map): _phi_def_map(phi_def_map) {};

		void operator()(IR *node)
		{
			if(node->isPhi())
			{
				PhiType *phi_node = dynamic_cast<PhiType*>(node);
				_phi_def_map[phi_node->getDefReg()] = node;
			}
		
		}
	};

	template<class _CFG>
	struct FillInstructionTable
	{

		typedef typename _CFG::IRType IR;
		typedef typename IR::PhiType PhiType;
		typedef std::map<typename IR::RegType, typename std::set<IR *> > InstrMapType;
		typedef std::map<typename IR::RegType, IR * > PhiMapType;
		typedef std::set<typename IR::RegType> IRSetType;
		typedef typename std::set<typename IR::RegType>::iterator IRSetItType;
		typedef typename PhiType::params_iterator p_it;

		InstrMapType & _map;
		PhiMapType & _phi_def_map;
	
		FillInstructionTable(InstrMapType &map, PhiMapType &phi_def_map): _map(map), _phi_def_map(phi_def_map) {};

		void operator()(IR *node)
		{
#ifdef _DEBUG_OPTIMIZER
			std::cout << "Considero il nodo: " << std::endl;
			node->printNode(std::cout, true);
			std::cout << "USI: " << std::endl;
#endif
			
			IRSetType regSet = node->getAllUses();
			typename IRSetType::iterator i;

			for(i = regSet.begin(); i != regSet.end(); ++i)
			{
#ifdef _DEBUG_OPTIMIZER
				std::cout << *i << std::endl;
#endif 
				_map[*i].insert(node);
				PhiType *phi_node;
				if(_phi_def_map.find(*i) != _phi_def_map.end() && !node->isPhi())
				{
					//only real uses should enable phi usefulness
					phi_node = dynamic_cast<PhiType*>(_phi_def_map[*i]);
					assert(phi_node != NULL && "Dynamic cast failed");
					phi_node->template setProperty<bool>("useful", true);
#ifdef _DEBUG_OPTIMIZER
					std::cout << "Il nodo ";
					phi_node->printNode(std::cout, true);
					std::cout << "è utile" << std::endl;					
#endif
					manage_phi_node<_CFG>(phi_node, _phi_def_map);
				}
			}
#ifdef _DEBUG_OPTIMIZER
			std::cout << std::endl;
#endif 
		}
	};

	template<class _CFG>
	struct FindUnusedRegisters
	{
		typedef typename _CFG::IRType IR;

		typedef std::map<typename IR::RegType, std::set<IR *> > InstrMapType;
		typedef std::set<typename IR::RegType> IRSetType;
		typedef typename std::set<typename IR::RegType>::iterator IRSetItType;

		InstrMapType &_map;
		FindUnusedRegisters(InstrMapType &map): _map(map) {};

		void operator()(IR *node)
		{
		}

	};

	template<class _CFG>
	void KillInstructions(std::map<typename _CFG::IRType::RegType, std::set<typename _CFG::IRType * /*, InstructionPosition<IR>*/> > &map, 
			_CFG &cfg,
			bool &changed)
	{
		typedef _CFG CFG;
		typedef typename CFG::BBType BBType;
		typedef typename CFG::IRType IR;
		typedef typename IR::PhiType PhiType;
		typedef typename std::list<IR*>::iterator stmtIteratorType;
		typedef std::set<typename IR::RegType> IRSetType;
		
		std::list<BBType *> *bblist(cfg.getBBList());
		typename std::list<BBType *>::iterator i;

#ifdef _DEBUG_OPTIMIZER
		std::cout << std::endl << "********Kill Instructions********" << std::endl << std::endl;
#endif
		
		for(i = bblist->begin(); i != bblist->end(); ++i) {
			std::list<IR *> &code((*i)->getCode());
			
			stmtIteratorType j, k;
#ifdef _DEBUG_OPTIMIZER
			std::cout << "BB ID: " << (*i)->getId() << std::endl;
#endif
			
			for(j = code.begin(); j != code.end();) {

				(*j)->setProperty("killed", false);

#ifdef _DEBUG_OPTIMIZER
				std::cout << "Considero lo statement ";
				(*j)->printNode(std::cout, true);
				std::cout << '\n';
#endif
			
				if((*j)->has_side_effects() || 
						(*j)->isJump() ) {
#ifdef _DEBUG_OPTIMIZER
					std::cout << "L'istruzione ha effetti collaterali e veràà saltata" << std::endl;
#endif
					++j;
					continue;
				}

				if((*j)->getTree() && !(*j)->getTree()->getDefs().size())
				{
#ifdef _DEBUG_OPTIMIZER
					std::cout << "L'istruzione non definisce registri e veràà saltata" << std::endl;
#endif
					++j;
					continue;
				}

				if((*j)->isPhi())
				{
					PhiType *phi_node = dynamic_cast<PhiType*>(*j);
					if(!phi_node->template getProperty<bool>("useful"))
						phi_node->template setProperty<bool>("killed", true);
					j++;
					continue;
				}

			
				if(!(*j)->getTree())
				{
					j++;
					continue;
				}
				IRSetType defs((*j)->getTree()->getAllDefs());

#ifdef _DEBUG_OPTIMIZER
				std::cout << "# Registri definiti" << defs.size() << std::endl;
#endif
				
				if(defs.size() == 0) { // no registers defined
					++j;
					continue;
				}

				
				
				typename IRSetType::iterator l;
				bool skip(false);
				for(l = defs.begin(); l != defs.end(); ++l) {
#ifdef _DEBUG_OPTIMIZER
					std::cout << "Definizione di " << *l << '\n';
#endif				
					std::set<IR *> uses(map[*l]);
					uses.erase(*j);
					if(uses.size()) { // this node defines a used register
#ifdef _DEBUG_OPTIMIZER
						std::cout << "Non cancello ";
						(*j)->printNode(std::cout, true);
						std::cout << "\na causa di ";
						(*uses.begin())->printNode(std::cout, true);
						std::cout << '\n';
#endif
					
						++j;
						skip = true;
						break;
					}
				}
				if(skip)
					continue;

#ifdef _DEBUG_OPTIMIZER
				std::cout << "Cancello ";
				(*j)->printNode(std::cout, true);
				std::cout << '\n';
#endif
				
				(*j)->setProperty("killed", true);
				++j;
				
				changed = true;
			}
		}
		for(i = bblist->begin(); i != bblist->end(); ++i) {
			std::list<IR *> &code((*i)->getCode());
			
			stmtIteratorType j, k;
			
			for(j = code.begin(); j != code.end();) {
				if( (*j)->template getProperty<bool>("killed") )
				{
					k = j;

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Elimino effettivamente il nodo: ";
					(*k)->printNode(std::cout, false);
					std::cout << std::endl;
#endif
					IR *to_kill = *k;
					j = code.erase(k);
					delete to_kill;
#ifdef ENABLE_COMPILER_PROFILING
					DeadcodeElimination2<_CFG>::incNumModifiedNodes();
#endif
				}
				else
					j++;
			}
		}
		delete bblist;
	}

	template<class _CFG>
		bool isRedundantCopy(typename _CFG::IRType *node)
		{
			typedef typename _CFG::IRType IR;
			typedef std::list<std::pair<typename IR::RegType, typename IR::RegType> > pairlist;
			typedef typename pairlist::iterator pairlistIt;

			pairlist copylist;
			bool allEq = false;
			copylist = node->getCopiedPair();
			if(node->isCopy() && copylist.size() != 0) // the node is not a move
			{
				allEq = true;
				for(pairlistIt i = copylist.begin(); i != copylist.end(); i++)
				{
					if( (*i).first != (*i).second) // if one pair contains different register the node cannot be removed
						allEq = false;
				}
			}

			return allEq;
		}

	template<class _CFG>
		class KillRedundantCopy: public nvmJitFunctionI
		{
			private:
				typedef _CFG CFG;
				typedef typename CFG::IRType IR;
				typedef typename CFG::BBType BBType;

				_CFG &_cfg;
			public:
				KillRedundantCopy(_CFG &cfg): nvmJitFunctionI("Kill redundant copy"),  _cfg(cfg) {};

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
		uint32_t KillRedundantCopy<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void KillRedundantCopy<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t KillRedundantCopy<_CFG>::getModifiedNodes() { return numModifiedNodes; }
#endif

	template <class _CFG>
		bool KillRedundantCopy<_CFG>::run()
				{
					typedef typename std::list<BBType* >::iterator _BBIt;
					typedef typename std::list<IR*>::iterator stmtIt;

					std::list<BBType* > *BBlist;
					IR *toRemoveNode;
					BBlist = _cfg.getBBList();		
					for(_BBIt i = BBlist->begin(); i != BBlist->end(); i++)
					{
						std::list<IR*> &codelist = (*i)->getCode();

						for(stmtIt j = codelist.begin(); j != codelist.end(); )
						{

							if(isRedundantCopy<CFG>(*j) && !(*j)->has_side_effects())
							{
								toRemoveNode = *j;
#ifdef _DEBUG_OPTIMIZER
								std::cout << "Redundant: Rimuovo l'istruzione:\n";
								toRemoveNode->printNode(std::cout, true);
								std::cout << std::endl;
#endif
								j = codelist.erase(j);
								delete toRemoveNode;
#ifdef ENABLE_COMPILER_PROFILING
								KillRedundantCopy<_CFG>::incModifiedNodes();
#endif
							}
							else
								j++;
						}
					}
					delete BBlist;
					return true;

				}

	template <class _CFG>
		bool DeadcodeElimination2<_CFG>::run()
		{
			bool dummy = false;
			start(dummy);
			return dummy;
		};


	template<class _CFG>
	void DeadcodeElimination2<_CFG>::start(bool &changed)
	{
		
		std::map<typename IR::RegType, std::set<IR * > > instrMap;
		std::map<typename IR::RegType, IR * > PhiDefMap;
		
		// fill the phi definition table
		// this has to be done here!!!
		FillPhiDefTable<_CFG> fpdt(PhiDefMap);
		visitCfgNodes(this->_cfg, &fpdt);

		// fill the table with every defined register and the position
		// in the code
		FillInstructionTable<_CFG> fit(instrMap, PhiDefMap);
		visitCfgNodes(this->_cfg, &fit);
		
		// remove table entry when use of the reg associated with that entry is found
		FindUnusedRegisters<_CFG> fur(instrMap);
		visitCfgNodes(this->_cfg, &fur);	

	
		//TODO implement this
		// kill instructions that produce a register that is never used
		KillInstructions<_CFG>(instrMap, this->_cfg, changed);
		//delete instructions that are copy of itselfs
		KillRedundantCopy<_CFG> krc(this->_cfg); // <- DOES NOT WORK WITH SSA FORM...
		krc.run();
	}
}	/* OPT */
}	/* JIT */

#define DeadcodeElimination DeadcodeElimination2

#endif
