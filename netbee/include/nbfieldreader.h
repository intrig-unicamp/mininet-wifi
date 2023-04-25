/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

#ifdef NETPDL_EXPORTS
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT __declspec(dllexport)
	#endif
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif

#include <nbpflcompiler.h>

#include <string>
#include <list>
#include <map>
using namespace std;


typedef map<string,int> nameTable;

//! This structure defines a STL list of STL strings. Please refer to the \ref fieldextractor.cpp sample for using this class.
typedef list<string> nbFieldNameList;


/*!
	\brief This class allows the user to parse the list of fields extracted from a network packet. 
	
	Typically, nbExtractedFieldsReader class is used when the nbPacketEngine class 
	is configured to extract fields from the packet. 
	The nbExtractedFieldsReader contains the data required to parse the list of fields extracted 
	by the nbPacketEngine class and can be used to get field values, their offsets and such. 
	All fields information are stored in the _nbFieldDescriptor structures.

	After the processing of the packet, the nbPacketEngine class uses the protected method 
	FillDescriptors() to set the structures with the current data in the info partition.
	
	Other methods offer the opportunity to check the validity of the current data. Besides, 
	it’s possible to get the list of field names and ask for a specific field calling the corresponding method. 
	
	To get the _nbFieldDescriptor structure that contain field information use the following methods:
	•	If the user needs to have all fields structures, use the GetField() method without any parameters, 
		it returns a DescriptorVector structure that contains all the data concern the last packet processed by the nbPacketEngine class.
	
	•	If only one field is necessary, use the GetField() overloading methods: if the parameter 
		is an integer the _nbFieldDescriptor structure returned corresponds to the field position 
		in the extraction list; if the parameter is a string the _nbFieldDescriptor structure returned 
		corresponds to the field that matches the name in the right format (protoname.fieldname).
	
	To know if a _nbFieldDescriptor structure contains valid data or if all extracted fields are present in the last packet processed, use the following methods: 
	•	The method IsValid() let to know if the field, corresponding to a _nbFieldDescriptor structure, 
		is present in the last packet processed by the nbPacketEngine class. If the return value is nbFAILURE the data are expired.
	•	If the user needs to know if all fields are present in the packet, use the IsComplete() method, 
		that returns the same result of a cycle of IsValid() method on all _nbFieldDescriptor structures.
	
	If the user doesn’t kwon the fields’ name, it’s possible to get them using the GetFieldNames() method that returns a list of them. The order of this list corresponds to the extraction one.

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
		\brief Returns a pointer to the Descriptor of the field specified by name.

		\param FieldName: Field name referring to a member of the nbFieldNameList returned by the method
						  GetFieldNames(). Format = protoname.fieldname

		\return A pointer to a _nbFieldDescriptor if no error occurred, NULL if the FieldName
		doesn't correspond to any _nbFieldDescriptor name.
		
		\note User get access to the member using  the properties of the '_nbFieldDescriptor' class
	*/
	virtual _nbFieldDescriptor *GetField(string FieldName)=0;

	/*!
		\brief Returns a pointer to the Descriptor of the field specified by index.

		\param Index: position of the field in the extraction list present in the
		              ExtractFields() action.

		\return A pointer to a _nbFieldDescriptor object if no error occurred, NULL if the index
		doesn't correspond to any _nbFieldDescriptor.
		
		\note User get access to the member using  the properties of the '_nbFieldDescriptor' class.
	*/
	virtual _nbFieldDescriptor* GetField(int index)=0;

	/*!
		\brief Returns a pointer to the DescriptorVector that contains all the fields extracted.

		\return A pointer to a DescriptorVector if no error occurred, NULL otherwise.
		if there isn't the ExtractFields action in the filter, the DescriptorVector
		is empty ( DescriptorVector->n=0 )

		\note User can scan the vector of _nbFieldDescriptor objects: DescriptorVector->ExFieldDescriptor
		and get access to each member using the properties of the '_nbFieldDescriptor' class.
	*/

	virtual DescriptorVector* GetField()=0;

	/*!
		\brief Returns the list of the field names that will be extracted. 
		       The name format is protoname.fieldname 

		\return A STL 'list' of STL 'string' elements if isn't empty, NULL otherwise
			
		\note User can scan the list using the functions defined for the 'list' STL class, and get
		access to each member using the properties of the 'string' STL class.
	*/
	virtual nbFieldNameList GetFieldNames()=0;

	/*!
		\brief	Returns nbSUCCESS if the field is present in the packet and it's extracted.
		        BitField with valid bit at 1; other field with Len > 0.

		\param	Des: pointer to the _nbFieldDescriptor structure of the field to verify.

		\return nbSUCCESS if the descriptor is valid, nbFAILURE if the field,
		specified by the descriptor, isn't present in the last packet processed.
		
		\note 
		fixed and varlen descriptors: if (len>0) then the descriptor is valid
		bit descriptor: if (the validity bit is 1) then the descriptor is valid
		allfields descriptor: if (the number of fields extracted > 0) then the descriptor is valid
		
		User can verify the validity of the descriptor to kwown if in the structure (_nbFieldDescriptor)
		there are valid information.
	*/
	virtual int IsValid(_nbFieldDescriptor *Des)=0;

	/*!
		\brief	returns nbSUCCESS if all fields to extract are present in the packet.

		\return nbSUCCESS if all descriptors are valid, nbFAILURE if at least one field to
		extract isn't present in the packet.

		\note User can verify the validity of all Descriptors to kwown if all fields are extracted
		and all structures contain valid informations.
	*/
	virtual int IsComplete()=0;

protected:
	/*!
		\brief	fills the Descriptor vector structure with the info partion data.
		this method analyzes the info partition with the information of the DescriptorVector
		and it fills the DescriptorVector structures (validity - offset - length - value - etc)

		\note 
		for “allfields” descriptor it creates a new DescriptorVector structure and 
		it fills it with the information about “allfields”. 
        Then it assigns the pointer “fieldVct” of the corresponding Descriptor with the structure created.

	*/
	virtual void FillDescriptors()=0;
};

/*!
	\brief A pointer to real object that has the same interface of the nbExtractedFieldsReader abstract class, or NULL in case of error.

	\param	fieldsVct		DescriptorVector to initialize the FieldReader. Typically is returned
	                        by the compiler.
	\param	dataInfo		pointer to the info partion, where the extraction data are stored
	\param	compiler		pointer to the nbpflcompiler

	\return a nbExtractedFieldsReader object if no errors occurred, NULL otherwise.
*/
nbExtractedFieldsReader DLL_EXPORT *nbAllocateExtractedFieldsReader(DescriptorVector* fieldsVct, unsigned char* dataInfo,nbNetPFLCompiler	*compiler);


/*!
	\brief De-allocates the nbExtractedFieldsReader Object

	\param	fieldReader 	pointer to the object that has to be deallocated
*/
void DLL_EXPORT nbDeallocateExtractedFieldsReader(nbExtractedFieldsReader *fieldReader);
