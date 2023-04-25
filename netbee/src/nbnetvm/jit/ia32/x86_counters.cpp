/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "x86_counters.h"
#include "inssel-ia32.h"

using namespace jit;
using namespace ia32;



void counter_access_mem_profiling( BasicBlock<x86Instruction> & BB, x86_dim_man::dim_mem_type type_mem)
{
#ifdef COUNTERS_PROFILING

	x86Instruction* counters_check_insn;

	switch(type_mem)
	{
		case x86_dim_man::packet :	

			counters_check_insn = x86_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X86_ADD, 1 ,(uint32_t) &(NetVMInstructionsProfiler.n_pkt_access) ,  x86_DWORD);
			x86_Asm_Append_Comment(counters_check_insn, "PROFILING JIT PKT_ACCESS");
			break;

		case x86_dim_man::info :

			counters_check_insn = x86_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X86_ADD, 1 ,   (uint32_t) &(NetVMInstructionsProfiler.n_info_access) ,  x86_DWORD); \
			x86_Asm_Append_Comment(counters_check_insn, "PROFILING JIT INFO_ACCESS");


			break;
		case x86_dim_man::data :

			counters_check_insn = x86_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X86_ADD, 1 ,   (uint32_t) &(NetVMInstructionsProfiler.n_data_access) ,  x86_DWORD); \
			x86_Asm_Append_Comment(counters_check_insn, "PROFILING JIT DATA_ACCESS");

			break;

		default:
			break;
	}

#endif

}

void counter_check_profiling( BasicBlock<x86Instruction> & BB, x86_dim_man::dim_mem_type type_mem)
{
#ifdef COUNTERS_PROFILING

	x86Instruction* counters_check_insn;

	switch(type_mem)
	{
		case x86_dim_man::packet :	

			counters_check_insn = x86_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X86_ADD, 1 ,(uint32_t) &(NetVMInstructionsProfiler.n_pkt_check) ,  x86_DWORD);
			x86_Asm_Append_Comment(counters_check_insn, "PROFILING JIT PKT_CHECK");
			break;

		case x86_dim_man::info :

			counters_check_insn = x86_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X86_ADD, 1 ,   (uint32_t) &(NetVMInstructionsProfiler.n_info_check) ,  x86_DWORD); \
			x86_Asm_Append_Comment(counters_check_insn, "PROFILING JIT INFO_CHECK");


			break;
		case x86_dim_man::data :

			counters_check_insn = x86_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X86_ADD, 1 ,   (uint32_t) &(NetVMInstructionsProfiler.n_data_check) ,  x86_DWORD); \
			x86_Asm_Append_Comment(counters_check_insn, "PROFILING JIT DATA_CHECK");

			break;

		default:
			break;
	}

#endif

}


