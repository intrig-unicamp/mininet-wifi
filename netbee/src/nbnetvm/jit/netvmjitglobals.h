/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#define nvmJitSUCCESS ((int32_t) 0)
#define nvmJitFAILURE ((int32_t) -1)

#define JIT_MAIN_LVL			11
#define JIT_MAIN_PASS_LVL		12
#define	JIT_PASS_LVL			13
#define JIT_BUILD_BLOCK_LVL		14
#define JIT_BUILD_BLOCK_LVL2	15
#define JIT_BUILD_BLOCK_LVL3	16
#define JIT_MEM_ALLOC_LVL		18
#define JIT_INSTR_LVL			20
#define JIT_INSTR_LVL2			22
#define JIT_DATA_STRUCTS_LVL	24
#define JIT_BITVECT_DEBUG_LVL	24
#define JIT_LISTS_DEBUG_LVL		24
#define JIT_TREES_DEBUG_LVL		24

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define FALSE 0
#define TRUE  1
#endif

#ifdef TIMEPERF
//!Macros used to get the performance counter (only if TIMEPERF is defined)
#define QUERY_PERF_COUNT_1	QueryPerformanceCounter(&lpPerformanceCount1)
#define QUERY_PERF_COUNT_2	QueryPerformanceCounter(&lpPerformanceCount2)
#define DELTA_PERF_COUNT (perfCount += lpPerformanceCount2.QuadPart - lpPerformanceCount1.QuadPart)
#else
#define QUERY_PERF_COUNT_1
#define QUERY_PERF_COUNT_2
#define DELTA_PERF_COUNT
#endif



#ifdef MEM_TRACK
#define MALLOC(SZ) my_malloc(SZ)
#define CALLOC(N, SZ) my_calloc(N, SZ)
uint32_t	totMem;
#else
#include <stdlib.h>
#define MALLOC(SZ) malloc(SZ)
#define CALLOC(N, SZ) calloc(N, SZ)
#endif

#ifdef MEM_TRACK
void *my_malloc(size_t sz);
void *my_calloc(size_t n, size_t sz);
#endif

/* Unix stuff */
#ifndef _WIN32
#define _snprintf snprintf
#else
#define snprintf _snprintf
#endif


#ifdef __cplusplus
}
#endif

