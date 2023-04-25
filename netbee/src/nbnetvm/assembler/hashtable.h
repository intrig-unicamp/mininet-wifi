/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


typedef struct hash_table_s hash_table;

/* Creates a new hash table of a given size. If size is 0 it will be chosen automatically. */
hash_table *hash_table_new (uint32_t size);

/* Adds a value-key pair to an hash table. Note that the key and value are used as pointers, not copied.
 * Returns 0 if something failed, > 0 otherwise. */
int hash_table_add (hash_table *ht, void *key, uint32_t keysize, void *value);

/* Searches for a given key in an hash table and retrieves the corresponding value. Returns 0 if the key was not found, > 0 otherwise. */
int hash_table_lookup (hash_table *ht, void *key, uint32_t keysize, void **value);

/* Frees the memory used by an hash table. Functions to free the key and value automatically can be provided if needed. */
void hash_table_destroy (hash_table *ht, void (*keyfreefunc) (void *), void (*valuefreefunc) (void *));

