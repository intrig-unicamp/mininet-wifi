/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef _REASSOCIATION_H
#define _REASSOCIATION_H

#include <set>
#include <map>
#include <list>
#include <iostream>
#include <algorithm>
#include "cfg.h"
#include "cfg_liveness.h"
#include "ssa_graph.h"
#include "irnode.h"
#include "function_interface.h"

//FIXME
#ifdef _JIT_BUILD
#include "jit_internals.h"
#endif
#ifdef _PFL_BUILD
#include "tree.h"
#endif

using std::min;

namespace jit {

	template <class _CFG>
class MemoryBarrier {


  private:
	uint32_t latest_pkt_version;
	uint32_t latest_data_version;

  public:
	typedef _CFG CFG;
	typedef typename CFG::IRType IRType;
	typedef typename IRType::RegType RegType;
	typedef typename CFG::BBType BBType;

	MemoryBarrier(BBType &);
	MemoryBarrier();
	~MemoryBarrier();
	
	void start_scan();
	
	bool is_current(IRType *);
	void mark(IRType *);
};


template <class _CFG>
class Reassociation: public nvmJitFunctionI {
	public:
		typedef _CFG CFG;
		typedef typename CFG::IRType IRType;
		typedef typename IRType::RegType RegType;
		typedef typename _CFG::BBType BBType;

	private:
		CFG &cfg;
		Liveness<CFG> liveness;
		SSA_Graph<CFG> *graph;
		std::map<RegType, IRType *> substitution;
		std::set<RegType> pinned;
		MemoryBarrier<CFG> *mb;

		typedef typename std::list<IRType *>::iterator iter_t;

		void process_node(BBType *bb, IRType *t);
		void fix_stores(BBType *bb /*, std::list<IRType *>::iterator t */);
		uint8_t num_kids;
		
		bool findCopro(IRType *n);

	public:
		Reassociation(CFG &cfg);
		~Reassociation();

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
		uint32_t Reassociation<_CFG>::numModifiedNodes(0);
	template <class _CFG>
		void Reassociation<_CFG>::incModifiedNodes() { numModifiedNodes++; }
	template <class _CFG>
		uint32_t Reassociation<_CFG>::getModifiedNodes() { return numModifiedNodes; numModifiedNodes=0; }
#endif

// TEMPLATE IMPLEMETATIONS
	template <class _CFG>
MemoryBarrier<_CFG>::MemoryBarrier(typename _CFG::BBType &bb) : latest_pkt_version(0), latest_data_version(0)
{
	list<IRType *> &code(bb.getCode());
	typename list<IRType *>::iterator i;
	
	for(i = code.begin(); i != code.end(); ++i)
	{
		typename IRType::IRNodeIterator j((*i)->nodeBegin());
		for(; j != (*i)->nodeEnd(); j++)
		{
			if((*j)->isOpMemPacket())
			{
				if( (*j)->isMemStore() )
				{
					++latest_pkt_version;
				}

				(*j)->template setProperty<uint32_t>("barrier", latest_pkt_version);
			}	
			else if((*j)->isOpMemData())
			{
				if( (*j)->isMemStore() )
				{
					++latest_data_version;
				}

				(*j)->template setProperty<uint32_t>("barrier", latest_data_version);
			}	
		}

		}

		latest_pkt_version = 0;
		latest_data_version = 0;

		return;
	}

	template<class _CFG>
		MemoryBarrier<_CFG>::MemoryBarrier() : latest_pkt_version(0), latest_data_version(0)
{
	// Nop
	return;
}

template <class _CFG>
MemoryBarrier<_CFG>::~MemoryBarrier()
{
	// Nop
	return;
}

template <class _CFG>
void MemoryBarrier<_CFG>::start_scan()
{
	latest_pkt_version = latest_data_version = 0;
	return;
}

template <class _CFG>
void MemoryBarrier<_CFG>::mark(typename _CFG::IRType *n)
{
	uint32_t version(n->template getProperty<uint32_t>("barrier"));

	if(n->isOpMemPacket())
	{
		if(n->isMemStore())
		{
			assert(version > latest_pkt_version);
			latest_pkt_version = version;
		} else {
			assert(version == latest_pkt_version);
		}
	}				
	else if(n->isOpMemData())
	{
		if(n->isMemStore())
		{
			assert(version > latest_data_version);
			latest_data_version = version;
		} else {
			assert(version == latest_data_version);
		}
	}

	return;
}

template <class _CFG>
bool MemoryBarrier<_CFG>::is_current(typename _CFG::IRType *n)
{
	if(!n)
		return true;

	uint32_t version(n->template getProperty<uint32_t>("barrier"));
	//uint32_t expected;
	bool good(true);
	
	if(n->isOpMemPacket())
	{
		if(n->isMemStore())
		{
			good = false; // It is NEVER good to move stores around, and we must not end up here, for that matter
		} else {
			good = (version == latest_pkt_version);
		}

	}	

	if(n->isOpMemData())
	{
		if(n->isMemStore())
		{
			good = false;
		} else {
			good = (version == latest_data_version);
		}

	}	
	
	if(good) {
		// Here we assume that we won't encounter any definition in the subtrees, which is entirely reasonable
		good = good && is_current(n->getKid(0)) && is_current(n->getKid(1));
	}

	return good;
}


template <class _CFG>
Reassociation<_CFG>::Reassociation(_CFG &c) : cfg(c), liveness(cfg), graph(NULL), mb(NULL) 
{
	liveness.run();
	num_kids = IRType::num_kids;
	return;
}

template <class _CFG>
Reassociation<_CFG>::~Reassociation()
{
	if(graph) {
		delete graph;
		graph = 0;
	}
	return;
}


template <class _CFG>
bool Reassociation<_CFG>::run()
{	typename std::list<BBType *>::iterator i;
	std::list<BBType *> *bblist(cfg.getBBList());
	
	bool done_something(false);

	for(i = bblist->begin(); i != bblist->end(); ++i) {
		bool done;

		do {
			done = true;
			graph = new SSA_Graph<CFG>(cfg, **i);
			mb = new MemoryBarrier<CFG>(**i);
			
			// Clear the pinned set before the visit starts
			pinned.clear();
			mb->start_scan();

			BBType *bb(*i);
			std::list<IRType *>& nodes(bb->getCode());
			typename std::list<IRType *>::iterator ni;
			
			substitution.clear();
			
			for(ni = nodes.begin(); ni != nodes.end(); ++ni) {
//				substitution.clear();
//				if(!(*ni)->isJump()) {
					process_node(bb, *ni);
//				}
			}
			
			//IVANO: prima processa tutti i nodi del BB, e poi va a fare i FIX

			// If process_node does anything, it fills the substitution table.		
			if(substitution.size()) {
				//IVANO: in the filter under exam, it enters here just once, and into the BB with the error...
				//Then, I think that the fix_stores is wrong. An alternative it is that the insertion in the
				//substitution list is wrong
			
				done = false;
				done_something = true;
					
				/* All the nodes in the definition map are STREGs whose kids have been
				 * reassociated. We must therefore move the STREGs _after_ the current statement
				 * and make them store the substituted register/value (if it is a weirdo).
				 */			
				//IVANO: se non eseguo questo FIX nel BB 11, il codice funziona correttamente!!!!!!!!!!!!!
				
				fix_stores(bb /*, ni */);				

				break;
			}

			delete graph;
			delete mb;
			graph = 0;
			mb = 0;
		} while(!done);
		
		// Now insert the final stores
		
	}

	delete bblist;
	
	return done_something;
}

template <class _CFG>
bool Reassociation<_CFG>::findCopro(typename _CFG::IRType *n)
{
	if(!n)
		return false;

	for(uint32_t i(0); i < num_kids; ++i)
	{
		if(findCopro( n->getKid(i)))
			return true;
	}
	
	if(n->getOpcode() == COPRUN)
		return true;	
	else 
		return false;
}



template <class _CFG>
void Reassociation<_CFG>::process_node(typename _CFG::BBType *bb, typename _CFG::IRType *n)
{
	if(!n)
		return;

	// Try the kids - order DOES matter.
	for(uint32_t i(0); i < num_kids; ++i)
		process_node(bb, n->getKid(i));
		
	// Process this node
	for(uint32_t i(0); i < num_kids; ++i) {

#ifdef _DEBUG_REASS
		std::cout << "Processo il nodo: "; 
		n->printNode(std::cout, true);
		std::cout << std::endl;
#endif

		IRType *kid(n->getKid(i));
		if(!kid)
			continue;

#ifdef _DEBUG_REASS
		std::cout << "\tFiglio ";
		kid->printNode(std::cout, true);
		std::cout << std::endl;
#endif
		if( !kid->isLoad() )
			continue;
			
		// Ok, we have a broken tree. I repeat, code name broken tree. Send in the marines.
		 
		// If we have a substitution ready, employ it.
		typename map<RegType, IRType *>::iterator mi;

#ifdef _DEBUG_REASS
		std::cout << "\tIl figlio è: ";
		kid->printNode(std::cout, true);
		std::cout << std::endl;
#endif
			
		if((mi = substitution.find(kid->getDefReg())) != substitution.end()) {
			if(mi->second->getDefs().size() == 1) {
				assert(kid->getDefReg() != *(mi->second->getDefs().begin()));
				// Normal case, where only 1 reg. is defined
				//kid->setDefReg(mi->second->getDefReg());
#ifdef _DEBUG_REASS
				std::cout << "\t\tVoglio riscrivere il nodo con il registro " << *mi->second->getDefs().begin();
				std::cout << "\t\tNodo prima di setDefReg è: ";
				kid->printNode(std::cout, true);
				std::cout << std::endl;
#endif
				kid->setDefReg(*(mi->second->getDefs().begin()));
#ifdef _DEBUG_REASS
				std::cout << "\t\tNodo dopo setDefReg è ";
				kid->printNode(std::cout, true);
				std::cout << std::endl;
#endif
				assert(kid->getDefReg() == mi->second->getDefReg());
#ifdef _DEBUG_REASS
				std::cout << "\t\tSostituzione è: ";
				kid->printNode(std::cout, true);
				std::cout << std::endl;
#endif
				pinned.insert(kid->getDefReg());
			} else {
				// We end up here if we had a constant or a weirdo. The kid must be replaced.
				
				// XXX: we assume this node DOES NOT CONTAIN any memory references (neither read nor write)
				
#ifdef _DEBUG_REASS
				std::cout << "\t\tSostituzione è(weirdo): ";
				kid->printNode(std::cout, true);
				std::cout << std::endl;
#endif

				delete kid;
				IRType *clone(mi->second->copy());
				n->setKid(clone, i);
			}

			// Bail out, this kid is done for.
			continue;
		}
		
		// We arrive here if we got no luck. We look for a replacement.
					
		// If not, go look and find the definition
		pair<BBType *, IRType *> place(graph->get_def(kid->getDefReg()));

		
		// If the definition is not in this BB, let's skip it
		if(!place.first || !place.second)
			continue;

#ifdef _DEBUG_REASS
		std::cout << "BB ID: " << place.first->getId() << " ";
		std::cout << "\tLa definizione trovata dal grafo SSA per " << kid->getDefReg() << " è: ";
		place.second->printNode(std::cout, true);
		std::cout << std::endl;
#endif
			
		// Some red ribbon never hurt anybody
		assert(place.second->getDefReg() == kid->getDefReg());
			
		// If the definition is not a STREG, we are not interested in rejoining this node.
		IRType *source(place.second);
		
		if(!source->isStore() || substitution.find(source->getDefReg()) != substitution.end() ){
			// In this case, WARNING!
			// If the source node is be moved, this node will not see the definition any longer!
			// So we insert the defined register into the pinned set. If a STREG subtree defines a
			// pinned register, that STREG won't be moved!
			pinned.insert(kid->getDefReg());

			continue;
		} else {
			// Check to see if we are fine with the node we want to replace
			if(!mb->is_current(source)) {
#ifdef _DEBUG_REASS
				std::cout << "MEMORY BARRIER TRIGGERED\n";
#endif
				continue;
			}

			std::set<RegType> defs(source->getAllDefs());
			std::set<RegType> t;
			
			std::set_intersection(defs.begin(), defs.end(), pinned.begin(), pinned.end(),
				std::insert_iterator<std::set<RegType> >(t, t.begin()));
				
			if(t.size()) {
#ifdef _DEBUG_REASS
				std::cout << "PINNED REGISTER: " << kid->getDefReg() << '\n';
#endif
				continue;
			}

			//if this is a jump we do not reassociate the register
			//unless nobody else uses it 
			if(n->isJump())
			{
				RegType comp_reg(kid->getDefReg());

				std::set<RegType> liveout(liveness.get_LiveOutSet(bb));

				if(liveout.find(comp_reg) != liveout.end())
					continue;
			}
		}
			
		// If we are here, everything is going mighty well. We can replace the friggin' LDREG. Finally.
		assert(source->getTree()->getKid(0) && "STREG with a null kid!");

		// Replace the kid
		delete kid;
		kid = source->getTree()->getKid(0);
		n->setKid(kid->copy(), i);
		
		// Enter this node in the substitution map
		if(!n->isJump())
			substitution[source->getTree()->getDefReg()] = kid->copy();
#ifdef ENABLE_COMPILER_PROFILING
		Reassociation<_CFG>::incModifiedNodes();
#endif
		
		// Done, go on with the other kids.
	} /* for uint32_t i... */

	// Make visible the effects of this node if it was a memory operation
	mb->mark(n);

	return;
}

template <class _CFG>
void Reassociation<_CFG>::fix_stores(typename _CFG::BBType *bb /*, list<IRType *>::iterator ni */)
{
	list<IRType *> &code(bb->getCode());
	typename list<IRType *>::iterator i, j;
	typename map<RegType, IRType *>::iterator si;

	// --------------------------------------------------------------------------------
	// Sanity check. All the substituted STREGs must be situated BEFORE the current node
	set<RegType> defined_before_here;
	
	for(i = code.begin(); /* i != ni && */ i != code.end(); ++i) {
		//if((*i)->getOpcode() == STREG) {
		if( (*i)->getTree() && (*i)->getTree()->isStore() ){
			defined_before_here.insert((*i)->getTree()->getDefReg());

#ifdef _DEBUG_REASS
			std::cout << "Inserisco la definizione di" << (*i)->getTree()->getDefReg() << std::endl;
			std::cout << "definito dal nodo:";
			(*i)->getTree()->printNode(std::cout, true);
			std::cout << std::endl;
#endif
		}
	}
	
	// If we arrived at the end, something is messed up (we should've encountered ni!)
//	assert(i != code.end());
	
	// Check that every register substituted is present in defined_before_here
	for(si = substitution.begin(); si != substitution.end(); ++si) {
		assert(defined_before_here.find(si->first) != defined_before_here.end());
	}
	// --------------------------------------------------------------------------------
	
	// If we arrive here the sanity check is passed.
	map<RegType, list<IRType *> > t;
	list<IRType *> head;
	list<bool> isCopro;

	// Insert the required STREGs after the current statement
	//std::cout << "itero sulle sostituzioni" << std::endl;

	for(si = substitution.begin(); si != substitution.end(); ++si) 
	{

#ifdef _DEBUG_REASS
		std::cout << "[FixStores] Sostituzione:" << std::endl;
		std::cout << "[FixStores] Registro definito:" << si->first << " Nodo:" <<std::endl;
		si->second->printNode(std::cout, true);
		std::cout << std::endl;
#endif

		assert(si->second && "Trying to substitute a register with a null node!");

		// Register to be substituted
		RegType defReg(si->first);

//		// Let i point one location past ni, so that the new STREG get inserted after the current node
//		// But don't touch ni!
//		i = ni;
//		++i;
	
		// XXX this part is heavily IR-dependent, but that should be OK.
		
		if(si->second->getDefs().size() == 1) {
			//IRType *load(new LDRegNode(bb->getId(), si->second->getDefReg()));
			//IRType *streg(MIRNode::unOp(STREG, bb->getId(), load, defReg));

			IRType * streg = IRType::make_copy(bb->getId(), si->second->getDefReg(), defReg);
			IRType * load = streg->getTree()->getKid(0);
			
#ifdef _DEBUG_REASS
			cout << "Accodo ";
			streg->printNode(cout, true);
			cout << '\n';
#endif
			
			t[load->getDefReg()].push_back(streg);
		} else {
			// XXX: this will never work if the weirdo uses register
			// IVANO: if the warning indicated in the line above also includes instruction
			// to read data from coprocessors, e.g., copro.in(), I fixed the BUG in that case.
			
			
#ifdef _DEBUG_REASS
			cout << "Weirdo: ";
			si->second->printNode(cout, true);
			cout << '\n';
			std::cout << std::endl << "Number of uses: " << si->second->getAllUses().size() << std::endl; 
#endif
			
			assert((!si->second->getAllUses().size() || si->second->isLoad())
				&& "Special handling would be required");
				
			/*
			*	Ivano: if the weirdo is a "copro.in" instruction, we must be careful in
			*	optimizations.
			*/		
			if(si->second->getOpcode() == COPIN)
				isCopro.push_back(true);
			else
				isCopro.push_back(false);
		
			// We have a weirdo, copy it as it is
			//IRType *streg(MIRNode::unOp(STREG, bb->getId(), si->second->copy(), defReg));
			IRType *streg = IRType::make_store(bb->getId(), si->second->copy(), defReg);
			
			// Mark it as null, if it is used twice is a mistake.
			substitution[defReg] = 0;
			
#ifdef _DEBUG_REASS
			cout << "Accodo ";
			streg->printNode(cout, true);
			cout << '\n';
#endif
			
			head.push_back(streg);		
		}
		
		delete(si->second);
	}

#ifdef _DEBUG_REASS
	cout << "Effettuato inserimento nel BB " << bb->getId() << '\n';
#endif

	// Clean up the BB by deleting the incriminating STREGs
	for(i = code.begin(); i != /*ni */ code.end();) {
		j = code.end();

		//if((*i)->getOpcode() == STREG && substitution.find((*i)->getDefReg()) != substitution.end()) {
		if( (*i)->getTree() && (*i)->getTree()->isStore() && substitution.find((*i)->getTree()->getDefReg()) != substitution.end()){
			// This STREG is condemned
			j = i;
			++i;
		}
		
		if(j != code.end()) {
			// XXX nasty: if we just simply delete the node, the kid is deleted as well. But we are using it.
			IRType *clone((*j)->getKid(0)->copy());
			(*j)->setKid(clone, 0);
			
			// Now we are good to go
			delete(*j);
			code.erase(j);
		} else {
			++i;
		}
	}
	
	/*
	*	IVANO: I modified the insertion of weirdos in order to take into account
	*	the instructions to read data from coprocessor. However, I actually don't 
	*	know what is the meaning of the world "weirdo", as well as I don't know
	*	the theory behind these optimizations.
	*/
	
	/*
	*	check if the weirdos contains instruction to read data
	*	from coprocessors
	*/
	list<bool>::iterator flag;
	for(flag = isCopro.begin(); flag != isCopro.end(); flag++)
	{
		if(*flag == true)
			break;
	}
	
	if(flag == isCopro.end())
	{		
		/*
		*	The weirdoes do not contain instructions to read data
		*	from coprocessors. Then, insert the weirdos (= constants, for now) 
		*	at the head... but not before the phis.
		*/
		for(i = code.begin(); i != code.end() && ((*i)->isPhi() || !(*i)->isStateChanger()); ++i);	
		code.insert(i, head.begin(), head.end());
	}
	else
	{
		/*
		*	The wiredoes are inserted after the instruction that executes the coprocessor, i.e., 
		*	copro.invoke
		*/
		for(i = code.begin(); i != code.end(); ++i) 
		{
			if(findCopro(*i))
			{
				/*
				*	The insertion is executed in this position + 1
				*/
				i++;
				code.insert(i, head.begin(), head.end());
				break;
			}
		}
		
		assert(i != code.end() && "There is a BUG! How is it possible that the weirdos cannot be inserted?! O_o");
	
	}
	
	// Insert the new STREGs in the proper place
	bool done;
	
	do {
		done = true;
	
		for(i = code.begin(); i != code.end(); ++i) {
			set<RegType> defs((*i)->getAllDefs());
			typename set<RegType>::iterator di;
			bool inserted(false);
		
			for(di = defs.begin(); !inserted && di != defs.end(); ++di) {
				typename map<RegType, list<IRType *> >::iterator mi;
				if((mi = t.find(*di)) != t.end()) {
					inserted = true;
					j = i;
					++j;
					code.insert(j, mi->second.begin(), mi->second.end());
					t.erase(mi);
					break;
				}
			}
			
			if(inserted) {
				done = false;
				break;
			}
		}
	} while(!done);

	return;
}



} /* namespace jit */
#endif
