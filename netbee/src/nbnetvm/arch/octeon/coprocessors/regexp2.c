/** @file regexp.c
 *	\brief This file contains the (Perl-compatible) Regular Expression matching coprocessor implementation
 */

#include <stdlib.h>
#include <string.h>
#include "nbnetvm.h"
#include "../../../coprocessor.h"
//#include <pcre.h>

#ifdef COPRO_REGEXP_DEBUG
#define redebug printf
#else
#define redebug(...)
#endif

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
//	pcre *re;					//!< Compiled regular expression (for libpcre).
//	pcre_extra *re_extra;				//!< Data to speed up matching (libpcre-specific).
} nvmRegExpCoproPattern;


/* This struct represents a match result. */
typedef struct nvmRegExpCoproMatchResult_s {
	nvmRegExpCoproPattern *rcp;
	int offset;
	int len;
} nvmRegExpCoproMatchResult;


typedef struct {
	nvmRegExpCoproPattern *patterns;
	uint32_t patterns_no;
	nvmRegExpCoproMatchResult match;
	int matched;				//!< True if we had a match
} nvmRegExpCoproInternalData;

/*
static nvmRegExpCoproInternalData redata;
*/

static int nvmRegExpCoproParseFlagsPcre (char *flags) {
	uint32_t i = 0, out = 0; /* Start with no flags */
/*	for (i = 0; i < strlen (flags); i++) {
		switch (flags[i]) {
			case 'i':
				out |= PCRE_CASELESS;
				break;
			case 's':
				out |= PCRE_DOTALL;
				break;
			case 'm':
				out |= PCRE_MULTILINE;
				break;
			case 'x':
				out |= PCRE_EXTENDED;
				break;
			case 'A':
				out |= PCRE_ANCHORED;
				break;
			case 'E':
				out |= PCRE_DOLLAR_ENDONLY;
				break;
			case 'G':
				out |= PCRE_UNGREEDY;
				break;
			case 'R':
			case 'U':
			case 'B':
				// These are Snort-specific flags, so we just ignore them and assume NetVMSnort
				 // will do what is needed to support them 
				break;
			default:
				fprintf (stderr, "Unsupported RegExp flag: %c (%s)\n", flags[i], flags);
				break;
		}
	}
*/
	return (out);
}


nvmRegExpCoproPattern *nvmRegExpCoproNewPattern (nvmRegExpCoproPattern *rcp, char *pattern, int pattern_length, char *flags) {
/*	pcre *re;
	const char *re_err;
	int re_err_offset;

	if (!(re = pcre_compile (pattern, nvmRegExpCoproParseFlagsPcre (flags), &re_err, &re_err_offset, NULL))) {
		redebug ("Error compiling regexp at character %d: %s\n", re_err_offset, re_err);
		rcp = NULL;
	} else {
		memset (rcp, 0, sizeof (nvmRegExpCoproPattern));
#ifdef COPRO_REGEXP_DEBUG
		rcp -> pattern = strdup (pattern);
#endif
		rcp -> re = re;
		rcp -> re_extra = pcre_study (re, 0, &re_err);
		if (re_err != NULL) {
			redebug ("Cannot study regular expression \"%s\"", pattern);
			rcp -> re_extra = NULL;		// Just make sure 
		}
	}
*/
	return (rcp);
}

#ifndef JIT_ARCH_OCTEON

#define SIZE_DB (sizeof (uint8_t))
#define SIZE_DW (sizeof (uint16_t))
#define SIZE_DD (sizeof (uint32_t))

int32_t nvmRegExpCoproInjectData (nvmCoprocessorState *c, uint8_t *data) {
	int32_t out;
/*	nvmRegExpCoproInternalData *redata = c -> data;
	uint16_t g = 0, i = 0, patterns_no = 0, pattern_length = 0, flags_length = 0;
	int32_t out;
	char flags[20], *p;

	redata = c -> data;

	redebug ("Regular expressions coprocessor initialising\n");

	redata -> patterns_no = *(uint16_t *) data;
	data += SIZE_DW;
	redebug ("* %hd RegExp's\n", redata -> patterns_no);
	redata -> patterns = (nvmRegExpCoproPattern *) malloc (sizeof (nvmRegExpCoproPattern) * redata -> patterns_no);

	for (g = 0; g < redata -> patterns_no; g++) {
		flags_length = *(uint16_t *) data;
		data += SIZE_DW;
		memcpy (flags, data, flags_length);
		flags[flags_length] = '\0';
		data += SIZE_DB * flags_length;
		redebug ("* Pattern:\n  - Flags length: %hd\n", flags_length);
		redebug ("  - Flags: \"%s\"\n", flags);

		pattern_length = *(uint16_t *) data;
		data += SIZE_DW;
		redebug ("  - Pattern length: %hd\n", pattern_length);
		p = (char *) malloc (sizeof (char) * (pattern_length + 1));
		memcpy (p, data, pattern_length);
		p[pattern_length] = '\0';
		redebug ("  - Text: \"%s\"\n", p);
		
		nvmRegExpCoproNewPattern (&(redata -> patterns[g]), p, pattern_length, flags);
		free (p);

		// On with next pattern 
		data += pattern_length * SIZE_DB;
	}
*/
	out = nvmSUCCESS;

	return (out);
}
#endif


#define OVECTSIZE 15		/* Should be a multiple of 3 */

static uint32_t nvmRegExpCoproTestPattern (uint16_t pattern_id, uint8_t *haystack, uint32_t haylen,
							uint32_t start_offset, nvmRegExpCoproInternalData *redata) {
	/*
	 nvmRegExpCoproPattern *rcp;
	int ovector[OVECTSIZE], m  = 0, matchoffset = 0 , matchlen = 0;
#ifdef COPRO_REGEXP_DEBUG
			const char *tmp;
#endif
	redata -> matched = 0;

	// Do the actual search 
	if (pattern_id < redata -> patterns_no) {
		rcp = &(redata -> patterns[pattern_id]);
		redebug ("Looking for RegExp: \"%s\"\n", rcp -> pattern);
		m = pcre_exec (rcp -> re, rcp -> re_extra, haystack + start_offset, haylen, 0, 0, ovector, OVECTSIZE);
		if (m > 0) {
			(redata -> match).rcp = rcp;
			(redata -> match).offset = matchoffset = ovector[0];
			(redata -> match).len = ovector[1] - ovector[0];
			redata -> matched = 1;
#ifdef COPRO_REGEXP_DEBUG
			pcre_get_substring (haystack + start_offset, ovector, m, 0, &tmp);
			redebug ("RegExp match at offset %d, len = %d: \"%s\"\n", matchoffset, matchlen, tmp);
			printf ("RegExp match at offset %d, len = %d: \"%s\"\n", matchoffset, matchlen, tmp);
			pcre_free_substring (tmp);
#endif
		}
	} else {
		printf ("RegExp coprocessor: tried to use inexistent pattern: %u\n", pattern_id);
	}
*/
	return (redata -> matched);
}



static int32_t nvmRegExpCoproRun (nvmCoprocessorState *c, uint32_t operation) {
#ifdef RTE_PROFILE_COUNTERS
	u_int64t *st = &c->ProfCounter_tot->TicksStart;
	GET_TICKS (st);
#endif
/*	
	
	nvmRegExpCoproInternalData *redata;
	uint32_t *r0 = NULL, *r1 = NULL, *r2 = NULL, *r3  = NULL, i  = 0;

	r0 = &((c -> registers)[0]);
	r1 = &((c -> registers)[1]);
	r2 = &((c -> registers)[2]);
	r3 = &((c -> registers)[3]);
	redata = c -> data;
	redebug ("RegExp coprocessor invocation starting: op=%u/R0=%u/R1=%u/R2=%u/R3=%u\n", operation, *r0, *r1, *r2, *r3);

	switch (operation) {
		case REGEXP_OP_INIT:
			// Reset coprocessor state - Do nothing ATM 
			redebug ("Regexp coprocessor reset\n");
			break;
		case REGEXP_OP_TRYMATCH:
			// Search for matches in whole payload 
			*r0 = nvmRegExpCoproTestPattern ((uint16_t) *r0, c -> xbuf -> PacketBuffer, *r2, 0, redata);
			break;
		case REGEXP_OP_TRYMATCH_WITH_OFFSET:
			// Search for matches starting from offset specified in R0 
			*r0 = nvmRegExpCoproTestPattern ((uint16_t) *r0, c -> xbuf -> PacketBuffer, *r2, *r1, redata);
			break;
		case REGEXP_OP_GETRESULT:
			// Return match result 
			if (*r0 > 0 && redata -> matched) {
				(*r0)--;
				*r1 = 0;
				*r2 = (redata -> match).offset + (redata -> match).len;//mi faccio restituire l'offset della fine della prima ricorrenza del pattern che trova ps le regexp 
				redebug ("Returning RegExp result: %u\n", *r1);
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
	#ifdef RTE_PROFILE_COUNTERS
	uint64_t *sto = &c->ProfCounter_tot->TicksEnd;
	GET_TICKS (sto);
	c->ProfCounter_tot->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
	c->ProfCounter[operation]->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
	#endif
  */ 
	return (nvmSUCCESS);
}
int32_t octeonRegExpInit(nvmCoprocessorState* regexp, void* initOffset)
{
	return (nvmSUCCESS);
}

int32_t OcteonCoproRegexpCreate(nvmCoprocessorState *regexp)
{
	static uint8_t flags[]={COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ};
	static uint32_t regs[]={0,0};
	if(regexp == NULL)
	{
		printf("Error in allocating regexp\n");
		return nvmFAILURE;
	}
	strncpy(regexp->name,"regexp",MAX_COPRO_NAME);
	regexp->n_regs = 4;
	memcpy(regexp->reg_flags, flags, 4* sizeof(uint8_t));
	memcpy(regexp->registers, regs, 4*sizeof(uint32_t));
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

int32_t nvmCoproRegexpCreate(nvmCoprocessorState *regexp)
{
	static uint8_t flags[]={COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ | COPREG_WRITE, COPREG_READ};
	static uint32_t regs[]={0,0};
	uint32_t i;
	if(regexp == NULL)
	{
		printf("Error in allocating regexp\n");
		return nvmFAILURE;
	}
	strncpy(regexp->name,"regexp",MAX_COPRO_NAME);
	regexp->n_regs = 4;
	memcpy(regexp->reg_flags, flags, 4* sizeof(uint8_t));
	memcpy(regexp->registers, regs, 4*sizeof(uint32_t));
	regexp->init = (nvmCoproInitFunct *) nvmRegExpCoproInjectData;
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
	#ifdef RTE_PROFILE_COUNTERS
		regexp->ProfCounter=calloc (1, 4 * sizeof(nvmCounter *));
	for (i=0;i<4;i++)
  	{
		regexp->ProfCounter[i]=calloc (1, sizeof(nvmCounter));
		nvmSampleDeltaTicks(&regexp->ProfCounter[i]->TicksDelta);
  	}
	regexp->ProfCounter_tot=calloc (1, sizeof(nvmCounter));
	nvmSampleDeltaTicks(&regexp->ProfCounter_tot->TicksDelta);
	#endif
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
