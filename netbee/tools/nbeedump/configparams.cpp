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
#include <nbnetvm.h>
#include "configparams.h"
#include "../utils/utils.h"


#ifdef WIN32
#define pcap_lib "WinPcap"
#else
#define pcap_lib "libpcap"
#endif

// Global variable for configuration
ConfigParams_t ConfigParams;

int PrintAdapters();
int GetAdapterName(int AdapterNumber, char *AdapterName, int AdapterNameSize);
void PrintBackends();
bool ValidateDumpBackend(int BackendID, nvmBackendDescriptor *BackendList);
int GetBackendIdFromName(char *Name, nvmBackendDescriptor *BackendList);

void Usage()
{
	char string0[]=	\
		"Usage:                                                                         \n"	\
		"  nbeedump [-netpdl filename] ( [-i adapter] [-p] | [-r filename] )            \n"	\
		"           [-w filename] [-c numpackets] [-n] [-q] [-validate]                 \n" \
		"           [-jit] [-noopt] [-backends] [-d [id [noopt] [inline]]]              \n";
	printf("\n%s", string0);

#ifdef	_DEBUG
	char string1[]=	\
		"           [-debug details_level] [-oc n_cycles]                               \n"	\
		"           [-dump_hir_code filename] [-dump_lir_code filename]                 \n"	\
		"           [-dump_netil_code filename] [-dump_lir_graph filename]              \n"	\
		"           [-dump_lir_no_opt_graph filename] [-dump_netil_graph filename]      \n"	\
		"           [-dump_no_code_graph filename] [-dump_proto_graph filename]         \n" \
		"           [-dump_filter_automaton filename]									\n";
#else
	char string1[]=	\
		"           [-debug details_level] [-d]                                         \n";
#endif
	printf("%s", string1);

	char string2[]=	\
		"           [-D] [-C max_file_size] [-s snaplen] [-h] [filterstring]            \n"	\
		"                                                                               \n"	\
		"                                                                               \n"	\
		"Basic options:                                                                 \n"	\
		" -h                                                                            \n"	\
		"        Print this help.                                                       \n"	\
		" -netpdl filename                                                              \n"	\
		"        Name of the file containing the NetPDL description. In case it is      \n"	\
		"        omitted, the NetPDL description embedded within the NetBee library will\n"	\
		"        be used.                                                               \n"	\
		" -validate                                                                     \n"	\
		"        It turns on the validation of the NetPDL file, just to be sure that    \n" \
		"        the protocol database is formally correct.                             \n" \
		" -D                                                                            \n"	\
		"        Display the list of interfaces available in the system.                \n"	\
		" -i interface                                                                  \n"	\
		"        Name or number of the interface that to be used for capturing packets. \n"	\
		"        It can be used only in case the '-r' parameter is void.                \n"	\
		" -p                                                                            \n"	\
		"        Do not put the adapter in promiscuous mode (default: promiscuous mode  \n"	\
		"        on). Valid only for live captures.                                     \n"	\
		" -r filename                                                                   \n"	\
		"        Name of the file containing the packet dump that has to be decoded. It \n"	\
		"        can be used only in case the '-i' parameter is void.                   \n"	\
		"        When this file is finished, nbeedump will check if another file named  \n"	\
		"        'filename1' is present; if so, it will open that as well, and so on.   \n"	\
		" -w filename                                                                   \n"	\
		"        Name of the file in which packets are saved. Packets are either printed\n"	\
		"        on screen or dumped on disk.                                           \n"	\
		" -c numpackets                                                                 \n"	\
		"        Capture only numpackets, then exit.                                    \n"	\
		" -C max_file_size (bytes)                                                      \n"	\
		"        Change the dump file when it exceedes 'max_file_size' bytes.           \n"	\
		"        This option is active only when the '-w' switch is used.               \n"	\
		" -s snaplen                                                                    \n"	\
		"        Capture only n_packets, then exit.                                     \n"	\
		" -n                                                                            \n"	\
		"        Do not resolve network addresses when printing packets on screen.      \n"	\
		" -q                                                                            \n"	\
		"        Quiet mode (no output).                                                \n"	\
		" -backends                                                                     \n"	\
		"        Print the list of implemented backends, highlighting which ones can be \n"	\
		"        used to dump the generated code.                                       \n"	\
		" -jit                                                                          \n"	\
		"        Compile the filter string into native (e.g. x86) code instead of using \n"	\
		"        NetVM interpreted code. For security reasons, the NetVM interpreter    \n"	\
		"        is used as default choice.                                             \n"	\
		" -noopt                                                                        \n"	\
		"        Do not optimize the intermediate code before producing the NetIL code. \n"	\
		"        This option can be used to prevent bugs caused by the NetIL optimizer. \n" \
		" filterstring                                                                  \n"	\
		"        Filter (in the NetPFL syntax) that defines which packets have to be    \n"	\
		"        captured and printed. In case the filterstring is missing, all packets \n"	\
		"        will be captured.                                                      \n"	\
		"                                                                               \n"	;
	printf("%s", string2);

	char string4[]=	\
		"                                                                               \n"	\
		"Debugging options:                                                             \n"	\
		" -debug details_level                                                          \n"	\
		"        Show execution information with the specified details level (0 for     \n"	\
		"        disabled, default; 1 for NetPDL initialization messages and simple     \n"	\
		"        execution information; 2 for NetPDL initialization messages and full   \n"	\
		"        execution information).                                                \n"	\
		" -d [backend_name [noopt] [inline] [-o filename]]                              \n"	\
		"        Dump the code generated by the specified backend on standard output and\n" \
		"        stop the execution. The 'backend_name' is the name of the backend as   \n" \
		"        given by the -backends option (it is \"default\" for the default one). \n" \
		"        The 'noopt' option can be used to disable optimizations before         \n" \
		"        generating the code. The 'inline' option can be only used if the       \n" \
		"        specified backend support it.                                          \n" \
		"        The list of available backends can be obtains by using the '-backends' \n" \
		"        option.                                                                \n" \
		"        If an optional filename is specified with the '-o' option, the assembly\n" \
		"        listing is written to file.                                            \n";
	printf("%s", string4);

#ifdef	_DEBUG
	char string5[]=	\
		" -dump_hir_code filename                                                       \n"	\
		"        Dump the HIR (high level) code on the specified file                   \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n"	\
		" -dump_lir_code filename                                                       \n"	\
		"        Dump the LIR (low level) code on the specified file                    \n"	\
		"        (otherwise the default name priveded by the NetBee library is used).   \n"	\
		" -dump_netil_code filename                                                     \n"	\
		"        Dump the NetIL code on the specified file                              \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n"	\
		"        This has the same effect of the '-o' option.                           \n"	\
		" -dump_mir_graph filename                                                      \n"	\
		"        Dump MIR (medium level) graph on the specified file                    \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n"	\
		" -dump_mir_no_opt_graph filename                                               \n"	\
		"        Dump MIR (before optimizations) graph on the specified file            \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n"	\
		" -dump_netil_graph filename                                                    \n"	\
		"        Dump NetIL graphs on the specified file                                \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n"	\
		" -dump_no_code_graph filename                                                  \n"	\
		"        Dump NetIL graph (without code) on the specified file                  \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n"	\
		" -dump_proto_graph filename                                                    \n"	\
		"        Dump protocol graph on the specified file                              \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n" \
		" -dump_filter_automaton filename                                               \n"	\
		"        Dump the automaton representing the filter on the specified file       \n"	\
		"        (otherwise the default name provided by the NetBee library is used).   \n";
	printf("%s", string5);
#endif

	char string6[]=	\
		"                                                                               \n"	\
		"Description                                                                    \n"	\
		"===============================================================================\n"	\
		"This program prints an extract of each packet, as defined in the visualization \n"	\
		"summary template on the NetPDL file. This program looks like tcpdump, but uses \n"	\
		"the protocol description contained in the NetPDL file and uses the NetVM       \n"	\
		"technology for packet filtering.                                               \n" \
		"                                                                               \n";
	printf("%s", string6);

	printf("\n");
}


int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;

	CurrentItem= 1;

	// Default values
	ConfigParams.ValidateNetPDL= false;
	ConfigParams.PromiscuousMode= 1;//PCAP_OPENFLAG_PROMISCUOUS;
	ConfigParams.DebugLevel= 0;
	ConfigParams.CaptureFileName= NULL;
	ConfigParams.CaptureFileNumber= 1;
	ConfigParams.AdapterName[0]= 0;
	ConfigParams.FilterString= NULL;
	ConfigParams.NPackets= -1;
	ConfigParams.SnapLen= 65535;
	ConfigParams.SaveFileName= NULL;
	ConfigParams.PcapDumpFile= NULL;
	ConfigParams.StopAfterDumpCode= false;
	ConfigParams.DumpCode= true;
	ConfigParams.QuietMode= false;
	ConfigParams.RotateFiles= 0;
	ConfigParams.UseJit= false;
	ConfigParams.NBackends= 1;
	ConfigParams.Backends[0].Id= 0;
	ConfigParams.Backends[0].Optimization= true;
	ConfigParams.Backends[1].Id= -1;
	ConfigParams.Backends[1].Optimization= true;

#ifdef	_DEBUG
	ConfigParams.DumpCodeFilename = nbNETPFLCOMPILER_DEBUG_ASSEMBLY_CODE_FILENAME;
	ConfigParams.DumpHIRCodeFilename = nbNETPFLCOMPILER_DEBUG_HIR_CODE_FILENAME;
	ConfigParams.DumpLIRCodeFilename = nbNETPFLCOMPILER_DEBUG_LIR_CODE_FILENAME;
	ConfigParams.DumpNetILGraphFilename = nbNETPFLCOMPILER_DEBUG_NETIL_GRAPH_FILENAME;
	ConfigParams.DumpNoCodeGraphFilename = nbNETPFLCOMPILER_DEBUG_NO_CODE_GRAPH_FILENAME;
	ConfigParams.DumpLIRNoOptGraphFilename = nbNETPFLCOMPILER_DEBUG_LIR_NOOPT_GRAPH_FILENAME;
	ConfigParams.DumpLIRGraphFilename = nbNETPFLCOMPILER_DEBUG_LIR_GRAPH_FILENAME;
	ConfigParams.DumpProtoGraphFilename = nbNETPFLCOMPILER_DEBUG_PROTOGRAH_DUMP_FILENAME;
	ConfigParams.DumpFilterAutomatonFilename = nbNETPFLCOMPILER_DEBUG_FILTERAUTOMATON_DUMP_FILENAME;
#endif
	// End defaults

	while (CurrentItem < argc)
	{

		if (strcmp(argv[CurrentItem], "-netpdl") == 0)
		{
			ConfigParams.NetPDLFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-validate") == 0)
		{
			ConfigParams.ValidateNetPDL= true;
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-r") == 0)
		{
			if (ConfigParams.AdapterName[0] != 0)
			{
				printf("Cannot specify the capture interface and a capture file at the same time.\n");
				return nbFAILURE;
			}

			ConfigParams.CaptureFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-w") == 0)
		{
			ConfigParams.SaveFileName= argv[CurrentItem+1];
			CurrentItem+= 2;

			//Set quiet mode to 'true'
			ConfigParams.QuietMode= true;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-C") == 0)
		{
			ConfigParams.RotateFiles= atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nbFAILURE;
		}

		if (strcmp(argv[CurrentItem], "-jit") == 0)
		{
			ConfigParams.UseJit = true;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-p") == 0)
		{
			ConfigParams.PromiscuousMode=0;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-c") == 0)
		{
			ConfigParams.NPackets= atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-s") == 0)
		{
			ConfigParams.SnapLen= atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-n") == 0)
		{
			ConfigParams.DoNotPrintNetworkNames=true;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-q") == 0)
		{
			ConfigParams.QuietMode= true;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-D") == 0)
		{
			PrintAdapters();
			return nbFAILURE;
		}

		if (strcmp(argv[CurrentItem], "-backends") == 0)
		{
			PrintBackends();
			return nbFAILURE;
		}

		if (strcmp(argv[CurrentItem], "-i") == 0)
		{
        char* Interface;
		int InterfaceNumber;

			if (ConfigParams.CaptureFileName != NULL)
			{
				printf("Cannot specify the capture interface and a capture file at the same time.\n");
				return nbFAILURE;
			}

			Interface= argv[CurrentItem+1];
			InterfaceNumber= atoi(Interface);

			// If we specify the interface by name, let's copy it in the proper structure
			// Otherwise, let's transform the interface index into a name
			if (InterfaceNumber == 0)
			{
				strncpy(ConfigParams.AdapterName, Interface, sizeof(ConfigParams.AdapterName));
				ConfigParams.AdapterName[sizeof(ConfigParams.AdapterName) - 1]= 0;
			}
			else
			{
				if (GetAdapterName(InterfaceNumber, ConfigParams.AdapterName, sizeof(ConfigParams.AdapterName)) == nbFAILURE)
				{
					printf("Error: the requested adapter is out of range.\n");
					return nbFAILURE;
				}
			}

			CurrentItem+=2;
			continue;
		}

		// Limit the number of optimization cycles
		if (strcmp(argv[CurrentItem], "-noopt") == 0)
		{
			ConfigParams.Backends[0].Optimization=false;
			CurrentItem+=1;
			continue;
		}

		// Debugging info levels
		if (strcmp(argv[CurrentItem], "-debug") == 0)
		{
			ConfigParams.DebugLevel= atoi(argv[CurrentItem+1]);
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-d") == 0)
		{
		nvmBackendDescriptor *BackendList;

			CurrentItem++;

			int id= -1;

			// Get the list of available backends
			BackendList= nvmGetBackendList(&(ConfigParams.NBackends));
			// We consider also the 'netil' backend, which is not returned back by the previous function
			ConfigParams.NBackends++;

			if (CurrentItem < argc)	// we have to avoid the case in which no other parameters follow
				sscanf(argv[CurrentItem], "%d", &id);

			if (id == -1)
			{
				if (CurrentItem < argc)
				{
					id= GetBackendIdFromName(argv[CurrentItem], BackendList);
					if (id == -1)
					{
						printf("\n\tBackend '%s' is not supported. Please use the '-backends' switch\n", argv[CurrentItem]);
						printf("\tto get the list of available backends.\n");
						return nbFAILURE;
					}
					else
						CurrentItem++;
				}
				else
				{
					printf("\n\tAfter the -d switch you need to specify the backend to use. Please use\n");
					printf("\tthe '-backends' switch to get the list of available backends.\n");
					return nbFAILURE;
				}
			}
			else
				CurrentItem++;

			if (id == 0)
			{
				// the default backend should be used
				ConfigParams.Backends[1].Id= -1;
				ConfigParams.Backends[1].Inline= false;
				ConfigParams.Backends[1].Optimization= true;
			}

			int bid=id;

			if (id > 0)
			{
				bid=1;
				ConfigParams.Backends[bid].Id=id;
			}

			if ((CurrentItem < argc) && strcmp(argv[CurrentItem], "noopt")==0)
			{
				ConfigParams.Backends[bid].Optimization=false;
				CurrentItem++;
			}

			if ((CurrentItem < argc) && strcmp(argv[CurrentItem], "inline")==0)
			{
				ConfigParams.Backends[bid].Inline= true;
				CurrentItem++;
			}


			if ((CurrentItem < argc) && strcmp(argv[CurrentItem], "-o") == 0)
			{
				ConfigParams.DumpCodeFilename = argv[CurrentItem+1];
				CurrentItem += 2;
			}

			if (ValidateDumpBackend(id, BackendList)==true)
			{
				ConfigParams.StopAfterDumpCode = true;
				ConfigParams.DumpCode=true;
			}
			continue;
		}

#ifdef	_DEBUG
		if (strcmp(argv[CurrentItem], "-dump_netil_code") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpCodeFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_lir_code") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpLIRCodeFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_hir_code") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpHIRCodeFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_netil_graph") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpNetILGraphFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_lir_graph") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpLIRGraphFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_lir_no_opt_graph") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpLIRNoOptGraphFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_no_code_graph") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpNoCodeGraphFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-dump_proto_graph") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpProtoGraphFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-dump_filter_automaton") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpFilterAutomatonFilename = argv[CurrentItem+1];
			CurrentItem+=2;
			continue;
		}
#endif

		if (argv[CurrentItem][0] == '-')
		{
			printf("\n\tError: parameter '%s' is not valid.\n", argv[CurrentItem]);
			return nbFAILURE;
		}

		// Current parameter is the filter string, which does not have any switch (e.g. '-something') in front
		if (ConfigParams.FilterString != NULL)
		{
			printf("\n\tError: only one filter must be defined without an option switch.\n");
			return nbFAILURE;
		}

		ConfigParams.FilterString = argv[CurrentItem];
		CurrentItem += 1;
		continue;
	}

	if ((ConfigParams.DumpCode) && (ConfigParams.StopAfterDumpCode) && ((ConfigParams.FilterString==NULL) || (ConfigParams.FilterString[0]=='\0')))
	{
		printf("\n\tNo filter specified! To dump the generated code you need to specify the filter to use.\n");
		return nbFAILURE;
	}

	if ((ConfigParams.DumpCode == false || ConfigParams.StopAfterDumpCode == false))
	{
		if ((ConfigParams.CaptureFileName == NULL) && (ConfigParams.AdapterName[0] == 0))
		{
            printf("Neither the capture file nor the capturing interface has been specified.\n");
			printf("Using the first adapter as default.\n");

			if (GetAdapterName(1, ConfigParams.AdapterName, sizeof(ConfigParams.AdapterName)) == nbFAILURE)
			{
				printf("Error when trying to retrieve the name of the first network interface.\n");
				printf("    Make sure that %s is installed, and that you have the right\n", pcap_lib);
				printf("    permissions to open the network adapters.\n\n");
				return nbFAILURE;
			}
		}
	}

	return nbSUCCESS;
}


void ListDevices()
{
pcap_if_t *alldevs;
pcap_if_t *device;
int i=0;
char errbuf[PCAP_ERRBUF_SIZE];

	// Retrieve the device list from the local machine and print it
	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		printf("\tError getting the list of installed adapters: %s\n", errbuf);
		exit(1);
	}


	printf("\nAvailable network interfaces:\n");
	printf(	"-------------------------------------------------------------------------------\n");
	printf(" N\tName\n");
	printf("  \tDescription\n");
	printf(	"-------------------------------------------------------------------------------\n");
	// Print the list
	for(device= alldevs; device != NULL; device= device->next)
	{
		printf("%2u\t%s\n", ++i, device->name);
		if (device->description)
			printf("  \t%s\n\n", device->description);
		else
			printf("  No description available\n\n");
	}

	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure that %s is installed, and that you have\n", pcap_lib);
		printf("the right permissions to open the network adapters.\n");
		return;
	}
	printf(	"-------------------------------------------------------------------------------\n");
	// We don't need any more the device list. Free it
	pcap_freealldevs(alldevs);
}


int PrintAdapters()
{
	ListDevices();
#if 0
	nbPacketCapture PacketCapture;
	int IfNumber;

	nbNetVmPortLocalAdapter *DeviceList= PacketCapture.GetListLocalAdapters();
	if (DeviceList == NULL)
	{
		printf("Error getting the devices installed in the system: %s\n", PacketCapture.GetLastError() );
		return nbFAILURE;
	}

	printf("\nAdapters installed in the system:");
	printf("\n=================================\n\n");

	IfNumber= 1;

	while (DeviceList)
	{
		printf("Interface %d: %s (%s)\n", IfNumber, DeviceList->Name, DeviceList->Description);

		DeviceList= DeviceList->GetNext();
		IfNumber++;
	}
#endif
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


int GetBackendIdFromName(char *Name, nvmBackendDescriptor *BackendList)
{
	if (strcmp(Name, "netil")==0)
		return 0;

	if (strcmp(Name, "default")==0)
		return 1;

	for (u_int32_t i=0; i < (ConfigParams.NBackends - 1); i++)
	{
		if (strcmp(Name, BackendList[i].Name)==0)
			return i+1;
	}

	return -1;
}


bool ValidateDumpBackend(int BackendID, nvmBackendDescriptor *BackendList)
{
	if (BackendID == 0)
	{
		if (ConfigParams.Backends[0].Inline != 0)
			printf("\tThe NetVM backend does not accept the inline options, and it will be ignored.\n");
	}
	else
	{
		if ((BackendID < 1) || ((u_int32_t) BackendID > (ConfigParams.NBackends - 1)))
		{
			printf("\tAvailable backends can be identifies from 0 to %u. The default backend ('netil') will be used.\n", ConfigParams.NBackends - 1);
			ConfigParams.Backends[1].Id= -1;
			ConfigParams.Backends[1].Optimization= true;
			ConfigParams.Backends[1].Inline= false;
			return false;
		}

		if (ConfigParams.Backends[1].Inline && (BackendList[BackendID-1].Flags & nvmDO_INLINE)==0)
		{
			printf("\tThe specified backend does not accept the inline options, and it will be ignored.\n");
			ConfigParams.Backends[1].Inline= false;
		}
	}

	return true;
}


void PrintBackends()
{
nvmBackendDescriptor *BackendList;
u_int32_t NBackends;

	// Get the list of available backends
	BackendList= nvmGetBackendList(&NBackends);
	// We consider also the 'netil' backend, which is not returned back by the previous function
	NBackends++;

	printf("\nAvailable backends:\n");
	printf("--------------------------------\n");
	printf("Name\tMax opt level\tInline\n");
	printf("--------------------------------\n");

	printf("%s\t", "default");
	printf("   %2d\t\t", 0);
	printf("no\n");

	printf("%s\t", "netil");
	printf("   %2d\t\t", -1);
	printf("no\n");

	for (u_int32_t i=0; i < NBackends-1; i++)
	{
		printf("%s\t", BackendList[i].Name);
		printf("   %2d\t\t", BackendList[i].MaxOptLevel);

		if ((BackendList[i].Flags & nvmDO_INLINE) != 0)
			printf("yes\n");
		else
			printf("no\n");
	}

	printf("\n\n");
}
