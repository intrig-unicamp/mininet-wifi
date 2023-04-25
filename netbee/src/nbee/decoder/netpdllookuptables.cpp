/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>		// for memcpy()

#include <nbprotodb_defs.h>
#include <nbprotodb.h>

#include "../utils/netpdlutils.h"
#include "netpdllookuptables.h"
#include "../globals/globals.h"
#include "../globals/debug.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
//#define SERVICE_AGE 1


/*!
	\brief Default constructor

	\param ErrBuf: pointer to a user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.
*/
CNetPDLLookupTables::CNetPDLLookupTables(char *ErrBuf, int ErrBufSize)
{
	// Store internally the pointer to the error buffer. This buffer belongs to the class that creates this one.
	m_errbuf= ErrBuf;
	m_errbufSize= ErrBufSize;
	
	m_currNumTables= 0;
	lookupExactCall = 0;
}


extern struct _nbNetPDLDatabase *NetPDLDatabase;

//! Default destructor
CNetPDLLookupTables::~CNetPDLLookupTables()
{
struct _TableEntry *CurrentEntry, *NextEntry;
int TableID;
int i;

	for (TableID= 0; TableID < m_currNumTables; TableID++)
	{

#ifdef DEBUG_LOOKUPTABLE
		PrintTableStats(TableID);
#endif

		free(m_tableList[TableID].Name);

		for (i= 0; i< m_tableList[TableID].NumberOfKeys; i++)
			free(m_tableList[TableID].KeyList[i].Name);

		free(m_tableList[TableID].KeyList);
		free(m_tableList[TableID].ExportedKeyList);

		for (i= 0; i< m_tableList[TableID].NumberOfData; i++)
			free(m_tableList[TableID].DataList[i].Name);

		free(m_tableList[TableID].DataList);
		free(m_tableList[TableID].ExportedDataList);

		free(m_tableList[TableID].KeyForCompare);

		CurrentEntry= m_tableList[TableID].FirstExactEntry;
		while(CurrentEntry)
		{
			NextEntry= CurrentEntry->NextEntry;
			free(CurrentEntry);
			CurrentEntry= NextEntry;
		}
		CurrentEntry= m_tableList[TableID].FirstVoidExactEntry;
		while(CurrentEntry)
		{
			NextEntry= CurrentEntry->NextEntry;
			free(CurrentEntry);
			CurrentEntry= NextEntry;
		}

		CurrentEntry= m_tableList[TableID].FirstMaskEntry;
		while(CurrentEntry)
		{
			NextEntry= CurrentEntry->NextEntry;
			free(CurrentEntry);
			CurrentEntry= NextEntry;
		}
		CurrentEntry= m_tableList[TableID].FirstVoidMaskEntry;
		while(CurrentEntry)
		{
			NextEntry= CurrentEntry->NextEntry;
			free(CurrentEntry);
			CurrentEntry= NextEntry;
		}
	}
}



// Documented in base class
int CNetPDLLookupTables::CreateTable(struct _nbNetPDLElementLookupTable* LookupTable)
{
int TotalEntrySize;
int TableID;
int i, RetVal;
struct _nbNetPDLElementKeyData* TempKeyData;

	TableID= m_currNumTables;
	m_currNumTables++;

	if (m_currNumTables > MAX_NUM_LOOKUP_TABLES)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: not enough memory for creating another lookup table.");
		return nbFAILURE;
	}

	memset(&m_tableList[TableID], 0, sizeof(struct _TableInfo));

	if ((m_tableList[TableID].Name= strdup(LookupTable->Name)) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
		return nbFAILURE;
	}

	m_tableList[TableID].NumberOfExactEntries= LookupTable->NumExactEntries;
	m_tableList[TableID].NumberOfMaskEntries= LookupTable->NumMaskEntries;
	m_tableList[TableID].AllowDynamicEntries= LookupTable->AllowDynamicEntries;

	// Get the number of keys and data that we have to create
	TempKeyData= LookupTable->FirstKey;
	while (TempKeyData)
	{
		m_tableList[TableID].NumberOfKeys++;
		m_tableList[TableID].KeysEntrySize+= TempKeyData->Size;
		TempKeyData= TempKeyData->NextKeyData;
	}
	TempKeyData= LookupTable->FirstData;
	while (TempKeyData)
	{
		m_tableList[TableID].NumberOfData++;
		m_tableList[TableID].DataEntrySize+= TempKeyData->Size;
		TempKeyData= TempKeyData->NextKeyData;
	}

	// Allocate space for keys
	m_tableList[TableID].KeyList= (struct _DataKeyEntry*) malloc (sizeof(struct _DataKeyEntry) * m_tableList[TableID].NumberOfKeys);
	if (m_tableList[TableID].KeyList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
		return nbFAILURE;
	}

	m_tableList[TableID].ExportedKeyList= (struct _nbLookupTableKey*) malloc (sizeof(struct _nbLookupTableKey) * m_tableList[TableID].NumberOfKeys);
	if (m_tableList[TableID].ExportedKeyList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
		return nbFAILURE;
	}

	
	// Copy values in the field table
	TempKeyData= LookupTable->FirstKey;
	for (i= 0; i < m_tableList[TableID].NumberOfKeys; i++)
	{
		m_tableList[TableID].KeyList[i].Name= strdup(TempKeyData->Name);
		if (m_tableList[TableID].KeyList[i].Name == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
			return nbFAILURE;
		}

		m_tableList[TableID].KeyList[i].Size= TempKeyData->Size;
		m_tableList[TableID].KeyList[i].KeyDataType= TempKeyData->KeyDataType;

		m_tableList[TableID].ExportedKeyList[i].Size= TempKeyData->Size;
		m_tableList[TableID].ExportedKeyList[i].KeyType= TempKeyData->KeyDataType;

		if (i == 0)
		{
			// PLEASE WARNING: the size (in the next line) is ok with the current structure of 'struct _TableEntry'.
			// If we change this structure, we may have to change this line.
			m_tableList[TableID].KeyList[i].KeyDataOffset= sizeof(struct _TableEntry) - sizeof(void *);
			m_tableList[TableID].KeyList[i].MaskOffset= m_tableList[TableID].KeyList[i].KeyDataOffset + m_tableList[TableID].KeysEntrySize + m_tableList[TableID].DataEntrySize;
		}
		else
		{
			m_tableList[TableID].KeyList[i].KeyDataOffset= m_tableList[TableID].KeyList[i-1].KeyDataOffset + m_tableList[TableID].KeyList[i-1].Size;
			m_tableList[TableID].KeyList[i].MaskOffset= m_tableList[TableID].KeyList[i-1].MaskOffset + m_tableList[TableID].KeyList[i-1].Size;
		}

		TempKeyData= TempKeyData->NextKeyData;
	}

	// Allocate space for custom data
	m_tableList[TableID].DataList= (struct _DataKeyEntry*) malloc (sizeof(struct _DataKeyEntry) * m_tableList[TableID].NumberOfData);
	if (m_tableList[TableID].DataList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
		return nbFAILURE;
	}

	m_tableList[TableID].ExportedDataList= (struct _nbLookupTableData*) malloc (sizeof(struct _nbLookupTableData) * m_tableList[TableID].NumberOfData);
	if (m_tableList[TableID].ExportedDataList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
		return nbFAILURE;
	}

	// Copy values in the field table
	TempKeyData= LookupTable->FirstData;
	for (i= 0; i < m_tableList[TableID].NumberOfData; i++)
	{
		m_tableList[TableID].DataList[i].Name= strdup(TempKeyData->Name);
		if (m_tableList[TableID].DataList[i].Name == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
			return nbFAILURE;
		}

		m_tableList[TableID].DataList[i].Size= TempKeyData->Size;
		m_tableList[TableID].DataList[i].KeyDataType= TempKeyData->KeyDataType;

		m_tableList[TableID].ExportedDataList[i].Size= TempKeyData->Size;
		m_tableList[TableID].ExportedDataList[i].DataType= TempKeyData->KeyDataType;

		if (i == 0)
		{
			m_tableList[TableID].DataList[i].KeyDataOffset= m_tableList[TableID].KeyList[m_tableList[TableID].NumberOfKeys - 1].KeyDataOffset + m_tableList[TableID].KeyList[m_tableList[TableID].NumberOfKeys - 1].Size;
			// We do not update the MaskOffset (we set it to -1), since this value is not needed in case of data; it is useful only for keys
			m_tableList[TableID].DataList[i].MaskOffset= -1;
		}
		else
		{
			m_tableList[TableID].DataList[i].KeyDataOffset= m_tableList[TableID].DataList[i-1].KeyDataOffset + m_tableList[TableID].DataList[i-1].Size;
			// We do not update the MaskOffset (we set it to -1), since this value is not needed in case of data; it is useful only for keys
			m_tableList[TableID].DataList[i].MaskOffset= -1;
		}

		TempKeyData= TempKeyData->NextKeyData;
	}


	// Let's allocate room for the key (this is needed in order to speed up the lookup process)
	m_tableList[TableID].KeyForCompare= (void*) malloc(m_tableList[TableID].KeysEntrySize);
	if (m_tableList[TableID].KeyForCompare == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
		return nbFAILURE;
	}

	// Get the size of each exact entry in the lookup table...
	TotalEntrySize= m_tableList[TableID].KeyList[0].KeyDataOffset + m_tableList[TableID].KeysEntrySize + m_tableList[TableID].DataEntrySize;

	// Let's allocate space for dynamic exact entries
	RetVal= AllocateEntries(m_tableList[TableID].NumberOfExactEntries, TotalEntrySize, &(m_tableList[TableID].FirstVoidExactEntry));

	if (RetVal == nbFAILURE)
		return nbFAILURE;

	// Get the size of each masked entry in the lookup table...
	TotalEntrySize= m_tableList[TableID].KeyList[0].KeyDataOffset + m_tableList[TableID].KeysEntrySize * 2 + m_tableList[TableID].DataEntrySize;

	// Let's allocate space for dynamic masked entries
	RetVal= AllocateEntries(m_tableList[TableID].NumberOfMaskEntries, TotalEntrySize, &(m_tableList[TableID].FirstVoidMaskEntry));
	if (RetVal == nbFAILURE)
		return nbFAILURE;

	// Let's set timestamp and lifetime IDs
	m_tableList[TableID].TimestampFieldID= m_tableList[TableID].NumberOfData;
	m_tableList[TableID].LifetimeFieldID= m_tableList[TableID].NumberOfData + 1;


#ifdef LOOKUPTABLE_PROFILER
	m_tableList[TableID].ProfilerBinsIntervalExact= PROFILER_BINS_INTERVAL_EXACT;
	m_tableList[TableID].ProfilerBinsIntervalMask= PROFILER_BINS_INTERVAL_MASK;

	m_tableList[TableID].CurrentNumberOfExactEntries= 0;
	m_tableList[TableID].CurrentNumberOfMaskEntries= 0;

	m_tableList[TableID].MaxNumberOfExactEntries= 0;
	m_tableList[TableID].MaxNumberOfMaskEntries= 0;

	for (i= 0; i < PROFILER_NBINS_EXACT; i++)
		m_tableList[TableID].ProfilerExactEntries[i]= 0;

	for (i= 0; i < PROFILER_NBINS_MASK; i++)
		m_tableList[TableID].ProfilerMaskedEntries[i]= 0;

	m_tableList[TableID].TotalNumberInsertionExact= 0;
	m_tableList[TableID].TotalNumberInsertionMask= 0;
#endif

	return nbSUCCESS;
}


// Documented in base class
int CNetPDLLookupTables::GetTableID(char* TableName, char *DataName, int *TableID, int *DataID)
{
int TableNumber, DataNumber;

	for (TableNumber= 0; TableNumber < m_currNumTables; TableNumber++)
	{
		if (strcmp(TableName, m_tableList[TableNumber].Name) == 0)
		{
			*TableID= TableNumber;

			// In case we're not interested in the field name, let's return now
			if (DataName == NULL)
				return nbSUCCESS;

			for (DataNumber= 0; DataNumber < m_tableList[TableNumber].NumberOfData; DataNumber++)
			{
				if (strcmp(DataName, m_tableList[TableNumber].DataList[DataNumber].Name) == 0)
				{
					*DataID= DataNumber;
					return nbSUCCESS;
				}
			}

			if (strcmp(DataName, NETPDL_LOOKUPTABLE_FIELDNAME_TIMESTAMP) == 0)
			{
				*DataID= m_tableList[TableNumber].TimestampFieldID;
				return nbSUCCESS;
			}
			if (strcmp(DataName, NETPDL_LOOKUPTABLE_FIELDNAME_LIFETIME) == 0)
			{
				*DataID= m_tableList[TableNumber].LifetimeFieldID;
				return nbSUCCESS;
			}

			// Table matches, but field name doesn't
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
									"Data field named '%s' cannot be found in lookup table '%s'.", DataName, TableName);
			return nbFAILURE;

		}
	}

	// Table name doesn't match
	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
							"Lookup table '%s' cannot be found in NetPDL description.", TableName);
	return nbFAILURE;
}



// Documented in base class
int CNetPDLLookupTables::AddTableEntry(int TableID, struct _nbLookupTableKey KeyList[], struct _nbLookupTableData DataList[], int TimestampSec, 
										int KeysHaveMasks, nbNetPDLUpdateLookupTableAddValidityTypes_t 	Validity, int KeepTime, int HitTime, int NewHitTime)
{
int i;

#ifdef _DEBUG
	if (TableID >= m_currNumTables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Requested an invalid lookup table.");
		return nbFAILURE;
	}
#endif

	if ((KeysHaveMasks) && (m_tableList[TableID].FirstVoidMaskEntry == NULL))
	{
	int TotalEntrySize, RetVal;

		// Get the size of each masked entry in the lookup table...
		TotalEntrySize= m_tableList[TableID].KeyList[0].KeyDataOffset + m_tableList[TableID].KeysEntrySize * 2 + m_tableList[TableID].DataEntrySize;

		RetVal= AllocateEntries(m_tableList[TableID].NumberOfMaskEntries, TotalEntrySize, &(m_tableList[TableID].FirstVoidMaskEntry));

		if (RetVal == nbFAILURE)
			return nbFAILURE;

		m_tableList[TableID].NumberOfMaskEntries= m_tableList[TableID].NumberOfMaskEntries * 2;

#ifdef DEBUG_LOOKUPTABLE
		fprintf(stderr, "Enlarged the 'masked' section of session table '%s', which is now %d.", m_tableList[TableID].Name, m_tableList[TableID].NumberOfMaskEntries);
		PrintEntireTable(TableID, 0, TimestampSec);
#endif
	}

	if ((!KeysHaveMasks) && (m_tableList[TableID].FirstVoidExactEntry == NULL))
	{
	int TotalEntrySize, RetVal;

		// Get the size of each exact entry in the lookup table...
		TotalEntrySize= m_tableList[TableID].KeyList[0].KeyDataOffset + m_tableList[TableID].KeysEntrySize + m_tableList[TableID].DataEntrySize;

		RetVal= AllocateEntries(m_tableList[TableID].NumberOfExactEntries, TotalEntrySize, &(m_tableList[TableID].FirstVoidExactEntry));

		if (RetVal == nbFAILURE)
			return nbFAILURE;

		m_tableList[TableID].NumberOfExactEntries= m_tableList[TableID].NumberOfExactEntries * 2;

#ifdef DEBUG_LOOKUPTABLE
		fprintf(stderr, "Enlarged the 'exact' section of session table '%s', which is now %d.", m_tableList[TableID].Name, m_tableList[TableID].NumberOfExactEntries);
		PrintEntireTable(TableID, 1, TimestampSec);
#endif
	}

	// Update pointers: move the first item which is in the 'empty' chain into the 'used' chain
	if (KeysHaveMasks)
		MoveNewMaskEntryOnTopOfList(TableID);
	else
		MoveNewExactEntryOnTopOfList(TableID);

	// Done with pointers. Now, let's copy keys and data in the new structure.
	for (i= 0; i < m_tableList[TableID].NumberOfKeys; i++)
	{
	char* FieldStartingPtr;
	char* FieldMaskStartingPtr = 0;
	char* PaddingStartingPtr;
	int PaddingSize;

		PaddingSize= m_tableList[TableID].KeyList[i].Size - KeyList[i].Size;

		if (KeysHaveMasks)
		{
			FieldStartingPtr= ((char*) m_tableList[TableID].FirstMaskEntry) + m_tableList[TableID].KeyList[i].KeyDataOffset;
			FieldMaskStartingPtr= ((char*) m_tableList[TableID].FirstMaskEntry) + m_tableList[TableID].KeyList[i].MaskOffset;
		}
		else
			FieldStartingPtr= ((char*) m_tableList[TableID].FirstExactEntry) + m_tableList[TableID].KeyList[i].KeyDataOffset;

		switch (KeyList[i].KeyType)
		{
			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
			{
				memcpy(FieldStartingPtr, &(KeyList[i].Value), KeyList[i].Size);
				if (KeysHaveMasks)
				{
					if (KeyList[i].Mask)
						memcpy(FieldMaskStartingPtr, &(KeyList[i].Mask), KeyList[i].Size);
					else
						memset(FieldMaskStartingPtr, '\xFF', KeyList[i].Size);
				}
				break;
			}

			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
			{
				memcpy(FieldStartingPtr, KeyList[i].Value, KeyList[i].Size);
				if (KeysHaveMasks)
				{
					if (KeyList[i].Mask)
						memcpy(FieldMaskStartingPtr, KeyList[i].Mask, KeyList[i].Size);
					else
						memset(FieldMaskStartingPtr, '\xFF', KeyList[i].Size);
				}
				break;
			}

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error.");
				return nbFAILURE;
			}
		}

		// If the current key is smaller than the space allocated to it
		if (PaddingSize)
		{
			PaddingStartingPtr= FieldStartingPtr + KeyList[i].Size;
			memset(PaddingStartingPtr, 0, PaddingSize);

			if (KeysHaveMasks)
			{
				PaddingStartingPtr= FieldMaskStartingPtr + KeyList[i].Size;
				memset(PaddingStartingPtr, '\xFF', PaddingSize);
			}
		}
	}

	// ... and data
	for (i= 0; i < m_tableList[TableID].NumberOfData; i++)
	{
	char* DataStartingPtr;
	char* PaddingStartingPtr;
	int PaddingSize;

		PaddingSize= m_tableList[TableID].DataList[i].Size - DataList[i].Size;

		if (KeysHaveMasks)
			DataStartingPtr= ((char*) m_tableList[TableID].FirstMaskEntry) + m_tableList[TableID].DataList[i].KeyDataOffset;
		else
			DataStartingPtr= ((char*) m_tableList[TableID].FirstExactEntry) + m_tableList[TableID].DataList[i].KeyDataOffset;

		switch (DataList[i].DataType)
		{
			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
			{
				memcpy(DataStartingPtr, &(DataList[i].Value), DataList[i].Size);
				break;
			}

			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
			{
				memcpy(DataStartingPtr, DataList[i].Value, DataList[i].Size);
				break;
			}

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error.");
				return nbFAILURE;
			}
		}

		if (PaddingSize)
		{
			PaddingStartingPtr= DataStartingPtr + DataList[i].Size;
			memset(PaddingStartingPtr, 0, PaddingSize);
		}
	}

	// Update timestamp and lifetime
	if (KeysHaveMasks)
	{
		m_tableList[TableID].FirstMaskEntry->Timestamp= TimestampSec;
		m_tableList[TableID].FirstMaskEntry->Lifetime= KeepTime;

		m_tableList[TableID].FirstMaskEntry->ValidityType= Validity;
		m_tableList[TableID].FirstMaskEntry->KeepTime= KeepTime;
		m_tableList[TableID].FirstMaskEntry->HitTime= HitTime;
		m_tableList[TableID].FirstMaskEntry->NewHitTime= NewHitTime;
	}
	else
	{
		m_tableList[TableID].FirstExactEntry->Timestamp= TimestampSec;
		m_tableList[TableID].FirstExactEntry->Lifetime= KeepTime;

		m_tableList[TableID].FirstExactEntry->ValidityType= Validity;
		m_tableList[TableID].FirstExactEntry->KeepTime= KeepTime;
		m_tableList[TableID].FirstExactEntry->HitTime= HitTime;
		m_tableList[TableID].FirstExactEntry->NewHitTime= NewHitTime;
		
		//We also add an element in the MapExactEntry of this Table
		HashKey key(((char*) m_tableList[TableID].FirstExactEntry) + m_tableList[TableID].KeyList[0].KeyDataOffset, m_tableList[TableID].KeysEntrySize);
		MapExact[TableID][key]= m_tableList[TableID].FirstExactEntry;
	}

	


#ifdef LOOKUPTABLE_PROFILER
	UpdateProfilerCounters(TableID, !KeysHaveMasks, +1);
	
#ifdef SERVICE_AGE
	int ServiceTableID;
	int ok=0;
	char *table;
			
	GetTableID("$KnownServerTable", NULL, &ServiceTableID, NULL);
	if(ServiceTableID == TableID){
		ok=1;
		table="TCP";
	}
			
	GetTableID("$KnownUDPServerTable", NULL, &ServiceTableID, NULL);
	if(ServiceTableID == TableID){
		ok=1;
		table="UDP";
	}
			
	GetTableID("$SkypeClientTable", NULL, &ServiceTableID, NULL);
	if(ServiceTableID == TableID){
		ok=1;
		table="SKYPE";
	}
	
			
	if(ok){
		
		uint32_t addr;
		memcpy(&addr, KeyList[0].Value, KeyList[0].Size);
		char str[50];
		inet_ntop(AF_INET, &addr, str, sizeof(str));
		
		uint16_t port;
		memcpy(&port, KeyList[1].Value, KeyList[1].Size);
		port = ntohs(port);
		
		int proto;
		memcpy(&proto, &(DataList[0].Value), DataList[0].Size);
		
		printf("Added service %d %s %u %s %s\n",TimestampSec, str, port, NetPDLDatabase->ProtoList[proto]->Name, table);
	}
#endif

#endif

#ifdef DEBUG_LOOKUPTABLE
	if (KeysHaveMasks)
		PrintTableEntry(TableID, m_tableList[TableID].FirstMaskEntry, "Lookup table Mask ADD");
	else
		PrintTableEntry(TableID, m_tableList[TableID].FirstExactEntry, "Lookup table Exact ADD");
#endif

	// Done. Let's return, now.
	return nbSUCCESS;
}



// Documented in base class
int CNetPDLLookupTables::LookupForTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int TimestampSec, int MatchExactEntries, int MatchMaskEntries, int GetFirstMatch)
{
int i;
struct _TableEntry *CurrentEntry, *NextEntry;

	if (TableID >= m_currNumTables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Requested an invalid lookup table.");
		return nbFAILURE;
	}

	if (GetFirstMatch)
	{
		// Let's copy keys...
		for (i= 0; i < m_tableList[TableID].NumberOfKeys; i++)
		{
		char* KeyStartingPtr;
		char* PaddingStartingPtr;
		int PaddingSize;

			PaddingSize= m_tableList[TableID].KeyList[i].Size - KeyList[i].Size;

			KeyStartingPtr= ((char*) m_tableList[TableID].KeyForCompare) + m_tableList[TableID].KeyList[i].KeyDataOffset - m_tableList[TableID].KeyList[0].KeyDataOffset;

			switch (KeyList[i].KeyType)
			{
				case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
				case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
				{
					memcpy(KeyStartingPtr, &(KeyList[i].Value), KeyList[i].Size);
					break;
				}

				case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
				{
					memcpy(KeyStartingPtr, KeyList[i].Value, KeyList[i].Size);
					break;
				}

				default:
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error.");
					return nbFAILURE;
				}
			}

			if (PaddingSize)
			{
				PaddingStartingPtr= KeyStartingPtr + KeyList[i].Size;
				memset(PaddingStartingPtr, 0, PaddingSize);
			}
		}
	}

	if (MatchExactEntries)
	{
		// After MAX_CALL_BEFORE_GARBAGE we perform a garbage collection
		lookupExactCall++;
		if(lookupExactCall > MAX_CALL_BEFORE_GARBAGE)
		{
			lookupExactCall=0;
			DoGarbageCollection(TimestampSec, 0);
		}


		// Let's start a linear search in the 'exact' section of the table
		if (GetFirstMatch)
			CurrentEntry= m_tableList[TableID].FirstExactEntry;
		else
			CurrentEntry= m_tableList[TableID].MatchingEntry->NextEntry;

		
		HashKey key((char*) m_tableList[TableID].KeyForCompare, m_tableList[TableID].KeysEntrySize);
		std::map<HashKey, _TableEntry*>::iterator it;
		
		//This perform a O(n log n) search
		it = MapExact[TableID].find(key);
	
		//No match For the entry
		if(it == MapExact[TableID].end())
		{
			CurrentEntry = NULL;
			return nbWARNING;
		}
		
		//We get the pointer to the _TableEntry structure
		CurrentEntry = it->second;

		//If we have dynamic entry, we should check validity of entry found
		if (m_tableList[TableID].AllowDynamicEntries)
		{
				if (CheckAndDeleteIfOldExactEntry(TableID, CurrentEntry, TimestampSec) == nbSUCCESS)
				{
					CurrentEntry = NULL;
					return nbWARNING;
				}
				else
				{
					m_tableList[TableID].MatchingEntryIsExact= 1;
					m_tableList[TableID].MatchingEntry= CurrentEntry;
					return nbSUCCESS;
				}
		}
		else
		{
			m_tableList[TableID].MatchingEntryIsExact= 1;
			m_tableList[TableID].MatchingEntry= CurrentEntry;
			return nbSUCCESS;
		}
	}

	if (MatchMaskEntries)
	{
		// Let's start a linear search in the 'masked' section of the table
		if (GetFirstMatch)
			CurrentEntry= m_tableList[TableID].FirstMaskEntry;
		else
			CurrentEntry= m_tableList[TableID].MatchingEntry->NextEntry;

		while (CurrentEntry)
		{
		int OffsetToCompare;
		char* KeyValue;
		char* EntryValue;
		char* MaskValue;
		int MatchFound= 1;

			NextEntry= CurrentEntry->NextEntry;

			if (CheckAndDeleteIfOldMaskEntry(TableID, CurrentEntry, TimestampSec) == nbSUCCESS)
			{
				// The current entry was too old and it has been deleted
				CurrentEntry= NextEntry;
				continue;
			}

			// Please note that we have to cast CurrentEntry into a char*, otherwise 'CurrentEntry + 1' 
			// means "CurrentEntry + the size of one struct _TableEntry"
			KeyValue= ((char*) m_tableList[TableID].KeyForCompare);
			EntryValue= ((char *) CurrentEntry) + m_tableList[TableID].KeyList[0].KeyDataOffset;
			MaskValue= ((char *) CurrentEntry) + m_tableList[TableID].KeyList[0].MaskOffset;

			for (OffsetToCompare= 0; OffsetToCompare < m_tableList[TableID].KeysEntrySize; OffsetToCompare++)
			{
			char Mask;

				Mask= *(MaskValue + OffsetToCompare);
				if ((*(KeyValue + OffsetToCompare) & Mask)  != (*(EntryValue + OffsetToCompare) & Mask))
				{
					MatchFound= 0;
					break;
				}
			}
			
			if (MatchFound)
			{
				m_tableList[TableID].MatchingEntryIsExact= 0;
				m_tableList[TableID].MatchingEntry= CurrentEntry;
				return nbSUCCESS;
			}

			CurrentEntry= CurrentEntry->NextEntry;
		}
	}

	return nbWARNING;
}


// Documented in base class
int CNetPDLLookupTables::LookupAndUpdateTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int TimestampSec)
{
int RetVal;

	RetVal= LookupForTableEntry(TableID, KeyList, TimestampSec, 1, 1, 1);

	if (RetVal != nbSUCCESS)
		return RetVal;

	// Let's move this entry on top of the list
	// In this way, the lookup table remains ordered according to the timestamp
	if (m_tableList[TableID].MatchingEntryIsExact)
		MoveExistingExactEntryOnTopOfList(TableID, m_tableList[TableID].MatchingEntry);
	else
		MoveExistingMaskEntryOnTopOfList(TableID, m_tableList[TableID].MatchingEntry);

	switch (m_tableList[TableID].MatchingEntry->ValidityType)
	{
		case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER:
		{
			return nbSUCCESS;
		}; break;

		case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPMAXTIME:
		{
			return nbSUCCESS;
		}; break;

		case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_UPDATEONHIT:
		{
			m_tableList[TableID].MatchingEntry->Timestamp= TimestampSec;
			if (m_tableList[TableID].MatchingEntry->GotHits == 0)
			{
				m_tableList[TableID].MatchingEntry->GotHits= 1;
				m_tableList[TableID].MatchingEntry->Lifetime= m_tableList[TableID].MatchingEntry->HitTime;
			}
			return nbSUCCESS;
		}; break;

		case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT:
		case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT:
		{
			if (m_tableList[TableID].MatchingEntryIsExact)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
					"Error: found an exact entry (in session table '%s') that has type 'replaceonhit' or 'addonhit'.", m_tableList[TableID].Name);
				return nbFAILURE;
			}

			if (m_tableList[TableID].FirstVoidExactEntry == NULL)
			{
			int TotalEntrySize;

				// Get the size of each exact entry in the lookup table...
				TotalEntrySize= m_tableList[TableID].KeyList[0].KeyDataOffset + m_tableList[TableID].KeysEntrySize + m_tableList[TableID].DataEntrySize;

				RetVal= AllocateEntries(m_tableList[TableID].NumberOfExactEntries, TotalEntrySize, &(m_tableList[TableID].FirstVoidExactEntry));

				if (RetVal == nbFAILURE)
					return nbFAILURE;

				m_tableList[TableID].NumberOfExactEntries= m_tableList[TableID].NumberOfExactEntries * 2;

#ifdef DEBUG_LOOKUPTABLE
				fprintf(stderr, "Enlarged the 'exact' section of session table '%s', which is now %d.", m_tableList[TableID].Name, m_tableList[TableID].NumberOfExactEntries);
				PrintEntireTable(TableID, 1, TimestampSec);
#endif
			}


			// Update pointers: move the first item which is in the 'empty' chain into the 'used' chain
			MoveNewExactEntryOnTopOfList(TableID);

			// Copy data
			m_tableList[TableID].FirstExactEntry->Timestamp= TimestampSec;

			// Set lifetime
			m_tableList[TableID].FirstExactEntry->Lifetime= m_tableList[TableID].MatchingEntry->NewHitTime;
			// Update GotHits of the new, exact item
			m_tableList[TableID].FirstExactEntry->GotHits= 1;


			m_tableList[TableID].FirstExactEntry->ValidityType= nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_UPDATEONHIT;
			m_tableList[TableID].FirstExactEntry->KeepTime= m_tableList[TableID].MatchingEntry->KeepTime;
			m_tableList[TableID].FirstExactEntry->HitTime= m_tableList[TableID].MatchingEntry->HitTime;
			m_tableList[TableID].FirstExactEntry->NewHitTime= m_tableList[TableID].MatchingEntry->NewHitTime;

			memcpy(((char*) m_tableList[TableID].FirstExactEntry) + m_tableList[TableID].KeyList[0].KeyDataOffset,
						m_tableList[TableID].KeyForCompare,	// We cannot use MatchingEntry, because the key is masked (so, it is not the real key)
						m_tableList[TableID].KeysEntrySize);
			memcpy(((char*) m_tableList[TableID].FirstExactEntry) + m_tableList[TableID].DataList[0].KeyDataOffset,
						((char*) m_tableList[TableID].MatchingEntry) + m_tableList[TableID].DataList[0].KeyDataOffset,
						m_tableList[TableID].DataEntrySize);

			if (m_tableList[TableID].MatchingEntry->ValidityType == nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT)
			{
				// Delete masked item
				return PurgeTableEntry(TableID, KeyList, 1 /* Masked Entry */, TimestampSec);
			}
			else
			{
				// Update GotHits of the old, masked item
				if (m_tableList[TableID].MatchingEntry->GotHits == 0)	// This is the old, masked item
				{
					m_tableList[TableID].MatchingEntry->GotHits= 1;
					m_tableList[TableID].MatchingEntry->Lifetime= m_tableList[TableID].MatchingEntry->HitTime;
				}

				// Update timestamp of the masked entry
				m_tableList[TableID].MatchingEntry->Timestamp= TimestampSec;
				return nbSUCCESS;
			}
		}; break;

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: code not allowed in the VALIDITY tag.");
			return nbFAILURE;
		}; break;
	}

	return nbSUCCESS;
}



// Documented in base class
int CNetPDLLookupTables::GetTableDataBuffer(int TableID, int DataID, unsigned int StartAt, unsigned int Size, unsigned char **DataValue, unsigned int *DataSize)
{
int DataOffset;

	if (m_tableList[TableID].MatchingEntry == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
				"Error: a successful lookup has to be performed for being able to return a value.");
		return nbFAILURE;
	}

	DataOffset= m_tableList[TableID].DataList[DataID].KeyDataOffset;

	*DataValue= ((unsigned char*) (m_tableList[TableID].MatchingEntry)) + DataOffset;
	*DataSize= m_tableList[TableID].DataList[DataID].Size;

	// Now, let's check if we have some offsets (i.e. we have to return only a portion of the data)
	if ((StartAt + Size) > (*DataSize))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
							"Requested an offset that does not exist within the NetPDL variable.");
		return nbFAILURE;
	}

	if (Size == 0)
		*DataSize= *DataSize - StartAt;
	else
		*DataSize= Size;

	*DataValue= *DataValue + StartAt;

	return nbSUCCESS;
}



// Documented in base class
int CNetPDLLookupTables::GetTableDataNumber(int TableID, int DataID, unsigned int* DataValue)
{
char *BufferDataValue;
int DataOffset;

	if (m_tableList[TableID].MatchingEntry == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
				"Error: a successful lookup has to be performed for being able to return a value.");
		return nbFAILURE;
	}

	if (DataID == m_tableList[TableID].TimestampFieldID)
	{
		*DataValue= m_tableList[TableID].MatchingEntry->Timestamp;
		return nbSUCCESS;
	}

	if (DataID == m_tableList[TableID].LifetimeFieldID)
	{
		*DataValue= m_tableList[TableID].MatchingEntry->Lifetime;
		return nbSUCCESS;
	}

	DataOffset= m_tableList[TableID].DataList[DataID].KeyDataOffset;

	BufferDataValue= ((char*) (m_tableList[TableID].MatchingEntry)) + DataOffset;

	// This code does not work; I'm not able to copy the integer with one single instruction
	// So, I have to rely on the memcpy, which is not really optimized, but it works
	// *DataValue = (unsigned int *) (*BufferDataValue);

	memcpy(DataValue, BufferDataValue, sizeof(unsigned int));

	return nbSUCCESS;
}



// Documented in base class
int CNetPDLLookupTables::SetTableDataBuffer(int TableID, int DataID, unsigned char *DataValue, unsigned int StartingOffset, unsigned int DataSize)
{
char* FieldStartingPtr;
char* PaddingStartingPtr;
int PaddingSize;

	if (m_tableList[TableID].MatchingEntry == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
				"Error: a successful lookup has to be performed for being able to return a value.");
		return nbFAILURE;
	}

	PaddingSize= m_tableList[TableID].DataList[DataID].Size - DataSize;

	FieldStartingPtr= ((char*) m_tableList[TableID].MatchingEntry) + m_tableList[TableID].DataList[DataID].KeyDataOffset + StartingOffset;

	memcpy(FieldStartingPtr, DataValue, DataSize);

#ifdef _DEBUG
	if ((StartingOffset + DataSize) > m_tableList[TableID].DataList[DataID].Size)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "A field member is not big enough to copy the given amount of data.");
		return nbFAILURE;
	}
#endif

	// If the current key is smaller than the space allocated to it
	if (PaddingSize)
	{
		PaddingStartingPtr= FieldStartingPtr + DataSize;
		memset(PaddingStartingPtr, 0, PaddingSize);
	}

	return nbSUCCESS;
}



// Documented in base class
int CNetPDLLookupTables::SetTableDataNumber(int TableID, int DataID, unsigned int DataValue)
{
char* FieldStartingPtr;
char* PaddingStartingPtr;
int PaddingSize;
int IntegerSize;

	if (m_tableList[TableID].MatchingEntry == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
				"Error: a successful lookup has to be performed for being able to return a value.");
		return nbFAILURE;
	}

	if (DataID == m_tableList[TableID].TimestampFieldID)
	{
		m_tableList[TableID].MatchingEntry->Timestamp= DataValue;
		return nbSUCCESS;
	}

	if (DataID == m_tableList[TableID].LifetimeFieldID)
	{
		m_tableList[TableID].MatchingEntry->Lifetime= DataValue;
		return nbSUCCESS;
	}

	IntegerSize= sizeof(unsigned int);

	PaddingSize= m_tableList[TableID].DataList[DataID].Size - IntegerSize;

	FieldStartingPtr= ((char*) m_tableList[TableID].MatchingEntry) + m_tableList[TableID].DataList[DataID].KeyDataOffset;

	memcpy(FieldStartingPtr, &DataValue, IntegerSize);

	// If the current key is smaller than the space allocated to it
	if (PaddingSize)
	{
		PaddingStartingPtr= FieldStartingPtr + IntegerSize;
		memset(PaddingStartingPtr, 0, PaddingSize);
	}

	return nbSUCCESS;
}


// Documented in base class
int CNetPDLLookupTables::PurgeTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int MaskedEntry, int TimestampSec)
{
int RetVal;

	if (TableID >= m_currNumTables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Requested an invalid lookup table.");
		return nbFAILURE;
	}

	// Check if the entry exists
	RetVal= LookupForTableEntry(TableID, KeyList, TimestampSec, !MaskedEntry, MaskedEntry, 1);

	// RetVal can be nbWARNING (i.e. the entry does not exist in the table); this is not an error
	// In case RetVal is nbFAILURE, the error message has already been set by the Lookup()
	if (RetVal == nbSUCCESS)
	{

#ifdef LOOKUPTABLE_PROFILER
		UpdateProfilerCounters(TableID, !MaskedEntry, -1);
		
#ifdef SERVICE_AGE					
		int ServiceTableID;
		int ok=0;
		char *table;
			
		GetTableID("$KnownServerTable", NULL, &ServiceTableID, NULL);
		if(ServiceTableID == TableID){
			ok=1;
			table="TCP";
		}
			
		GetTableID("$KnownUDPServerTable", NULL, &ServiceTableID, NULL);
		if(ServiceTableID == TableID){
			ok=1;
			table="UDP";
		}
			
		GetTableID("$SkypeClientTable", NULL, &ServiceTableID, NULL);
		if(ServiceTableID == TableID){
			ok=1;
			table="SKYPE";
		}
	
			
		if(ok){
	
			uint32_t addr;
			memcpy(&addr, KeyList[0].Value, KeyList[0].Size);
			char str[50];
			inet_ntop(AF_INET, &addr, str, sizeof(str));
			
			uint16_t port;
			memcpy(&port, KeyList[1].Value, KeyList[1].Size);
			port = ntohs(port);
			
			printf("Removed_purge service %d %s %u %s\n",m_tableList[TableID].MatchingEntry->Timestamp, str, port, table);
		}
#endif
#endif

#ifdef DEBUG_LOOKUPTABLE
		PrintTableEntry(TableID, m_tableList[TableID].MatchingEntry, "Lookup table PURGE");
#endif

		if (MaskedEntry)
			PurgeExistingMaskEntry(TableID, m_tableList[TableID].MatchingEntry);
		else
			PurgeExistingExactEntry(TableID, m_tableList[TableID].MatchingEntry);
	}
	return nbSUCCESS;
}



// Documented in base class
int CNetPDLLookupTables::ObsoleteTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int MaskedEntry, int TimestampSec)
{
int RetVal;

	if (TableID >= m_currNumTables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Requested an invalid lookup table.");
		return nbFAILURE;
	}

	// Check if the entry exists
	RetVal= LookupForTableEntry(TableID, KeyList, TimestampSec, !MaskedEntry, MaskedEntry, 1);

	// RetVal can be nbWARNING (i.e. the entry does not exist in the table); this is not an error
	// In case RetVal is nbFAILURE, the error message has already been set by the Lookup()
	if (RetVal == nbSUCCESS)
	{
		m_tableList[TableID].MatchingEntry->Lifetime= 300;
		m_tableList[TableID].MatchingEntry->Timestamp= TimestampSec;
		m_tableList[TableID].MatchingEntry->ValidityType= nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPMAXTIME;
	
#ifdef DEBUG_LOOKUPTABLE
		PrintTableEntry(TableID, m_tableList[TableID].MatchingEntry, "Lookup table OBSOLETE");
#endif
	}
	return nbSUCCESS;
}


int CNetPDLLookupTables::ScanTableEntries(int TableID, int ScanExactEntries, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList, void** CurrentElementHandler)
{
static struct _TableEntry* CurrentTableEntry;
int i;

	if (TableID >= m_currNumTables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Requested an invalid lookup table.");
		return nbFAILURE;
	}

	if (*CurrentElementHandler == NULL)
	{
		if (ScanExactEntries)
			CurrentTableEntry= m_tableList[TableID].FirstExactEntry;
		else
			CurrentTableEntry= m_tableList[TableID].FirstMaskEntry;

		*CurrentElementHandler= (void*) CurrentTableEntry;
	}
	else
	{
		CurrentTableEntry= CurrentTableEntry->NextEntry;
	}

	if (CurrentTableEntry == NULL)
		// We've finished the entry list
		return nbWARNING;

	for (i= 0; 	i < m_tableList[TableID].NumberOfKeys; i++)
	{
		m_tableList[TableID].ExportedKeyList[i].KeyType= m_tableList[TableID].KeyList[i].KeyDataType;
		m_tableList[TableID].ExportedKeyList[i].Size= m_tableList[TableID].KeyList[i].Size;
		m_tableList[TableID].ExportedKeyList[i].Mask= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].KeyList[i].MaskOffset;
		m_tableList[TableID].ExportedKeyList[i].Value= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].KeyList[i].KeyDataOffset;
	}

	for (i= 0; 	i < m_tableList[TableID].NumberOfData; i++)
	{
		m_tableList[TableID].ExportedDataList[i].DataType= m_tableList[TableID].DataList[i].KeyDataType;
		m_tableList[TableID].ExportedDataList[i].Size= m_tableList[TableID].DataList[i].Size;
		m_tableList[TableID].ExportedDataList[i].Value= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].DataList[i].KeyDataOffset;
	}

	*KeyList= m_tableList[TableID].ExportedKeyList;
	*DataList= m_tableList[TableID].ExportedDataList;

	return nbSUCCESS;
}


int CNetPDLLookupTables::GetTableFirstEntry(int TableID, int GetExactEntry, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList)
{
struct _TableEntry* CurrentTableEntry;
int i;

	if (TableID >= m_currNumTables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Requested an invalid lookup table.");
		return nbFAILURE;
	}

	if (GetExactEntry)
		CurrentTableEntry= m_tableList[TableID].FirstExactEntry;
	else
		CurrentTableEntry= m_tableList[TableID].FirstMaskEntry;

	for (i= 0; 	i < m_tableList[TableID].NumberOfKeys; i++)
	{
		m_tableList[TableID].ExportedKeyList[i].KeyType= m_tableList[TableID].KeyList[i].KeyDataType;
		m_tableList[TableID].ExportedKeyList[i].Size= m_tableList[TableID].KeyList[i].Size;
		m_tableList[TableID].ExportedKeyList[i].Mask= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].KeyList[i].MaskOffset;
		m_tableList[TableID].ExportedKeyList[i].Value= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].KeyList[i].KeyDataOffset;
	}

	for (i= 0; 	i < m_tableList[TableID].NumberOfData; i++)
	{
		m_tableList[TableID].ExportedDataList[i].DataType= m_tableList[TableID].DataList[i].KeyDataType;
		m_tableList[TableID].ExportedDataList[i].Size= m_tableList[TableID].DataList[i].Size;
		m_tableList[TableID].ExportedDataList[i].Value= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].DataList[i].KeyDataOffset;
	}

	*KeyList= m_tableList[TableID].ExportedKeyList;
	*DataList= m_tableList[TableID].ExportedDataList;

	return nbSUCCESS;
}


int CNetPDLLookupTables::GetTableMatchingEntry(int TableID, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList)
{
struct _TableEntry* CurrentTableEntry;
int i;

	if (TableID >= m_currNumTables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Requested an invalid lookup table.");
		return nbFAILURE;
	}

	if (m_tableList[TableID].MatchingEntry == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
				"Error: a successful lookup has to be performed for being able to return a value.");
		return nbFAILURE;
	}

	CurrentTableEntry= m_tableList[TableID].MatchingEntry;

	for (i= 0; 	i < m_tableList[TableID].NumberOfKeys; i++)
	{
		m_tableList[TableID].ExportedKeyList[i].KeyType= m_tableList[TableID].KeyList[i].KeyDataType;
		m_tableList[TableID].ExportedKeyList[i].Size= m_tableList[TableID].KeyList[i].Size;
		m_tableList[TableID].ExportedKeyList[i].Mask= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].KeyList[i].MaskOffset;
		m_tableList[TableID].ExportedKeyList[i].Value= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].KeyList[i].KeyDataOffset;
	}

	for (i= 0; 	i < m_tableList[TableID].NumberOfData; i++)
	{
		m_tableList[TableID].ExportedDataList[i].DataType= m_tableList[TableID].DataList[i].KeyDataType;
		m_tableList[TableID].ExportedDataList[i].Size= m_tableList[TableID].DataList[i].Size;
		m_tableList[TableID].ExportedDataList[i].Value= ((unsigned char*) CurrentTableEntry) + m_tableList[TableID].DataList[i].KeyDataOffset;
	}

	*KeyList= m_tableList[TableID].ExportedKeyList;
	*DataList= m_tableList[TableID].ExportedDataList;

	return nbSUCCESS;

}


int CNetPDLLookupTables::GetTableNextMatchingEntry(int TableID, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList)
{
int RetVal;
int TimestampSec;

	if (m_tableList[TableID].MatchingEntry == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "This function cannot be called for returning the first match in the lookup table.");
		return nbFAILURE;
	}

	// We use a fake timestamp here
	TimestampSec= m_tableList[TableID].MatchingEntry->Timestamp;

	RetVal= LookupForTableEntry(TableID, NULL, TimestampSec, m_tableList[TableID].MatchingEntryIsExact, !m_tableList[TableID].MatchingEntryIsExact, 0);

	if (RetVal == nbFAILURE)
		return RetVal;

	// In case we got a match in the 'masked' section, when we end with that section we have to continue
	// with the 'exact' section to see if there are some entries in there as well.
	if ((RetVal == nbWARNING) && (m_tableList[TableID].MatchingEntryIsExact == 0))
	{
		RetVal= LookupForTableEntry(TableID, NULL, TimestampSec, 1, 0, 0);

		if (RetVal != nbSUCCESS)
			return RetVal;
	}

	return (GetTableMatchingEntry(TableID, KeyList, DataList));
}


/*!
	\brief Move a new entry from the 'void' list to the top of the 'valid entries' list.

	This function is called by the Add() method in order to add a new entry in the list.
	The new entry is placed at the top of the list, because it is rather likely that 
	we want to get access to this entry in the future.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.
*/
void CNetPDLLookupTables::MoveNewExactEntryOnTopOfList(int TableID)
{
struct _TableEntry *OldFirstTableEntry;

	// Update pointers: move the first item which in the 'empty' chain into the 'used' chain
	OldFirstTableEntry= m_tableList[TableID].FirstExactEntry;

	m_tableList[TableID].FirstExactEntry= m_tableList[TableID].FirstVoidExactEntry;
	m_tableList[TableID].FirstVoidExactEntry= m_tableList[TableID].FirstVoidExactEntry->NextEntry;

	m_tableList[TableID].FirstExactEntry->NextEntry= OldFirstTableEntry;

	if (m_tableList[TableID].FirstVoidExactEntry)
		m_tableList[TableID].FirstVoidExactEntry->PreviousEntry= NULL;

	if (OldFirstTableEntry)
		OldFirstTableEntry->PreviousEntry= m_tableList[TableID].FirstExactEntry;

	// In case the entry list was void, let's update also the 'LastEntry' member
	if (OldFirstTableEntry == NULL)
		m_tableList[TableID].LastExactEntry= m_tableList[TableID].FirstExactEntry;
}


/*!
	\brief Move a new entry from the 'void' list to the top of the 'valid entries' list.

	This function is called by the Add() method in order to add a new entry in the list.
	The new entry is placed at the top of the list, because it is rather likely that 
	we want to get access to this entry in the future.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.
*/
void CNetPDLLookupTables::MoveNewMaskEntryOnTopOfList(int TableID)
{
struct _TableEntry *OldFirstTableEntry;

	// Update pointers: move the first item which in the 'empty' chain into the 'used' chain
	OldFirstTableEntry= m_tableList[TableID].FirstMaskEntry;

	m_tableList[TableID].FirstMaskEntry= m_tableList[TableID].FirstVoidMaskEntry;
	m_tableList[TableID].FirstVoidMaskEntry= m_tableList[TableID].FirstVoidMaskEntry->NextEntry;

	m_tableList[TableID].FirstMaskEntry->NextEntry= OldFirstTableEntry;

	if (m_tableList[TableID].FirstVoidMaskEntry)
		m_tableList[TableID].FirstVoidMaskEntry->PreviousEntry= NULL;

	if (OldFirstTableEntry)
		OldFirstTableEntry->PreviousEntry= m_tableList[TableID].FirstMaskEntry;

	// In case the entry list was void, let's update also the 'LastEntry' member
	if (OldFirstTableEntry == NULL)
		m_tableList[TableID].LastMaskEntry= m_tableList[TableID].FirstMaskEntry;
}


/*!
	\brief Move an existing entry at the top of the 'valid entries' list.

	This function is called by some other method (e.g. Update()) in order to move some entries 
	(that is more likely to be accessed in the future) on top of the list.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.

	\param MatchingEntry: pointer to the entry that has to be moved on top of the list.
*/
void CNetPDLLookupTables::MoveExistingExactEntryOnTopOfList(int TableID, struct _TableEntry *MatchingEntry)
{
struct _TableEntry *OldFirstTableEntry, *OldPreviousMatchingTableEntry, *OldNextMatchingTableEntry;

	// Let's check if this is really needed
	if (MatchingEntry == m_tableList[TableID].FirstExactEntry)
		return;

	// Let's extract the matching entry from its position in the chain (and let's update the chain accordingly)
	OldPreviousMatchingTableEntry= MatchingEntry->PreviousEntry;
	if (OldPreviousMatchingTableEntry)
		OldPreviousMatchingTableEntry->NextEntry= MatchingEntry->NextEntry;

	OldNextMatchingTableEntry= MatchingEntry->NextEntry;
	if (OldNextMatchingTableEntry)
		OldNextMatchingTableEntry->PreviousEntry= MatchingEntry->PreviousEntry;;

	// Move this entry to the top of the entry list, and let's push all the other members down one position
	OldFirstTableEntry= m_tableList[TableID].FirstExactEntry;

	m_tableList[TableID].FirstExactEntry= MatchingEntry;

	m_tableList[TableID].FirstExactEntry->PreviousEntry= NULL;
	m_tableList[TableID].FirstExactEntry->NextEntry= OldFirstTableEntry;

	OldFirstTableEntry->PreviousEntry= MatchingEntry;

	// In case the item that has been moved was the 'last member' of the list, let's update this value
	if (MatchingEntry == m_tableList[TableID].LastExactEntry)
		m_tableList[TableID].LastExactEntry= OldPreviousMatchingTableEntry;
}


/*!
	\brief Move an existing entry at the top of the 'valid entries' list.

	This function is called by some other method (e.g. Update()) in order to move some entries 
	(that is more likely to be accessed in the future) on top of the list.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.

	\param MatchingEntry: pointer to the entry that has to be moved on top of the list.
*/
void CNetPDLLookupTables::MoveExistingMaskEntryOnTopOfList(int TableID, struct _TableEntry *MatchingEntry)
{
struct _TableEntry *OldFirstTableEntry, *OldPreviousMatchingTableEntry, *OldNextMatchingTableEntry;

	// Let's check if this is really needed
	if (MatchingEntry == m_tableList[TableID].FirstMaskEntry)
		return;

	// Let's extract the matching entry from its position in the chain (and let's update the chain accordingly)
	OldPreviousMatchingTableEntry= MatchingEntry->PreviousEntry;
	if (OldPreviousMatchingTableEntry)
		OldPreviousMatchingTableEntry->NextEntry= MatchingEntry->NextEntry;

	OldNextMatchingTableEntry= MatchingEntry->NextEntry;
	if (OldNextMatchingTableEntry)
		OldNextMatchingTableEntry->PreviousEntry= MatchingEntry->PreviousEntry;;

	// Move this entry to the top of the entry list, and let's push all the other members down one position
	OldFirstTableEntry= m_tableList[TableID].FirstMaskEntry;

	m_tableList[TableID].FirstMaskEntry= MatchingEntry;

	m_tableList[TableID].FirstMaskEntry->PreviousEntry= NULL;
	m_tableList[TableID].FirstMaskEntry->NextEntry= OldFirstTableEntry;

	OldFirstTableEntry->PreviousEntry= MatchingEntry;

	// In case the item that has been moved was the 'last member' of the list, let's update this value
	if (MatchingEntry == m_tableList[TableID].LastMaskEntry)
		m_tableList[TableID].LastMaskEntry= OldPreviousMatchingTableEntry;
}


/*!
	\brief Move an existing entry from the 'valid entry' list to the 'void' list.

	This function is called when an entry is no longer in use, hence it is placed in
	the 'void' list for future reuse.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.

	\param MatchingEntry: pointer to the entry that has to be deleted.
*/
void CNetPDLLookupTables::PurgeExistingExactEntry(int TableID, struct _TableEntry *MatchingEntry)
{
struct _TableEntry *OldFirstVoidTableEntry;
struct _TableEntry *OldPreviousMatchingTableEntry;
struct _TableEntry *OldNextMatchingTableEntry;
	
	//We must also delete the corresponding entry in the MapExactEntry structure
	HashKey key(((char*) MatchingEntry) + m_tableList[TableID].KeyList[0].KeyDataOffset, m_tableList[TableID].KeysEntrySize);
	MapExact[TableID].erase(key);

	OldFirstVoidTableEntry= m_tableList[TableID].FirstVoidExactEntry;
	OldPreviousMatchingTableEntry= MatchingEntry->PreviousEntry;
	OldNextMatchingTableEntry= MatchingEntry->NextEntry;

	// Let's update the pointer at the top of the 'void entry' list
	m_tableList[TableID].FirstVoidExactEntry= MatchingEntry;

	m_tableList[TableID].FirstVoidExactEntry->PreviousEntry= NULL;
	m_tableList[TableID].FirstVoidExactEntry->NextEntry= OldFirstVoidTableEntry;

	// Now, let's update the links to the elements that were in between the entry that has been cancelled
	if (OldFirstVoidTableEntry)
		OldFirstVoidTableEntry->PreviousEntry= m_tableList[TableID].FirstVoidExactEntry;
	if (OldPreviousMatchingTableEntry)
		OldPreviousMatchingTableEntry->NextEntry= OldNextMatchingTableEntry;
	if (OldNextMatchingTableEntry)
		OldNextMatchingTableEntry->PreviousEntry= OldPreviousMatchingTableEntry;

	// ...and let's check if we have to update some pointers in the main structure
	if (MatchingEntry == m_tableList[TableID].FirstExactEntry)
		m_tableList[TableID].FirstExactEntry= OldNextMatchingTableEntry;

	if (MatchingEntry == m_tableList[TableID].LastExactEntry)
		m_tableList[TableID].LastExactEntry= OldPreviousMatchingTableEntry;
}


/*!
	\brief Move an existing entry from the 'valid entry' list to the 'void' list.

	This function is called when an entry is no longer in use, hence it is placed in
	the 'void' list for future reuse.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.

	\param MatchingEntry: pointer to the entry that has to be deleted.
*/
void CNetPDLLookupTables::PurgeExistingMaskEntry(int TableID, struct _TableEntry *MatchingEntry)
{
struct _TableEntry *OldFirstVoidTableEntry;
struct _TableEntry *OldPreviousMatchingTableEntry;
struct _TableEntry *OldNextMatchingTableEntry;
	
	OldFirstVoidTableEntry= m_tableList[TableID].FirstVoidMaskEntry;
	OldPreviousMatchingTableEntry= MatchingEntry->PreviousEntry;
	OldNextMatchingTableEntry= MatchingEntry->NextEntry;

	// Let's update the pointer at the top of the 'void entry' list
	m_tableList[TableID].FirstVoidMaskEntry= MatchingEntry;

	m_tableList[TableID].FirstVoidMaskEntry->PreviousEntry= NULL;
	m_tableList[TableID].FirstVoidMaskEntry->NextEntry= OldFirstVoidTableEntry;

	// Now, let's update the links to the elements that were in between the entry that has been cancelled
	if (OldFirstVoidTableEntry)
		OldFirstVoidTableEntry->PreviousEntry= m_tableList[TableID].FirstVoidMaskEntry;
	if (OldPreviousMatchingTableEntry)
		OldPreviousMatchingTableEntry->NextEntry= OldNextMatchingTableEntry;
	if (OldNextMatchingTableEntry)
		OldNextMatchingTableEntry->PreviousEntry= OldPreviousMatchingTableEntry;

	// ...and let's check if we have to update some pointers in the main structure
	if (MatchingEntry == m_tableList[TableID].FirstMaskEntry)
		m_tableList[TableID].FirstMaskEntry= OldNextMatchingTableEntry;

	if (MatchingEntry == m_tableList[TableID].LastMaskEntry)
		m_tableList[TableID].LastMaskEntry= OldPreviousMatchingTableEntry;
}


/*!
	\brief Allocate the set of record entries requested.
	\param NumberOfEntries: number of entries that have to be allocated.
	\param EntrySize: size of each entry (in bytes).
	\param FirstEntry: pointer to the variable that will contain the first allocated entry.
	This parameter is used to return back the pointer to the first allocated entry.
	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
*/
int CNetPDLLookupTables::AllocateEntries(int NumberOfEntries, int EntrySize, struct CNetPDLLookupTables::_TableEntry **FirstEntry)
{
struct _TableEntry *CurrentEntry, *PreviousEntry;
int i;

	*FirstEntry= NULL;
	CurrentEntry= NULL;
	PreviousEntry= NULL;

	for (i= 0; i < NumberOfEntries; i++)
	{
		CurrentEntry= (struct _TableEntry *) malloc(EntrySize);
		if (CurrentEntry == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
			return nbFAILURE;
		}

		memset(CurrentEntry, 0, EntrySize);

		CurrentEntry->PreviousEntry= PreviousEntry;

		if (PreviousEntry)
			PreviousEntry->NextEntry= CurrentEntry;
		else
			*FirstEntry= CurrentEntry;

		PreviousEntry= CurrentEntry;
	}

	return nbSUCCESS;
}


// Documented in base class
void CNetPDLLookupTables::DoGarbageCollection(int TimestampSec, int AggressiveScan)
{
struct _TableEntry *CurrentEntry, *PreviousEntry;
int TableID;
//int Timestamp;
//int Lifetime;
int Result;

	// This function is optimized in order not to consume too much time. This is the logic:
	// - all the entries are scanned every 10 calls to this function
	//   (variable Counter is used for that)
	// - if we have the 'AggressiveScan' flag turned on, the scan ends when the first item that is still 'alive'
	//   is encountered (entries are ordered in Last Accessed Came First order)
	// - otherwise, all entries are scanned 

	for (TableID= 0; TableID < m_currNumTables; TableID++)
	{
		if (m_tableList[TableID].AllowDynamicEntries == 0)
			continue;

		// Always clean the matching entry (if one)
		m_tableList[TableID].MatchingEntry= NULL;

		// Let's start a linear search in the 'exact' section
		CurrentEntry= m_tableList[TableID].LastExactEntry;

		while (CurrentEntry)
		{
			PreviousEntry= CurrentEntry->PreviousEntry;

			Result= CheckAndDeleteIfOldExactEntry(TableID, CurrentEntry, TimestampSec);

			if ((Result == nbWARNING) && (AggressiveScan))
				// Let's jump to the masked scan loop
				break;
			
			CurrentEntry= PreviousEntry;
		}

		// Let's start a linear search in the 'masked' section
		CurrentEntry= m_tableList[TableID].LastMaskEntry;

		while (CurrentEntry)
		{
			PreviousEntry= CurrentEntry->PreviousEntry;

			Result= CheckAndDeleteIfOldMaskEntry(TableID, CurrentEntry, TimestampSec);

			if ((Result == nbWARNING) && (AggressiveScan))
				// Let's jump over the masked scan loop
				break;
				
			CurrentEntry= PreviousEntry;
		}
	}
}


/*!
	\brief Check if the given entry has to be deleted because it is too old.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.

	\param CurrentEntry: entry that has to be checked.

	\param TimestampSec: current value of the timestamp.

	\return nbSUCCESS if the entry has been deleted, nbWARNING if the entry is still valid and
	cannot be deleted this time.
*/
int CNetPDLLookupTables::CheckAndDeleteIfOldExactEntry(int TableID, struct _TableEntry *CurrentEntry, int TimestampSec)
{
int Timestamp;
int Lifetime;

	if (CurrentEntry->ValidityType != nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER)
	{
		// Let's save the previous entry right now, because the ->PreviousEntry pointer 
		// may be invalid in case the entry has to be cancelled
		Timestamp= CurrentEntry->Timestamp;
		Lifetime= CurrentEntry->Lifetime;

		// If the entry is too old, let's purge this entry.
		// Otherwise, let's break the loop (entries are ordered by timestamp, therefore
		// if an entry is still valid, all the entries *before* this one are probably
		// valid as well
		if ((Timestamp + Lifetime) < TimestampSec)
		{

#ifdef LOOKUPTABLE_PROFILER
			UpdateProfilerCounters(TableID, 1, -1);
			
#ifdef SERVICE_AGE
			int ServiceTableID;
			int ok=0;
			char *table;
			
			GetTableID("$KnownServerTable", NULL, &ServiceTableID, NULL);
			if(ServiceTableID == TableID){
				ok=1;
				table="TCP";
			}
			
			GetTableID("$KnownUDPServerTable", NULL, &ServiceTableID, NULL);
			if(ServiceTableID == TableID){
				ok=1;
				table="UDP";
			}
			
			GetTableID("$SkypeClientTable", NULL, &ServiceTableID, NULL);
			if(ServiceTableID == TableID){
				ok=1;
				table="SKYPE";
			}
	
			
			if(ok){
				uint32_t addr;
				unsigned char *FieldStartingOffset= ((unsigned char*) CurrentEntry) + m_tableList[TableID].KeyList[0].KeyDataOffset;
				
				memcpy(&addr, FieldStartingOffset, m_tableList[TableID].KeyList[0].Size);
				char str[50];
				inet_ntop(AF_INET, &addr, str, sizeof(str));
				
				FieldStartingOffset= ((unsigned char*) CurrentEntry) + m_tableList[TableID].KeyList[1].KeyDataOffset;
				uint16_t port;
				memcpy(&port, FieldStartingOffset, m_tableList[TableID].KeyList[1].Size);
				port = ntohs(port);
				
				printf("Removed_expire service %d %s %u %s\n",CurrentEntry->Timestamp, str, port, table);
			}
#endif
#endif

#ifdef DEBUG_LOOKUPTABLE
		char Message[1024];

			sprintf(Message, "Lookup table GARBAGE (TS: %d) ", TimestampSec);
			PrintTableEntry(TableID, CurrentEntry, "Lookup table GARBAGE");
#endif

			PurgeExistingExactEntry(TableID, CurrentEntry);
			return nbSUCCESS;
		}
		else
		{
			return nbWARNING;
		}
	}

	return nbWARNING;
}


/*!
	\brief Check if the given entry has to be deleted because it is too old.

	\param TableID: ID of the table where there is the entry that has to be deleted.
	This value is returned back by the Create() method.

	\param CurrentEntry: entry that has to be checked.

	\param TimestampSec: current value of the timestamp.

	\return nbSUCCESS if the entry has been deleted, nbWARNING if the entry is still valid and
	cannot be deleted this time.
*/
int CNetPDLLookupTables::CheckAndDeleteIfOldMaskEntry(int TableID, struct _TableEntry *CurrentEntry, int TimestampSec)
{
int Timestamp;
int Lifetime;

	if (CurrentEntry->ValidityType != nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER)
	{
		Timestamp= CurrentEntry->Timestamp;
		Lifetime= CurrentEntry->Lifetime;

		// If the entry is too old, let's purge this entry.
		// Otherwise, let's break the loop (entries are ordered by timestamp, therefore
		// if an entry is still valid, all the entries *before* this one are probably
		// valid as well
		if ((Timestamp + Lifetime) < TimestampSec)
		{
#ifdef LOOKUPTABLE_PROFILER
			UpdateProfilerCounters(TableID, 0, -1);
#endif

#ifdef DEBUG_LOOKUPTABLE
		char Message[1024];

			sprintf(Message, "Lookup table GARBAGE (TS: %d) ", TimestampSec);
			PrintTableEntry(TableID, CurrentEntry, "Lookup table GARBAGE");
#endif

			PurgeExistingMaskEntry(TableID, CurrentEntry);
			return nbSUCCESS;
		}
		else
		{
			return nbWARNING;
		}
	}

	return nbWARNING;
}


// Documented in base class
struct _nbLookupTableKey* CNetPDLLookupTables::GetStructureForTableKey(int TableID)
{
	return m_tableList[TableID].ExportedKeyList;
};



// Documented in base class
struct _nbLookupTableData* CNetPDLLookupTables::GetStructureForTableData(int TableID)
{
	return m_tableList[TableID].ExportedDataList;
};



#ifdef LOOKUPTABLE_PROFILER
// Value: +1 if we need to add an item, '-1' if we're purging an item.
void CNetPDLLookupTables::UpdateProfilerCounters(int TableID, int IsExact, int Value)
{
int Bin;

	if (IsExact)
	{
		m_tableList[TableID].CurrentNumberOfExactEntries+= Value;

		if (Value == 1)		// We're adding a new item
		{
			if (m_tableList[TableID].CurrentNumberOfExactEntries > m_tableList[TableID].MaxNumberOfExactEntries)
				m_tableList[TableID].MaxNumberOfExactEntries= m_tableList[TableID].CurrentNumberOfExactEntries;

			m_tableList[TableID].TotalNumberInsertionExact++;
		}

		// Let's update bins
		Bin= m_tableList[TableID].CurrentNumberOfExactEntries / m_tableList[TableID].ProfilerBinsIntervalExact;
		if (Bin >= PROFILER_NBINS_EXACT)
		{	
			m_tableList[TableID].ProfilerBinsIntervalExact= m_tableList[TableID].ProfilerBinsIntervalExact * 5;

			for (int OldBinsIndex= 1; OldBinsIndex < PROFILER_NBINS_EXACT; OldBinsIndex++)
			{
			int NewBin;

				NewBin= OldBinsIndex / 5;

				m_tableList[TableID].ProfilerExactEntries[NewBin]+= m_tableList[TableID].ProfilerExactEntries[OldBinsIndex];
				m_tableList[TableID].ProfilerExactEntries[OldBinsIndex]= 0;
			}

			Bin= m_tableList[TableID].CurrentNumberOfExactEntries / m_tableList[TableID].ProfilerBinsIntervalExact;
		}

		// Add a new hit to the current number of hits within this bin
		m_tableList[TableID].ProfilerExactEntries[Bin]++;
	}
	else
	{
		m_tableList[TableID].CurrentNumberOfMaskEntries+= Value;

		if (Value == 1)		// We're adding a new item
		{
			if (m_tableList[TableID].CurrentNumberOfMaskEntries > m_tableList[TableID].MaxNumberOfMaskEntries)
				m_tableList[TableID].MaxNumberOfMaskEntries= m_tableList[TableID].CurrentNumberOfMaskEntries;

			m_tableList[TableID].TotalNumberInsertionMask++;
		}

		// Let's update bins
		Bin= m_tableList[TableID].CurrentNumberOfMaskEntries / m_tableList[TableID].ProfilerBinsIntervalMask;
		if (Bin >= PROFILER_NBINS_MASK)
		{	
			m_tableList[TableID].ProfilerBinsIntervalMask= m_tableList[TableID].ProfilerBinsIntervalMask * 5;

			for (int OldBinsIndex= 1; OldBinsIndex < PROFILER_NBINS_MASK; OldBinsIndex++)
			{
			int NewBin;

				NewBin= OldBinsIndex / 5;

				m_tableList[TableID].ProfilerMaskedEntries[NewBin]+= m_tableList[TableID].ProfilerMaskedEntries[OldBinsIndex];
				m_tableList[TableID].ProfilerMaskedEntries[OldBinsIndex]= 0;
			}

			Bin= m_tableList[TableID].CurrentNumberOfMaskEntries / m_tableList[TableID].ProfilerBinsIntervalMask;
		}

		// Add a new hit to the current number of hits within this bin
		m_tableList[TableID].ProfilerMaskedEntries[Bin]++;
	}
}


void CNetPDLLookupTables::PrintTableStats(int TableID)
{
int i;

	printf("\n\n======================================================================\n");
	printf("Table:\t%s\n", m_tableList[TableID].Name);
	printf("Size of each exact entry:\t%u\n", m_tableList[TableID].KeysEntrySize + m_tableList[TableID].DataEntrySize);
	printf("Size of each masked entry:\t%u\n", m_tableList[TableID].KeysEntrySize * 2 + m_tableList[TableID].DataEntrySize);
	printf("Current number of exact entries:\t%u\n", m_tableList[TableID].CurrentNumberOfExactEntries);
	printf("Current number of masked entries:\t%u\n", m_tableList[TableID].CurrentNumberOfMaskEntries);
	printf("Maximum number of exact entries used:\t%u\n", m_tableList[TableID].MaxNumberOfExactEntries);
	printf("Maximum number of masked entries used:\t%u\n", m_tableList[TableID].MaxNumberOfMaskEntries);
	printf("Total number of new exact entries:\t%u\n", m_tableList[TableID].TotalNumberInsertionExact);
	printf("Total number of new masked entries:\t%u\n", m_tableList[TableID].TotalNumberInsertionMask);
	
	printf("\n");
	printf("Exact entries distribution\n");
	printf("Bin size:\t%d\n", m_tableList[TableID].ProfilerBinsIntervalExact);
	printf("Num bins:\t%d\n\n", PROFILER_NBINS_EXACT);

	for (i= 0; i < PROFILER_NBINS_EXACT; i++)
		printf("\t%d\t%d\t%d\n", i, m_tableList[TableID].ProfilerBinsIntervalExact * (i + 1), m_tableList[TableID].ProfilerExactEntries[i]);

	printf("\n");
	printf("Mask entries distribution\n");
	printf("Bin size:\t%d\n", m_tableList[TableID].ProfilerBinsIntervalMask);
	printf("Num bins:\t%d\n\n", PROFILER_NBINS_MASK);

	for (i= 0; i < PROFILER_NBINS_MASK; i++)
		printf("\t%d\t%d\t%d\n", i, m_tableList[TableID].ProfilerBinsIntervalMask * (i + 1), m_tableList[TableID].ProfilerMaskedEntries[i]);

	printf("\n\n");
}

int CNetPDLLookupTables::GetNumberOfEntries(int TableID)
{
	return m_tableList[TableID].CurrentNumberOfExactEntries;
}
#endif

#ifndef LOOKUPTABLE_PROFILER
int CNetPDLLookupTables::GetNumberOfEntries(int TableID)
{
	printf("Error: nbee.dll must be compiled with LOOKUPTABLE define for using GetNumberOfEntries\n");
	return nbFAILURE;
}
#endif

#ifdef DEBUG_LOOKUPTABLE
void CNetPDLLookupTables::PrintEntireTable(int TableID, int IsExact, int TimestampSec)
{
struct _TableEntry *CurrentEntry;
int i;

	if (IsExact)
		CurrentEntry= m_tableList[TableID].FirstExactEntry;
	else
		CurrentEntry= m_tableList[TableID].FirstMaskEntry;

	i= 1;
	printf("\n\n");
	while (CurrentEntry)
	{
	unsigned char* FieldStartingOffset;

		printf("%d) %d/%d (%d-%d)- ", i, TimestampSec, CurrentEntry->Timestamp + CurrentEntry->Lifetime, CurrentEntry->Timestamp, CurrentEntry->Lifetime);

		FieldStartingOffset= ((unsigned char*) CurrentEntry) + m_tableList[TableID].KeyList[0].KeyDataOffset;

		printf("%d.%d.%d.%d => ", *FieldStartingOffset, *(FieldStartingOffset+1), *(FieldStartingOffset+2), *(FieldStartingOffset+3));

		FieldStartingOffset+= m_tableList[TableID].KeyList[0].Size;
		printf("%d.%d.%d.%d, ", *FieldStartingOffset, *(FieldStartingOffset+1), *(FieldStartingOffset+2), *(FieldStartingOffset+3));

		FieldStartingOffset+= m_tableList[TableID].KeyList[1].Size;
		printf("%d => ", *FieldStartingOffset * 256 + *(FieldStartingOffset+1));

		FieldStartingOffset+= m_tableList[TableID].KeyList[2].Size;
		printf("%d\n", *FieldStartingOffset * 256 + *(FieldStartingOffset+1));

		CurrentEntry= CurrentEntry->NextEntry;
		i++;
	}
}


void CNetPDLLookupTables::PrintTableEntry(int TableID, struct _TableEntry *CurrentEntry, char *Message)
{
unsigned char* FieldStartingOffset;

if (TableID > 2) return;

	fprintf(stderr, "Table %d - %d/%d - ", TableID, CurrentEntry->Timestamp, CurrentEntry->Lifetime);

	FieldStartingOffset= ((unsigned char*) CurrentEntry) + m_tableList[TableID].KeyList[0].KeyDataOffset;

	fprintf(stderr, "%s (", Message);
	fprintf(stderr, "%d.%d.%d.%d => ", *FieldStartingOffset, *(FieldStartingOffset+1), *(FieldStartingOffset+2), *(FieldStartingOffset+3));

	FieldStartingOffset+= m_tableList[TableID].KeyList[0].Size;
	fprintf(stderr, "%d.%d.%d.%d, ", *FieldStartingOffset, *(FieldStartingOffset+1), *(FieldStartingOffset+2), *(FieldStartingOffset+3));

	FieldStartingOffset+= m_tableList[TableID].KeyList[1].Size;
	fprintf(stderr, "%d => ", *FieldStartingOffset * 256 + *(FieldStartingOffset+1));

	FieldStartingOffset+= m_tableList[TableID].KeyList[2].Size;
	fprintf(stderr, "%d): ", *FieldStartingOffset * 256 + *(FieldStartingOffset+1));

	fprintf(stderr, "#Entries: %d (exact), %d (mask)\n", m_tableList[TableID].CurrentNumberOfExactEntries,m_tableList[TableID].CurrentNumberOfMaskEntries);
}
#endif

