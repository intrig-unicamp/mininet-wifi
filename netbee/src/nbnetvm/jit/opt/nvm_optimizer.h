/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file nvm_optimizer.h
 * \brief this file contains the declaration of the class implementing the optimization framework
 */
#ifndef NVM_OPTIMIZER_H
#define NVM_OPTIMIZER_H

#include "constant_folding.h"
#include "deadcode_elimination_2.h"
#include "constant_propagation.h"
#include "controlflow_simplification.h"
#include "copy_propagation.h"
#include "redistribution.h"
#include "cfg_printer.h"
#include "jit_internals.h"
#include "copy_folding.h"
#include "reassociation.h"

namespace jit {
	/*!
	 * \brief This namespace constains classes and methods for code optimization
	 */
namespace opt{

		/*!
		 * \brief this class is the main class of the optimizing framework.
		 *
		 * It only contains a cfg reference and its main method do_it() to drive the optimization process
		 */
		template <typename _CFG>
			class Optimizer: public nvmJitFunctionI
			{
				public:
					typedef _CFG CFG;
					//!Constructs the object with a reference to the ControlFlowGraph
					Optimizer(CFG & cfg): nvmJitFunctionI("Optimizer"),  _cfg(cfg) {};
					~Optimizer(){};
					//!this is the main function called to start optimization process
					bool run();
				private:
					//!reference to the CFG
					CFG &_cfg;
			};


		/*\brief main method of the optimizing framework
		 *
		 * This method executes a loop calling all possibles optimization steps until there are no more changes in the code.
		 * This is done because a single optimization can create conditions to make other optimization work.
		 */
		template <typename _CFG>
			bool Optimizer<_CFG>::run()
			{
				bool changed = true;
				bool reassociation_changed = true;
				int round = 0, ext_round = 0;
				while(reassociation_changed)
				{
					round = 0;
					reassociation_changed = false;
					changed = true;
					while(changed)
					{
						changed = false;

						ConstantFolding<_CFG> cf(_cfg);
						cf.start(changed);

						AlgebraicSimplification<_CFG> as(_cfg);
						as.start(changed);
				
						#ifdef _DEBUG_ALGEBRAIC_SIMPLIFICATION
						{
								stringstream name;
								name << "dbg_algebraic_simplification_" << ext_round << "_" << round << ".txt";
								string str = name.str();
								ofstream of(str.c_str());
								jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
								of << codeprinter;
								of.close();
						}
						#endif

						DeadcodeElimination<CFG> dce(_cfg);
						dce.start(changed);
						
						#ifdef _DEBUG_DEAD_CODE_ELIMINATION
						{
								stringstream name;
								name << "dbg_dead_code_elimination_" << ext_round << "_"  << round << ".txt";
								string str = name.str();
								ofstream of(str.c_str());
								jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
								of << codeprinter;
								of.close();
						}
						#endif

						CopyPropagation<CFG> copyp(_cfg);
						//copyp.start(changed);

						ConstantPropagation<CFG> cp(_cfg);
						cp.start(changed);
						
						#ifdef _DEBUG_CONSTANT_PROPAGATION
						{
								stringstream name;
								name << "dbg_constant_propagation_" << ext_round << "_"  << round << ".txt";
								string str = name.str();
								ofstream of(str.c_str());
								jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
								of << codeprinter;
								of.close();
						}
						#endif

						ControlFlowSimplification<CFG> cfs(_cfg);
						cfs.start(changed);

						#ifdef _DEBUG_CONTROL_FLOW_SIMPLIFICATION
						{
							stringstream name;
							name << "dbg_control_flow_simplification_" << ext_round << "_"  << round << ".txt";
							string str = name.str();
							ofstream of(str.c_str());
							jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
							of << codeprinter;
							of.close();
						}
						#endif


						Redistribution<CFG> red(_cfg);
						red.start(changed);

						#ifdef _DEBUG_REDISTRIBUTION
						{
							stringstream name;
							name << "dbg_redistribution_" << ext_round << "_" << round << ".txt";
							string str = name.str();
							ofstream of(str.c_str());
							jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
							of << codeprinter;
							of.close();
						}
						#endif

						round++;
					}
					
					jit::Reassociation<CFG> reassociation(_cfg);
					reassociation_changed = reassociation.run();
					
					#ifdef _DEBUG_REASS
					{
						stringstream name;
						name << "dbg_reassociation_" << ext_round << ".txt";
						string str = name.str();
						ofstream of(str.c_str());
						jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
						of << codeprinter;
						of.close();
					}
					#endif
				
					ext_round++;
				}
				#ifdef _DEBUG_OPTIMIZER
				{
					stringstream name;
					name << "dbg_optimization.txt";
					string str = name.str();
					ofstream of(str.c_str());
					jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(_cfg);
					of << codeprinter;
					of.close();
				}
				#endif
				return true;
			}

} /* OPT */
} /* JIT */
#endif
