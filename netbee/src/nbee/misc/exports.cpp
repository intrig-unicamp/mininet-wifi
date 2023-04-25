/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <stdio.h>

#include <nbsockutils.h>
#include <nbee_initcleanup.h>
#include <nbprotodb.h>
#include "../globals/globals.h"

#include "../decoder/netpdldecoder.h"
#include "initialize.h"
#include "os_utils.h"
#include "../utils/netpdlutils.h"
#include "../packetprocessing/packet_pcapdumpfile.h"

#include <nbee_packetengine.h>
#include <nbee_extractedfieldreader.h>
#include "../nbpacketengine/nbeepacketengine.h"

#include "../globals/profiling.h"


//! Default file containing the NetPDL database, in case no files are given.
#define NETPDL_DEFAULT_FILE "netpdl.xml"




/*!
	\brief Pointer to a global object that keeps the common data for the NetPDL library.

	This object helps to improve speed, since NetPDL files are parsed once when the program
	starts; then, all classes that need to access to the NetPDL file will refer to this
	objects.
*/
struct _nbNetPDLDatabase *NetPDLDatabase;

//! Buffer that will contain the new NetPDL file, in case the user wants to download an updated NetPDL database.
char* NetPDLDataFile;
//! Type of the NetPDL database we loaded. It defines if the database includes visualization primitives or not.
int NetPDLProtoDBFlags;


nbPacketDecoder* nbAllocatePacketDecoder(int Flags, char *ErrBuf, int ErrBufSize)
{
CNetPDLDecoder *NetPDLParser;

	// Check that the user asked to generate the proper PDML (either reduced or with visualization primitives)
	if ( ((Flags & nbDECODER_GENERATEPDML) == 0) && ((Flags & nbDECODER_GENERATEPDML_COMPLETE) == 0))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
			"Error allocating the packet decoder: the PDML generation (either simple or with visualization " \
			"primitives) has to be turned on in order to be able to allocate the Packet Decoder.");

		return NULL;
	}

	NetPDLParser= new CNetPDLDecoder(Flags);

	if (NetPDLParser == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Not enough memory to allocate the Packet Decoder.");

		return NULL;
	}

	if (NetPDLParser->Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize, 
			"%s", NetPDLParser->GetLastError() );

		delete NetPDLParser;
		return NULL;
	}
	else
		return (nbPacketDecoder *) NetPDLParser;
}

void nbDeallocatePacketDecoder(nbPacketDecoder *PacketDecoder)
{
	delete PacketDecoder;
}


nbPacketDecoderLookupTables *nbAllocatePacketDecoderLookupTables(char *ErrBuf, int ErrBufSize)
{
CNetPDLLookupTables *NetPDLLookupTables;

	NetPDLLookupTables= new CNetPDLLookupTables(ErrBuf, ErrBufSize);

	if (NetPDLLookupTables == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Not enough memory to allocate the Packet Decoder.");

		return NULL;
	}

	return (nbPacketDecoderLookupTables *) NetPDLLookupTables;
}

void nbDeallocatePacketDecoderLookupTables(nbPacketDecoderLookupTables *PacketDecoderLookupTables)
{
	delete PacketDecoderLookupTables;
}



nbPSMLReader *nbAllocatePSMLReader(char *FileName, char *ErrBuf, int ErrBufSize)
{
CPSMLReader *PSMLReader;

	PSMLReader= new CPSMLReader();

	if (PSMLReader == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Not enough memory to allocate the PSMLReader object.");

		return NULL;
	}

	if (PSMLReader->Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"%s", PSMLReader->GetLastError() );

		delete PSMLReader;
		return NULL;
	}

	if (PSMLReader->InitializeSource(FileName) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"%s", PSMLReader->GetLastError() );

		delete PSMLReader;
		return NULL;
	}
	else
		return (nbPSMLReader *) PSMLReader;
}


void nbDeallocatePSMLReader(nbPSMLReader* PSMLReader)
{
	delete PSMLReader;
}


nbPDMLReader *nbAllocatePDMLReader(char *FileName, char *ErrBuf, int ErrBufSize)
{
CPDMLReader *PDMLReader;

	PDMLReader= new CPDMLReader();

	if (PDMLReader == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Not enough memory to allocate the PDMLReader object.");

		return NULL;
	}

	if (PDMLReader->Initialize() == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"%s", PDMLReader->GetLastError() );

		delete PDMLReader;
		return NULL;
	}

	if (PDMLReader->InitializeSource(FileName) == nbFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"%s", PDMLReader->GetLastError() );

		delete PDMLReader;
		return NULL;
	}
	else
		return (nbPDMLReader *) PDMLReader;
}


void nbDeallocatePDMLReader(nbPDMLReader* PDMLReader)
{
	delete PDMLReader;
}



nbNetPDLUtils *nbAllocateNetPDLUtils(char *ErrBuf, int ErrBufSize)
{
	nbNetPDLUtils* NetPDLUtils= new CNetPDLDecoderUtils();

	if (NetPDLUtils == NULL)
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
		"Cannot create the nbNetPDLUtils object: not enough memory.");

	return NetPDLUtils;
}


void nbDeallocateNetPDLUtils(nbNetPDLUtils* NetPDLUtils)
{
	delete NetPDLUtils;
}


nbPacketDumpFilePcap* nbAllocatePacketDumpFilePcap(char *ErrBuf, int ErrBufSize)
{
	nbPacketDumpFilePcap* PacketDumpFilePcap= new CPcapPacketDumpFile();

	if (PacketDumpFilePcap == NULL)
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
		"Cannot create the nbPacketDumpFilePcap object: not enough memory.");

	return PacketDumpFilePcap;
}


void nbDeallocatePacketDumpFilePcap(nbPacketDumpFilePcap* PacketDumpFilePcap)
{
	delete PacketDumpFilePcap;
}



struct _nbNetPDLDatabase *nbGetNetPDLDatabase(char *ErrBuf, int ErrBufSize)
{
	if (NetPDLDatabase == NULL)
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
		"Cannot return a valid NetPDL Database. Did you call the nbInitialize() before?");

	return NetPDLDatabase;
}


int nbInitialize(const char *NetPDLFileLocation, int Flags, char *ErrBuf, int ErrBufSize)
{
// Please take care: this control is enabled only in Windows
#ifdef WIN32
const char* WinPcapVerString;
int WinPcapMajor;
int WinPcapMinor;

	WinPcapVerString= pcap_lib_version();
	// The string begins with the format "WinPcap version X.Y"
	sscanf(WinPcapVerString + strlen("WinPcap version"), "%d.%d", &WinPcapMajor, &WinPcapMinor);

	if ((WinPcapMajor < 4) ||
		((WinPcapMajor == 4) && (WinPcapMinor < 1)))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"Missing library: currently WinPcap version %d.%d is installed, while version >= 4.1 is required.",
			WinPcapMajor, WinPcapMinor);

			return nbFAILURE;
	}

#endif

	// Save flags for future references (users are not allowed to change the type of database after initialization)
	NetPDLProtoDBFlags= Flags;

	// Initialize Socket support
	if (sock_init(ErrBuf, ErrBufSize) == sockFAILURE)
		return nbFAILURE;


	if (NetPDLFileLocation != NULL)
	{
		NetPDLDatabase= nbProtoDBXMLLoad(NetPDLFileLocation, Flags, ErrBuf, ErrBufSize);

		if (NetPDLDatabase == NULL)
			return nbFAILURE;
	}
	else
	{
	char NetPDLFileLocation[1024];
	FILE *NetPDLFileHandle;

		if (LocateLibraryLocation(NetPDLFileLocation, sizeof(NetPDLFileLocation), ErrBuf, ErrBufSize) == nbFAILURE)
			return nbFAILURE;

		// Check if the NetPDL file exists. For this, we open and close the file.
		NetPDLFileHandle= fopen(NetPDLFileLocation, "r");

		if (!NetPDLFileHandle)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
				"Cannot find the NetPDL database. Please download the most updated one from http://www.nbee.org/.\n"
				"Please note that the NetPDL database must reside in the same folder of the NetBee shared library.");

				return nbFAILURE;
		}
		fclose(NetPDLFileHandle);

		NetPDLDatabase= nbProtoDBXMLLoad(NetPDLFileLocation, Flags, ErrBuf, ErrBufSize);

		if (NetPDLDatabase == NULL)
			return nbFAILURE;
	}

	// Set-up data related to plugins
	return InitializeNetPDLPlugins(ErrBuf, ErrBufSize);
}



int nbIsInitialized()
{
	if (NetPDLDatabase)
		return nbSUCCESS;
	else
		return nbFAILURE;
}


void nbCleanup()
{
	if (NetPDLDatabase)
	{
		nbProtoDBXMLCleanup();
		NetPDLDatabase= NULL;
	}

	if (NetPDLDataFile)
	{
		free(NetPDLDataFile);
		NetPDLDataFile= NULL;
	}


	// Clean Socket support
	sock_cleanup();

#if defined(_DEBUG) && defined(_MSC_VER) && defined (_CRTDBG_MAP_ALLOC)
	// Dump memory leaks
	_CrtDumpMemoryLeaks();
//	#pragma message( "Memory leaks checking turned on" )
#endif
}


int nbUpdateNetPDLDescription(const char *NetPDLFileLocation, char *ErrBuf, int ErrBufSize)
{
	if (NetPDLDatabase)
	{
		nbProtoDBXMLCleanup();
		NetPDLDatabase= NULL;
	}

	return nbInitialize(NetPDLFileLocation, NetPDLProtoDBFlags, ErrBuf, ErrBufSize);
}


struct _nbVersion* nbGetVersion()
{
static struct _nbVersion nbVersion;

	nbVersion.Major = NETBEE_VERSION_MAJOR;
	nbVersion.Minor= NETBEE_VERSION_MINOR;
	nbVersion.RevCode= NETBEE_VERSION_REVCODE;
	nbVersion.Date= NETBEE_VERSION_DATE;
	nbVersion.SupportedNetPDLMajor= NETPDL_VERSION_MAJOR;
	nbVersion.SupportedNetPDLMinor= NETPDL_VERSION_MINOR;

	if (nbIsInitialized() == nbSUCCESS)
	{
		nbVersion.NetPDLCreator= NetPDLDatabase->Creator;
		nbVersion.NetPDLDate= NetPDLDatabase->CreationDate;
		nbVersion.NetPDLMajor= NetPDLDatabase->VersionMajor;
		nbVersion.NetPDLMinor= NetPDLDatabase->VersionMinor;
	}
	else
	{
		nbVersion.NetPDLCreator= NULL;
		nbVersion.NetPDLDate= NULL;
		nbVersion.NetPDLMajor= 0;
		nbVersion.NetPDLMinor= 0;
	}

	return &(nbVersion);
}



// Commented in the .h file
char nbNetPDLUtils::HexCharToDec(char HexChar)
{
	return ConvertHexCharToDec(HexChar);
}


// Commented in the .h file
int nbNetPDLUtils::HexDumpAsciiToHexDumpBin(char *HexDumpAscii, unsigned char *HexDumpBin, int HexDumpBinSize)
{
	return ConvertHexDumpAsciiToHexDumpBin(HexDumpAscii, HexDumpBin, HexDumpBinSize);
}


// Commented in the .h file
int nbNetPDLUtils::HexDumpBinToHexDumpAscii(char *HexDumpBin, int HexDumpBinSize, int HexDumpIsInNetworkByteOrder, char *HexDumpAscii, int HexDumpAsciiSize)
{
	return ConvertHexDumpBinToHexDumpAscii(HexDumpBin, HexDumpBinSize, (int) HexDumpIsInNetworkByteOrder, HexDumpAscii, HexDumpAsciiSize);
}



int nbRegisterPacketDecoderCallHandle(char* Namespace, char* Function, nbPacketDecoderExternalCallHandler* ExternalHandler, char *ErrBuf, int ErrBufSize)
{
char* TmpPtr;
int ElementFound;

	ElementFound= 0;

	for (unsigned int i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
		switch(NetPDLDatabase->GlobalElementsList[i]->Type)
		{
			case nbNETPDL_IDEL_UPDATELOOKUPTABLE:
			{
			struct _nbNetPDLElementUpdateLookupTable *NetPDLElement;

				NetPDLElement= (struct _nbNetPDLElementUpdateLookupTable *) NetPDLDatabase->GlobalElementsList[i];

				if (NetPDLElement->CallHandlerInfo)
				{
					TmpPtr= NetPDLElement->CallHandlerInfo->FunctionName;

					// Check namespace
					if (strcmp(TmpPtr, Namespace) != 0)
						continue;

					TmpPtr= TmpPtr + strlen(TmpPtr) + 1;

					// Check function name
					if (strcmp(TmpPtr, Function) != 0)
						continue;

					NetPDLElement->CallHandlerInfo->CallHandler= (void*) ExternalHandler;
					ElementFound= 1;
				}

			}; break;
		}
	}

	if (ElementFound)
		return nbSUCCESS;
	else
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"The registered function '%s:%s' does not match any element inside the NetPDL database.", Namespace, Function);

		return nbFAILURE;
	}
}


nbPacketEngine *nbAllocatePacketEngine(bool UseJit, char *ErrBuf, int ErrBufSize)
{
	if (NetPDLDatabase == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
			"The NetPDL database is not valid.");
		return NULL;
	}

	nbeePacketEngine* PacketEngine= new nbeePacketEngine(NetPDLDatabase, UseJit);

	return (nbPacketEngine*) PacketEngine;
}


void nbDeallocatePacketEngine(nbPacketEngine *PacketEngine)
{
	if (PacketEngine)
		delete PacketEngine;
}


nbProfiler *nbAllocateProfiler(char *ErrBuf, int ErrBufSize)
{
	nbProfiler* Profiler= new CProfilingExecTime();

	if (Profiler == NULL)
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ErrBuf, ErrBufSize,
		"Cannot create the nbProfiler object: not enough memory.");

	return Profiler;
}


void nbDeallocateProfiler(nbProfiler* Profiler)
{
	delete Profiler;
}

