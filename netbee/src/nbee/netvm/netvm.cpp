/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include <stdio.h>
#include <pcap.h>
#include <nbee_netvm.h>


#ifndef WIN32
#include <string.h>
/* stdlib want numbers, not empty define  */
#define	__LITTLE_ENDIAN	1234
#define __BYTE_ORDER __LITTLE_ENDIAN
#include <stdlib.h>
#endif


#include "../globals/utils.h"
#include "../globals/debug.h"
#include "../globals/globals.h"



nbNetVm::nbNetVm(bool UseJit)
{
	if (UseJit)
		VmState = nvmNetVmCreate(1);
	else
		VmState = nvmNetVmCreate(0);
};


nbNetVm::~nbNetVm(void)
{
	nvmNetVmClose(VmState);
};

void nbNetVm::SetProfilingResultsFile(FILE *file)
{
	nvmProfResultsFile = file;
}


int nbNetVm::AttachPort(nbNetVmPortDataDevice * NetVmExternalPort, int Mode)
{
	//Attach the input port to NetVM
	if( nvmAttachPort(this->VmState, NetVmExternalPort->GetNetVmPortDataDevice(), Mode)!= nvmSUCCESS )
	{
		printf("Cannot attach the input port to NetVM\n");
		return nbFAILURE;
	}
	return nbSUCCESS; 
}


int nbNetVm::ConnectPorts(nvmExternalPort *firstPort, nvmExternalPort *secondPort)
{
	//connect the  (port #) of the PE to (port#) of the next PE
	if  (nvmConnectExternalPort(firstPort, secondPort) == nvmFAILURE)
	{
		printf("Cannot create the connection between the two external ports\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}

int nbNetVm::ConnectPorts(nvmExternalPort *firstPort, nvmInternalPort *secondPort)
{
	//connect the collector port to the first port (port #0) of the PE
	if  (nvmConnectCollectorToInternal(firstPort, secondPort) == nvmFAILURE)
	{
		printf("Cannot create the connection between the two ports\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}


int nbNetVm::ConnectPorts(nvmInternalPort *firstPort, nvmInternalPort *secondPort)
{
	//connect the  (port #) of the PE to (port#) of the next PE
	if  (nvmConnectInternalPorts(firstPort, secondPort) == nvmFAILURE)
	{
		printf("Cannot create the connection between the two internal ports\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}

int nbNetVm::ConnectPorts(nvmInternalPort *firstPort, nvmExternalPort *secondPort)
{
	//connect second port (port #1) of the PE3 the exporter
	if  (nvmConnectInternalToExporter(firstPort, secondPort) == nvmFAILURE)
	{
		printf("Cannot create the connection between the two ports\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}


int nbNetVm::Start(void)
{
	//start the virual machine
	if (nvmStart(this->GetNetVmPointer()) == nvmFAILURE)
	{
		printf ("Cannot start the virtual machine\n");
		return nbFAILURE;
	}	
	return nbSUCCESS;
}


void nbNetVm::Stop(void)
{

}