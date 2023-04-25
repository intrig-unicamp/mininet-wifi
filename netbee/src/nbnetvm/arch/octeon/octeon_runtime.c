/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "../../helpers.h"
//#include "debug.h"
#include <string.h>

#ifdef WIN32
#define strdup _strdup
#endif

#include "arch.h"
#include "octeon_runtime.h"
#include "cvmx-config.h"
#include "cvmx.h"
#include "cvmx-fpa.h"
#include "cvmx-pko.h"
#include "cvmx-dfa.h"
#include "cvmx-pow.h"
#include "cvmx-bootmem.h"
#include "cvmx-helper.h"



#define NUM_PACKET_BUFFERS  4096
#define NUM_WORK_BUFFERS    NUM_PACKET_BUFFERS
#define NUM_DFA_BUFFERS 512


struct _ExbufData
{
	uint32_t ID;
};

typedef struct _ExbufData ExbufData;


nvmRESULT octRT_nvmInit()
{

	cvmx_user_app_init();

	if(cvmx_helper_initialize_fpa (NUM_PACKET_BUFFERS, NUM_WORK_BUFFERS, CVMX_PKO_MAX_OUTPUT_QUEUES*64, 0, 0)!=0)
		printf("ERRORE inizializzazione fpa");
#ifdef REGEX_COPRO_ENABLED

  void *dfa_buffers;

	printf("Faccio la alloc dei buffer\n");
	dfa_buffers = cvmx_bootmem_alloc(CVMX_FPA_DFA_POOL_SIZE * NUM_DFA_BUFFERS, 128);
	if (dfa_buffers == NULL) {
	    printf("Could not allocate dfa_buffers.\n");
	    printf("TEST FAILED\n");
	    return 1;
	}
	printf("faccio inizializzazione fpa pre dfa");
	cvmx_fpa_setup_pool(CVMX_FPA_DFA_POOL, "dfa", dfa_buffers, CVMX_FPA_DFA_POOL_SIZE, NUM_DFA_BUFFERS);

	printf("Initializing the DFA\n");
	cvmx_dfa_initialize();

#endif
	printf("fatta inizializzazione\n");
return nvmSUCCESS;
}

void *octRT_AllocRTObject(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf)
{
	/*
		We want every object allocated inside the Runtime environment to be freed when the runtime is destroyed.
		In this first implementation we keep a list of allocated objects, although it would be better to implement
		an arena, or pool based allocator
	*/
	
	
	void *obj = nvmAllocObject(size, errbuf);
	//void *obj = cvmx_bootmem_alloc (size, 2^4);
	if (obj == NULL)
		return NULL;
	if (SLLst_Add_Item_Tail(RTObj->AllocData, obj) != nvmSUCCESS)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, ALLOC_FAILURE);
		return NULL;
	}
	return obj;
}

void *octRT_AllocFPAObject(nvmRuntimeEnvironment *RTObj, uint32_t size, uint64_t pool)
{
	 void *res= cvmx_fpa_alloc(pool); 
		return res;
}

void *octRT_ReleaseFPAObject(nvmRuntimeEnvironment *RTObj)
{
/*
static void cvmx_fpa_free  	(   	void *   	 ptr,
		uint64_t  	pool,
		uint64_t  	num_cache_lines
	)
	*/
return NULL;
}

void *octRT_ReleaseRTObject(nvmRuntimeEnvironment *RTObj)
{
free(RTObj);
}
nvmMemDescriptor *octRT_AllocPEMemory(nvmRuntimeEnvironment *RTObj, uint32_t size, char *errbuf)
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

	descriptor->Size = size;
	descriptor->Flags = 0;
	return descriptor;
}


nvmRESULT octRT_CreateExbufPool(nvmRuntimeEnvironment *RTObj, uint32_t numExBuf, uint32_t pkt_size, uint32_t info_size, char *errbuf)
{
	uint32_t i = 0;
	RTObj->ExbufPool = arch_AllocRTObject(RTObj, sizeof(nvmExbufPool), errbuf);
	if (RTObj->ExbufPool == NULL)
		return nvmFAILURE;
	RTObj->ExbufPool->Pool = arch_AllocRTObject(RTObj, sizeof(nvmExchangeBuffer*) * numExBuf, errbuf);
	if (RTObj->ExbufPool->Pool == NULL)
		return nvmFAILURE;
	RTObj->ExbufPool->PktData = octRT_AllocFPAObject(RTObj, pkt_size * numExBuf,CVMX_FPA_PACKET_POOL);
	if (RTObj->ExbufPool->PktData == NULL)
	{	printf("impossibile allocare pktdata\n");
		return nvmFAILURE;
	}
	RTObj->ExbufPool->InfoData = octRT_AllocFPAObject(RTObj, info_size * numExBuf,CVMX_FPA_PACKET_POOL);
	if (RTObj->ExbufPool->InfoData == NULL)
	{	printf("impossibile allocare infodata\n");
		return nvmFAILURE;
		}
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

nvmExchangeBuffer *octRT_GetExbuf(nvmRuntimeEnvironment *RTObj)
{
	NETVM_ASSERT(RTObj != NULL, "RTObj cannot be NULL");
	NETVM_ASSERT(RTObj->ExbufPool != NULL, "Exbuf pool not allocated");
	
	nvmExchangeBuffer *res;
	cvmx_wqe_t * work = octRT_AllocFPAObject(RTObj, 0,CVMX_FPA_WQE_POOL);
	work->grp = 14;
	work->tag_type=CVMX_POW_TAG_TYPE_ORDERED;
	work->tag= 2;
	work->qos= 1;
	
	if (RTObj->ExbufPool->Top <= 1)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Exchange buffer pool is empty");
		return NULL;
	}
	res= RTObj->ExbufPool->Pool[--RTObj->ExbufPool->Top];
	res->ArchData = work;
	return res;
}


nvmRESULT octRT_ReleaseExbuf(nvmRuntimeEnvironment *RTObj, nvmExchangeBuffer *exbuf)
{
	NETVM_ASSERT(RTObj != NULL, "RTObj cannot be NULL");
	NETVM_ASSERT(exbuf != NULL, "exbuf cannot be NULL");
	NETVM_ASSERT(RTObj->ExbufPool != NULL, "Exbuf pool not allocated");
	if (RTObj->ExbufPool->Top >= RTObj->ExbufPool->Size)
	{
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Exchange buffer pool is full");
		return nvmFAILURE;
	}
	//the exchange buffer is reassociated with its original packet and info memory chunks
	exbuf->PacketBuffer = &RTObj->ExbufPool->PktData[exbuf->ID * RTObj->ExbufPool->PacketLen];
	memset(exbuf->PacketBuffer, 0, RTObj->ExbufPool->PacketLen);
	exbuf->PacketLen = 0;
	exbuf->InfoData = &RTObj->ExbufPool->InfoData[exbuf->ID * RTObj->ExbufPool->InfoLen];
	memset(exbuf->InfoData, 0, RTObj->ExbufPool->InfoLen);
	exbuf->InfoLen = 0;
	exbuf->TStamp_s = 0;
	exbuf->TStamp_us = 0;
	RTObj->ExbufPool->Pool[RTObj->ExbufPool->Top++] = exbuf;

	return nvmSUCCESS;
}


nvmRESULT octRT_CreatePhysInterfList(nvmRuntimeEnvironment *RTObj, char *errbuf)
{
	/*
		We impose general purpose architectures to have lib/winpcap installed, in order to gather packets from the network.
		Moreover, on non-windows platforms, libnet is required in order to be able to bind NetVM output ports to output 
		interfaces.
	*/
	/*
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
		
		PhysInterface = octRT_AllocRTObject(RTObj, sizeof(nvmPhysInterface), errbuf);
		info = octRT_AllocRTObject(RTObj,sizeof(nvmPhysInterfaceInfo),errbuf);

		if (PhysInterface == NULL)
			return nvmFAILURE;
		PhysInterface->Name = strdup(d->name);
		info->Name = strdup(d->name);
		if(d->description != NULL)
		{	PhysInterface->Description = strdup(d->description);
			info->Description = strdup(d->description);
		}
		PhysInterface->RTEnv = RTObj;
		PhysInterface->PhysInterfInfo = info;
		info->PhysInterface = PhysInterface;
		
		if (SLLst_Add_Item_Tail(RTObj->PhysInterfaces, PhysInterface) == nvmFAILURE)
			return nvmFAILURE;
		
    }
    
    // We don't need any more the device list. Free it
    pcap_freealldevs(alldevs);
		
	*/
	return nvmSUCCESS;
}



nvmRESULT	octRT_SetTStamp(nvmExchangeBuffer *exbuf, char *errbuf)
{
//TODO
	return nvmSUCCESS;
}



nvmRESULT	octRT_ReadFromPhys(nvmExchangeBuffer *exbuf, nvmPhysInterface *PhysInterface,char *errbuf)
{
/*
	pcap_t *descr;
	struct pcap_pkthdr *pkthdr;
	const uint8_t *packet;
	int r=1;
	
	descr = pcap_open_live (PhysInterface->Name, BUFSIZ, 0, -1, errbuf);
	if (descr == NULL) {
		errsnprintf(errbuf, nvmERRBUF_SIZE, "pcap_open_live() \n");
		exit (1);
	}
	
	

	do {
		if ((r = pcap_next_ex (descr, &pkthdr, &packet)) == 1) {
			exbuf->PacketBuffer = packet;
			exbuf->PacketLen = pkthdr->len;
			
			if(arch_WriteExBuff(exbuf, PhysInterface->CtdPort,PhysInterface->CtdHandler) == nvmFAILURE)
			{
				errsnprintf(errbuf, nvmERRBUF_SIZE, "Impossible to write on the exchange buffer\n");
				return nvmFAILURE;
			}	

		}
	} while (r >= 0 );
*/
return nvmSUCCESS;
}

//used by push
nvmRESULT	octRT_WriteExBuff(nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState)
{

	char errbuf[250];
	//pcap_t *descr;
	char * name;

			cvmx_buf_ptr_t pkt;
			pkt.ptr = (void *)exbuf->PacketBuffer; 
			((cvmx_wqe_t *) (exbuf->ArchData))->packet_ptr= pkt;
			cvmx_pow_work_submit 	( (cvmx_wqe_t *) (exbuf->ArchData), ((cvmx_wqe_t *) (exbuf->ArchData))->tag, ((cvmx_wqe_t *) (exbuf->ArchData))->tag_type,((cvmx_wqe_t *) (exbuf->ArchData))->qos,((cvmx_wqe_t *) (exbuf->ArchData))->grp);
		printf("dopo submit\n");	
	
	/*
	if (HandlerState->PEState->ConnTable[port].CtdHandlerType == HANDLER_TYPE_CALLBACK)
		{
			HandlerState->PEState->ConnTable[port].CtdCallback(exbuf);
			return nvmSUCCESS;
		}

	//it's a physic interface that wants to send packets out 	
	if (HandlerState->PEState->ConnTable[port].CtdHandlerType == HANDLER_TYPE_INTERF_OUT && HandlerState->Handler->HandlerType== PUSH_HANDLER)
		{
		
		name = HandlerState->PEState->ConnTable[port].CtdInterf->Name;
	
		descr = pcap_open_live (name, BUFSIZ, 0, -1, errbuf);
		if (descr == NULL) {
			errsnprintf(errbuf, nvmERRBUF_SIZE, "pcap_open_live()\n");
			exit (1);
		}

			#ifdef RTE_PROFILE_COUNTERS
  				octRT_SetTime_Now(FORWARDED, HandlerState);
				HandlerState->ProfCounters->numPktOut++;
			#endif	
		pcap_sendpacket(descr,(u_char *)exbuf->PacketBuffer,(int) exbuf->PacketLen); 
		pcap_close (descr);
		return nvmSUCCESS;
		}




	if (HandlerState->Handler->HandlerType== PUSH_HANDLER)
		{
			cvmx_buf_ptr_t pkt;
			pkt.ptr = (void *)exbuf->PacketBuffer; 
			((cvmx_wqe_t *) (exbuf->ArchData))->packet_ptr= pkt;
			cvmx_pow_work_submit 	( (cvmx_wqe_t *) (exbuf->ArchData), ((cvmx_wqe_t *) (exbuf->ArchData))->tag, ((cvmx_wqe_t *) (exbuf->ArchData))->tag_type,((cvmx_wqe_t *) (exbuf->ArchData))->qos,((cvmx_wqe_t *) (exbuf->ArchData))->grp);
		printf("dopo submit\n");	
			if(PORT_IS_CONN_PE(HandlerState->Handler->OwnerPE->PortTable[port].PortFlags))
			{
				uint32_t ctdPort;
				ctdPort =HandlerState->Handler->OwnerPE->PortTable[port].CtdPort; 
				#ifdef RTE_PROFILE_COUNTERS
  			//		octRT_SetTime_Now(START, HandlerState->PEState->ConnTable[port].CtdHandler);
			//		HandlerState->PEState->ConnTable[port].CtdHandler->ProfCounters->numPktIn++;
				#endif	
				HandlerState->PEState->ConnTable[port].CtdHandlerFunct(&exbuf,ctdPort,HandlerState->PEState->ConnTable[port].CtdHandler);
			}
			else
			{
				#ifdef RTE_PROFILE_COUNTERS
  			//		octRT_SetTime_Now(START, HandlerState);
			//		HandlerState->ProfCounters->numPktIn++;
				#endif	
				HandlerState->PEState->ConnTable[port].CtdHandlerFunct(&exbuf,port,HandlerState);
				}
			return nvmSUCCESS;
		}


	if (HandlerState->Handler->HandlerType== PULL_HANDLER)
		{
			errsnprintf(errbuf, nvmERRBUF_SIZE, "It's a pull handler type: it's wrong");
			return nvmFAILURE;
		}
	if (HandlerState->PEState->ConnTable[port].CtdHandlerType == HANDLER_TYPE_POLLING)
		{
			errsnprintf(errbuf, nvmERRBUF_SIZE, "It's a polling handler type: it's wrong");
			return nvmFAILURE;
		}
*/
return nvmSUCCESS;
}


//used by pull
nvmRESULT octRT_ReadExBuff(nvmExchangeBuffer **exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
	char errbuf[250];
	
	if (HandlerState->PEState->ConnTable[port].CtdHandlerType == HANDLER_TYPE_POLLING)
	{
			*exbuf = arch_GetExbuf(HandlerState->PEState->RTEnv);
			HandlerState->PEState->ConnTable[port].CtdPolling(*exbuf, port);
			return nvmSUCCESS;	
	}

	if (HandlerState->Handler->HandlerType== PULL_HANDLER)
	{
		HandlerState->PEState->ConnTable[port].CtdHandlerFunct(exbuf,port,HandlerState);
	}

return nvmSUCCESS;
}


int32_t	octRT_OpenPhysIn(nvmPhysInterface *PhysInterface, char *errbuf)
{
	return nvmSUCCESS;
}


int32_t octRT_OutPhysicIn (nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
	return nvmSUCCESS;
}

uint64_t octRT_SampleDeltaTicks()
{
	errorprintf(__FILE__, __FUNCTION__, __LINE__, "This function has not been implemented.\n");
	// To be implemented
	return 0;
}






