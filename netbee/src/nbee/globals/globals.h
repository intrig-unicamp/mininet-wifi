/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

// this define if host is LITTLE_ENDIAN o BIG_ENDIAN
#include <stdlib.h>

// Please remember that the same #define is present in the "NetBee.h" header
// So, please be careful to keep them aligned
#ifndef nbSUCCESS
	#define nbSUCCESS 0		//!< Return code for 'success'
	#define nbFAILURE -1	//!< Return code for 'failure'
	#define nbWARNING -2	//!< Return code for 'warning' (we can go on, but something wrong occurred)
#endif

//! Macro that checks if a pointer is valid; if so, it deletes the pointer and sets the variable to NULL.
#define DELETE_PTR(ptr) {if (ptr) delete ptr; ptr= NULL;}
#define DELETE_ARRAY_PTR(ptr) {if (ptr) delete[] ptr; ptr= NULL;}
#define FREE_PTR(ptr) {if (ptr) free(ptr); ptr= NULL;}

//! Standard string for allocation error
// FULVIO: extend this string throughout the entire library
#define ERROR_ALLOC_FAILED "Memory allocation failed: cannot create the requested object"

//! Maximum string length allowed in this NetPDL engine (used for messages and such).
#define NETPDL_MAX_STRING 2048
//! Maximum packet length allowed in this NetPDL engine. It should be enough to support jumbo frames on GigaEthernet.
//#define NETPDL_MAX_PACKET 10240
#define NETPDL_MAX_PACKET 65536


#if (defined(_WIN32) && defined(_M_IX86) && defined (_MSC_VER))
	// If we're on Windows and we're compiling on x86, we're using a little endian machine
	#define HOST_IS_LITTLE_ENDIAN
#elif defined(_WIN32) && defined(_X86_)
	#define HOST_IS_LITTLE_ENDIAN
#elif defined(_WIN64) && defined(_X64_)
	#define HOST_IS_LITTLE_ENDIAN
#elif defined(LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__) || defined(_LITTLE_ENDIAN)
	#define HOST_IS_LITTLE_ENDIAN
#elif defined(BIG_ENDIAN) || defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)
	#define HOST_IS_BIG_ENDIAN
#else
	#define HOST_IS_LITTLE_ENDIAN
	// #warning Not sure which OS and platform; setting as little endian (such as in x86)
#endif





