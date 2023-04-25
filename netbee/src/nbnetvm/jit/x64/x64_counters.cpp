/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "x64_counters.h"
#include "inssel-x64.h"

using namespace jit;
using namespace x64;



void counter_access_mem_profiling( BasicBlock<x64Instruction> & BB, x64_dim_man::dim_mem_type type_mem)
{
#ifdef COUNTERS_PROFILING

	x64Instruction* counters_check_insn;

	switch(type_mem)
	{
		case x64_dim_man::packet :	

			counters_check_insn = x64_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X64_ADD, 1 ,(uint64_t) &(NetVMInstructionsProfiler.n_pkt_access) ,  x64_DWORD);
			x64_Asm_Append_Comment(counters_check_insn, "PROFILING JIT PKT_ACCESS");
			break;

		case x64_dim_man::info :

			counters_check_insn = x64_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X64_ADD, 1 ,   (uint64_t) &(NetVMInstructionsProfiler.n_info_access) ,  x64_DWORD); \
			x64_Asm_Append_Comment(counters_check_insn, "PROFILING JIT INFO_ACCESS");


			break;
		case x64_dim_man::data :

			counters_check_insn = x64_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X64_ADD, 1 ,   (uint64_t) &(NetVMInstructionsProfiler.n_data_access) ,  x64_DWORD); \
			x64_Asm_Append_Comment(counters_check_insn, "PROFILING JIT DATA_ACCESS");

			break;

		default:
			break;
	}

#endif

}

void counter_check_profiling( BasicBlock<x64Instruction> & BB, x64_dim_man::dim_mem_type type_mem)
{
#ifdef COUNTERS_PROFILING

	x64Instruction* counters_check_insn;

	switch(type_mem)
	{
		case x64_dim_man::packet :	

			counters_check_insn = x64_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X64_ADD, 1 ,(uint64_t) &(NetVMInstructionsProfiler.n_pkt_check) ,  x64_DWORD);
			x64_Asm_Append_Comment(counters_check_insn, "PROFILING JIT PKT_CHECK");
			break;

		case x64_dim_man::info :

			counters_check_insn = x64_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X64_ADD, 1 ,   (uint64_t) &(NetVMInstructionsProfiler.n_info_check) ,  x64_DWORD); \
			x64_Asm_Append_Comment(counters_check_insn, "PROFILING JIT INFO_CHECK");


			break;
		case x64_dim_man::data :

			counters_check_insn = x64_Asm_Op_Imm_To_Mem_Displ(BB.getCode(), X64_ADD, 1 ,   (uint64_t) &(NetVMInstructionsProfiler.n_data_check) ,  x64_DWORD); \
			x64_Asm_Append_Comment(counters_check_insn, "PROFILING JIT DATA_CHECK");

			break;

		default:
			break;
	}

#endif

}


