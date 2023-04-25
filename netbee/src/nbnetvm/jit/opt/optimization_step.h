/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file optimization_step.h
 * \brief this file contains the definition of OptimizationStep interface 
 * */
#ifndef OPTIMIZATION_STEP
#define OPTIMIZATION_STEP
#include "cfg.h"
#include "irnode.h"

//#include "../cfg_printer.h"

namespace jit{



		/*!
		 * \brief this class is a generic optimization step. 
		 *
		 * Inheriting classe must provide the pure virtual method start(bool). This is necessary because Optimizer must know if the steps are changing the code.
		 */
	template <typename _CFG>
		class OptimizationStep
		{
			public:
				typedef _CFG CFG;
				typedef typename CFG::IRType IRType;
				typedef typename IRType::RegType RegType;
				typedef typename CFG::BBType BBType;

				OptimizationStep(CFG &cfg): _cfg(cfg) { };	
				virtual ~OptimizationStep() {};
				/*!
				 * \brief method to start the optimization step 
				 * \param reference to a bool value. If the step has modified the code structure that reference must be set to true
				 */
				virtual void start(bool &code_changed) = 0;
			protected:
				CFG &_cfg;
		};

		/*!
		 * \brief this function calls operator() of the provided function on every statement of the given cfg
		 * \param the cfg
		 * \param object to call on every statement
		 */
		template<class function, class _CFG>
			void visitCfgNodes(_CFG &cfg, function *fct)
			{
				typedef _CFG CFG;
				typedef typename CFG::BBType BBType;
				typedef typename CFG::IRType IR;

				std::list<BBType* > *BBlist;
				typedef typename std::list<BBType* >::iterator _BBIt;
				typedef typename BBType::IRStmtNodeIterator stmIt;
				BBlist = cfg.getBBList();		
				for(_BBIt i = BBlist->begin(); i != BBlist->end(); i++)
				{
					for(stmIt stmIter = (*i)->codeBegin(); stmIter != (*i)->codeEnd(); stmIter++)
					{
						IR *node;
						node = *stmIter;
						(*fct)(node);
					}
				}
				delete BBlist;
				return;
			};
			
		template<class function, class _CFG>
			void visitCfgNodesPosition(_CFG &cfg, function *fct)
			{
				typedef _CFG CFG;
				typedef typename CFG::BBType BBType;
				typedef typename CFG::IRType IR;

				std::list<BBType* > *BBlist;
				typedef typename std::list<BBType* >::iterator _BBIt;
				typedef typename BBType::IRStmtNodeIterator stmIt;
				BBlist = cfg.getBBList();		
				for(_BBIt i = BBlist->begin(); i != BBlist->end(); i++)
				{
					for(stmIt stmIter = (*i)->codeBegin(); stmIter != (*i)->codeEnd(); stmIter++)
					{
						IR *node;
						node = *stmIter;
						
//						std::cout << "Esamino il nodo ";
//						node->printNode(std::cout, true);
//						std::cout << '\n';
						
						(*fct)(node, stmIter, (*i)->getCode());
						
						// Print the current BB
//						std::cout << jit::BBPrinter<MIRNode, jit::SSAPrinter<MIRNode> >(**i) << '\n';
					}
				}
				delete BBlist;
				return;
			};
			
}/* jit */
#endif
