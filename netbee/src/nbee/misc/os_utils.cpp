/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>

#include <nbsockutils.h>
#include "os_utils.h"
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"

#ifndef WIN32
#include <dlfcn.h>
#include "../../../include/nbee_initcleanup.h"
#endif

#ifdef NBEE_EXPORTS
	#define NBEE_SHAREDLIB_FILE_NAME "nbee.dll"
#else
	#define NBEE_SHAREDLIB_FILE_NAME "libnbee.so"
#endif

//! Default file containing the NetPDL database, in case no files are given.
#define NETPDL_DEFAULT_FILE "netpdl.xml"



int LocateLibraryLocation(char* NetPDLFileLocation, int NetPDLFileLocationSize, char *ErrBuf, int ErrBufSize)
{
int NetBeeLocationPathLength;
char *TempPtr;
#ifdef WIN32
HMODULE ModuleHandle;

	// In case 'NetPDLFileLocation' does not contain a valid name, let's using the description
	// that ships with the NetBee library
	// The XML database file is in the same folder as the DLL; so, let's get the path to the DLL itself
	// and we'll be able to locate the XML file immediately.

	// Get an handle to the current DLL
	if ( (ModuleHandle= GetModuleHandleA(NBEE_SHAREDLIB_FILE_NAME)) == NULL)
	{
		nbGetLastErrorEx(__FILE__,__FUNCTION__, __LINE__, "Cannot locate the '" NBEE_SHAREDLIB_FILE_NAME "' shared library: ", ErrBuf, ErrBufSize);
//		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
//					  FORMAT_MESSAGE_MAX_WIDTH_MASK,
//					  NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//					  ErrBuf, ErrBufSize, NULL);

		return nbFAILURE;
	}

	NetBeeLocationPathLength= GetModuleFileNameA(ModuleHandle, NetPDLFileLocation, NetPDLFileLocationSize);

	if (NetBeeLocationPathLength == 0)
	{
		nbGetLastErrorEx(__FILE__,__FUNCTION__, __LINE__, "Cannot locate the '" NBEE_SHAREDLIB_FILE_NAME "' shared library: ", ErrBuf, ErrBufSize);
//		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
//					  FORMAT_MESSAGE_MAX_WIDTH_MASK,
//					  NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//					  ErrBuf, ErrBufSize, NULL);

		return nbFAILURE;
	}

#elif defined(linux)

	void* dllHandle = dlopen(NBEE_SHAREDLIB_FILE_NAME, RTLD_LAZY);
	if (dllHandle == NULL)
	{
		nbGetLastErrorEx(__FILE__,__FUNCTION__, __LINE__, "Cannot locate the '" NBEE_SHAREDLIB_FILE_NAME "' shared library: ", ErrBuf, ErrBufSize);
//		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Cannot locate the %s shared library.\n", NBEE_SHAREDLIB_FILE_NAME);
		return nbFAILURE;
	}

	Dl_info info;
	memset(&info, 0, sizeof(Dl_info));

	// nbIsInitialized is just a pointer to a function exported by the library
	if ( dladdr( (const void*) &nbIsInitialized, &info ) )
	{
		NetBeeLocationPathLength = strlen(info.dli_fname);
		if (NetBeeLocationPathLength == 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Cannot locate the '%s' shared library.\n", NBEE_SHAREDLIB_FILE_NAME);
			return nbFAILURE;
		}
		else
			sstrncpy(NetPDLFileLocation, (char *) info.dli_fname, NetPDLFileLocationSize);
	}
	else
	{
		nbGetLastErrorEx(__FILE__,__FUNCTION__, __LINE__, "Cannot locate the '" NBEE_SHAREDLIB_FILE_NAME "' shared library: ", ErrBuf, ErrBufSize);
//		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Cannot locate the '%s' shared library: %s.\n", NBEE_SHAREDLIB_FILE_NAME, dlerror() );
		return nbFAILURE;
	}

	if (NetBeeLocationPathLength == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Cannot locate the '%s' shared library.\n", NBEE_SHAREDLIB_FILE_NAME);
		return nbFAILURE;
	}

#else

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, "Cannot locate the '%s' shared library; this Operating System is not supported. \n", NBEE_SHAREDLIB_FILE_NAME);
	return nbFAILURE;

#endif

	TempPtr= (char *)strstr(NetPDLFileLocation, NBEE_SHAREDLIB_FILE_NAME);
	strncpy(TempPtr, NETPDL_DEFAULT_FILE, NetPDLFileLocationSize - (TempPtr - NetPDLFileLocation));
	NetPDLFileLocation[NetPDLFileLocationSize - 1]= 0;

	return nbSUCCESS;
}

