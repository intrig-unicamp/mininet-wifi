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
#include <time.h>
#include <nbee.h>


#define PDML_TEMPFILENAME "pdmltemp.xml"
#define DEFAULT_CAPTUREFILENAME "samplecapturedump.acp"

char *NetPDLFileName;
const char *CaptureFileName= DEFAULT_CAPTUREFILENAME;


// These functions print a PDML packet on screen
void PrintPDMLPacket(struct _nbPDMLPacket *PDMLPacket);
void PrintPDMLFields(struct _nbPDMLField *FieldItem, int Level);


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  pdmlreader [CaptureFile] [NetPDLFile]\n\n"	\
	"Options:\n"	\
	" CaptureFile: name (and *path*) of the file containing the packets to decode\n"		\
	" NetPDLFile: name (and *path*) of the file containing the NetPDL\n"					\
	"     description. In case it is omitted, the NetPDL description embedded\n"			\
	"     within the NetBee library will be used.\n"										\
	" -h: prints this help message.\n\n"													\
	"Description\n"																			\
	"============================================================================\n"		\
	"This program creates and displays PDML packets through three steps:\n"					\
	"- read and decode a capture file, then display on screen the decoded packets \n"		\
	"  as soon as they get decoded\n"														\
	"- still using the decoder, it scans the decoded packets starting at the \n"			\
	"  beginning and it displays them again\n"												\
	"- close the PDML file, and opens it again as PDML file (the decoder is no  \n"			\
	"  longer used). Then, it scans all the packets and displays them on screen.\n\n";

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

		// This should be the Capture file
		if (CurrentItem == 1)
		{
			CaptureFileName= argv[CurrentItem];
			CurrentItem++;
			continue;
		}

		// This should be the NetPDL file
		if (CurrentItem == 2)
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
char ErrBuf[1024];
nbPacketDecoder *Decoder;
nbPacketDumpFilePcap* PcapPacketDumpFile;
nbNetPDLLinkLayer_t LinkLayerType;
unsigned long PacketCounter= 0;
struct _nbPDMLPacket *PDMLPacketPtr;
int PDMLNumProto;
nbPDMLReader *PDMLReader;
unsigned long i;
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

	Decoder= nbAllocatePacketDecoder(nbDECODER_GENERATEPDML_COMPLETE | nbDECODER_GENERATEPDML_RAWDUMP |
		nbDECODER_GENERATEPSML | nbDECODER_KEEPALLPSML | nbDECODER_KEEPALLPDML, ErrBuf, sizeof(ErrBuf));

	// Create a NetPDL Parser to decode packet
	if (Decoder == NULL)
	{
		printf("Error creating the NetPDLParser: %s.\n", ErrBuf);
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
	printf("Printing PDML data as soon as it gets decoded.\n\n");

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


		// Get the current item in PDML format and print it on screen
		PDMLNumProto= PDMLReader->GetCurrentPacket(&PDMLPacketPtr);
		if (PDMLNumProto == nbFAILURE)
		{
			printf("Reading packet from PDMLReader failed %s\n", PDMLReader->GetLastError() );
			return nbFAILURE;
		}
		PrintPDMLPacket(PDMLPacketPtr);
	}

	// Dump PDML file to disk
	if (PDMLReader->SaveDocumentAs(PDML_TEMPFILENAME) == nbFAILURE)
		printf("%s\n", PDMLReader->GetLastError());


	printf("\n\n==========================================================================\n");
	printf("Now printing PDML data reading everything from file, using the decoder\n");
	printf("created at the previous step.\n\n");

	for (i= 1; i <= PacketCounter; i++)
	{
		// Get the current item in PDML format and print it on screen
		PDMLNumProto= PDMLReader->GetPacket(i, &PDMLPacketPtr);

		if ((PDMLNumProto == nbFAILURE) || (PDMLNumProto == nbWARNING))
		{
			printf("Reading packet from PDMLReader failed %s\n", PDMLReader->GetLastError() );
			return nbFAILURE;
		}
		PrintPDMLPacket(PDMLPacketPtr);
	}


	// Delete the decoder; is is no longer in use
	// The decoder will delete also the PDMLReader.
	nbDeallocatePacketDecoder(Decoder);


	printf("\n\n==========================================================================\n");
	printf("Now reading the previously created PDML file (as new) and printing its data.\n\n");

	PDMLReader= nbAllocatePDMLReader((char*) PDML_TEMPFILENAME, ErrBuf, sizeof(ErrBuf));
	if (PDMLReader == NULL)
	{
		printf("PDMLReader creation failed: %s\n", ErrBuf);
		return nbFAILURE;
	}

	i= 1;
	while (1)
	{
		// Get the current item in PDML format and print it on screen
		PDMLNumProto= PDMLReader->GetPacket(i, &PDMLPacketPtr);

		if (PDMLNumProto == nbFAILURE)
		{
			printf("Reading packet from PDMLReader failed %s\n", PDMLReader->GetLastError() );
			return nbFAILURE;
		}

		// If we do not have any more packets, let's exit
		if (PDMLNumProto == nbWARNING)
			break;

		PrintPDMLPacket(PDMLPacketPtr);
		i++;
	}

	nbDeallocatePDMLReader(PDMLReader);

	// NetBee cleanup
	nbCleanup();

	// Remove temporary file
	remove(PDML_TEMPFILENAME);

	return nbSUCCESS;
}



/***********************************************************/
/*     PRINTING FUNCTIONS                                  */
/***********************************************************/

// Print the elements contained in this PDML packet
void PrintPDMLPacket(struct _nbPDMLPacket *PDMLPacket)
{
struct _nbPDMLProto *ProtoItem;
struct tm *Time;
time_t TimeSec;
char TimeString[1024];

	printf("\nPacket number %ld:", PDMLPacket->Number);
	printf("\n\tTotal lenght= %ld", PDMLPacket->Length);
	printf("\n\tCaptured lenght= %ld", PDMLPacket->CapturedLength);
	printf("\n\tTimestamp= ");

	// Format timestamp; please note that localtime requires a 64 bits value
	TimeSec= (long) PDMLPacket->TimestampSec;
	Time= localtime( &TimeSec );

	strftime(TimeString, sizeof(TimeString), "%H:%M:%S", Time);
	printf("%s.%.6ld (%ld.%ld)\n", TimeString, PDMLPacket->TimestampUSec, PDMLPacket->TimestampSec, PDMLPacket->TimestampUSec);


	ProtoItem= PDMLPacket->FirstProto;

	while(ProtoItem)
	{
		printf ("\n%s: size %ld, offset %ld\n", ProtoItem->LongName, ProtoItem->Size, ProtoItem->Position);
		if (ProtoItem->FirstField)
			PrintPDMLFields(ProtoItem->FirstField, 0);
		ProtoItem= ProtoItem->NextProto;
	}
	printf("\n");

	printf("Raw packet dump:\n");
	for (unsigned int i= 0; i < PDMLPacket->CapturedLength; i++)
		printf("%X", PDMLPacket->PacketDump[i]);

	printf("\n\n");
}


// Print the list of fields from this one to the last one of the current tree
void PrintPDMLFields(struct _nbPDMLField *FieldItem, int Level)
{
	while(FieldItem)
	{
		// Create proper indentation for this field
		for (int i=0; i<= Level; i++)
			printf ("\t");

		// Let's check: there are some fields that do not the 'showvalue' attribute, because the user
		// does not expect them to be printed
		if (FieldItem->ShowValue)
			printf ("%s = %s: size %ld, offset %ld\n", FieldItem->LongName, FieldItem->ShowValue, FieldItem->Size, FieldItem->Position);
		else
			printf ("%s: size %ld, offset %ld\n", FieldItem->LongName, FieldItem->Size, FieldItem->Position);

		if (FieldItem->FirstChild)
			PrintPDMLFields(FieldItem->FirstChild, Level + 1);

		FieldItem= FieldItem->NextField;
	}
}
