/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdlib.h>
#include <string.h>
#include "nbnetvm.h"
#include <helpers.h>
#include "../../../../nbee/globals/debug.h"
#include "../../../../nbee/globals/profiling-functions.h"
#include "../../../coprocessor.h"

#ifdef WIN32
#define STATIC_INLINE __forceinline
#else
#define STATIC_INLINE static inline
#endif

/* Coprocessor operations */
enum {
	LOOKUP_OP_INIT = 0,
	LOOKUP_OP_INSERT = 1,
	LOOKUP_OP_LOOKUP = 2
};

#define MAX_HASH_DATA_SIZE 5
#define HASH_TABLE_ENTRIES 0x10000


// #define LOOKUP_COPRO_DEBUG

#ifdef LOOKUP_COPRO_DEBUG
#define ludebug(...) fprintf(stderr, __VA_ARGS__)
#include <ctype.h>
#else
#define ludebug(...)
#endif

typedef struct hash_table_data_s {
	uint32_t data[MAX_HASH_DATA_SIZE];
	uint32_t val0, val1;
	struct hash_table_data_s *next;
} hash_table_data;


typedef struct {
	uint32_t *hash_data;
	int data_used;
	hash_table_data *table[HASH_TABLE_ENTRIES];
	uint32_t counters[HASH_TABLE_ENTRIES];
} lookup_data;



STATIC_INLINE void lookup_new_print_data(uint8_t *data, uint32_t len, const char* endl)
{
	uint32_t i;
	for (i = 0; i < len; i++)
	{
		ludebug("%u ", data[i]);
	}
	ludebug("%s", endl);
}



STATIC_INLINE hash_table_data *lookup_new_data_new (uint32_t data[MAX_HASH_DATA_SIZE], int used, uint32_t val0, uint32_t val1) {
	hash_table_data *out;

	out = (hash_table_data *) malloc (sizeof (hash_table_data));
	if (!out) {
		exit (2);
	} else {
		memcpy (out -> data, data, used * sizeof (uint32_t));
		out -> val0 = val0;
		out -> val1 = val1;
		out -> next = NULL;
	}

	return (out);
}


STATIC_INLINE hash_table_data *lookup_new_list_get (hash_table_data *head, uint32_t data[MAX_HASH_DATA_SIZE], int used) {
	hash_table_data *cur;

	for (cur = head; cur; cur = cur -> next) {
		if (memcmp (cur -> data, data, used * sizeof (uint32_t)) == 0)
			break;
	}

	return (cur);
}


STATIC_INLINE hash_table_data *lookup_new_list_append (hash_table_data *head, hash_table_data *element) {
	hash_table_data *cur;

	if (!head) {
		head = element;
	} else {
		for (cur = head; cur -> next; cur = cur -> next)
			;
		cur -> next = element;
	}

	return (head);
}


/*
	HASH Function code (C) by Paul Hsieh
	http://www.azillionmonkeys.com/qed/hash.html
*/

#if !defined (get16bits)
#define get16bits(d) ((((const uint8_t *)(d))[1] << 8)\
                      +((const uint8_t *)(d))[0])
#endif

STATIC_INLINE uint32_t lookup_new_hsieh_hash (uint8_t *data, uint8_t len) {
	uint32_t hash = len, tmp = 0;
	int rem = 0;

    NETVM_ASSERT(len > 0 && data != NULL, "wrong arguments");

    ludebug("Hashing ");
    lookup_new_print_data(data, len, "\n");


    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 2;
    hash += hash >> 15;
    hash ^= hash << 10;

    ludebug("result: %u\n", hash);
    return hash;
}

static void lookup_new_reset_regs(nvmCoprocessorState *c)
{
	memset(c->registers, 0, c->n_regs * sizeof(uint32_t));
}



static  void lookup_new_insert (nvmCoprocessorState *c) {
	uint32_t hash;
	hash_table_data **ptr, *entry;
	lookup_data *ldata = c->data;

	hash = lookup_new_hsieh_hash ((uint8_t *) c->registers, ldata -> data_used * 4);
	ludebug ("Hash: %u\n", hash);
	hash %= HASH_TABLE_ENTRIES;
	ludebug ("Hashed data to %u\n", hash);
	ludebug("insert data_used: %u ", ldata->data_used);
	entry = lookup_new_list_get ((ldata -> table)[hash], ldata -> hash_data, ldata -> data_used);
	if (entry) {
		/* An entry in the list for the given data already exists, just update it */
		ludebug ("Entry in list already exists, updating\n");

		entry -> val0 = c->registers[5];
		entry -> val1 = c->registers[6];
	} else {
		/* Create a new entry in the list */
		ludebug ("Creating new entry in list\n");

		ptr = &((ldata -> table)[hash]);
		entry = lookup_new_data_new (ldata -> hash_data, ldata -> data_used, c->registers[5], c->registers[6]);
		*ptr = lookup_new_list_append (*ptr, entry);
		(ldata -> table)[hash] = *ptr;
		(ldata -> counters)[hash]++;
		ludebug ("Num entries for hash %u: %u\n", hash, (ldata -> counters)[hash]);
	}

}


static int lookup_new_lookup (nvmCoprocessorState *c){
	uint32_t hash;
	hash_table_data *entry;
	lookup_data *ldata = c->data;

	hash = lookup_new_hsieh_hash ((uint8_t *) c->registers, ldata -> data_used * 4);
	hash %= HASH_TABLE_ENTRIES;


	entry = lookup_new_list_get ((ldata -> table)[hash], ldata -> hash_data, ldata -> data_used);
	if (entry) {
		c->registers[5] = entry -> val0;
		c->registers[6] = entry -> val1;
#ifndef _EXP_COPROCESSOR_MODEL
		c->registers[7] = 1;
#endif
		ludebug ("Got match!\n");
		return 1;
	} else {
		c->registers[5] = 0;
		c->registers[6] = 0;
#ifndef _EXP_COPROCESSOR_MODEL
		c->registers[7] = 0;
#endif
		ludebug ("No match!\n");
		return 0;
	}

	return 0;
}


static void lookup_new_print_copro_state(nvmCoprocessorState *c)
{
	ludebug("Lookup Coprocessor Registers:\n");
	ludebug("Key:\n");
	ludebug("R0-R1-R2-R3-R4: %x-%x-%x-%x-%x\n", c->registers[0], c->registers[1], c->registers[2],
			c->registers[3], c->registers[4]);
	ludebug("Value:\n");
	ludebug("R5-R6: %x-%x\n", c->registers[5], c->registers[6]);
	ludebug("Match:\n");
	ludebug("R7: %x\n", c->registers[7]);

}

/********* CALLBACKS *********/


int32_t nvmLookupNewCoproInit (nvmCoprocessorState *c, void *useless) {
	lookup_data *ldata = c->data;
	uint32_t i;

	ludebug ("Lookup coprocessor initialising\n");

	for (i = 0; i < HASH_TABLE_ENTRIES; i++) {
		(ldata -> table)[i] = NULL;
	}

//	ldata->data_used = c->registers[0];
	return (nvmSUCCESS);
}



static int32_t nvmLookupNewCoproInvoke (nvmCoprocessorState *c, uint32_t operation)
{

#ifdef RTE_PROFILE_COUNTERS
	c->ProfCounter_tot->TicksStart= nbProfilerGetTime();
#endif

	ludebug ("Lookup coprocessor invoked: op=%u\n", operation);
	lookup_new_print_copro_state(c);

	switch (operation) {
		case LOOKUP_OP_INIT:
			nvmLookupNewCoproInit(c, NULL);
			break;

		case LOOKUP_OP_INSERT:
			ludebug ("Lookup coprocessor - Insert op\n");

			lookup_new_insert(c);
			break;
		case LOOKUP_OP_LOOKUP:
			/* Retrieve the value assigned to hashed data. */
			ludebug ("Lookup coprocessor - Lookup op\n");

			lookup_new_lookup(c);
			break;
		default:
			/* Unsupported operation. This should throw an exception. */
			printf ("Lookup coprocessor: unsupported operation: %u\n", operation);
			exit (1);
			break;
	}

#ifdef RTE_PROFILE_COUNTERS
	c->ProfCounter_tot->TicksEnd= nbProfilerGetTime();
	c->ProfCounter_tot->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
	c->ProfCounter[operation]->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
	if (operation == 1)
		c->ProfCounter[operation]->NumPkts++;
#endif

	return (nvmSUCCESS);
}




int32_t nvmCoproLookupNewCreate(nvmCoprocessorState *lookup)
{
	/*
	 * Coprocessor registers are laid out in this way:
	 *
	 * 160-bit Key:
	 * R0	W
	 * R1	W
	 * R2	W
	 * R3	W
	 * R4	W
	 *
	 * 64-bit Value:
	 * R5	RW
	 * R6	RW
	 *
	 * Match Flag:
	 * R7	R
	 *
	 */
	static uint8_t flags[]={
			COPREG_WRITE,				//R0
			COPREG_WRITE,				//R1
			COPREG_WRITE,				//R2
			COPREG_WRITE,				//R3
			COPREG_WRITE,				//R4
			COPREG_READ|COPREG_WRITE,	//R5
			COPREG_READ|COPREG_WRITE,	//R6
			COPREG_READ					//R7
	};

	uint32_t num_regs = 8;

#ifdef RTE_PROFILE_COUNTERS
	uint32_t i;
#endif

	ludebug ("Allocating lookup coprocessor\n");

	if(lookup == NULL)
	{
		printf("Error allocating lookup\n");
		return nvmFAILURE;
	}
	strncpy(lookup->name,"lookupnew",MAX_COPRO_NAME);
	lookup->n_regs = num_regs;
	memcpy(lookup->reg_flags, flags, num_regs * sizeof(uint8_t));
	lookup_new_reset_regs(lookup);
	lookup->init = nvmLookupNewCoproInit;
	lookup->write =nvmCoproStandardRegWrite;
	lookup->read = nvmCoproStandardRegRead;
	lookup->invoke = nvmLookupNewCoproInvoke;
#ifdef _EXP_COPROCESSOR_MODEL
	memset(lookup->OpFunctions, 0, MAX_COPRO_OPS * sizeof(void*));
	lookup->OpFunctions[LOOKUP_OP_INSERT] = lookup_new_insert;
	lookup->OpFunctions[LOOKUP_OP_LOOKUP] = lookup_new_lookup;
#endif

	lookup->data =calloc(1, sizeof(lookup_data));
	if(lookup->data == NULL)
	{
		printf("Error in allocating lookup->data\n");
		return nvmFAILURE;
	}
	((lookup_data*)lookup->data)->hash_data = lookup->registers;
	((lookup_data*)lookup->data)->data_used = 5; //

	lookup->xbuf = NULL;
#ifdef RTE_PROFILE_COUNTERS
	lookup->ProfCounter=calloc (1, 5 * sizeof(nvmCounter *));
	for (i=0; i<5; i++)
  	{
		lookup->ProfCounter[i]= calloc (1, sizeof(nvmCounter));
		lookup->ProfCounter[i]->TicksDelta= nbProfilerGetMeasureCost();
  	}
  	lookup->ProfCounter_tot=calloc (1, sizeof(nvmCounter));
	lookup->ProfCounter_tot->TicksDelta= nbProfilerGetMeasureCost();
#endif
	return nvmSUCCESS;
}

