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


#define PSML_TEMPFILENAME "psmltemp.xml"
#define DEFAULT_CAPTUREFILENAME "samplecapturedump.acp"

char *NetPDLFileName;
const char *CaptureFileName= DEFAULT_CAPTUREFILENAME;


// This function prints a buffer containing a PSML string on screen
void PrintPSMLBuffer(char *PSMLAsciiBuffer, int PSMLElements);


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  psmlreader [CaptureFile] [NetPDLFile]\n\n"	\
	"Options:\n"	\
	" CaptureFile: name (and *path*) of the file containing the packets to decode\n"		\
	" NetPDLFile: name (and *path*) of the file containing the NetPDL\n"					\
	"     description. In case it is omitted, the NetPDL description embedded\n"			\
	"     within the NetBee library will be used.\n"										\
	" -h: prints this help message.\n\n"													\
	"Description\n"																			\
	"============================================================================\n"		\
	"This program creates and displays PSML packets through three steps:\n"					\
	"- read and decode a capture file, then display on screen the decoded packets \n"		\
	"  as soon as they get decoded\n"														\
	"- still using the decoder, it scans the decoded packets starting at the\n"				\
	"  beginning and it displays them again\n"												\
	"- close the PSML file, and opens it again as PSML file (the decoder is no  \n"			\
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
char *PSMLAsciiBuffer;
int PSMLElements;
nbPSMLReader *PSMLReader;
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

	Decoder= nbAllocatePacketDecoder(nbDECODER_GENERATEPDML_COMPLETE | 
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
	printf("Printing PSML data as soon as it gets decoded.\n\n");

	// Create a new PSML Manager to get data from NetBeePacketDecoder
	PSMLReader= Decoder->GetPSMLReader();
	if (PSMLReader == NULL)
	{
		printf("PSMLReader initialization failed: %s\n", Decoder->GetLastError() );
		return nbFAILURE;
	}

	PSMLElements= PSMLReader->GetSummary(&PSMLAsciiBuffer);

	if (PSMLElements == nbFAILURE)
	{
		printf("Reading summary from PSMLReader failed %s\n", PSMLReader->GetLastError() );
		return nbFAILURE;
	}
	PrintPSMLBuffer(PSMLAsciiBuffer, PSMLElements);

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


		// Get the current item in PSML format and print it on screen
		PSMLElements= PSMLReader->GetCurrentPacket(&PSMLAsciiBuffer);
		if (PSMLElements == nbFAILURE)
		{
			printf("Reading summary from PSMLReader failed %s\n", PSMLReader->GetLastError());
			return nbFAILURE;
		}
		PrintPSMLBuffer(PSMLAsciiBuffer, PSMLElements);
	}

	// Dump PSML file to disk
	if (PSMLReader->SaveDocumentAs(PSML_TEMPFILENAME) == nbFAILURE)
	{
		printf("%s\n", PSMLReader->GetLastError() );
		return nbFAILURE;
	}


	printf("\n\n==========================================================================\n");
	printf("Now printing PSML data reading everything from file, through the decoder.\n\n");

	PSMLElements= PSMLReader->GetSummary(&PSMLAsciiBuffer);

	if (PSMLElements == nbFAILURE)
	{
		printf("Reading summary from PSMLReader failed %s\n", PSMLReader->GetLastError() );
		return nbFAILURE;
	}

	PrintPSMLBuffer(PSMLAsciiBuffer, PSMLElements);

	for (unsigned long i= 1; i <= PacketCounter; i++)
	{
		// Get the current item in PSML format and print it on screen
		PSMLElements= PSMLReader->GetPacket(i, &PSMLAsciiBuffer);

		if ((PSMLElements == nbFAILURE) || (PSMLElements == nbWARNING))
		{
			printf("Reading summary from PSMLReader failed %s\n", PSMLReader->GetLastError() );
			return nbFAILURE;
		}
		PrintPSMLBuffer(PSMLAsciiBuffer, PSMLElements);
	}


	// Delete the decoder; is is no longer in use
	// The decoder will delete also the PSMLReader.
	nbDeallocatePacketDecoder(Decoder);


	printf("\n\n==========================================================================\n");
	printf("Now printing PSML data reading everything directly from file.\n\n");

	PSMLReader= nbAllocatePSMLReader((char*) PSML_TEMPFILENAME, ErrBuf, sizeof(ErrBuf));
	if (PSMLReader == NULL)
	{
		printf("PSMLReader creation failed: %s\n", ErrBuf);
		return nbFAILURE;
	}

	PSMLElements= PSMLReader->GetSummary(&PSMLAsciiBuffer);

	if (PSMLElements == nbFAILURE)
	{
		printf("Reading summary from PSMLReader failed %s\n", PSMLReader->GetLastError() );
		return nbFAILURE;
	}

	PrintPSMLBuffer(PSMLAsciiBuffer, PSMLElements);

	for (unsigned long i= 1; i <= PacketCounter; i++)
	{
		// Get the current item in PSML format and print it on screen
		PSMLElements= PSMLReader->GetPacket(i, &PSMLAsciiBuffer);

		if ((PSMLElements == nbFAILURE) || (PSMLElements == nbWARNING))
		{
			printf("Reading summary from PSMLReader failed %s\n", PSMLReader->GetLastError() );
			return nbFAILURE;
		}

		PrintPSMLBuffer(PSMLAsciiBuffer, PSMLElements);
	}

	nbDeallocatePSMLReader(PSMLReader);

	// NetBee cleanup
	nbCleanup();

	// Remove temporary file
	remove(PSML_TEMPFILENAME);

	return nbSUCCESS;
}


// Print the ascii elements contained in the PSML buffer
void PrintPSMLBuffer(char *PSMLAsciiBuffer, int PSMLElements)
{
int BufferPtr;

	BufferPtr= 0;

	printf("\n");

	for (int i= 0; i < PSMLElements; i++)
	{
	int PrintedChars;

		PrintedChars= printf("%s", &PSMLAsciiBuffer[BufferPtr]);
		BufferPtr= BufferPtr + PrintedChars + 1;

		// Print a separator between items
		printf("\t");
	}

	printf("\n");
}
