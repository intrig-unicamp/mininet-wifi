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


char *NetPDLFileName;

void ConvertField(nbNetPDLUtils *NetPDLUtils, char *Proto, char *Field, int FieldLength, char *Example);
void ConvertFieldUsingTemplate(nbNetPDLUtils *NetPDLUtils, char *Template, int FieldLength, char *Example);


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  netpdlutils [NetPDLFile]\n\n"	\
	"Options:\n"	\
	" - NetPDLFile: name (and *path*) of the file containing the NetPDL\n"				\
	"     description. In case it is omitted, the NetPDL description embedded\n"		\
	"     within the NetBee library will be used.\n"									\
	" -h: prints this help message.\n\n"												\
	"Description\n"																		\
	"============================================================================\n"	\
	"This program demonstrates how to use the conversion functions provided by\n"		\
	"  NetBee, which aims at converting the value of a field from its natural\n"		\
	"  format to the internal format used by machines, i.e. the hexadecimal dump.\n"	\
	"This program converts the value of some protocol fields (e.g. 'ip.source')\n"		\
	"  from the 'human' form (e.g. '10.1.1.1') into its hex form (e.g. the binary\n"	\
	"  buffer containing the value '0A010101'), and then back to its 'human'\n"			\
	"  form.\n"																			\
	"Please note that the field value must be typed in the expected format.\n"		\
	"  For instance, if the NetPDL database expects this field to be printed in\n"		\
	"  hexadecimal, the user must insert this value in hexadecimal."					\
	"\n\n";

	printf("%s", string);
}


int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;
	
	CurrentItem= 1;

	while (CurrentItem < argc)
	{
		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nbFAILURE;
		}

		// This should be the NetPDL file
		if (argv[CurrentItem][0] != '-')
		{
			NetPDLFileName= argv[CurrentItem];
			CurrentItem++;
			continue;
		}

		printf("Error: parameter '%s' is not valid.\n", argv[CurrentItem]);
		return nbFAILURE;
	}

	return nbSUCCESS;
}


int main(int argc, char *argv[])
{
char ErrBuf[2048];
int Res;

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	printf("\n\nLoading NetPDL protocol database...\n");

	Res= nbInitialize(NetPDLFileName, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf) );

	if (Res == nbFAILURE)
	{
		printf("Error initializing the NetBee Library; %s\n", ErrBuf);
		printf("\n\nUsing the NetPDL database embedded in the NetBee library instead.\n");
	}

	// In case the NetBee library has not been initialized,
	// initialize right now with the embedded NetPDL protocol database instead
	if (nbIsInitialized() == nbFAILURE)
	{
		if (nbInitialize(NULL, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf)) == nbFAILURE)
		{
			printf("Error initializing the NetBee Library; %s\n", ErrBuf);
			return nbFAILURE;
		}
	}

	printf("NetPDL Protocol database loaded.\n");

	// Let's create a nbNetPDLUtils object.
	nbNetPDLUtils *NetPDLUtils= nbAllocateNetPDLUtils(ErrBuf, sizeof(ErrBuf));
	if (NetPDLUtils == NULL)
	{
		printf("Error creating the nbNetPDLUtils class: %s\n", ErrBuf);
		return nbFAILURE;
	}

	// Convert a field which is always present within a protocol
	ConvertField(NetPDLUtils, (char*) "ip", (char*) "src", 4, (char*) "10.11.12.13");

	// Let's convert a 'difficult' field. Please note that the IPv6 address
	// must be typed entirely (no "::" sign within the address), e.g.
	// FE80:0000:1111:2222:3333:4444:5555:6666
	ConvertField(NetPDLUtils, (char*) "ipv6", (char*) "src", 16, (char*) "FE80:0000:1111:2222:3333:4444:5555:6666");

	// Convert an optional field (may not be present within the protocol)
	ConvertField(NetPDLUtils, (char*) "tcp", (char*) "maxssize", 2, (char*) "2048");

	// Convert a field using the visualization template directly
	ConvertFieldUsingTemplate(NetPDLUtils, (char*) "MACaddressEth", 6, (char*) "000800-AABBCC");

	nbDeallocateNetPDLUtils(NetPDLUtils);

	nbCleanup();

	return nbSUCCESS;
}


void ConvertField(nbNetPDLUtils *NetPDLUtils, char *Proto, char *Field, int FieldLength, char *Example)
{
char FieldPrintableValue[2048];
unsigned char FieldHexValue[2048];
unsigned int FieldHexSize;
int Res;

	printf("\nEnter a value that is valid as %s.%s field (%d bytes, default '%s'):\n\t ", Proto, Field, FieldLength, Example);
	fgets(FieldPrintableValue, sizeof(FieldPrintableValue), stdin);
	
	// fgets reads also the newline character, so we have to reset it
	FieldPrintableValue[strlen(FieldPrintableValue) - 1]= 0;

	// In case the user doesn't type anything, use the value provided with the example
	if (strlen(FieldPrintableValue) == 0)
		strcpy(FieldPrintableValue, Example);

	// Initialize the buffer size
	FieldHexSize= sizeof(FieldHexValue);

	// Convert the field in hex
	Res= NetPDLUtils->GetHexValueNetPDLField(Proto, Field, FieldPrintableValue, FieldHexValue, &FieldHexSize, true);

	if (Res == nbFAILURE)
	{
		printf("Error converting the field value in its binary form: %s", NetPDLUtils->GetLastError());
		return;
	}

	// Convert the result in something that can be printable, e.g. the ascii dump of the resulting buffer
	NetPDLUtils->HexDumpBinToHexDumpAscii((char *) FieldHexValue, FieldHexSize, true,
				(char *) FieldPrintableValue, sizeof(FieldPrintableValue));

	printf("\n  The hex value of the current field (%s.%s) is %s (size %d)", 
				Proto, Field, FieldPrintableValue, FieldHexSize);

	// Convert the field in its printable format (again)
	printf("\n\n  Now converting the hex value back to a 'human' form");

	Res= NetPDLUtils->FormatNetPDLField(Proto, Field, 
				FieldHexValue, FieldHexSize, FieldPrintableValue, sizeof(FieldPrintableValue));

	if (Res == nbFAILURE)
	{
		printf("Error converting the field value in its printable form: %s", NetPDLUtils->GetLastError());
		return;
	}

	printf("\n  The 'human' value of the current field (%s.%s) is %s\n\n", Proto, Field, FieldPrintableValue);
}




void ConvertFieldUsingTemplate(nbNetPDLUtils *NetPDLUtils, char *Template, int FieldLength, char *Example)
{
char FieldPrintableValue[2048];
unsigned char FieldHexValue[2048];
unsigned int FieldHexSize;
int Res;

	printf("\nEnter a value that is valid for the '%s' visualization template (%d bytes, e.g. '%s'): ", Template, FieldLength, Example);
	fgets(FieldPrintableValue, sizeof(FieldPrintableValue), stdin);

	// fgets reads also the newline character, so we have to reset it
	FieldPrintableValue[strlen(FieldPrintableValue) - 1]= 0;

	// In case the user doesn't type anything, use the value provided with the example
	if (strlen(FieldPrintableValue) == 0)
		strcpy(FieldPrintableValue, Example);

	// Initialize the buffer size
	FieldHexSize= sizeof(FieldHexValue);

	// Convert the field in hex
	Res= NetPDLUtils->GetHexValueNetPDLField(Template, FieldPrintableValue, FieldHexValue, &FieldHexSize, true);

	if (Res == nbFAILURE)
	{
		printf("Error converting the field value in its binary form: %s", NetPDLUtils->GetLastError());
		return;
	}

	// Convert the result in something that can be printable, e.g. the ascii dump of the resulting buffer
	NetPDLUtils->HexDumpBinToHexDumpAscii((char *) FieldHexValue, FieldHexSize, true,
				(char *) FieldPrintableValue, sizeof(FieldPrintableValue));

	printf("\n  The hex value of the current field (according to template '%s') is   %s (size %d)", 
				Template, FieldPrintableValue, FieldHexSize);

	// Convert the field in its printable format (again)
	printf("\n\n  Now converting the hex value into a 'human' form (again)");

	Res= NetPDLUtils->FormatNetPDLField(Template, 
				FieldHexValue, FieldHexSize, 1 /* network byte order */, FieldPrintableValue, sizeof(FieldPrintableValue));

	if (Res == nbFAILURE)
	{
		printf("Error converting the field value in its printable form: %s", NetPDLUtils->GetLastError());
		return;
	}

	printf("\n  The 'human' value of the current field (according to template '%s')  is %s\n\n", Template, FieldPrintableValue);
}
