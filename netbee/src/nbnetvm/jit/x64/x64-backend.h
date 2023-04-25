/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef X64_BACKEND_H
#define X64_BACKEND_H

#include "nbnetvm.h"
#include "mirnode.h"
#include "basicblock.h"
#include "cfg.h"
#include "tracebuilder.h"
#include "genericbackend.h"
#include "copy_folding.h"
#include "x64-asm.h"
#include "jit_internals.h"

jit::TargetDriver* x64_getTargetDriver(nvmNetVM*, nvmRuntimeEnvironment*, jit::TargetOptions*);

namespace jit {
	namespace x64 {

		class x64TargetDriver : public TargetDriver
		{
			public:
				x64TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options);
				void init(CFG<MIRNode>& cfg);

			protected:
				GenericBackend* get_genericBackend(CFG<MIRNode>& cfg);
		};

		class x64TraceBuilder : public TraceBuilder<jit::CFG<x64Instruction> >
		{
			public:
				typedef BasicBlock<x64Instruction> bb_t;

				x64TraceBuilder(CFG<x64Instruction>& cfg);

				void handle_no_succ_bb(bb_t *bb);
				void handle_one_succ_bb(bb_t *bb);
				void handle_two_succ_bb(bb_t *bb);
		};

		//! this class is the interface exported by the x64 backend
		class x64Backend : public GenericBackend
		{
			public:
				//!constructor
				x64Backend(CFG<MIRNode>& cfg);
				uint8_t *emitNativeFunction();
				void emitNativeAssembly(std::string filename);
				void emitNativeAssembly(std::ostream &str);
				//!destructor
				~x64Backend();

			private:

				//!make the instruction selection and register allocation
				bool create_code();

				CFG<MIRNode>& MLcfg; //!<source CFG
				CFG<x64Instruction> LLcfg; //!<the new CFG with x64 instruction
				bool code_created; //!<has the code already been created?
				uint8_t* buffer;  //!<where the buffer is located in memory
				uint32_t actual_buff_sz; //!<size of the binary function in bytes
				x64TraceBuilder trace_builder; //!<object with the order of bb emission
		};

		//!rules to fold copy of registers for x64 backend
		struct x64Checker : public Fold_Copies< CFG<MIRNode> >::CheckCompatible
		{
			typedef Fold_Copies< CFG<MIRNode> >::RegType RegType;
			uint32_t copro_space; //!<coprocessor register space
			x64Checker();
			/*!
			 *
			 * returns true nor a neither b are coprocessor register
			 * or if they are the same coprocessor register
			 */
			bool operator()(RegType &a, RegType &b);
		};
	}//namespace x64
}//namespace jit

#endif
