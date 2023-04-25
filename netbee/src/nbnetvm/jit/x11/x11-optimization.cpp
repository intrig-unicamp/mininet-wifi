/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x11-optimization.h"

#include "cfg_ssa.h"
#include "cfg_liveness.h"
#include "ssa_graph.h"

#include "x11-util.h"

#include <algorithm>

using namespace std;

namespace jit {
namespace X11 {

Simple_Redundancy_Elimination::Simple_Redundancy_Elimination(CFG &c) : cfg(c)
{
	// Nop
	return;
}

Simple_Redundancy_Elimination::~Simple_Redundancy_Elimination()
{
	// Nop
	return;
}

void Simple_Redundancy_Elimination::walk_definitions(SSA_Graph<CFG> &graph, set<RegType> &definitions)
{
	set<RegType>::iterator j;
	bool stop(false);

	// Go and look for the register used when defining the current value
	do {
		stop = true;
		
		for(j = definitions.begin(); j != definitions.end(); ++j) {
			cout << "Esamino la definizione di " << printReg(*j) << '\n';
			
			pair<BasicBlock *, CFG::IRType *> p(graph.get_def(*j));
				
			if(!p.second) {
				cout << "Non la trovo\n";
				continue;
			}
				
			p.second->printNode(cout, true);
			cout << '\n';
								
			// Track only assignements and phis, other instructions are not interesting for now.
			list<pair<RegType, RegType> > copies(p.second->getCopiedPair(true));

			if(copies.size()) {
//			if(p.second->isCopy(true) || p.second->isPhi()) {
				cout << "Assegnazione tramite copia / phi\n";
				stop = false;
//				set<RegType> uses(p.second->getSources()), t;
				set<RegType> uses, t;
					
				cout << "Sorgenti dell'assegnazione: ";
//				set<RegType>::iterator w;
				list<pair<RegType, RegType> >::iterator w;
				for(w = copies.begin(); w != copies.end(); ++w) {
					// Only take into account the currently examined defined register
					if(w->first == *j) {
						uses.insert(w->second);
						cout << w->second << ' ';
					}
				}
				cout << '\n';

				definitions.erase(j);
				set_union(uses.begin(), uses.end(), definitions.begin(), definitions.end(),
					insert_iterator<set<RegType> >(t, t.begin()));
				definitions = t;
					
				break;
			}
		}
	} while(!stop);
}

void Simple_Redundancy_Elimination::run()
{
	// Set the IR to the overwrite analysis mode
//	X11IRNode::enable_overwrite_analysis_mode(true);

	list<list<CFG::IRType *>::iterator> it_work_list;
	list<OverwriteAnalysisWrap *> work_list;

	list<BasicBlock* >* bbList(cfg.getBBList());
	
	RegisterInstance po(REG_SPACE(sp_offset), off_pkt, 0);
	RegisterInstance ro(REG_SPACE(sp_offset), off_reg, 0);

	{
		list<BasicBlock *>::iterator i;
		for(i = bbList->begin(); i != bbList->end(); ++i) {
			list<CFG::IRType *> &codeList((*i)->getCode());
			list<CFG::IRType *>::iterator j;
			for(j = codeList.begin(); j != codeList.end(); ++j) {
				set<CFG::IRType::RegType> defs((*j)->getDefs());
			
				// Wrap the load nodes (and every node we want to process)
				if(defs.find(po) != defs.end()) {
					OverwriteAnalysisWrap *oaw(new OverwriteAnalysisWrap((*i)->getId(), *j));
					*j = oaw;
					work_list.push_back(oaw);
					it_work_list.push_back(j);
				}

				if(defs.find(ro) != defs.end()) {
					OverwriteAnalysisWrap *oaw(new OverwriteAnalysisWrap((*i)->getId(), *j));
					*j = oaw;
					work_list.push_back(oaw);
					it_work_list.push_back(j);
				}
			}
		}
	}
	
	
	// Compute liveness information
	//cfg_computeLiveSet<X11IRNode>(cfg);

	// Put the CFG in SSA form.
	SSA<CFG> ssa(cfg);
	ssa.run();
	
	// Build the SSA graph
	SSA_Graph<CFG> graph(cfg);
	
	// Rewrite table
	map<RegType, RegType> rewrite;
	
	
	// For every node in the work list walk the definitions and accumulate them in a set
	list<OverwriteAnalysisWrap *>::iterator i;
	for(i = work_list.begin(); i != work_list.end(); ++i) {
		set<RegType> definitions((*i)->getReachingDefs());
		assert(definitions.size() == 1 && "Nodes with multiple definitions not supported (for now)");
		
		/* FIXME: devo considerare seriamente l'evenienza che il tal registro sia definito
		 * in termini di se stesso (ovvero vedo prima l'uso e poi la definizione...)
		 */
		/* Per ora questo è un problema _solo_ con po.0, perché gli altri sono sicuramente
		 * successivi ad una definizione di po stesso - po.0 viene ignorato di default
		 */
		if(!(*(definitions.begin())).version()) {
			cout << "Prima definizione, la ignoro\n";
			continue;
		}
		
		walk_definitions(graph, definitions);
		
		if(definitions.size() == 1) {
			cout << "Definizione univoca per " << '\n';
			(*i)->printNode(cout, true);
			cout << '\n';
			cout << ": " << printReg(*(definitions.begin())) << '\n';
			
			// Check if we are redefining the register with the same value
			set<RegType> actual_defs((*i)->getDefs());
			
			walk_definitions(graph, actual_defs);
			
			if(actual_defs == definitions) {
				cout << "NODO RIDEFINITO TROVATO: rinominare da " << *((*i)->getDefs().begin()) <<
					" a " << *((*i)->getReachingDefs().begin()) << '\n';

				rewrite[*((*i)->getDefs().begin())] = *((*i)->getReachingDefs().begin());
#if 0					
				// FIXME: for now, just mark the node to be eliminated
				(*i)->setProperty<bool>("kill_me", true);
#endif
			} else {
				cout << "Nodo NON RIDEFINITO: originale:\n";
				set<RegType>::iterator z;
				for(z = actual_defs.begin(); z != actual_defs.end(); ++z) {
					cout << printReg(*z) << '\n';
				}
			}
		} else {
			cout << "Definizione NON univoca, elenco:\n";
			set<RegType>::iterator i;
			
			for(i = definitions.begin(); i != definitions.end(); ++i)
				cout << *i << ' ';
			cout << '\n';
		}
	}

#if 1
	// Eliminate helper nodes
	list<list<CFG::IRType *>::iterator>::iterator it;
	
	for(i = work_list.begin(), it = it_work_list.begin();
		it != it_work_list.end()  && i != work_list.end(); ++i, ++it) {
#if 0
		if((*i)->getProperty<bool>("kill_me")) {
			cout << "CANCELLO un nodo dal basicblock\n";
		
			// Delete from the list.
			BasicBlock *b(cfg.getBBById((*i)->belongsTo()));
			b->getCode().erase(*it);
			
			// Dispose of the instruction
			delete (*i)->getReal();
			delete *i;
		} else {
#endif
			CFG::IRType *real((*i)->getReal());
			delete **it;
			**it = real;
#if 0
		}
#endif
	}
#endif

	// Print rewriting table
	{
		map<RegType, RegType>::iterator i;
		cout << "Rewrite rules:\n";
		for(i = rewrite.begin(); i != rewrite.end(); ++i)
			cout << printReg(i->first) << " -> " << printReg(i->second) << '\n';
	}

	// Rename uses
	{
		list<BasicBlock *>::iterator i;
		for(i = bbList->begin(); i != bbList->end(); ++i) {
			list<CFG::IRType *> &codeList((*i)->getCode());
			list<CFG::IRType *>::iterator j;
			for(j = codeList.begin(); j != codeList.end(); ++j) {
				set<CFG::IRType::RegType> uses((*j)->getUses());
				set<CFG::IRType::RegType>::iterator w;
				map<CFG::IRType::RegType, CFG::IRType::RegType>::iterator q;
				
				for(w = uses.begin(); w != uses.end(); ++w)
					if((q = rewrite.find(*w)) != rewrite.end()) {
						cout << "Rinomino " << printReg(*w) << " in " << printReg(q->second) << '\n';
						(*j)->rewrite_use(*w, q->second);
					}
			}
		}
	}
	
	// Print the resulting IR
	{
		std::cout << "After exit from Simple Redundacy Elimination in optimizer (before dead code): "
			<< cfg.getName() << std::endl;
		jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(cfg);
		std::cout << codeprinter;
	}

	// Dead code elimination pass
	Dead_Code_Elimination dce(cfg);
	dce.run(false);

#if 1
	// Undo SSA form
	UndoSSA<CFG> undo_ssa(cfg);
	undo_ssa.run();
#endif

	// Print the resulting IR
	{
		std::cout << "After exit from Simple Redundacy Elimination in optimizer: "
			<< cfg.getName() << std::endl;
		jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(cfg);
		std::cout << codeprinter;
	}

	delete bbList;

	return;
}


// --------------------------------------------------------------------------------------------------

Dead_Code_Elimination::Dead_Code_Elimination(CFG &c) : dead_code(c), cfg(c)
{
	// Nop
	return;
}

Dead_Code_Elimination::~Dead_Code_Elimination()
{
	// Nop
	return;
}
	
void Dead_Code_Elimination::run(bool go_to_ssa)
{
	if(go_to_ssa) {
//		// Compute liveness information
//		jit::Liveness<_IR> liveness(cfg);
//		liveness.run();

		// Put the CFG in SSA form.
		SSA<CFG> ssa(cfg);
		ssa.run();
	}
	
	// TODO: run dead code elimination
	bool changed(true);
	do {
		changed = false;
		dead_code.start(changed);
	} while(changed);
	
	if(go_to_ssa) {
		// Undo SSA form (we need copies, not phis).
		UndoSSA<CFG> undo_ssa(cfg);
		undo_ssa.run();
	}

	return;
}

// --------------------------------------------------------------------------------------------------

Zero_Versions::Zero_Versions(CFG &c) : cfg(c)
{
	// Nop
	return;
}

Zero_Versions::~Zero_Versions()
{
	// Nop
	return;
}
	
void Zero_Versions::run()
{
	list<BasicBlock* >* bbList(cfg.getBBList());

	{
		list<BasicBlock *>::iterator i;
		for(i = bbList->begin(); i != bbList->end(); ++i) {
			list<CFG::IRType *> &codeList((*i)->getCode());
			list<CFG::IRType *>::iterator j;
			for(j = codeList.begin(); j != codeList.end(); ++j) {
				CFG::IRType::IRNodeIterator k((*j)->nodeBegin());
				for(; k != (*j)->nodeEnd(); ++k) {
					set<CFG::IRType::RegType> uses((*j)->getUses());
					set<CFG::IRType::RegType>::iterator w;
					
					for(w = uses.begin(); w != uses.end(); ++w) {
						CFG::IRType::RegType zero(*w);
						zero.version() = 0;
						(*k)->rewrite_use(*w, zero);
					}
					
					if((*k)->getOwnReg()) {
						CFG::IRType::RegType zero(*((*k)->getOwnReg()));
						zero.version() = 0;
						(*k)->setDefReg(zero);
					}
				}
			}
		}
	}

	delete bbList;
	return;
}

// --------------------------------------------------------------------------------------------------
	
Merge::Merge(CFG &c) : cfg(c)
{
	// Nop
	return;
}

Merge::~Merge()
{
	// Nop
	return;
}
	
void Merge::run()
{
	// Get a list of every BB
	list<BasicBlock* >* bbList(cfg.getBBList());

	list<BasicBlock *>::iterator i;
	for(i = bbList->begin(); i != bbList->end(); ++i) {
		list<CFG::IRType *> &codeList((*i)->getCode());
		list<CFG::IRType *>::iterator j;
		
		bool is_first(true);
		set<CFG::IRType::unit_t> munits;
		set<CFG::IRType::RegType> muses, mdefs;
		bool reset(false);
		
		for(j = codeList.begin(); j != codeList.end(); ++j) {
			CFG::IRType::IRNodeIterator k((*j)->nodeBegin());

			for(; k != (*j)->nodeEnd(); ++k) {			
				set<CFG::IRType::RegType> uses((*j)->getUses());
				set<CFG::IRType::RegType> defs((*j)->getDefs());
				set<CFG::IRType::unit_t> units((*j)->get_units());

				// Skip the first one, as it can't be merged with anything
				if(is_first) {
					is_first = false;
					muses = uses;
					mdefs = defs;
					munits = units;
					continue;
				}
				
				{
					// First of all, check if it is unit-compatible with the current one
					set<CFG::IRType::unit_t> unit_i;
				
					set_intersection(units.begin(), units.end(), munits.begin(), munits.end(),
						insert_iterator<set<CFG::IRType::unit_t> >(unit_i, unit_i.begin()));

					if(unit_i.size())
						reset = true;
					
					unit_i = munits;
					munits.clear();
					set_union(unit_i.begin(), unit_i.end(), units.begin(), units.end(),
						insert_iterator<set<CFG::IRType::unit_t> >(munits, munits.begin()));
				}
				
				if(!reset) {
					set<CFG::IRType::RegType> reg_i;
					
					set_intersection(mdefs.begin(), mdefs.end(), uses.begin(), uses.end(),
						insert_iterator<set<CFG::IRType::RegType> >(reg_i, reg_i.begin()));
						
					if(reg_i.size())
						reset = true;
				}
				
				if(!reset) {
					set<CFG::IRType::RegType> reg_i;	
				
					// Merge the uses and definitions sets
					reg_i = mdefs;
					mdefs.clear();
					set_union(reg_i.begin(), reg_i.end(), defs.begin(), defs.end(),
						insert_iterator<set<CFG::IRType::RegType> >(mdefs, mdefs.begin()));
						
					reg_i = muses;
					muses.clear();
					set_union(reg_i.begin(), reg_i.end(), uses.begin(), uses.end(),
						insert_iterator<set<CFG::IRType::RegType> >(muses, muses.begin()));
				}
				
				if(reset) {
					// Set this instruction as the leader
					munits = units;
					muses = uses;
					mdefs = defs;
					reset = false;
				} else {
					(*j)->set_merge(true);
				}
			}
		}
	}

	delete bbList;
	return;
}


} /* namespace X11 */
} /* namespace jit */
