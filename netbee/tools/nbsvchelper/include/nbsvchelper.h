/*
 * Copyright (c) 2002 - 2011
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Politecnico di Torino nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#pragma once


#ifdef SVCHELPER_EXPORTS
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT __declspec(dllexport)
	#endif
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
// Defines also getopt(), which does not exist in Windows
int getopt(int nargc, char * const *nargv, const char *ostr);

// let the main program know about optind to skip all
// options and to get first non option argument
extern int optind;
#endif


typedef void (*_MainStartupHandler)(int, char **);
typedef void (*_MainCleanupHandler)(void);



/*!
	\brief Structure that contains the startup and cleanup handlers.
*/
struct _ServiceFunctionHandler
{
	//! Pointer to the function that will initialize user code (this function will be outside the DLL).
	_MainStartupHandler StartupHandler;
	//! Pointer to the function that will clean user code (this function will be outside the DLL).
	_MainCleanupHandler CleanupHandler;
};

/*!
	\brief Defines the handlers, so that the DLL know what it has to call.
	\param StartupHandler: startup handler (in the user code, outside the DLL). This function will be called
	as soon as the program is started (both as a daemon and as a standalone program).
	\param CleanupHandler: cleanup handler (in the user code, outside the DLL). This function will be called
	when the program is going to be closed (both as a daemon and as a standalone program).
*/
void SvcHelperSetHandlers(_MainStartupHandler StartupHandler, _MainCleanupHandler CleanupHandler);


/*!
	\brief Moves the control to the "main" function, inside the DLL.

	\param argc: the argc parameter of standard C programs.
	\param argv: the argv parameter of standard C programs.
*/
int DLL_EXPORT SvcHelperMain(int argc, char *argv[]);

int DLL_EXPORT GetOpt(int argc, char **argv, char *FormatString, char **ParameterArgument);


#ifdef __cplusplus
}
#endif
