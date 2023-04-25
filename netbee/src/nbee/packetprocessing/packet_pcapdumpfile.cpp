/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <pcap.h>
#include "packet_pcapdumpfile.h"
#include "../globals/utils.h"
#include "../globals/debug.h"
#include "../globals/globals.h"



//! Default constructor.
CPcapPacketDumpFile::CPcapPacketDumpFile()
{
	m_createIndexing= 0;
	m_isFileNew= 0;

	m_pcapHandle= NULL;
	m_pcapDumpFileHandle= NULL;

	memset(m_errbuf, 0, sizeof(m_errbuf));
}


//! Default destructor
CPcapPacketDumpFile::~CPcapPacketDumpFile()
{
	if (m_pcapHandle)
		// Using function in savefile.c instead of the one provided by the WinPcap library
//		sf_close(m_pcapHandle);
		pcap_close(m_pcapHandle);

	if (m_pcapDumpFileHandle)
		pcap_dump_close(m_pcapDumpFileHandle);
}


int CPcapPacketDumpFile::OpenDumpFile(const char* FileName, bool CreateIndexing)
{
	m_createIndexing= CreateIndexing;
	m_isFileNew= 0;

	// Open the pcap file
	if ((m_pcapHandle= pcap_open_offline(FileName, m_errbuf)) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot open the capture source file.\n");
		return nbFAILURE;
	}

	// Scan the entire file in order to create the index
	if (m_createIndexing)
	{
	struct pcap_pkthdr* PktHeader;
	unsigned char* PktData;
	int RetVal;

		if (CPacketDumpFile::InitializeIndex() == nbFAILURE)
			return nbFAILURE;

		// Save the pcap_dumper_t handle
		// This is really dirty, but it seems to work.
		// Although the pcap_file() is not guaranted to work on Windows, in our casw we're returning a file pointer that
		// we're using again inside the library, through the pcap_dump_ftell(), So, this should work anyway.
		pcap_dumper_t* TempPcapDumpFileHandle= (pcap_dumper_t*) pcap_file(m_pcapHandle);

		while (1)
		{
		long StartinOffset, EndingOffset;

			if ((StartinOffset= pcap_dump_ftell(TempPcapDumpFileHandle)) == -1)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error reading position from dump file.\n");
				return nbFAILURE;
			}

			RetVal= pcap_next_ex(m_pcapHandle, &PktHeader, (const unsigned char **) &PktData);

			// RetVal == 0 (timeout) is not necessary here (it refers to a live capture)
			if (RetVal == -1)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s\n", pcap_geterr(m_pcapHandle));
				return nbFAILURE;
			}

			if (RetVal == -2) // end of file
				break;

			if ((EndingOffset= pcap_dump_ftell(TempPcapDumpFileHandle)) == -1)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error reading position from dump file.\n");
				return nbFAILURE;
			}

			if (CreateNewPositionInIndex(StartinOffset) == nbFAILURE)
				return nbFAILURE;

			if (UpdateNewPositionInIndex(EndingOffset) == nbFAILURE)
				return nbFAILURE;
		}

		// Now, we have to re-initialize the file, in order to move all the pointers to the beginning
		pcap_close(m_pcapHandle);

		// Re-open the pcap file
		if ((m_pcapHandle= pcap_open_offline(FileName, m_errbuf)) == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot open the capture source file.\n");
			return nbFAILURE;
		}

	}

	return nbSUCCESS;
}


int CPcapPacketDumpFile::CreateDumpFile(const char* FileName, int LinkLayerType, bool CreateIndexing)
{
	m_createIndexing= CreateIndexing;
	m_isFileNew= 1;

	// if the file exists, we can use the standard pcap_open_offline() function.
	// However, this is another case: we want to create a *new* dump file from scratch.
	// In this case we have to use other functions, i.e. a pcap_open_dead() to open a fake pcap_t, 
	// and then a pcap_dump_open to open a file.

	// Create the pcap handle
	if ((m_pcapHandle= pcap_open_dead(LinkLayerType, 999999 /* snaplen */)) == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot create a pcap handle.\n");
		return nbFAILURE;
	}

	// Open the dump file
	m_pcapDumpFileHandle = pcap_dump_open(m_pcapHandle, FileName);
	if (m_pcapDumpFileHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "\nError opening dump file: %s\n", pcap_geterr(m_pcapHandle));
		return nbFAILURE;
	}

	if (CreateIndexing)
		return (CPacketDumpFile::InitializeIndex());
	else
		return nbSUCCESS;
}


int CPcapPacketDumpFile::CloseDumpFile()
{
	if (m_pcapDumpFileHandle)
	{
		pcap_dump_close(m_pcapDumpFileHandle);
		m_pcapDumpFileHandle= NULL;
	}

	if (m_pcapHandle)
	{
//		// Using function in savefile.c instead of the one provided by the WinPcap library
//		sf_close(m_pcapHandle);
		pcap_close(m_pcapHandle);
		m_pcapHandle= NULL;
	}

	if (m_createIndexing)
		CPacketDumpFile::DeleteIndex();

	return nbSUCCESS;
}


int CPcapPacketDumpFile::AppendPacket(const struct pcap_pkthdr* PktHeader, const unsigned char* PktData, bool FlushData)
{
	if (m_isFileNew == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot append a packet to an existing file.\n");
		return nbFAILURE;
	}

	if (m_pcapDumpFileHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file must be created first.\n");
		return nbFAILURE;
	}

	if (m_createIndexing)
	{
		if (CreateNewPositionInIndex( pcap_dump_ftell(m_pcapDumpFileHandle) ) == nbFAILURE)
			return nbFAILURE;
	}

	pcap_dump((unsigned char *)m_pcapDumpFileHandle, PktHeader, PktData);

	if (FlushData)
	{
		if (pcap_dump_flush(m_pcapDumpFileHandle) == -1)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Error flushing data to file: %s.\n", pcap_geterr(m_pcapHandle));
			return nbFAILURE;
		}
	}

	if (m_createIndexing)
	{
		if (UpdateNewPositionInIndex( pcap_dump_ftell(m_pcapDumpFileHandle) ) == nbFAILURE)
			return nbFAILURE;
	}

	return nbSUCCESS;
}


// Take care: nbWARNING for EOF
int CPcapPacketDumpFile::GetPacket(unsigned long PacketNumber, struct pcap_pkthdr** PktHeader, const unsigned char** PktData)
{
	if (m_isFileNew == 1)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot read packets from a file opened in writing mode.\n");
		return nbFAILURE;
	}

	if (m_pcapHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file must be opened first.\n");
		return nbFAILURE;
	}

	if (m_createIndexing == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file must be indexed in order to use this function.\n");
		return nbFAILURE;
	}

	if (PacketNumber > m_currNumPackets)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Requested a packet that is out of range.\n");
		return nbWARNING;
	}

#ifdef WIN32
	// FULVIO 19/05/2008 Warning: currently we're missing a pcap_dump_seek in WinPcap. Asked Gianluca to put this patch in.
	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Currently the GetPacket is not supported to du a bug in WinPcap.\n");
	return nbFAILURE;
#else
	fseek(pcap_file(m_pcapHandle), m_packetList[PacketNumber-1].StartingOffset, SEEK_SET);
#endif

	if (pcap_next_ex(m_pcapHandle, PktHeader, PktData) == -1)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s\n", pcap_geterr(m_pcapHandle));
		return nbFAILURE;
	}

//	*PktData= m_packetBuffer;

	return nbSUCCESS;

}


int CPcapPacketDumpFile::GetNextPacket(struct pcap_pkthdr** PktHeader, const unsigned char** PktData)
{
int RetVal;

	if (m_isFileNew == 1)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot read packets from a file opened in writing mode.\n");
		return nbFAILURE;
	}

	if (m_pcapHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file must be opened first.\n");
		return nbFAILURE;
	}

	RetVal= pcap_next_ex(m_pcapHandle, PktHeader, PktData);

	if (RetVal == -1)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "%s\n", pcap_geterr(m_pcapHandle));
		return nbFAILURE;
	}

	if (RetVal == -2)
		return nbWARNING;

//	*PktData= m_packetBuffer;

	return nbSUCCESS;
}


int CPcapPacketDumpFile::RemovePacket(unsigned long PacketNumber)
{
	if (m_isFileNew == 1)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot delete packets from a file opened in writing mode.\n");
		return nbFAILURE;
	}

	if (m_pcapHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file must be opened first.\n");
		return nbFAILURE;
	}

	if (m_createIndexing == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "The file must be indexed in order to use this function.\n");
		return nbFAILURE;
	}

	return CPacketDumpFile::RemovePacket(PacketNumber);
}


int CPcapPacketDumpFile::GetLinkLayerType(nbNetPDLLinkLayer_t &LinkLayerType)
{
	if (m_pcapHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "A file must be opened first.\n");
		return nbFAILURE;
	}

	switch (pcap_datalink(m_pcapHandle))
	{
		case 1:
			LinkLayerType= nbNETPDL_LINK_LAYER_ETHERNET;
			break;

		case 6:
			LinkLayerType= nbNETPDL_LINK_LAYER_TOKEN_RING;
			break;

		case 10:
			LinkLayerType= nbNETPDL_LINK_LAYER_FDDI;
			break;

		case 13:
			LinkLayerType= nbNETPDL_LINK_LAYER_HCI;
			break;

		case 105:
			LinkLayerType= nbNETPDL_LINK_LAYER_IEEE_80211;
			break;

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf),
				"The current dump file has a link-layer type that is not supported by this library.\n");

			return nbFAILURE;
		}
	}

	return nbSUCCESS;
}

