/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




/** @file hashtbl.c
 * \brief This file contains the functions that implement a Hash Table structure
 *       
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hashtbl.h"
#include "../../nbee/globals/debug.h"


uint32_t IntHashFunct(uint32_t key, uint32_t size)
{
double hash, i, f, c;

	c = (sqrt(5) - 1) / 2;
	f = modf(key * c, &i);
	hash = size *  f;

#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Generated integer hash %u for key %",
			 (uint32_t)hash, key);
#endif

	return ((uint32_t)hash);
}


PIntHashTbl New_Int_HTbl(uint32_t	size)
{
PIntHashTbl newHashTable;

	if (size == 0)
		return NULL;
	
	newHashTable = CALLOC(1, sizeof(IntHashTbl));
	
	if (newHashTable == NULL)
		return NULL;

	newHashTable->Table = CALLOC(size, sizeof(PIntHashTblEntry));
	if (newHashTable->Table == NULL)
	{
		free(newHashTable);
		return NULL;
	}

	newHashTable->Size = size;
	return newHashTable;
}


void Free_Int_HTbl(PIntHashTbl hashTable)
{
	uint32_t i;
	PIntHashTblEntry currItem, nextItem;

	if (hashTable == NULL)
		return;

	for (i = 0; i< hashTable->Size; i++)
	{
		currItem = hashTable->Table[i];
		while (currItem != NULL)
		{
			nextItem = currItem->Next;
			free(currItem);
			currItem = nextItem;
		}
	}
	free(hashTable->Table);
	free(hashTable);
}



PIntHashTblEntry Int_HTbl_LookUp(PIntHashTbl hashTable, uint32_t key, uint32_t value, uint32_t addItem)
{
    uint32_t index, depth = 0;
	PIntHashTblEntry currItem;

	NETVM_ASSERT((hashTable != NULL), "Null Args");

	index = IntHashFunct(key, hashTable->Size);
	currItem = hashTable->Table[index];
	while (currItem != NULL)
	{
		if (currItem->Key == key)
		{
			if(addItem)
				currItem->Value = value;
			return currItem;
		}
		currItem = currItem->Next;
		depth++;
	}
	
#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Key %u, Hash %u for key %",
			 key, index, depth);
#endif

	if (addItem)
	{
		currItem = (PIntHashTblEntry)CALLOC(1, sizeof(IntHashTblEntry));
		if (currItem == NULL)
			return NULL;
		currItem->Key = key;
		currItem->Value = value;
		currItem->Next = hashTable->Table[index];
		hashTable->Table[index] = currItem;

	}
	return currItem;

}

uint32_t StringToIntHash(char *str)
{
	uint32_t hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

