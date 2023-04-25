/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <string.h>
#include <nbee_netvm.h>
#include <pcap.h>			
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"



nbNetVmPortLocalAdapter::nbNetVmPortLocalAdapter()
{
	// Create the port that implements the collector
	ExternalPort = nvmCreateExternalPort();
	if (ExternalPort == NULL)
	{
		printf("Cannot create the port\n");
	}
	Flags= 0;
	AddressList= NULL;
}


nbNetVmPortLocalAdapter::~nbNetVmPortLocalAdapter()
{
	DELETE_PTR(AddressList);
}


int nbNetVmPortLocalAdapter::Initialize(char *Name, char *Description, int Flags)
{
	this->Flags= Flags;
	
	if(nbNetVmPortDataDevice::Initialize(Name, Description)==nbFAILURE)
		return nbFAILURE;
	
	if(this->OpenAsDataSrc()==nbFAILURE)
		{
			printf("Cannot initialize winpcap\n");
			return nbFAILURE;
		}

	return nbSUCCESS;
}


nbNetVmPortDataDevice *nbNetVmPortLocalAdapter::Clone(char *Name, char *Description, int Flags)
{
	nbNetVmPortLocalAdapter * LocalAdapter;
	LocalAdapter = new nbNetVmPortLocalAdapter;

	if (LocalAdapter == NULL)
	{
		ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
		return NULL;
	}

	if (LocalAdapter->Initialize(Name, Description, Flags) == nbFAILURE)
		return NULL;

	// Copy the remaining info present in this class in the new object
	strcpy(LocalAdapter->m_errbuf, m_errbuf);

	// Copy embedded objects, in case these are present
	if (AddressList) 
	{
		LocalAdapter->AddressList= AddressList->Clone();
		if (LocalAdapter->AddressList == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), __FUNCTION__ " %s", AddressList->GetLastError() );
			delete LocalAdapter;
			return NULL;
		}
	}

	if (Next)
	{
		LocalAdapter->Next= ((nbNetVmPortLocalAdapter*)Next)->Clone(Name, Description, Flags);
		if (LocalAdapter->Next == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), __FUNCTION__ " %s", Next->GetLastError() );
			delete LocalAdapter;
			return NULL;
		}
	}

	return LocalAdapter;
};

int nbNetVmPortLocalAdapter::OpenAsDataSrc()
{
	int IsPromiscuous;
	char errbuf[PCAP_ERRBUF_SIZE];

	if (nbFLAG_ISSET(Flags, nbNETDEVICE_PROMISCUOUS))
		IsPromiscuous = PCAP_OPENFLAG_PROMISCUOUS;
	else
		IsPromiscuous = 0;

	m_WinPcapDevice = pcap_open(Name,65535,IsPromiscuous,1000,NULL,errbuf);
 
	if (m_WinPcapDevice == NULL)
		
			return nbFAILURE;
		
	return nbSUCCESS;
}


int nbNetVmPortLocalAdapter::OpenAsDataDst()
{
	return nbFAILURE;
}

int nbNetVmPortLocalAdapter::Close()
{
	pcap_close((pcap_t*) m_WinPcapDevice);
	return nbSUCCESS;
}

u_int32_t nbNetVmPortLocalAdapter::SrcPollingFunctionWinPcap(void * ptr, nvmExchangeBufferState *buffer, int *dummy)
{
	struct pcap_pkthdr *header;
	u_char *pkt_data;
	int res;

	nbNetVmPortLocalAdapter *ClassHandler = (nbNetVmPortLocalAdapter *) ptr;
	
	NETVM_ASSERT(ClassHandler->m_WinPcapDevice != NULL, __FUNCTION__ "  - WinPcapDevice not opened, this is weird");

	do
	{
		res = pcap_next_ex( (pcap_t*)ClassHandler->m_WinPcapDevice, &header, (const u_char**)&pkt_data);

		NETVM_ASSERT(res >= 0, __FUNCTION__ "  - I don't know what to do, the next_ex failed... GV");
	    
		if (res == 1)
		{
			buffer->timestamp_sec = header->ts.tv_sec;
			buffer->timestamp_usec = header->ts.tv_usec;
			buffer->pkt_pointer=pkt_data;
			buffer->pktlen=header->len;
			buffer->infolen=0;
			return nbSUCCESS;
		}
	}
	while (res == 0);
	return nbFAILURE;
}

int nbNetVmPortLocalAdapter::Write(nbNetPkt *PacketData)
{
	nvmWriteExchangeBuffer(this->GetNetVmPortDataDevice(), PacketData);
	return nbSUCCESS;
}