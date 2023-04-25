/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <string.h>
#include <stdlib.h>
#include <nbpacketprocess.h>
#include <nbsockutils.h>
#ifdef WIN32
#include <Packet32.h>
#endif
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"


nbPacketProcessBase::nbPacketProcessBase()
{
	m_returnedDeviceList= NULL;
	memset(m_errbuf, 0, sizeof(m_errbuf));
};


nbPacketProcessBase::~nbPacketProcessBase()
{
	DELETE_PTR(m_returnedDeviceList);
};


nbNetVmPortRemoteAdapter *nbPacketProcessBase::GetListRemoteAdapters()
{
	return NULL;
}


nbNetVmPortLocalAdapter *nbPacketProcessBase::GetListLocalAdapters()
{
	char		*Name;
	const char	*Description;
	char		*TempBuffer;
	unsigned long NameLength;
	nbNetVmPortLocalAdapter *CurrentDevice, **PtrToPreviousDevice;

	// In case we already returned the interface list previously, let's delete the old one
	DELETE_PTR(m_returnedDeviceList);
		
	// This is needed for getting the space needed to contain the adapter names
	PacketGetAdapterNames(NULL, &NameLength);

	if (NameLength <= 0)
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__, NULL, m_errbuf, sizeof(m_errbuf));
		return NULL;
	}

	TempBuffer = new char [NameLength];
	if (TempBuffer == NULL)
	{
		ssnprintf(m_errbuf, sizeof(m_errbuf), "Cannot allocate enough memory to list the adapters.");
		return NULL;
	}			

	if (!PacketGetAdapterNames( (PTSTR) TempBuffer, &NameLength))
	{
		nbGetLastErrorEx(__FILE__, __FUNCTION__, __LINE__, NULL, m_errbuf, sizeof(m_errbuf));
		free(TempBuffer);
		return NULL;
	}
	
	// "PacketGetAdapterNames()" returned a list of null-terminated ASCII interface name strings,
	// terminated by a null string, followed by a list of null-terminated ASCII interface description
	// strings, terminated by a null string.
	// This means there are two ASCII nulls at the end of the first list.
	//
	// Find the end of the first list; that's the beginning of the second list.
	Description = &TempBuffer[0];
	while ( (*Description != '\0') || (*(Description + 1) != '\0') )
		Description++;
	
 	// Found it - "desc" points to the first of the two nulls at the end of the list of names, so the
	// first byte of the list of descriptions is two bytes after it.
	Description += 2;
	
	// Loop over the elements in the first list.
	Name = &TempBuffer[0];

	PtrToPreviousDevice= (nbNetVmPortLocalAdapter **) &m_returnedDeviceList;

	while (*Name != '\0')
	{
	npf_if_addr AddressList[MAX_NETWORK_ADDRESSES];
	long NumberOfAddrs;

		CurrentDevice = new nbNetVmPortLocalAdapter;
		if (CurrentDevice == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "Cannot allocate enough memory to list the adapters.");
			return NULL;
		}

		*PtrToPreviousDevice= CurrentDevice;

		if (CurrentDevice->Initialize(Name, (char *) Description, 0) == nbFAILURE)
			return NULL;

		NumberOfAddrs= MAX_NETWORK_ADDRESSES;

		// Add an entry for this interface.

		// Get the list of addresses for the interface.
		if (PacketGetNetInfoEx(Name, AddressList, &NumberOfAddrs) == 0)
		{
			// Failure. However, we don't return an error, because this can happen with
			// NdisWan interfaces, and we want to list them even if we can't supply their addresses.
			// So, we return an entry with an empty address list.
			Name += strlen(Name) + 1;
			Description += strlen(Description) + 1;
			continue;
		}

		// Now add the addresses.
		nbNetAddress **PtrToPreviousAddress= &(CurrentDevice->AddressList);
		while (NumberOfAddrs-- > 0)
		{
			// Allocate the new entry and fill it in.
			(*PtrToPreviousAddress)= new (nbNetAddress);
			if ( (*PtrToPreviousAddress == NULL))
			{
				ssnprintf(m_errbuf, sizeof(m_errbuf), "Cannot allocate enough memory to list the adapters.");
				return NULL;
			}

			(*PtrToPreviousAddress)->AddressFamily= AddressList[NumberOfAddrs].IPAddress.ss_family;

			// Allocate the memory needed to keep address and netmask (if needed)
			(*PtrToPreviousAddress)->Address= new char [128];	// 128 bytes should be enough
			if ( (*PtrToPreviousAddress)->Address == NULL)
			{
				ssnprintf(m_errbuf, sizeof(m_errbuf), "Cannot allocate enough memory to list the adapters.");
				return NULL;
			}

			if ((*PtrToPreviousAddress)->AddressFamily == AF_INET)
			{
				(*PtrToPreviousAddress)->Netmask= new char [128];	// 128 bytes should be enough
				if ( (*PtrToPreviousAddress)->Netmask == NULL)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), "Cannot allocate enough memory to list the adapters.");
					return NULL;
				}
			}

			// Now, let's copy the requested info in the previously allocated structures
			if ((*PtrToPreviousAddress)->AddressFamily == AF_INET)
			{
				if (getnameinfo((struct sockaddr *) &AddressList[NumberOfAddrs].IPAddress, sizeof(struct sockaddr_in), 
								(*PtrToPreviousAddress)->Address, 128, NULL, NULL, NI_NUMERICHOST) != 0)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), "Error when getting the printable form of a network address.");
					return NULL;
				}

				if (getnameinfo((struct sockaddr *) &AddressList[NumberOfAddrs].SubnetMask, sizeof(struct sockaddr_in), 
								(*PtrToPreviousAddress)->Netmask, 128, NULL, NULL, NI_NUMERICHOST) != 0)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), "Error when getting the printable form of a network address.");
					return NULL;
				}
			}
			else
			{
				if (getnameinfo((struct sockaddr *) &AddressList[NumberOfAddrs].IPAddress, sizeof(struct sockaddr_in6), 
								(*PtrToPreviousAddress)->Address, 128, NULL, NULL, NI_NUMERICHOST) != 0)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), "Error when getting the printable form of a network address.");
					return NULL;
				}
				//! \todo We have to copy the IPv6 prefix length: how to get it?
			}

			// Point to the next location to be filled in, in case a new address is present
			PtrToPreviousAddress= &((*PtrToPreviousAddress)->Next);
		}

		Name += strlen(Name) + 1;
		Description += strlen(Description) + 1;

		// Saves a pointer to the previous device
		PtrToPreviousDevice= (nbNetVmPortLocalAdapter **) &(CurrentDevice->Next);
	}
		
	free(TempBuffer);
	return (nbNetVmPortLocalAdapter *) m_returnedDeviceList;
};

