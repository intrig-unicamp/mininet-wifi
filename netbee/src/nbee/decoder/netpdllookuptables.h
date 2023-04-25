/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/*!
	\file netpdllookuptables.h

	This file defines the code that manages lookup tables.
*/


#pragma once

#include <nbee_packetdecoderutils.h>

#include <map>
#include <cstring>

#ifdef WIN32
#include <winsock2.h>			// needed for u_int64
#endif

//! Maximum number of lookup tables supported by this engine
#define MAX_NUM_LOOKUP_TABLES 20
//! Maximum lookups before we force a garbage collection
#define MAX_CALL_BEFORE_GARBAGE 1000


 
//#define DEBUG_LOOKUPTABLE
#define LOOKUPTABLE_PROFILER

// If the debug code of the lookup table is enable, this must turn on also the lookuptable profiler
#ifdef DEBUG_LOOKUPTABLE
#define LOOKUPTABLE_PROFILER
#endif

/*!
	\brief This class is used to populate the MapExact structure for O(n log n) lookup search
	The first element must be the pointer to the Key filed in the _TableInfo structure
	The second element must indicate the size of the first element
*/
class HashKey
{
public: 
	char* Key;
	size_t KeySize;
	HashKey(char* _Key, size_t _KeySize): Key(_Key), KeySize(_KeySize){}
    bool operator < (const HashKey & Key1) const
    {
	    return std::memcmp(Key, Key1.Key, KeySize < Key1.KeySize ? KeySize : Key1.KeySize) < 0;
    }

	bool operator== (const HashKey & Key1) const
    {
	    return std::memcmp(Key, Key1.Key, KeySize < Key1.KeySize ? KeySize : Key1.KeySize) == 0;
    }


};

/*!
	\brief This class is devoted to the management of the lookup tables (e.g. for storing TCP sessions).
*/
class CNetPDLLookupTables : public nbPacketDecoderLookupTables
{
public:
	CNetPDLLookupTables(char *ErrBuf, int ErrBufSize);
	virtual ~CNetPDLLookupTables();

	struct _DataKeyEntry
	{
		//! Names of the key/data.
		char* Name;
		//! Size (in bytes) of the corresponding key/data.
		unsigned int Size;
		//! Offset (in bytes from the beginning of the struct _TableInfo) of the corresponding key/data.
		unsigned int KeyDataOffset;
		//! Offset (in bytes from the beginning of the struct _TableInfo) of the corresponding mask.
		unsigned int MaskOffset;
		//! Data Type. It is a duplication of the same member of struct _nbNetPDLElementKeyData. 
		nbNetPDLLookupTableKeyDataTypes_t KeyDataType;
	};

	/*!
		\brief Structure that contains all data related to a single entry in the lookup table.

		The structure is rather complex because it has been designed with (some) performance issue
		in mind. There are mainly two characteristics that need to be kept in mind:
		- these entries are structured in a bi-directional linked list (each entry points to the
		previous and the following element)
		- this entry is structured with some member at the beginning and a void * at the end.
		In fact, this void pointer is never used; what we have is that when we allocate the entire
		entry, we allocate the space needed for this structure AND the sum of the space required 
		to keep keys and data.

		It turns out that key and data are stored in memory in a "memory buffer" which has not 
		been declared explicitly. However, this method makes easy to manage whatever size (and number)
		of the keys and custom data we have.
	*/
	struct _TableEntry
	{
		//! Pointer to previous entry in the chain.
		struct _TableEntry *PreviousEntry;
		//! Pointer to next entry in the chain.
		struct _TableEntry *NextEntry;
		//! 'true' if the entry had one or more hits, 'false' if the entry had no hit so far. This member is not used for all entry types.
		int GotHits;
		//! Timestamp of the current entry. It is valid only in tables whose entries have dynamic lifetime.
		int Timestamp;
		//!	Lifetime of the current entry. It is valid only in tables whose entries have dynamic lifetime.
		//! This value is copied from KeepTime, HitTime and NewHitTime, according to the entry type and if it is the first hit or not.
		int Lifetime;
		//! Validity of the entry (i.e. live forever, move this entry to the 'exact' table on hit, ...)
		int ValidityType;
		//! Maximum time of validity of the current entry (unless refreshed).
		int KeepTime;
		//! Additional time of validity of the current entry (if refreshed).
		int HitTime;
		//! Additional time of validity of the current entry (if refreshed): valid only for 'addonhit' entries
		int NewHitTime;
		//! Fake member; the space starting from here is used to store keys, data and mask (in this order).
		//! By the way, this is never accessed through the code, but it's better to have it for debugging purposes
		void* EntryValue;
	};

	//! Structure that contains the data related to a single lookup table.
	struct _TableInfo
	{
		//! Name of the table.
		char *Name;

		//! Number of struct _TableEntry entries. This value is an upper limit on the size of the table
		//! (this is a limit of current implementation).
		int NumberOfExactEntries;
		//! Number of struct _TableEntry entries. This value is an upper limit on the size of the table
		//! (this is a limit of current implementation).
		int NumberOfMaskEntries;

		//! Number of keys present in this table.
		int NumberOfKeys;
		//! Number of custom data present in this table.
		int NumberOfData;

		//! Array that contains the information related to the keys.
		struct _DataKeyEntry* KeyList;

		//! Array that contains the information related to the data fields.
		struct _DataKeyEntry* DataList;

		//! Structure that is used to pass parameters to this class.
		//! It is created within this class and it is filled up by external functions.
		//! See also GetStructureForKey().
		struct _nbLookupTableKey* ExportedKeyList;
		//! Structure that is used to pass parameters to this class.
		//! It is created within this class and it is filled up by external functions.
		//! See also GetStructureForData().
		struct _nbLookupTableData* ExportedDataList;

		//! Pointer to the first entry (struct _TableEntry) that contains valid data.
		//! It is use to scan the list when we have to do a lookup.
		struct _TableEntry* FirstExactEntry;
		//! Pointer to the last entry (struct _TableEntry)) that contains valid data.
		//! It is use to scan the list when we have to do a cleanup (garbage collection).
		struct _TableEntry* LastExactEntry;
		//! Pointer to the first entry (struct _TableEntry) that contains no valid data.
		//! It is used when a new entry has to be added in the lookup table.
		struct _TableEntry* FirstVoidExactEntry;

		//! Pointer to the first entry (struct _TableEntry) that contains valid data.
		//! It is use to scan the list when we have to do a lookup.
		struct _TableEntry* FirstMaskEntry;
		//! Pointer to the last entry (struct _TableEntry)) that contains valid data.
		//! It is use to scan the list when we have to do a cleanup (garbage collection).
		struct _TableEntry* LastMaskEntry;
		//! Pointer to the first entry (struct _TableEntry) that contains no valid data.
		//! It is used when a new entry has to be added in the lookup table.
		struct _TableEntry* FirstVoidMaskEntry;

		//! Total size of the keys
		//! (i.e. if we have 2 keys, one 2 and the other 3 bytes, this value will be 5).
		int KeysEntrySize;
		//! Total size of data
		int DataEntrySize;
		//! Pointer to a buffer (whose size is KeysEntrySize) used to store (temporarily) the keys when we have to to a lookup.
		void *KeyForCompare;

		//! Structure that contains a pointer to the matching entry (after a lookup).
		struct _TableEntry* MatchingEntry;
		//! Flag that is 'true' if the matching entry is in the 'exact' entry list, or 'false' if it belongs to the 'masked' entry list.
		int MatchingEntryIsExact;

		//! ID of the 'timestamp' data field.
		int TimestampFieldID;
		//! ID of the 'lifetime' data field.
		int LifetimeFieldID;

		//! 'true' if the table allows dynamic entries (the ones that required the automatic purging, through the timestamp).
		//! Currently, this flag is almost unused (is should be used to avoid to create the timestamp and lifetime members in each entry).
		int AllowDynamicEntries;
		

#ifdef LOOKUPTABLE_PROFILER
#define PROFILER_NBINS_EXACT 100
#define PROFILER_BINS_INTERVAL_EXACT 10
#define PROFILER_NBINS_MASK 100
#define PROFILER_BINS_INTERVAL_MASK 10

		int ProfilerBinsIntervalExact;
		int ProfilerBinsIntervalMask;

		//! Current number of items in the lookup table
		int CurrentNumberOfExactEntries;
		//! Current number of items in the lookup table
		int CurrentNumberOfMaskEntries;

		//! Maximum number of items in the lookup table
		int MaxNumberOfExactEntries;
		//! Maximum number of items in the lookup table
		int MaxNumberOfMaskEntries;

		//! Array for storing the profiling of the lookup table
		int ProfilerExactEntries[PROFILER_NBINS_EXACT];
		//! Array for storing the profiling of the lookup table
		int ProfilerMaskedEntries[PROFILER_NBINS_MASK];

		//! Total number of 'add' operations in the table; needed to know the number of items we inserted in the table
		int TotalNumberInsertionExact;
		//! Total number of 'add' operations in the table; needed to know the number of items we inserted in the table
		int TotalNumberInsertionMask;


#endif
	};


	int CreateTable(struct _nbNetPDLElementLookupTable* LookupTable);

	int GetTableID(char* TableName, char *DataName, int *TableID, int *DataID);

	struct _nbLookupTableKey* GetStructureForTableKey(int TableID);
	struct _nbLookupTableData* GetStructureForTableData(int TableID);

	int AddTableEntry(int TableID, struct _nbLookupTableKey KeyList[], struct _nbLookupTableData DataList[], int TimestampSec, 
						int KeysHaveMasks, nbNetPDLUpdateLookupTableAddValidityTypes_t 	Validity, int KeepTime, int HitTime, int NewHitTime);
	int PurgeTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int MaskedEntry, int TimestampSec);
	int ObsoleteTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int MaskedEntry, int TimestampSec);

	int LookupForTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int TimestampSec, int MatchExactEntries, int MatchMaskEntries, int GetFirstMatch);
	int LookupAndUpdateTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int TimestampSec);
	int GetTableDataBuffer(int TableID, int DataID, unsigned int StartAt, unsigned int Size, unsigned char **DataValue, unsigned int *DataSize);
	int GetTableDataNumber(int TableID, int DataID, unsigned int* DataValue);
	int SetTableDataBuffer(int TableID, int DataID, unsigned char *DataValue, unsigned int StartingOffset, unsigned int DataSize);
	int SetTableDataNumber(int TableID, int DataID, unsigned int DataValue);

	int GetTableFirstEntry(int TableID, int GetExactEntry, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList);
	int GetTableMatchingEntry(int TableID, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList);
	int GetTableNextMatchingEntry(int TableID, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList);

	int ScanTableEntries(int TableID, int ScanExactEntries, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList, void** CurrentElementHandler);

	void DoGarbageCollection(int TimestampSec, int AggressiveScan);
	char *GetLastError() { return m_errbuf; };

	int GetNumberOfEntries(int TableID);



private:
	void MoveNewExactEntryOnTopOfList(int TableID);
	void MoveExistingExactEntryOnTopOfList(int TableID, struct _TableEntry *MatchingEntry);
	void PurgeExistingExactEntry(int TableID, struct _TableEntry *MatchingEntry);
	void MoveNewMaskEntryOnTopOfList(int TableID);
	void MoveExistingMaskEntryOnTopOfList(int TableID, struct _TableEntry *MatchingEntry);
	void PurgeExistingMaskEntry(int TableID, struct _TableEntry *MatchingEntry);
	int AllocateEntries(int NumberOfEntries, int EntrySize, struct _TableEntry **FirstEntry);

	int CheckAndDeleteIfOldExactEntry(int TableID, struct _TableEntry *CurrentEntry, int TimestampSec);
	int CheckAndDeleteIfOldMaskEntry(int TableID, struct _TableEntry *CurrentEntry, int TimestampSec);

#ifdef LOOKUPTABLE_PROFILER
	void UpdateProfilerCounters(int TableID, int IsExact, int Value);
	void PrintTableStats(int TableID);
#endif

#ifdef DEBUG_LOOKUPTABLE
	void PrintEntireTable(int TableID, int IsExact, int TimestampSec);
	void PrintTableEntry(int TableID, struct _TableEntry *CurrentEntry, char* Message);
#endif

	int lookupExactCall;

	//! List of tables (allocated statically at the beginning)
	struct _TableInfo m_tableList[MAX_NUM_LOOKUP_TABLES];

	//! Current number of tables configured in the system
	int m_currNumTables;

	//! Pointer to the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	char *m_errbuf;
	//! Size of the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	int m_errbufSize;

	/*! 
		Array of binary tree map used to perfrom lookup over Exact entries
		The key element must be a HashKey element.
		The data element must be the pointer to the corresponding _TableEntry element
	*/
	std::map<HashKey, _TableEntry*> MapExact[MAX_NUM_LOOKUP_TABLES];
};


