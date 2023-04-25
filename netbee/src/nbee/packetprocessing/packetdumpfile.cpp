/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "../globals/debug.h"
#include <stdio.h>
#include <string.h>
#include "packetdumpfile.h"





//! Default constructor.
CPacketDumpFile::CPacketDumpFile()
{
	m_packetList= NULL;

	m_currNumPackets=0;
	m_maxNumPackets= 0;

	memset(m_errbuf, 0, sizeof(m_errbuf));
}


//! Default destructor
CPacketDumpFile::~CPacketDumpFile()
{
	DeleteIndex();
}


/*!
	\brief Initializes the variables contained into this class.

	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPacketDumpFile::InitializeIndex()
{
unsigned long i;

	m_maxNumPackets= PXML_MINIMUM_LIST_SIZE;
	m_currNumPackets= 0;

	m_packetList= new struct _PacketList [m_maxNumPackets];
	if (m_packetList == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the new packetlist buffer.");
		return nbFAILURE;
	}

	for (i= m_currNumPackets; i < m_maxNumPackets; i++)
	{
		m_packetList[i].StartingOffset= 0;
		m_packetList[i].EndingOffset= 0;
	}

	return nbSUCCESS;
}


void CPacketDumpFile::DeleteIndex()
{
	if (m_packetList)
	{
		delete m_packetList;
		m_packetList= NULL;
	}

	m_maxNumPackets= 0;
	m_currNumPackets=0;
}


int CPacketDumpFile::CreateNewPositionInIndex(unsigned long StartingOffset)
{
	if (StartingOffset <= 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Invalid starting offset in the pcap file.\n");
		return nbFAILURE;
	}

	// Check if the vector is enough; if not, let's allocate a new one, and let's hope it is enough
	if (CheckPacketListSize() == nbFAILURE)
		return nbFAILURE;

	m_currNumPackets++;

	m_packetList[m_currNumPackets-1].StartingOffset= StartingOffset;

	return nbSUCCESS;
}


int CPacketDumpFile::UpdateNewPositionInIndex(unsigned long EndingOffset)
{
	if (EndingOffset <= 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Invalid ending offset in the pcap file.\n");
		return nbFAILURE;
	}

	m_packetList[m_currNumPackets-1].EndingOffset= EndingOffset;

	return nbSUCCESS;
}


/*!
	\brief Checks the number of packets in 'm_packetList'; it enlarges this vector if needed.

	\return nbSUCCESS if everything is fine, nbFAILURE in case of error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CPacketDumpFile::CheckPacketListSize()
{
	// The vector is not enough; Please allocate a new one, and let's hope it is enough
	if (m_currNumPackets == m_maxNumPackets)
	{
	struct _PacketList *NewVector;
	unsigned long i;
	unsigned long NewSize;

		NewSize= m_maxNumPackets * 10;

		NewVector= new struct _PacketList [NewSize];
		if (NewVector == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Not enough memory to allocate the new packetlist buffer.");
			return nbFAILURE;
		}

		for (i= 0; i < m_currNumPackets; i++)
		{
			NewVector[i].StartingOffset= m_packetList[i].StartingOffset;
			NewVector[i].EndingOffset= m_packetList[i].EndingOffset;
		}

		for (i= m_currNumPackets; i < m_maxNumPackets; i++)
		{
			NewVector[i].StartingOffset= 0;
			NewVector[i].EndingOffset= 0;
		}

		delete m_packetList;

		m_maxNumPackets= NewSize;
		m_packetList= NewVector;
	}

	return nbSUCCESS;
}


int CPacketDumpFile::RemovePacket(unsigned long PacketNumber)
{
	if (PacketNumber > m_currNumPackets)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Requested a packet that is out of range.\n");
		return nbFAILURE;
	}

	for (unsigned long i= (PacketNumber - 1); i < (m_currNumPackets - 1); i++)
	{
		m_packetList[i].StartingOffset = m_packetList[i+1].StartingOffset;
		m_packetList[i].EndingOffset = m_packetList[i+1].EndingOffset;
	}

	m_currNumPackets--;

	return nbSUCCESS;
}

