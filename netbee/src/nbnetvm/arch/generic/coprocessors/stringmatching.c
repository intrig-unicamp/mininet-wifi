/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/** @file stringmatching.c
 *	\brief This file contains the string-matching coprocessor implementation
 */

#include <stdlib.h>
#include <string.h>
#include "nbnetvm.h"
#include "../../../coprocessor.h"
#include "../../../../nbee/globals/profiling-functions.h"
#include "acsmx2.h"

#ifdef COPRO_STRINGMATCH_DEBUG
#define smdebug printf
#include <ctype.h>
#else
#define smdebug(...)
#endif

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
	nvmStringMatchCoproPattern *p;
	int offset;
} nvmStringMatchCoproMatchResult;


#define MAX_MATCHES 1600		//!< Maximum number of mathes returned

/* This struct contains all data representing the state of the coprocessor. */
typedef struct {
	ACSM_STRUCT2 **acsm;
	uint32_t graphs_no;

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

int32_t nvmStringMatchCoproInjectData (nvmCoprocessorState *c, uint8_t *data) {
	uint32_t /*byte_order,*/ pattern_data;
	uint16_t g, i, patterns_no, pattern_length, pattern_nocase;
	nvmStringMatchCoproInternalData *smcdata = c -> data;
	nvmStringMatchCoproPattern *p, *q;
	int32_t out;

	smdebug ("String-matching coprocessor initialising\n");

	/* Get first byte, which contains info on the data */
// 	byte_order = *(uint32_t *) data;
// 	data += SIZE_DD;		// So that it points to pattern data

	smcdata -> patterns = NULL;

	smcdata -> graphs_no = *(uint16_t *) data;
	data += SIZE_DW;
	smdebug ("* %hd pattern groups\n", smcdata -> graphs_no);
	smcdata -> acsm = (ACSM_STRUCT2 **) malloc (sizeof (ACSM_STRUCT2 *) * smcdata -> graphs_no);

	/* Read data for all graphs */
	for (g = 0; g < smcdata -> graphs_no; g++) {
		/* Init Aho-Corasick state machine */
		smcdata -> acsm[g] = acsmNew2 ();
		smcdata -> acsm[g] -> acsmFormat = ACF_FULL;		// For the moment...

		patterns_no = *(uint16_t *) data;
		data += SIZE_DW;
		smdebug ("* %hd patterns\n", patterns_no);
		for (i = 0; i < patterns_no; i++) {
			pattern_length = *(uint16_t *) data;
			data += SIZE_DW;
			smdebug ("* Pattern:\n  - Length: %hd\n", pattern_length);
			pattern_nocase = *(uint16_t *) data;
			data += SIZE_DW;
			smdebug ("  - Nocase: %hd\n", pattern_nocase);
			pattern_data = *(uint32_t *) data;
			data += SIZE_DD;
			smdebug ("  - Data: %u\n", pattern_data);

			smdebug ("  - Text: \"");
			print_pattern ((char *)data, pattern_length);
			smdebug ("\"\n");

			/* Add pattern to list */
			if (!(p = (nvmStringMatchCoproPattern *) malloc (sizeof (nvmStringMatchCoproPattern)))) {
				fprintf (stderr, "Cannot allocate memory for pattern\n");
				exit (9);
			}
			p -> pattern = (char *) malloc (pattern_length * SIZE_DB);
			memcpy (p -> pattern, data, pattern_length);
			p -> len = pattern_length;
			p -> data = pattern_data;
			p -> next = NULL;

			/* Append new pattern to list */
			if (smcdata -> patterns != NULL) {
				for (q = smcdata -> patterns; q -> next; q = q -> next)
					;
				q -> next = p;
			} else {
				smcdata -> patterns = p;
			}

			/* Add pattern to Aho-Corasick SM */
			acsmAddPattern2 (smcdata -> acsm[g], data, pattern_length, pattern_nocase, 0, 0, p, i);
			smcdata -> patterns_no++;

			/* On with next pattern */
			data += pattern_length * SIZE_DB;
		}

		/* All patterns added: compile graph */
	// 	Print_DFA (smcdata -> acsm);
		acsmCompile2 (smcdata -> acsm[g]);
#ifdef COPRO_STRINGMATCH_DEBUG
		acsmPrintInfo2 (smcdata -> acsm[g]);
#endif
	}

	/* Prepare stuff for results */
	smcdata -> matches_no = 0;
	smcdata -> next_match_id = 0;

	out = nvmSUCCESS;

	return (out);
}


static int nvmStringMatchCoproMatchFoundCallback (void *add_data, int pattern_offset, void *search_data) {
	nvmStringMatchCoproInternalData *smcdata;
	nvmStringMatchCoproPattern *p;
	nvmStringMatchCoproMatchResult *r;

	smcdata = (nvmStringMatchCoproInternalData *) search_data;
	p = (nvmStringMatchCoproPattern *) add_data;
	smdebug ("String match %d at offset %d, len = %d, data = %u: \"", smcdata -> matches_no, pattern_offset, p -> len, p -> data);
	//printf ("String match %d at offset %d, len = %d, data = %u: \"", smcdata -> matches_no, pattern_offset, p -> len, p -> data);
	print_pattern (p -> pattern, p -> len);
	smdebug ("\"\n");

	if (smcdata -> matches_no < MAX_MATCHES) {
		r = &(smcdata -> matches[smcdata -> matches_no]);
		r -> p = p;
		r -> offset = pattern_offset;
		smcdata -> matches_no++;
	} else {
		fprintf (stderr, "Too many matches, cannot add last match!\n");
	}

	return (0);		// 0 = Go on, 1 = Stop search (?)
}


static uint32_t nvmStringMatchCoproTestPatternGroup (uint16_t group_id, uint8_t *haystack, uint32_t haylen,
							uint32_t start_offset, nvmStringMatchCoproInternalData *smcdata) {
	ACSM_STRUCT2 *acsm;
	//nvmStringMatchCoproMatchResult *r = NULL, *next = NULL;

	/* Reset match results data */
	smcdata -> matches_no = 0;
	smcdata -> next_match_id = 0;

	//printf("group_id: %d\n",group_id);
	//printf("haylen: %d\n",haylen);
	//printf("haystack: %s\n",haystack+start_offset);
	/* Do the actual search */
	if (group_id < smcdata -> graphs_no) {
		smdebug ("Matching pattern group %u\n", group_id);
		acsm = smcdata -> acsm[group_id];
		acsmSearch2 (acsm, haystack + start_offset, haylen, nvmStringMatchCoproMatchFoundCallback, smcdata);
	} else {
		printf ("String-matching coprocessor: tried to use inexistent graph\n");
	}

	//printf("matches_no: %d\n",smcdata->matches_no);
	return (smcdata -> matches_no);
}


static int32_t nvmStringMatchCoproRun (nvmCoprocessorState *c, uint32_t operation) {

	nvmStringMatchCoproInternalData *smcdata;
	nvmStringMatchCoproMatchResult *r;
	uint32_t *r0, *r1, *r2, *r3;
#ifdef RTE_PROFILE_COUNTERS
	c->ProfCounter_tot->TicksStart= nbProfilerGetTime();
#endif
	r0 = &((c -> registers)[0]);
	r1 = &((c -> registers)[1]);
	r2 = &((c -> registers)[2]);
	r3 = &((c -> registers)[3]);
        // suppress gcc warning
        (void) r3;
	smcdata = c -> data;
	smdebug ("String-matching coprocessor invocation starting: op=%u/R0=%u/R1=%u/R2=%u/R3=%u\n", operation, *r0, *r1, *r2, *r3);

	switch (operation) {
		case SM_OP_RESET:
			/* Reset coprocessor state - Do nothing ATM */
			smdebug ("String-matching coprocessor reset\n");

			break;
		case SM_OP_TRYMATCH:
				//printf ("group id: %d\n", *r0);
			/* Search for matches in whole payload using graph specified in R0 */
			*r0 = nvmStringMatchCoproTestPatternGroup ((uint16_t) *r0, c -> xbuf -> PacketBuffer, *r2, 0, smcdata);
			break;
		case SM_OP_TRYMATCH_WITH_OFFSET:
				//printf ("group id: %d\n", *r0);
			/* Search for matches starting from offset specified in R1 using graph specified in R0 */
			*r0 = nvmStringMatchCoproTestPatternGroup ((uint16_t) *r0, c -> xbuf -> PacketBuffer, *r2, *r1, smcdata);
			break;
		case SM_OP_GETRESULT:
			/* Return next result */
			if (*r0 > 0 && smcdata->next_match_id < smcdata->matches_no) {
				r = &(smcdata -> matches[smcdata -> next_match_id]);
				(*r0)--;
				*r1 = r -> p -> data;
				*r2 = r -> offset + r->p->len;
				//*r3 = r -> p -> len;
				smcdata -> next_match_id++;
				smdebug ("Returning String-matching result: %u\n", *r1);
				//printf ("R0: %d\n", *r0);
				//printf ("r1:R->p->data: %d\n", *r1);
				//printf ("r2:R->offset: %d\n", *r2);
			} else {
				/* No more results to return, an exception should be thrown. */
				printf ("String-matching coprocessor: tried to read inexistent match result\n");
				exit (2);
			}
			break;
		default:
			/* Unsupported operation. This should throw an exception. */
			printf ("String-matching coprocessor: unsupported operation: %u\n", operation);
			exit (1);
			break;
	}
	smdebug ("String-matching coprocessor invocation ending: op=%u/R0=%u/R1=%u/R2=%u/R3=%u\n", operation, *r0, *r1, *r2, *r3);

#ifdef RTE_PROFILE_COUNTERS
	c->ProfCounter_tot->TicksEnd= nbProfilerGetTime();
	c->ProfCounter_tot->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
	c->ProfCounter[operation]->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
	if (operation == 3)
		c->ProfCounter_tot->NumPkts++;
#endif
	return (nvmSUCCESS);
}


/* Cleanup memory used by this coprocessor. Not used at the moment, but useful to keep track of what should be released. */
int32_t nvmStringMatchCoproCleanup (nvmCoprocessorState *c, uint8_t *data) {
	nvmStringMatchCoproInternalData *smcdata = c -> data;
	nvmStringMatchCoproPattern *p = NULL, *nextp = NULL;
	//nvmStringMatchCoproMatchResult *r = NULL, *nextr = NULL;
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

#ifdef RTE_PROFILE_COUNTERS
	uint32_t i;
#endif

	if (stringmatch == NULL)
	{
		printf("Error in allocating stringmatch\n");
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
#ifdef RTE_PROFILE_COUNTERS
	stringmatch->ProfCounter= calloc (1, 4 * sizeof(nvmCounter *));

	for (i=0; i<4; i++)
	{
		stringmatch->ProfCounter[i]=calloc (1, sizeof(nvmCounter));
		stringmatch->ProfCounter[i]->TicksDelta= nbProfilerGetMeasureCost();
	}
	stringmatch->ProfCounter_tot=calloc (1, sizeof(nvmCounter));
	stringmatch->ProfCounter_tot->TicksDelta= nbProfilerGetMeasureCost();
#endif

	stringmatch->xbuf = NULL;
	return nvmSUCCESS;
}

