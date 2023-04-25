/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file constant_folding.h
 * \brief In this file are defined ConstantFolding and AlgebraicSimplification optimization steps
 * */
#ifndef CONSTANT_FOLDING_H
#define CONSTANT_FOLDING_H

#include <list>

#include "cfg.h"
#include "irnode.h"
#include "optimization_step.h"
#include "function_interface.h"

//FIXME
#ifdef _JIT_BUILD
#include "jit_internals.h"
#endif
#ifdef _PFL_BUILD
#include "tree.h"
#endif

namespace jit {
namespace opt {


	template <class _CFG>
	class ConstantFolding : public OptimizationStep<_CFG>, nvmJitFunctionI
	{
		typedef typename _CFG::IRType IR;
		private:
			std::list<IR *> patches;

		public:
			ConstantFolding(_CFG & cfg);
			void start(bool &changed);
			bool run();
#ifdef ENABLE_COMPILER_PROFILING
			static uint32_t numModifiedNodes;
			static void incModifiedNodes();
			static uint32_t getModifiedNodes();
#endif
	};

#ifdef ENABLE_COMPILER_PROFILING
	template <class _CFG>
		uint32_t ConstantFolding<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void ConstantFolding<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t ConstantFolding<_CFG>::getModifiedNodes() { return numModifiedNodes;  numModifiedNodes=0; }
#endif

	template <class _CFG>
	class AlgebraicSimplification : public OptimizationStep<_CFG>	{
		typedef typename _CFG::IRType IR;
		private:
			std::list<IR *> patches;

		public:
			AlgebraicSimplification(_CFG & cfg);
			void start(bool &changed);
			void start();
#ifdef ENABLE_COMPILER_PROFILING
			static uint32_t numModifiedNodes;
			static void incModifiedNodes();
			static uint32_t getModifiedNodes();
#endif


	};
#ifdef ENABLE_COMPILER_PROFILING
	template <class _CFG>
		uint32_t AlgebraicSimplification<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void AlgebraicSimplification<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t AlgebraicSimplification<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif

	template <class _CFG>
	class CanonicalizationStep : public OptimizationStep<_CFG>, public nvmJitFunctionI
	{
		public:
			CanonicalizationStep(_CFG & cfg);
			void start(bool & changed);
			bool run();
	};

	template <class _CFG>
		ConstantFolding<_CFG>::ConstantFolding(_CFG & cfg) : OptimizationStep<_CFG>(cfg) {};


	template <class _CFG>
		void handleArithmeticSub(typename _CFG::IRType *node)
		{
			typedef typename _CFG::IRType IR;
			typedef typename IR::ConstType ConstNode;

			node->swapKids();
			ConstNode* con = dynamic_cast<ConstNode *>(node->getKid(1));
			con->setValue(-con->getValue());

			switch(node->getOpcode())
			{
				case SUB:
					node->setOpcode(ADD);
					break;
				case SUBSOV:
					node->setOpcode(ADDSOV);
					break;
				case SUBUOV:
					node->setOpcode(ADDUOV);
					break;
			}
			return;
		}

	/*!\brief Step to move every constant kid to the right
	 *
	 * If a node has a const kid on the left and a non-const kid on the right
	 * its kids has to be swapped
	 */
	template <class _CFG>
		void canonicalizeNode(typename _CFG::IRType *node, bool & _changed)
		{
			typedef typename _CFG::IRType IR;
			IR *kid0, *kid1;
			kid0 = node->getKid(0);
			kid1 = node->getKid(1);
			if(kid0 && kid1)
			{
						switch(node->getOpcode())
						{
							case ADD:
							case ADDSOV:
							case ADDUOV:
							case IMUL:
							case IMULSOV:
							case IMULUOV:
							case OR:
							case AND:
							case XOR:
								if(kid0->isConst() && !kid1->isConst())
								{
									node->swapKids();
									_changed = true;
								}
								break;
							case SUB:
							case SUBSOV:
							case SUBUOV:
								if(kid0->isConst() && !kid1->isConst())
									handleArithmeticSub<_CFG>(node);
								;
								break;
							/*
							case JCMPEQ:
							case JCMPNEQ:
								if(kid0->isConst() && !kid1->isConst())
								{
									node->swapKids();
									_changed = true;
								}
							*/
							default:
								break;
						}
			}
		}

	//!this class does constant folding
	template <class _CFG>
		struct ConstantFoldingFunctor
		{
			typedef typename _CFG::IRType IR;
			typedef typename _CFG::BBType BBType;
			typedef typename IR::ConstType ConstNode;
			bool & _changed;
			std::list<IR*> &_patches;

			ConstantFoldingFunctor(bool &changed, std::list<IR*> &patches) :
				_changed(changed), _patches(patches) {};

			/*!\brief folding for sub operations
			 *
			 * in order to move constant kid to right is necessary for SUB operation
			 * a little bit of work: the node becomes an add and the constant operand get
			 * a new father that negates its value
			 */


			/*!\brief Step of optimization that implements constant folding
			 *
			 * If an arithmetic node has two constant kids the operation can be done at
			 * compilation time and operation node substituted by the new computed constant
			 */
			IR* constantFolding(IR * node)
			{
				IR *leftkid, *rightkid, *newnode;

				leftkid = node->getKid(0);
				rightkid = node->getKid(1);

				//canonicalization
				canonicalizeNode<_CFG>(node, _changed);

				//post order visit
				if(leftkid)
				{
					newnode = constantFolding(leftkid);
					if(newnode != leftkid)
					{
#ifdef _DEBUG_OPTIMIZER
						std::cout << "Vecchio nodo:" << std::endl;
						std::cout << *leftkid << std::endl;
						std::cout << "Nuovo nodo:" << std::endl;
						std::cout << *newnode << std::endl;
#endif

						// Don't lose definitions while folding constants
						if(leftkid->getDefs().size() == 1) {
							IR *store = IR::make_store(leftkid->belongsTo(), newnode->copy(), leftkid->getDefReg());
							_patches.push_back(store);
							//							std::cout << "Nodo creato: ";
							//							store->printNode(std::cout, true);
							//							std::cout << '\n';
						}

						//delete leftkid;

						node->setKid(newnode, 0);
						leftkid = node->getKid(0);
						_changed = true;
#ifdef ENABLE_COMPILER_PROFILING
						ConstantFolding<_CFG>::incModifiedNodes();
#endif
					}
				}
				if(rightkid)
				{
					newnode = constantFolding(rightkid);
					if(newnode != rightkid)
					{
#ifdef _DEBUG_OPTIMIZER
						std::cout << "Vecchio nodo:" << std::endl;
						std::cout << *rightkid << std::endl;
						std::cout << "Nuovo nodo:" << std::endl;
						std::cout << *newnode << std::endl;
#endif

						// Don't lose definitions while folding constants
						if(rightkid->getDefs().size() == 1) {
							IR *store = IR::make_store(rightkid->belongsTo(), newnode->copy(), rightkid->getDefReg());
							_patches.push_back(store);
							//							std::cout << "Nodo creato: ";
							//							store->printNode(std::cout, true);
							//							std::cout << '\n';
						}

						//delete rightkid;

						node->setKid(newnode, 1);
						rightkid = newnode;
						_changed = true;
#ifdef ENABLE_COMPILER_PROFILING
						ConstantFolding<_CFG>::incModifiedNodes();
#endif
					}
				}

				if(leftkid && rightkid)
				{
					if( (leftkid->isConst()) && (rightkid->isConst()) )
					{
						ConstNode *leftkidconst, *rightkidconst;
						leftkidconst = (ConstNode*) leftkid;
						rightkidconst = (ConstNode*) rightkid;
							switch (node->getOpcode())
							{
								case ADD:
								case ADDSOV:
								case ADDUOV:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue() + rightkidconst->getValue(), node->getDefReg());
									break;
								case SUB:
								case SUBSOV:
								case SUBUOV:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue() - rightkidconst->getValue(), node->getDefReg());
									break;
								case IMUL:
								case IMULSOV:
								case IMULUOV:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue() * rightkidconst->getValue(), node->getDefReg());
									break;
								case MOD:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue() % rightkidconst->getValue(), node->getDefReg());
								case AND:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue() & rightkidconst->getValue(), node->getDefReg());
									break;
								case OR:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue() | rightkidconst->getValue(), node->getDefReg());
									break;
							} // end switch
					} // and both kids constants
				} // end node with two kis
				if(leftkid && !rightkid)
				{
					if(leftkid->isConst())
					{
						ConstNode *leftkidconst;
						leftkidconst = (ConstNode*) leftkid;
							switch(node->getOpcode())
							{
								case NEG:
									return IR::make_const(node->belongsTo(), - leftkidconst->getValue(), node->getDefReg());
									break;
								case IINC_1:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()+1, node->getDefReg());
									break;
								case IINC_2:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()+2, node->getDefReg());
									break;
								case IINC_3:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()+3, node->getDefReg());
									break;
								case IINC_4:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()+4, node->getDefReg());
									break;
								case IDEC_1:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()-1, node->getDefReg());
									break;
								case IDEC_2:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()-2, node->getDefReg());
									break;
								case IDEC_3:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()-3, node->getDefReg());
									break;
								case IDEC_4:
									return IR::make_const(node->belongsTo(), leftkidconst->getValue()-4, node->getDefReg());
									break;
							}
					}
				}
				return node;
			}// end function

			// node is a statement
			void operator()(IR *node, typename BBType::IRStmtNodeIterator i, std::list<IR *> &code)
			{
				_patches.clear();
				constantFolding(node);
				code.insert(++i, _patches.begin(), _patches.end());
				_patches.clear();
			}
		};


	template <class _CFG>
		void ConstantFolding<_CFG>::start(bool & changed)
		{
			struct ConstantFoldingFunctor<_CFG> cff(changed, patches);
			visitCfgNodesPosition(this->_cfg, &cff);
		}

	template <class _CFG>
		bool ConstantFolding<_CFG>::run()
		{
			bool res = false;
			start(res);
			return res;
		}


	template <class _CFG>
		AlgebraicSimplification<_CFG>::AlgebraicSimplification(_CFG & cfg): OptimizationStep<_CFG>(cfg) {};

	/*!
	 * \brief optimization step that tries to simplify some arithmetic operation
	 *
	 * I'll explain the behaviour with an example: if a node is an ADD of a load and 0 the result will be
	 * load
	 */
	template <class _CFG>
		struct AlgebraicSimplificationFunctor
		{
			typedef typename _CFG::IRType IR;
			typedef typename _CFG::BBType BBType;
			typedef typename IR::ConstType ConstNode;
			bool &_changed;
			//bool _make_patches;
			std::list<IR *> &_patches;
			AlgebraicSimplificationFunctor(bool & changed, std::list<IR *> &patches) :
				_changed(changed), _patches(patches){};

			IR * algebraicSimplify(IR *node, IR *leftkid, IR *rightkid)
			{
				if(leftkid && rightkid)
				{
					switch ( node->getOpcode())
					{
						case ADD:
						case ADDSOV:
						case ADDUOV:
							if( rightkid->isConst() && (((ConstNode*)rightkid)->getValue() == 0))
							{
								node->setKid(NULL, 0);
								return leftkid;
							}
							break;
						case SUB:
						case SUBSOV:
						case SUBUOV:
							if( rightkid->isConst() && (((ConstNode*)rightkid)->getValue() == 0))
							{
								node->setKid(NULL, 0);
								return leftkid;
							}
							break;
						case IMUL:
						case IMULSOV:
						case IMULUOV:
							if( rightkid->isConst())
							{
								if( ((ConstNode*)rightkid)->getValue() == 1)
								{
									node->setKid(NULL, 0);
									return leftkid;
								}
								if( ((ConstNode*)rightkid)->getValue() == 0)
								{
									node->setKid(NULL, 0);
									node->setKid(NULL, 1);

									return IR::make_const(node->belongsTo(), 0, node->getDefReg());
								}
							}
							break;
					} // end switch
				} // both kids
				return node;
			}

			//!visits the statement tree in postorder

			IR * algebraicSimplifyVisit(IR *node)
			{
				IR *leftkid, *rightkid, *newnode;
				leftkid = node->getKid(0);
				rightkid = node->getKid(1);

				if(leftkid)
				{
					newnode = algebraicSimplifyVisit(leftkid);
					if(newnode != leftkid)
					{
						// Don't lose definitions while folding constants
						if(leftkid->getDefs().size() == 1) {
							IR * store = IR::make_store(leftkid->belongsTo(), newnode->copy(), leftkid->getDefReg());
							_patches.push_back(store);

#ifdef _DEBUG_OPTIMIZER
							std::cout << "Algebraic ****** Nodo creato: ************" << std::endl;
							store->printNode(std::cout, true);
							std::cout << '\n';
#endif
						}

						//delete leftkid;

						node->setKid(newnode,0);
						leftkid = newnode;
						_changed = true;
#ifdef ENABLE_COMPILER_PROFILING
						AlgebraicSimplification<_CFG>::incModifiedNodes();
#endif
					}
				}
				if(rightkid)
				{
					newnode = algebraicSimplifyVisit(rightkid);
					if(newnode != rightkid)
					{
						// Don't lose definitions while folding constants
						if(rightkid->getDefs().size() == 1) {
							IR *store = IR::make_store(rightkid->belongsTo(), newnode->copy(), rightkid->getDefReg());
							_patches.push_back(store);

#ifdef _DEBUG_OPTIMIZER
							std::cout << "Algebraic ************Nodo creato: ************" << std::endl;
							store->printNode(std::cout, true);
							std::cout << '\n';
#endif
						}

						//delete rightkid;

						node->setKid(newnode,1);
						rightkid = newnode;
						_changed = true;
					}
				}
				return algebraicSimplify(node, leftkid, rightkid);
			}

			void operator()(IR *node, typename BBType::IRStmtNodeIterator i, std::list<IR *> &code)
			{
				_patches.clear();
				algebraicSimplifyVisit(node);
				code.insert(++i, _patches.begin(), _patches.end());
				_patches.clear();
			}
		};


	template <class _CFG>
	void AlgebraicSimplification<_CFG>::start(bool & changed)
	{
		struct AlgebraicSimplificationFunctor<_CFG> asf(changed, patches);
		visitCfgNodesPosition(this->_cfg, &asf);
		return;
	}



	template <class _CFG>
		CanonicalizationStep<_CFG>::CanonicalizationStep(_CFG &cfg) : OptimizationStep<_CFG>(cfg), nvmJitFunctionI("Canonicalizatio step")  {};

	template <class _CFG>
		struct canonicalizationFunctor
		{
			typedef typename _CFG::IRType IR;
			void operator()(IR *node /*, BasicBlock<MIRNode>::IRStmtNodeIterator, std::list<MIRNode *> & */)
			{
				typedef typename IR::IRNodeIterator node_it_t;
				bool dummy;

				for(node_it_t i = node->nodeBegin(); i != node->nodeEnd(); i++)
					canonicalizeNode<_CFG>(*i, dummy);
			}
		};

	template <class _CFG>
		void CanonicalizationStep<_CFG>::start(bool &changed)
		{
			canonicalizationFunctor<_CFG> cf;
			visitCfgNodes(this->_cfg, &cf);
		}

	template <class _CFG>
		bool CanonicalizationStep<_CFG>::run()
		{
			bool dummy = true;
			while(dummy)
			{
				dummy = false;
				start(dummy);
			}
			return true;
		}
} /* JIT */
} /* OPT */

#endif
