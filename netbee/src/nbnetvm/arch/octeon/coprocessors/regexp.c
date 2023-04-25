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

/** @file regexp.c
 *	\brief This file contains the (Perl-compatible) Regular Expression matching coprocessor implementation
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nbnetvm.h"
#include "../arch.h"
#include "../../../coprocessor.h"
#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-helper.h"
#include "cvmx-sysinfo.h"
#include "cvmx-coremask.h"
#include "cvmx-fpa.h"
#include "cvmx-fau.h"
#include "cvmx-dfa.h"
#include "cvmx-bootmem.h"
#include "dfa-infox4.h"

//#include <sys/types.h> 
//#include <sys/stat.h> 
//#include <fcntl.h>
// #define COPRO_REGEXP_DEBUG

#ifdef COPRO_REGEXP_DEBUG
#define redebug printf
#else
#define redebug(...)
#endif
//#define REPLICATION 2

//CVMX_SHARED char *input;
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
	REGEXP_OP_INIT = 0,
	REGEXP_OP_TRYMATCH = 1,
	REGEXP_OP_TRYMATCH_WITH_OFFSET = 2,
	REGEXP_OP_GETRESULT = 3
};


typedef struct {
#ifdef COPRO_REGEXP_DEBUG
	char *pattern;					//!< RegExp pattern in human format.
#endif
	//pcre *re;					//!< Compiled regular expression (for libpcre).
	//pcre_extra *re_extra;				//!< Data to speed up matching (libpcre-specific).
} nvmRegExpCoproPattern;


/* This struct represents a match result. */
typedef struct nvmRegExpCoproMatchResult_s {
	nvmRegExpCoproPattern *rcp;
	int offset;
	int len;
} nvmRegExpCoproMatchResult;


typedef struct {
	nvmRegExpCoproPattern *patterns;
	u_int32_t patterns_no;
	nvmRegExpCoproMatchResult match;
	int matched;				//!< True if we had a match
} nvmRegExpCoproInternalData;

static nvmRegExpCoproInternalData redata;


static int nvmRegExpCoproParseFlagsPcre (char *flags) {
	u_int32_t i = 0, out = 0; /* Start with no flags */
	return (out);
}






#define OVECTSIZE 15		/* Should be a multiple of 3 */

static u_int32_t nvmRegExpCoproTestPattern (u_int16_t pattern_id, u_int8_t *haystack, u_int32_t haylen,
							u_int32_t start_offset, nvmRegExpCoproInternalData *redata) {

    cvmx_dfa_graph_t graph;
    unsigned int replication=REPLICATION;
    unsigned int replication_code;
	// convert replication to replication code
	for(replication_code=0; ((unsigned)1<<replication_code) < replication; replication_code++) ;
//#define MAX_REPLICATION 8

	pattern_id ++;//le regexp partono da 1 i pattern da 0
	redata -> matched = 0;
	//printf("Setting up the large graph (which has already been loaded into LL memory)\n");
	graph.base_address = 0;
	graph.replication = replication_code;
	graph.start_node = 0;
	graph.type = CVMX_DFA_GRAPH_TYPE_LG;


	int num_engines,j,length;
    dfa_result_t *result[MAX_NUM_PARALLEL];
    cycles_t cycles[MAX_NUM_PARALLEL];
	char *string;
	char *input;//= "..L..n....E..E..B?.@.@................J.Az................B>....uPASSabcdefg..\n";
    int match=0;
    uint16_t *p;
    //INPUT da definire globale nella shared mem COMMENTARE QUI SOTTO
/*
	printf("prima di alloc\n");
	input = cvmx_bootmem_alloc(65536, 128);
	if (input == NULL) {
	    printf("Could not allocate input.\n");
	    printf("TEST FAILED\n");
	    return 1;
	}
	length =strlen(input);
	memset(input, 0, length);
*/	
	length = strlen(haystack + start_offset);
	printf("length =%d haylen=%d\n",length,haylen);
	//strcpy(input,haystack + start_offset);
	//strcpy(haystack,input);
//TODO [MB] num di coprocesso di che si voule usare decisi a compile time????
    num_engines=1;

    for (j=0; j<num_engines;j++) {
	result[j] = cvmx_fpa_alloc(CVMX_FPA_DFA_POOL);
	if (result[j] == NULL) {
	    printf("Could not allocate result.\n");
	}
    }

    for (j=0; j<num_engines;j++) {
    // dispatch jobs
	cycles[j].start = cvmx_get_cycle();
	cvmx_dfa_submit(&graph, graph.start_node,cvmx_phys_to_ptr(haystack + start_offset)/* input + start_offset*/, /*length*/haylen, 0, 0,
                   &result[j]->r0, (sizeof(*result[0])/8)-1, NULL);
    }

    // wait for jobs
    for (j=0; j<num_engines;j++) {
	while (!cvmx_dfa_is_done(&result[j]->r0))
	{
//	printf("spinning\n");
	    /* Spinning */
	}
	cycles[j].end = cvmx_get_cycle();
    
	//printf("pattern_id:%d\n",pattern_id);
	for (match=0; match< result[j]->r0.s.num_entries; match++) {
	//printf("match:%d result[j]->r0.s.num_entries:%d\n ",match,result[j]->r0.s.num_entries);
	    p = cvm_dfa_expression_list( result[j]->r1[match].s.current_node,
		    NUM_MARKED_NODES, NUM_TERMINAL_NODES, TERMINAL_NODE_DELTA_MAP_LG);
	    while (*p != 0) {
		if (*p++ == pattern_id)
		{
			(redata->match).offset = result[j]->r1[match].s.byte_offset;
			//(redata->match).len = result[j]->r1[match].s.byte_offset - start_offset;
	  /*  	printf("%3u prev  BERGI ENTRA  = %6u current = %6u offset=%6u\n num_entries=%u\n",
		    match,
		    result[j]->r1[match].s.prev_node,
		    result[j]->r1[match].s.current_node,
		    result[j]->r1[match].s.byte_offset,
			result[j]->r0.s.num_entries);
		*/	
			redata->matched=1;
			return  (redata -> matched);
 
		}
		//printf("Match of Exp %4d: @%4d\n", *p, result[j]->r1[match].s.byte_offset);
	    }
	   /* printf("%3u prev BERGI non entra   = %6u current = %6u offset=%6u\n num_entries=%u\n",
		    match,
		    result[j]->r1[match].s.prev_node,
		    result[j]->r1[match].s.current_node,
		    result[j]->r1[match].s.byte_offset,
			result[j]->r0.s.num_entries);
		*/	
	}
	return (redata -> matched);
}
}


static int32_t nvmRegExpCoproRun (nvmCoprocessorState *c, u_int32_t operation) {
	
	nvmRegExpCoproInternalData *redata;
	u_int32_t *r0 = NULL, *r1 = NULL, *r2 = NULL, *r3  = NULL, i  = 0;

	r0 = &((c -> registers)[0]);
	r1 = &((c -> registers)[1]);
	r2 = &((c -> registers)[2]);
	r3 = &((c -> registers)[3]);
	redata = c -> data;
	redebug ("RegExp coprocessor invocation starting: op=%u/R0=%u/R1=%u/R2=%u/R3=%u\n", operation, *r0, *r1, *r2, *r3);

	switch (operation) {
		case REGEXP_OP_INIT:
			// Reset coprocessor state - Do nothing ATM 
			printf ("Regexp coprocessor reset\n");
			break;
		case REGEXP_OP_TRYMATCH:
			// Search for matches in whole payload 
			*r0 = nvmRegExpCoproTestPattern /*(1,NULL,1,1,NULL);*/ ((u_int16_t) *r0, c -> xbuf -> PacketBuffer, *r2, 0, redata);
			break;
		case REGEXP_OP_TRYMATCH_WITH_OFFSET:
			// Search for matches starting from offset specified in R0 
			*r0 = nvmRegExpCoproTestPattern ((u_int16_t) *r0, c -> xbuf -> PacketBuffer, *r2, *r1, redata);
			break;
		case REGEXP_OP_GETRESULT:
			// Return match result
			
			if (*r0 > 0 && redata -> matched) {
				(*r0)--;
				*r1 = 0;
				*r2 = (redata -> match).offset + 1;//mi faccio restituire l'offset della fine della prima ricorrenza del pattern che trova ps le regexp 
			//	*r3 = (redata -> match).len;
				//redebug ("Returning RegExp result: %u\n", *r1);
			//BERGI	printf ("Returning RegExp result: %u\n", *r2);
			} else {
				// No more results to return, an exception should be thrown. 
				printf ("RegExp coprocessor: tried to read inexistent match result\n");
				exit (2);
			}

			break;
		default:
			// Unsupported operation. This should throw an exception. 
			printf ("RegExp coprocessor: unsupported operation: %u\n", operation);
			exit (1);
			break;
	}

	redebug ("RegExp coprocessor invocation ending: op=%u/R0=%u/R1=%u/R2=%u/R3=%u\n", operation, *r0, *r1, *r2, *r3);
	
	return (nvmSUCCESS);
}

int32_t octeonRegExpInit(nvmCoprocessorState* regexp, void* initOffset)
{
	return (nvmSUCCESS);
}

int32_t OcteonCoproRegexpCreate(nvmCoprocessorState *regexp)
{
	static u_int8_t flags[]={COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ};
	static u_int32_t regs[]={0,0};
	if(regexp == NULL)
	{
		printf("Error in allocating regexp\n");
		return nvmFAILURE;
	}
	strncpy(regexp->name,"regexp",MAX_COPRO_NAME);
	regexp->n_regs = 4;
	memcpy(regexp->reg_flags, flags, 4* sizeof(u_int8_t));
	memcpy(regexp->registers, regs, 4*sizeof(u_int32_t));
	regexp->init = octeonRegExpInit;
	regexp->write = nvmCoproStandardRegWrite;
	regexp->read = nvmCoproStandardRegRead;
	regexp->invoke = nvmRegExpCoproRun;
	regexp->data =malloc(sizeof(nvmRegExpCoproInternalData));
	if(regexp->data == NULL)
	{
		printf("Error in allocating regexp->data\n");
		return nvmFAILURE;
	}
	regexp->xbuf = NULL;
	return nvmSUCCESS;
}
/*

nvmCoprocessorState nvmRegExpCoprocessor = {
	"regexp",
	4,
	{COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ},
	{0, 0},
	(nvmCoproInitFunct *) nvmRegExpCoproInjectData,
	nvmCoproStandardRegWrite,
	nvmCoproStandardRegRead,
	nvmRegExpCoproRun,
	&redata
};
*/
