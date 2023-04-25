/*
 * Copyright (c) 2002 - 2007
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following condition 
 * is met:
 * 
 * Neither the name of the Politecnico di Torino nor the names of its 
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
#include "nbsvchelper.h"

#ifdef WIN32
#define	_WINSOCKAPI_
#include <windows.h>	// for Sleep()
#endif


// Function prototypes
void main_startup(int argc, char **argv);
void main_cleanup(void);

// Global variables
extern char *optarg;	// for getopt()


void main(int argc, char **argv)
{
	SvcHelperSetHandlers(main_startup, main_cleanup);
	SvcHelperMain(argc, argv);
}


// This function is just an helper for printing info on screen (and on file, since
// Win32 services do not have the standard error file
// Please note that default directory for Win32 is /windows/system32, hence this log file
// will be in there
int PrintMessageString(char* String)
{
FILE* LogFile;

	LogFile= fopen("log.txt", "a+");

	if (LogFile == NULL)
		return -1;

	fprintf(LogFile, "%s", String);
	fclose(LogFile);

	fprintf(stderr, String);

	return 0;
}


// It represents the entry point in this program
void main_startup(int argc, char **argv)
{
char Message[1024];
char *ReturnedParam;
int RetVal;
int i;


	sprintf(Message, "\nThe startup function in the main program has been called.\nParameters:\n\t");

	i= 0;
	while (i != argc)
	{
		strncat(Message, argv[i], sizeof(Message));
		strncat(Message, "  ", sizeof(Message));
		i++;
	}

	strncat(Message, "\n\n", sizeof(Message));

	PrintMessageString(Message);

	// Getting the proper command line options
	while ((RetVal = GetOpt(argc, argv, "d", &ReturnedParam)) != -1)
	{
		switch (RetVal)
		{
			case 'd': break;	// This is handled by the service main
			default: break;
		}
	}

	PrintMessageString("Now, printing a dot every second.\n");
	PrintMessageString("   This will be printed on screen and on file log.txt,\n");
	PrintMessageString("   (either in the current folder, or in windows\\system32\n");
	PrintMessageString("   in case this program is launched as service).\n\n");

	while(1)
	{
		PrintMessageString(".");
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}
}



// Contains the code for closing (gracefully) the program.
void main_cleanup(void)
{
	PrintMessageString("\nThe cleanup function in the main program has been called.\n");
	return;
}

