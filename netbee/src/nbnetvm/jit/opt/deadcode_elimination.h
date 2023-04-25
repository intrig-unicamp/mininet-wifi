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
#include "jit_internals.h"
#include <cassert>
#include <iostream>


namespace jit{
	namespace opt{

		template<class IR>
			class DeadcodeElimination: public OptimizationStep<jit::CFG<IR> >
		{
			public:
				DeadcodeElimination(jit::CFG<IR> &cfg);
				void start(bool &changed);

		};

		template<class IR>
			struct InstructionPosition
			{
				uint32_t BBId;
				IR* nodeP;
			};


		template<class IR>
			DeadcodeElimination<IR>::DeadcodeElimination(jit::CFG<IR> &cfg): OptimizationStep<jit::CFG<IR> >(cfg) {};

		template<class IR>
			struct FillInstructionTable
			{
				typedef std::map<typename IR::RegType, InstructionPosition<IR>*> InstrMapType;
				typedef std::set<typename IR::RegType> IRSetType;
				typedef typename std::set<typename IR::RegType>::iterator IRSetItType;

				InstrMapType &_map;
				FillInstructionTable(InstrMapType &map): _map(map) {};

				void operator()(IR *node)
				{
					//FIXME [PV]: I think only register defined by statement could be unused
					// every register defined by a node that has a father is used by the father itself.
					// For this reason we should look for unused register only in root nodes

					//			for(typename IR::IRNodeIterator i = node->nodeBegin(); i != node->nodeEnd(); i++)
					//			{
					IRSetType regSet = node->getDefs();	
					InstructionPosition<IR> *pos = new InstructionPosition<IR>();
					pos->BBId = node->belongsTo();
					pos->nodeP = node;
					for(IRSetItType regIt = regSet.begin(); regIt != regSet.end(); regIt++)
					{
						//TODO: inserire nella mappa i registri e le posizioni
						_map[*regIt] = pos;
					}
					//			}

				}
			};

		template<class IR>
			struct FindUnusedRegisters
			{
				typedef std::map<typename IR::RegType, InstructionPosition<IR>*> InstrMapType;
				typedef std::set<typename IR::RegType> IRSetType;
				typedef typename std::set<typename IR::RegType>::iterator IRSetItType;

				InstrMapType &_map;
				FindUnusedRegisters(InstrMapType &map): _map(map) {};

				void operator()(IR *node)
				{
					for(typename IR::IRNodeIterator i = node->nodeBegin(); i != node->nodeEnd(); i++)
					{
						IRSetType usesSet = (*i)->getUses();
						for(IRSetItType setIt = usesSet.begin(); setIt != usesSet.end(); setIt++)
						{
							InstructionPosition<IR> *iPos = _map[*setIt];
							if(iPos != NULL)
							{
								MIRNode *node = iPos->nodeP;
								IRSetType defs = node->getDefs();
#ifdef _DEBUG_OPTIMIZER
								std::cout << "Rimuovo dall mappa tutti i registri definiti dall'istruzione";
								node->printNode(std::cout, true);
								std::cout << std::endl;
#endif
								for(IRSetItType defsIt = defs.begin(); defsIt != defs.end(); defsIt++)
								{
#ifdef _DEBUG_OPTIMIZER
									std::cout << "Rimuovo " << *defsIt << std::endl;
#endif
									_map.erase(*defsIt);	
								}
							}
						}
					}	
				}

			};

		template<class IR>
			void KillInstructions(std::map<typename IR::RegType, InstructionPosition<IR>*> &map, 
					jit::CFG<IR> &cfg,
					bool &changed)
			{
				typedef typename std::map<typename IR::RegType, InstructionPosition<IR>*>::iterator MapIteratorType;
				typedef typename std::list<IR*>::iterator stmtIteratorType;

				MapIteratorType i;
				stmtIteratorType si;

				for(i = map.begin(); i != map.end(); i++)
				{
					InstructionPosition<IR> *pos = (*i).second;
					if(pos != NULL)
					{
						BasicBlock<IR> *bb = cfg.getBBById(pos->BBId);	
						std::list<IR*> &codelist = bb->getCode();
						si = std::find(codelist.begin(), codelist.end(), pos->nodeP);
#ifdef _DEBUG_OPTIMIZER
						std::cout << "BBID: " << pos->BBId << " registro inutilizzato: " << (*i).first << std::endl;
#endif
						if(si != codelist.end() && !(*si)->has_side_effects())
						{
							unsigned int size = codelist.size();

#ifdef _DEBUG_OPTIMIZER
							std::cout << "Rimuovo l'istruzione:\n";
							(*si)->printNode(std::cout, true);
							std::cout << std::endl;
#endif
							delete *si;
							codelist.erase(si);
							assert(codelist.size() == (size -1) );
							changed = true;
						}
					}
				}
			}

		template<class IR>
			bool isRedundantCopy(IR *node)
			{
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
					_CFG _cfg;
				public:
					KillRedundantCopy(_CFG &cfg): nvmJitFunctionI("Kill redundant copy"),  _cfg(cfg) {};
					bool run();

			};


		template<class IR>
			void DeadcodeElimination<IR>::start(bool &changed)
			{

				std::map<typename IR::RegType, InstructionPosition<IR>*> instrMap;

				// fill the table with every defined register and the position
				// in the code
				FillInstructionTable<IR> fit(instrMap);
				visitCfgNodes(this->_cfg, &fit);

				// remove table entry when use of the reg associated with that entry is found
				FindUnusedRegisters<IR> fur(instrMap);
				visitCfgNodes(this->_cfg, &fur);	

				//TODO implement this
				// kill instructions that produce a register that is never used
				KillInstructions<IR>(instrMap, this->_cfg, changed);
				// delete instructions that are copy of itselfs
				KillRedundantCopy<IR> krc(this->_cfg); // <- DOES NOT WORK WITH SSA FORM...
				krc.run();
			}

		template<class IR>
			bool KillRedundantCopy::run()
			{
				typedef typename std::list<BasicBlock<IR>* >::iterator _BBIt;
				typedef typename std::list<IR*>::iterator stmtIt;

				std::list<BasicBlock<IR>* > *BBlist;
				IR *toRemoveNode;
				BBlist = cfg.getBBList();		
				for(_BBIt i = BBlist->begin(); i != BBlist->end(); i++)
				{
					std::list<IR*> &codelist = (*i)->getCode();

					for(stmtIt j = codelist.begin(); j != codelist.end(); )
					{
						if(isRedundantCopy(*j) && !(*j)->has_side_effects())
						{
							toRemoveNode = *j;
#ifdef _DEBUG_OPTIMIZER
							std::cout << "Rimuovo l'istruzione:\n";
							toRemoveNode->printNode(std::cout, true);
							std::cout << std::endl;
#endif
							j = codelist.erase(j);
							delete toRemoveNode;
						}
						else
							j++;
					}
				}


			};
	}	/* OPT */
}	/* JIT */

#endif
