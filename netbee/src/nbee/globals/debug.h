/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


// Set this only if we're using a Microsoft Visual Studio compiler and we're going to trace memory leaks
// Warning: we cannot trace memory leaks related to the 'new' operator, because this requires the class to use
// a standard 'new' operator, which is not the case for Xerces classes. So, if we uncomment the line below,
// all xerces classes start complaining
// Therefore, malloc() and free() should be used when possible instead of 'new' and 'delete'
//
// Please note that in order to have memory leaks turned on, you MUST:
// - define _CRTDBG_MAP_ALLOC in your project
// - include this file in your main() file
// - call _CrtDumpMemoryLeaks() before leaving the program
// Please check the "Enabling Memory Leak Detection" page on MSDN.
#if defined(_DEBUG) && defined(_MSC_VER) && defined (_CRTDBG_MAP_ALLOC)
	#include <crtdbg.h>
	#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
	#define new DEBUG_NEW
#endif



#ifdef WIN32
// In Windows, please remember to define VC_EXTRALEAN in the project settings, in order to avoid
// including the most esoteric default windows headers
#include <winsock2.h>
#include <windows.h>
#else
#include <stdio.h>	// vsnprintf
#include <stdarg.h>	// va_list
#endif


/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


// FULVIO PER GIANLUCA ANCORA DA FARE : questo file e' diventato membro dei vari progetti (NetVM, NeBee, etc); sarebbe da pulire un po'.

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __ERR_MSG
	#define NETVM_ASSERT(cond, msg) \
		{\
			if (!(cond))\
			{\
				MessageBoxA(NULL,msg, "-- NETVM_ASSERTION --",0);\
				DebugBreak();\
			}\
		}
#else
	#define NETVM_ASSERT(cond, msg) { }
#endif


#ifdef ENABLE_NETVM_LOGGING
	#define LOG_NETIL_INTERPRETER 1
	#define LOG_JIT_BUILD_BLOCK_LVL2 2
	#define LOG_JIT_LISTS_DEBUG_LVL 3
	#define LOG_BYTECODE_LOAD_SAVE 4
	#define LOG_RUNTIME_CREATE_PEGRAPH 5

/*!
	\brief Logs a given message to the specified file

	This function logs the specific message on file. Files are predefined,
	and depend on the firt argument of the function.

	\param LogScope: scope of the log; each scope will originate a 
	different log file with a predefined name.
	
	\param Format: format-control string.
*/
extern void logdata(int LogScope, const char *Format, ...);

#endif



/*!
	\brief Formats an error string and it prints the error (only in debug mode)

	This function is basically an snprintf() enriched with some parameter needed to 
	print errors in a better way.

	This function guarantees that the returned string is always correctly 
	terminated ('\\0') and the number of characters printed does not exceed BufSize.

	This function calls nbPrintError() (in _DEBUG mode) in order to display
	the error message automatically.

	\param File: name of the file in which this function has been invoked.
	\param Function: function in which this function has been invoked.
	\param Line: line in which this function has been invoked.
	\param Buffer: user-allocated buffer for output.
	\param BufSize: size of the previous buffer.
	\param Format: format-control string.

	\return The number of characters printed (not counting the last '\\0' 
	character), or -1 if the size of the buffer was not enough to contain
	the resulting string.

	\note This function checks if the Buffer is NULL; in this case it does not
	write (and print) anything.
*/
extern int errorsnprintf(const char *File, const char *Function, int Line, char* Buffer, int BufSize, const char *Format, ...);


/*!
	\brief Formats an error string and it prints the error.

	This function is basically a printf() enriched with some parameter needed to 
	print errors in a better way.

	This function calls nbPrintError() in order to display
	the error message automatically.

	\param File: name of the file in which this function has been invoked.
	\param Function: function in which this function has been invoked.
	\param Line: line in which this function has been invoked.
	\param Format: format-control string.

	\return The number of characters printed (not counting the last '\\0' 
	character), or -1 if the size of the buffer was not enough to contain
	the resulting string.

	\note This function function will always print the error message, in the location
	specified in the nbPrintError() function. This behaviour is different from 
	errorsnprintf(), which may be silent in release mode.
*/
extern int errorprintf(const char *File, const char *Function, int Line, const char *Format, ...);


/*!
	\brief Prints an error message.

	This function is used to print an error message. The programmer can define
	where the error message has to be printed (i.e. screen, stdout, ...)
	simply by changing this function.

	Currently, it prints the error on standard error and in the 'output' 
	window of the Microsoft Visual Studio IDE.

	Please check the usage of errorsnprintf(), which may be useful to
	format the message string.

	In _DEBUG mode, the error message is enriched with the function name,
	the file in which the error occurred, and the line number. These 
	information are not printed in RELEASE mode,

	\param File: name of the file in which this function has been invoked.
	\param Function: function in which this function has been invoked.
	\param Line: line in which this function has been invoked.
	\param Buffer: buffer containing the error message to be printed.
*/
extern void nbPrintError(const char *File, const char *Function, int Line, char* Buffer);


// [ds] Added function and constants to print debugging information
#define	DBG_TYPE_INFO		0
#define	DBG_TYPE_WARNING	1
#define	DBG_TYPE_ERROR		2
#define	DBG_TYPE_ASSERT		3


/*!
	\brief Prints a debugging message.

	\param Msg: message to print.
	\param DebugType: defines the type of debugging line to print (DBG_TYPE_xxx).
	\param File: name of the file in which this function has been invoked.
	\param Function: function in which this function has been invoked.
	\param Line: line in which this function has been invoked.
	\param Indentation: indentation level.
*/
extern void nbPrintDebugLine(const char* Msg, unsigned int DebugType, const char *File, const char *Function, int Line, unsigned int Indentation);

#ifndef WIN32
	#ifndef __func__
		#define __FUNCTION__ " __func__ not available"
	#else
		#define __FUNCTION__  "%s %s", __func__,
	#endif
#endif

#ifdef __cplusplus
}
#endif

