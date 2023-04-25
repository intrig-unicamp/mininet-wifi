/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*!
	\file nbee_initcleanup.h

	Keeps the definitions and exported functions related to the initialization and the cleanup of the NetBee library.
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


/*!
	\defgroup NetBeeInitCleanup NetBee Initialization and Cleanup
*/

/*! \addtogroup NetBeeInitCleanup
	\{
*/



/*********************************************************/
/*  Version of NetBee, NetPDL, ....                      */
/*********************************************************/

//! Major version of the NetPDL language
#define NETPDL_VERSION_MAJOR	0
//! Minor version of the NetPDL language
#define NETPDL_VERSION_MINOR	2
//! Major version of the NetBee library
#define NETBEE_VERSION_MAJOR	0
//! Major version of the NetBee library
#define NETBEE_VERSION_MINOR	2
//! Current revision code of the NetBee library
#define NETBEE_VERSION_REVCODE	13
//! Release date (dd-mm-yyyy) of the current NetBee library
#define NETBEE_VERSION_DATE		"22-08-2012"	/* day-month-year */



//! Structure that contains version number related to the NetBee library
typedef struct _nbVersion
{
	//! Major version of the NetBee library.
	int Major;
	//! Minor version of the NetBee library.
	int Minor;
	//! Current revision code of the NetBee library.
	int RevCode;
	//! Release date (dd-mm-yyyy) of the NetBee library.
  const char* Date;
	//! Major version of the NetPDL language supported by this library.
	int SupportedNetPDLMajor;
	//! Minor version of the NetPDL language supported by this library.
	int SupportedNetPDLMinor;
	//! Creator of the NetPDL database (if available, NULL otherwise).
	char* NetPDLCreator;
	//! Release date (dd-mm-yyyy) of the NetPDL database (if available, NULL otherwise).
	char* NetPDLDate;
	//! Major version of the NetPDL language, as written in the NetPDL database (if available, '0' otherwise).
	int NetPDLMajor;
	//! Minor version of the NetPDL language, as written in the NetPDL database (if available, '0' otherwise).
	int NetPDLMinor;
} _nbVersion;



/*!
	\brief It initializes the NetBee library.

	This call must be invoked before any actions on the NetBee library.
	This function allocates some global variables that are required by some NetBee classes.
	After using the NetPDL engine, the user MUST call the nbCleanup() to clean
	all the allocated stuff.

	\param NetPDLFileLocation: string that points to the file containing the NetPDL description.
	This parameter can be NULL: in this case, the internal NetPDL description embedded in this
	engine will be used. However this description may not be up to date.

	\param Flags: type of the NetPDL protocol database we want to load. It can be 'nbPROTODB_FULL',
	(default) in case the NetPDL database we want to load is a complete database including visualization 
	primitives, or 'nbPROTODB_MINIMAL' in case the database includes only protocol format and encapsulation.
	In the latter case, some of the features of the NetBee library may not be available (e.g. the
	complete packet decoding).
	Furthermore, nbPROTODB_VALIDATE will force the NetBee library to perform a sanity check
	against the XSD schema file.

	\param ErrBuf: user-allocated buffer that will keep a description of the error (if any).

	\param ErrBufSize: size of the previous buffer.

	\return nbFAILURE in case some error occurred, nbSUCCESS if the initialization is fine.

	\note In case the NetPDL protocol database pointed by 'NetPDLFileLocation' contains some
	errors, this function does not use the internal database.
	In order to inizialize the NetBee library with the internal database, the user has to call
	another this function again, setting 'NetPDLFileLocation' to NULL.

	\warning This function must be invoked ONCE per program.
*/
DLL_EXPORT int nbInitialize(const char *NetPDLFileLocation, int Flags, char *ErrBuf, int ErrBufSize);


/*!
	\brief It checks if the library has already been initialized.

	\return nbSUCCESS if the library has already been initialized with nbInitialize(), 
	nbFAILURE if the initialization has not been done (or if it failed).
*/
DLL_EXPORT int nbIsInitialized();


/*!
	\brief It deallocates the internal structure needed by the NetBee library.

	This call must be invoked when the NetBee library is no longer required.
	This function deallocates some global variables that are required by some NetBee classes.
	This function is the dual of nbInitialize().

	\warning This function must be invoked ONCE per program.
*/
DLL_EXPORT void nbCleanup();


/*!
	\brief It returns the version number of the current library.

	\return A structure containing the version of the current library.
*/
DLL_EXPORT struct _nbVersion* nbGetVersion();


/*!
	\brief It updates the NetPDL description currently used by the NetBee library.


	\param NetPDLFileLocation: string that points to the file containing the NetPDL description.
	This parameter can be NULL: in this case, the internal NetPDL description distributed with this
	library will be used. However this description may not be up to date.

	\param ErrBuf: user-allocated buffer that will keep a description of the error (if any).

	\param ErrBufSize: size of the previous buffer.

	\return nbFAILURE in case some error occurred, nbSUCCESS if the initialization is fine and the NetPDL 
	description used is the one provided by the user, nbWARNING if the initialization is fine and 
	the NetPDL description used is the one embedded within the NetBee library.

	\warning This function will deallocate some internal variables; therefore, the user must be
	carefully not to call any other method belonging to the NetBee library during the execution
	of this function.

	\note This function assumes that the NetPDL file is valid. It is under the responsibility
	of the programmer to check that the NetPDL file is compliant with the NetPDL formatting rules.
*/
DLL_EXPORT int nbUpdateNetPDLDescription(const char *NetPDLFileLocation, char *ErrBuf, int ErrBufSize);


/*!
	\brief It downloads a new copy of the NetPDL database and it returns the content of the file in an ascii buffer.

	This function is able to download a new copy of the NetPDL database from the online
	repository, and returns it into an ascii buffer. The downloaded file is checked for compatibility
	(i.e. it checks that the NetPDL version is supported); however, no checks are done on the file
	(it may happen that the file is corrupted). These checks are left to the programmer.

	\param NetPDLDataBuffer: buffer (allocated inside the library itself) that will contain the
	new NetPDL database. It is up to the user to save this buffer on a file, if needed.
	This buffer will be 'zero-terminated'. Please note that the user does not have to free()
	this buffer.

	\param ErrBuf: user-allocated buffer that will keep a description of the error (if any).

	\param ErrBufSize: size of the previous buffer.

	\return nbFAILURE in case some error occurred, nbSUCCESS if the download terminated successfully
	and the NetPDLDataBuffer contains valid data.

	\warning In order this function to succeed, you need a direct HTTP connectivity to the Internet.
*/
DLL_EXPORT int nbDownloadNewNetPDLFile(char** NetPDLDataBuffer, char* ErrBuf, int ErrBufSize);


/*!
	\brief It downloads a new copy of the NetPDL schema and it returns the content of the file in an ascii buffer.

	This function is able to download a new copy of the NetPDL schema from the online
	repository, and returns it into an ascii buffer.

	\param NetPDLSchemaDataBuffer: buffer (allocated inside the library itself) that will contain the
	downloaded XML Schema. It is up to the user to save this buffer on a file, if needed.
	This buffer will be 'zero-terminated'. Please note that the user does not have to free()
	this buffer.

	\param NetPDLSchemaMinDataBuffer: buffer (allocated inside the library itself) that will contain the
	XML Schema related to the 'minimum' version of NetPDL (i.e., no visualization primitives and such).
	It is up to the user to save this buffer on a file, if needed.
	This buffer will be 'zero-terminated'. Please note that the user does not have to free()
	this buffer.

	\param ErrBuf: user-allocated buffer that will keep a description of the error (if any).

	\param ErrBufSize: size of the previous buffer.

	\return nbFAILURE in case some error occurred, nbSUCCESS if the download terminated successfully
	and the NetPDLDataBuffer contains valid data.

	\warning In order this function to succeed, you need a direct HTTP connectivity to the Internet.
*/
DLL_EXPORT int nbDownloadNewNetPDLSchemaFile(char** NetPDLSchemaDataBuffer, char** NetPDLSchemaMinDataBuffer, char* ErrBuf, int ErrBufSize);


/*!
	\brief It downloads a new copy of the NetPDL database and it replaces the NetPDL file currently in use.

	This function is able to download a new copy of the NetPDL database from the online
	repository, and it replaces the specified NetPDL file with the new one. Moreover, it reload the
	NetBee library with the new protocol database.

	This function returns correctly only if the version of the new NetPDL file is compatible
	with the current release of the library, and if the file loads correctly. In this case
	the new description is saved in the specified NetPDL file (or in the default NetPDL file 
	in case the first parameter is NULL), and it is loaded in memory (so that the new description
	is immediately available for use). In case of errors, the library might fall back on the
	default NetPDL database distributed with the library itself.

	\param NetPDLFileLocation: string that points to the file containing the NetPDL description
	to be replaced. This parameter can be NULL: in this case, the internal NetPDL description 
	distributed with this library will be replaced.

	\param ErrBuf: user-allocated buffer that will keep a description of the error (if any).

	\param ErrBufSize: size of the previous buffer.

	\return nbFAILURE in case some error occurred, nbSUCCESS if the download terminated successfully
	and the NetPDLDataBuffer contains valid data.

	\warning In order this function to succeed, you need a direct HTTP connectivity to the Internet.

	\note This function can be called also without having the NetBee library initialized.
	This is useful in case you deleted the netpdl database and you want to get a new one
	from the online repository, and in this case the nbInitialize() will always fail
	without a valid NetPDL database.
*/
DLL_EXPORT int nbDownloadAndUpdateNetPDLFile(const char* NetPDLFileLocation, char* ErrBuf, int ErrBufSize);


/*!
	\brief It returns a pointer to a _nbNetPDLDatabase structure, which can be used to scan the NetPDL protocol database.

	This method is useful to return a pointer to a class that contains the entire NetPDL 
	protocol database.

	In order to see how to get access to the returned structure, please check at the 
	<a href="../nbprotodb/index.htm" target="_top">NetPDL Protocol Database Manager module</a>
	and the related examples.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return A pointer to a _nbNetPDLDatabase object, or NULL if something was wrong.
	In that case, the error message will be returned in the ErrBuf buffer.

	\warning The object returned by this class MUST not be deallocated by the user. This object will
	be automatically deallocated by the NetBee library when it is being closed.
*/
DLL_EXPORT struct _nbNetPDLDatabase* nbGetNetPDLDatabase(char *ErrBuf, int ErrBufSize);


/*!
	\}
*/

