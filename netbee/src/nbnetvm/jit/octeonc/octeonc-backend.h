/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef OCTEONC_BACKEND_H
#define OCTEONC_BACKEND_H

#include "mirnode.h"
#include "cfg.h"
#include "octeonc-asm.h"
#include "genericbackend.h"
#include "copy_folding.h"
#include "tracebuilder.h"
#include "jit_internals.h"

jit::TargetDriver* octeonc_getTargetDriver(nvmNetVM*, nvmRuntimeEnvironment*, jit::TargetOptions*);

namespace jit {

	namespace octeonc {

		class octeoncTargetDriver : public TargetDriver
		{
			public:
				octeoncTargetDriver(nvmNetVM* netvm, 
                                    nvmRuntimeEnvironment* RTObj, 
                                    TargetOptions* options);
				void init(CFG<MIRNode>& cfg);
				int32_t check_options(uint32_t index);

			protected:
				GenericBackend* get_genericBackend(CFG<MIRNode>& cfg);
				DiGraph<nvmNetPE*>* precompile(
                    DiGraph<nvmNetPE*>* pe_graph);

		};

		class octeoncTraceBuilder : 
            public TraceBuilder<jit::CFG<octeonc_Insn> >
		{
			public:
			typedef BasicBlock<octeonc_Insn> bb_t;

			octeoncTraceBuilder(CFG<octeonc_Insn>& cfg);
			void build_trace();

			void handle_no_succ_bb(bb_t* bb) {};
			void handle_one_succ_bb(bb_t* bb) {};
			void handle_two_succ_bb(bb_t* bb) {};

			private:
			void transform_long_jumps() {};
		};

		struct octeoncChecker : public Fold_Copies< CFG<MIRNode> >::CheckCompatible
		{
			typedef Fold_Copies< CFG<MIRNode> >::RegType RegType;
			uint32_t copro_space; //!<coprocessor register space
			octeoncChecker();
			/*!
			 *
			 * returns true nor a neither b are coprocessor register
			 * or if they are the same coprocessor register
			 */
			bool operator()(RegType &a, RegType &b);
		};

		class octeoncBackend : public GenericBackend
		{
			public:

			//!constructor
			octeoncBackend(CFG<MIRNode>& cfg);
			//!destructor
			~octeoncBackend();

			void emitNativeAssembly(std::ostream &str);

			void emitNativeAssembly(std::string filename);

			private:
			CFG<MIRNode>& MLcfg;      //!<source cfg
			octeoncCFG LLcfg;   //!<resulting cfg

			uint8_t* buffer;  //!<where the buffer is located in memory
			octeoncTraceBuilder trace_builder; //!<object with the order of bb emission
		};

	} //namespace octeonc
}//namespace jit

#endif
