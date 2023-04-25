/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include <nbee_packetdecoder.h>

#include "../globals/globals.h"


//! By default, we are able to keep track of 10000 packet stored on disk, then we have to enlarge the buffer
#define PXML_MINIMUM_LIST_SIZE 10000




/*!
	\brief This class implements some basic features that are required by CPSMLReader and CPDMLReader classes.

	Basically, this class helps in some functions (like dumping the XML document on file)
	that are common throughout both classes.
*/
class CPacketDumpFile
{
public:
	CPacketDumpFile();
	virtual ~CPacketDumpFile();

protected:
	int InitializeIndex();
	void DeleteIndex();
	int CheckPacketListSize();

	int UpdateNewPositionInIndex(unsigned long EndingOffset);
	int CreateNewPositionInIndex(unsigned long StartingOffset);
	int RemovePacket(unsigned long PacketNumber);

struct _PacketList
{
	unsigned long StartingOffset;
	unsigned long EndingOffset;
};

	//! Current size of the array that keeps the offsets of the packet on the file
	unsigned long m_maxNumPackets;

	//! Keeps how many elements we have currently in the array
	unsigned long m_currNumPackets;

	//! Buffer that keeps the error message (if any)
	char m_errbuf[2048];

	struct _PacketList* m_packetList;
};

