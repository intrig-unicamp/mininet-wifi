/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/*!
	\file nbprotodb.h

	This file defines the functions that are exported by the NetPDL Protocol Database module.
*/


// Allow including this file only once
#pragma once


#ifdef PROTODB_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif


// For the internal structure related to each NetPDL element
#include "nbprotodb_exports.h"
#include "nbprotodb_defs.h"




/*!
	\mainpage NetPDL Protocol Database Manager

	This set of functions is used to load the NetPDL description contained in an
	XML file, parse it, and create the corresponding description in memory.

	The exported functions are very simple:
	- nbProtoDBXMLLoad(): Load an XML file containing the NetPDL database and
	create an internal representation of the file itself
	- nbProtoDBXMLCleanup(): clean all the internal structures allocated at the 
	previous step.

	The biggest advantage of this library is that it creates a more user-friendly 
	representation of the NetPDL database, structured as a set of 'C' structures.
	All the database is organized under the form of the struct _nbNetPDLDatabase.

	For details about accessing data contained in the NetPDL database, please
	refer to the documentation of the struct _nbNetPDLDatabase.
*/

// Planned:
//- nbProtoDBXMLSave(): save the internal representation to a binary file,
//	which provides a more compact way to store the NetPDL database. This binary file 
//	is still portable across different platforms.




/*!
	\brief It creates automatically a mapping between NetPDL element names and their IDs.
	The result is something like:
	<pre>
		enum
		{
			nbNETPDL_IDEL_DUMMYFIRST= -1,
			nbNETPDL_IDEL_NETPDL,
			...
		}
	</pre>
	This is a trick that uses a &#35;define immediately followed by an &#35;undef.
	This means that the IDs for NetPDL elements starts at 0, and then go on...
*/
#ifndef NBNETPDLELEMENTLIST
enum nbNetPDLElementList {
	nbNETPDL_EL_DUMMYFIRST= -1,
	#define nbNETPDL_ELEMENT(ElementName, ElementStringID, ElementCodeID, PtrToAllocationFunction, PtrToCrossLinkFunction, PtrToCleanupFunction, PtrToSerializeFunction) ElementCodeID,
	#include "nbprotodb_elements_xml.h"
	#undef nbNETPDL_ELEMENT
};
#define NBNETPDLELEMENTLIST
#endif


/*!
	\brief It creates automatically a mapping between NetPDL element names and their string ID.

	The result is something like:
	<pre>
		const char * const nbNETPDL_EL_NETPDL="netpdl";
		const char * const nbNETPDL_EL_PROTO="protocol";
		...
	 </pre>
	This is a trick that uses a &#35;define immediately followed by an &#35;undef.
	The result looks closer to a &#35;define, since the value assigned to the variable cannot be changed.
*/
#define nbNETPDL_ELEMENT(ElementName, ElementStringID, ElementCodeID, PtrToAllocationFunction, PtrToCrossLinkFunction, PtrToCleanupFunction, PtrToSerializeFunction) const char * const ElementStringID=ElementName;
#include "nbprotodb_elements_xml.h"
#undef nbNETPDL_ELEMENT




/*!
	\brief Opens an XML file containing the NetPDL description and creates an easier representation in memory.

	\param FileName: name of the XML file that contains the NetPDL description.
	\param Flags: Type of the NetPDL protocol database we want to load. It can assume the values defined in #nbProtoDBFlags.
	\param ErrBuf: user-allocated buffer that will contain the error message (if any).
	\param ErrBufSize: size of the previous buffer.

	\return The _nbNetPDLDatabase structure, or NULL if something fails.
	In the latter case, the error message is stored in the ErrBuf variable.
	A pointer to this structure (which is stored internally) is returned to the 
	caller for its convenience; however, please note that you should not modify any
	member of this structure, or the NetPDL database may become incoherent.

	\note The returned structure must be cleared with the nbNetPDLDatabaseCleanup().

	\note Although some other functions may exists in other libraries, the user must be careful
	in order to invoke the cleanup function which belongs to the same library of the function
	user to create the database in memory.

	\warning Although this function returns a pointer to the _nbNetPDLDatabase, data related
	to it is also stored internally. Therefore, a call to this function MUST be followed
	by a call to nbProtoDBXMLCleanup() in order to clean internal data.
	Two consecutive call to nbProtoDBXMLLoad() without nbProtoDBXMLCleanup() 
	will cause memory leaks, since the structure allocated the first time will not be cleared.
*/
DLL_EXPORT struct _nbNetPDLDatabase *nbProtoDBXMLLoad(const char *FileName, int Flags, char *ErrBuf, int ErrBufSize);



/*!
	\brief Deallocates all the structures contained in the NetPDLDatabase (in memory).

	\note Although some other functions may exists in other libraries, the user must be careful
	in order to invoke the cleanup function which belongs to the same library of the function
	user to create the database in memory.
*/
DLL_EXPORT void nbProtoDBXMLCleanup();


/*
	\brief Saves the current NetPDL database on a file, for faster access.
	This function is used in order to save the NetPDL data to an intermediate representation
	which does not require XML to be parsed. This helps speeding up the initial NetPDL 
	processing time.

	\param FileName: name of the binary file that will contain the NetPDL description.
	This file is platform-independent, hence it can be moved from a mathine to another.
	\param ErrBuf: user-allocated buffer that will contain the error message (if any).
	\param ErrBufSize: size of the previous buffer.
	
	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
	In the latter case, the error message is stored in the ErrBuf variable.
*/
//DLL_EXPORT int nbProtoDBXMLSave(const char *FileName, char *ErrBuf, int ErrBufSize);


