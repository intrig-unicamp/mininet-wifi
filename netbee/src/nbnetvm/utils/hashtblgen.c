/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file hashtblgen.c
 * \brief This file contains the functions that implement a Hash Table structure with a general (void*) item
 *       
 */

#include <stdio.h>
#include <stdlib.h>
/* flavio "math" -> <math>  link con -lm*/
#include <math.h>
#include "../jit/netvmjitglobals.h"
#include "hashtbl.h"
#include "../../nbee/globals/debug.h"


PGenHashTbl New_Gen_HTbl(uint32_t	size)
{
	PGenHashTbl newHashTable;
	if (size == 0)
		return NULL;
	newHashTable = CALLOC(1, sizeof(GenHashTbl));
	if (newHashTable == NULL)
		return NULL;

	newHashTable->Table = CALLOC(size, sizeof(PGenHashTblEntry));
	if (newHashTable->Table == NULL){
		free(newHashTable);
		return NULL;
	}

	newHashTable->Size = size;
	return newHashTable;
}


void Free_Gen_HTbl(PGenHashTbl hashTable)
{
	uint32_t i;
	PGenHashTblEntry currItem, nextItem;

	if (hashTable == NULL)
		return;

	for (i = 0; i< hashTable->Size; i++){
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



PGenHashTblEntry Gen_HTbl_LookUp(PGenHashTbl hashTable, uint32_t key, void *value, uint32_t addItem)
{
    uint32_t index, depth = 0;
	PGenHashTblEntry currItem;

	NETVM_ASSERT((hashTable != NULL), "Null Args");

	index = IntHashFunct(key, hashTable->Size);
	currItem = hashTable->Table[index];
	while (currItem != NULL){
		if (currItem->Key == key)
			return currItem;
		currItem = currItem->Next;
		depth++;
	}
	
#ifdef ENABLE_NETVM_LOGGING
	logdata(LOG_JIT_BUILD_BLOCK_LVL2, "Key %u, Hash %u for key %",
			 key, index, depth);
#endif

	if (addItem){
		currItem = (PGenHashTblEntry)CALLOC(1, sizeof(genHashTblEntry));
		if (currItem == NULL)
			return NULL;
		currItem->Key = key;
		currItem->Value = value;
		currItem->Next = hashTable->Table[index];
		hashTable->Table[index] = currItem;

	}
	return currItem;

}

