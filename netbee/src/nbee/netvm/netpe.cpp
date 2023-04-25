/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdlib.h>

#include <nbee_netvm.h>
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"

#ifndef WIN32
#include <string.h>
#endif

nbNetPe::nbNetPe(nbNetVm *netVmState)
{
	NetPeState = nvmNetPeCreate(netVmState->GetNetVmPointer());			
	VmState = netVmState->GetNetVmPointer();	
};


nbNetPe::~nbNetPe(void)
{

};

nvmInternalPort * nbNetPe::GetPEPort(int PortId)
{
	return nvmGetInternalPort(this->GetPEPointer(),PortId);	
}


nbNetVmByteCode * nbNetPe::CompileProgram(char *source_code)
{
	return NULL;	
}

int nbNetPe::InjectCode(nbNetVmByteCode *NetVmByteCode)
{
	// Initialize the state of the PE1 using the data provided by the bytecode
	if ( nvmNetPeInject(this->NetPeState,NetVmByteCode->GetNetVmByteCodePointer()) != nvmSUCCESS )
	{
		printf("Cannot inject the code into PE\n");
		return nbFAILURE;
	}
	return nbSUCCESS;
}