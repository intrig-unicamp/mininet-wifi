/*
 * Copyright (c) 2002 - 2005
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Politecnico di Torino nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/** @file stringmatching.c
 *	\brief This file contains the string-matching coprocessor implementation
 */

#include <stdlib.h>
#include <string.h>
#include "nbnetvm.h"
#include "../arch.h"
#include "../../../coprocessor.h"
#include "stringmatching.h"
#include "acsmx2.h"
#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-helper.h"
#include "cvmx-sysinfo.h"
#include "cvmx-coremask.h"
#include "cvmx-fpa.h"
#include "cvmx-fau.h"
#include "cvmx-dfa.h"
#include "cvmx-llm.h"
#include "cvmx-bootmem.h"
#include "cvmx-asm.h"

//#include "dfa_info.h"
//#include "graph.h"
#define REPLICATION 1

#ifdef COPRO_STRINGMATCH_DEBUG
#define smdebug printf
#include <ctype.h>
#else
#define smdebug(...)
#endif
//#define REPLICATION 2

#define MAX_REPORT_MATCHES 15
#define MAX_NUM_PARALLEL 15

typedef struct {
    cvmx_dfa_result0_t  r0;
    cvmx_dfa_result1_t  r1[MAX_REPORT_MATCHES];
} dfa_result_t;

typedef struct {
    uint64_t start;
    uint64_t end;
    uint64_t delta;
} cycles_t;


/* Coprocessor operations */
enum {
	SM_OP_RESET = 0,
	SM_OP_TRYMATCH = 1,
	SM_OP_TRYMATCH_WITH_OFFSET = 2,
	SM_OP_GETRESULT = 3
};


/* This struct represents a pattern. The 'pattern' field itself is not very useful, as it is duplicated in the Aho-Corasick
 * state machine anyway, but might be useful for debugging, ATM. */
typedef struct nvmStringMatchCoproPattern_s {
	char *pattern;					//!< Compiled regular expression (for libpcre).
	int len;
	uint32_t data;					//!< Data to be returned in case this RE is matched.

	struct nvmStringMatchCoproPattern_s *next;
} nvmStringMatchCoproPattern;


/* This struct represents a match result. */
typedef struct nvmStringMatchCoproMatchResult_s {
	//nvmStringMatchCoproPattern *p;
	octeonPatternInfo *p;
	int offset;
} nvmStringMatchCoproMatchResult;


#define MAX_MATCHES 1600		//!< Maximum number of mathes returned

/* This struct contains all data representing the state of the coprocessor. */
typedef struct {
	ACSM_STRUCT2 **acsm;
	uint32_t graphs_no;
	uint64_t *graph_address;
	nvmStringMatchCoproPattern *patterns;
	uint32_t patterns_no;

	nvmStringMatchCoproMatchResult matches[MAX_MATCHES];
	uint32_t next_match_id;
	uint32_t matches_no;
} nvmStringMatchCoproInternalData;

/*
nvmStringMatchCoproInternalData smcdata;
*/


static void print_pattern (char *pattern, int len) {
#ifdef COPRO_STRINGMATCH_DEBUG
	int i;
	unsigned char c;

	for (i = 0; i < len; i++) {
		c = pattern[i];
		if (isprint (c) && c != '\"' && c != '\\')
			smdebug ("%c", c);
		else
			smdebug ("\\x%02X", c);
	}
#endif
	return;
}


#define SIZE_DB (sizeof (uint8_t))
#define SIZE_DW (sizeof (uint16_t))
#define SIZE_DD (sizeof (uint32_t))

extern octeonStringMatchInternalData smcdata;
extern info_dfa info_prova;

#ifdef STRINGMATCHING_PROFILE
uint64_t totalStringmatching;
uint64_t StringMatchingCallsNum;
#endif

// TODO
// should receive the data to load as a parameter instead of 
// relying on the global variable
int32_t loadGraphIntoLLM(nvmCoprocessorState *stringmatch)
{
	int i = 0;
	uint32_t n = smcdata.graph_no;
    
	nvmStringMatchCoproInternalData *d;
	cvmx_llm_address_t llm_addr = {0};
	d = (nvmStringMatchCoproInternalData *)stringmatch->data;
	d->graph_address = malloc(n * sizeof(uint64_t));
	smdebug("loading %d\n graphs into llm.", n);
	//stringmatch->graph_address = *address;
	for (i = 0; i < n; i++)
    {
        d->graph_address[i] = llm_addr.u64;	
        smdebug("Loading graph '%s' from the image into LL memory\n", smcdata.co_gr_name[i]);
        const uint64_t *pword, *pend;
        if(i != 0)
        {
            smdebug("sizeof co_gr: %d - sizeof pend: %d\n",
                    smcdata.co_gr_image_size[i] + 8 - smcdata.co_gr_image_size[i-1],
                    sizeof(*pend));
            pend = &smcdata.co_gr[i][
                (smcdata.co_gr_image_size[i] + 8- smcdata.co_gr_image_size[i-1]) 
                / sizeof(*pend)];
        }
        else
        {
            smdebug("sizeof co_gr: %d - sizeof pend: %d\n",
                    (smcdata.co_gr_image_size[i] + 8), sizeof(*pend));
            pend = &smcdata.co_gr[i][
                (smcdata.co_gr_image_size[i] + 8) / sizeof(*pend)];
        }

        for (pword=smcdata.co_gr[i]; pword != pend; pword++) {
            uint64_t word;
            uint64_t run;
            union {
                uint64_t u64;
                struct {
                    uint64_t copies:28;
                    uint64_t value:36;
                } s;
            } item;

            item.u64 = *pword;
            word = item.s.value;
            for (run = item.s.copies+1; run > 0; run--) {
                cvmx_llm_write36(llm_addr, word, 0);
                llm_addr.s.address += 4;
            }
        }
    }
}

// TODO
// should receive the data to load as a parameter instead of 
// relying on the global variable
int32_t nvmStringMatchCoproInjectData (nvmCoprocessorState *c, uint8_t *data) {
	int32_t out;

	octeonStringMatchInternalData *int_data = &smcdata; //(octeonStringMatchInternalData *)data;
	uint32_t pattern_data;
	uint16_t g, i, patterns_no, pattern_length, pattern_nocase;
	nvmStringMatchCoproInternalData *smc_data = c -> data;
	nvmStringMatchCoproPattern *p, *q, *tail;

	smdebug ("String-matching coprocessor initialising\n");
    loadGraphIntoLLM(c);
	smdebug ("Graphs loaded in llm.\n");
	smc_data -> patterns = NULL;

	smc_data -> graphs_no = int_data->graph_no;
	smdebug ("* %hd pattern groups\n", smc_data -> graphs_no);

	// Prepare stuff for results 
	smc_data -> matches_no = 0;
	smc_data -> next_match_id = 0;

	out = nvmSUCCESS;

	return (out);
}

static uint16_t *cvm_dfa_expression_list(
    uint16_t ngraph, 
    uint64_t *node_to_expression_lists, 
    uint64_t *node_to_expression_lists_overflow, 
    uint64_t node_id, 
    uint64_t num_marked_nodes, 
    uint64_t num_terminal_nodes, 
    uint64_t terminal_node_delta_map)
{
    uint16_t *ret;
    if (node_id >= num_marked_nodes+num_terminal_nodes) 
        node_id -= terminal_node_delta_map;
    if (node_id >= 
        ((*(info_prova.size_of_ex_li[ngraph])*sizeof(node_to_expression_lists))/sizeof(uint64_t))) 
        node_id = 0;
    ret = (uint16_t *)&node_to_expression_lists[node_id];
    if (ret[3] != 0) 
        ret = (uint16_t *)((uint8_t *)node_to_expression_lists_overflow + (*(uint32_t *)ret));
    return (ret);
}

static uint32_t nvmStringMatchCoproTestPatternGroup(
    uint16_t group_id, uint8_t *haystack, uint32_t haylen,
	uint32_t start_offset, nvmStringMatchCoproInternalData *smcdatai) {
	
	/* Reset match results data */
	smcdatai->matches_no = 0;
	smcdatai->next_match_id = 0;

	// group_id is the graph num
    smdebug("group_id: %d\n",group_id);

	// convert replication to replication code
    unsigned int replication = REPLICATION;
    unsigned int replication_code;
	for(replication_code=0; ((unsigned)1<<replication_code) < replication; replication_code++);

    cvmx_dfa_graph_t graph;
	graph.base_address = smcdatai->graph_address[group_id]; 
	graph.replication = replication_code;
	graph.start_node = 0;
	graph.type = CVMX_DFA_GRAPH_TYPE_LG;

    //TODO [MB] num di coprocesso di che si voule usare decisi a compile time????
	int num_engines = 1;
    dfa_result_t *result[MAX_NUM_PARALLEL];
    cycles_t cycles[MAX_NUM_PARALLEL];

    int j;
    for (j=0; j<num_engines;j++) {
        result[j] = cvmx_fpa_alloc(CVMX_FPA_DFA_POOL);
        if (result[j] == NULL) {
            printf("Could not allocate result.\n");
        }
    }

    // dispatch jobs
    for (j=0; j<num_engines;j++) {
        cycles[j].start = cvmx_get_cycle();
        cvmx_dfa_submit(&graph, graph.start_node,
                        (haystack + start_offset), // cvmx_phys_to_ptr
                        haylen, // lenght
                        0, 0, &result[j]->r0, (sizeof(*result[0])/8)-1, NULL);
    }

#ifdef STRINGMATCHING_PROFILE
    uint64_t before, after;
    CVMX_RDHWR(before, 31);
#endif
    
    // wait for jobs
    int match;
    for (j=0; j<num_engines;j++) {
        while (!cvmx_dfa_is_done(&result[j]->r0)); // Spinning

#ifdef STRINGMATCHING_PROFILE
        CVMX_RDHWR(after, 31);
        smdebug("operation: %d ticks per stringmatching %lld\n", 0, after - before);
        totalStringmatching += after - before;
#endif

        for (match=0; match< result[j]->r0.s.num_entries; match++) {
            uint16_t *p = cvm_dfa_expression_list(
                    group_id, 
                    (info_prova.nel[group_id]), 
                    (info_prova.nelo[group_id]), 
                    result[j]->r1[match].s.current_node,
                    *(info_prova.nmn[group_id]),
                    *(info_prova.ntn[group_id]) , 
                    *(info_prova.tndml[group_id]));
            while (*p != 0) {
                //se è current_node==NUM_TERMINAL_NODES allora è il match finto inoltr prev_node e current_node sono uguali
                if (result[j]->r1[match].s.prev_node !=result[j]->r1[match].s.current_node )
                {
                    (smcdatai->matches)[smcdatai -> matches_no].offset = result[j]->r1[match].s.byte_offset;
                    (smcdatai->matches)[smcdatai -> matches_no].p = &smcdata.graphs[group_id].patterns[(*p)-1];
                    smcdatai->matches_no++;
                }
                p++;
            }
        }
        cvmx_fpa_free(result[j], CVMX_FPA_DFA_POOL, 0);
    }
    return (smcdatai -> matches_no);
}


static int32_t nvmStringMatchCoproRun (nvmCoprocessorState *c, uint32_t operation) {
	nvmStringMatchCoproInternalData *smcdatai;
	nvmStringMatchCoproMatchResult *r;
	uint32_t *r0, *r1, *r2, *r3;
	r0 = &((c -> registers)[0]);
	r1 = &((c -> registers)[1]);
	r2 = &((c -> registers)[2]);
	r3 = &((c -> registers)[3]);
	smcdatai = c -> data;
	smdebug ("String-matching coprocessor invocation starting: op=%u/R0=%u/R1=%u/R2=%u/R3=%u\n", operation, *r0, *r1, *r2, *r3);
//	printf("prima di switch: %d\n",operation);
	switch (operation) {
		case SM_OP_RESET:
			/* Reset coprocessor state - Do nothing ATM */
			smdebug ("String-matching coprocessor reset\n");
			break;
		case SM_OP_TRYMATCH:
			/* Search for matches in whole payload using graph specified in R0 */
			*r0 = nvmStringMatchCoproTestPatternGroup ((uint16_t) *r0, c -> xbuf -> PacketBuffer, *r2, 0, smcdatai);
			break;
		case SM_OP_TRYMATCH_WITH_OFFSET:
			/* Search for matches starting from offset specified in R1 using graph specified in R0 */
			*r0 = nvmStringMatchCoproTestPatternGroup ((uint16_t) *r0, c -> xbuf -> PacketBuffer, *r2, *r1, smcdatai);
			break;
		case SM_OP_GETRESULT:
			/* Return next result */
			if (*r0 > 0 && smcdatai->next_match_id < smcdatai->matches_no) {
				r = &(smcdatai -> matches[smcdatai -> next_match_id]);
				(*r0)--;
				*r1=0;
				*r1 = r -> p -> data;
				*r2 = (r -> offset) + 1;
				*r3 =0;// r -> p -> len;
				smcdatai -> next_match_id++;
				smdebug ("Returning String-matching result: %u\n", *r1);
				//printf ("r0: %d\n", *r0);
				//printf ("r1:R->p->data: %d\n",*r1);// r->p->data);
				//printf ("r2:R->offset: %d\n", *r2);//r->offset + 1);
			} else {
				/* No more results to return, an exception should be thrown. */
				fprintf (stderr, "String-matching coprocessor: tried to read inexistent match result\n");	
				exit (2);
			}
			break;
		default:
			/* Unsupported operation. This should throw an exception. */
			fprintf (stderr, "String-matching coprocessor: unsupported operation: %u\n", operation);
			exit (1);
			break;
	}

	smdebug ("String-matching coprocessor invocation ending: op=%u/R0=%u/R1=%u/R2=%u/R3=%u\n", operation, *r0, *r1, *r2, *r3);

#ifdef STRINGMATCHING_PROFILE
    StringMatchingCallsNum++;
#endif
	return (nvmSUCCESS);
}


/* Cleanup memory used by this coprocessor. Not used at the moment, but useful to keep track of what should be released. */
int32_t nvmStringMatchCoproCleanup (nvmCoprocessorState *c, uint8_t *data) {
	nvmStringMatchCoproInternalData *smcdata = c -> data;
	nvmStringMatchCoproPattern *p = NULL, *nextp = NULL;
	nvmStringMatchCoproMatchResult *r = NULL, *nextr = NULL;
	uint16_t g;

	for (p = smcdata -> patterns; p; p = nextp) {
		free (p -> pattern);
		nextp = p -> next;
		free (p);
	}

	/* Reset match results data */
	free (smcdata -> matches);

	/* Free graphs data */
	for (g = 0; g < smcdata -> graphs_no; g++)
		acsmFree2 (smcdata -> acsm[g]);

	/* Free whole object */
	free (smcdata);

	return (nvmSUCCESS);
}

int32_t nvmCoproStringmatchCreate(nvmCoprocessorState *stringmatch)
{
	static uint8_t flags[]=	{COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ};
	static uint32_t regs[]={0,0};
	if(stringmatch == NULL)
	{
		fprintf(stderr, "Error allocating stringmatch\n");
		return nvmFAILURE;
	}
	strncpy(stringmatch->name,"stringmatching",MAX_COPRO_NAME);
	stringmatch->n_regs = 4;
	memcpy(stringmatch->reg_flags, flags, 4*sizeof(uint8_t));
	memcpy(stringmatch->registers, regs, 4*sizeof(uint32_t));
	stringmatch->init =(nvmCoproInitFunct *) nvmStringMatchCoproInjectData;
	stringmatch->write =nvmCoproStandardRegWrite;
	stringmatch->read = nvmCoproStandardRegRead;
	stringmatch->invoke = nvmStringMatchCoproRun;
	stringmatch->data =malloc(sizeof(nvmStringMatchCoproInternalData));
	if(stringmatch->data == NULL)
	{
		printf("Error in allocating stringmatch->data\n");
		return nvmFAILURE;
	}
	stringmatch->xbuf = NULL;
	return nvmSUCCESS;
}

/*
nvmCoprocessorState nvmStringMatchingCoprocessor = {
	"stringmatching",
	4,
	{COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ},
	{0, 0},
	(nvmCoproInitFunct *) nvmStringMatchCoproInjectData,
	nvmCoproStandardRegWrite,
	nvmCoproStandardRegRead,
	nvmStringMatchCoproRun,
	&smcdata
};
*/
