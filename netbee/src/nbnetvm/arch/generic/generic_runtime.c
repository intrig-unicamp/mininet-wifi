//*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "../../helpers.h"
#include "../../../nbee/globals/debug.h"
#include <string.h>

#ifdef WIN32
#define strdup _strdup
#endif

#ifdef HAVE_PCAP
#include <pcap.h>
#endif

#include "arch.h"
#include "generic_runtime.h"


//TODO [OM]: this structure seems not to be used!
//struct _ExbufData
//{
//	uint32_t ID;
//};
//
//typedef struct _ExbufData ExbufData;



void *genRT_AllocRTObject(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf)
{
	/*
		We want every object allocated inside the Runtime environment to be freed when the runtime is destroyed.
		In this first implementation we keep a list of allocated objects, although it would be better to implement
		an arena, or pool based allocator
	*/
	void *obj = nvmAllocObject(size, errbuf);
	if (obj == NULL)
		return NULL;
	if (SLLst_Add_Item_Tail(RTObj->AllocData, obj) != nvmSUCCESS)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}
	return obj;
}

void genRT_ReleaseRTObject(nvmRuntimeEnvironment *RTObj)
{
	#ifdef HAVE_PCAP

	SLLstElement *phys;
	if(! SLLst_Is_Empty(RTObj->PhysInterfaces))
	{
		for (phys=RTObj->PhysInterfaces->Head; phys != RTObj->PhysInterfaces->Tail; phys=phys->Next)
		{
			if (((nvmPhysInterface *) phys->Item)->ArchData != NULL)
			{
				pcap_close(((nvmPhysInterface *) phys->Item)->ArchData);
			}
		}
		if (phys == RTObj->PhysInterfaces->Tail)
		{
			if (((nvmPhysInterface *) phys->Item)->ArchData != NULL)
			{
				pcap_close(((nvmPhysInterface *) phys->Item)->ArchData);
			}
		}
	}
	#endif
	genRT_FreeAllocData(RTObj);
	Free_SingLinkedList(RTObj->PEStates, NULL);
	Free_SingLinkedList(RTObj->HandlerStates,NULL);
	Free_SingLinkedList(RTObj->PhysInterfaces,NULL);
	Free_SingLinkedList(RTObj->ApplInterfaces, NULL);


#ifdef RTE_PROFILE_COUNTERS
  	//free(RTObj->ProfCounterTot);
#endif


	free(RTObj);

}


void genRT_FreeAllocData(nvmRuntimeEnvironment *RTObj)
{

	Free_SingLinkedList(RTObj->AllocData, (nvmFreefunctSLL *) nvmFreeObject );
}


nvmMemDescriptor *genRT_AllocPEMemory(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf)
{
	/*
		The allocation of PE data/shared memory is extremely arch dependent. For general purpose architectures
		we allocate such buffers in the generic Runtime Environment mem-pool, although on other architectures a
		more complicated schema could be used
	*/
	nvmMemDescriptor *descriptor = NULL;
	NETVM_ASSERT(RTObj != NULL, "RTObj cannot be NULL");
	NETVM_ASSERT(size > 0, "size is 0");
	descriptor = arch_AllocRTObject(RTObj, sizeof(nvmMemDescriptor), errbuf);
	if (descriptor == NULL)
		return NULL;

	descriptor->Base = arch_AllocRTObject(RTObj, size, errbuf);
	if (descriptor->Base == NULL)
		return NULL;
	// [GM] Shouldn't descriptor be freed here, before returning?

	descriptor->Size = size;
	descriptor->Flags = 0;
	return descriptor;
}


nvmRESULT genRT_CreateExbufPool(nvmRuntimeEnvironment *RTObj, uint32_t numExBuf, uint32_t pkt_size, uint32_t info_size, char *errbuf)
{
	uint32_t i = 0;
	RTObj->ExbufPool = arch_AllocRTObject(RTObj, sizeof(nvmExbufPool), errbuf);
	if (RTObj->ExbufPool == NULL)
		return nvmFAILURE;
	RTObj->ExbufPool->Pool = arch_AllocRTObject(RTObj, sizeof(nvmExchangeBuffer*) * numExBuf, errbuf);
	if (RTObj->ExbufPool->Pool == NULL)
		return nvmFAILURE;
	RTObj->ExbufPool->PktData = arch_AllocRTObject(RTObj, pkt_size * numExBuf, errbuf);
	if (RTObj->ExbufPool->PktData == NULL)
		return nvmFAILURE;
	RTObj->ExbufPool->InfoData = arch_AllocRTObject(RTObj, info_size * numExBuf, errbuf);
	if (RTObj->ExbufPool->InfoData == NULL)
		return nvmFAILURE;
	RTObj->ExbufPool->Size = numExBuf;
	RTObj->ExbufPool->Top = numExBuf;
	RTObj->ExbufPool->PacketLen = pkt_size;	//default packet size
	RTObj->ExbufPool->InfoLen = info_size;	//default info-partition size
	for (i = 0; i < numExBuf; i++)
	{
	RTObj->ExbufPool->Pool[i]=arch_AllocRTObject(RTObj, sizeof(nvmExchangeBuffer), errbuf);

	}
	// Preallocated areas for packet and info are subdivided in numExBuf chunks and each one is assigned to a different exchange buffer
	for (i = 0; i < numExBuf; i++)
	{
		RTObj->ExbufPool->Pool[i]->ID = i;	//ID will select the right chunks when releasing the exchange buffer
		RTObj->ExbufPool->Pool[i]->PacketBuffer = &RTObj->ExbufPool->PktData[pkt_size * i];
		RTObj->ExbufPool->Pool[i]->PacketLen = pkt_size;
		RTObj->ExbufPool->Pool[i]->InfoData = &RTObj->ExbufPool->InfoData[info_size *i];
		RTObj->ExbufPool->Pool[i]->InfoLen = info_size;
		memset(RTObj->ExbufPool->Pool[i]->InfoData, 0, RTObj->ExbufPool->Pool[i]->InfoLen);
	}
	return nvmSUCCESS;
}



void genRT_ReleaseExbufPool(nvmExbufPool *ExbufPool)
{
	uint32_t i;
	for (i=0; i < ExbufPool->Top;i++)
		free(ExbufPool->Pool[i]);

	free(ExbufPool->PktData);
	free(ExbufPool->InfoData);
	free(ExbufPool);

}

nvmRESULT genRT_CreatePhysInterfList(nvmRuntimeEnvironment *RTObj, char *errbuf)
{
	/*
		We impose general purpose architectures to have lib/winpcap installed, in order to gather packets from the network.
		Moreover, on non-windows platforms, libnet is required in order to be able to bind NetVM output ports to output
		interfaces.
	*/
#ifdef HAVE_PCAP
	pcap_if_t *alldevs;
	pcap_if_t *d;
	int i=0;
	nvmPhysInterface *PhysInterface;
	nvmPhysInterfaceInfo *info;
    // Retrieve the device list from the local machine and print it
    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Error in pcap_findalldevs\n");
        exit(1);
    }



    // Print the list
    for (d = alldevs; d != NULL; d= d->next)
    {
		for(i=0; i<2; i++)
		{
			PhysInterface = genRT_AllocRTObject(RTObj, sizeof(nvmPhysInterface), errbuf);
			info = genRT_AllocRTObject(RTObj,sizeof(nvmPhysInterfaceInfo),errbuf);

			if (PhysInterface == NULL)
				return nvmFAILURE;
			info->Name = strdup(d->name);
						PhysInterface->RTEnv = RTObj;
			PhysInterface->PhysInterfInfo = info;
			info->PhysInterface = PhysInterface;
			if(i == 0)
			{
				info->InterfDir = INTERFACE_DIR_IN;
				if(d->description != NULL)
				{
					info->Description = strdup(d->description);
				}
				else
					info->Description = "interfaccia in input";
			}
			else
			{
				info->InterfDir = INTERFACE_DIR_OUT;
				if(d->description != NULL)
				{
					info->Description = strdup(d->description);
				}
				else
					info->Description = "interfaccia in output";
			}
			if (SLLst_Add_Item_Tail(RTObj->PhysInterfaces, PhysInterface) == nvmFAILURE)
				return nvmFAILURE;
		}
    }
    // We don't need any more the device list. Free it
    pcap_freealldevs(alldevs);
#endif

	return nvmSUCCESS;
}



nvmRESULT	genRT_SetTStamp(nvmExchangeBuffer *exbuf, char *errbuf)
{
//TODO
	return nvmSUCCESS;
}



nvmRESULT	genRT_ReadFromPhys(nvmExchangeBuffer *exbuf, nvmPhysInterface *PhysInterface,char *errbuf)
{
#ifdef HAVE_PCAP
	pcap_t *descr;
	struct pcap_pkthdr *pkthdr;
	const uint8_t *packet;
	int r=1;
	descr = pcap_open_live (PhysInterface->PhysInterfInfo->Name, BUFSIZ, 0, -1, errbuf);
	if (descr == NULL) {
		errsnprintf(errbuf, nvmERRBUF_SIZE, "pcap_open_live() \n");
		exit (1);
	}

	PhysInterface->ArchData = descr;

	do {
		if ((r = pcap_next_ex (descr, &pkthdr, &packet)) == 1) {
			exbuf->PacketBuffer = (uint8_t*)packet;
			exbuf->PacketLen = pkthdr->caplen;

			#ifdef RTE_PROFILE_COUNTERS
			PhysInterface->CtdHandler->ProfCounters->NumPkts++;
			#endif
			PhysInterface->CtdHandler->PEState->ConnTable[PhysInterface->CtdPort].CtdHandlerFunct(&exbuf,PhysInterface->CtdPort,PhysInterface->CtdHandler);


		}
	} while (r >= 0 );
#endif
return nvmSUCCESS;
}

int32_t	genRT_OpenPhysIn(nvmPhysInterface *PhysInterface, char *errbuf)
{
	char * name;
	pcap_t *descr;
	name = PhysInterface->PhysInterfInfo->Name;
	descr = pcap_open_live (name, BUFSIZ, 0, -1, errbuf);
	if (descr == NULL) {
		errsnprintf(errbuf, nvmERRBUF_SIZE, "pcap_open_live()\n");
		exit (1);
	}
	PhysInterface->ArchData = descr;

	return nvmSUCCESS;
}

int32_t genRT_OutPhysicIn (nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
		pcap_sendpacket(HandlerState->CtdInterf->ArchData,(u_char *)exbuf->PacketBuffer,(int) exbuf->PacketLen);
		return nvmSUCCESS;
}


//used by push
nvmRESULT	genRT_WriteExBuff(nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
			#ifdef RTE_PROFILE_COUNTERS

				HandlerState->ProfCounters->NumTicks += (HandlerState->ProfCounters->TicksEnd -	HandlerState->ProfCounters->TicksStart) -HandlerState->ProfCounters->TicksDelta;
			#endif

	return nvmSUCCESS;
}


//used by pull
nvmRESULT genRT_ReadExBuff(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
#ifdef HAVE_PCAP
	//char errbuf[250];

	if (HandlerState->PEState->ConnTable[port].CtdHandlerType == HANDLER_TYPE_POLLING)
	{
			#ifdef RTE_PROFILE_COUNTERS
				//HandlerState->ProfCounters->numPktIn++;
			#endif
			*exbuf = arch_GetExbuf(HandlerState->PEState->RTEnv);
			HandlerState->PEState->ConnTable[port].CtdPolling(*exbuf, port);
			return nvmSUCCESS;
	}

	if (HandlerState->Handler->HandlerType== PULL_HANDLER)
	{
		HandlerState->PEState->ConnTable[port].CtdHandlerFunct(exbuf,port,HandlerState);
	}
#endif

return nvmSUCCESS;
}


nvmRESULT genRT_nvmInit()
{
return nvmSUCCESS;
}









