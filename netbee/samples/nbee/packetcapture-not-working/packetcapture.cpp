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
#include <string.h>
#include <nbee.h>


u_int32_t ReadPacketsCallback(void* voidDummy, nvmExchangeBufferState *exbuffer, int* intDummy)
{
	if(exbuffer!=NULL)
		{
			unsigned char *b = (unsigned char*)exbuffer -> pkt_pointer;
			printf("Bytes 12-14: %d %d %d - Length: %d\n", b[12], b[13], b[14], exbuffer->pktlen);
		}	
	return nbSUCCESS;
}



void Usage()
{
char string[]= \
	"\nUsage:\n"	\
	"  packetcapture [NetPDLFile]\n\n"	\
	"Options:\n"	\
	" - NetPDLFile: name (and *path*) of the file containing the NetPDL\n"				\
	"     description. In case it is omitted, the NetPDL description embedded\n"		\
	"     within the NetBee library will be used.\n\n"									\
	"Description\n"																		\
	"============================================================================\n"	\
	"This program can be used to capture packets from a network adapter and print\n"	\
	"  them on screen. Packets can be filtered.\n";	
	"This program can either receive packets through a ReadPacket() loop, or\n"			\
	"  receive them through a callback function. The program will prompt the user.\n"	\
	"  for this choice during its execution.\n\n";

	printf("%s", string);
}


int main(int argc, char *argv[])
{
nbPacketCapture *PacketCapture;
nbNetVmPortLocalAdapter *DeviceList, *TargetDestDevice;
int IfNumber;
int IfChosen;
char ReadMethod;
bool ReceiveFromCallback;
char FilterString[2048];
char ErrBuf[2048];

	Usage();

	printf("\n...Loading NetPDL protocol database.\n");
	if (argc == 2)
	{
	int Res;

		Res= nbInitialize(argv[1], ErrBuf, sizeof(ErrBuf) );

		if (Res == nbFAILURE)
		{
			printf("Error initializing the NetBee Library; %s\n", ErrBuf);
			printf("\n\nUsing the NetPDL database embedded in the NetBee library instead.\n");
		}
	}

	// In case the NetBee library has not been initialized, initialize right now with the embedded NetPDL protocol database instead
	if (nbIsInitialized() == nbFAILURE)
	{
		if (nbInitialize(NULL, ErrBuf, sizeof(ErrBuf)) == nbFAILURE)
		{
			printf("Error initializing the NetBee Library; %s\n", ErrBuf);
			return nbFAILURE;
		}
	}

	printf("...NetPDL Protocol database loaded.\n\n");

	PacketCapture= new nbPacketCapture;
	if (PacketCapture == NULL)
	{
		printf("Error creating the nbPacketCapture class\n");
		goto end;
	}

	DeviceList= PacketCapture->GetListLocalAdapters();
	if (DeviceList == NULL)
	{
		printf("Error getting the devices installed in the system: %s\n", PacketCapture->GetLastError() );
		goto end;
	}

	printf("Installed adapters:\n");

	IfNumber= 1;
	TargetDestDevice= DeviceList;

	while (TargetDestDevice)
	{
		printf("\tInterface number %d: %s (%s)\n", IfNumber, TargetDestDevice->Description, TargetDestDevice->Name);
		TargetDestDevice= TargetDestDevice->GetNext();
		IfNumber++;
	}
	
	printf("\nPlease enter the number (1-%d) corresponding to the desired device: ", IfNumber - 1);

    scanf("%d", &IfChosen);
    
    if (IfChosen < 1 || IfChosen >= IfNumber)
    {
        printf("\nInterface number out of range.\n");
		goto end;
    }

	TargetDestDevice= DeviceList;
	for (int i= 1; i < IfChosen; TargetDestDevice= TargetDestDevice->GetNext(), i++);

	
	//Cloning + state information of source device updating. 
	if (PacketCapture->SetSrcDataDevice(TargetDestDevice,TargetDestDevice->Name,TargetDestDevice->Description,nbNETDEVICE_PROMISCUOUS) == nbFAILURE)
	{
		printf("Error setting the source device: %s", PacketCapture->GetLastError() );
		goto end;
	}
	
	printf("\nPlease type 'C' if you want to receive packets through the callback,");
	printf("\n         or 'R' if you want to receive packets through a read() function: ");

	fflush(stdin);
    scanf("%c", &ReadMethod);

	if (ReadMethod == 'C')
		ReceiveFromCallback= true;
	else
		ReceiveFromCallback= false;

	if (ReceiveFromCallback)
	{
		if (PacketCapture->RegisterCallbackFunction(ReadPacketsCallback, NULL) == nbFAILURE)
		{
			printf("Error setting the destination callback function: %s", PacketCapture->GetLastError() );
			goto end;
		}
	}

	fflush(stdin);
	printf("\n\nType name of filter file (.asm)\n Press [Enter] to avoid filter loading.\nSelection:");
	gets(FilterString);

	if (PacketCapture->InjectFilter(FilterString) == nbFAILURE)
	{
		printf("Error setting the packet filter: %s", PacketCapture->GetLastError() );
		goto end;
	}

	if (PacketCapture->Start() == nbFAILURE)
	{
		printf("Cannot start the capture process: %s", PacketCapture->GetLastError() );
		goto end;
	}

	printf("\nListening on %s...\n", TargetDestDevice->Description);

	if (ReceiveFromCallback)
	{
		nbNetPkt * Packet; 
		while(1)
		{	
			Packet= PacketCapture->GetPacket();
			PacketCapture->WritePacket(Packet);
			PacketCapture->ReleaseExBuf(Packet);
		}	
	}
	else
	{
	nbNetPkt *Packet;
	while(1)
		{
			if (PacketCapture->ReadPacket(&Packet, NULL) == nbFAILURE)
			{
				printf("Error reading packets from the given source: %s", PacketCapture->GetLastError() );
				goto end;
			}
			ReadPacketsCallback(NULL, Packet, NULL);
			PacketCapture->ReleaseExBuf(Packet);
		}
	}

end:
	PacketCapture->Stop();
	nbCleanup();
	return nbSUCCESS;
}
