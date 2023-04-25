/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <nbnetvm.h>
#include "netvmprofiling.h"
#include "../nbee/globals/debug.h"
#include "../nbee/globals/profiling.h"

#ifdef COUNTERS_PROFILING
#include <string.h>
#endif
nbProfiler *NetVMProfiler;
struct _NetVMInstructionsProfiler NetVMInstructionsProfiler= {0,0,0,0,0,0,0,0,0,0,0};


void AllocateProfilerExecTime()
{
int RetVal;

	NetVMProfiler= new CProfilingExecTime();

	if (NetVMProfiler == NULL)
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error allocating the profiler class.\n");

	RetVal= NetVMProfiler->Initialize(CODEXEC_SAMPLES,
								CODEXEC_RUNS_PER_SAMPLE,
								CODEXEC_TRANSIENT,
								CODEXEC_DISCARD_SMALLEST,
								CODEXEC_DISCARD_BIGGEST);

	if (RetVal == nvmFAILURE)
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error allocating the profiler class: %s.\n", NetVMProfiler->GetLastError());
}


void ProfilerStoreSample(uint64_t Start, uint64_t End)
{
int RetVal;

	RetVal= NetVMProfiler->StoreSample(Start, End);

	if (RetVal == nvmFAILURE)
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error storing data in the profiler class: %s.\n", NetVMProfiler->GetLastError());
}


void ProfilerProcessData()
{	
	int RetVal;

	RetVal= NetVMProfiler->ProcessProfilerData();

	if (RetVal == nvmFAILURE)
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error processing data in the profiler class: %s.\n", NetVMProfiler->GetLastError());
}


void ProfilerPrintSummary()
{
int RetVal;
char ResultBuffer[16000];

	RetVal= NetVMProfiler->FormatResults(ResultBuffer, sizeof(ResultBuffer), false);

	if (RetVal == nvmFAILURE)
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Error formatting data in the profiler class: %s.\n", NetVMProfiler->GetLastError());

	printf("%s", ResultBuffer);

#ifdef COUNTERS_PROFILING
//	printf("Num:\n\n");
	if (!useJIT_flag)
	{
		printf("\tNumber of interpreted instructions executed in the current program:\t %d\n\n", NetVMInstructionsProfiler.n_instruction / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
	}
	printf("\tPKT memory access:\t %d\n\n", NetVMInstructionsProfiler.n_pkt_access / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
	printf("\tINFO memory access:\t %d\n\n", NetVMInstructionsProfiler.n_info_access / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
	printf("\tDATA memory access:\t %d\n\n", NetVMInstructionsProfiler.n_data_access / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
	printf("\tCOPRO memory access:\t %d\n\n", NetVMInstructionsProfiler.n_copro_access / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
	printf("\tINITEDMEM check:\t %d\n\n", NetVMInstructionsProfiler.n_initedmem_access / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
	if (useJIT_flag)
	{
		printf("\tPKT check:\t %d\n\n", NetVMInstructionsProfiler.n_pkt_check / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
		printf("\tINFO check:\t %d\n\n", NetVMInstructionsProfiler.n_info_check / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
		printf("\tDATA check:\t %d\n\n", NetVMInstructionsProfiler.n_data_check / (CODEXEC_RUNS_PER_SAMPLE * CODEXEC_SAMPLES));
	}

#ifdef STACK_CHECK
	printf("\tSTACK check:\t\t %d\n\n", NetVMInstructionsProfiler.n_stack_access / (NumRunsPerSample * NumSamples));
#endif

#ifdef JUMP_CHECK
	printf("\tJUMP check:\t\t %d\n\n", NetVMInstructionsProfiler.n_jump / (NumRunsPerSample * NumSamples));
#endif


	// Set again all the values to zero
	memset(&NetVMInstructionsProfiler, 0, sizeof(_NetVMInstructionsProfiler));

#endif
}


void ProfilerExecTimeCleanup()
{
	if (NetVMProfiler)
		delete NetVMProfiler;
}
