/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _OCTEON_EMIT_H
#define _OCTEON_EMIT_H

#include "cfg.h"
#include "octeonc-asm.h"
#include "tracebuilder.h"

namespace jit
{
	namespace octeonc
	{
		//!class used to emit octeonc code to file
		class octeoncEmitter
		{
			public:
				//!constructor
				octeoncEmitter(
                    octeoncCFG& cfg, 
                    TraceBuilder<CFG<octeonc_Insn> >& trace_builder);
				//!main function
				void emit(std::string filename);
				void emit(std::ostream &os);

			private:

				void emit_bb(
                    std::ostream& os, BasicBlock<octeonc_Insn>* bb);
				void emit_epilogue(std::ostream& os);
				void emit_prologue(std::ostream& os);
				const std::string& get_function_name();

				octeoncCFG& cfg; //!<cfg to emit
				TraceBuilder<CFG<octeonc_Insn> >& trace_builder;
				std::string function_name;
		};
	} //namespace octeonc
} //namespace jit

inline const std::string& 
jit::octeonc::octeoncEmitter::get_function_name()
{
	return function_name;
}

#endif
