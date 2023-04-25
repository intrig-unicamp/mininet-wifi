/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/* This lookup coprocessor has eight registers that are used in both the 
initialization of the lookup table than in other operation.
INITIALIZATION:
- R0, W:   -Number of tables to manage (coprocessor initialization)
           -Id of the table to use     (table initialization)
- R1, W:   -Number of max entries of the specified table (table initialization)
- R2, W:   -Entry key_size(in byte) of the specified table (table initialization)
- R3, W:   -Entry value_size(in 4Byte) of the specified table (table initialization)

OTHER OPERATION:
- R0, W:   -Id of the table to use   
- R1, W:   -Value of the key/value to set(4B) /Offset to read or write(operations: GetValue and UpdateValue)
- R2, W:   -Value of the key to set(4B)
- R3, W:   -Value of the key to set(4B)  
- R4, W:   -Value of the key to set(4B)
- R5, W:   -Key/Value size in byte
- R6, R/W: -Value to read(operation: get value)- Value to write(operation: update value) 
- R7, R:   -Flag register
*/


#include <stdlib.h>
#include <string.h>
#include "nbnetvm.h"
#include "../../../coprocessor.h"
#include "../../../../nbee/globals/profiling-functions.h"


/* Coprocessor operations */
enum {
	LOOKUP_EX_OP_INIT = 0,
	LOOKUP_EX_OP_INIT_TABLE = 1,
	LOOKUP_EX_OP_ADD_KEY = 2,
	LOOKUP_EX_OP_ADD_VALUE = 3,
	LOOKUP_EX_OP_SELECT = 4,
	LOOKUP_EX_OP_GET_VALUE = 5,
	LOOKUP_EX_OP_UPD_VALUE = 6,
	LOOKUP_EX_OP_DELETE = 7,
	LOOKUP_EX_OP_RESET = 8
};

//#define LOOKUP_EX_COPRO_DEBUG

typedef struct hash_table_entry_ex_s {
	uint32_t *key;
	uint32_t *value;
	struct hash_table_entry_ex_s *next;
} hash_table_entry_ex;


typedef struct {
	uint32_t tables;

	uint32_t *entries;
	uint32_t *key_size;
	uint32_t *value_size;
	uint32_t *key_used;
	uint32_t *value_used;
	uint8_t  **hash_key;
	uint32_t  **hash_value;

	hash_table_entry_ex **selected;
	uint32_t *hash;
	hash_table_entry_ex ***table;
} lookup_ex_data;


static hash_table_entry_ex *list_get (hash_table_entry_ex *head, uint8_t *key, int key_size) {
	hash_table_entry_ex *cur;

	for (cur = head; cur; cur = cur -> next) {
		if (memcmp (cur -> key, key, key_size * sizeof(uint8_t)) == 0)
			break;
	}

	return (cur);
}


static hash_table_entry_ex *list_append (hash_table_entry_ex *head, hash_table_entry_ex *element) {
	hash_table_entry_ex *cur;

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

static uint32_t hsieh_hash (uint8_t *data, uint8_t len) {
	uint32_t hash = len, tmp = 0;
	int rem = 0;

	if (len <= 0 || data == NULL) return 0;

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

	return hash;
}

static /*inline*/ int8_t reset_table (nvmCoprocessorState *c) {
	 lookup_ex_data *ldata = c->data;

	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> key_used == NULL)
		return nvmFAILURE;
	if (ldata -> value_used == NULL)
		return nvmFAILURE;
	if (ldata -> selected == NULL)
		return nvmFAILURE;


	(ldata -> key_used)[c->registers[0]] = 0;
	(ldata -> value_used)[c->registers[0]] = 0;
	(ldata -> selected)[c->registers[0]] = NULL;
	(ldata -> hash)[c->registers[0]] = 0;

	return nvmSUCCESS;
}


static /*inline*/ int8_t init (nvmCoprocessorState *c) {
    lookup_ex_data *ldata = c->data;

	ldata -> key_size = calloc(c->registers[0], sizeof(uint32_t));
	if (ldata -> key_size == NULL)
		return nvmFAILURE;

	ldata -> key_used = calloc(c->registers[0], sizeof(uint32_t));
	if (ldata -> key_used == NULL)
		return nvmFAILURE;

	ldata -> value_size = calloc(c->registers[0], sizeof(uint32_t));
	if (ldata -> value_size == NULL)
		return nvmFAILURE;

	ldata -> value_used = calloc(c->registers[0], sizeof(uint32_t));
	if (ldata -> value_used == NULL)
		return nvmFAILURE;

	ldata -> hash_key = calloc(c->registers[0], sizeof(uint32_t *));
	if (ldata -> hash_key == NULL)
		return nvmFAILURE;

	ldata -> hash_value = calloc(c->registers[0], sizeof(uint32_t *));
	if (ldata -> hash_value == NULL)
		return nvmFAILURE;

	ldata -> entries = calloc(c->registers[0], sizeof(uint32_t *));
	if (ldata -> entries == NULL)
		return nvmFAILURE;

	ldata -> hash = calloc(c->registers[0], sizeof(uint32_t *));
	if (ldata -> hash == NULL)
		return nvmFAILURE;

	ldata -> table = calloc(c->registers[0], sizeof(hash_table_entry_ex **));
	if (ldata -> table == NULL)
		return nvmFAILURE;

	ldata -> selected = calloc(c->registers[0], sizeof(hash_table_entry_ex *));
	if (ldata -> selected == NULL)
		return nvmFAILURE;

	ldata -> tables = c->registers[0];

	return nvmSUCCESS;
}

static /*inline*/ int8_t init_table (nvmCoprocessorState *c) {
    lookup_ex_data *ldata = c->data;

	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> entries == NULL)
		return nvmFAILURE;
	if (ldata -> key_size == NULL)
		return nvmFAILURE;
	if (ldata -> value_size == NULL)
		return nvmFAILURE;

	(ldata -> entries)[c->registers[0]] = c->registers[1];
	(ldata -> key_size)[c->registers[0]] = c->registers[2];
	(ldata -> value_size)[c->registers[0]] = c->registers[3];

	if ((ldata -> entries)[c->registers[0]] > 0)
	{
		(ldata -> table)[c->registers[0]] = calloc((ldata -> entries)[c->registers[0]], sizeof(hash_table_entry_ex *));
		if ((ldata -> table)[c->registers[0]] == NULL)
			return nvmFAILURE;
	}
	if ((ldata -> key_size)[c->registers[0]] > 0)
	{
		(ldata -> hash_key)[c->registers[0]] = calloc((ldata -> key_size)[c->registers[0]], sizeof(uint8_t));
		if ((ldata -> hash_key)[c->registers[0]] == NULL)
			return nvmFAILURE;
	}
	if ((ldata -> value_size)[c->registers[0]] > 0)
	{
		(ldata -> hash_value)[c->registers[0]] = calloc((ldata -> value_size)[c->registers[0]], sizeof(uint32_t));
		if ((ldata -> hash_value)[c->registers[0]] == NULL)
			return nvmFAILURE;
	}

	reset_table (c);
	return nvmSUCCESS;
}



static hash_table_entry_ex *key_new (uint8_t *key, uint32_t key_size, uint32_t *value, uint32_t value_size) {
	hash_table_entry_ex *out;

	out = (hash_table_entry_ex *) malloc (sizeof (hash_table_entry_ex));
	out -> key = malloc(key_size * sizeof(uint8_t));
	out -> value = malloc(value_size * sizeof(uint32_t));

	memcpy (out -> key, key, key_size * sizeof(uint8_t));
	memcpy (out -> value, value, value_size * sizeof(uint32_t));
	out -> next = NULL;

	return (out);
}

static /*inline*/ int8_t add_key (nvmCoprocessorState *c) {
	lookup_ex_data *ldata = c->data;

	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> key_used == NULL)
		return nvmFAILURE;
	if (ldata -> hash_key == NULL || (ldata -> hash_key)[c->registers[0]] == NULL)
		return nvmFAILURE;

	/*control key field in byte*/
	switch(c->registers[5]){
		case 2:
			if (((ldata -> key_used)[c->registers[0]]+ c->registers[5]-1) < (ldata -> key_size)[c->registers[0]])
			{ 
				memcpy(&((ldata -> hash_key)[c->registers[0]][(ldata -> key_used)[c->registers[0]]]), &(c->registers[1]), 2*sizeof(uint8_t) );
				(ldata -> key_used)[c->registers[0]]+= 2;
			}else
			{
		#ifdef LOOKUP_EX_COPRO_DEBUG
				printf("LookupEx ERROR: You cannot add a key with a size greater than what has been initialized\n");
		#endif
				return nvmFAILURE;
			}
			break;
		case 4:
			if (((ldata -> key_used)[c->registers[0]]+ c->registers[5]-1) < (ldata -> key_size)[c->registers[0]])
			{ 
				memcpy(&((ldata -> hash_key)[c->registers[0]][(ldata -> key_used)[c->registers[0]]]),&(c->registers[1]), 4*sizeof(uint8_t));
				(ldata -> key_used)[c->registers[0]]+= 4;
			}else
			{
		#ifdef LOOKUP_EX_COPRO_DEBUG
				printf("LookupEx ERROR: You cannot add a key with a size greater than what has been initialized\n");
		#endif
				return nvmFAILURE;
			}
			break;
	}
	return nvmSUCCESS;
}


static /*inline*/ int8_t add_value (nvmCoprocessorState *c) {
	uint32_t hash;
	hash_table_entry_ex **ptr, *entry;
	lookup_ex_data *ldata = c->data;

	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> value_used == NULL)
		return nvmFAILURE;
	if (ldata -> hash_value == NULL || (ldata -> hash_value)[c->registers[0]] == NULL)
		return nvmFAILURE;
  
	if ((ldata -> value_used)[c->registers[0]] == 0)
	{
        if (ldata -> key_used == NULL)
			return nvmFAILURE;
		if (ldata -> hash_key == NULL || (ldata -> hash_key)[c->registers[0]] == NULL)
			return nvmFAILURE;
		if (ldata -> table == NULL || (ldata -> table)[c->registers[0]] == NULL)
			return nvmFAILURE;
		if (ldata -> hash == NULL)
			return nvmFAILURE;

		hash = hsieh_hash ((uint8_t *) &(ldata -> hash_key)[c->registers[0]], (ldata -> key_size)[c->registers[0]]);
		hash %= (ldata -> entries)[c->registers[0]];
		entry = list_get ((ldata -> table)[c->registers[0]][hash], (ldata -> hash_key)[c->registers[0]], (ldata -> key_used)[c->registers[0]]);
		if (entry) {
			/* An entry in the list for the given key already exists, just update it */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("     - LookupEx: table %u entry to add already existant, update it\n", table_id);
#endif
			memcpy (entry -> value, (ldata -> hash_value)[c->registers[0]], (ldata -> value_size)[c->registers[0]] * sizeof(uint32_t));
		} else {
			/* Create a new entry in the list */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("     - LookupEx: table %u entry to add not found, add it\n", table_id);
#endif
            ptr = &((ldata -> table)[c->registers[0]][hash]);
			entry = key_new ((ldata -> hash_key)[c->registers[0]], (ldata -> key_size)[c->registers[0]], (ldata -> hash_value)[c->registers[0]], (ldata -> value_size)[c->registers[0]]);
			if (entry == NULL)
				return nvmFAILURE;
			*ptr = list_append (*ptr, entry);
			(ldata -> table)[c->registers[0]][hash] = *ptr;
		}

		(ldata -> selected)[c->registers[0]] = entry;
		(ldata -> hash)[c->registers[0]] = hash;
	}

	if ((ldata -> value_used)[c->registers[0]] < (ldata -> value_size)[c->registers[0]])
	{
		(ldata -> hash_value)[c->registers[0]][(ldata -> value_used)[c->registers[0]]] = c->registers[1];
		(ldata -> value_used)[c->registers[0]]++;
	}

	return nvmSUCCESS;
}


static /*inline*/ int8_t delete_selection (nvmCoprocessorState *c) {
	lookup_ex_data *ldata = c->data;
	hash_table_entry_ex *cur = NULL;
	hash_table_entry_ex *prev = NULL;
	hash_table_entry_ex *to_delete = NULL;

	if (ldata -> table == NULL || (ldata -> table)[c->registers[0]] == NULL)
		return nvmFAILURE;
	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> selected == NULL)
		return nvmFAILURE;
	if (ldata -> hash == NULL)
		return nvmFAILURE;

	// entry to delete
	to_delete = (ldata -> selected)[c->registers[0]];

	if ( (ldata -> selected)[c->registers[0]] != NULL && (ldata -> selected)[c->registers[0]] -> value != NULL)
	{
		cur = (ldata -> table)[c->registers[0]][ (ldata -> hash)[c->registers[0]] ];
		prev = (ldata -> table)[c->registers[0]][ (ldata -> hash)[c->registers[0]] ];
		while (cur != NULL) {
			if (cur == to_delete)
			{
				if (prev == cur)
					// remove the head
					(ldata -> table)[c->registers[0]][ (ldata -> hash)[c->registers[0]] ] = cur -> next;
				else
					// replace the 'next' pointer of the previous entry
					prev -> next = cur -> next;

				break;
			}

			if (prev != cur)
				prev = cur;
			cur = cur -> next;
		}

		free(to_delete -> key);
		free(to_delete -> value);
		free(to_delete);

#ifdef LOOKUP_EX_COPRO_DEBUG
		printf("LookupEx: selected entry deleted\n");
#endif
	}
	else
	{
#ifdef LOOKUP_EX_COPRO_DEBUG
		printf("LookupEx ERROR: trying to delete when no entry has been selected\n");
#endif
		return nvmFAILURE;
	}

	return nvmSUCCESS;
}


static /*inline*/ int8_t select_value (nvmCoprocessorState *c) {
	uint32_t hash;
	hash_table_entry_ex *entry;
	lookup_ex_data *ldata = c->data;
	
	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> key_used == NULL)
		return nvmFAILURE;
	if (ldata -> hash_key == NULL || (ldata -> hash_key)[c->registers[0]] == NULL)
		return nvmFAILURE;
	if (ldata -> table == NULL || (ldata -> table)[c->registers[0]] == NULL)
		return nvmFAILURE;
	if (ldata -> entries == NULL)
		return nvmFAILURE;
	if (ldata -> selected == NULL)
		return nvmFAILURE;

	hash = hsieh_hash ((uint8_t *) &(ldata -> hash_key)[c->registers[0]], (ldata -> key_size)[c->registers[0]]);
	hash %= (ldata -> entries)[c->registers[0]];
	entry = list_get ((ldata -> table)[c->registers[0]][hash], (ldata -> hash_key)[c->registers[0]], (ldata -> key_used)[c->registers[0]]);
	(ldata -> hash)[c->registers[0]] = hash;

	if (entry) {
		(ldata -> selected)[c->registers[0]] = entry;
		c->registers[7] = 1;
	} else {
		c->registers[7] = 0;
	}
	return nvmSUCCESS;
}

static /*inline*/ int8_t get_value (nvmCoprocessorState *c) {
	lookup_ex_data *ldata = c->data;

	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> value_size == NULL)
		return nvmFAILURE;
	if (c->registers[1] >= (ldata -> value_size)[c->registers[0]])
		return nvmFAILURE;
	if (ldata -> selected == NULL || (ldata -> selected)[c->registers[0]] == NULL)
		return nvmFAILURE;

	c->registers[6] = (ldata -> hash_value)[c->registers[0]][c->registers[1]];

	return nvmSUCCESS;
}

static /*inline*/ int8_t upd_value (nvmCoprocessorState *c) {
	lookup_ex_data *ldata = c->data;

	if (c->registers[0] >= ldata -> tables)
		return nvmFAILURE;
	if (ldata -> value_size == NULL)
		return nvmFAILURE;
	if (c->registers[1] >= (ldata -> value_size)[c->registers[0]])
		return nvmFAILURE;
	if (ldata -> selected == NULL)
		return nvmFAILURE;

	if ( (ldata -> selected)[c->registers[0]] != NULL && (ldata -> selected)[c->registers[0]] -> value != NULL)
		( (ldata -> selected)[c->registers[0]] -> value )[c->registers[1]] = c->registers[6];
	else
	{
#ifdef LOOKUP_EX_COPRO_DEBUG
		printf("LookupEx ERROR: trying to update a value when no entry has been selected\n");
#endif
		return nvmFAILURE;
	}

	return nvmSUCCESS;
}

/********* CALLBACKS *********/

static int32_t copro_lookup_run (nvmCoprocessorState *c, uint32_t operation) {

	int8_t result = nvmFAILURE;

#ifdef RTE_PROFILE_COUNTERS
	c->ProfCounter_tot->TicksStart= nbProfilerGetTime();
#endif
		
	switch (operation) {
		case LOOKUP_EX_OP_INIT:
			/* Reset coprocessor state. */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("- LookupEx: reset (tables %u)\n", *table_id);
#endif
			result = init (c);
			break;

		case LOOKUP_EX_OP_INIT_TABLE:
			/* Reset table state. */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("  - LookupEx: table %u reset (entries %u, key size %u, value size %u)\n", *table_id, *reg1, *reg2, *reg3);
#endif
			result = init_table (c);
			break;

		case LOOKUP_EX_OP_ADD_KEY:
			/* Add a value to compute the hash on. */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("    - LookupEx: table %u add key (%X)\n", *table_id, *reg1);
#endif
			result = add_key (c);
			break;

		case LOOKUP_EX_OP_ADD_VALUE:
			/* Set value to be assigned to hashed key. Also triggers hash computation. */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("    - LookupEx: table %u add value (%X)\n", *table_id, *reg1);
#endif
			result = add_value (c);
			break;

		case LOOKUP_EX_OP_SELECT:
			/* Select the entry assigned to hashed key. */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("  - LookupEx: table %u select entry \n", *table_id);
#endif
			result = select_value(c);
#ifdef LOOKUP_EX_COPRO_DEBUG
			if (*reg1!=0)
				printf ("  - LookupEx: table %u entry found\n", *table_id);
#endif
			break;

		case LOOKUP_EX_OP_GET_VALUE:
			/* Retrieve value with the specified offset */
			result = get_value (c);
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("  - LookupEx: table %u read %X at offset %u\n", *table_id, *reg2, *reg1);
#endif
			break;

		case LOOKUP_EX_OP_UPD_VALUE:
			/* Update value with the specified offset */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("  - LookupEx: table %u update value (offset %u, value %u)\n", *table_id, *reg1, *reg2);
#endif
			result = upd_value (c);
			break;

		case LOOKUP_EX_OP_DELETE:
			/* Delete the selected entry */
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("  - LookupEx: table %u delete entry\n", *table_id);
#endif
			//result = delete_selection (ldata, *table_id);
			result = delete_selection (c);
			break;

		case LOOKUP_EX_OP_RESET:
#ifdef LOOKUP_EX_COPRO_DEBUG
			printf ("  - LookupEx: table %u reset\n", *table_id);
#endif
			result = reset_table (c);
			break;

		default:
			/* Unsupported operation. This should throw an exception. */
			printf ("LookupEx: unsupported operation: %u\n", operation);
			result = nvmFAILURE;
			break;
	}

#ifdef LOOKUP_EX_COPRO_DEBUG
	if (result == nvmFAILURE)
		printf("LookupEx ERROR: an error occurred during the execution of the specified operation\n");
#endif
#ifdef RTE_PROFILE_COUNTERS
	c->ProfCounter_tot->TicksEnd= nbProfilerGetTime();
	c->ProfCounter_tot->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
	c->ProfCounter[operation]->NumTicks += c->ProfCounter_tot->TicksEnd - c->ProfCounter_tot->TicksStart - c->ProfCounter_tot->TicksDelta;
#endif

	return result;
}


int32_t nvmCoproLookupEx_Create(nvmCoprocessorState *lookup)
{
	static uint8_t flags[]={
							COPREG_WRITE,					//RO
							COPREG_WRITE,					//R1
							COPREG_WRITE,					//R2
							COPREG_WRITE,					//R3
							COPREG_WRITE,					//R4
							COPREG_WRITE,					//R5
							COPREG_WRITE|COPREG_READ,		//R6
							COPREG_READ						//R7
	};

	static uint32_t regs[]={0,0,0,0,0,0,0,0};

#ifdef RTE_PROFILE_COUNTERS
	uint32_t i;
#endif

	if (lookup == NULL)
	{
		printf("Error in allocating lookup\n");
		return nvmFAILURE;
	}

	strncpy(lookup->name,"lookup_ex",MAX_COPRO_NAME);
	lookup->n_regs = 8;
	memcpy(lookup->reg_flags, flags, 8* sizeof(uint8_t));
	memcpy(lookup->registers, regs, 8*sizeof(uint32_t));
	lookup->init = NULL;
	lookup->write =nvmCoproStandardRegWrite;
	lookup->read = nvmCoproStandardRegRead;
	lookup->invoke = copro_lookup_run;
	lookup->data =malloc(sizeof(lookup_ex_data));
	if(lookup->data == NULL)
	{
		printf("Error in allocating lookup->data\n");
		return nvmFAILURE;
	}
	lookup->xbuf = NULL;

#ifdef RTE_PROFILE_COUNTERS
		lookup->ProfCounter=calloc (1, 5 * sizeof(nvmCounter *));
		for (i=0; i<4; i++)
		{
			lookup->ProfCounter[i]=calloc (1, sizeof(nvmCounter));
			lookup->ProfCounter[i]->TicksDelta= nbProfilerGetMeasureCost();
		}	

		lookup->ProfCounter_tot=calloc (1, sizeof(nvmCounter));
		lookup->ProfCounter_tot->TicksDelta= nbProfilerGetMeasureCost();
#endif
	return nvmSUCCESS;
}


