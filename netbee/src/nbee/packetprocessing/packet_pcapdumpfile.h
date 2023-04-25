/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "packetdumpfile.h"
#include <nbee_packetdumpfiles.h>



/*!
	\brief This class implements some basic features that are requred to manage dump files, with indexing enabled.

	Basically, this class helps managing WinPcap/libpcap dump files, and it allows creating an index for the
	packets. This enables random access to packets.
*/
class CPcapPacketDumpFile : public CPacketDumpFile, public nbPacketDumpFilePcap
{
public:
	CPcapPacketDumpFile();
	virtual ~CPcapPacketDumpFile();

	int OpenDumpFile(const char* FileName, bool CreateIndexing= false);
	int CreateDumpFile(const char* FileName, int LinkLayerType, bool CreateIndexing= false);
	int CloseDumpFile();
	int GetLinkLayerType(nbNetPDLLinkLayer_t &LinkLayerType);

	int AppendPacket(const struct pcap_pkthdr* PktHeader, const unsigned char* PktData, bool FlushData= false);
	int GetPacket(unsigned long PacketNumber, struct pcap_pkthdr** PktHeader, const unsigned char** PktData);
	int GetNextPacket(struct pcap_pkthdr** PktHeader, const unsigned char** PktData);
	int RemovePacket(unsigned long PacketNumber);

	//! Return the error messages (if any)
	char *GetLastError() { return m_errbuf; };

private:

	int m_isFileNew;
	int m_createIndexing;

	pcap_t *m_pcapHandle;
	pcap_dumper_t *m_pcapDumpFileHandle;

	//! Contains the packet data, as read from the file
	unsigned char m_packetBuffer[10000];
};

