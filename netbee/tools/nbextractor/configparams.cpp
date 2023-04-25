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


#include <nbee.h>
#include <nbnetvm.h>
#include "configparams.h"


#ifdef WIN32
#define pcap_lib "WinPcap"
#else
#define pcap_lib "libpcap"
#endif

#ifdef ENABLE_SQLITE3
#define SQLITE3_RELATED_FMT \
" -sqldb databasefilename                                                       \n" \
"        Name of the database file where the extracted fields will be dumped.   \n" \
"        This switch the tool to quiet mode (no packets' output on screen).     \n" \
"        Please note that for performance reasons the sqlite database will have \n" \
"        both journaling and syncronous writes disabled. So, you may lose some  \n" \
"        data in case something goes wrong (e.g., disk failure, etc.), but this \n" \
"        guarantees the best thoughput, which is what matters here.             \n"

#define SQLITE3_RELATED_OPTS \
" -sqltable tablename                                                           \n" \
"        Name of the table in database to dump into.                            \n" \
"        Should be used with -sqldb format specifier; if not provided,          \n" \
"        then the default value 'DefaultDump' will be used.                     \n" \
" -sqlindex field                                                               \n" \
"        Field on which an index must be created.                               \n" \
"        It can be repeated an arbitrary number of times.                       \n" \
" -sqltransize transaction_size                                                 \n" \
"        Number of records that have to be aggregated within a single           \n" \
"        transaction in order to be 'inserted' in the database. Aggregating     \n" \
"        multiple insertions within a single transaction decreases              \n" \
"        dramatically the overhead of the database. Default: no transactions    \n" \
"        are used (each 'insert' is atomic); a reasonable value is about        \n" \
"        20000 insertions per transaction.                                      \n"

#define SQLITE3_RELATED_ARGS2 \
"        This option rotates SQL databases as well, following the same rules.   \n"

#else
#define SQLITE3_RELATED_FMT
#define SQLITE3_RELATED_OPTS
#define SQLITE3_RELATED_ARGS2
#endif

#ifdef ENABLE_REDIS
#define REDIS_RELATED_ARGS \
" -redistcp host:port                                                           \n" \
"        Host address and port of the Redis DB.                                 \n" \
"        Will switch to quiet mode (no packets' output on screen).              \n" \
" -redisunix socketname                                                         \n" \
"        Path and name of unix sockete of Redis DB.                             \n" \
" -redismulti number                                                            \n" \
"        Number of record to insert before EXEC.                                \n" \
"        Default: 1				                                \n" \
" -redischannel channel_name							\n" \
"        Name of channel where pubblish records.                                \n" \
"        Default: NBExtractor                                                   \n" \
" -redisexpire seconds								\n" \
"        Time to live in seconds of the keys.                                   \n" \
"        Default: 10                                                            \n" \
" -redissocks number                                                            \n" \
"        Number of socket toward Redis DB.                                      \n" \
"        Default: 1                                                            \n" 
#else
#define REDIS_RELATED_ARGS
#endif

// Global variable for configuration
ConfigParams_t ConfigParams;

void PrintAdapters();
int GetAdapterName(int AdapterNumber, char *AdapterName, int AdapterNameSize);
void PrintBackends();
bool ValidateDumpBackend(int BackendID, nvmBackendDescriptor *BackendList);
int GetBackendIdFromName(char *Name, nvmBackendDescriptor *BackendList);


void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  nbextractor [format specifier] [options] extractstring\n\n"	\
	"The format specifier is at most one among:\n                                   \n" \
	" -cp                                                                           \n" \
	"        Use compact printing (one line per packet).                            \n" \
	" -scp                                                                          \n" \
	"        Use super-compact printing (one line per packet, no other data).       \n" \
	" -scpn                                                                         \n" \
	"        Use super-compact printing, but with the packet number.                \n" \
	" -scpt                                                                         \n" \
	"        Use super-compact printing, but with the timestamp.                    \n" \
	SQLITE3_RELATED_FMT \
	"                                                                               \n" \
	"Options:\n                                                                     \n" \
	" -h                                                                            \n" \
	"        Print this help.                                                       \n" \
	" -netpdl filename                                                              \n" \
	"        Name of the file containing the NetPDL description. In case it is      \n" \
	"        omitted, the NetPDL description embedded within the NetBee library will\n" \
	"        be used.                                                               \n" \
	" -D                                                                            \n" \
	"        Display the list of interfaces available in the system.                \n" \
	" -i interface                                                                  \n" \
	"        Name or number of the interface that has to be used for capturing      \n" \
	"        packets. It can be used only in case the '-r' parameter is void.       \n" \
	" -p                                                                            \n" \
	"        Do not put the adapter in promiscuous mode (default: promiscuous mode  \n" \
	"        on). Valid only for live captures.                                     \n" \
	" -r filename                                                                   \n"	\
	"        Name of the file containing the packet dump that has to be processed.  \n"	\
	"        It can be used only in case the '-i' parameter is void.                \n"	\
	"        When this file is finished, nbeextractor will check if another file    \n"	\
	"        named 'filename1' is present; if so, it will open that as well,        \n"	\
	"        and so on.                                                             \n"	\
	" -c numpackets                                                                 \n" \
	"        Capture only numpackets, then exit.                                    \n" \
	" -q                                                                            \n" \
	"        Quiet mode (no output).                                                \n" \
	SQLITE3_RELATED_OPTS \
	REDIS_RELATED_ARGS \
	" -anonip filename argument_list                                                \n" \
	"        'filename' is the name of the configuration file containing            \n" \
	"        the IP address ranges that should be anonymized.                       \n" \
	"        'argument_list' is the list of the extractfields() arguments           \n" \
	"        to be anonymized: it is expressed as a list of numbers, i.e.           \n" \
	"        '1,2' denotes that the first and the second argument should            \n" \
	"        be anonymized.                                                         \n" \
	"        The file is simply a list of networks in the form a.b.c.d/prefix, one  \n" \
	"        per line (lines beginning with '#' are ignored), and each IP address   \n" \
	"        belonging to one of those ranges will be scrambled with another        \n" \
	"        address belonging to the same range.                                   \n" \
	"        Mapping is different in different ranges, but is kept for the          \n" \
	"        entire duration of the capture.                                        \n" \
	" -r filename                                                                   \n" \
	"        Name of the file containing the packet dump that has to be decoded. It \n" \
	"        can be used only in case the '-i' parameter is void.                   \n" \
	" -w filename                                                                   \n" \
	"        Name of the file in which packets are saved. Packets are either printed\n" \
	"        on screen or dumped on disk.                                           \n" \
	" -C number_of_packets                                                          \n" \
	"        Change the file in which results are saved every 'number_of_packets'.  \n" \
	"        This option is active only when the '-w' switch is used.               \n" \
	SQLITE3_RELATED_ARGS2 \
	" -jit                                                                          \n" \
	"        Make NetVM to use the native code on the current platform instead of   \n" \
	"        NetIL code; by default the NetIL code is used (for safety reasons).    \n" \
	" -noopt                                                                        \n" \
	"        Do not optimize the intermediate code before producing the NetIL code. \n" \
	"        This option can be used to prevent bugs caused by the NetIL optimizer. \n" \
	" -backends                                                                     \n" \
	"        Print the list of implemented backends, highlighting which ones can be \n" \
	"        used to dump the generated code.                                       \n" \
	" -d [backend_name [noopt] [inline]]                                            \n" \
	"        Dump the code generated by the specified backend on standard output and\n" \
	"        stop the execution. The 'backend_name' is the name of the backend as   \n" \
	"        given by the -backends option (it is \"default\" for the default one). \n" \
	"        The 'noopt' option can be used to disable optimizations before         \n" \
	"        generating the code. The 'inline' option can be only used if the       \n" \
	"        specified backend support it.                                          \n" \
	"        The list of available backends can be obtained by using the '-backends'\n" \
	"        option.                                                                \n" \
/*	" -nojit                                                                        \n"	\
	"        Make NetVM to use the NetIL interpreter instead of the JIT compiled    \n"	\
	"        code; by default the NetIL code is passed to the jitter.               \n"	\ */
	" extractstring                                                                 \n" \
	"        String (in the NetPFL syntax) that defines which packets have to be    \n" \
	"        captured and which fields have to be extracted. For example, the string\n" \
	"        'extractfields(ip.src,ip.dst) will extract IP source and destination.  \n" \
	" -dump_hir_code filename                                                       \n" \
	"        Dump the HIR code on the specified file.                               \n" \
	" -dump_netil_code filename                                                     \n" \
	"        Dump the NetIL code on the specified file.                             \n" \
	" -dump_filter_automaton filename                                               \n"	\
	"        Dump the automaton representing the filter on the specified file       \n" \
	"        (otherwise the default name provided by the NetBee library is used).   \n" \
	"                                                                               \n" \
	"Description                                                                    \n" \
	"===============================================================================\n" \
	"This program prints a list of field that have to be extracted from each packet.\n" \
	"For example if the extractstring is 'ip.src', it will print the IP source      \n" \
	"address contained in each input packet.                                        \n" \
	"                                                                               \n" \
	"Additional info                                                                \n" \
	"===============================================================================\n" \
	"This program supports rotating files when dumping the result on disk (only on  \n" \
	"Linux). The current file will be closed and another file will be opened when a \n" \
	"SIGHUP signal is caught.\n\n";

	fprintf(stderr, "%s", string);
}



int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;

	CurrentItem= 1;

	// Default values
	ConfigParams.PromiscuousMode= 1;		// PCAP_OPENFLAG_PROMISCUOUS;
	ConfigParams.CaptureFileName= NULL;
	ConfigParams.CaptureFileNumber= 1;
	ConfigParams.AdapterName[0]= 0;
	ConfigParams.SaveFileName= NULL;
	ConfigParams.FilterString= NULL;
	ConfigParams.UseJit= false;
	ConfigParams.PrintingMode= DEFAULT;
	ConfigParams.RotateFiles= 0;
	ConfigParams.NPackets= -1;
	ConfigParams.QuietMode= false;

	ConfigParams.NBackends= 1;
	strcpy(ConfigParams.DumpCodeFilename, nbNETPFLCOMPILER_DEBUG_ASSEMBLY_CODE_FILENAME);
	ConfigParams.DumpFilterAutomatonFilename = nbNETPFLCOMPILER_DEBUG_FILTERAUTOMATON_DUMP_FILENAME;
	ConfigParams.StopAfterDumpCode= false;
	ConfigParams.DumpCode= true;
	ConfigParams.Backends[0].Id= 0;
	ConfigParams.Backends[0].Optimization= true;
	ConfigParams.Backends[1].Id= -1;
	ConfigParams.Backends[1].Optimization= true;

	ConfigParams.IPAnonFileName= NULL;
	ConfigParams.IPAnonFieldsList= NULL;

#ifdef  ENABLE_SQLITE3
	ConfigParams.SQLDatabaseFileBasename= NULL;
	ConfigParams.SQLTableName= (char*) "DefaultDump";
	ConfigParams.SQLTransactionSize= 0;
	ConfigParams.IndexNo = 0;
#endif

#ifdef  ENABLE_REDIS
	ConfigParams.RedisMulti = 1;
	ConfigParams.RedisChannel = (char*) "NBExtractor";
	ConfigParams.RedisExpire = 10;
	ConfigParams.RedisSocks = 1;
#endif

// End defaults


	while (CurrentItem < argc)
	{

		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nbFAILURE;
		}

		if (strcmp(argv[CurrentItem], "-netpdl") == 0)
		{
			ConfigParams.NetPDLFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
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
			continue;
		}

		if (strcmp(argv[CurrentItem], "-C") == 0)
		{
			ConfigParams.RotateFiles= atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-c") == 0)
		{
			ConfigParams.NPackets= atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-cp") == 0)
		{
			if (ConfigParams.PrintingMode != DEFAULT)
			{
				printf("Error with format specifier '-cp': another format specifier has already been specified.\n");
				return nbFAILURE;
			}
			ConfigParams.PrintingMode= CP;
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-scp") == 0)
		{
			if (ConfigParams.PrintingMode != DEFAULT)
			{
				printf("Error with format specifier '-scp': another format specifier has already been specified.\n");
				return nbFAILURE;
			}
			ConfigParams.PrintingMode= SCP;
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-scpt") == 0)
		{
			if (ConfigParams.PrintingMode != DEFAULT)
			{
				printf("Error with format specifier '-scpt': another format specifier has already been specified.\n");
				return nbFAILURE;
			}
			ConfigParams.PrintingMode= SCPT;
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-scpn") == 0)
		{
			if (ConfigParams.PrintingMode != DEFAULT)
			{
				printf("Error with format specifier '-scpn': another format specifier has already been specified.\n");
				return nbFAILURE;
			}
			ConfigParams.PrintingMode= SCPN;
			CurrentItem++;
			continue;
		}

#ifdef ENABLE_SQLITE3
		if (strcmp(argv[CurrentItem], "-sqldb") == 0)
		{
			if (ConfigParams.PrintingMode != DEFAULT)
			{
				printf("Error with format specifier '-sqldb': another format specifier has already been specified.\n");
				return nbFAILURE;
			}
			ConfigParams.PrintingMode= SQLITE3;
			ConfigParams.SQLDatabaseFileBasename = argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-sqltable") == 0)
		{
			ConfigParams.SQLTableName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-sqlindex") == 0)
		{
			ConfigParams.SQLIndexFields[ConfigParams.IndexNo] = (char*)malloc(sizeof(char)*strlen(argv[CurrentItem+1])+1);
			ConfigParams.SQLIndexFields[ConfigParams.IndexNo] = argv[CurrentItem+1];
			ConfigParams.IndexNo++;
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-sqltransize") == 0)
		{
			// Please note that in SQLite 3 no changes can be made to the database except within a transaction.
			// Any command that changes the database (basically, any SQL command other than SELECT)
			// will automatically start a transaction if one is not already in effect.
			// Automatically started transactions are committed when the last query finishes.
			//
			// The 'SQLTransactionSize' is used to define manually the transactions and group
			// multiple insertions in the same commit, which boosts the efficiency quite a lot.

			ConfigParams.SQLTransactionSize = atoi( argv[CurrentItem+1] );
			CurrentItem+= 2;
			continue;
		}
#endif

#ifdef ENABLE_REDIS
		if (strcmp(argv[CurrentItem], "-redistcp") == 0)
		{
			if (ConfigParams.PrintingMode != DEFAULT)
			{
				printf("Error with output specifier '-redistcp': another format specifier has already been specified.\n");
				return nbFAILURE;
			}
			ConfigParams.PrintingMode = REDIS;
			ConfigParams.RedisSockType = TCP;
			ConfigParams.RedisSock.RedisTCP = argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-redisunix") == 0)
		{
			if (ConfigParams.PrintingMode != DEFAULT)
			{
				printf("Error with output specifier '-redisunix': another format specifier has already been specified.\n");
				return nbFAILURE;
			}
			ConfigParams.PrintingMode= REDIS;
			ConfigParams.RedisSockType = Unix;
			ConfigParams.RedisSock.RedisUnix = argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-redismulti") == 0)
		{
			ConfigParams.RedisMulti = atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-redischannel") == 0)
		{
			ConfigParams.RedisChannel = argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-redisexpire") == 0)
		{
			ConfigParams.RedisExpire = atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-redissocks") == 0)
		{
			ConfigParams.RedisSocks = atoi(argv[CurrentItem+1]);
			CurrentItem+= 2;
			continue;
		}

#endif

		if (strcmp(argv[CurrentItem], "-anonip") == 0)
		{
			ConfigParams.IPAnonFileName= argv[CurrentItem+1];
			ConfigParams.IPAnonFieldsList= argv[CurrentItem+2];
			CurrentItem+= 3;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-jit") == 0)
		{
			ConfigParams.UseJit= true;
			CurrentItem+= 1;
			continue;
		}


		if (strcmp(argv[CurrentItem], "-p") == 0)
		{
			ConfigParams.PromiscuousMode=0;
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

		if (strcmp(argv[CurrentItem], "-noopt") == 0)
		{
			ConfigParams.Backends[0].Optimization= false;
			CurrentItem+=1;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-backends") == 0)
		{
			PrintBackends();
			return nbFAILURE;
		}
		
		
		if (strcmp(argv[CurrentItem], "-dump_netil_code") == 0)
		{
			ConfigParams.NetILOutputFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}
		
		
		if (strcmp(argv[CurrentItem], "-dump_hir_code") == 0)
		{
			ConfigParams.HIROutputFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}
		
		if (strcmp(argv[CurrentItem], "-dump_filter_automaton") == 0)
		{
			if (strncmp(argv[CurrentItem+1], "-", 255) != 0)
				ConfigParams.DumpFilterAutomatonFilename = argv[CurrentItem+1];
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
						printf("\n\tBackend '%s' is not supported. Please use the '-backends' switch to get\n", argv[CurrentItem]);
						printf("\tthe list of available backends.\n");
						return nbFAILURE;
					}
					else
						CurrentItem++;
				}
				else
				{
					printf("\n\tAfter the -d switch you need to specify the backend to use. Please use the '-backends'\n");
					printf("\tswitch to get the list of available backends.\n");
					return nbFAILURE;
				}
			}
			else
				CurrentItem++;

			if (id == 0)
			{
				// the default backend should be used
				ConfigParams.Backends[1].Id= -1;
				ConfigParams.Backends[1].Inline= 0;
				ConfigParams.Backends[1].Optimization= true;
			}

			int bid=id;

			if (id > 0)
			{
				bid= 1;
				ConfigParams.Backends[bid].Id= id;
			}

			if ((CurrentItem < argc) && strcmp(argv[CurrentItem], "noopt")==0)
			{
				ConfigParams.Backends[bid].Optimization= false;
				CurrentItem++;
			}

			if ((CurrentItem < argc) && strcmp(argv[CurrentItem], "inline")==0)
			{
				ConfigParams.Backends[bid].Inline= true;
				CurrentItem++;
			}


			if (ValidateDumpBackend(id, BackendList) == true)
			{
				ConfigParams.StopAfterDumpCode= true;
				ConfigParams.DumpCode=true;
			}
			continue;
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


		if (argv[CurrentItem][0] == '-')
		{
			printf("\n\tError: parameter '%s' is not valid.\n", argv[CurrentItem]);
			return nbFAILURE;
		}

		// We do not have any '-' switch; so, it should be the filter string
		ConfigParams.FilterString = argv[CurrentItem];
		CurrentItem += 1;
		continue;
	}

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

	if (ConfigParams.FilterString == NULL)
	{
		printf("\n\tCommand line error: missing the filtering/extraction string\n  (e.g. 'ip extractfields(ip.src,ip.dst)').\n");
		return nbFAILURE;
	}

	if ((ConfigParams.IPAnonFileName != NULL) && (ConfigParams.IPAnonFieldsList == NULL))
	{
		printf("\n\tCommand line error: IP anonymization argument missing!\n");
		return nbFAILURE;
	}

	return nbSUCCESS;
}


void PrintAdapters()
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
		if (ConfigParams.Backends[0].Inline!=0)
			printf("\tThe NetVM backend does not accept the inline options, and it will be ignored.\n");
		ConfigParams.DumpCodeFilename[0]= 0;
	}
	else
	{
		if ((BackendID < 1) || ((u_int32_t) BackendID > (ConfigParams.NBackends - 1)))
		{
			printf("\tAvailable backends can be identifies from 0 to %u. The default backend ('netil') will be used.\n", ConfigParams.NBackends - 1);
			ConfigParams.Backends[1].Id= -1;
			ConfigParams.Backends[1].Optimization= true;
			ConfigParams.Backends[1].Inline= 0;
			return false;
		}

		if (ConfigParams.Backends[1].Inline && (BackendList[BackendID-1].Flags & nvmDO_INLINE)==0)
		{
			printf("\tThe specified backend does not accept the inline options, and it will be ignored.\n");
			ConfigParams.Backends[1].Inline=0;
		}

		sprintf(ConfigParams.DumpCodeFilename, "%s_", BackendList[BackendID-1].Name);
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

