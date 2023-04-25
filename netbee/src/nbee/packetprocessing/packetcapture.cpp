/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <string.h>
#include <nbnetvm.h>
#include <nbpacketprocess.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"


nbPacketCapture::nbPacketCapture()
{
	m_netVMHandle= NULL;
	m_netPEHandle= NULL;
	m_byteCodeHandle= NULL;
	m_srcNetDevice= NULL;
	m_dstNetDevice= NULL;

	m_nvmIsRunning= false;
};


nbPacketCapture::~nbPacketCapture()
{
	Stop();
	DELETE_PTR(m_srcNetDevice);
	DELETE_PTR(m_dstNetDevice);
	DELETE_PTR(m_byteCodeHandle);
};



int nbPacketCapture::SetSrcDataDevice(nbNetVmPortDataDevice *NetDevice, char *Name, char *Description, int Flags)
{
	DELETE_PTR(m_srcNetDevice);

	m_srcNetDevice= ((nbNetVmPortLocalAdapter*) NetDevice)->Clone(Name, Description, Flags);

	if (m_srcNetDevice == NULL)
		return nbFAILURE;

	return nbSUCCESS;
}


int nbPacketCapture::SetDstDataDevice(nbNetVmPortDataDevice *NetDevice, char *Name, char *Description, int Flags)
{
	DELETE_PTR(m_dstNetDevice);

	m_dstNetDevice = ((nbNetVmPortLocalAdapter*)NetDevice)->Clone(Name, Description, Flags);

	if (m_dstNetDevice == NULL)
		return nbFAILURE;
	
	return nbSUCCESS;
}


int nbPacketCapture::RegisterCallbackFunction(nbDataDstCallbackFunction *DataDstCallbackFunction, void *CustomPtr)
{
	nbNetVmPortApplication * NetVmPortApplication;

	if (m_dstNetDevice)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "Only one receiver is allowed. Currently, the target receiver is a network device.");
			return nbFAILURE;
		}

	NetVmPortApplication= new nbNetVmPortApplication;
	if (NetVmPortApplication == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
			return nbFAILURE;
		}

	if (NetVmPortApplication->RegisterCallbackFunction(DataDstCallbackFunction, CustomPtr) == nbFAILURE)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), NetVmPortApplication->GetLastError() );
			return nbFAILURE;
		}

	m_dstNetDevice=  NetVmPortApplication;

	return nbSUCCESS;
}


int nbPacketCapture::InjectFilter(char *FilterString)
{
	if (strlen(FilterString)==0)
		return nbSUCCESS;
	
	if (m_byteCodeHandle)
		m_byteCodeHandle->close();
	else
	{
		m_byteCodeHandle= new nbNetVmByteCode();
		if (m_byteCodeHandle == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
			return nbFAILURE;
		}
	}

	if (m_byteCodeHandle->CreateBytecodeFromAsmFile(FilterString) == nbFAILURE)
	{
		ssnprintf(m_errbuf, sizeof(m_errbuf), "%s", m_byteCodeHandle->GetLastError() );
		return nbFAILURE;
	}

	/*if (m_netPEHandle)
	{
		if ( m_netPEHandle->InjectCode(m_byteCodeHandle) == nbFAILURE)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "%s", m_netPEHandle->GetLastError() );
			return nbFAILURE;
		}
	}*/

	return nbSUCCESS;
}


int nbPacketCapture::Start()
{
	int ConnectionType;

	if (m_srcNetDevice == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "Source network device not specified: cannot start the capture");
			return nbFAILURE;
		}
	
	Stop();

	m_netVMHandle= new nbNetVm(0);
	if (m_netVMHandle == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "Error instantiating the NetVM");
			return nbFAILURE;
		}

	//PULL behavior
	if (m_dstNetDevice == NULL)
		{	
			ConnectionType = nvmCONNECTION_PULL;
			
			//m_dstNetDevice = new nbNetVmPortLocalAdapter;
			m_dstNetDevice = new nbNetVmPortApplication();
			if (m_dstNetDevice == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Cannot create the destination port for the NetVM.");
					return nbFAILURE;
				}
			
			if (m_srcNetDevice->RegisterPollingFunction(nbNetVmPortLocalAdapter::SrcPollingFunctionWinPcap, m_srcNetDevice) == nbFAILURE)
				{
					printf("Cannot register the polling function on the data source\n" );
					return nbFAILURE;
				}

			if (m_netVMHandle->AttachPort(m_srcNetDevice, nvmPORT_COLLECTOR | ConnectionType) == nbFAILURE)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), m_netVMHandle->GetLastError());
					return nbFAILURE;
				}

			if (m_netVMHandle->AttachPort(m_dstNetDevice, nvmPORT_EXPORTER | ConnectionType) == nbFAILURE)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), m_netVMHandle->GetLastError());
					return nbFAILURE;
				}
		}
	else
		{
			ConnectionType= nvmCONNECTION_PUSH;

			if (m_netVMHandle->AttachPort(m_srcNetDevice, nvmPORT_COLLECTOR | ConnectionType) == nbFAILURE)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), m_netVMHandle->GetLastError());
					return nbFAILURE;
				}

			if (m_netVMHandle->AttachPort(m_dstNetDevice, nvmPORT_EXPORTER | ConnectionType) == nbFAILURE)
				{
					ssnprintf(m_errbuf, sizeof(m_errbuf), m_netVMHandle->GetLastError());
					return nbFAILURE;
				}
		}
		

	if (m_byteCodeHandle)
	{
		m_netPEHandle = new nbNetPe(m_netVMHandle);
		if (m_netPEHandle == NULL)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
			return nbFAILURE;
		}

		if ( m_netPEHandle->InjectCode(m_byteCodeHandle) == nbFAILURE)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "%s", m_netPEHandle->GetLastError() );
			return nbFAILURE;
		}

		if  (m_netVMHandle->ConnectPorts(m_srcNetDevice->GetNetVmPortDataDevice(), m_netPEHandle->GetPEPort(0)) == nbFAILURE)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "Error connecting the NetVM input port toward the NetPE: %s", m_netVMHandle->GetLastError() );
			return nbFAILURE;
		}

		if  (m_netVMHandle->ConnectPorts(m_netPEHandle->GetPEPort(1), m_dstNetDevice->GetNetVmPortDataDevice()) == nbFAILURE)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "Error connecting the NetPE output port toward the NetVM: %s", m_netVMHandle->GetLastError() );
			return nbFAILURE;
		}
	}
	else
	{
		if  (m_netVMHandle->ConnectPorts(m_srcNetDevice->GetNetVmPortDataDevice(), m_dstNetDevice->GetNetVmPortDataDevice()) == nbFAILURE)
		{
			ssnprintf(m_errbuf, sizeof(m_errbuf), "Error creating the connection between the two NetVM ports: %s", m_netVMHandle->GetLastError() );
			return nbFAILURE;
		}
	}

	// Start the virtual machine
	if (m_netVMHandle->Start() == nbFAILURE)
	{
		ssnprintf(m_errbuf, sizeof(m_errbuf), "Error starting the virtual machine: %s", m_netVMHandle->GetLastError() );
		return nbFAILURE;
	}

	m_nvmIsRunning= true;
	return nbSUCCESS;
}

void nbPacketCapture::Stop()
{
	m_netVMHandle->Stop();
	m_nvmIsRunning= false;
	DELETE_PTR(m_netVMHandle);
	DELETE_PTR(m_netPEHandle);
}

int nbPacketCapture::ReadPacket(nbNetPkt **Packet, int *Flags)
{
	return ((nbNetVmPortApplication*)m_dstNetDevice)->Read(Packet,Flags);
}

int nbPacketCapture::WritePacket(nbNetPkt *Packet)
{
	return ((nbNetVmPortLocalAdapter *)m_srcNetDevice)->Write(Packet);
	
}

nbNetPkt * nbPacketCapture::GetPacket()
{
	nvmExchangeBufferState	*Packet = NULL;
	Packet = nvmGetExBuf(((nbNetVmPortLocalAdapter *)m_srcNetDevice)->GetNetVmPortDataDevice() );
	((nbNetVmPortLocalAdapter *)m_srcNetDevice)->SrcPollingFunctionWinPcap(this->m_srcNetDevice,Packet,NULL);	
	return Packet;
}

int nbPacketCapture::ReleaseExBuf(nbNetPkt *Packet)
{
	m_dstNetDevice->ReleaseExBuf(Packet);
	return nbSUCCESS;
}