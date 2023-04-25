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
#include <conio.h>
#include "../utils/utils.h"
#include "../utils/debug.h"


int ServiceDelete(LPCTSTR ServiceName);
int ServiceStop(LPCTSTR ServiceName);
int ServiceStart(LPCTSTR ServiceName);
int ServiceCreate(LPCTSTR ServiceName,LPCTSTR ServiceDescriptionShort,LPCTSTR ServiceDescriptionLong,LPCTSTR ServicePath);
int ServiceChangeStartType(LPCTSTR ServiceName, DWORD StartType);



typedef WINADVAPI BOOL  (WINAPI *MyChangeServiceConfig2)(
  SC_HANDLE hService,  // handle to service
  DWORD dwInfoLevel,   // information level
  LPVOID lpInfo        // new data
);


void Usage()
{
char String[]= \
	"\nTool for installing, uninstalling and managing Win32 services\n\n"	\
	"Usage:\n"	\
	"  nbsvcinstaller [-s] [-x] [-u] [-i] [-r] [-a] [-d]\n\n"	\
	"Where:\n"	\
	"   -s ServiceName: starts service 'ServiceName'\n"		\
	"   -x ServiceName: stops service 'ServiceName'\n"		\
	"   -u ServiceName: uninstalls service 'ServiceName'\n"		\
	"   -i ServiceName DescrShort DescrLong CommandLine: \n"	\
	"                   installs service 'ServiceName' with following parameters:\n"	\
	"                   - DescrShort: short description of the service\n"	\
	"                   - DescrLong: long description of the service\n"	\
	"                   - CommandLine: command line needed to launch the service\n"	\
	"                     (please note the it can include environment variables\n"	\
	"                     such as %%ProgramFiles%%)\n"	\
	"   -r ServiceName: uninstalls and reinstalls service 'ServiceName'\n"					\
	"   -a ServiceName: changes the start-type of 'ServiceName' to auto-start\n"			\
	"   -d ServiceName: changes the start-type of 'ServiceName' to demand-start\n\n"			\
	"   -q ServiceName: check if the status of 'ServiceName' \n\n"			\
	"Example (How-to quickly install and launch a service from command line):\n"			\
	"   nbsvcinstaller -i siouxhttp siouxhttp siouxhttp \"c:\\analyzer\\siouxhttp.exe -d\"\n"	\
	"   nbsvcinstaller -s siouxhttp\n\n";

	printf(String);
}


int main(int argc, char **argv)
{
	if (argc < 2)
	{
		Usage();
		return -1;
	}

	// Getting the proper command line options
	switch(argv[1][1])
	{
	case 's':
		return ServiceStart(argv[2]);

	case 'x':
		return ServiceStop(argv[2]);

	case 'u':
		return ServiceDelete(argv[2]);

	case 'i':
		if (argc < 6)
		{
			Usage();
			return -1;
		}

		return ServiceCreate(argv[2], argv[3], argv[4], argv[5]);

	case 'r':
		if (argc < 6)
		{
			Usage();
			return -1;
		}

		(void)ServiceDelete(argv[2]);
		Sleep(100);
		return ServiceCreate(argv[2], argv[3], argv[4], argv[5]);

	case 'a':
		return ServiceChangeStartType(argv[2], SERVICE_AUTO_START);

	case 'd':
		return ServiceChangeStartType(argv[2], SERVICE_DEMAND_START);

	default:
		Usage();
		return -1;
	}

	return 0;
}


int ServiceDelete(LPCTSTR ServiceName)
{
char ErrBuf[1024];
SC_HANDLE SCM_Handle;
SC_HANDLE ServiceHandle;
SERVICE_STATUS ServiceStatus;

	DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle==NULL)
	{
		nbGetLastError("Error opening Service Control Manager: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle,
					ServiceName,
					SERVICE_ALL_ACCESS);


	if (ServiceHandle==NULL)
	{
		nbGetLastError("Error opening Service Control Manager: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		
		if (!CloseServiceHandle(SCM_Handle))
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
					
		return -1;
	}

	ReturnValue=0;

	if (!ControlService(ServiceHandle,SERVICE_CONTROL_STOP,&ServiceStatus))
	{
		DWORD Err=GetLastError();

		if (Err != ERROR_SERVICE_NOT_ACTIVE)
		{
			nbGetLastError("Error stopping the service: ", ErrBuf, sizeof(ErrBuf));
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
			ReturnValue= -1;
		}
	}
	else
		fprintf(stderr,"Service %s successfully stopped\n",ServiceName);

	if (!DeleteService(ServiceHandle))
	{
		nbGetLastError("Error deleting the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		ReturnValue= -1;
		
	}
	else
		fprintf(stderr,"Service %s successfully deleted\n",ServiceName);
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		nbGetLastError("Error closing the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		ReturnValue= -1;
	}

	if (!CloseServiceHandle(SCM_Handle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
		ReturnValue= -1;
	}
	
	return ReturnValue;

}

int ServiceStop(LPCTSTR ServiceName)
{
char ErrBuf[1024];
SC_HANDLE SCM_Handle;
SC_HANDLE ServiceHandle;
SERVICE_STATUS ServiceStatus;
DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle==NULL)
	{
		nbGetLastError("Error opening Service Control Manager: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle, ServiceName, SERVICE_ALL_ACCESS);

	if (ServiceHandle==NULL)
	{
		nbGetLastError("Error opening the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		if (!CloseServiceHandle(SCM_Handle))
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
					
		return -1;
	}

	ReturnValue=0;

	if (!ControlService(ServiceHandle,SERVICE_CONTROL_STOP,&ServiceStatus))
	{
		nbGetLastError("Error stopping the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		ReturnValue= -1;
		
	}
	else
		fprintf(stderr,"Service %s successfully stopped\n",ServiceName);
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		nbGetLastError("Error closing the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		
		ReturnValue= -1;
	}

	if (!CloseServiceHandle(SCM_Handle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
		ReturnValue= -1;
	}
	
	return ReturnValue;

}

int ServiceInterrogate(LPCTSTR ServiceName)
{
char ErrBuf[1024];
SC_HANDLE SCM_Handle;
SC_HANDLE ServiceHandle;
SERVICE_STATUS ServiceStatus;
DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle==NULL)
	{
		nbGetLastError("Error opening Service Control Manager: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle, ServiceName, SERVICE_ALL_ACCESS);

	if (ServiceHandle==NULL)
	{
		nbGetLastError("Error opening the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		if (!CloseServiceHandle(SCM_Handle))
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
					
		return -1;
	}

	ReturnValue = ControlService(ServiceHandle,SERVICE_CONTROL_INTERROGATE,&ServiceStatus);

	if (!ReturnValue)
	{
		nbGetLastError("Error interrogating the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		ReturnValue= -1;
		
	}
	else if(ReturnValue == ERROR_SERVICE_NOT_ACTIVE){
		fprintf(stderr,"Service %s is not running\n",ServiceName);
		return -1;
	}
	else{
		fprintf(stderr,"Service %s is running\n",ServiceName);
	}
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		nbGetLastError("Error closing the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		
		ReturnValue= -1;
	}

	if (!CloseServiceHandle(SCM_Handle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
		ReturnValue= -1;
	}
	
	return ReturnValue;

}
int ServiceStart(LPCTSTR ServiceName)
{
char ErrBuf[1024];
SC_HANDLE SCM_Handle;
SC_HANDLE ServiceHandle;
DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle == NULL)
	{
		nbGetLastError("Error opening Service Control Manager: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle,
					ServiceName,
					SERVICE_ALL_ACCESS);


	if (ServiceHandle == NULL)
	{
		nbGetLastError("Error opening the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		if (!CloseServiceHandle(SCM_Handle))
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
					
		return -1;
	}

	ReturnValue=0;

	if (!StartService(ServiceHandle,0,NULL))
	{
		nbGetLastError("Error starting the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		ReturnValue= -1;
		
	}
	else
		fprintf(stderr,"Service %s successfully started\n",ServiceName);
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing the service.");
		ReturnValue= -1;
	}

	if (!CloseServiceHandle(SCM_Handle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
		ReturnValue= -1;
	}
	
	return ReturnValue;

}


int ServiceCreate(LPCTSTR ServiceName, LPCTSTR ServiceDescriptionShort, LPCTSTR ServiceDescriptionLong, LPCTSTR ServicePath)
{
char ErrBuf[1024];
SC_HANDLE SCM_Handle;
SC_HANDLE ServiceHandle;
DWORD ReturnValue;
SERVICE_DESCRIPTION ServiceDescription;
HMODULE hModule;
MyChangeServiceConfig2 fMyChangeServiceConfig2;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle==NULL)
	{
		nbGetLastError("Error opening Service Control Manager: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		return -1;
	}

	ServiceHandle=CreateService(SCM_Handle,
					ServiceName,
					ServiceDescriptionShort,
					SERVICE_ALL_ACCESS,
					SERVICE_WIN32_OWN_PROCESS,
					SERVICE_DEMAND_START,
					SERVICE_ERROR_NORMAL,
					ServicePath,
					NULL,
					NULL,
					"",
					NULL,
					NULL);

	if (ServiceHandle==NULL)
	{
		nbGetLastError("Error creating the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		if (!CloseServiceHandle(SCM_Handle))
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
					
		return -1;
	}

	fprintf(stderr,"Service %s successfully created.\n", ServiceName);

	//the following hack is necessary because the ChangeServiceConfig2 is not defined on NT4, so 
	//we cannot statically import ChangeServiceConfig2 from advapi32.dll, we have to load it dynamically

	hModule = LoadLibrary("advapi32.dll");
	
	if (hModule != NULL)
	{
		fMyChangeServiceConfig2 = (MyChangeServiceConfig2)GetProcAddress(hModule,"ChangeServiceConfig2A");
		if (fMyChangeServiceConfig2 != NULL)
		{
	
			ServiceDescription.lpDescription = (LPTSTR)ServiceDescriptionLong;

			if (!fMyChangeServiceConfig2(ServiceHandle,SERVICE_CONFIG_DESCRIPTION,&ServiceDescription))
			{
				nbGetLastError("Error setting service description: ", ErrBuf, sizeof(ErrBuf));
				nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
			}
		}
		FreeLibrary(hModule);
	}

	
	ReturnValue=0;

	if (!CloseServiceHandle(ServiceHandle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing the service.");
		ReturnValue= -1;
	}


	if (!CloseServiceHandle(SCM_Handle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
		ReturnValue= -1;
	}
	
	return ReturnValue;

}


int ServiceChangeStartType(LPCTSTR ServiceName, DWORD StartType)
{
char ErrBuf[1024];
SC_HANDLE SCM_Handle;
SC_HANDLE ServiceHandle;
DWORD ReturnValue;

	SCM_Handle=OpenSCManager(NULL,  /*local machine  */
		NULL,						/*active database*/
		SC_MANAGER_ALL_ACCESS);

	if (SCM_Handle == NULL)
	{
		nbGetLastError("Error opening Service Control Manager: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);
		return -1;
	}

	ServiceHandle=OpenService(SCM_Handle,
					ServiceName,
					SERVICE_ALL_ACCESS);


	if (ServiceHandle == NULL)
	{
		nbGetLastError("Error opening the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		if (!CloseServiceHandle(SCM_Handle))
			nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
					
		return -1;
	}

	ReturnValue=0;

	if (!ChangeServiceConfig(ServiceHandle,
		SERVICE_NO_CHANGE,
		StartType,
		SERVICE_NO_CHANGE,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL))
	{
		nbGetLastError("Error changing start type for the service: ", ErrBuf, sizeof(ErrBuf));
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, ErrBuf);

		ReturnValue= -1;
		
	}
	else
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Successfully changed start-type for the service.");
	
	if (!CloseServiceHandle(ServiceHandle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing the service");
		ReturnValue= -1;
	}


	if (!CloseServiceHandle(SCM_Handle))
	{
		nbPrintError(__FILE__, __FUNCTION__, __LINE__, "Error closing Service Control Manager.");
		ReturnValue= -1;
	}
	
	return ReturnValue;
}
