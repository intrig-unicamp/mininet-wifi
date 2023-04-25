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


#include <nbee.h>

#define BUFFERS_NUMBER	1000


u_int32_t PrintPacket(nvmExchangeBufferState * buffer)
{
	if(buffer!=NULL)
		{
			unsigned char *b = (unsigned char*)buffer -> pkt_pointer;
			printf("Bytes 12-14: %d %d %d - Length: %d\n", b[12], b[13], b[14], buffer->pktlen);
		}	
	return nbSUCCESS;
}

nbNetVmPortLocalAdapter * GetListAdapters()
{
	nbPacketCapture *PacketCapture;
	nbNetVmPortLocalAdapter *DeviceList, *TargetDestDevice;
	int IfNumber;
	int IfChosen;

	PacketCapture= new nbPacketCapture;
	if (PacketCapture == NULL)
	{
		printf("Error creating the nbPacketCapture class\n");
		return NULL;
	}

	DeviceList= PacketCapture->GetListLocalAdapters();
	if (DeviceList == NULL)
	{
		printf("Error getting the devices installed in the system: %s\n", PacketCapture->GetLastError() );
		return NULL;
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
		return NULL;
    }

	TargetDestDevice= DeviceList;
	for (int i= 1; i < IfChosen; TargetDestDevice= TargetDestDevice->GetNext(), i++);

	return TargetDestDevice;		
}


int main(void)
{
nbNetVm						*NetVMHandle= NULL;
nbNetPe						*PEHandle= NULL;
nbNetVmByteCode				*BytecodeHandle= NULL;
nbNetVmPortLocalAdapter		*Device= NULL;
nbNetVmPortLocalAdapter		*NetVMInPort= NULL;
nbNetVmPortApplication		*NetVMOutPort= NULL;
int i;

	//Create an handle to the NetVM's state
	NetVMHandle = new nbNetVm(0);
	if (NetVMHandle == NULL)
	{
		printf("Cannot create the state of the virtual machine\n");
		return nbFAILURE;
	}

	// Create a new PE state
	PEHandle = new nbNetPe(NetVMHandle);
	if (PEHandle == NULL)
	{
		printf("Cannot create NetPE\n");
		goto end_failure;
	}
	
	// Open the bytecode of the ip filter
	BytecodeHandle = new nbNetVmByteCode();
	if (BytecodeHandle == NULL)
	{
		printf("cannot create the temporary structure used to load the bytecode in the NetPE\n");
		goto end_failure;
	}
			
	if( BytecodeHandle->CreateBytecodeFromAsmFile("../Modules/NetBee/Samples/NBeeNetVMPcap/ip_pull.asm") != nbSUCCESS )
	{
		printf("Cannot open the bytecode\n");
		delete BytecodeHandle;
		goto end_failure;
	}

	// Initialize the state of the PE using the data provided by the bytecode
	if ( PEHandle->InjectCode(BytecodeHandle) != nbSUCCESS )
	{
		printf("Cannot inject the code\n");
		delete BytecodeHandle;
		goto end_failure;
	}		

	delete BytecodeHandle;

	// Create the port that will implement the exporter
	NetVMOutPort= new nbNetVmPortApplication;
	if (NetVMOutPort == NULL)
	{
		printf("Cannot create the output port\n");
		goto end_failure;
	}

	// Attach this port to the virtual machine
	if (NetVMHandle->AttachPort(NetVMOutPort, nvmPORT_EXPORTER | nvmCONNECTION_PULL) == nbFAILURE)
	{
		printf("Cannot add the output port to the NetVM\n");
		goto end_failure;
	}  	

	// Create the port that implements the capture socket
	Device= GetListAdapters();
	if (Device == NULL)
	{
		printf("Cannot create the input port\n");
		goto end_failure;
	}		
	
	NetVMInPort= (nbNetVmPortLocalAdapter *)Device->Clone(Device->Name, Device->Description, nbNETDEVICE_PROMISCUOUS);
	if (NetVMInPort == NULL)
	{
		printf("Cannot create the local capture device\n" );
		goto end_failure;
	}
	
	delete(Device);

	// Register the function that has to be called for getting data
	if (NetVMInPort->RegisterPollingFunction(nbNetVmPortLocalAdapter::SrcPollingFunctionWinPcap, NetVMInPort) == nbFAILURE)
	{
		printf("Cannot register the polling function on the data source\n" );
		goto end_failure;
	}

	// This port will be connected to a push input port
	if (NetVMHandle->AttachPort(NetVMInPort, nvmPORT_COLLECTOR | nvmCONNECTION_PULL) == nbFAILURE)
	{
		printf("Cannot add the input port to the NetVM\n");
		goto end_failure;
	}	

	// Connect the collector port to the first port (port #0) of the PE
	if  (NetVMHandle->ConnectPorts(NetVMInPort->GetNetVmPortDataDevice(), PEHandle->GetPEPort(0)) == nbFAILURE)
	{
		printf("Cannot create the connection between the external world and the PE: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}

	// Connect the exporter port to the second port (port #1) of the PE
	if  (NetVMHandle->ConnectPorts(PEHandle->GetPEPort(1), NetVMOutPort->GetNetVmPortDataDevice()) == nbFAILURE)
	{
		printf("Cannot create the connection between the external world and the PE:\n");
		goto end_failure;
	}	
	
	//start the virual machine
	if (NetVMHandle->Start() == nbFAILURE)
	{
		printf ("Cannot start the virtual machine\n");
		goto end_failure;
	}	

	i = 0;
	// BUFFERS_NUMBER packets are sent to the NetVM
	while(i < BUFFERS_NUMBER)
	{
	nbNetPkt *exbuff;
	NetVMOutPort->Read(&exbuff, NULL);
	PrintPacket(exbuff);
	NetVMOutPort->ReleaseExBuf(exbuff);
	i++;
	}

	return nbSUCCESS;

end_failure:
	if (NetVMHandle) delete NetVMHandle;
	if (PEHandle) delete PEHandle;
	return nbFAILURE;
}