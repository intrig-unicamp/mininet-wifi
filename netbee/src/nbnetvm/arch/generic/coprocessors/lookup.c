/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/** @file lookup.c
 *	\brief This file contains a simple lookup coprocessor implementation.
 */

/* This lookup coprocessor has two registers:
   - R0, R/W: Used to provide/retrieve data.
   - R1, W: Set to a non-zero value if, when reading data, data is valid. */

#include <stdlib.h>
#include <string.h>
#ifdef COPRO_SYNCH
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>
#endif
#include "nbnetvm.h"
#include <helpers.h>
#include "../../../coprocessor.h"
#include "../../../../nbee/globals/profiling-functions.h"


/* Coprocessor operations */
enum {
	LOOKUP_OP_INIT = 0,
	LOOKUP_OP_ADD_DATA = 1,
	LOOKUP_OP_ADD_VALUE = 2,
	LOOKUP_OP_READ_VALUE = 3,
	LOOKUP_OP_RESET = 4
};

#define MAX_HASH_DATA_SIZE 16
#define HASH_TABLE_ENTRIES 50000


// #define LOOKUP_COPRO_DEBUG

#ifdef LOOKUP_COPRO_DEBUG
#define ludebug(...) fprintf(stderr, __VA_ARGS__)
#include <ctype.h>
#else
#define ludebug(...)
#endif

typedef struct hash_table_data_s {
	uint32_t data[MAX_HASH_DATA_SIZE];
	uint32_t value;
	struct hash_table_data_s *next;
} hash_table_data;


typedef struct {
	uint32_t hash_data[MAX_HASH_DATA_SIZE];
	int data_used;
	hash_table_data *table[HASH_TABLE_ENTRIES];
} lookup_data;


static hash_table_data *data_new (uint32_t data[MAX_HASH_DATA_SIZE], int used, uint32_t value) {
	hash_table_data *out;

	out = (hash_table_data *) malloc (sizeof (hash_table_data));
	if (!out) {
		exit (2);
	} else {
		memcpy (out -> data, data, used * sizeof (uint32_t));
		out -> value = value;
		out -> next = NULL;
	}

	return (out);
}


static hash_table_data *list_get (hash_table_data *head, uint32_t data[MAX_HASH_DATA_SIZE], int used) {
	hash_table_data *cur;

	for (cur = head; cur; cur = cur -> next) {
		if (memcmp (cur -> data, data, used * sizeof (uint32_t)) == 0)
			break;
	}

	return (cur);
}


static hash_table_data *list_append (hash_table_data *head, hash_table_data *element) {
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

static uint32_t hsieh_hash (uint8_t *data, uint8_t len)
{
uint32_t hash = len, tmp = 0;
int rem = 0;

	if (len <= 0 || data == NULL)
		return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--)
	{
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		hash  += hash >> 11;
	}


	/* Handle end cases */
	switch (rem)
	{
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


static void init (lookup_data *ldata)
{
int i;

	ldata -> data_used = 0;
	for (i = 0; i < HASH_TABLE_ENTRIES; i++)
	{
		(ldata -> table)[i] = NULL;
	}

	return;
}


static void add_data (lookup_data *ldata, uint32_t data)
{
	(ldata -> hash_data)[ldata -> data_used] = data;
	(ldata -> data_used)++;

	return;
}


static void add_value (lookup_data *ldata, uint32_t value)
{
uint32_t hash;
hash_table_data **ptr, *entry;

	hash = hsieh_hash ((uint8_t *) &(ldata -> hash_data), ldata -> data_used * 4);
	hash %= HASH_TABLE_ENTRIES;
	ludebug ("Hashed data to %u\n", hash);

	entry = list_get ((ldata -> table)[hash], ldata -> hash_data, ldata -> data_used);
	if (entry)
	{
		/* An entry in the list for the given data already exists, just update it */
		ludebug ("Entry in list already exists, updating\n");

		entry -> value = value;
	}
	else
	{
		/* Create a new entry in the list */
		ludebug ("Creating new entry in list\n");

		ptr = &((ldata -> table)[hash]);
		entry = data_new (ldata -> hash_data, ldata -> data_used, value);
		*ptr = list_append (*ptr, entry);
		(ldata -> table)[hash] = *ptr;
	}

	return;
}


static void get_value (lookup_data *ldata, uint32_t *value, uint32_t *valid)
{
uint32_t hash;
hash_table_data *entry;

	hash = hsieh_hash ((uint8_t *) &(ldata -> hash_data), ldata -> data_used * 4);
	hash %= HASH_TABLE_ENTRIES;

#ifdef LOOKUP_COPRO_DEBUG
	ludebug("Data: ");
	hex_and_ascii_print_with_offset(stdout, "\n", (uint8_t *) &(ldata -> hash_data), ldata -> data_used * 4, 0);
	ludebug("\n");
#endif

	entry = list_get ((ldata -> table)[hash], ldata -> hash_data, ldata -> data_used);
	if (entry) {
		*value = entry -> value;
		*valid = 1;
		ludebug ("Got match!\n");
	} else {
		*valid = 0;
		ludebug ("No match!\n");
	}

	return;
}

/********* CALLBACKS *********/


static int32_t copro_lookup_run (nvmCoprocessorState *c, uint32_t operation)
{
	lookup_data *ldata;
	uint32_t *data, *valid;

	
#ifdef RTE_PROFILE_COUNTERS
	c->ProfCounter_tot->TicksStart= nbProfilerGetTime();
#endif

	data = &((c -> registers)[0]);
	valid = &((c -> registers)[1]);
	ldata = c -> data;

	ludebug ("Lookup coprocessor invoked (%p): op=%u/R0=%u/R1=%u\n", c, operation, *data, *valid);

	switch (operation) {
		case LOOKUP_OP_INIT:
			/* Reset coprocessor state. */
			ludebug ("Lookup coprocessor reset\n");
			
			/* When coprocessor access is serialized, process get the
			 * semaphore in this reset call and release the mutex on
			 * LOOKUP_OP_READ_VALUE.
			 * Attention: this operation is called during coprocessor
			 * initialization as well, thus semphore needs to be
			 * initialized according to the number of cores. See for
			 * instance netvmsnort/src/muiltiprocess2.c:182 */
			
#ifdef COPRO_SYNCH
			sem_wait(&(c->sems->serializing_semaphore));
			ludebug("Pid %d, running on copro sem\n", getpid());
#endif

			init (ldata);
			break;
		case LOOKUP_OP_ADD_DATA:
			/* Add a value to compute the hash on. */
			ludebug ("Lookup coprocessor data added: %u\n", *data);

			add_data (ldata, *data);
			break;
		case LOOKUP_OP_ADD_VALUE:
			/* Set value to be assigned to hashed data. Also triggers hash computation. */
			ludebug ("Lookup coprocessor - Insert op, added value: %u\n", *data);

			add_value (ldata, *data);
			break;
		case LOOKUP_OP_READ_VALUE:
			/* Retrieve value assigned to hashed data. */
			ludebug ("Lookup coprocessor - Lookup op\n");

			get_value (ldata, data, valid);

#ifdef COPRO_SYNCH
			sem_post(&(c->sems->serializing_semaphore));
			ludebug("Pid %d, posted on copro sem\n", getpid());		 
#endif

			break;
		case LOOKUP_OP_RESET:
			ludebug ("Lookup coprocessor reset\n");
			/* Please note that this is NOT meant to reset the whole coprocessor */
			ldata -> data_used = 0;
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


int32_t nvmLookupCoproInit (nvmCoprocessorState *c, void *useless) {
	lookup_data *ldata = c -> data;

	ludebug ("Lookup coprocessor initialising\n");

	init (ldata);

	return (nvmSUCCESS);
}


int32_t nvmCoproLookupCreate(nvmCoprocessorState *lookup)
{
	static uint8_t flags[]={COPREG_WRITE | COPREG_READ, COPREG_READ};
	static uint32_t regs[]={0,0};

#ifdef RTE_PROFILE_COUNTERS
	uint32_t i;
#endif

#ifdef COPRO_SYNCH
	int fd;
	struct copro_semaphores *ptr;
#endif

	ludebug ("Allocating lookup coprocessor (at %p)\n", lookup);

	if(lookup == NULL)
	{
		printf("Error allocating lookup\n");
		return nvmFAILURE;
	}
	strncpy(lookup->name,"lookup",MAX_COPRO_NAME);
	lookup->n_regs = 2;
	memcpy(lookup->reg_flags, flags, 2* sizeof(uint8_t));
	memcpy(lookup->registers, regs, 2*sizeof(uint32_t));
	lookup->init = nvmLookupCoproInit;
	lookup->write =nvmCoproStandardRegWrite;
	lookup->read = nvmCoproStandardRegRead;
	lookup->invoke = copro_lookup_run;
	lookup->data =malloc(sizeof(lookup_data));
	if(lookup->data == NULL)
	{
		printf("Error in allocating lookup->data\n");
		return nvmFAILURE;
	}
	lookup->xbuf = NULL;

#ifdef RTE_PROFILE_COUNTERS
	lookup->ProfCounter=calloc (1, 5 * sizeof(nvmCounter *));
	for (i=0; i<5; i++)
	{
		lookup->ProfCounter[i]=calloc (1, sizeof(nvmCounter));
		lookup->ProfCounter[i]->TicksDelta= nbProfilerGetMeasureCost();
	}
	lookup->ProfCounter_tot=calloc (1, sizeof(nvmCounter));
	lookup->ProfCounter_tot->TicksDelta= nbProfilerGetMeasureCost();
#endif
	
#ifdef COPRO_SYNCH
	fd = shm_open(COPRO_TMP_FNAME, O_RDWR , S_IRWXU); /* open */
	if( (ptr = mmap(NULL, sizeof(struct copro_semaphores), PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, 0)) == -1){
		ludebug("child cannot open shared mem");
		exit(-1);
	};
	ftruncate(fd, sizeof(struct copro_semaphores)); /* resize */
	close(fd);								 /* close fd */
	lookup->sems = ptr;
#endif
	
	return nvmSUCCESS;
}
