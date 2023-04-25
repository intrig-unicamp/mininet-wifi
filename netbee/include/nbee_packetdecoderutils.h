/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#pragma once

/*!
	\file nbpacketdecoderutils.h

	This file defines some classes that allow to get access to NetPDL internal variables and lookup tables.
*/


#ifdef NBEE_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif



#include <nbprotodb.h>


/*! \addtogroup NetBeePacketDecoder
	\{
*/





/*!
	\brief This class defines a way to get access to NetPDL variables, as used in the decoding process.

	NetPDL uses several internal variables to perform its tasks. This class allow to get access to these
	variables, and to read, modify, create (and more) NetPDL variables.

	\warning This class can be used only when there is an instance of the nbPacketDecoder class.
*/
class DLL_EXPORT nbPacketDecoderVars
{
public:
	nbPacketDecoderVars() {};
	virtual ~nbPacketDecoderVars() {};

	/*!
		\brief Create a new variable.

		\param Variable: structure that contains the attributes of the new variable.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int CreateVariable(struct _nbNetPDLElementVariable* Variable)= 0;

	/*!
		\brief Return the internal ID corresponding to a given variable.

		This method is used to return the internal variable ID to the caller, so that any
		future reference to a given variable can be done through its ID (which is much faster).

		This method must be used prior of any call that makes use of the VariableID.

		\param Name: name of the variable

		\param VariableID: when the function returns, this parameter will contain the VariableID.

		\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetVariableID(const char* Name, int* VariableID)= 0;

	/*!
		\brief Clear the variables whose validity is limited to one packet.

		This method is used to clear all the variable whose validity is limited to the current packet.
		This method has to be called each time a new packet is being submitted to the NetPDL analysis
		engine.

		\param TimestampSec: current timestamp; this is needed in order to clean old sessions
		in lookup tables.
	*/
	virtual void DoGarbageCollection(int TimestampSec)= 0;

	/*!
		\brief Set the value of a run-time variable.

		This method can be used only for numeric and protocol variables.
		
		\param Name: the name of the variable to set.

		\param Value: the value of the variable.

		\return nbSUCCESS if the variable has been set correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning In case this function is used often and performance are important,
		the equivalent function that accepts the VariableID parameter should be used instead.
	*/
	virtual int SetVariableNumber(char* Name, unsigned int Value)= 0;

	/*!
		\brief Set the value (as a buffer) of a run-time variable.

		This method can be used only for buffer variables.
		
		\param Name: the name of the variable to set.

		\param Value: pointer to the value of the variable.

		\param StartingOffset: offset (starting from the beginning of the 'Value' buffer)
		of the valid data. In other words, data will be copied only starting at that offset.

		\param Size: number of bytes of valid data in the 'Value' buffer.

		\return nbSUCCESS if the variable has been set correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning In case this function is used often and performance are important,
		the equivalent function that accepts the VariableID parameter should be used instead.
	*/
	virtual int SetVariableBuffer(char* Name, unsigned char* Value, unsigned int StartingOffset, unsigned int Size)= 0;

	/*!
		\brief Set the value (as a pointer) of a run-time variable.
		
		\param Name: the name of the variable to set.

		\param Value: pointer to the value of the variable.

		\param StartingOffset: offset (starting from the beginning of the 'Value' buffer)
		of the valid data. In other words, the pointer will be calculated starting at that offset.

		\param Size: number of bytes of valid data in the 'Value' buffer.

		\return nbSUCCESS if the variable has been set correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning In case this function is used often and performance are important,
		the equivalent function that accepts the VariableID parameter should be used instead.
	*/
	virtual int SetVariableRefBuffer(char* Name, unsigned char* Value, unsigned int StartingOffset, unsigned int Size)= 0;

	/*!
		\brief Get the value of a run-time variable.

		This method can be used only for numeric and protocol variables.
		
		\param Name: the name of the variable to get.

		\param ReturnValue: when the function return, this pointer will contain the value of the wanted variable.

		\return nbSUCCESS if the variable has been obtained correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning In case this function is used often and performance are important,
		the equivalent function that accepts the VariableID parameter should be used instead.
	*/
	virtual int GetVariableNumber(char* Name, unsigned int* ReturnValue)= 0;

	/*!
		\brief Get the value of a run-time variable.

		This method can be used for both 'buffer' and 'refbuffer' variables, and the returned value
		is always a buffer.
		
		\param Name: the name of the variable to get.

		\param ReturnBufferPtr: when the function return, this pointer will contain a pointer
		to the buffer containing the variable value.

		\param ReturnBufferSize: when the function return, this pointer will contain the number of valid
		bytes within the returned buffer.

		\return nbSUCCESS if the variable has been obtained correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning In case this function is used often and performance are important,
		the equivalent function that accepts the VariableID parameter should be used instead.
	*/
	virtual int GetVariableBuffer(char* Name, unsigned char** ReturnBufferPtr, unsigned int* ReturnBufferSize)= 0;

	/*!
		\brief Set the value of a run-time variable.

		This method can be used only for numeric and protocol variables.
		
		\param VariableID: ID of the wanted variable.

		\param Value: the value of the variable.

		\return nbSUCCESS if the variable has been set correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning Please note that the library does not check the validity of the VariableID.
	*/
	virtual void SetVariableNumber(int VariableID, unsigned int Value)= 0;

	/*!
		\brief Set the value (as a buffer) of a run-time variable.

		This method can be used only for buffer variables.
		
		\param VariableID: ID of the wanted variable.

		\param Value: pointer to the value of the variable.

		\param StartingOffset: offset (starting from the beginning of the 'Value' buffer)
		of the valid data. In other words, data will be copied only starting at that offset.

		\param Size: number of bytes of valid data in the 'Value' buffer.

		\warning Please note that the library does not check the validity of the VariableID.
	*/
	virtual void SetVariableBuffer(int VariableID, unsigned char* Value, int StartingOffset, int Size)= 0;

	/*!
		\brief Set the value (as a pointer) of a run-time variable.
		
		\param VariableID: ID of the wanted variable.

		\param PtrValue: pointer to the value of the variable.

		\param StartingOffset: offset (starting from the beginning of the 'Value' buffer)
		of the valid data. In other words, the pointer will be calculated starting at that offset.

		\param Size: number of bytes of valid data in the 'Value' buffer.

		\return nbSUCCESS if the variable has been set correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning Please note that the library does not check the validity of the VariableID.
	*/
	virtual void SetVariableRefBuffer(int VariableID, unsigned char* PtrValue, int StartingOffset, int Size)= 0;

	/*!
		\brief Get the value of a run-time variable.

		This method can be used only for numeric and protocol variables.
		
		\param VariableID: ID of the wanted variable.

		\param ReturnValue: when the function return, this pointer will contain the value of the wanted variable.

		\return nbSUCCESS if the variable has been obtained correctly, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\warning Please note that the library does not check the validity of the VariableID.
	*/
	virtual void GetVariableNumber(int VariableID, unsigned int* ReturnValue)= 0;

	/*!
		\brief Return the pointer to the requested variable.

		\param VariableID: ID of the wanted variable.

		\param ReturnBufferPtr: when the function returns, this parameter will contain a pointer to the
		starting point of the buffer that has to be returned.

		\param ReturnBufferSize: when the function returns, this parameter will keep the number of bytes 
		waiting to be copied.

		\warning Please note that the library does not check the validity of the VariableID.
	*/
	virtual void GetVariableBuffer(int VariableID, unsigned char** ReturnBufferPtr, unsigned int* ReturnBufferSize)= 0;

	/*!
		\brief Return the pointer to the requested variable.

		\param VariableID: ID of the wanted variable.

		\param StartAt: offset (within the variable) of the first byte that has to be returned.

		\param Size: number of bytes that have to be returned, or '0' if the entire size of the buffer
		has to be returned.

		\param ReturnBufferPtr: when the function returns, this parameter will contain a pointer to the
		starting point of the buffer that has to be returned. This pointer takes into account the
		'StartAt' parameter (in this case, this pointer is moved onward compared to the real starting
		point of the buffer).

		\param ReturnBufferSize: when the function returns, this parameter will keep the number of bytes 
		waiting to be copied (starting at StartAt). This number is useful in case the function is invoked 
		with 'Size=0', because it tells the remaining size of the variable.

		\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
		In case of error, the error message can be retrieved by the GetLastError() method.
		Please note that a special behaviour exists if the variable is the packet buffer and
		we are trying to get access to a location that is outside the packet boundaries. In this case
		(and only in this case) an nbWARNING code is returned.

		\warning Please note that the library does not check the validity of the VariableID.
	*/
	virtual int GetVariableBuffer(int VariableID, int StartAt, int Size, unsigned char** ReturnBufferPtr, unsigned int* ReturnBufferSize)= 0;


	/*! 
		\brief Return a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	virtual char *GetLastError()= 0;
};






/*!
	\brief Structure that contains a single key entry.

	This structure is used to pass data to the most of the functions of the CNetPDLLookupTables 
	and CNetPDLTernaryLookupTables classes.

	This structure is similar to the _LookupTableData, but it contains also the 'Mask' member, which is used
	mainly when an entry is created/deleted, but not in case of lookup, and only in case of keys.

	Particularly, the 'Size' member is important, because a key may have a given size in the lookup table,
	but we may have a smaller size when we submit data.

	For instance, we may have a lookup table tha manages both IPv4 and IPv6 addresses. For doing
	this, we must have a 16-bytes key. However, when we insert only IPv4 addresses, we have only
	4 bytes; therefore we have to know the size of the 'actual' key.
*/
struct _nbLookupTableKey
{
	//! This  member can be:
	//! - a pointer to the value of the given key/data, in case the data type is a 'buffer'
	//! - the actual data, in case the data type is a 'number' or a 'protocol' (in order to avoid one indirection) (this supposes sizeof(char*) = sizeof(int))
	unsigned char* Value;
	//! This member is similar to the 'Value', but it contains the mask. It is meaningful only in case the entry under processing has a mask, otherwise it can be NULL.
	//! Please note that masks must have the same size as the value, and that a binary '1' means "the corrisponding bit in the Value field
	//! must be considered", and '0' means "the value of the corresponding bit in the Value field must not be considered".
	unsigned char* Mask;
	//! Size of 'Value' member.
	unsigned int Size;
	//! Data Type; it can assume the 'number', 'buffer' and 'protocol' values, encoded according to the VariableDataType element of struct _nbNetPDLElementVariable (in the 'nbprotodb' module).
	//! It is a duplication of the same member of struct _nbNetPDLElementKeyData, defined in the 'nbprotodb' module.
	int KeyType;
};


/*!
	\brief Structure that contains a single data entry.

	This structure is similar to the _LookupTableKey, but it does not contain the 'Mask' member,
	which is not needed for data (which does not have to be masked)

	For more details about this structure, please check the documentation of the _LookupTableKey.
*/
struct _nbLookupTableData
{
	//! This  member can be:
	//! - a pointer to the value of the given key/data, in case the data type is a 'buffer'
	//! - the actual data, in case the data type is a 'number' or a 'protocol' (in order to avoid one indirection) (this supposes sizeof(char*) = sizeof(int))
	unsigned char* Value;
	//! Size of the 'Value' member.
	unsigned int Size;
	//! Data Type; it can assume the 'number', 'buffer' and 'protocol' values, encoded according to the VariableDataType element of struct _nbNetPDLElementVariable (in the 'nbprotodb' module).
	//! It is a duplication of the same member of struct _nbNetPDLElementKeyData, defined in the 'nbprotodb' module.
	int DataType;
};


/*!
	\brief This class defines a way to get access to NetPDL lookup tables, as used in the decoding process.

	NetPDL uses several internal lookup tables to perform its tasks. This class allow to get access to these
	lookup tables, and to read, modify, create (and more) lookup entries and lookup tables.

	\warning This class can also be used when there is not an instance of the nbPacketDecoder class. In this case, 
	please use the nbAllocatePacketDecoderLookupTables() function in order to create an instance of this class.
	For cleanup, the programmer must use the nbDeallocatePacketDecoderLookupTables() function.
*/
class DLL_EXPORT nbPacketDecoderLookupTables
{
public:
	nbPacketDecoderLookupTables() {};
	virtual ~nbPacketDecoderLookupTables() {};

	/*!
		\brief Creates a lookup table.

		\param LookupTable: structure that contains the lookup table, as it has been defined
		in the NetPDL file.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int CreateTable(struct _nbNetPDLElementLookupTable* LookupTable)= 0;

	/*!
		\brief Return the internal ID corresponding to a given lookup table.

		This method is used to return the internal variable ID to the caller, so that any
		future reference to a given variable can be done through its ID (which is much faster).

		\param TableName: name of the lookup table we are looking for.

		\param DataName: name of the data field in the lookup table we are looking for. Please
		note that this parameter can be NULL; in this case, the DataID parameter does not have
		any meaning.

		\param TableID: when the function returns, this parameter will contain the TableID.

		\param DataID: when the function returns, this parameter will contain the FieldID.

		\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetTableID(char* TableName, char *DataName, int *TableID, int *DataID)= 0;

	/*!
		\brief Check if there are old entries and it deletes them.

		This method checks all the entries to see if they are too old and need to be deleted.

		\param TimestampSec: current timestamp (seconds, in UNIX time). Entries whose
		(Timestamp + Lifetime) is less than this value are deleted from the table.

		\param AggressiveScan: if 'true', it implement a method that is faster to purge old
		data, but it does not guarantee to purge everything is needed. If 'false', it scans
		the entire table instead.
		In order to maximize performances, you can use the AggressiveScan most of the times, and
		sometimes call the this function with this flag turned off.
	*/
	virtual void DoGarbageCollection(int TimestampSec, int AggressiveScan)= 0;

	/*!
		\brief Returns a structure used to pass parameters to most of the members of this class.

		Most of the members of this class needs to get the key list and data list as parameters.
		However, these structure may be rather complicated; therefore, this structure is created
		internally to this class, passed outside in order to let the caller personalize some of the
		items in there, and then used to call members of this class.

		Please note that when this structure is returned back, some members are already initialized.
		Particularly:
		- Value: must be updated with the field value
		- Size: must be updated with the field size
		- Mask: must be updated with the field value (or set to NULL) in case this is not needed
		- KeyType: it is already filled in by the NetBee library and it should be left untouched.

		\param TableID: ID of the table that we are looking for.
		This value is returned back by the GetTableID() method.

		\return The _nbLookupTableKey structure if everything is fine, NULL otherwise.
		This structure is an array of pointers; we can access data by using the 
		<code>KeyData[i]->member</code> notation.

		\note The size of this array is not returned back. However, the user usually knows its
		size thanks to the _nbNetPDLElementLookupKeyData structure.
	*/
	virtual struct _nbLookupTableKey* GetStructureForTableKey(int TableID)= 0;

	/*!
		\brief Returns a structure used to pass parameters to most of the members of this class.

		Most of the members of this class needs to get the key list and data list as parameters.
		However, this structure may be rather complicated; therefore, this structure is created
		internally to this class, passed outside in order to let the caller personalize some of the
		items in there, and then used to call members of this class.

		Please note that when this structure is returned back, some members are already initialized.
		Particularly:
		- Value: must be updated with the data value
		- Size: must be updated with the data size
		- DataType: it is already filled in by the NetBee library and it should be left untouched.

		\param TableID: ID of the table that we are looking for.
		This value is returned back by the GetTableID() method.

		\return The _nbLookupTableData structure if everything is fine, NULL otherwise.
		This structure is an array; we can access data by using the 
		<code>KeyData[i].member</code> notation.

		\note The size of this array is not returned back. However, the user usually knows its
		size thanks to the _nbNetPDLElementLookupKeyData structure.
	*/
	virtual struct _nbLookupTableData* GetStructureForTableData(int TableID)= 0;

	/*!
		\brief Add a new valid entry in the lookup table.

		\param TableID: ID of the table in which the new member has to be added.
		This value is returned back by the GetTableID() method.

		\param KeyList: array of pointers to the keys that have to be inserted. Please note 
		that the fact that this array contains (char *) is not related with the type 
		of data we have. For instance, each key is copied "as is", byte after byte (i.e. if the 
		key is 2 bytes, this function copies the two bytes in memory at the address referred by 
		this pointer). The size of this array must be equal to the number of keys defined in 
		the Create() method.

		\param DataList: array of pointers to the custom data that have to be inserted. Please note 
		that the fact that this array contains (char *) is not related with the type 
		of data we have. For instance, each data is copied "as is", byte after byte (i.e. if the 
		size of the data is 2 bytes, this function copies the two bytes in memory at the address 
		referred by this pointer). The size of this array must be equal to the number of custom
		data defined in the Create() method.

		\param TimestampSec: timestamp that has to be associated to the current entry.

		\param KeysHaveMasks: 'true' if some of the keys are masked. This flag is used in order 
		to turn on some custom processing. The value of this flag can be found in the 
		struct _nbNetPDLElementUpdateLookupTable, which defines the UpdateLookupTable action in the
		NetPDL language.

		\param Validity: In case we're going to add a 'masked' item, it contains how long this masked 
		entry must stay in the lookup table. It can assume the values listed in 
		#nbNetPDLUpdateLookupTableAddValidityTypes_t. It is valid only in case of the 'add' action.
		This value can be found in the struct _nbNetPDLElementUpdateLookupTable, which defines the 
		UpdateLookupTable action in the NetPDL language.

		\param KeepTime: Value of the 'keeptime' attribute in the NetPDL element, when present.
		This value can be found in the struct _nbNetPDLElementUpdateLookupTable, which defines the 
		UpdateLookupTable action in the NetPDL language.

		\param HitTime: Value of the 'hittime' attribute in the NetPDL element, when present.
		This value can be found in the struct _nbNetPDLElementUpdateLookupTable, which defines the 
		UpdateLookupTable action in the NetPDL language.

		\param NewHitTime: Value of the 'newhitime' attribute in the NetPDL element, when present.
		This value can be found in the struct _nbNetPDLElementUpdateLookupTable, which defines the 
		UpdateLookupTable action in the NetPDL language.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int AddTableEntry(int TableID, struct _nbLookupTableKey KeyList[], struct _nbLookupTableData DataList[], int TimestampSec,
								int KeysHaveMasks, nbNetPDLUpdateLookupTableAddValidityTypes_t 	Validity, int KeepTime, int HitTime, int NewHitTime)= 0;

	/*!
		\brief Delete a given entry from the lookup table.

		\param TableID: ID of the table where there is the entry that has to be deleted.
		This value is returned back by the GetTableID() method.

		\param KeyList: array of pointers to the keys of the entry that has to be deleted.
		The format of this parameter is equal to the one present in the Add() method.

		\param MaskedEntry: 'true' if the KeyList contains a masked entry, 'false' if we're
		managing an exact entry.

		\param TimestampSec: current timestamp (in seconds).

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int PurgeTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int MaskedEntry, int TimestampSec)= 0;

	/*!
		\brief Mark as obsolete a given entry from the lookup table.

		This function sets the 'lifetime' of the current session to 5 minutes; after this time
		it will be purged by the garbage collection routine.
		This is one of the possible logic; an alternate logic may be to mark the session 
		as obsolete, and purge that session when we need to space in memory. However,
		this method is simpler.

		\param TableID: ID of the table where there is the entry that has to be deleted.
		This value is returned back by the GetTableID() method.

		\param KeyList: array of pointers to the keys of the entry that has to be deleted.
		The format of this parameter is equal to the one present in the Add() method.

		\param MaskedEntry: 'true' if the KeyList contains a masked entry, 'false' if we're
		managing an exact entry.

		\param TimestampSec: current timestamp (in seconds).

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int ObsoleteTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int MaskedEntry, int TimestampSec)= 0;

	/*!
		\brief Checks if a given entry is present in the lookup table.

		Please note that exact entries are examined first.

		\param TableID: ID of the table in which the entry has to be looked for.
		This value is returned back by the GetTableID() method.

		\param KeyList: array of pointers to the keys of the element that has to be checked.
		The format of this parameter is equal to the one present in the Add() method.

		\param TimestampSec: current timestamp (in seconds).

		\param MatchExactEntries: 'true' if we have to search through the exact entries; 
		if it is false, we do not have to scan these entries.

		\param MatchMaskEntries: 'true' if we have to search through the mask entries; 
		if it is false, we do not have to scan these entries.

		\param GetFirstMatch: 'true' if the first match has to be returned. This parameter should never be used; in case
		some other matches have to be returned, the user should call the GetTableNextMatchingEntry(). 

		\return nbSUCCESS if an entry with that key is present in the table, nbWARNING
		if the entry is not present (but no errors occur), nbFAILURE in case of error.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\note (for NetBee developers only) If the entry is found in the table, the
		'MatchingEntry' pointer of the main table structure will point to the matching element.
	*/
	virtual int LookupForTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int TimestampSec, int MatchExactEntries, int MatchMaskEntries, int GetFirstMatch= 1)= 0;

	/*!
		\brief Checks if a given entry is present in the lookup table; if so, it updates also the entry (if needed).

		This function does the same as the LookupForTableEntry(), but it checks if the entry needs to be updated.
		If so, it updates the entry accordingly (e.g. it sets the timestamp, etc).

		Please note that exact entries are examined first.

		\param TableID: ID of the table in which the entry has to be looked for.
		This value is returned back by the GetTableID() method.

		\param KeyList: array of pointers to the keys of the element that has to be checked.
		The format of this parameter is equal to the one present in the Add() method.

		\param TimestampSec: current timestamp (in seconds).

		\return nbSUCCESS if an entry with that key is present in the table, nbWARNING
		if the entry is not present (but no errors occur), nbFAILURE in case of error.
		In case of error, the error message can be retrieved by the GetLastError() method.

		\note (for NetBee developers only) If the entry is found in the table, the
		'MatchingEntry' pointer of the main table structure will point to the matching element.
	*/
	virtual int LookupAndUpdateTableEntry(int TableID, struct _nbLookupTableKey KeyList[], int TimestampSec)= 0;

	/*!
		\brief Scan the table and returns all the table entries contained in there.

		This function has to be called several times, and each time it returns another entry present
		in the table. The loop ends when no more entries are contained in there.

		\param TableID: ID of the table in which the entry that has to be scanned.
		This value is returned back by the GetTableID() method.

		\param ScanExactEntries: 'true' if we want to list exact entries, 'false' if we want to scan
		masked entries. If we want to scan both, we have to create a double look, scanning the exact ones
		first, and the masked ones later.

		\param CurrentElementHandler: this parameter has no external meaning. It is a boid*, passed by reference,
		which must be set to NULL the first time the function is called. Then, this function will update this 
		parameter internally in order to remember the current position. In the next calls, this parameter must 
		be left untouched, i.e. new calls must keep the value returned by the previous call.

		\param KeyList: when the function return, this member will contains the KeyList. Keys must be accessed
		using code like KeyList[Number].Value and such.

		\param DataList: when the function return, this member will contains the DataList. Data must be accessed
		using code like DataList[Number].Value and such.

		\return nbSUCCESS if an entry with that key is present in the table, nbWARNING
		if the entry is not present (but no errors occur), nbFAILURE in case of error.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int ScanTableEntries(int TableID, int ScanExactEntries, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList, void** CurrentElementHandler)= 0;

	/*!
		\brief Return the value associated to a given data field in the lookup table.

		This function must be called after a Lookup(), which selects a 'matching entry'.
		Then, this function return to the caller a given field of the matching entry.

		\param TableID: ID of the table where there is the entry that has to be returned.
		This value is returned back by the GetTableID() method.

		\param DataID: ID of the field that has to be returned back. This value is returned
		by the GetTableID().

		\param StartAt: offset (within the variable) of the first byte that has to be returned.

		\param Size: number of bytes that have to be returned, or '0' if the entire size of the buffer
		has to be returned.

		\param DataValue: when the function returns, this pointer will point to the data value.
		This value is a generic binary string, hence it is not zero-terminated.

		\param DataSize: when the function returns, it contains the size of the valid data 
		returned in previous buffer.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetTableDataBuffer(int TableID, int DataID, unsigned int StartAt, unsigned int Size, unsigned char **DataValue, unsigned int *DataSize)= 0;

	/*!
		\brief Return the value (as number) of the requested variable.

		This function must be called after a Lookup(), which selects a 'matching entry'.
		Then, this function return to the caller a given field of the matching entry.

		\param TableID: ID of the table where there is the entry that has to be returned.
		This value is returned back by the GetTableID() method.

		\param DataID: ID of the field that has to be returned back. This value is returned
		by the GetTableID().

		\param DataValue: when the function returns, this pointer will point to the data value.
		This value is a number.

		\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetTableDataNumber(int TableID, int DataID, unsigned int* DataValue)= 0;

	/*!
		\brief Set the value associated to a given data field in the lookup table.

		This function must be called after a Lookup(), which selects a 'matching entry'.
		Then, this function sets the value of a given field of the matching entry.

		\param TableID: ID of the table where there is the entry that has to be updated.
		This value is returned back by the GetTableID() method.

		\param DataID: ID of the field that has to be updated. This value is returned
		by the GetTableID().

		\param DataValue: pointer to the buffer that contains the new value for the entry.
		This value is a generic binary string, hence it is not zero-terminated.

		\param StartingOffset: offset, within the lookuptable buffer, at which we have to
		start copying data.

		\param DataSize: size of the data that has to be copied (it is equal to the DataValue
		size in case StartingOffset is equal to zero).

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int SetTableDataBuffer(int TableID, int DataID, unsigned char *DataValue, unsigned int StartingOffset, unsigned int DataSize)= 0;

	/*!
		\brief Set the value associated to a given data field in the lookup table.

		This function must be called after a Lookup(), which selects a 'matching entry'.
		Then, this function sets the value of a given field of the matching entry.

		\param TableID: ID of the table where there is the entry that has to be updated.
		This value is returned back by the GetTableID() method.

		\param DataID: ID of the field that has to be updated. This value is returned
		by the GetTableID().

		\param DataValue: Variable that contains the new value for the entry.
		This value is a number.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int SetTableDataNumber(int TableID, int DataID, unsigned int DataValue)= 0;

	/*!
		\brief Return the first entry in the table.

		\param TableID: ID of the table whose entry has to be returned.
		This value is returned back by the GetTableID() method.

		\param GetExactEntry: 'true' if we want to get the fist exact entry, 'false' if we want to get
		the first masked entry.

		\param KeyList: when the function return, this member will contains the KeyList. Keys must be accessed
		using code like KeyList[Number].Value and such.

		\param DataList: when the function return, this member will contains the DataList. Data must be accessed
		using code like DataList[Number].Value and such.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetTableFirstEntry(int TableID, int GetExactEntry, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList)= 0;

	/*!
		\brief Return the current matching entry in the table.

		This function must be called after a Lookup(), which selects a 'matching entry'.
		Then, this function return to the caller the entire matching entry.

		\param TableID: ID of the table whose entry has to be returned.
		This value is returned back by the GetTableID() method.

		\param KeyList: when the function return, this member will contains the KeyList. Keys must be accessed
		using code like KeyList[Number].Value and such.

		\param DataList: when the function return, this member will contains the DataList. Data must be accessed
		using code like DataList[Number].Value and such.

		\return nbSUCCESS if everything is fine, nbFAILURE otherwise.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetTableMatchingEntry(int TableID, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList)= 0;

	/*!
		\brief Return the next matching entry in the table.

		This function must be called after a Lookup() and after the GetTableMatchingEntry()., which selects a 'matching entry'.
		Then, this function return to the caller the entire matching entry.

		\param TableID: ID of the table whose entry has to be returned.
		This value is returned back by the GetTableID() method.

		\param KeyList: when the function return, this member will contains the KeyList. Keys must be accessed
		using code like KeyList[Number].Value and such.

		\param DataList: when the function return, this member will contains the DataList. Data must be accessed
		using code like DataList[Number].Value and such.

		\return nbSUCCESS if an entry with that key is present in the table, nbWARNING
		if the entry is not present (but no errors occur), nbFAILURE in case of error.
		In case of error, the error message can be retrieved by the GetLastError() method.
	*/
	virtual int GetTableNextMatchingEntry(int TableID, struct _nbLookupTableKey** KeyList, struct _nbLookupTableData** DataList)= 0;

	/*!
		\brief Return the number of exact entries present in the table specified.

		\param TableID: ID of the table whose entry has to be returned.
		This value is returned back by the GetTableID() method.

		\return the number of exact entries present in the table.
	*/
	virtual int GetNumberOfEntries(int TableID)=0;

	/*! 
		\brief Return a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	virtual char *GetLastError()= 0;
};



/*!
	\brief It creates a new instance of a nbPacketDecoderLookupTables object and returns it to the caller.

	This function is needed in order to create the proper object derived from nbPacketDecoderLookupTables.
	For instance, nbPacketDecoderLookupTables is a virtual class and it cannot be instantiated.
	The returned object has the same interface of nbPacketDecoderLookupTables, but it is a real object.

	A nbPacketDecoderLookupTables class is a class that manages lookup tables.

	This class is provided only for special cases in which the user needs to use lookup tables without
	using the nbPacketDecoder class (for instance, lookup tables can be used without an instance of the packet
	decoder, but NetPDL variables cannot). In most of the cases, user should first allocate the nbPacketDecoder
	class, and then use the nbPacketDecoder::GetPacketDecoderLookupTables() method in order to get a pointer
	to the lookup tables.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will eventually 
	keep an error message (if one).

	\param ErrBufSize: the length of the buffer that keeps the error message.

	\return A pointer to real object that has the same interface of the nbPacketDecoderLookupTables abstract class,
	or NULL in case of error. In case of failure, the error message is returned into the ErrBuf buffer.

	\warning Be carefully that the returned object must be deallocated through the nbDeallocatePacketDecoderLookupTables().
*/
DLL_EXPORT nbPacketDecoderLookupTables* nbAllocatePacketDecoderLookupTables(char *ErrBuf, int ErrBufSize);


/*!
	\brief It deallocates the instance of nbPacketDecoderLookupTables created through the nbAllocatePacketDecoderLookupTables().

	\param PacketDecoderLookupTables: pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocatePacketDecoderLookupTables(nbPacketDecoderLookupTables* PacketDecoderLookupTables);


/*!
	\}
*/

