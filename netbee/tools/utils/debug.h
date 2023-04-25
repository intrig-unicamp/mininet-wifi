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


#ifdef __cplusplus
extern "C" {
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
	write anything.
*/
extern int errorsnprintf(char *File, char *Function, int Line, char* Buffer, int BufSize, char *Format, ...);


/*!
	\brief Prints an error message.

	This function is used to print an error message. The program can define
	where the error message is printed by simply changing this function.

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
extern void nbPrintError(char *File, char *Function, int Line, char* Buffer);


#ifdef __cplusplus
}
#endif

