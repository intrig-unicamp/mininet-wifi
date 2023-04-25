/*
 * Copyright (c) 2002 - 2007
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

#include <nbee.h>
//#include <nbsockutils.h>	// For af:inet and such


int main()
{
nbPacketCapture PacketCapture;
int IfNumber;

	nbNetVmPortLocalAdapter *DeviceList= PacketCapture.GetListLocalAdapters();
	if (DeviceList == NULL)
	{
		printf("Error getting the devices installed in the system: %s\n", PacketCapture.GetLastError() );
		return nbFAILURE;
	}

	printf("Printing the details of the adapters:\n");
	printf("=====================================\n\n");

	IfNumber= 0;

	while (DeviceList)
	{
		printf("Interface number %d:\n", IfNumber);

		printf("\tName: %s\n", DeviceList->Name);
		printf("\tDescription: %s\n", DeviceList->Description);

		nbNetAddress *NetAddress= DeviceList->AddressList;
		while (NetAddress)
		{
			printf("\t\tAddress family: %s\n", (NetAddress->AddressFamily == AF_INET) ? "IPv4" : "IPv6");
			printf("\t\tNetwork address: %s\n", NetAddress->Address);

			if (NetAddress->AddressFamily == AF_INET)
				printf("\t\tNetwork address: %s\n", NetAddress->Netmask);
			else
				printf("\t\tPrefix length: %d\n", NetAddress->PrefixLen);

			NetAddress= NetAddress->Next;
			printf("\n");
		}

		DeviceList= DeviceList->GetNext();
		IfNumber++;
	}

	return nbSUCCESS;
}



