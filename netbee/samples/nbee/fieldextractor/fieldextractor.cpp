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
#define DEFAULT_EXTRACTSTRING "extractfields(ip.src, ip.dst)"
#define DEFAULT_CAPTUREFILENAME "samplecapturedump.acp"
#define MAX_FIELD_SIZE 2048		/* We do not expect a field larger than this value */


typedef struct _ConfigParams
{
	const char*	NetPDLFileName;
	const char*	CaptureFileName;
	char		AdapterName[1024];
	int		PromiscuousMode;
	bool		UseJit;
	const char*	NetPFLExtractString;
} ConfigParams_t;


// Global variable for configuration
ConfigParams_t ConfigParams;



// Function prototypes
int GetAdapterName(int AdapterNumber, char *AdapterName, int AdapterNameSize);



void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  fieldextractor [options] [extractstring]\n\n"	\
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
	" -jit                                                                          \n"	\
	"        Make NetVM to use the native code on the current platform instead of   \n"	\
	"        NetIL code; by default the NetIL code is used (for safety reasons).    \n"	\
	" [extractstring]                                                               \n" \
	"        List of fields to be extracted from network packets, with an optional  \n" \
	"        filter in front (e.g. 'ip extractfields(ip.src,ip.dst)')               \n" \
	"        (default: extractfields(ip.src,ip.dst))						        \n" \
	" -h: prints this help message.\n\n"												\
	"Description\n"																		\
	"=============================================================================\n"	\
	"This program prints the value of the given fields extracted from each packet.\n"	\
	"An asterisk ('*') will be printed if the field is not available.\n\n";

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
	ConfigParams.NetPFLExtractString = DEFAULT_EXTRACTSTRING;
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
		ConfigParams.NetPFLExtractString = argv[CurrentItem++];
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

// Data for parsing packets and extracting fields
nbPacketEngine *PacketEngine;
nbExtractedFieldsReader* FieldReader;
_nbExtractedFieldsDescriptor *DescriptorVector;
_nbExtractFieldInfo *ExtractedFieldsInfoVector;
int numFields;
nbNetPDLUtils* NetPDLUtils;
int PacketCounter;
int AcceptedPkts;

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	fprintf(stderr, "\nLoading NetPDL protocol database...\n");

	if (ConfigParams.NetPDLFileName)
	{
		int Res;

		Res= nbInitialize(ConfigParams.NetPDLFileName, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf) );

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
		if (nbInitialize(NULL, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf)) == nbFAILURE)
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

	
	fprintf(stderr, "Compiling filter \'%s\'...\n", ConfigParams.NetPFLExtractString);
	Res= PacketEngine->Compile((char* ) ConfigParams.NetPFLExtractString, nbNETPDL_LINK_LAYER_ETHERNET);
	

	if (Res == nbFAILURE)
	{
		fprintf(stderr, "Error compiling the filter '%s': %s", ConfigParams.NetPFLExtractString, PacketEngine->GetLastError());
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

	if (ConfigParams.CaptureFileName == NULL)
		fprintf(stderr, "\nReading network packets from interface: %s \n\n", ConfigParams.AdapterName);
	else
		fprintf(stderr, "\nReading network packets from file: %s \n\n", ConfigParams.CaptureFileName);

	fprintf(stderr, "===============================================================================\n");


	// Initialize packet counters
	PacketCounter= 0;
	AcceptedPkts= 0;

	// Initialize classes for packet processing
	NetPDLUtils= nbAllocateNetPDLUtils(ErrBuf, sizeof(ErrBuf));
	FieldReader= PacketEngine->GetExtractedFieldsReader();

	// Get the DescriptorVector of fields that will be extracted from each packet
	DescriptorVector=FieldReader->GetFields();
	
	numFields = FieldReader->getNumFields();
	
	ExtractedFieldsInfoVector = FieldReader->GetFieldsInfo();

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

			printf("Packet %d Timestap %d.%d\n", PacketCounter, (int) PktHeader->ts.tv_sec, (int) PktHeader->ts.tv_usec);
			

			for (int j=0; j < numFields; j++)
			{
			char FormattedField[1024];

				switch (ExtractedFieldsInfoVector[j].FieldType)
				{
					case PDL_FIELD_TYPE_ALLFIELDS:
					{
						
					};
					break;

					case PDL_FIELD_TYPE_BIT:
					{
						
					};
					break;

					default:
					{
						if (DescriptorVector[j].Valid)
						{
							if (DescriptorVector[j].Length > MAX_FIELD_SIZE)
							{
								fprintf(stderr, "The field size is larger than the max value allowed in this program.");
								return nbFAILURE;
							}

							// If the raw packet dump is enough for you, you can get rid off this function
							Res= NetPDLUtils->FormatNetPDLField(ExtractedFieldsInfoVector[j].Proto,ExtractedFieldsInfoVector[j].Name,
																	PktData + DescriptorVector[j].Offset, DescriptorVector[j].Length, FormattedField,
																	sizeof(FormattedField));
							
							if (Res == nbSUCCESS)
							{
								printf("\t%s.%s: offset=%d len=%d value=%s\n", ExtractedFieldsInfoVector[j].Proto,ExtractedFieldsInfoVector[j].Name,
											DescriptorVector[j].Offset, DescriptorVector[j].Length, FormattedField);
							}
						}
						else
						{
							printf("\t%s.%s: offset= * len= * value= *\n", ExtractedFieldsInfoVector[j].Proto,ExtractedFieldsInfoVector[j].Name);
						}
					}
				}
			}
		}
	}

	ElapsedTime.EndAndPrint();

	fprintf(stderr, "\n\nThe filter accepted %d out of %d packets\n", AcceptedPkts, PacketCounter);

	if (PcapHandle)
		pcap_close(PcapHandle);

	nbDeallocatePacketEngine(PacketEngine);
	nbCleanup();

	return nbSUCCESS;
}

