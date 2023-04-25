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


#define PDML_TEMPFILENAME "pdmltemp.xml"
#define DEFAULT_CAPTUREFILENAME "samplecapturedump.acp"
#define DEFAULT_FIELDNAME "ip.src"

char *NetPDLFileName;
const char *CaptureFileName= DEFAULT_CAPTUREFILENAME;
const char *ProtoAndFieldName= DEFAULT_FIELDNAME;


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  pdmlfieldextractor [ProtoFieldName] [CaptureFile] [NetPDLFile]\n\n"	\
	"Options:\n"	\
	" -netpdl FileName: name (and *path*) of the file containing the NetPDL\n"			\
	"     description. In case it is omitted, the NetPDL description embedded\n"		\
	"     within the NetBee library will be used.\n"									\
	" -capturefile FileName: name (and *path*) of the file containing the packet\n"		\
	"     dump that has to be processed. If missing, file '" DEFAULT_CAPTUREFILENAME "'\n"		\
	"     will be used.\n"																\
	" -field ProtoFieldName: protocol and field that we need to extract from\n"			\
	"     the capture, in the format 'proto.field'.\n"									\
	"     Default: field  '" DEFAULT_FIELDNAME "' is extracted.\n"						\
	" -h: prints this help message.\n\n"												\
	"Description\n"																		\
	"============================================================================\n"	\
	"This program operates according to the following steps:\n"							\
	" - decodes the first packet contained within a given capture file\n"				\
	" - looks for the protocol field specified by the user and it prints its value\n"	\
	" - continues decoding all the packets contained in the capture file and, for\n"	\
	"   each packet, it prints the value of the protocol field selected by the user.\n"	\
	"When the capture file ends, this program saves the PDML file and it scans\n"		\
	"  it again (packet after packet) printing the value of the protocol fields\n"		\
	"  selected by the user. This second step is done in order to show how to\n"		\
	"  extract the value of protocol fields when they have already been decoded\n"		\
	"  and saved on disk.\n\n";

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

		if (strcmp(argv[CurrentItem], "-netpdl") == 0)
		{
			NetPDLFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-capturefile") == 0)
		{
			CaptureFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-field") == 0)
		{
			ProtoAndFieldName= argv[CurrentItem+1];
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
char ErrorMsg[1024];
nbPacketDecoder *Decoder;
nbPacketDumpFilePcap* PcapPacketDumpFile;
nbNetPDLLinkLayer_t LinkLayerType;
unsigned long PacketCounter= 0;
nbPDMLReader *PDMLReader;
//char Buffer[2048];
char ProtoName[2048];
char FieldName[2048];
char ErrBuf[2048];
int Res;
int i, ProtoNameLength;

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	//printf("\nEnter the protocol and field you want to look at (default: 'ip.src'):\n   ");
	//fgets(Buffer, sizeof(Buffer), stdin);

	//// fgets reads also the newline character, so we have to reset it
	//Buffer[strlen(Buffer) - 1]= 0;

	// In case the user enters a given field, let's copy it within the proper variables
	// Let's copy the fieldname into the proper variables
	i=0;

	while ( (ProtoAndFieldName[i]) && (ProtoAndFieldName[i] != '.') && (i < (sizeof(ProtoName) - 1)) )
	{
		ProtoName[i]=  ProtoAndFieldName[i];
		i++;
	}
	ProtoName[i]= 0;

	ProtoNameLength= i;
	i=0;

	while ( (ProtoAndFieldName[ProtoNameLength + i + 1]) && (i < (sizeof(FieldName) - 1)) )
	{
		FieldName[i]=  ProtoAndFieldName[ProtoNameLength + i + 1];
		i++;
	}
	FieldName[i]= 0;


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

	Decoder= nbAllocatePacketDecoder(nbDECODER_GENERATEPDML_COMPLETE | 
		nbDECODER_GENERATEPSML | nbDECODER_KEEPALLPSML | nbDECODER_KEEPALLPDML, ErrorMsg, sizeof(ErrorMsg));

	// Create a NetPDL Parser to decode packet
	if (Decoder == NULL)
	{
		printf("Error creating the NetPDLParser: %s.\n", ErrorMsg);
		return nbFAILURE;
	}

	// Allocate a pcap dump file reader
	if ((PcapPacketDumpFile= nbAllocatePacketDumpFilePcap(ErrBuf, sizeof(ErrBuf))) == NULL)
	{
		printf("Error creating the PcapPacketDumpFile: %s.\n", ErrBuf);
		return nbFAILURE;
	}

	// Open the pcap file
	if (PcapPacketDumpFile->OpenDumpFile(CaptureFileName, 0) == nbFAILURE)
	{
		printf("%s", PcapPacketDumpFile->GetLastError());
		return nbFAILURE;
	}

	// Get the link layer type (will be used in the decoding process)
	if (PcapPacketDumpFile->GetLinkLayerType(LinkLayerType) == nbFAILURE)
	{
		printf("%s", PcapPacketDumpFile->GetLastError());
		return nbFAILURE;
	}


	printf("\n\n==========================================================================\n");
	printf("Printing PDML fields values data as soon as packets get decoded.\n\n");

	// Get a new PDML Manager
	PDMLReader= Decoder->GetPDMLReader();

	// Initialize the PDMLReader to get data from NetBeePacketDecoder
	if (PDMLReader == NULL)
	{
		printf("PDMLReader initialization failed: %s\n", Decoder->GetLastError() );
		return nbFAILURE;
	}

	while (1)
	{
	int RetVal;
	struct pcap_pkthdr *PktHeader;
	const unsigned char *PktData;
	struct _nbPDMLField *PDMLField;

		RetVal= PcapPacketDumpFile->GetNextPacket(&PktHeader, &PktData);

		if (RetVal == nbWARNING)
			break;		// capture file ended

		if (RetVal == nbFAILURE)
		{
			printf("Cannot read from the capture source file: %s\n", PcapPacketDumpFile->GetLastError() );
			return nbFAILURE;
		}

		PacketCounter++;

		// Decode packet
		if (Decoder->DecodePacket(LinkLayerType, PacketCounter, PktHeader, PktData) == nbFAILURE)
		{
			printf("\nError decoding a packet %s\n\n", Decoder->GetLastError());
			return nbFAILURE;
		}

		PDMLField= NULL;

		// Please remember that this loop is usually executed twice
		// In fact, if the selected field is found in the packet, we have to repeat the loop to see if
		// another instance of that field is still there.
		// In case there is not another instance of that field, the second iteration goes into the
		// nbWARNING part, and then it exits.
		while (1)
		{
		int Ret;

			Ret= PDMLReader->GetPDMLField(ProtoName, FieldName, PDMLField, &PDMLField);

			if (Ret == nbSUCCESS)
			{
				printf("The value of field '%s.%s' is %s\n", ProtoName, FieldName, PDMLField->ShowValue);
				continue;
			}

			if (Ret == nbFAILURE)
			{
				printf("Field '%s.%s' not found in packet #%ld. Is protocol '%s' present in packet #%ld? (NetBee returned error: %s)\n\n",
								ProtoName, FieldName, PacketCounter, ProtoName, PacketCounter, PDMLReader->GetLastError());
				break;
			}

			if (Ret == nbWARNING)
				break;
		}
	}

	// Dump PDML file to disk
	if (PDMLReader->SaveDocumentAs(PDML_TEMPFILENAME) == nbFAILURE)
		printf("%s\n", PDMLReader->GetLastError() );


	// Delete the decoder; is is no longer in use
	// The decoder will delete also the PDMLReader.
	nbDeallocatePacketDecoder(Decoder);


	printf("\n\n==========================================================================\n");
	printf("Now printing PDML field values by reading PDML packets from file.\n\n");

	PDMLReader= nbAllocatePDMLReader((char*) PDML_TEMPFILENAME, ErrorMsg, sizeof(ErrorMsg));
	if (PDMLReader == NULL)
	{
		printf("PDMLReader creation failed: %s\n", ErrorMsg);
		return nbFAILURE;
	}

	for (unsigned long i= 1; i <= PacketCounter; i++)
	{
	struct _nbPDMLField *PDMLField;

		PDMLField= NULL;

		while (1)
		{
		int Ret;

			Ret= PDMLReader->GetPDMLField(i, ProtoName, FieldName, PDMLField, &PDMLField);

			if (Ret == nbSUCCESS)
			{
				printf("The value of field '%s.%s' is %s\n", ProtoName, FieldName, PDMLField->ShowValue);
				continue;
			}

			if (Ret == nbFAILURE)
			{
				printf("Field '%s.%s' not found in packet #%ld. Is protocol '%s' present in packet #%ld? (NetBee returned error: %s)\n\n",
									ProtoName, FieldName, i, ProtoName, i, PDMLReader->GetLastError());
				break;
			}

			if (Ret == nbWARNING)
				break;
		}
	}

	nbDeallocatePDMLReader(PDMLReader);

	// NetBee cleanup
	nbCleanup();

	// Remove temporary file
	remove(PDML_TEMPFILENAME);

	return nbSUCCESS;
}

