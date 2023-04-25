/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once


/*!
	\file nbee_extractedfieldreader.h

	Keeps the definitions related to the nbExtractedFieldsReader class used in the NetBee library.
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

#include <nbpflcompiler.h>

#include <string>
#include <list>
using namespace std;





/*! \addtogroup NetBeePacketEngine
	\{
*/


//! This structure contains the name of the extracted fields as a list of 'protocol.fieldname' items; it is implemented as an STL list of STL strings.
typedef list<string> _nbExtractedFieldsNameList;


/*!
	\brief This class allows the user to parse the list of fields extracted from a network packet. 
	
	Typically, nbExtractedFieldsReader class is used when the nbPacketEngine class 
	is configured to extract fields from the packet. 
	The nbExtractedFieldsReader contains the data required to parse the list of fields extracted 
	by the nbPacketEngine class and can be used to get field values, their offsets and such. 
	All fields information are stored in the _nbExtractedFieldsDescriptor structures.

	After the processing of the packet, the nbPacketEngine class uses the protected method 
	FillDescriptors() to set the structures with the current data in the info partition.
	
	Other methods offer the opportunity to check the validity of the current data. Besides, 
	it is possible to get the list of field names and ask for a specific field calling the corresponding method. 
	
	To get the _nbExtractedFieldsDescriptor structure that contain field information use the following methods:
	- 	If the user needs to have all fields structures, use the GetField() method without any parameters, 
		it returns a _nbExtractedFieldsDescriptorVector structure that contains all the data concern the last packet processed by the nbPacketEngine class.
	
	- 	If only one field is necessary, use the GetField() overloading methods: if the parameter 
		is an integer the _nbExtractedFieldsDescriptor structure returned corresponds to the field position 
		in the extraction list; if the parameter is a string the _nbExtractedFieldsDescriptor structure returned 
		corresponds to the field that matches the name in the right format (protoname.fieldname).
	
	To know if a _nbExtractedFieldsDescriptor structure contains valid data or if all extracted fields are present in the last packet processed, use the following methods: 
	-	The method IsValid() let to know if the field, corresponding to a _nbExtractedFieldsDescriptor structure, 
		is present in the last packet processed by the nbPacketEngine class. If the return value is nbFAILURE the data are expired.
	-	If the user needs to know if all fields are present in the packet, use the IsComplete() method, 
		that returns the same result of a cycle of IsValid() method on all _nbExtractedFieldsDescriptor structures.
	
	If the user does not know the fields name, it is possible to get them using the GetFieldNames() method that returns a list of them. The order of this list corresponds to the extraction one.

*/
class DLL_EXPORT nbExtractedFieldsReader
{
friend class nbeePacketEngine;

public:
	/*!
		\brief	Object constructor

	*/
	nbExtractedFieldsReader(){};

	/*!
		\brief	Object destructor
	*/
	virtual ~nbExtractedFieldsReader(void){};

	/*!
		\brief Returns a pointer to the _nbExtractedFieldsDescriptor of the field specified by name.

		\param FieldName: Field name referring to a member of the _nbExtractedFieldsNameList returned by the method
						  GetFieldNames(). The format of this parameter is 'protoname.fieldname'

		\return A pointer to a _nbExtractedFieldsDescriptor if no error occurred, NULL if the FieldName
		does not correspond to any _nbExtractedFieldsDescriptor name.
	
	*/
	virtual _nbExtractedFieldsDescriptor *GetField(string FieldName)=0;

	/*!
		\brief Returns a pointer to the _nbExtractedFieldsDescriptor of the field specified by index.

		\param Index: position of the field in the extraction list present in the
		              ExtractFields() action.

		\return A pointer to a _nbExtractedFieldsDescriptor object if no error occurred, NULL if the index
		does not correspond to any _nbExtractedFieldsDescriptor.
	*/
	virtual _nbExtractedFieldsDescriptor* GetField(int Index)=0;

	/*!
		\brief Returns a pointer to the _nbExtractedFieldsDescriptorVector that contains all the fields extracted.

		\return A pointer to a Vector of _nbExtractedFieldsDescriptor if no error occurred, NULL otherwise.
		The Vector returned is empty
		in case the filtering string (entered by the user) does not contain any field to be extracted.
	*/
	virtual _nbExtractedFieldsDescriptor* GetFields()=0;
	
	/*!
		\brief Returns a pointer to the _nbExtractFieldInfo that contains all the information of the fields extracted.


		\return A pointer to a Vector of _nbExtractFieldInfo if no error occurred, NULL otherwise.
		The Vector returned is empty
		in case the filtering string (entered by the user) does not contain any field to be extracted.
		The position of information in the vector is refferred to the same position on the vector of extracted field.
	*/
	virtual _nbExtractFieldInfo* GetFieldsInfo()=0;
	
	/*!
		\brief Returns a pointer to the _nbExtractFieldInfo that contains all the information of the fields extracted for the allfields entry.


		\return A pointer to a Vector of _nbExtractFieldInfo if no error occurred, NULL otherwise.
		The Vector returned is NULL
		in case the filtering string (entered by the user) does not contain any allfields protocol to be extracted.
		The position of information in the vector is refferred to the same position on the vector inside the extracted allfields entry on the extracted field vector.
	*/
	
	virtual _nbExtractFieldInfo* GetAllFieldsInfo()=0;

	/*!
		\brief Returns the the number of the fields extracted.
	*/
	virtual int getNumFields()=0;
	
	/*!
		\brief Returns the list of the field names that will be extracted, in the format 'protoname.fieldname'.

		\return A STL 'list' of STL 'string' elements, or NULL in case the filtering string
		(entered by the user) does not contain any field to be extracted.
			
		\note The user can scan the list using the functions defined for the 'list' STL class, and get
		access to each member using the properties of the 'string' STL class.
	*/
	virtual _nbExtractedFieldsNameList GetFieldNames()=0;

	/*!
		\brief	Returns nbSUCCESS if the field is present in the packet and it has been extracted.
		        BitField with valid bit at 1; other field with Len > 0.

		Some fields may not be present in all the packets; this is the case of optional fields,
		or fields that do not appear because the packet is truncated, and more.
		This method allow the user to know if the wanted field is present in the packet or not;
		in the fist case the field can be printed, otherwise it can be omitted.

		\param	FieldDescriptor: pointer to the _nbExtractedFieldsDescriptor structure of the field to verify.

		\return nbSUCCESS if the descriptor is valid, nbFAILURE if the field,
		specified by the descriptor, is not present in the last packet processed.
		
		\note 
		fixed and varlen descriptors: if (len>0) then the descriptor is valid
		bit descriptor: if (the validity bit is 1) then the descriptor is valid
		allfields descriptor: if (the number of fields extracted > 0) then the descriptor is valid
	*/
	virtual int IsValid(_nbExtractedFieldsDescriptor *FieldDescriptor)=0;

	/*!
		\brief	returns nbSUCCESS if all fields that we want to extract are present in the packet.

		\return nbSUCCESS if all descriptors are valid, nbFAILURE if at least one field to
		extract is not present in the packet.

		\note This function is a shortcut for having to call the IsValid() for all the field descriptors.
	*/
	virtual int IsComplete()=0;


protected:
	/*!
		\brief	fills the Descriptor vector structure with the info partion data.
		this method analyzes the info partition with the information of the _nbExtractedFieldsDescriptorVector
		and it fills the _nbExtractedFieldsDescriptorVector structures (validity - offset - length - value - etc)

		\note 
		for \93allfields\94 descriptor it creates a new _nbExtractedFieldsDescriptorVector structure and 
		it fills it with the information about \93allfields\94. 
        Then it assigns the pointer \93fieldVct\94 of the corresponding Descriptor with the structure created.

	*/
	virtual void FillDescriptors()=0;
};



/*!
	\}
*/

