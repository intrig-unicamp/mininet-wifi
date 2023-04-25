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


u_int32_t ApplicationCallback(void* voidDummy, nvmExchangeBufferState *xbuffer, int* intDummy)
{
	unsigned char *b = (unsigned char *) xbuffer -> pkt_pointer;
	printf("Bytes 12 13: %d %d - Length: %d\n", b[12], b[13], xbuffer -> pktlen);
	return nbSUCCESS;
}


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



int main(void)
{
nbNetVm *NetVMHandle = NULL;
nbNetPe *PEHandle = NULL;
nbNetVmByteCode *BytecodeHandle = NULL;
nbNetVmPortApplication *NetVMInPort = NULL, *NetVMOutPort = NULL;
int i;

	printf("NetVMPushIP execution: going to process %d network packets,\n but only %d of them are IP\n", BUFFERS_NUMBER, BUFFERS_NUMBER / 2);

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

			
	if (BytecodeHandle->CreateBytecodeFromAsmFile("../Modules/NetBee/Samples/NBeeNetVMPush/ip_push.asm") != nbSUCCESS )
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


	// Create the port that implements the collector
	NetVMInPort= new nbNetVmPortApplication;
	if (NetVMInPort == NULL)
	{
		printf("Cannot create the input port\n");
		goto end_failure;
	}		

	// This port will be connected to a push input port
	if (NetVMHandle->AttachPort(NetVMInPort, nvmPORT_COLLECTOR | nvmCONNECTION_PUSH) == nbFAILURE)
	{
		printf("Cannot add the input port to the NetVM: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}

	// Create the port that will implement the exporter
	NetVMOutPort= new nbNetVmPortApplication;
	if (NetVMOutPort == NULL)
	{
		printf("Cannot create the output port\n");
		goto end_failure;
	}
	// Register the callback function on that port
	if (NetVMOutPort->RegisterCallbackFunction(ApplicationCallback, (void *)NULL) == nbFAILURE)
	{
		printf("Cannot register the callback function: %s\n", NetVMOutPort->GetLastError() );
		goto end_failure;
	}		

	// This port will be connected to a push input port.
	if (NetVMHandle->AttachPort(NetVMOutPort, nvmPORT_EXPORTER | nvmCONNECTION_PUSH) == nbFAILURE)
	{
		printf("Cannot add the output port to the NetVM: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}

	//connect the collector port to the first port (port #0) of the PE
	if  (NetVMHandle->ConnectPorts(NetVMInPort->GetNetVmPortDataDevice(), PEHandle->GetPEPort(0)) == nbFAILURE)
	{
		printf("Cannot create the connection between the two ports: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}

	//connect the exporter port to the second port (port #1) of the PE
	if  (NetVMHandle->ConnectPorts(PEHandle->GetPEPort(1), NetVMOutPort->GetNetVmPortDataDevice()) == nbFAILURE)
	{
		printf("Cannot create the connection between the two ports: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}	
	
	//start the virual machine
	if (NetVMHandle->Start() == nbFAILURE)
	{
		printf ("Cannot start the virtual machine: %s\n", NetVMHandle->GetLastError() );
		goto end_failure;
	}	

	i = 0;

	//BUFFERS_NUMBER packets are sent to the NetVM
	while(i < BUFFERS_NUMBER)
	{
		u_int8_t buff[PACKET_SIZE];

		CreateIPARPPacket(buff, PACKET_SIZE, i%2);

		if (NetVMInPort->Write(buff, PACKET_SIZE,nvmEXCHANGE_BUFFER_NOCOPY) == nbFAILURE)
		{
			printf("It was not possible to insert the buffer #%d in the virtual machine\n", i);
			goto end_failure;
		}
		i++;
	}

	return nbSUCCESS;

end_failure:
	if (NetVMHandle) delete NetVMHandle;
	if (PEHandle) delete PEHandle;
	return nbFAILURE;
}