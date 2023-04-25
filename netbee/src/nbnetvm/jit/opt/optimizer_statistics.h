/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#ifndef _OPTIMIZER_STATISTICS
#define _OPTIMIZER_STATISTICS
#include "optimizer_statistics.h"
#include "constant_folding.h"
#include "deadcode_elimination_2.h"
#include "constant_propagation.h"
#include "controlflow_simplification.h"
#include "copy_propagation.h"
#include "reassociation.h"
#include <string>
namespace jit{
	namespace opt{
		template <class _CFG>
			class OptimizerStatistics
			{
				public:
					OptimizerStatistics(std::string str): before_print(str) {};
					void printStatistics(std::ostream &);
				private:
					std::string before_print;
			};

		template <class _CFG>
			void OptimizerStatistics<_CFG>::printStatistics(std::ostream &os)
			{
				os << before_print << std::endl;

				os << "Constant Folding: " << ConstantFolding<_CFG>::getModifiedNodes() << std::endl;
				os << "Dead Code Elimination: " << DeadcodeElimination<_CFG>::getModifiedNodes() << std::endl;
				os << "Copy Propagation: " << CopyPropagation<_CFG>::getModifiedNodes() << std::endl;
				os << "Constant Propagation: " << ConstantPropagation<_CFG>::getModifiedNodes() << std::endl;
				os << "Algebraic Simplification: " << AlgebraicSimplification<_CFG>::getModifiedNodes() << std::endl;
				os << "Jump to Jump Elimination: " << JumpToJumpElimination<_CFG>::getModifiedNodes() << std::endl;
				os << "Control flow simplification: " << ControlFlowSimplification<_CFG>::getModifiedNodes() << std::endl;
				os << "Basic Block Elimination: " << BasicBlockElimination<_CFG>::getModifiedNodes() << std::endl;
				os << "Empty Basic Block Elimination: " << EmptyBBElimination<_CFG>::getModifiedNodes() << std::endl;
				os << "Reassociation: " << Reassociation<_CFG>::getModifiedNodes() << std::endl;
			};

		template <class _CFG>
			std::ostream& operator<<(std::ostream& os, OptimizerStatistics<_CFG>& optStat)
			{
				optStat.printStatistics(os);	
				return os;
			}

		template <class _CFG>
			class StatementsCounter
			{
				private:
					_CFG &_cfg;

				public:
					StatementsCounter(_CFG &cfg): _cfg(cfg) {}
					uint32_t start();
			};

		template <class _CFG>
			uint32_t StatementsCounter<_CFG>::start()
			{
				typedef typename _CFG::BBType BBType;
				typedef typename _CFG::IRType IRType;
				typedef typename _CFG::SortedIterator s_it;
				typedef typename std::list<IRType*>::iterator c_it;

				uint32_t counter = 0;

				_cfg.SortPreorder(*_cfg.getEntryNode());
				for(s_it i = _cfg.FirstNodeSorted(); i != _cfg.LastNodeSorted(); i++)
				{
					BBType* bb = (*i)->NodeInfo;
					std::list<IRType*> &codelist = bb->getCode();
					for(c_it j = codelist.begin(); j != codelist.end(); j++)
						if( (*j)->isStateChanger() )
							counter++;
				}
				return counter;
			}
	} /* NAMESPACE OPT */
} /* NAMESPACE JIT */

#endif
