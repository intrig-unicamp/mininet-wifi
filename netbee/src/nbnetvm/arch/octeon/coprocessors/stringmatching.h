#include "nbnetvm.h"
#ifndef _OCTEON_STRING_MATCH
#define _OCTEON_STRING_MATCH


typedef struct _octeonPatternInfo
{
	uint32_t length;
	uint32_t nocase;
	uint32_t data;
	char *pattern;
} octeonPatternInfo;

typedef struct _octeonGraphInfo
{
	uint32_t pattern_no;
	octeonPatternInfo *patterns;
} octeonGraphInfo;

typedef struct _info_dfa
{
	 uint32_t **nmn;
	 uint32_t **ntn;
	 uint32_t **tndml;
	uint64_t **nel;
	uint64_t **nelo;
	 uint32_t **size_of_ex_li;
	 uint32_t **size_of_ex_li_ov;
}info_dfa;

typedef struct _octeonStringMatchInternalData
{
	uint32_t graph_no;
	octeonGraphInfo *graphs;
	char **co_gr_info;
	char **co_gr_name;
	uint64_t **co_gr;
	uint64_t *co_gr_image_size;
	uint64_t *co_gr_llm_size;
	info_dfa *info;
} octeonStringMatchInternalData;

#endif
