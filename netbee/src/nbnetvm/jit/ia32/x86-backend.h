/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef X86_BACKEND_H
#define X86_BACKEND_H

#include "nbnetvm.h"
#include "mirnode.h"
#include "basicblock.h"
#include "cfg.h"
#include "tracebuilder.h"
#include "genericbackend.h"
#include "copy_folding.h"
#include "x86-asm.h"
#include "jit_internals.h"

jit::TargetDriver* x86_getTargetDriver(nvmNetVM*, nvmRuntimeEnvironment*, jit::TargetOptions*);

namespace jit {
	namespace ia32 {

		class x86TargetDriver : public TargetDriver
		{
			public:
				x86TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options);
				void init(CFG<MIRNode>& cfg);

			protected:
				GenericBackend* get_genericBackend(CFG<MIRNode>& cfg);
		};

		class x86TraceBuilder : public TraceBuilder<jit::CFG<x86Instruction> >
		{
			public:
				typedef BasicBlock<x86Instruction> bb_t;

				x86TraceBuilder(CFG<x86Instruction>& cfg);

				void handle_no_succ_bb(bb_t *bb);
				void handle_one_succ_bb(bb_t *bb);
				void handle_two_succ_bb(bb_t *bb);
		};

		//! this class is the interface exported by the x86 backend
		class x86Backend : public GenericBackend
		{
			public:
				//!constructor
				x86Backend(CFG<MIRNode>& cfg);
				uint8_t *emitNativeFunction();
				void emitNativeAssembly(std::string filename);
				void emitNativeAssembly(std::ostream &str);
				//!destructor
				~x86Backend();

			private:

				//!make the instruction selection and register allocation
				bool create_code();

				CFG<MIRNode>& MLcfg; //!<source CFG
				CFG<x86Instruction> LLcfg; //!<the new CFG with x86 instruction
				bool code_created; //!<has the code already been created?
				uint8_t* buffer;  //!<where the buffer is located in memory
				uint32_t actual_buff_sz; //!<size of the binary function in bytes
				x86TraceBuilder trace_builder; //!<object with the order of bb emission
		};

		//!rules to fold copy of registers for x86 backend
		struct x86Checker : public Fold_Copies< CFG<MIRNode> >::CheckCompatible
		{
			typedef Fold_Copies< CFG<MIRNode> >::RegType RegType;
			uint32_t copro_space; //!<coprocessor register space
			x86Checker();
			/*!
			 *
			 * returns true nor a neither b are coprocessor register
			 * or if they are the same coprocessor register
			 */
			bool operator()(RegType &a, RegType &b);
		};
	}//namespace ia32
}//namespace jit

#endif
