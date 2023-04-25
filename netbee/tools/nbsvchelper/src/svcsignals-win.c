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
#include <signal.h>
#include "../include/nbsvchelper.h"
#include "svcsignals.h"

#define	_WINSOCKAPI_
#include <windows.h>


extern int orig_argc;
extern char **orig_argv;
extern struct _ServiceFunctionHandler ServiceFunctionHandler;


extern int PrintErrorString(char* String);


// Variables that keep the status of the service,
//   which are shared among most of the functions
SERVICE_STATUS_HANDLE ServiceStatusHandle;
SERVICE_STATUS ServiceStatus;


// Function prototypes
void Win32ServiceGetError(char *CallerString, char *Message, int MessageBufSize);
void WINAPI Win32ServiceMain(DWORD argc, char **argv);



int Win32ServiceStart(char *ProgramName)
{
char Message[1024];
int RetCode;

	SERVICE_TABLE_ENTRY ServiceTableEntry[] =
	{
		{ ProgramName, Win32ServiceMain },
		{ NULL, NULL }
	};

	// This call is blocking. A new thread is created which will launch
	// the Win32ServiceMain() function
	if ( (RetCode = StartServiceCtrlDispatcher(ServiceTableEntry)) == 0)
	{
		Win32ServiceGetError("StartServiceCtrlDispatcher()", Message, sizeof(Message));
		PrintErrorString(Message);
	}

	return RetCode; // FALSE if this is not started as a service
}


void Win32ServiceGetError(char *CallerString, char *Message, int MessageBufSize)
{
char String[1024];
int RetVal;

	RetVal= GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
				  FORMAT_MESSAGE_MAX_WIDTH_MASK,
				  NULL, RetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPSTR) String, sizeof(String), NULL);

	_snprintf(Message, MessageBufSize, "%s failed with error %d: %s", CallerString, RetVal, String);
}



void WINAPI Win32ServiceControlHandler(DWORD Opcode)
{
	ServiceStatus.dwWin32ExitCode= 0;
	ServiceStatus.dwCheckPoint= 0;
	ServiceStatus.dwWaitHint= 0;

	switch(Opcode)
	{
		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			ServiceStatus.dwCurrentState= SERVICE_STOPPED;
			raise(SIGNAL_KILLALL);
			SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
			break;

		// Pause and continue are not supported at this time
/*		case SERVICE_CONTROL_PAUSE:
			service_status.dwCurrentState= SERVICE_PAUSED;
			SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
			break;
		
		case SERVICE_CONTROL_CONTINUE:
			service_status.dwCurrentState= SERVICE_RUNNING;
			SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
			break;
*/
		case SERVICE_CONTROL_INTERROGATE:
			// Fall through to send current status.
			SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
			break;
	}

	// Send current status.
	return;
}



// ServiceMain is called when the Service Control Manager wants to
// launch the service.  It should not return until the service has
// stopped. To accomplish this, for this example, it waits blocking
// on an event just before the end of the function.  That event gets
// set by the function which terminates the service above.  Also, if
// there is an error starting the service, ServiceMain returns immediately
// without launching the main service thread, terminating the service.
//
// This function has a limited view of the command line: it receives only the
// service name and NOT the command line specified in the service creation string.
// For instance, the window that allows launching a service, has a box in
// which you can type additiona parameters. These parameters are passed to this 
// function. However, these parameters CANNOT be saved, therefore can be used
// only as 'one shot' parameters.
// The only thing we receive here, is just the service name (argv[0]).
//
// http://www.commsoft.com/services.html
void WINAPI Win32ServiceMain(DWORD argc, char **argv)
{
	ServiceStatusHandle= RegisterServiceCtrlHandler(argv[0], Win32ServiceControlHandler);

	if (ServiceStatusHandle == NULL)
		return;

	ServiceStatus.dwServiceType= SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
	ServiceStatus.dwCurrentState= SERVICE_RUNNING;
	// Pause and continue are not supported at this time
	ServiceStatus.dwControlsAccepted= SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN; // | SERVICE_ACCEPT_PAUSE_CONTINUE;
	ServiceStatus.dwWin32ExitCode= 0;
	ServiceStatus.dwServiceSpecificExitCode= 0;
	ServiceStatus.dwCheckPoint= 0;
	ServiceStatus.dwWaitHint= 0;

	SetServiceStatus(ServiceStatusHandle, &ServiceStatus);

	// Call startup handler in the user program
	// The startup handler is called with the original values of the command line
	ServiceFunctionHandler.StartupHandler(orig_argc, orig_argv);
}


