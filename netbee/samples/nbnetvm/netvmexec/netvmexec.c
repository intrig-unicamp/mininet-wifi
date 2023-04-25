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
#include <time.h>

#include <nbnetvm.h>	// for the NetVM Framework API
#include "utils.h"
#include "multicore.h"



// Function prototypes
int ParseCommandLine(int argc, char *argv[]);


// Global variable for configuration
ConfigParams_t ConfigParams;
// We define our own measurement infrastructure since we do not want to rely on Netbee primitives,
// in order to lòink only against netvm and to avid mising C++ classes here
my_timer timer;


/* 
	Function for printing packet information 
*/
void print_packet(nvmExchangeBuffer *xbuffer)
{
u_int8_t *ipsource;
u_int8_t *ipdest;

    /* print the length of the packet */
	printf("len: %d ", xbuffer->PacketLen);

	//retrieve ip source and destination addresses
	ipsource = &xbuffer->PacketBuffer[26];
	ipdest = &xbuffer->PacketBuffer[30];

    /* print ip addresses and udp ports */
    printf("%d.%d.%d.%d -> %d.%d.%d.%d\n",
        ipsource[0],
        ipsource[1],
        ipsource[2],
        ipsource[3],
		ipdest[0],
		ipdest[1],
		ipdest[2],
		ipdest[3]);
}


/* 
	Function for printing packet information 
*/
void print_packet_from_info_data(nvmExchangeBuffer *xbuffer)
{
	u_int16_t ip_src_offs = 0;
	u_int16_t ip_src_len = 0;
	u_int16_t ip_dst_offs = 0;
	u_int16_t ip_dst_len = 0;
	
	u_int8_t *ipsource = NULL;
	u_int8_t *ipdest = NULL;

	ip_src_offs = *(u_int16_t*)&xbuffer->InfoData[0];
	ip_src_len = *(u_int16_t*)&xbuffer->InfoData[2];
	ip_dst_offs = *(u_int16_t*)&xbuffer->InfoData[4];
	ip_dst_len = *(u_int16_t*)&xbuffer->InfoData[6];
	
	// Retrieve ip source and destination addresses
	ipsource = &xbuffer->PacketBuffer[ip_src_offs];
	ipdest = &xbuffer->PacketBuffer[ip_dst_offs];

	printf("Packet %u\n", ConfigParams.n_pkt++);
	printf("ip.src: offs = %u, len = %u, value = ", ip_src_offs, ip_src_len);

	if (ip_src_len == 0)
		printf("null\n");
	else
		printf("%d.%d.%d.%d\n", ipsource[0], ipsource[1], ipsource[2], ipsource[3]);
		
	printf("ip.dst: offs = %u, len = %u, value = ", ip_dst_offs, ip_dst_len);

	if (ip_src_len == 0)
		printf("null\n");
	else
		printf("%d.%d.%d.%d\n\n",  ipdest[0], ipdest[1], ipdest[2], ipdest[3]);	
}



/*
	The user may provide a callback function that should be registered on an output application interface.
	This function is called when an exchange buffer is sent toward an output socket of the NetVM application.
*/
int32_t ApplicationCallback(nvmExchangeBuffer *xbuffer)
{
struct pcap_pkthdr pkthdr;
u_int8_t *packet;

	if (ConfigParams.dump_handle)
	{
		packet = xbuffer->PacketBuffer;
		pkthdr.caplen = xbuffer->PacketLen;
		pkthdr.len = xbuffer->PacketLen;
		pkthdr.ts.tv_sec = xbuffer->TStamp_s;
		pkthdr.ts.tv_usec = xbuffer->TStamp_us;
		pcap_dump((u_int8_t *) ConfigParams.dump_handle, &pkthdr, packet);
	}
	else if (ConfigParams.useInfo)
	{
		print_packet_from_info_data(xbuffer);
	}
	else
	{
		print_packet(xbuffer);
	}

	return nvmSUCCESS;
}


int32_t EmptyCallback(nvmExchangeBuffer *xbuffer)
{
	return nvmSUCCESS;
}


typedef int32_t callback(nvmExchangeBuffer *);

callback *netvmCallback= ApplicationCallback;



/*
	Basic function for retrieving packets from a capture file and injecting them into NetVM
*/
void write_packets(nvmAppInterface* InInterf)
{
struct pcap_pkthdr* pkthdr;
const u_int8_t* packet;
char errbuf[nvmERRBUF_SIZE];
u_int32_t n_Packets = 0;
	
	my_timer_start(&timer);

	// Start retrieving packets from file and inject them into NetVM
	while (pcap_next_ex(ConfigParams.in_descr, &pkthdr, &packet) == 1)
	{
		if (nvmWriteAppInterface (InInterf, (u_int8_t *) packet, pkthdr->len, pkthdr, errbuf) != nvmSUCCESS)
		{
			fprintf(stderr, "Error writing packets to the NetVM application interface\n");
			exit(1);
		}
		n_Packets++;
	}
	
	my_timer_stop(&timer);
	
	printf("Packets/second: %.3f\n", n_Packets / my_timer_elapsed (&timer));
}


/*
	Function for creating and intializing a NetVM application with the code contained in a file
	provided as argument.
	This function also starts the actual packet processing task
*/
int NetVMPacketProcess(char* filename) 
{
	//NetVM structures
	nvmByteCode				*BytecodeHandle = NULL;	//Bytecode image
	nvmNetVM				*NetVM=NULL;			//NetVM application Object
	nvmNetPE				*NetPE1=NULL;			//NetPE
	nvmSocket				*SocketIn=NULL;			//Input Socket	
	nvmSocket				*SocketOut=NULL;		//Output Socket
	nvmRuntimeEnvironment	*RT=NULL;				//Runtime environment
	nvmAppInterface			*InInterf=NULL;			//Input Application Interface 
	nvmAppInterface			*OutInterf=NULL;		//Output Application Interface 
	nvmPhysInterfaceInfo	*info=NULL;				//List of physical interface descriptors
	char					errbuf[nvmERRBUF_SIZE]; //buffer for error messages


	// Create a NetVM application for runtime execution
	NetVM = nvmCreateVM(nvmRUNTIME_COMPILEANDEXECUTE, errbuf);

	// Create a bytecode image by assembling a NetIL program read from file
	BytecodeHandle = nvmAssembleNetILFromFile(filename, errbuf);

	if (BytecodeHandle == NULL)
	{
		fprintf(stderr, "Error assembling netil source code: %s\n", errbuf);
		return -1;
	}
	
	// Create a PE inside NetVM using the bytecode image
	NetPE1 = nvmCreatePE(NetVM, BytecodeHandle, errbuf);
	if (NetPE1 == NULL)
	{
		fprintf(stderr, "Error Creating PE: %s\n", errbuf);
		return -1;
	}
	nvmSetPEFileName(NetPE1, filename);

	// Create two netvm sockets inside NetVM
	SocketIn= nvmCreateSocket(NetVM, errbuf);
	SocketOut= nvmCreateSocket(NetVM, errbuf);

	// Connect the netvm sockets to the input (i.e. 0) and output (i.e. 1) ports of the PE
	nvmConnectSocket2PE(NetVM, SocketIn, NetPE1, 0, errbuf);
	nvmConnectSocket2PE(NetVM, SocketOut, NetPE1, 1, errbuf);

	RT= nvmCreateRTEnv(NetVM, ConfigParams.creationFlag, errbuf);
	if (RT == NULL)
	{
		fprintf(stderr, "Error creating NetVM runtime environment\n");
		return -1;
	}

	if (ConfigParams.dobcheck)
	{
		ConfigParams.jitflags |= nvmDO_BCHECK;
	}
	else
	{
		ConfigParams.jitflags &= ~nvmDO_BCHECK;
	}

	if (ConfigParams.creationFlag == nvmRUNTIME_COMPILEONLY)
	{
		ConfigParams.jitflags &= ~nvmDO_NATIVE; //reset DO_NATIVE flag
		ConfigParams.jitflags |= nvmDO_ASSEMBLY;//set DO_ASSEMBLY flag

		if (nvmCompileApplication(NetVM,
					RT,
					ConfigParams.backend,
					ConfigParams.jitflags,
					ConfigParams.opt_level,
					NULL,
					errbuf) != nvmSUCCESS)
		{
			fprintf(stderr, "Error compiling the NetVM application:\n%s\n", errbuf);
			return -1;
		}
		
		printf("Target code:\n%s\n", nvmGetTargetCode(RT));
		return 0;
	}

	// Create an output interface for receiving packets from the NetVM
	OutInterf = nvmCreateAppInterfacePushOUT(RT, netvmCallback, errbuf);

	// Bind the output interface to the output socket
	nvmBindAppInterf2Socket(OutInterf,SocketOut);

	// Create an input interface
	if (ConfigParams.ifname)
	{	
		// Enumerate the available physical interfaces 
		info= nvmEnumeratePhysInterfaces(RT, errbuf);

		// Search the interface name into the list
		for (; info != NULL; info = info->Next)
		{
			if (strcmp(info->Name, ConfigParams.ifname) == 0 && info->InterfDir == INTERFACE_DIR_IN)
				break;
		}

		if (info == NULL)
		{
			fprintf(stderr, "Interface %s not found\n", ConfigParams.ifname);
			exit(1);
		}

		// Bind the physical inteface to the NetVM input socket
		nvmBindPhysInterf2Socket(info->PhysInterface,SocketIn);
	} 
	else if (ConfigParams.infile)
	{
		//create an application input inteface, since we manually inject packets into NetVM
		InInterf = nvmCreateAppInterfacePushIN(RT, errbuf);
		
		//bind the application interface to the NetVM input socket
		nvmBindAppInterf2Socket(InInterf, SocketIn);
	}
	else
	{
		fprintf(stderr, "you shoud provide an input file or an input interface!");
		exit(1);
	}


	// Initialize the runtime environment for the execution
	if (nvmNetStart(NetVM,RT, ConfigParams.usejit, ConfigParams.jitflags, ConfigParams.opt_level, errbuf) != nvmSUCCESS)
	{
		fprintf(stderr, "Error Starting the Runtime Environment:\n%s\n",errbuf);
		exit(1);
	}

	// The actual packet processing can start:
	if (!ConfigParams.infile)
	{
		// Start the physical interface
		nvmStartPhysInterface(info->PhysInterface, SocketIn, errbuf);
	}
	else
	{
		// Start reading packets from file
		write_packets(InInterf);
	}

	// Destroy the whole runtime environment
	nvmDestroyRTEnv(RT);

	return (nvmSUCCESS);
}


/*
	Print the usage help
*/
void Usage()
{
	fprintf (stderr, \
	"\nUsage: \n"\
	"  netvmexec [-r capfile] [-o outfile] [-i interf] [-j] [-O optlevel] netilfile\n\n" \
	"\nOptions:\n"\
	"  netilfile    File containing the NetIL program to execute\n"\
	"  -r capfile   Name of the input capture file (if no '-i' is defined)\n"\
	"  -o outfile   Output file where to save processed packets\n"\
	"  -i interf    Name of the input interface (if no '-r' is defined)\n"\
	"  -jit         Use jitted (native) code\n"\
	"  -nocheck     Disable boundscheck. For JIT only\n"
	"  -d backend   Dump the code generated by the specified NetVM backend\n"\
	"  -backends    Print a list of the available backends\n"\
	"  -O optlevel  Specify an optimization level\n"\
	"  -f           Field extraction sample (to be used with extract.netil)\n");
#ifdef ENABLE_MULTICORE
	fprintf(stderr, \
	"  -c ncores    Number of corse to use\n");
#endif
	fprintf(stderr, \
	"  -h           Prints this help message.\n\n"												\
	"Description\n"\
	"===============================================================================\n"\
	"This program loads an NetIL assembly file and executes it. It creates a NetVM\n"\
	"environment with a single NetPE (single input and single output) and it loads\n"\
	"the NetIL code into that PE.\n"\
	"This program has been engineered with filtering in mind; the output may be \n"\
	"unsatisfactory in case the program does not perform packet filtering.\n"\
	"Processing data (i.e., packets) is loaded from either a physical interface or a\n"\
	"file on disk.\n"\
	"Filtered packets can be saved on pcap file ('-o' option).\n\n"\
	"Note: please remember to turn on the multicore support in the makefile if\n"\
	"you want to compile this sample with the capability to run on multiple cores.\n\n");

	exit (1);
}


void PrintBackends(void)
{
nvmBackendDescriptor *BackendList;
u_int32_t NBackends = 0;
u_int32_t i = 0;

	BackendList= nvmGetBackendList(&NBackends);
	
	printf("\nAvailable backends:\n");
	printf("-------------------------------------------------------------------------------\n");
	printf("Name\tMax opt level\tInline\n");
	printf("-------------------------------------------------------------------------------\n");

	for (i = 0; i < NBackends; i++)
	{
		printf("%s\t", BackendList[i].Name);
		printf("   %2d\t\t", BackendList[i].MaxOptLevel);
		if ((BackendList[i].Flags & nvmDO_INLINE) != 0)
			printf("yes\n\n");
		else
			printf("no\n\n");
	}

	printf("\n");
}



int GetBackendIdFromName(char *Name)
{
nvmBackendDescriptor *BackendList;
u_int32_t NBackends = 0;
u_int32_t i=0;

	BackendList= nvmGetBackendList(&NBackends);

	for (i=0; i < NBackends; i++)
	{
		if (strcmp(Name, BackendList[i].Name)==0)
			return i;
	}

	return -1;
}


int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;

	// Set defaults
	ConfigParams.in_descr= NULL;
	ConfigParams.dump_handle= NULL;
	ConfigParams.count= 0;
	ConfigParams.usejit= 0;
	ConfigParams.backend= -1;
	ConfigParams.creationFlag= nvmRUNTIME_COMPILEANDEXECUTE;
	ConfigParams.dumpBackends= 0;
	ConfigParams.useInfo= 0;
	ConfigParams.opt_level= 3;
	ConfigParams.jitflags= nvmDO_NATIVE | nvmDO_BCHECK;
	ConfigParams.dobcheck= 1;
	ConfigParams.outfile= NULL;
	ConfigParams.netilfile= NULL;
	ConfigParams.infile= NULL;
	ConfigParams.ifname= NULL;
	ConfigParams.n_pkt= 0;

#ifdef ENABLE_MULTICORE
	ConfigParams.ncores= 0;
#endif
	// End defaults

	CurrentItem= 1;


	while (CurrentItem < argc)
	{
		if (strcmp(argv[CurrentItem], "-jit") == 0)
		{
			ConfigParams.usejit= 1;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-nocheck") == 0)
		{
			if (ConfigParams.usejit)
			{
				ConfigParams.jitflags= nvmDO_NATIVE;
				ConfigParams.dobcheck= 0;
				CurrentItem+= 1;
				continue;
			}
			else
			{
				printf("Option nocheck requires JIT compilation.\n");
				return nvmFAILURE;
			}
		}

		if (strcmp(argv[CurrentItem], "-O") == 0)
		{
			ConfigParams.opt_level= atoi(argv[CurrentItem + 1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-r") == 0)
		{
			ConfigParams.infile= argv[CurrentItem + 1];
			CurrentItem+= 2;
			continue;
		}
	
		if (strcmp(argv[CurrentItem], "-i") == 0)
		{
			ConfigParams.ifname= argv[CurrentItem + 1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-o") == 0)
		{
			ConfigParams.outfile= argv[CurrentItem + 1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-noout") == 0)
		{
			netvmCallback= EmptyCallback;
			CurrentItem+= 1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-backends") == 0)
		{
			PrintBackends();
			return nvmFAILURE;
		}

		if (strcmp(argv[CurrentItem], "-d") == 0)
		{
			ConfigParams.backend= GetBackendIdFromName(argv[CurrentItem + 1]);
			ConfigParams.creationFlag= nvmRUNTIME_COMPILEONLY;

			if (ConfigParams.backend == -1)
			{
				printf("\n\tBackend '%s' is not supported. Please use the '-backend' switch to get\n", argv[CurrentItem + 1]);
				printf("\tthe list of available backends.\n");
				return nvmFAILURE;
			}
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-f") == 0)
		{
			ConfigParams.useInfo= 1;
			CurrentItem+= 1;
			continue;
		}

#ifdef ENABLE_MULTICORE
		if (strcmp(argv[CurrentItem], "-c") == 0)
		{
			ConfigParams.ncores= atoi(argv[CurrentItem + 1]);
			CurrentItem+= 2;
			continue;
		}
#endif
		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nvmFAILURE;
		}

		if (argv[CurrentItem][0] == '-')
		{
			printf("\n\tError: parameter '%s' is not valid.\n", argv[CurrentItem]);
			return nvmFAILURE;
		}

		// Current parameter is the netil file, which does not have any switch (e.g. '-something') in front
		ConfigParams.netilfile= argv[CurrentItem++];
	}

	if (ConfigParams.infile && ConfigParams.ifname)
	{
		fprintf(stderr, "Input file and input interface are mutually exclusive!");
		return nvmFAILURE;
	}

	if (ConfigParams.netilfile == NULL)
	{
		fprintf(stderr, "Missing netil file.\n");
		return nvmFAILURE;
	}

	return nvmSUCCESS;
}


/*
	Main program
*/
int main (int argc, char **argv)
{
char errbuf[PCAP_ERRBUF_SIZE];
int i = 0;

	if (ParseCommandLine(argc, argv) == nvmFAILURE)
		return nvmFAILURE;

	if (ConfigParams.infile)
	{
		if ((ConfigParams.in_descr= pcap_open_offline(ConfigParams.infile, errbuf)) == NULL)
		{
			fprintf(stderr, "Cannot open input capture file %s: %s\n", ConfigParams.infile, errbuf);
			exit(1);
		}
	}
	if (ConfigParams.outfile)
	{
		if ((ConfigParams.dump_handle= pcap_dump_open(ConfigParams.in_descr, ConfigParams.outfile)) == NULL)
		{
			fprintf(stderr, "Cannot open output capture file%s: %s\n", ConfigParams.outfile, errbuf);
			exit(1);
		}
	}

	/* if user specified multicore run, call the right function*/
#ifdef ENABLE_MULTICORE
	if (ConfigParams.ncores > 0)
	{
		if (go_multi_core(ConfigParams.netilfile) != nvmSUCCESS)
			exit (-1);
	}
	else
	{
#endif
		if (NetVMPacketProcess(ConfigParams.netilfile) != nvmSUCCESS)
			exit(-1);
#ifdef ENABLE_MULTICORE
	}
#endif

	if (ConfigParams.in_descr)
		pcap_close(ConfigParams.in_descr);

	if (ConfigParams.dump_handle)
		pcap_dump_close(ConfigParams.dump_handle);

	exit(0);
}



