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


#include <stdio.h>
#include <errno.h>		// for the errno variable
#include <string.h>		// for strtok, etc
#include <stdlib.h>		// for malloc(), free(), ...
#include <signal.h>		// for signal()
#include "svcsignals.h"
#include "../include/nbsvchelper.h"

#ifndef WIN32
#include <unistd.h>		// for exit()
#include <sys/wait.h>	// waitpid()
#endif


// Variable that contains the original command line values;
//   these are used in case of Win32 services
int orig_argc;
char **orig_argv;
struct _ServiceFunctionHandler ServiceFunctionHandler;


// Global variables
extern char *optarg;	// for getopt()
extern int opterr;		// for getopt()
extern int optind;		// for getopt()


// Function prototypes
void MainStopAndCleanup(int Signal);
void MainStopReloadAndRestart(int Signal);
int PrintErrorString(char* String);
#ifdef WIN32
int Win32ServiceStart(char *ProgramName);
#endif


//! Program main
int SvcHelperMain(int argc, char *argv[]);


void SvcHelperSetHandlers(_MainStartupHandler StartupHandler, _MainCleanupHandler CleanupHandler)
{
	ServiceFunctionHandler.StartupHandler= StartupHandler;
	ServiceFunctionHandler.CleanupHandler= CleanupHandler;
}


int SvcHelperMain(int argc, char *argv[])
{
int IsService= 0;					// Not null if the user wants to run this program as a daemon

	// Save command line parameters for future reference
	orig_argc= argc;
	orig_argv= argv;

	/* Use a for() because some implementation of getopt() reorder argv
	   depending on the option string passed. This way we're sure that the
	   Startup function will receive the /same/ argument vector that the
	   user has typed */
	for (++argv; *argv != NULL; ++argv)
		if (strcmp(*argv, "-d") == 0)
			IsService = 1;

	// Restore the original argument vector
	argv = orig_argv;

	// forking a daemon, if it is needed
	if (IsService)
	{
	#ifndef WIN32
	int ProgramID;

		// Generated with 'kill -HUP'
		signal(SIGNAL_STOP_RELOAD_AND_RESTART, MainStopReloadAndRestart);

		// Generated with 'kill -TERM'
		signal(SIGNAL_KILLALL, MainStopAndCleanup);

		// Unix Network Programming, pg 336
		if ( (ProgramID = fork() ) != 0)
			exit(0);		// Parent terminates

		// First child continues
		// Set daemon mode
		setsid();
		
		if ( (ProgramID = fork() ) != 0)
			exit(0);		// First child terminates

		// LINUX WARNING: the current linux implementation of pthreads requires a management thread
		// to handle some hidden stuff. So, as soon as you create the first thread, two threads are
		// created. Fom this point on, the number of threads active are always one more compared
		// to the number you're expecting

		// Second child continues
//		umask(0);
//		chdir("/");

		// Call the 'true' main
		ServiceFunctionHandler.StartupHandler(argc, argv);

		PrintErrorString("Exiting from the program.\n");
		exit(0);
	#else
		// We use the SIGABRT signal to kill the Win32 service
		// Here we register the callback function for this signal
		signal(SIGNAL_KILLALL, (void *) ServiceFunctionHandler.CleanupHandler);

		// If this call succeeds, it is blocking on Win32
		if ( Win32ServiceStart(argv[0]) != 1)
			PrintErrorString("Unable to start the service.\n");

		PrintErrorString("Exiting from the program.\n");

		// When the previous call returns, the entire application has to be stopped.
		exit(0);
	#endif
	}
	else	// Console mode
	{
#ifndef WIN32
		// Generated with 'kill -HUP'
		signal(SIGNAL_STOP_RELOAD_AND_RESTART, MainStopReloadAndRestart);
#endif
		// This is useless in case we're in console-mode on Windows; but never mind
		signal(SIGNAL_KILLALL, MainStopAndCleanup);

		// Enable the catching of Ctrl+C (or 'kill -INT')
		signal(SIGNAL_CONTROLC, MainStopAndCleanup);

		// Call the 'true' main
		ServiceFunctionHandler.StartupHandler(argc, argv);

		PrintErrorString("Exiting from the program.\n");

		exit(0);
	}
}



// Called in case we want to stop and cleanup the program
void MainStopAndCleanup(int Signal)
{
	PrintErrorString("MainStopAndCleanup() has been called.\n");

	if ( (Signal == SIGNAL_CONTROLC) || (Signal == SIGNAL_KILLALL) )
	{
		ServiceFunctionHandler.CleanupHandler();
		PrintErrorString("The program is closing.\n");
		exit(0);
	}

#ifdef Win32
	// If we're a Win32 service, we do not have to call exit()
	// because this will be handled internally by Windows.
	// What we have to do, is close all the stuff that is under the
	// responsibility of the program
	if (Signal == SIGNAL_KILLALL_NOEXIT)
	{
		ServiceFunctionHandler.CleanupHandler();
		PrintErrorString("MainStopAndCleanup(): the program is closing.\n");
		return;
	}
	else
	{
		PrintErrorString("MainStopAndCleanup(): the program is not closing (wrong signal).\n");
		return;
	}
#else
	PrintErrorString("MainStopAndCleanup(): the program is not closing (wrong signal).\n");
	return;
#endif
}


void MainStopReloadAndRestart(int Signal)
{
	PrintErrorString("MainStopReloadAndRestart() called.\n");
	ServiceFunctionHandler.CleanupHandler();
	ServiceFunctionHandler.StartupHandler(orig_argc, orig_argv);
}




int PrintErrorString(char* String)
{
#ifdef _DEBUG
FILE* LogFile;

	LogFile= fopen("log.txt", "a+");

	if (LogFile == NULL)
		return -1;

	fprintf(LogFile, "%s\n", String);
	fclose(LogFile);

	fprintf(stderr, String);
#endif

	return 0;
}


int GetOpt(int argc, char **argv, char *FormatString, char **ParameterArgument)
{
int RetVal;

	RetVal= getopt(argc, argv, FormatString);
	*ParameterArgument= optarg;

	return RetVal;
}
