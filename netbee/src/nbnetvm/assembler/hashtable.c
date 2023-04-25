#include <stdlib.h>
#include <string.h>
#include "nbnetvm.h"
#include "hashtable.h"

#define HASH_TABLE_DEFAULT_SIZE 2048
// #define HASH_TABLE_DEBUG


typedef struct hash_table_data_s {
	void *key;
	uint32_t keysize;
	void *value;

	struct hash_table_data_s *next;
} hash_table_data;


struct hash_table_s {
	uint32_t size;
	hash_table_data **table;
};


static hash_table_data *data_new (void *key, uint32_t keysize, void *value) {
	hash_table_data *out;

	out = (hash_table_data *) malloc (sizeof (hash_table_data));
	if (!out) {
		exit (2);
	} else {
		out -> key = key;
		out -> keysize = keysize;
		out -> value = value;
		out -> next = NULL;
	}

	return (out);
}


static hash_table_data *list_get (hash_table_data *head, void *key, uint32_t keysize) {
	hash_table_data *cur;

	for (cur = head; cur; cur = cur -> next) {
		if (memcmp (cur -> key, key, keysize) == 0)
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

static uint32_t hsieh_hash (uint8_t *data, uint32_t len) {
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

/*** EXPORTED FUNCTIONS ***/

hash_table *hash_table_new (uint32_t size)
{
	unsigned int i;
	hash_table *ht;

	if (size == 0)
		size = HASH_TABLE_DEFAULT_SIZE;

	ht = (hash_table *) malloc (sizeof (hash_table));
	if (ht) {
		ht -> size = size;
		if ((ht -> table = (hash_table_data **) malloc (sizeof (hash_table_data *) * size))) {
			for (i = 0; i < size; i++) {
				ht -> table[i] = NULL;
			}
		} else {
			free (ht);
			ht = NULL;
		}
	}

	return (ht);
}


void hash_table_destroy (hash_table *ht, void (*keyfreefunc) (void *), void (*valuefreefunc) (void *))
{
	unsigned int i;
	hash_table_data *head, *cur, *next;

	for (i = 0; i < ht -> size; i++) {
		head = ht -> table[i];

		for (cur = head; cur; cur = next) {
			next = cur -> next;
			if (keyfreefunc)
				keyfreefunc (cur -> key);
			if (valuefreefunc)
				valuefreefunc (cur -> value);
			free (cur);
		}
	}
	free (ht);

	return;
}


int hash_table_add (hash_table *ht, void *key, uint32_t keysize, void *value) {
	uint32_t hash;
	hash_table_data **ptr, *entry;
	int ret;

	hash = hsieh_hash ((uint8_t *) key, keysize);
	hash %= ht -> size;
#ifdef HASH_TABLE_DEBUG
	printf ("Hashed data to %u\n", hash);
#endif
	entry = list_get (ht -> table[hash], key, keysize);
	if (entry) {
		/* An entry in the list for the given data already exists, just update it */
#ifdef HASH_TABLE_DEBUG
		printf ("Entry in list already exists, updating\n");
#endif
		entry -> value = value;
		ret = 1;
	} else {
		/* Create a new entry in the list */
#ifdef HASH_TABLE_DEBUG
		printf ("Creating new entry in list\n");
#endif
		ptr = &(ht -> table[hash]);
		if ((entry = data_new (key, keysize, value))) {
			*ptr = list_append (*ptr, entry);
			ht -> table[hash] = *ptr;
			ret = 1;
		} else {
			ret = 0;
		}
	}

	return (ret);
}


int hash_table_lookup (hash_table *ht, void *key, uint32_t keysize, void **value) {
	uint32_t hash;
	hash_table_data *entry;
	int ret;

	hash = hsieh_hash ((uint8_t *) key, keysize);
	hash %= ht -> size;
#ifdef HASH_TABLE_DEBUG
	printf ("Hashed data to %u\n", hash);
#endif
	entry = list_get (ht -> table[hash], key, keysize);
	if (entry) {
		if (value)
			*value = entry -> value;
		ret = 1;
	} else {
		ret = 0;
	}

	return (ret);
}

hash_table *htval;
