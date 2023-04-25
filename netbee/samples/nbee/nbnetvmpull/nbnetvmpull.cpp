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

#define BUFFERS_NUMBER	10
#define PACKET_SIZE		1024


void CreateIPARPPacket(u_int8_t *payload, u_int32_t length, u_int32_t flag)
{
	for (int i = 0 ; i < (int)length; i++)
		payload[i] = (u_int8_t)i;

	if (flag == 1)
	{ // IP
		payload[12] = 0x08;
		payload[13] = 0x00;
	}
	else
	{ //not IP, it's ARP
		payload[12] = 0x08;
		payload[13] = 0x06;
	}
}


u_int32_t PrintPacket(nvmExchangeBufferState * buffer)
{
	if(buffer!=NULL)
		{
			unsigned char *b = (unsigned char*)buffer -> pkt_pointer;
			printf("Bytes 12-14: %d %d %d - Length: %d\n", b[12], b[13], b[14], buffer->pktlen);
		}	
	return nbSUCCESS;
}


u_int32_t DataSourcePollingFuntion(void * ptr, nvmExchangeBufferState *buffer, int *dummy)
{
static int i;

	CreateIPARPPacket((u_int8_t *) ptr, PACKET_SIZE, (i++) % 2);
	buffer -> pkt_pointer = (u_int8_t *)ptr;
	// 	memcpy (buffer -> pkt_pointer, ptr, PACKET_SIZE);
	buffer->pktlen = PACKET_SIZE;

	return nbSUCCESS;
}


int main(void)
{
nbNetVm					*NetVMHandle = NULL;
nbNetPe					*PEHandle = NULL;
nbNetVmByteCode			*BytecodeHandle = NULL;
nbNetVmPortApplication	*NetVMInPort = NULL, *NetVMOutPort = NULL;
u_int8_t buff[PACKET_SIZE];
int i;

	//Create an handle to the NetVM's state
	NetVMHandle = new nbNetVm(0);
	if (NetVMHandle == NULL)
	{
		printf("Cannot create the state of the virtual machine\n");
		return nbFAILURE;
	}

	//Create a new PE state
	PEHandle = new nbNetPe(NetVMHandle);
	if (PEHandle == NULL)
	{
		printf("Cannot create NetPE\n");
		goto end_failure;
	}
	
	//open the bytecode of the ip filter
	BytecodeHandle = new nbNetVmByteCode();
	if (BytecodeHandle == NULL)
	{
		printf("cannot create the temporary structure used to load the bytecode in the NetPE\n");
		goto end_failure;
	}

	if ( BytecodeHandle->CreateBytecodeFromAsmFile("../Modules/NetBee/Samples/NBeeNetVMPull/ip_pull.asm") != nbSUCCESS )
	{
		printf("Cannot open the bytecode: %s\n", BytecodeHandle->GetLastError() );
		delete BytecodeHandle;
		goto end_failure;
	}

	// Initialize the state of the PE using the data provided by the bytecode
	if ( PEHandle->InjectCode(BytecodeHandle) != nbSUCCESS )
	{
		printf("Cannot inject the code: %s\n", PEHandle->GetLastError() );
		delete BytecodeHandle;
		goto end_failure;
	}		

	delete BytecodeHandle;

	// Create the port that will implement the collector
	NetVMInPort= new nbNetVmPortApplication;
	if (NetVMInPort == NULL)
	{
		printf("Cannot create the input port\n");
		goto end_failure;
	}

	// Register the function that has to be called for getting data
	if (NetVMInPort->RegisterPollingFunction(DataSourcePollingFuntion, (void*)buff) == nbFAILURE)
	{
		printf("Cannot register the polling function on the data source: %s\n", NetVMInPort->GetLastError() );
		goto end_failure;
	}

	// Attach this port to the virtual machine
	if (NetVMHandle->AttachPort(NetVMInPort, nvmPORT_COLLECTOR | nvmCONNECTION_PULL) == nbFAILURE)
	{
		printf("Cannot add the output port to the NetVM\n");
		goto end_failure;
	}

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

	// Connect the collector port to the first port (port #0) of the PE
	if  (NetVMHandle->ConnectPorts(NetVMInPort->GetNetVmPortDataDevice(), PEHandle->GetPEPort(0)) == nbFAILURE)
	{
		printf("Cannot create the connection between the external world and the PE: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}

	// Cconnect the exporter port to the second port (port #1) of the PE
	if  (NetVMHandle->ConnectPorts(PEHandle->GetPEPort(1), NetVMOutPort->GetNetVmPortDataDevice()) == nbFAILURE)
	{
		printf("Cannot create the connection between the external world and the PE: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}	
	
	//start the virual machine
	if (NetVMHandle->Start() == nbFAILURE)
	{
		printf ("Cannot start the virtual machine: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}	

	i = 0;
	// BUFFERS_NUMBER packets are sent to the NetVM
	while (i < BUFFERS_NUMBER)
	{
		nbNetPkt * exbuff;
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