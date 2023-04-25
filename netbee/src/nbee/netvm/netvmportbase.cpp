/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <nbee_netvm.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"

#ifndef WIN32
#include <string.h>
#endif


nbNetVmPortBase::nbNetVmPortBase()
{
	//m_OwnedExchangeBuffer= NULL;
	memset(m_errbuf, 0, sizeof(m_errbuf));
	//memset(&m_PortStruct, 0, sizeof (m_PortStruct));
};


nbNetVmPortDataDevice::nbNetVmPortDataDevice()
{
	Name= NULL;
	Description= NULL;
	Next= NULL;
}


nbNetVmPortDataDevice::~nbNetVmPortDataDevice()
{
	DELETE_PTR(Name);
	DELETE_PTR(Description);
	DELETE_PTR(Next);
}


int nbNetVmPortDataDevice::Initialize(char *Name, char *Description)
{
	// Allocate space for the new members
	if (Name) 
	{	
		this->Name= new char [strlen(Name) + 1];
		if (this->Name == NULL) goto AllocMemFailed;
		strcpy(this->Name, Name);
	}
	if (Description)
	{	
		this->Description= new char [strlen(Description) + 1];
		if (this->Description == NULL) goto AllocMemFailed;
		strcpy(this->Description, Description);
	}
	return nbSUCCESS;

AllocMemFailed:
	ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
	return nbFAILURE;
}


int nbNetVmPortDataDevice::RegisterCallbackFunction(nbDataDstCallbackFunction *DataDstCallbackFunction, void *CustomPtr)
{
	// Register the callback function on that port
	if (nvmRegisterCallbackFunction((nvmCallbackFunction*)DataDstCallbackFunction, (void *)CustomPtr, this->GetNetVmPortDataDevice()) == nvmFAILURE)
	{
		printf("Cannot register the callback function\n");
		return nbFAILURE;
	}	
	return nbSUCCESS;
}


int nbNetVmPortDataDevice::RegisterPollingFunction(nbDataSrcPollingFunction *DataSrcPollingFunction, void *CustomPtr)
{
	// Register the function that has to be called for getting data
	if (nvmRegisterPollingFunction((nvmPollingFunction*)DataSrcPollingFunction, (void*)CustomPtr, this->GetNetVmPortDataDevice()) == nvmFAILURE)
	{
		printf("Cannot register the polling function on the data source\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}	


int nbNetVmPortDataDevice::ReleaseExBuf(nbNetPkt *Packet)
{
	nvmReleaseExBuf(this->GetNetVmPortDataDevice(),Packet);
	return nbSUCCESS;
}


nbNetAddress::nbNetAddress()
{
	AddressFamily= 0;
	Address= NULL;
	Netmask= NULL;
	PrefixLen= 0;
	Next= NULL;
	memset(m_errbuf, 0, sizeof(m_errbuf));
}


nbNetAddress::~nbNetAddress()
{
	DELETE_PTR(Address);
	DELETE_PTR(Netmask);
	DELETE_PTR(Next);
}


nbNetAddress *nbNetAddress::Clone()
{
	nbNetAddress * NewNetAddress;
	NewNetAddress = new nbNetAddress;

	if (NewNetAddress == NULL)
	{
		ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
		return NULL;
	}

	// Allocate space for the new members
	if (Address)
	{
		NewNetAddress->Address= new char [strlen(Address) + 1];
		if (NewNetAddress->Address == NULL) goto AllocMemFailed;
		strcpy(NewNetAddress->Address, Address);
	}
	if (Netmask)
	{
		NewNetAddress->Netmask= new char [strlen(Netmask) + 1];
		if (NewNetAddress->Netmask == NULL) goto AllocMemFailed;
		strcpy(NewNetAddress->Netmask, Netmask);
	}

	// Copy the remaining info present in this class in the new object
	NewNetAddress->AddressFamily= AddressFamily;
	NewNetAddress->PrefixLen= PrefixLen;
	strcpy(NewNetAddress->m_errbuf, m_errbuf);

	if (Next)
	{
		NewNetAddress->Next= Next->Clone();
		if (NewNetAddress->Next == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "nbNetAddress: %s", Next->GetLastError() );
			delete NewNetAddress;
			return NULL;
		}
	}

	return NewNetAddress;

AllocMemFailed:
	delete NewNetAddress;
	ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
	return NULL;
}