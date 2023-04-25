/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "../jit/netvmjitglobals.h"
#include "lists.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file hashtbl.h
* \brief This file contains the definition and prototypes to use the has tables.
*/

//! seeking operation or add operation
#define HASH_TEST_ONLY	FALSE
#define HASH_ADD_ITEM	TRUE


typedef struct _TABLE_ENTRY *PIntHashTblEntry;

//! generic entry for an entry containing an integer values
typedef struct _TABLE_ENTRY
{
	uint32_t		Key;
	uint32_t		Value;
	PIntHashTblEntry	Next;
}IntHashTblEntry; 


//! has table definition for integer values
typedef struct _HASH_TABLE
{
	uint32_t		Size;
	PIntHashTblEntry	*Table;
}IntHashTbl, *PIntHashTbl;

/*!
  \brief Hash function

  \param key the number to be "hashed"

  \param size the size of the table

  \return the key
*/
uint32_t IntHashFunct(uint32_t key, uint32_t size);

/*!
  \brief Hash table creation

  \param size the initial size of the table

  \return a pointer to the newly created table
*/
PIntHashTbl New_Int_HTbl(uint32_t	size);

/*!
  \brief Hash table distruction

  \return nothing
*/
void Free_Int_HTbl(PIntHashTbl hashTable);

/*!
  \brief Insertion in the table or seek of a value

  \param hashTable the table to search in

  \param key the key used to be searched or the key that the new element will have

  \param value the value of the element (useless in case of seeking)

  \param addItem specify the operation: seeking (HASH_TEST_ONLY) or insetion (HASH_ADD_ITEM)

  \return a pointer to the entry of the table or NULL if the key is not found
*/
PIntHashTblEntry Int_HTbl_LookUp(PIntHashTbl hashTable, uint32_t key, uint32_t value, uint32_t addItem);

typedef struct _TABLE_ENTRY_GEN *PGenHashTblEntry;


typedef struct _TABLE_ENTRY_GEN
{
	uint32_t			Key;
	void				*Value;
	PGenHashTblEntry	Next;
}genHashTblEntry; 



typedef struct _GEN_HASH_TABLE
{
	uint32_t			Size;
	PGenHashTblEntry	*Table;
}GenHashTbl, *PGenHashTbl;


PGenHashTbl New_Gen_HTbl(uint32_t	size);
void Free_Gen_HTbl(PGenHashTbl hashTable);
PGenHashTblEntry Gen_HTbl_LookUp(PGenHashTbl hashTable, uint32_t key, void *value, uint32_t addItem);

uint32_t StringToIntHash(char *str);

#ifdef __cplusplus
}
#endif

