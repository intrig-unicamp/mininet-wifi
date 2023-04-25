#ifndef PFL_TRACE_BUILDER_H
#define PFL_TRACE_BUILDER_H

#include "tracebuilder.h"
#include "mironode.h"
#include "pflcfg.h"

class PFLTraceBuilder: public jit::TraceBuilder<PFLCFG>
{
	public:
		typedef PFLCFG::BBType bb_t;

		PFLTraceBuilder(PFLCFG& cfg);

		void handle_no_succ_bb(bb_t *bb);
		void handle_one_succ_bb(bb_t *bb);
		void handle_two_succ_bb(bb_t *bb);
};



#endif
