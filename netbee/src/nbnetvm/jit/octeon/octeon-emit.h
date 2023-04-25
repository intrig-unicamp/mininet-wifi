/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _OCTEON_EMIT_H
#define _OCTEON_EMIT_H

#include "cfg.h"
#include "octeon-asm.h"
#include "gc_regalloc.h"
#include "tracebuilder.h"

namespace jit
{
	namespace octeon
	{
		//!class used to emit octeon code to file
		class octeonEmitter
		{
			public:
				//!constructor
				octeonEmitter(octeonCFG& cfg, GCRegAlloc<CFG<octeon_Insn> >& regAlloc, TraceBuilder<CFG<octeon_Insn> >& trace_builder);
				//!main function
				void emit(std::string filename);
				void emit(std::ostream &os);
				static const uint32_t stack_space;

			private:

				void emit_bb(std::ostream& os, BasicBlock<octeon_Insn>* bb);
				void emit_epilogue(std::ostream& os);
				void emit_prologue(std::ostream& os);
				void emit_jump_tables(std::ostream& os);
				const std::string& get_function_name();
				uint32_t get_stack_space();

				octeonCFG& cfg; //!<cfg to emit
				GCRegAlloc<CFG<octeon_Insn> >& regAlloc;//!<register allocation information
				TraceBuilder<CFG<octeon_Insn> >& trace_builder;
				std::string function_name;
		};
	} //namespace octeon
} //namespace jit

inline const std::string& jit::octeon::octeonEmitter::get_function_name()
{
	return function_name;
}

#endif
