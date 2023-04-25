/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


/*!
	\brief This structure holds information for the profiling
*/

#ifdef __cplusplus
extern "C" {
#endif


#define CODEXEC_SAMPLES 10000
#define CODEXEC_RUNS_PER_SAMPLE 100
#define CODEXEC_TRANSIENT 50		/* Number of samples at the beginning of the measurement that will be discarded */
#define CODEXEC_DISCARD_SMALLEST 50	/* After ordering the list, smallest samples will be discarded */
#define CODEXEC_DISCARD_BIGGEST 50		/* After ordering the list, biggest samples will be discarded */


typedef struct _nvmCounter
{
	uint32_t	NumPkts;
	uint32_t	NumFwdPkts;
	uint64_t	TicksStart;
	uint64_t	TicksEnd;
	uint64_t	TicksDelta;
	uint64_t	NumTicks;
}nvmCounter;


struct _NetVMInstructionsProfiler
{
	uint32_t n_pkt_access;
	uint32_t n_info_access;
	uint32_t n_data_access;
	uint32_t n_copro_access;
	uint32_t n_stack_access;
	uint32_t n_initedmem_access;
	uint32_t n_jump;
	uint32_t n_instruction;
	uint32_t n_pkt_check;
	uint32_t n_info_check;
	uint32_t n_data_check;
};

extern struct _NetVMInstructionsProfiler NetVMInstructionsProfiler;

extern int32_t useJIT_flag;



#ifdef ENABLE_PROFILING
	void AllocateProfilerExecTime();
	void ProfilerStoreSample(uint64_t Start, uint64_t End);
	void ProfilerProcessData();
	void ProfilerPrintSummary();
	void ProfilerExecTimeCleanup();
#endif


#ifdef __cplusplus
}
#endif

