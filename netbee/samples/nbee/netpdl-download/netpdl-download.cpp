/*
 * Copyright (c) 2002 - 2011
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
#include <stdlib.h>
#include <string.h>
#include <nbee.h>


// Global variable for configuration
char* NetPDLFileName;


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  netpdl-download [options]\n\n"	\
	"Options:\n"	\
	" -netpdl FileName: name (and *path*) of the file containing the NetPDL\n"			\
	"     description to be updated. In case it is omitted, the NetPDL description\n"	\
	"     embedded within the NetBee library will be replaced.\n"						\
	" -h: prints this help message.\n\n"												\
	"Description\n"																		\
	"============================================================================\n"	\
	"This program downloads a new working copy of the NetPDL database from the\n"		\
	"NetBee main site, and it replaces the NetPDL file present on disk.\n\n";

	printf("%s", string);
}


int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;

	CurrentItem= 1;

	while (CurrentItem < argc)
	{
		if (strcmp(argv[CurrentItem], "-netpdl") == 0)
		{
			NetPDLFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nbFAILURE;
		}

		printf("Error: parameter '%s' is not valid.\n", argv[CurrentItem]);
		return nbFAILURE;
	}

	return nbSUCCESS;
}



int main(int argc, char *argv[])
{
char ErrBuf[2048];
int RetVal;
struct _nbVersion* nbVersion;

	nbVersion= nbGetVersion();

	printf("\n****************************************************************************\n");
	printf("* NetBee library version %d.%d.%d (release date %s)\n", nbVersion->Major, nbVersion->Minor, nbVersion->RevCode, nbVersion->Date);
	printf("* Supported NetPDL language: version %d.%d\n", nbVersion->SupportedNetPDLMajor, nbVersion->SupportedNetPDLMinor);
	printf("****************************************************************************\n");
	// We do not print anything about NetPDL, because the NetBee library has not been initialized,
	// to the internal structure nbVersion has not been filled properly

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	RetVal= nbDownloadAndUpdateNetPDLFile(NetPDLFileName, ErrBuf, sizeof(ErrBuf));

	if (RetVal == nbFAILURE)
	{
		printf("Error downloading the new NetPDL file: %s.\n", ErrBuf);
		return nbFAILURE;
	}

	if (NetPDLFileName)
		printf("\nThe NetPDL database '%s'\n"
				"  has been updated correctly with a newer version from the Internet.\n\n", NetPDLFileName);
	else
		printf("\nThe NetPDL database provided with the NetBee library has been\n"
				"  updated correctly with a newer version from the Internet.\n\n");

	// Now, let's print the version of the NetPDL file downloaded from the Internet
	nbVersion= nbGetVersion();

	printf("Most important parameters of the downloaded NetPDL database:\n");
	printf("  Creator %s, release date %s, language version %d.%d\n\n",
			nbVersion->NetPDLCreator, nbVersion->NetPDLDate, nbVersion->NetPDLMajor, nbVersion->NetPDLMinor);

	nbCleanup();

	return nbSUCCESS;
}
