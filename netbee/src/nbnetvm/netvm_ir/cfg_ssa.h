/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef __CFG_SSA_H__
#define __CFG_SSA_H__

/*!
 * \file cfg_ssa.h
 * \brief this file contains the declaration of the routine to build the SSA form
 */
//#include "netvmjitglobals.h"
//#include "../opcode_consts.h"
#include "cfg.h"
#include "cfg_liveness.h"
#include "irnode.h"
#include "basicblock.h"
#include "registers.h"
#include "cfgdom.h"
//#include "jit_internals.h"
#include <stack>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <iostream>
#include <string>
#include <typeinfo>

//#include "cfg_printer.h"

namespace jit {

template<typename _CFG>
class Place_Phis {
  public:
	//Template Specializations
	typedef _CFG CFG;
	typedef typename CFG::RegType RegType;
	typedef typename CFG::BBType BBType;
	typedef typename _CFG::IRType IRType;
	typedef typename std::set<BBType *, BBIDCmp<_CFG> > BBSet_t;
	typedef typename std::set<RegType> RegSet_t;
	typedef typename std::map<uint32_t, RegSet_t > RegMap_t;
	typedef typename std::map<uint32_t, uint32_t> IDMap_t;
	typedef typename std::map<RegType, BBSet_t> RegToBBMap_t;
	typedef typename std::pair<RegType, BBSet_t> RegToBBPair_t;

  private:
	CFG *cfg;
	RegToBBMap_t defsites;
	RegMap_t orig;
	RegMap_t phi;
	uint32_t iter_count;
	IDMap_t has_already;
	IDMap_t work;

	int32_t fill_defsites(BBType *);
	int32_t fill_defsites_tree(IRType *, BBType *);

	class _place_phis {
	  private:
		RegToBBMap_t &defsites;
		RegMap_t &orig;
		RegMap_t &phi;
//		uint32_t &iter_count;
//		IDMap_t &has_already;
//		IDMap_t &work;
		
		BBSet_t t;
		typename BBSet_t::iterator j;
		BBType *jj;
//		std::pair<RegType, std::set<BasicBlock<typename CFG::IRType> *> > i;
		
//		static int32_t place_inner_loop(BasicBlock *df_son, _place_phis *p);
//		int32_t place_inner_loop(BasicBlock<typename CFG::IRType> *df_son,
//			std::set<BasicBlock<typename CFG::IRType> *> *W);

	  public:
		_place_phis(RegToBBMap_t &ds,
			RegMap_t &o,
			RegMap_t &p,
			uint32_t &iter_count,
			IDMap_t &has_already,
			IDMap_t &work);

		bool operator()(RegToBBPair_t);
	};

  public:
	Place_Phis(CFG *cfg);
	
	void run();
};

template<typename _CFG>
class RemovePhis {
  public:
	typedef _CFG CFG;
	typedef typename CFG::BBType BBType;

  private:
	CFG *cfg;
	std::set<typename CFG::IRType::RegType> dead_phi_regs;
	
	bool erasure_done;
	
	bool scan_bb_remove_phi(BBType *bb);
	bool find_live_phis(BBType *bb);
//	bool  _find_live_phis(typename CFG::IRType *i);
	bool remove_dead_phis(BBType *bb);


  public:
	RemovePhis(CFG *);
	
	void run();
};

template<typename _CFG>
class UndoSSA: public nvmJitFunctionI {
  private:
	typedef _CFG CFG;
	typedef typename CFG::BBType 	BBType;
	typedef typename CFG::RegType	Register;
	typedef typename CFG::IRType 	IRType;
	
	CFG &cfg;
	
  public:
	UndoSSA(CFG &);
	
	bool run();
  
};

template <typename _CFG>
class SSA: public nvmJitFunctionI {
  public:
	typedef _CFG CFG;
	typedef typename CFG::RegType::ModelType Model;
	typedef typename CFG::RegType			 Register;
	typedef uint32_t						 Version;
	typedef typename CFG::BBType BBType;

  private:
	std::map<Model, Version> counters;
	std::map<Model, std::stack<Version> > stacks;
	uint32_t gCounter;
	
	CFG &cfg;
	jit::Liveness<CFG> liveness;
  

  	/*!
	 * \brief create a new SSA register 
	 * \param reg the old name
	 * \param counters a vector containing the counters for every register
	 * \param stacks a stack for every register
	 * \param gCounter global counter
	 */
	Register SSANewName(Model *reg);
	
	/*!
	 * \brief rename a phi function definition
	 * \param BB the BB whose phi function will be renamed
	 * \param counters a counter for each register
	 * \param stacks a stack for every register
	 * \param gCounter global counter
	 */
	void renamePhiFunctDefs(BBType *BB);
			
	/*!
	 * \brief rename a BB's functions arguments
	 * \param BB the BB whose functions will be renamed
	 * \param counters a counter for each register
	 * \param stacks a stack for every register
	 * \param gCounter global counter
	 *
	 */
	void renameInstructionsArgs(BBType  *BB);
			
	void fillSuccPhiParms(BBType * BB);
		
	void renameSuccInDomTree(BBType *BB);
		
	void popBBDefRegs(BBType* BB);
		
	void SSARenameNode(BBType* BB);


	//!a functor to insert phi nodes at the beginning of a basic block
	struct InsertPhiInNode {
		typedef typename _CFG::node_t node_t;
		_CFG &cfg;
		
		InsertPhiInNode(_CFG &_cfg) : cfg(_cfg) {};
		void operator()(node_t *node) {
			BBType* BB = node->NodeInfo;		
			if(BB->getPredecessors().size() <= 1)
				return;

			typedef typename BBType::Liveness_Info info_t;
			info_t *info = BB->get_LivenessInfo();

			typename std::set<Register>::iterator i;
			for(i = info->LiveInSet.begin(); i != info->LiveInSet.end(); ++i) {
				typename CFG::IRType::PhiType *phiNode = CFG::IRType::make_phi(BB->getId(), *i);
				BB->addHeadInstruction(phiNode);
			}

			return;
		};
	};
	
  public:
	/*!
	 * \param cfg The cfg to rename
	 */
	SSA(CFG &cfg);
	
	/*!
	 * Run the SSA algorithm.
	 */
	bool run();

}; /* class SSA */

template<typename _CFG>
SSA<_CFG>::SSA(_CFG &_cfg) : nvmJitFunctionI("SSA"), gCounter(0), cfg(_cfg), liveness(cfg)
{
	// Nop
}

template<typename _CFG>
bool SSA<_CFG>::run()
{
#if 0
	//insert phi function pruned
	cfg.SortRevPostorder(*cfg.getEntryNode());
	std::for_each(cfg.FirstNodeSorted(), cfg.LastNodeSorted(), InsertPhiInNode(cfg));
#endif

	// Phi placement algorithms
	Place_Phis<CFG> pp(&cfg);
	pp.run();
	
	// Stampa il CFG risultante
#ifdef _DEBUG_SSA
	{
		std::cout << "Dopo inserimento phi " << cfg.getName() << std::endl;
		//jit::CodePrint<typename CFG::IRType , jit::SSAPrinter<typename CFG::IRType > > codeprinter(cfg);
		//std::cout << codeprinter;
	}
#endif


	// FIXME: does globalCounter need to be initialized?
	SSARenameNode(cfg.getEntryNode()->NodeInfo);
	
#ifdef _DEBUG_SSA
	{
		std::cout << "Dopo renaming phi " << cfg.getName() << std::endl;
		//jit::CodePrint<typename CFG::IRType , jit::SSAPrinter<typename CFG::IRType > > codeprinter(cfg);
		//std::cout << codeprinter;
	}
#endif
	
	// Spurious phi removal
	RemovePhis<CFG> rp(&cfg);	
	rp.run();

	//[PV]DO we have to update cfg.manager.getCount()???
	//look in SSANewName
	cfg.setInSSAForm(true);
	
#ifdef _DEBUG_SSA
	{
		std::cout << "After spurious phi removal " << cfg.getName() << std::endl;
		//jit::CodePrint<typename CFG::IRType , jit::SSAPrinter<typename CFG::IRType > > codeprinter(cfg);
		//std::cout << codeprinter;
	}
#endif

	
	return true;
}


/*!
 * \brief create a new SSA register 
 * \param reg the old name
 * \param counters a vector containing the counters for every register
 * \param stacks a stack for every register
 * \param gCounter global counter
 */
template<typename _CFG>
typename SSA<_CFG>::Register SSA<_CFG>::SSANewName(Model *reg)
{
	Version i;
	Version newName;

	//	std::cout << "Chiesto un nuovo nome per il registro " << SSA_PREFIX(reg) << std::endl;
	if (*reg == Model::invalid)
		return 0; //nvmJitSUCCESS;

	i = counters[*reg];
	counters[*reg]++;
	gCounter++;
	newName = i;
	stacks[*reg].push(newName);
	
	RegisterInstance new_reg(reg, newName);
//	std::cout << "\n++++++++++++ push di " << new_reg << '\n';
	
	return new_reg;
}

//template<typename _CFG>
//SSA<_CFG>::InsertPhiInNode<_CFG>::InsertPhiInNode(typename SSA<_CFG>::CFG<typename CFG::IRType>& cfg)
//	: cfg(cfg)
//{
//	// Nop
//}

///*!
// * \brief inserts phi nodes at the beginning of a basic block
// * \param node the basic block
// *
// * For every basic block that has more than one predecessor inserts a phi
// * for every variable in this basic block LiveInSet
// */
//template<typename _CFG>
//void SSA<_CFG>::InsertPhiInNode<_CFG>::operator()(node_t *node)
//{
//
//}

template<typename _CFG>
void SSA<_CFG>::renamePhiFunctDefs(typename _CFG::BBType *BB)
{
	typedef typename BBType::IRStmtNodeIterator nodeIT;

	for(nodeIT i = BB->codeBegin(); i != BB->codeEnd(); i++) {
		if((*i)->isPhi() ) {
// WTF?!?
//			PhiMIRNode *phiNode = dynamic_cast<PhiMIRNode*>(*i);
			(*i)->setDefReg(SSANewName((*i)->getDefReg().get_model()));
		}
	}
	
	return;
}

template<typename _CFG>
void SSA<_CFG>::renameInstructionsArgs(typename SSA<_CFG>::BBType *BB)
{
//	std::cout << "---- eseguo per il BB " << BB->getId() << '\n';


	typedef typename BBType::IRStmtNodeIterator nodeIT;
	for(nodeIT i = BB->codeBegin(); i != BB->codeEnd(); i++) {
		for(typename CFG::IRType::IRNodeIterator j = (*i)->nodeBegin(); j != (*i)->nodeEnd(); j++) {		
			Register *reg;
			
			if((*j)->isPhi() || (*j)->isPFLStmt())
				continue;
				
			//std::cout << "Considero il nodo\n";
			//j->printNode(std::cout, false);
			//std::cout << '\n';
			
			// FIXME: this is insufficient for nodes that define more than one register.
				std::set<Register> uses((*j)->getUses());
				typename std::set<Register>::iterator k;
				for(k = uses.begin(); k != uses.end(); ++k) {
					// XXX: avoid overwriting registers that were never defined.
					if(stacks.find(*(k->get_model())) != stacks.end() && stacks[*(k->get_model())].size()) {
//						std::cout << "comando renaming di " << *k << " in ";

						Register t(*k);
						t.version() = stacks[*(k->get_model())].top();
						
//						std::cout << t << '\n';

						(*j)->rewrite_use(*k, t);
					}
				}	
			
				std::set<Register> defs((*j)->getDefs());
				
				//std::cout << "Il nodo ";
				//j->printNode(std::cout, true);
				//std::cout << " definisce i registri:\n";
				//for(k = defs.begin(); k != defs.end(); ++k) {
				//	std::cout << *k << ' ';
				//}
				//std::cout << '\n';
				
				assert(defs.size() < 2); // We lack a way to rename a general register.
				for(k = defs.begin(); k != defs.end(); ++k) {
					reg = (*j)->getOwnReg();	// XXX BUG WRONG-O!!!
					assert(reg != NULL && defs.find(*reg) != defs.end());
					//std::cout << "rinomino la definizione " << *reg << " in ";
					assert(reg->version() == 0); 					
					reg->version() = SSANewName(reg->get_model()).version();
					//std::cout << *reg << '\n';
				} 
		}
	}
	
	return;
}

template<typename _CFG>
void SSA<_CFG>::fillSuccPhiParms(typename _CFG::BBType * BB)
{
	typedef typename BBType::BBIterator BBIterator;
	typedef typename BBType::IRStmtNodeIterator nodeIT;

	for(BBIterator succIt = BB->succBegin(); succIt != BB->succEnd(); succIt++) {
		for(nodeIT i = (*succIt)->codeBegin(); i != (*succIt)->codeEnd(); i++) {
			if( (*i)->isPhi()) { // Insert_Phi_Parameters
				//[PV] oldJIt checks the number of parameter
				
//				std::cout << typeid(*i).name() << '\n';
				
				typename CFG::IRType::PhiType *phiNode =
					dynamic_cast<typename CFG::IRType::PhiType *>(*i);
//				typename CFG::IRType::PhiType *phiNode = (typename CFG::IRType::PhiType *)(*i);
										

				RegisterModel *m = phiNode->getDefReg().get_model();
				
//				std::cout << "Tento di riempire una phi per il registro " << *m << '\n';
				
				if(!stacks[*m].size()) {
					phiNode->addParam(RegisterInstance::invalid, BB->getId());
				} else {
					phiNode->addParam(RegisterInstance(m, stacks[*(m)].top()), BB->getId());
				}
			}
		}
	}

	return;
}

template<typename _CFG>
void SSA<_CFG>::renameSuccInDomTree(typename _CFG::BBType *BB)
{
	typedef BBType * BasicBlockPtr;
	typedef typename std::set< BBType *, BBIDCmp<_CFG> >::iterator domSuccIt;
	std::set<BBType *, BBIDCmp<_CFG> >* domSuccessors;
	domSuccessors = BB->template getProperty<std::set<BBType *, BBIDCmp<_CFG> >*>("DomSuccessors");
	if(domSuccessors == 0)
		return;

	for(domSuccIt i = domSuccessors->begin(); i != domSuccessors->end(); i++) {
		//the parent must be the idom 
		assert( (*i)->template getProperty< BBType* >("IDOM") == BB );	
		//std::cout << "BB: " << BB->getId() << " Successor: " << (*i)->getId() << std::endl;
		SSARenameNode(*i);
	}
	//std::cout << std::endl;

	return;
}

template<typename _CFG>
void SSA<_CFG>::popBBDefRegs(typename _CFG::BBType* BB)
{
	typedef typename BBType::IRStmtNodeIterator nodeIT;
//	typename CFG::IRType::OpCodeType opcode;
	for(nodeIT i = BB->codeBegin(); i != BB->codeEnd(); i++) {
#if 0
		if((*i)->get_defined_count() == 1) {
			if(!((*i)->getDefReg() == Register::invalid))	{
#endif
		typename std::set<Register>::iterator j;
		std::set<Register> defs((*i)->getAllDefs());
		for(j = defs.begin(); j != defs.end(); ++j) {
//		if((*i)->getDefs()) {			
//				stacks[*((*i)->getDefReg().get_model())].pop();
//				std::cout << "Pop del registro ";
//				std::operator<<(std::cout, (*i)->getDefReg().get_model()) << std::endl;
//			}
			assert(*j != Register::invalid);
			assert(stacks[*(j->get_model())].size());
//			std::cout << "++++++++++++++++++++ Pop del registro " << *(j->get_model()) << '.' << stacks[*(j->get_model())].top() << '\n';
			stacks[*(j->get_model())].pop();
		}
	}	

	return;
}

template<typename _CFG>
void SSA<_CFG>::SSARenameNode(typename _CFG::BBType* BB)
{
//	std::cout << "---------------- INIZIO L'ANALISI DEL BB " << BB->getId() << " ---------\n";

	renamePhiFunctDefs(BB);
	renameInstructionsArgs(BB);
	fillSuccPhiParms(BB);
	renameSuccInDomTree(BB);
	popBBDefRegs(BB);
	
//	std::cout << "---------------- FINISCO L'ANALISI DEL BB " << BB->getId() << " ---------\n";
	return;
}







template<typename _CFG>
int32_t Place_Phis<_CFG>::fill_defsites_tree(IRType *n, typename _CFG::BBType *b)
{
	typename std::set<typename CFG::IRType::RegType> defs(n->getDefs());
	typename std::set<typename CFG::IRType::RegType>::iterator i;

	for(i = defs.begin(); i != defs.end(); ++i) {
#ifdef _DEBUG_SSA
  	std::cout << "Aggiungo " << b->getId() << " ai defsites di " << *i << '\n';
#endif
		defsites[*i].insert(b);
		orig[b->getId()].insert(*i);
	}
	
#if 0
	if(n->get_defined_count() == 1 /* && n->Code != LDREG */) {
//		std::cout << "Il BB " << b->getId() << " definisce la variabile " << n->getDefReg() << '\n';
		defsites[n->getDefReg()].insert(b);
		orig[b->getId()].insert(n->getDefReg());
	}
#endif

	return 0; //nvmJitSUCCESS;
}

template<typename _CFG>
int32_t Place_Phis<_CFG>::fill_defsites(typename _CFG::BBType *nodeN)
{
	typename _CFG::BBType::IRStmtNodeIterator i;
	for(i = nodeN->codeBegin(); i != nodeN->codeEnd(); ++i) {
		typename IRType::IRNodeIterator j((*i)->nodeBegin());
		for(; j != (*i)->nodeEnd(); j++) {
			fill_defsites_tree(*j, nodeN);
		}
	}
	return 0; //nvmJitSUCCESS;
}

template<typename _CFG>
Place_Phis<_CFG>::Place_Phis(CFG *c) : cfg(c)
{
	// Fill the defsites map
	std::list<BBType *> *list(c->getBBList());
	typename std::list<BBType *>::iterator i;
	
	for(i = list->begin(); i != list->end(); ++i) {
		fill_defsites(*i);
	}
	
	delete list;

	return;
}

template<typename _CFG>
void Place_Phis<_CFG>::run()
{
#ifdef _DEBUG_SSA
	std::cout << "!!!---!!! INIZIO PLACE PHIS !!!---!!!\n";
#endif

	iter_count = 0;
	has_already.clear();
	work.clear();
	
	typename RegToBBMap_t::iterator i;
//	for(i = defsites.begin(); i != defsites.end(); ++i)
//		std::cout << i->first << '\n';

	_place_phis p(defsites, orig, phi, iter_count, has_already, work);

	// Do the real algorithm
	std::for_each(defsites.begin(), defsites.end(), p);
	
#ifdef _DEBUG_SSA
	std::cout << "!!!---!!! FINE PLACE PHIS !!!---!!!\n";
#endif
	
	return;
}

template<typename _CFG>
//Place_Phis<_CFG>::_place_phis::_place_phis(std::map<RegType, std::set<typename Place_Phis<_CFG>::BBType *> > &ds,
Place_Phis<_CFG>::_place_phis::_place_phis(RegToBBMap_t &ds,
												RegMap_t &o,
												RegMap_t &p,
												uint32_t &ic,
												IDMap_t &ha,
												IDMap_t &w) :
	defsites(ds), orig(o), phi(p) /*, iter_count(ic), has_already(ha), work(w) */
		/*, i(RegType::invalid, std::set<BasicBlock<typename CFG::IRType> *>()) */
{
	// Nop
}

template<typename _CFG>
bool Place_Phis<_CFG>::_place_phis::operator()(RegToBBPair_t z)
{
	t = z.second;
	
#ifdef _DEBUG_SSA
	std::cout << "Passata per il registro " << z.first << '\n';
#endif

	// Cycle on every BB where the register z.first has been defined
	while((j = t.begin()) != t.end()) {
		jj = *j;			// Get the BB
		t.erase(j);			// Remove from the working list
#ifdef _DEBUG_SSA
		std::cout << "Analizzo il BB " << jj->getId() << '\n';
#endif

		// Fetch the dom frontier for this BB
		std::set<BBType *, BBIDCmp<_CFG> > *domFrontier(0);		
		domFrontier = jj->template getProperty<std::set< BBType*, BBIDCmp<_CFG> > *>("DomFrontier");
		
		if(!domFrontier) {
			continue;
		}
		
		// Iteratre over the dom frontier - the current BB in *n
		typename std::set<BBType *, BBIDCmp<_CFG> >::iterator n;
		for(n = domFrontier->begin(); n != domFrontier->end(); ++n) {
		
#ifdef _DEBUG_SSA
			std::cout << "Considero " << (*n)->getId() << " nella DF di " << jj->getId() << '\n';
#endif

			if(phi[(*n)->getId()].find(z.first) == phi[(*n)->getId()].end()) { /* Not yet inserted */
#ifdef _DEBUG_SSA
				std::cout << "Inserisco una phi per " << z.first << " in " << (*n)->getId() << '\n';
#endif
			
				typename CFG::IRType::PhiType *phiNode = CFG::IRType::make_phi((*n)->getId(), z.first);
				(*n)->addHeadInstruction(phiNode);

				phi[(*n)->getId()].insert(z.first);

				/* Since the newly inserted PHI is under every account a definition of z.first, we should add
				 * the basic block to the working list - but only _once_ */
				if(orig[(*n)->getId()].find(z.first) == orig[(*n)->getId()].end()) {
#ifdef _DEBUG_SSA
					std::cout << "Inserisco " << (*n)->getId() << " nella work list\n";
#endif
					t.insert((*n));
				}
			}
		}
	}

	return true;
}

#if 0
template<typename _CFG>
int32_t Place_Phis<_CFG>::_place_phis::place_inner_loop(BasicBlock<typename CFG::IRType> *df_son,
	std::set<BasicBlock<typename CFG::IRType> *> *W)
{

	if(has_already[df_son->getId()] < iter_count) {
//		std::cout << "Non ho ancora inserito nell'iterazione corrente\n";

		typename CFG::IRType::PhiType *phiNode = CFG::IRType::make_phi(df_son->getId(), i.first);
//		std::cout << "Piazzo una phi in " << df_son->getId() << " per il registro " << i.first << '\n';
		df_son->addHeadInstruction(phiNode);

		has_already[df_son->getId()] = iter_count;

		if(work[df_son->getId()] < iter_count) {
//			std::cout << "Reinserisco perche' modificato nell'iterazione corrente\n";
			work[df_son->getId()] = iter_count;
			W->insert(df_son);
		}
	}

	
	return nvmJitSUCCESS;
}
#endif




template<typename _CFG>
RemovePhis<_CFG>::RemovePhis(_CFG *c) : cfg(c)
{
	// Nop
	return;
}

template<typename _CFG>
void RemovePhis<_CFG>::run() {

	std::list<typename _CFG::BBType*> *list(cfg->getBBList());

	// Removing a phi can render another phi dead, so we iterate until there no more removals happen.
	do {
		erasure_done = false;
		dead_phi_regs.clear();
		
		{
			/* This is not strictly required, but it fills the dead_phi_regs set on the fly and
			 * it does no harm */

			typename std::list<typename _CFG::BBType*>::iterator i;
	
			for(i = list->begin(); i != list->end(); ++i) {
				scan_bb_remove_phi(*i);
			}

			for(i = list->begin(); i != list->end(); ++i) {
				find_live_phis(*i);
			}
			
		}

#if 0
		{
			typename std::set<typename CFG::IRType::RegType>::iterator i;
			std::cout << "Elenco di registri phi-definiti morti:\n";
			for(i = dead_phi_regs.begin(); i != dead_phi_regs.end(); ++i)
				std::cout << *i << '\n';
		}
#endif

		{
			typename std::list<typename _CFG::BBType*>::iterator i;
			
			for(i = list->begin(); i != list->end(); ++i) {
				remove_dead_phis(*i);
			}
		}
	} while(erasure_done == true);
	
	delete list;

	return;
}

template<typename _CFG>
bool RemovePhis<_CFG>::remove_dead_phis(typename _CFG::BBType *bb)
{
	typename CFG::IRType *i(0);
	typename BBType::IRStmtNodeIterator j, k;
	
	/* We scan only the beginning of the BB, where only PHIs are normally present;
	 * however, our algorithms has already replaced some phis with nops, so we must
	 * accept them as well */
	for(j = bb->codeBegin(); j != bb->codeEnd() && (i = *j, (i->isPhi()));) {
		if(dead_phi_regs.find((*j)->getDefReg()) != dead_phi_regs.end()) {
			erasure_done = true;
			k = j;
			++j;
			bb->eraseCode(k);
			//delete *k;
			//std::cout << "BB: " << bb->getId() << "E' stata rimossa una PHI" << std::endl;
		} else {
			++j;
		}
	}
	
	return true;
}

template<typename _CFG>
bool RemovePhis<_CFG>::find_live_phis(typename _CFG::BBType *bb)
{
	typename BBType::IRStmtNodeIterator i;
	
	for(i = bb->getCode().begin(); i != bb->getCode().end(); ++i) {
		//std::cout << nvmOpCodeTable[(*i)->getOpcode()].CodeName << " usa: ";
		//(*i)->printNode(std::cout, true) << " usa: ";
	
		std::set<typename CFG::IRType::RegType> uses((*i)->getAllUses());
		
		typename std::set<typename CFG::IRType::RegType>::iterator j;
		//for(j = uses.begin(); j != uses.end(); ++j)
		//	std::cout << *j << ' ';
		//std::cout << '\n';
		
		std::set<typename CFG::IRType::RegType> t(dead_phi_regs);
		dead_phi_regs.clear();
		
		std::set_difference(t.begin(), t.end(), uses.begin(), uses.end(),
			std::insert_iterator<std::set<typename CFG::IRType::RegType> >(dead_phi_regs, dead_phi_regs.begin()));
		
//		dead_phi_regs.erase(uses.begin(), uses.end());
	}
		
	return true;
}

template<typename _CFG>
bool RemovePhis<_CFG>::scan_bb_remove_phi(typename _CFG::BBType *bb)
{
	// Scan the BB for phis and remove them if any arg is invalid
	typename CFG::IRType *i(0);
	typename BBType::IRStmtNodeIterator j, l;

	for(j = bb->getCode().begin(); j != bb->getCode().end();) {
		i = *j;

		assert(i);
		
		if(!i->isPhi())
			break;
		
		bool removed(false);
		
		// FIXME: we need to obtain the phi object, and check its parameters.
		typename CFG::IRType::PhiType *phiNode = dynamic_cast<typename CFG::IRType::PhiType *>(i);
		typename CFG::IRType::PhiType::params_iterator k;
		
		//std::cout << "Esamino la phi per " << i->getDefReg() << '\n';


		for(k = phiNode->paramsBegin(); !removed && k != phiNode->paramsEnd(); ++k) {
			if(k->second == CFG::IRType::RegType::invalid) {
				//std::cout << "WARNING: POSSIBLE UNDEFINED USAGE OF VARIABLE DETECTED\n";
//				removed = true;
			}
		}

		if(i->getDefReg() == CFG::IRType::RegType::invalid) {
			//std::cout << "Elimino per reg. definito nullo\n";
			removed = true;
		}

		if(!removed) {
//			std::cout << "Non elimino, pero ora\n";
			// We assume every phi to be dead code
			dead_phi_regs.insert(i->getDefReg());
			++j;
		} else {
			// Do the actual phi deletion
			l = j;
			++j;
			bb->getCode().erase(l);
			delete *l;
		}
	}

	return true;
}






template<typename _CFG>
UndoSSA<_CFG>::UndoSSA(CFG &c) : nvmJitFunctionI("Undo SSA"),  cfg(c)
{
	// Nop
}

template<typename _CFG>
bool UndoSSA<_CFG>::run()
{
	typedef typename BBType::IRStmtNodeIterator nodeIT;

	// Precondition: the CFG is in split-edge form and thus free of critical edges

	// Iterate over BBs
	typename std::list<BBType *>::iterator i;
	typedef std::list<IRType*> code_l;
	typedef typename code_l::iterator code_li;

	std::list<BBType *> *il(cfg.getBBList());
	
	for(i = il->begin(); i != il->end(); ++i) {
		// For each BB, iterate over phis
		nodeIT j;

		for(j = (*i)->codeBegin(); j != (*i)->codeEnd() && (*j)->isPhi(); ++j) {
			// For each phi, iterate over the arguments / predecessors
			typename CFG::IRType::PhiType *phiNode =
				dynamic_cast<typename CFG::IRType::PhiType *>(*j);
				
//			typename std::vector<uint16_t> &sources(phiNode->param_bb);
//			typename std::vector<uint16_t>::iterator q;
				
			typename CFG::IRType::PhiType::params_iterator ka;
//			std::list<typename BasicBlock<typename CFG::IRType>::node_t *> pred_list = (*i)->getPredecessors();
//			typename std::list<typename BasicBlock<typename CFG::IRType>::node_t *>::iterator kb;
			
//			assert(pred_list.size() > 1);

			for(ka = phiNode->paramsBegin(); ka != phiNode->paramsEnd(); ++ka) {
				if(ka->second != CFG::IRType::RegType::invalid) {
					// Add a copy operation to the tail of the appropriate predecessor node
					BBType *src(cfg.getBBById(ka->first));
					src->add_copy_tail(ka->second, phiNode->getDefReg());
				}
			}

#if 0			
			for(q = sources.begin(), ka = phiNode->paramsBegin(), kb = pred_list.begin(); ka != phiNode->paramsEnd();
				++ka, ++kb, ++q) {
				assert(kb != pred_list.end());
				
				assert(*q == (*kb)->NodeInfo->getId());

				if(*ka != CFG::IRType::RegType::invalid) {
					// Add a copy operation to the tail of the appropriate predecessor node
					(*kb)->NodeInfo->add_copy_tail(*ka, phiNode->getDefReg());
				}
			}
#endif
		}
		
		// Delete this BB's phi functions
		//for(j = (*i)->codeBegin(); j != (*i)->codeEnd() && (*j)->isPhi(); ++j)
		//	;
		//	
		//(*i)->getCode().erase((*i)->codeBegin(), j);
		code_l &codeList = (*i)->getCode();
		for(code_li j = codeList.begin(); j != codeList.end(); )
		{
			IRType *phi;
			if( (*j)->isPhi() )
			{
				phi = *j;
				j = codeList.erase(j);
				delete phi;
			}
			else
				j++;
		}
	}
	
	delete il;
	
	return true;
}


} /* namespace jit */


#if 0
#define SSA_MASK 0xFFFF0000 
//!get the SSA prefix of a register
#define SSA_PREFIX(VAR) (VAR & ~SSA_MASK)
//!get the SSA suffix of a register
#define SSA_SUFFIX(VAR) ((VAR & SSA_MASK) >> 16)
#endif

#if 0
namespace jit
{
	//! the main routine to call to put a CFG in SSA form
	void translateIntoSSA(CFG<MIRNode>& cfg);
	//! the main routine which rewrites the register in SSA form
	void SSARenameNode(BasicBlock<MIRNode>* BB, 
			std::vector<uint32_t> &counters,
			std::vector<std::stack<uint32_t> > &stacks,
			uint32_t &gCounter);
}
#endif

#endif /* __CFG_SSA_H__ */
