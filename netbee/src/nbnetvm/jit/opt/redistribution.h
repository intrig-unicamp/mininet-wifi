/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __REDISTRIBUTION_H__
#define __REDISTRIBUTION_H__

#include "cfg.h"
#include "irnode.h"
//#include "basicblock.h"
#include "optimization_step.h"


//FIXME
#ifdef _JIT_BUILD
#include "jit_internals.h"
#endif
#ifdef _PFL_BUILD
#include "tree.h"
#endif

#include <list>
#include <map>

namespace jit {
namespace opt {


	template <class _CFG>
class Redistribution : public OptimizationStep<_CFG> {
	typedef _CFG CFG;
	typedef typename CFG::IRType IR;
	typedef typename IR::RegType RegType;
	typedef typename CFG::BBType BBType;
	typedef typename IR::ConstType ConstType;

  private:
//	_CFG &cfg;
	std::list<IR *> patches;
	
	struct RedistributionFunctor {
		bool &_changed;
		std::list<IR*> &_patches;
		
		std::map<RegType, uint32_t> count;
		
		RedistributionFunctor(bool &, std::list<IR*> &);
		
		bool is_add_with_constant(IR*, uint32_t &, IR*&);
		
		void operator()(IR*, typename BBType::IRStmtNodeIterator,
			std::list<IR*> &);
			
		bool redistribute(IR*);
			
	};

  public:
	Redistribution(_CFG & cfg);
	void start(bool &changes);
};

template <class _CFG>
Redistribution<_CFG>::Redistribution(_CFG &c) : OptimizationStep<_CFG>(c)
{
	// Nop
	return;
}

template <class _CFG>
void Redistribution<_CFG>::start(bool &changed)
{
	RedistributionFunctor rf(changed, patches);
	visitCfgNodesPosition(this->_cfg, &rf);
	{
//		std::cout << "Dopo Redistribution " << _cfg.getName() << std::endl;
//		jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
//		std::cout << codeprinter;
	}
}


template <class _CFG>
Redistribution<_CFG>::RedistributionFunctor::RedistributionFunctor(bool &c, std::list<typename _CFG::IRType *> &p) :
	_changed(c), _patches(p)
{
	// Nop
	return;
}


// node is a statement
template <class _CFG>
void Redistribution<_CFG>::RedistributionFunctor::operator()(typename _CFG::IRType *node, typename _CFG::BBType::IRStmtNodeIterator i,
	std::list<typename _CFG::IRType *> &code)
{
	_patches.clear();
	
	// Fill the usage count map
	count.clear();
	typename IR::IRNodeIterator k(node->nodeBegin());
	
	for(; k != node->nodeEnd(); k++) {
		std::set<RegType> uses((*k)->getUses());
		typename std::set<RegType>::iterator j;
		
		for(j = uses.begin(); j != uses.end(); ++j) {
			++count[*j];
		}
	}
	
	redistribute(node);
	code.insert(++i, _patches.begin(), _patches.end());
	_patches.clear();
}

template <class _CFG>
bool Redistribution<_CFG>::RedistributionFunctor::redistribute(typename _CFG::IRType *n)
{
	/* FIXME: we should be careful not to perform redistribution on a node that defines
	 * a register that is used later in this same statement! */

	if(!n)
		return false;
		
	if(n->isJump())
		return false;

#ifdef _DEBUG_OPTIMIZER
		std::cout << "[Redistribution]" << std::endl;
#endif
		
	for(uint32_t i(0); i < 2; ++i)
	{
		#ifdef _DEBUG_OPTIMIZER
		std::cout << "[Redistribution]: going to visit kid " << i << std::endl;
		#endif
		if(redistribute(n->getKid(i)))
			return true;
	}
		
	uint32_t my_const = 0, other_const = 0;
	IR *other, *dummy;
	
	if(is_add_with_constant(n, my_const, dummy) && count[n->getDefReg()] == 1) {
#ifdef _DEBUG_OPTIMIZER
		std::cout << "[Redistribution] :Il nodo e' una add con costante" << std::endl;
		n->printNode(std::cout, true);
		std::cout << std::endl;
#endif
		for(uint32_t i(0); i < 2; ++i) {
			if(is_add_with_constant(n->getKid(i), other_const, other)) {
#ifdef _DEBUG_OPTIMIZER
				std::cout << "[Redistribution] :ha un figlio che e' add con costante" << std::endl;
				n->getKid(i)->printNode(std::cout, true);
				std::cout << std::endl;
#endif
				// Play it safe: first of all, save the register defined by the kid to the patches list.
				//MIRNode *store(MIRNode::unOp(STREG, n->belongsTo(), n->getKid(i)->copy(), n->getKid(i)->getDefReg()));
				{
					IR *other_kid(NULL);
					uint32_t other_kid_n(0);
					IR *store = IR::make_store(n->belongsTo(), n->getKid(i)->copy(), n->getKid(i)->getDefReg());
					IR *add = store->getKid(0);
					other_kid_n = add->getKid(0)->isConst() ? 1 : 0;
					other_kid = add->getKid(other_kid_n);

					RegType defReg = other_kid->getDefReg();
					add->setKid(IR::make_load(add->belongsTo(), defReg), other_kid_n);
					delete other_kid;
					

					store->getKid(0)->setDefReg(RegType::get_new(0));
					_patches.push_back(store);
				}

				// Copy the non-constant kid of the add_with_constant son
				dummy = other->copy();
				
				// Then delete the current kids
				delete n->getKid(0);
				n->setKid(NULL, 0);
				delete n->getKid(1);
				n->setKid(NULL, 1);
				
				// Finally, redistribute this addition
				n->setKid(dummy, 0);
				//n->setKid(new ConstNode(n->belongsTo(), my_const+other_const, RegisterInstance::invalid), 1);
				n->setKid(IR::make_const(n->belongsTo(), my_const+other_const, RegType::invalid), 1);

#ifdef _DEBUG_OPTIMIZER
				std::cout << "[Redistribution]: nodo alla fine della redistr:" << std::endl;
				n->printNode(std::cout, true);
				std::cout << std::endl;
#endif
				_changed = true;
				return true;
			}
		}
	}

	return false;
}


template <class _CFG>
bool Redistribution<_CFG>::RedistributionFunctor::is_add_with_constant(typename _CFG::IRType *n, uint32_t &con, typename _CFG::IRType *&other)
{
	assert(n && "Parent addition did not have a son!");

	if(n->getOpcode() != ADD && n->getOpcode() != SUB)
		return false;
		
	assert(n->getKid(0) && n->getKid(1) && "Addition without kids!");
	
	for(uint32_t i(0); i < 2; ++i) {
		if(n->getKid(i)->isConst())
		{
			typename IR::ConstType *cnst(dynamic_cast<ConstType *>(n->getKid(i)));
			if(!cnst)
				continue;

			if(n->getOpcode() == ADD)
				con = cnst->getValue();
			else
				con = - cnst->getValue();
			other = n->getKid((i+1)%2);

			return true;
		}
	}
	
	return false;
}

} /* namespace jit */
} /* namespace opt */

#endif /* __REDISTRIBUTION_H__ */
