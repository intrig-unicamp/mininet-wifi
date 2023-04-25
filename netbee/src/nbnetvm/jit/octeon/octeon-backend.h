/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef OCTEON_BACKEND_H
#define OCTEON_BACKEND_H

#include "mirnode.h"
#include "cfg.h"
#include "octeon-asm.h"
#include "genericbackend.h"
#include "copy_folding.h"
#include "tracebuilder.h"
#include "jit_internals.h"

jit::TargetDriver* octeon_getTargetDriver(nvmNetVM*, nvmRuntimeEnvironment*, jit::TargetOptions*);

namespace jit {

	namespace octeon {

		class octeonTargetDriver : public TargetDriver
		{
			public:
				octeonTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options);
				void init(CFG<MIRNode>& cfg);
				int32_t check_options(uint32_t index);

			protected:
				GenericBackend* get_genericBackend(CFG<MIRNode>& cfg);
				DiGraph<nvmNetPE*>* precompile(DiGraph<nvmNetPE*>* pe_graph);

		};

		class octeonTraceBuilder : public TraceBuilder<jit::CFG<octeon_Insn> >
		{
			public:
			typedef BasicBlock<octeon_Insn> bb_t;

			octeonTraceBuilder(CFG<octeon_Insn>& cfg);
			void build_trace();

			void handle_no_succ_bb(bb_t* bb);
			void handle_one_succ_bb(bb_t* bb);
			void handle_two_succ_bb(bb_t* bb);

			private:
			void transform_long_jumps();
		};

		struct octeonChecker : public Fold_Copies< CFG<MIRNode> >::CheckCompatible
		{
			typedef Fold_Copies< CFG<MIRNode> >::RegType RegType;
			uint32_t copro_space; //!<coprocessor register space
			octeonChecker();
			/*!
			 *
			 * returns true nor a neither b are coprocessor register
			 * or if they are the same coprocessor register
			 */
			bool operator()(RegType &a, RegType &b);
		};

		class octeonBackend : public GenericBackend
		{
			public:

			//!constructor
			octeonBackend(CFG<MIRNode>& cfg);
			//!destructor
			~octeonBackend();

			void emitNativeAssembly(std::ostream &str);

			void emitNativeAssembly(std::string filename);

			private:
			CFG<MIRNode>& MLcfg;      //!<source cfg
			octeonCFG LLcfg;   //!<resulting cfg

			uint8_t* buffer;  //!<where the buffer is located in memory
			octeonTraceBuilder trace_builder; //!<object with the order of bb emission
		};

	} //namespace octeon
}//namespace jit

#endif
