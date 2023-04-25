/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef CONTROLFLOW_SIMPLIFICATION_H
#define CONTROLFLOW_SIMPLIFICATION_H

#include "optimization_step.h"
#include "cfg.h"
#include "irnode.h"

#ifdef _JIT_BUILD
#include "jit_internals.h"
#endif
#ifdef _PFL_BUILD
#include "tree.h"
#endif

namespace jit{
	namespace opt{
		/*!
		 * \brief Optimizaton step to perform Control Flow simplification
		 *
		 * This optimization looks for branches evaluated on conditions that are constants
		 * and so their values are known at compile time. If found, the compiler evaluates the
		 * condition and modifies branch instructions to jump directly to the right target.
		 * This step can lead to a situation in which BasicBlock has no more predecessors and
		 * cannot be reached. Further step is to remove that basicblocks from control flow graph
		 */
		template <class _CFG>
		class ControlFlowSimplification: public OptimizationStep<_CFG>
		{
			typedef _CFG CFG;
			typedef typename CFG::IRType IRType;
			typedef typename CFG::BBType BBType;
			typedef typename IRType::RegType RegType;
			typedef typename IRType::JumpType JumpType;
			typedef typename IRType::SwitchType SwitchType;
			typedef typename IRType::ConstType ConstType;

			public:
				//! Constructor that takes a reference to the cfg
				ControlFlowSimplification(_CFG &cfg): OptimizationStep<_CFG>(cfg) {};

				void start(bool &);

				//! method called for each statement from visitCfgNodes function
				void operator()(IRType *node);
			private:
				bool *_changed;
				//! analizes the node and evaluates conditions. Called if the node has both kids constant
				void bothKidConstant(JumpType *node);
				//! analizes the node and evaluates conditions. Called if the node has one kid constant
				void oneKidConstant(JumpType *node);

				//!remove useless switch
				void handleSwitch(SwitchType *node);
				//!remove switch with constant and replace with single jump
				JumpType* handle_constant_switch(SwitchType *node);
				//!remove switch with only one case replace with a single conditioned jump
				JumpType* handle_two_succ_switch(SwitchType *node);
				//! updates references in lists
				void updateSuccPredLists(IRType *node, IRType *newnode, uint32_t nonReachedBB);
#ifdef ENABLE_COMPILER_PROFILING
			public:
				static uint32_t numModifiedNodes;
				static void incModifiedNodes();
				static uint32_t getModifiedNodes();
#endif
		};
#ifdef ENABLE_COMPILER_PROFILING
	template <class _CFG>
		uint32_t ControlFlowSimplification<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void ControlFlowSimplification<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t ControlFlowSimplification<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif



		template <class _CFG>
		class JumpToJumpElimination: public OptimizationStep<_CFG>, public nvmJitFunctionI
		{
			typedef _CFG CFG;
			typedef typename CFG::IRType IRType;
			typedef typename CFG::BBType BBType;
			typedef typename IRType::RegType RegType;
			typedef typename IRType::JumpType JumpType;
			typedef typename IRType::SwitchType SwitchType;
			private:
			bool _changed;

			public:
				JumpToJumpElimination(_CFG &cfg): OptimizationStep<_CFG>(cfg), nvmJitFunctionI("JumpToJump elimination") {};
				void start(bool &);
				bool targetIsJump(const uint32_t, uint32_t &newtarget);
				void simplifyTarget(IRType *, uint32_t);
				bool run() {
					bool dummy = true;
					start(dummy);
					return dummy;
				}
#ifdef ENABLE_COMPILER_PROFILING
			public:
				static uint32_t numModifiedNodes;
				static void incModifiedNodes();
				static uint32_t getModifiedNodes();
#endif

		};
#ifdef ENABLE_COMPILER_PROFILING
	template <class _CFG>
		uint32_t JumpToJumpElimination<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void JumpToJumpElimination<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t JumpToJumpElimination<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif


		//! this function removes all BBs that have only one predecessor and one successor and contain no instructions
		template<class _CFG>
			class EmptyBBElimination: public nvmJitFunctionI
			{
				private:
					_CFG &_cfg;
				public:
					EmptyBBElimination(_CFG &cfg): nvmJitFunctionI("Empty BB elimination"),  _cfg(cfg) {}
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
		uint32_t EmptyBBElimination<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void EmptyBBElimination<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t EmptyBBElimination<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif


		template <class _CFG>
			class RedundantJumpSimplification: public nvmJitFunctionI
		{
			typedef typename _CFG::IRType IRType;

			private:
				_CFG &_cfg;
				bool changed;
				bool isRedundantSwitch(IRType *node, uint16_t &target);
				bool isRedundantJump(IRType *node, uint16_t &target);
				void simplifySwitch(IRType *node);
			public:
				RedundantJumpSimplification(_CFG &cfg): nvmJitFunctionI("Redundant Jump Simplification"), _cfg(cfg) {}
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
		uint32_t RedundantJumpSimplification<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void RedundantJumpSimplification<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t RedundantJumpSimplification<_CFG>::getModifiedNodes() { return numModifiedNodes; }
#endif


		template<class _CFG>
		class BasicBlockElimination: public OptimizationStep<_CFG>, public nvmJitFunctionI
		{
			public:
				BasicBlockElimination(_CFG &cfg): OptimizationStep<_CFG>(cfg), nvmJitFunctionI("Basic Block Elimination") {};

			void start(bool &);
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
		uint32_t BasicBlockElimination<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void BasicBlockElimination<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t BasicBlockElimination<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif


		template<class _CFG>
			bool BasicBlockElimination<_CFG>::run()
			{
				bool dummy = true;
				while(dummy)
				{
					dummy = false;
					start(dummy);
				}
				return true;
			}








//==================================================================

#if 0
		template<class _CFG>
			void BasicBlockElimination<_CFG>::start(bool &changed)
			{
				typedef typename _CFG::BBType BBType;
				typedef std::list<BBType*> BBListType;
				typedef typename std::list<BBType*>::iterator BBListItType;
				typedef typename BBType::BBIterator bbIt;
				typedef typename DiGraph<BBType*>::SortedIterator s_it_t;
				typedef typename DiGraph<BBType*>::GraphNode GraphNode;

#ifdef _DEBUG_OPTIMIZER
				std::cout << "******* Basic Block Elimination" << std::endl;
#endif

				BBListType *bblist = this->_cfg.getBBList();

				// we iterate over all BBs and we remove those BB that cannot be reache
				for(BBListItType i = bblist->begin(); i != bblist->end(); i++)
				{
					if ((*i) == this->_cfg.getEntryBB() || (*i) == this->_cfg.getExitBB())
						continue;

					if((*i)->getNode()->GetPredecessors().size() == 0)
					{
						BBType *bb = *i;
#ifdef _DEBUG_OPTIMIZER
						std::cout << "Rimuovo il BB: " << *bb << std::endl;
#endif
						this->_cfg.removeBasicBlock(bb->getId());
						changed = true;
						//delete bb;
#ifdef ENABLE_COMPILER_PROFILING
						BasicBlockElimination<_CFG>::incModifiedNodes();
#endif
					}
					//delete reached;
				}
				delete bblist;
			}
#endif

//<<Versione Nuova
//  Versione Vecchia>>


		template<class _CFG>
			void BasicBlockElimination<_CFG>::start(bool &changed)
			{
				typedef typename _CFG::BBType BBType;
				typedef std::list<BBType*> BBListType;
				typedef typename std::list<BBType*>::iterator BBListItType;
				typedef typename BBType::BBIterator bbIt;
				typedef typename DiGraph<BBType*>::SortedIterator s_it_t;
				typedef typename DiGraph<BBType*>::GraphNode GraphNode;

#ifdef _DEBUG_OPTIMIZER
				std::cout << "******* Basic Block Elimination" << std::endl;
#endif
				this->_cfg.SortPostorder(*this->_cfg.getEntryNode());

				BBListType *bblist = this->_cfg.getBBList();
				// we set the reached property to false for all BB
				for(BBListItType i = bblist->begin(); i != bblist->end(); i++)
				{
					//(*i)->setProperty("reached", new bool(false));
					(*i)->setProperty("reached", false);
				}
				// using the iterator we can know if a BB is reached
				for(s_it_t i = this->_cfg.FirstNodeSorted(); i != this->_cfg.LastNodeSorted(); i++)
				{
					(*i)->NodeInfo->template setProperty<bool>("reached", true);
					/*
					bool *res = (*i)->NodeInfo->template getProperty<bool *>("reached");
					if(res)
					{
						*res = true;
					}
					*/
				}

				// we iterate over all BBs and we remove those that cannot be reached
				for(BBListItType i = bblist->begin(); i != bblist->end(); i++)
				{
					bool reached = (*i)->template getProperty<bool>("reached");
					//assert(reached != NULL);
					if(reached == false)
					{
						BBType *bb = *i;
						//FIXME OM: this test should not be here!!!
						if(bb->getNode()->GetPredecessors().size() != 0)
							continue;
#ifdef _DEBUG_OPTIMIZER
						std::cout << "Rimuovo il BB: " << *bb;
						std::cout << " num predecessori: " << bb->getNode()->GetPredecessors().size() << std::endl;
#endif

						this->_cfg.removeBasicBlock(bb->getId());
						changed = true;
						//delete bb;
#ifdef ENABLE_COMPILER_PROFILING
						BasicBlockElimination<_CFG>::incModifiedNodes();
#endif
					}
					//delete reached;
				}
				delete bblist;
			}



//==================================================================

		template<class _CFG>
		bool EmptyBBElimination<_CFG>::run()
		{
			typedef typename _CFG::IRType IRType;
			typedef typename IRType::SwitchType SwitchType;
			typedef typename IRType::JumpType JumpType;
			typedef typename _CFG::BBType BBType;
			typedef std::list<BBType*> BBListType;
			typedef typename std::list<BBType*>::iterator BBListItType;
			typedef typename DiGraph<BBType*>::GraphNode GraphNode;
			typedef std::list<GraphNode*> p_list_t;
			typedef typename p_list_t::iterator l_it;

			#ifdef _DEBUG_OPTIMIZER
			std::cout << "******* Empty Basic Block Elimination" << std::endl;
			#endif
			bool changed = false;
			BBListType *bblist = _cfg.getBBList();
			for(BBListItType i = bblist->begin(); i != bblist->end(); i++)
			{
				if((*i)->getId() == 0)
					continue;
				if((*i)->getCodeSize() == 0) // FIXME
				{
					// BB instruction list is empty
					if( (*i)->getNode()->GetSuccessors().size() == 1 && (*i)->getId() != 0)
					{
						// we can remove this BB
						//first, update lists
						GraphNode *succ;
						BBType *bb = *i;
						succ = *(*i)->getNode()->GetSuccessors().begin();
						//now remove the node
						p_list_t preds( (*i)->getNode()->GetPredecessors() );
						for(l_it j = preds.begin(); j != preds.end(); j++)
						{
#ifdef _DEBUG_OPTIMIZER
  						std::cout << "Predecessore :" << *(*j)->NodeInfo << std::endl;
#endif
							_cfg.AddEdge(*(*j), *succ);
							_cfg.DeleteEdge(*(*j), *bb->getNode());
							IRType *lastNode = (*j)->NodeInfo->getLastStatement();
#ifdef _DEBUG_OPTIMIZER
  						std::cout << "Riscrivo il nodo: " << *lastNode << std::endl;
#endif
							if(lastNode->isJump())
							{
								if(lastNode->isSwitch())
								{
									SwitchType *switchNode = dynamic_cast<SwitchType*>(lastNode);
									switchNode->rewrite_destination(bb->getId(), succ->NodeInfo->getId());
								}
								else
								{
									JumpType *predJ = dynamic_cast<JumpType*>(lastNode);
									predJ->rewrite_destination(bb->getId(), succ->NodeInfo->getId());
								}
							}
#ifdef _DEBUG_OPTIMIZER
  						std::cout << "Risultato: " << *lastNode << std::endl;
#endif
						}

#ifdef _DEBUG_OPTIMIZER
  						std::cout << "Elimino BB: " << (bb)->getId() << std::endl;
#endif

						_cfg.removeBasicBlock((bb)->getId());
						//delete bb;
						changed = true;
#ifdef ENABLE_COMPILER_PROFILING
						EmptyBBElimination<_CFG>::incModifiedNodes();
#endif
						continue;
					}
				}
			}
			delete bblist;
			return changed;
		};

		template <class _CFG>
		void ControlFlowSimplification<_CFG>::bothKidConstant(typename _CFG::IRType::JumpType *node)
		{

			ConstType *leftkid, *rightkid;
			leftkid = (ConstType*)node->getTree()->getKid(0);
			rightkid = (ConstType*)node->getTree()->getKid(1);
			uint32_t nonReachedBB, BBId;
			BBId = node->belongsTo();
			JumpType *newnode;

			switch(node->getOpcode())
			{
				case JCMPEQ:
					if(leftkid->getValue() == rightkid->getValue()) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPNEQ:
					if(leftkid->getValue() != rightkid->getValue()) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPG:
					if(leftkid->getValue() > rightkid->getValue()) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPGE:
					if(leftkid->getValue() >= rightkid->getValue()) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPL:
					if(leftkid->getValue() < rightkid->getValue()) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPLE:
					if(leftkid->getValue() <= rightkid->getValue()) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPG_S:
					if( ((int32_t)leftkid->getValue()) > ((int32_t)rightkid->getValue()) ) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPGE_S:
					if( ((int32_t)leftkid->getValue()) >= ((int32_t)rightkid->getValue()) ) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPL_S:
					if( ((int32_t)leftkid->getValue()) < ((int32_t)rightkid->getValue()) ) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JCMPLE_S:
					if( ((int32_t)leftkid->getValue()) <= ((int32_t)rightkid->getValue()) ) //condition TRUE
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				default:
					return;
			} // end switch
			updateSuccPredLists(node, newnode, nonReachedBB);
		}


		/*!
		 * \brief This method analyze a node with one kids that is constant
		 *
		 * JEQ, JNE
		 * are analyzed in this function and a new unconditonal JUMP node is created
		 * pointing to the evaluated target
		 */
		template <class _CFG>
		void ControlFlowSimplification<_CFG>::oneKidConstant(typename _CFG::IRType::JumpType *node)
		{
			ConstType *leftkid = (ConstType*)node->getKid(0);
			uint32_t BBId = node->belongsTo();
			uint32_t nonReachedBB;
			JumpType *newnode;

			switch(node->getOpcode())
			{
				case JEQ:
					if(leftkid->getValue() == 0)
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					break;
				case JNE:
					if(leftkid->getValue() == 0)
					{
						newnode = new JumpType(JUMPW, BBId, node->getFalseTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getTrueTarget();
					}
					else
					{
						newnode = new JumpType(JUMPW, BBId, node->getTrueTarget(), 0, node->getDefReg(), NULL, NULL);
						nonReachedBB = node->getFalseTarget();
					}
					break;
				default:
					return;
			} // end switch
			updateSuccPredLists(node, newnode, nonReachedBB);
			return;
		}

		template <class _CFG>
		typename _CFG::IRType::JumpType* ControlFlowSimplification<_CFG>::handle_constant_switch(typename _CFG::IRType::SwitchType *node)
		{
			assert(node != NULL);
			typedef typename SwitchType::targets_iterator T_it;

			BBType *bb = this->_cfg.getBBById(node->belongsTo()), *nr_bb;
			uint32_t kidValue = ((ConstType*)node->getKid(0))->getValue();
			JumpType *newnode = NULL;

			for(T_it i = node->TargetsBegin(); i != node->TargetsEnd(); i++)
			{
				if((*i).first != kidValue) // not reached target
				{
					nr_bb = this->_cfg.getBBById((*i).second);
					this->_cfg.DeleteEdge(*bb->getNode(), *nr_bb->getNode());
				}
				else //this is our destination
				{
					newnode = new JumpType(JUMPW, node->belongsTo(), (*i).second, 0, node->getDefReg(), NULL, NULL);
#ifdef _DEBUG_OPTIMIZER
					std::cout << "Il nodo switch appartenente al BB: " << node->belongsTo() << "E' diventato un jump" << std::endl;
#endif
				}
			}

			if(newnode == NULL) // default case
			{
					newnode = new JumpType(JUMPW, node->belongsTo(), node->getDefaultTarget(), 0, node->getDefReg(), NULL, NULL);
#ifdef _DEBUG_OPTIMIZER
					std::cout << "Il nodo switch appartenente al BB: " << node->belongsTo() << "E' diventato un jump" << std::endl;
#endif
			}
			else
			{
				nr_bb = this->_cfg.getBBById(node->getDefaultTarget());
				this->_cfg.DeleteEdge(*bb->getNode(), *nr_bb->getNode());
			}

			return newnode;
		}

		template <class _CFG>
		typename _CFG::IRType::JumpType* ControlFlowSimplification<_CFG>::handle_two_succ_switch(typename _CFG::IRType::SwitchType *node)
		{
			assert(node != NULL);
			typedef typename SwitchType::targets_iterator T_it;

			JumpType *newnode = NULL;
			IRType* left = node->getKid(0);

			int n = 0;
			for(T_it i = node->TargetsBegin(); i != node->TargetsEnd(); i++)
			{
				n++;
				ConstType* con = IRType::make_const(node->belongsTo(), (*i).first, node->getDefReg());
				node->unlinkKid(0);
				newnode = new JumpType(JCMPEQ, node->belongsTo(),
						(*i).second, node->getDefaultTarget(), node->getDefReg(),
						left, con);
			}

			assert(n == 1);

			return newnode;
		}

		template <class _CFG>
		void ControlFlowSimplification<_CFG>::handleSwitch(typename _CFG::IRType::SwitchType *node)
		{

			assert(node != NULL);

			JumpType *newnode = NULL;

			if(node->getKid(0)->isConst())
				newnode = handle_constant_switch(node);
			else if (node->get_targets_num() == 1)
				newnode = handle_two_succ_switch(node);

			if (newnode)
			{
				//replace old node with the new node
				std::list<IRType*> &BBCodeList = this->_cfg.getBBById(node->belongsTo())->getCode();
				std::replace(BBCodeList.begin(), BBCodeList.end(), (IRType*)node, (IRType*)newnode);

				//delete node;
#ifdef ENABLE_COMPILER_PROFILING
				ControlFlowSimplification<_CFG>::incModifiedNodes();
#endif
			}

			return;
		}

		/*!
		 * \brief this fuction replaces the oldnode with the new one in the BB CodeList, then removes non reached BB from this node successors list\
			and deletes current node from non reached one's predecessors list
		 * \param old node
		 * \param new node
		 * \param id of the non reached BB
		 */
		template <class _CFG>
		void ControlFlowSimplification<_CFG>::updateSuccPredLists(typename _CFG::IRType *node, typename _CFG::IRType *newnode, uint32_t nonReachedBB)
		{

			uint32_t BBId = node->belongsTo();
			std::list<IRType*> &BBCodeList = this->_cfg.getBBById(BBId)->getCode();
			//replace old node with the new node
			std::replace(BBCodeList.begin(), BBCodeList.end(), node, newnode);

			if( ((JumpType*)node)->getTrueTarget() != ((JumpType*)node)->getFalseTarget())
			{
				//get pointer to he BBs
				BBType *thisBB_p = this->_cfg.getBBById(BBId);
				BBType *nrBB_p = this->_cfg.getBBById(nonReachedBB);

				//remove current BB from non reached BB predecessors
				this->_cfg.DeleteEdge(*thisBB_p->getNode(), *nrBB_p->getNode());
				//delete node;
			}
			delete node;
			*_changed = true;
#ifdef ENABLE_COMPILER_PROFILING
			ControlFlowSimplification<_CFG>::incModifiedNodes();
#endif
		}

		/*!
		 * \brief this function is called for each statement in the CFG. It inspects the node and it calls the right method choosing from \
		 * bothKidConstant oneKidConstant.
		 */
		template <class _CFG>
		void ControlFlowSimplification<_CFG>::operator()(typename _CFG::IRType *node)
		{
			if(!node->isJump() && !node->isSwitch())
				return;


			if(node->isSwitch())
			{
				handleSwitch(dynamic_cast<SwitchType*>(node));
			}
			else
			{
				if(node->isJump())
				{
					JumpType *jump_node = dynamic_cast<JumpType*>(node);
					assert(jump_node && "cast failed");
					if(jump_node->getFalseTarget() != 0)
					{
						IRType *leftkid, *rightkid;


						leftkid = node->getTree()->getKid(0);
						rightkid = node->getTree()->getKid(1);

						if(leftkid && rightkid)
						{
							if(leftkid->isConst() && rightkid->isConst())
								bothKidConstant((JumpType*)node);
						}
						else
							if(leftkid && leftkid->isConst())
								oneKidConstant((JumpType*)node);

					}
				}
			}
			return;
		}

		template <class _CFG>
		void ControlFlowSimplification<_CFG>::start(bool &changed)
		{
			_changed = &changed;
			visitCfgNodes(this->_cfg, this);
			return;
		}



		template <class _CFG>
		bool JumpToJumpElimination<_CFG>::targetIsJump(const uint32_t targetId, uint32_t &newTarget)
		{
			typedef typename BBType::IRStmtNodeIterator node_it;
			BBType *BB = this->_cfg.getBBById(targetId);

			if(BB->getCodeSize() == 1)
			{
				IRType *node = BB->getLastStatement();
				if( node->getOpcode() == JUMP || node->getOpcode() == JUMPW)
				{
					newTarget = dynamic_cast<JumpType*>(node)->getTrueTarget();
					return true;
				}
			}
			return false;
		}


		template <class _CFG>
		void JumpToJumpElimination<_CFG>::simplifyTarget(typename _CFG::IRType *node, uint32_t target)
		{
			#ifdef _DEBUG_OPTIMIZER
			std::cout << "************BB Id: " << node->belongsTo() << std::endl;
			std::cout << "target: " << target;
			#endif
			uint32_t new_target = 0;
			if(targetIsJump(target, new_target))
			{
#ifdef _DEBUG_OPTIMIZER
				std::cout << "Il nodo: " << *node << std::endl;
				std::cout << "Non salta + a " << target << " ma a: " << new_target << std::endl << std::endl;
#endif
				node->rewrite_destination(target,
						new_target);
				BBType *currBB, *succBB, *newSucc;
				currBB = this->_cfg.getBBById(node->belongsTo());
				succBB = this->_cfg.getBBById(target);
				newSucc = this->_cfg.getBBById(new_target);
				this->_cfg.DeleteEdge(*currBB->getNode(), *succBB->getNode());
				this->_cfg.AddEdge(*currBB->getNode(), *newSucc->getNode());

				//simplifyTarget(node, new_target);
				_changed = true;
#ifdef ENABLE_COMPILER_PROFILING
				JumpToJumpElimination<_CFG>::incModifiedNodes();
#endif
			}
		}

		template <class _CFG>
		void JumpToJumpElimination<_CFG>::start(bool &changed)
		{

				typedef std::list<BBType*> list_t;
				typedef typename std::list<BBType*>::iterator list_it_t;
				typedef typename BasicBlock<IRType>::BBIterator bb_it_t;

				_changed = false;
				list_t *bblist = this->_cfg.getBBList();
				for(list_it_t bb = bblist->begin(); bb != bblist->end(); bb++)
				{
					uint32_t bbId = (*bb)->getId();
					if(bbId == 0)
						continue;

					#ifdef _DEBUG_OPTIMIZER
					std::cout << "***** Iterating over BB Id: " << bbId << std::endl;
					#endif
					IRType *lastNode = (*bb)->getLastStatement();
					if(lastNode != NULL)
					{
						if(lastNode->isJump() && !lastNode->isSwitch())
						{
							if( (*bb)->getNode()->GetSuccessors().size() == 1)
							{
								#ifdef _DEBUG_OPTIMIZER
								std::cout << " Last Instruction is a Jump"  << std::endl;
								#endif
								JumpType *jNode = dynamic_cast<JumpType*>(lastNode);
								uint32_t target = jNode->getTrueTarget();
								simplifyTarget(jNode, target);
							}
							if( (*bb)->getNode()->GetSuccessors().size() == 2)
							{
								#ifdef _DEBUG_OPTIMIZER
								std::cout << " Last Instruction is a Branch" << std::endl;
								#endif
								JumpType *jNode = dynamic_cast<JumpType*>(lastNode);
								uint32_t target = jNode->getTrueTarget();
								simplifyTarget(jNode, target);

								target = jNode->getFalseTarget();
								simplifyTarget(jNode, target);
							} // end if two successors
						}// end if jump
						if (lastNode->isSwitch())
						{
							SwitchType *swNode = dynamic_cast<SwitchType*>(lastNode);
							#ifdef _DEBUG_OPTIMIZER
							std::cout << " Last Instruction is a Switch" << std::endl;
							swNode->printNode(std::cout);
							#endif
							for(typename SwitchType::targets_iterator cs = swNode->TargetsBegin(); cs != swNode->TargetsEnd(); cs++)
								simplifyTarget(swNode, (*cs).second);
							simplifyTarget(swNode, swNode->getDefaultTarget());
						} // en if switch
					} // end if node != null
				} // end for
				changed = _changed;

				delete bblist;

			return;
		}


		template <class _CFG>
			bool RedundantJumpSimplification<_CFG>::run()
			{
				typedef typename _CFG::BBType BBType;
				typedef std::list<BBType*> list_t;
				typedef typename std::list<BBType*>::iterator list_it_t;
				typedef typename BasicBlock<IRType>::BBIterator bb_it_t;
				typedef typename _CFG::IRType::JumpType JumpType;

				uint16_t target = 0;

				bool changed = false;
				list_t *bblist = this->_cfg.getBBList();
				for(list_it_t i = bblist->begin(); i != bblist->end(); i++)
				{
					if((*i)->getId() == 0)
						continue;

					IRType *lastNode = (*i)->getLastStatement();

					if(lastNode != NULL)
					{
						if(lastNode->isJump())
						{
							JumpType *newnode = NULL;
							if(lastNode->isSwitch())
							{
								if(isRedundantSwitch(lastNode, target))
								{
#ifdef _DEBUG_OPTIMIZER
									std::cout << "Il nodo: " << std::endl;
									lastNode->printNode(std::cout, true);
									std::cout << "E' una switch costante" << std::endl;

#endif
									newnode = new JumpType(JUMPW, lastNode->belongsTo(), target, 0, lastNode->getDefReg(), NULL, NULL);
								}
								else
								{
									simplifySwitch(lastNode);
								}
							}
							else
							{
								//jump case
								if(isRedundantJump(lastNode, target))
								{
									newnode = new JumpType(JUMPW, lastNode->belongsTo(), target, 0, lastNode->getDefReg(), NULL, NULL);
								}
							}
							if(newnode != NULL)
							{
								std::list<IRType*> &BBCodeList = this->_cfg.getBBById(lastNode->belongsTo())->getCode();
								std::replace(BBCodeList.begin(), BBCodeList.end(), (IRType*)lastNode, (IRType*)newnode);
								changed = true;
								delete lastNode;
#ifdef ENABLE_COMPILER_PROFILING
								RedundantJumpSimplification<_CFG>::incModifiedNodes();
#endif
							}
						}
					}
				}
				return changed;
			}

		template<class _CFG>
			bool RedundantJumpSimplification<_CFG>::isRedundantSwitch(IRType *node, uint16_t &target)
			{
				typedef std::vector<std::pair<uint32_t, uint32_t> > t_l;
				typedef typename t_l::iterator t_l_it;
				typedef typename _CFG::IRType::SwitchType SwitchType;
				typedef typename SwitchType::targets_iterator t_it;

				uint32_t act_target;

				SwitchType *switch_node = dynamic_cast<SwitchType*>(node);
				assert(node != NULL && "Dynamic cast failed");
				act_target = switch_node->getDefaultTarget();
				for(t_it i = switch_node->TargetsBegin(); i != switch_node->TargetsEnd(); i++)
				{
					if(i->second != act_target)
						return false;
				}
				target = act_target;
				return true;
			}


		template<class _CFG>
			bool RedundantJumpSimplification<_CFG>::isRedundantJump(IRType *node, uint16_t &target)
			{
				typedef typename _CFG::IRType::JumpType JumpType;

				JumpType *jump_node = dynamic_cast<JumpType*>(node);
				assert(jump_node != NULL && "Dynamic cast failed");

				if(jump_node->getTrueTarget() == jump_node->getFalseTarget())
				{
					target = jump_node->getTrueTarget();
					return true;
				}
				else
					return false;
			}

		template <class _CFG>
			void RedundantJumpSimplification<_CFG>::simplifySwitch(typename _CFG::IRType *node)
			{
				typedef std::vector<std::pair<uint32_t, uint32_t> > t_l;
				typedef typename t_l::iterator t_l_it;
				typedef typename _CFG::IRType::SwitchType SwitchType;
				typedef typename SwitchType::targets_iterator t_it;

				uint32_t def_target;

				SwitchType *switch_node = dynamic_cast<SwitchType*>(node);
				assert(node != NULL && "Dynamic cast failed");
				def_target = switch_node->getDefaultTarget();
				for(t_it i = switch_node->TargetsBegin(); i != switch_node->TargetsEnd(); i++)
				{
					if(i->second == def_target)
						switch_node->removeCase(i->first);
				}
			}
	}	/* OPT */

}	/* JIT */

#endif
