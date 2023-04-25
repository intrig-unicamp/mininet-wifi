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
#include <pcap.h>
#include <nbee.h>
#include "../common/measure.h"



// Global variables for configuration
#define DEFAULT_FILTERSTRING "ip"
#define DEFAULT_CAPTUREFILENAME "samplecapturedump.acp"
#define DEFAULT_DUMPFILENAME "packetcapturedump.acp"


typedef struct _ConfigParams
{
	const char*	NetPDLFileName;
	const char*	CaptureFileName;
	const char*	DumpFileName;
	char		AdapterName[1024];
	int			PromiscuousMode;
	bool		UseJit;
	const char*	FilterString;
} ConfigParams_t;


// Global variable for configuration
ConfigParams_t ConfigParams;



// Function prototypes
int GetAdapterName(int AdapterNumber, char *AdapterName, int AdapterNameSize);



void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  packetcapture [options] [filterstring]\n\n"	\
	"Options:\n                                                                     \n"	\
	" -netpdl filename                                                              \n"	\
	"        Name of the file containing the NetPDL description. In case it is      \n"	\
	"        omitted, the NetPDL description embedded within the NetBee library will\n"	\
	"        be used.                                                               \n"	\
	" -i interface                                                                  \n"	\
	"        Name or number of the interface that to be used for capturing packets. \n"	\
	"        It can be used only in case the '-r' parameter is void.                \n"	\
	" -p                                                                            \n"	\
	"        Do not put the adapter in promiscuous mode (default: promiscuous mode  \n"	\
	"        on). Valid only for live captures.                                     \n"	\
	" -r filename                                                                   \n"	\
	"        Name of the file containing the packet dump that has to be opened. It  \n"	\
	"        can be used only in case the '-i' parameter is void.                   \n"	\
	" -w filename                                                                   \n"	\
	"        Name of the dump file that will contain the captured packets.          \n"	\
	" -jit                                                                          \n"	\
	"        Make NetVM to use the native code on the current platform instead of   \n"	\
	"        NetIL code; by default the NetIL code is used (for safety reasons).    \n"	\
	" [filterstring]                                                                \n" \
	"        List of fields to be extracted from network packets, with an optional  \n" \
	"        filter in front (e.g. 'ip extractfields(ip.src,ip.dst)')               \n" \
	"        (default: extractfields(ip.src,ip.dst))						        \n" \
	" -h: prints this help message.\n\n"												\
	"Description\n"																		\
	"=============================================================================\n"	\
	"This program opens a dump file or a physical interface and reads packets       \n"	\
	"  on that device. Packets are then filtered according to the give filtering    \n"	\
	"  string, and the packet that matche the filter are dumped on disk.            \n"	\
	"This sample can be used to demonstrate the nbPacketEngine class when dealing   \n"	\
	"  with packet captures.\n\n";
	
	fprintf(stderr, "%s", string);
}


int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;
int InterfaceNumber= -1;

	CurrentItem= 1;

	// Default values
	ConfigParams.PromiscuousMode= 1;		//PCAP_OPENFLAG_PROMISCUOUS;
	ConfigParams.UseJit= 0;
	ConfigParams.DumpFileName= DEFAULT_DUMPFILENAME;
	ConfigParams.FilterString = DEFAULT_FILTERSTRING;
	ConfigParams.CaptureFileName= DEFAULT_CAPTUREFILENAME;
	// End defaults


	while (CurrentItem < argc)
	{
		if (strcmp(argv[CurrentItem], "-netpdl") == 0)
		{
			ConfigParams.NetPDLFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-r") == 0)
		{
			ConfigParams.CaptureFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-w") == 0)
		{
			ConfigParams.DumpFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-jit") == 0)
		{
			ConfigParams.UseJit= 1;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-p") == 0)
		{
			ConfigParams.PromiscuousMode= 0;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-i") == 0)
		{
		char* Interface;

		ConfigParams.CaptureFileName= NULL;

			Interface= argv[CurrentItem+1];
			InterfaceNumber= atoi(Interface);

			if (InterfaceNumber == 0)
			{
				strncpy(ConfigParams.AdapterName, Interface, sizeof(ConfigParams.AdapterName));
				ConfigParams.AdapterName[sizeof(ConfigParams.AdapterName) - 1]=0;
				continue;
			}

			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nbFAILURE;
		}

		if (argv[CurrentItem][0] == '-')
		{
			printf("\n\tError: parameter '%s' is not valid.\n", argv[CurrentItem]);
			return nbFAILURE;
		}

		// Current parameter is the filter string, which does not have any switch (e.g. '-something') in front
		ConfigParams.FilterString= argv[CurrentItem++];
	}

	if ((ConfigParams.CaptureFileName == NULL) && (ConfigParams.AdapterName[0] == 0))
	{
        printf("Neither the capture file nor the capturing interface has been specified.\n");
		printf("Using the first adapter as default.\n");

		if (GetAdapterName(1, ConfigParams.AdapterName, sizeof(ConfigParams.AdapterName)) == nbFAILURE)
		{
			printf("Error when trying to retrieve the name of the first network interface.\n");
			printf("    Make sure that WinPcap/libpcap is installed, and that you have\n");
			printf("    the right permissions to open the network adapters.\n\n");
			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}


int GetAdapterName(int AdapterNumber, char* AdapterName, int AdapterNameSize)
{
int i;
pcap_if_t *alldevs;
pcap_if_t *device;
char errbuf[PCAP_ERRBUF_SIZE + 1] = "";

	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		printf("Error when getting the list of the installed adapters%s\n", errbuf );
		return nbFAILURE;
	}

	// Get interface name
	i= 1;
	for (device=alldevs; device != NULL; device=device->next)
	{
		if (i == AdapterNumber)
		{
			strncpy(AdapterName, device->name, AdapterNameSize);
			AdapterName[AdapterNameSize-1]= 0;

			// Free the device list
			pcap_freealldevs(alldevs);
			return nbSUCCESS;
		}

		i++;
	}

	return nbFAILURE;
}





int main(int argc, char* argv[])
{
char ErrBuf[PCAP_ERRBUF_SIZE + 1] = "";
int Res;	// Generic Variable for return code

// Pcap related structures
char pcapSourceName[2048] = "";
pcap_t *PcapHandle = NULL;
pcap_dumper_t *PcapDumpHandle = NULL;

// Data for parsing packets and extracting fields
nbPacketEngine *PacketEngine;
int PacketCounter;
int AcceptedPkts;

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	fprintf(stderr, "\nLoading NetPDL protocol database...\n");

	if (ConfigParams.NetPDLFileName)
	{
		int Res;

		// We do not need the full NetPDL database here, since we just filter traffic and dump it on disk
		Res= nbInitialize(ConfigParams.NetPDLFileName, nbPROTODB_MINIMAL, ErrBuf, sizeof(ErrBuf) );

		if (Res == nbFAILURE)
		{
			fprintf(stderr, "Error initializing the NetBee Library: %s\n", ErrBuf);
			fprintf(stderr, "Trying to use the NetPDL database embedded in the NetBee library instead.\n");
		}
	}

	// In case the NetBee library has not been initialized
	// initialize right now with the embedded NetPDL protocol database instead
	// Previous initialization may fail because the NetPDL file has errors, or because it is missing,
	// or because the user didn't specify a NetPDL file
	if (nbIsInitialized() == nbFAILURE)
	{
		if (nbInitialize(NULL, nbPROTODB_MINIMAL, ErrBuf, sizeof(ErrBuf)) == nbFAILURE)
		{
			fprintf(stderr, "Error initializing the NetBee Library: %s\n", ErrBuf);
			return nbFAILURE;
		}
	}

	fprintf(stderr, "NetPDL Protocol database loaded.\n\n");

	PacketEngine= nbAllocatePacketEngine(ConfigParams.UseJit, ErrBuf, sizeof(ErrBuf));
	if (PacketEngine == NULL)
	{
		fprintf(stderr, "Error retrieving the PacketEngine: %s", ErrBuf);
		return nbFAILURE;
	}

	
	fprintf(stderr, "Compiling filter \'%s\'...\n", ConfigParams.FilterString);
	Res= PacketEngine->Compile((char* ) ConfigParams.FilterString, nbNETPDL_LINK_LAYER_ETHERNET);

	if (Res == nbFAILURE)
	{
		fprintf(stderr, "Error compiling the filter '%s': %s", ConfigParams.FilterString, PacketEngine->GetLastError());
		return nbFAILURE;
	}

	if (PacketEngine->InitNetVM(nbNETVM_CREATION_FLAG_COMPILEANDEXECUTE)== nbFAILURE)
	{
		fprintf(stderr, "Error initializing the netVM : %s", PacketEngine->GetLastError());
		return nbFAILURE;
	}

	if (ConfigParams.CaptureFileName)
	{
		// Let's set the source file
		if ((PcapHandle= pcap_open_offline(ConfigParams.CaptureFileName, ErrBuf)) == NULL)
		{
			fprintf(stderr, "Cannot open the capture source file: %s\n", ErrBuf);
			return nbFAILURE;
		}
	}
	else
	{
		// Let's set the device
		if ((PcapHandle= pcap_open_live(ConfigParams.AdapterName, 65535 /*snaplen*/,
					ConfigParams.PromiscuousMode, 1000 /* read timeout in ms */, ErrBuf)) == NULL)
		{
			fprintf(stderr, "Cannot open the capture interface: %s\n", ErrBuf);
			return nbFAILURE;
		}
	}

	// Open dump file to save packets
	PcapDumpHandle= pcap_dump_open(PcapHandle, ConfigParams.DumpFileName);

	if (PcapDumpHandle == NULL)
	{
		fprintf(stderr, "Error opening dump file: %s\n", pcap_geterr(PcapHandle));
		return nbFAILURE;
	}


	if (ConfigParams.CaptureFileName == NULL)
		fprintf(stderr, "\nReading network packets from interface: '%s'\n", ConfigParams.AdapterName);
	else
		fprintf(stderr, "\nReading network packets from  file: '%s'\n", ConfigParams.CaptureFileName);

	fprintf(stderr, "Filtering the following traffic: '%s'\n", ConfigParams.FilterString);
	fprintf(stderr, "Dumping accepted packets in file: '%s'\n\n", ConfigParams.DumpFileName);

	fprintf(stderr, "Starting capture...\n");


	// Initialize packet counters
	PacketCounter= 0;
	AcceptedPkts= 0;


	CMeasurement ElapsedTime;
	ElapsedTime.Start();

	while (1)
	{
	struct pcap_pkthdr *PktHeader;
	const unsigned char *PktData;
	int RetVal;
	int Result;

		RetVal= pcap_next_ex(PcapHandle, &PktHeader, &PktData);

		if (RetVal == -2)
			break;		// capture file ended

		if (RetVal < 0)
		{
			fprintf(stderr, "Cannot read packet: %s\n", pcap_geterr(PcapHandle));
			return nbFAILURE;
		}

		// Timeout expired
		if (RetVal == 0)
			continue;

		PacketCounter++;

		Result= PacketEngine->ProcessPacket(PktData, PktHeader->len);

		if (Result != nbFAILURE)
		{
			AcceptedPkts++;

			pcap_dump((unsigned char *) PcapDumpHandle, PktHeader, PktData);
			fprintf(stderr, "#");
		}
	}

	fprintf(stderr, "\n\n");
	ElapsedTime.EndAndPrint();

	fprintf(stderr, "\n\nThe filter accepted %d out of %d packets\n", AcceptedPkts, PacketCounter);
	fprintf(stderr, "Accepted packets are saved on the dump file.\n\n", AcceptedPkts, PacketCounter);

	nbDeallocatePacketEngine(PacketEngine);
	nbCleanup();

	// Close pcap-related structures
	pcap_dump_close(PcapDumpHandle);
	pcap_close(PcapHandle);

	return nbSUCCESS;
}

