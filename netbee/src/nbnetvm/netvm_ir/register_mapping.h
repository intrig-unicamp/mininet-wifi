#ifndef __REGISTER_MAPPING_H__
#define __REGISTER_MAPPING_H__

#include "cfg.h"
#include "basicblock.h"
#include "registers.h"
//#include "jit_internals.h"
#include "function_interface.h"

#include <set>
#include <cassert>

namespace jit {

//! class that maps all the registers in a cfg to a new space
template<typename _CFG>
class Register_Mapping: public nvmJitFunctionI {
  public:
	typedef _CFG CFG;
	typedef typename CFG::BBType BBType;
	
  private:
	CFG &cfg; //!<cfg to map
	RegisterManager rm; //!<class that holds mapping information

  public:
	Register_Mapping(CFG &, uint32_t output_space, std::set<uint32_t> ignore);
	~Register_Mapping();
	
	//!<get the manager which holds mapping info
	RegisterManager& get_manager();

	//!map and rename a cfg
	bool run();
	//!only map a cfg
	void define_new_names();
	//!only rename a cfg
	void rename();
	
}; /* class Register_Mapping */

template<typename _CFG>
Register_Mapping<_CFG>::Register_Mapping(typename Register_Mapping<_CFG>::CFG &c, uint32_t os,
	std::set<uint32_t> ign) : nvmJitFunctionI("Register mapping"),  cfg(c), rm(os, ign)
{
	// Nop
	return;
}

template<typename _CFG>
Register_Mapping<_CFG>::~Register_Mapping()
{
	// Nop
	return;
}

template<typename _CFG>
RegisterManager& Register_Mapping<_CFG>::get_manager()
{
	return rm;
}

template<typename _CFG>
void Register_Mapping<_CFG>::define_new_names()
{
	std::list<BBType *> *bblist(cfg.getBBList());
	typename std::list<BBType *>::iterator i;
	
	//iterates over the basic blocks
	for(i = bblist->begin(); i != bblist->end(); ++i) {
		BBType *bb(*i);
		
		#ifdef _DEBUG_OPTIMIZATION_ALGORITHMS	
			cout << "Considering BB: " << *bb << endl;
		#endif
		
		std::list<typename CFG::IRType *> &code_list(bb->getCode());
		typename std::list<typename CFG::IRType *>::iterator j;
		
		#ifdef _DEBUG_OPTIMIZATION_ALGORITHMS	
			cout << "\tThis BB has: " << code_list.size() << " instructions" << endl;
		#endif
	
		//iterates over the instructions
		for(j = code_list.begin(); j != code_list.end(); ++j) {
			typename CFG::IRType *ir(*j);
			
			typename CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
			for(; k != ir->nodeEnd(); k++) 
			{
				#ifdef _DEBUG_OPTIMIZATION_ALGORITHMS	
					cout << "\t\tnode" << endl;
				#endif
			
				typename CFG::IRType *n(*k);

				// Scan all the uses
				std::set<typename CFG::IRType::RegType> uses(n->getUses());
				typename std::set<typename CFG::IRType::RegType>::iterator k;
				for(k = uses.begin(); k != uses.end(); ++k) {
					RegisterInstance t(*k);
					rm.addNewRegister(RegisterInstance(*k));
					#ifdef _DEBUG_OPTIMIZATION_ALGORITHMS	
						cout << "\t\t\tRegister: " << rm.getName(RegisterInstance(*k)) << " is used" << endl;
					#endif
				}
				
				std::set<typename CFG::IRType::RegType> defs(n->getDefs());
				for(k = defs.begin(); k != defs.end(); ++k) {
					RegisterInstance t(*k);
					rm.addNewRegister(RegisterInstance(*k));
					#ifdef _DEBUG_OPTIMIZATION_ALGORITHMS	
						cout << "\t\t\tRegister: " << rm.getName(RegisterInstance(*k)) << " is defined" << endl;
					#endif
				}
				
			}
		}
	}

	delete bblist;
}

//!\todo problem with setOwnReg setDefReg rewrite_use!!!
template<typename _CFG>
void Register_Mapping<_CFG>::rename()
{
	std::list<BBType *> *bblist(cfg.getBBList());
	typename std::list<BBType *>::iterator i;

	for(i = bblist->begin(); i != bblist->end(); ++i) {
		BBType *bb(*i);
		
		std::list<typename CFG::IRType *> &code_list(bb->getCode());
		typename std::list<typename CFG::IRType *>::iterator j;
		
		for(j = code_list.begin(); j != code_list.end(); ++j) {
			typename CFG::IRType *ir(*j);
			
			typename CFG::IRType::IRNodeIterator k(ir->nodeBegin());
			
			for(; k != ir->nodeEnd(); k++) {
				typename CFG::IRType *n(*k);
				
				// XXX: here we encounter the same problem we found in the SSA rewriting code.
				
				// Rewrite all the uses
				std::set<typename CFG::IRType::RegType> uses(n->getUses());
				typename std::set<typename CFG::IRType::RegType>::iterator k;
				for(k = uses.begin(); k != uses.end(); ++k) {
					RegisterInstance t(*k);
					n->rewrite_use(t, rm.getName(RegisterInstance(*k)));
				}
				
				// Rewrite the definition				
				if(n->getOwnReg() && n->getDefs().size() == 1 /* n->get_defined_count() */) {
//					assert(n->get_defined_count() == 1 && "Can't handle ops that define more than a register for now!");
					n->setDefReg(rm.getName(*(n->getOwnReg())));
				}
			}
		}
	}

	delete bblist;
}

template<typename _CFG>
bool Register_Mapping<_CFG>::run()
{
	// 2 passes: the first one scans and defines the new names, the second one renames.
	
#ifdef _DEBUG_OPTIMIZATION_ALGORITHMS	
	cout << "Running 'Register Mapping'" << endl;
#endif

	define_new_names();
	rename();
	
#ifdef _DEBUG_OPTIMIZATION_ALGORITHMS	
	cout << "Ending 'Register Mapping'" << endl;
#endif
	
	
	return true;
}

} /* namespace jit */

#endif /* __REGISTER_MAPPING_H__ */


