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

#include <string>
#include <list>
#include <map>
#include <nbee.h>
using namespace std;

typedef map<string,int> nameTable;	
typedef list<string> nameList;	

/*!
	\brief This class represents the Field Reader
*/
class DLL_EXPORT FieldReader
{

private:
    int N;
	Descriptor **NDescriptor;
	nameTable Ntable;
	nameList fieldNames;
	unsigned char * DataInfo;
	
	
	/*!
		\brief	returns the pointer descriptor of the field specified by the index.

		\param	index index of the field in the NDescriptor vector
		
		\return a pointer to a Descriptor
	*/
	Descriptor* GetFieldbyIndex(int index);

public:
	/*!
		\brief	Object constructor

		\param	fields			reference to the vector of descriptor of fields that can be extracted
		\param	N				number of fields in the fields vector.
		\param	dataInfo		reference to the Info Partition.

		\return nothing
	*/
	FieldReader(Descriptor_vct fields_vct, unsigned char* dataInfo);

	/*!
		\brief	Object destructor
	*/
	~FieldReader(void);

	/*!
		\brief	returns the pointer descriptor of the field specified by the string name.

		\param	name Field name
		
		\return a pointer to a Descriptor if no error occurred, NULL otherwise
	*/
	Descriptor *GetField(string name);

	/*!
		\brief	returns a list of all field's name that can be extracted.
		
		\return a list of string
	*/
	nameList GetFieldNames();
};
