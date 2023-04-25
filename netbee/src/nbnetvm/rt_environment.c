/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file rt_environment.c
 *	\brief This file contains the functions that implement NetVM Runtime Environment
 */

#include <nbnetvm.h>
#include <string.h>
#include <stdlib.h>
#include "helpers.h"
#include "int_structs.h"
#include "rt_environment.h"
#include "./arch/arch_runtime.h"
#include "coprocessor.h"


#include "./jit/jit_interface.h"
#include "netvmprofiling.h"
#include "../../nbee/globals/profiling-functions.h"


#define NUM_EX_BUF 10
#define PKT_SIZE 1500
#define INFO_SIZE 2048


#define TARGETS_NUM ((sizeof(targets) / sizeof(nvmBackendDescriptor)) - 1)


static nvmBackendDescriptor targets[] =
{
#ifdef ENABLE_X86_BACKEND
	{"x86", 	nvmBACKEND_X86, 3, (nvmDO_BCHECK |nvmDO_NATIVE | nvmDO_ASSEMBLY | nvmDO_INLINE )},
#endif
#ifdef ENABLE_X64_BACKEND
	{"x64", 	nvmBACKEND_X64, 3, (nvmDO_BCHECK |nvmDO_NATIVE | nvmDO_ASSEMBLY | nvmDO_INLINE )},
#endif
#ifdef ENABLE_X11_BACKEND
	{"x11", 	nvmBACKEND_X11, 3, (nvmDO_ASSEMBLY | nvmDO_INLINE)},
#endif
#ifdef ENABLE_OCTEON_BACKEND
	{"octeon",	nvmBACKEND_OCTEON, 3, (nvmDO_ASSEMBLY | nvmDO_INLINE | nvmDO_INIT)},
#endif
#ifdef ENABLE_OCTEON_BACKEND
	{"octeonc",	nvmBACKEND_OCTEONC, 3, (nvmDO_ASSEMBLY | nvmDO_INLINE | nvmDO_INIT)},
#endif
  {NULL, 0, 0}
};


nvmBackendDescriptor* nvmGetBackendList(uint32_t *Num)
{
	*Num = TARGETS_NUM;
	return targets;
}


#ifdef RTE_PROFILE_COUNTERS
  nvmCounter	jitProfCounters;		//!< Profiling counters
//	uint64_t *callbjitstop;
	uint64_t *callsta;
	uint64_t *callsto;
	 nvmCounter	callprof;		//!< Profiling counters
#endif


//EXPORT

nvmRuntimeEnvironment *nvmCreateRTEnv(nvmNetVM *netVMApp, uint32_t flags, char *errbuf)
{
	uint32_t shd_size=10;

	nvmRuntimeEnvironment *RTObj = NULL;

	arch_nvmInit();

	RTObj = nvmAllocObject(sizeof(nvmRuntimeEnvironment), errbuf);
	if (RTObj == NULL)
		return NULL;

	RTObj->PEStates = New_SingLinkedList(NOT_SORTED_LST);
	RTObj->HandlerStates= New_SingLinkedList(NOT_SORTED_LST);
	RTObj->PhysInterfaces= New_SingLinkedList(NOT_SORTED_LST);
	RTObj->ApplInterfaces= New_SingLinkedList(NOT_SORTED_LST);
	RTObj->AllocData=New_SingLinkedList(NOT_SORTED_LST);
	RTObj->VerbosityLevel= 1;
	RTObj->VerboseOutput= stdout;
	RTObj->TargetCode= NULL;

	//it creates and append to the rigth list the PEState and the HandlerState
	if (SLLst_Iterate_3Args(netVMApp->NetPEs, (nvmIteratefunct3Args *) nvmCreatePEStates, RTObj, &shd_size, errbuf) == nvmFAILURE)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "PEStates and HandlerStates not created");
		return NULL;
	}

	//it creates the ConnectionTable for each PEState
	if (SLLst_Iterate_2Args(RTObj->PEStates, (nvmIteratefunct2Args *)nvmCreateConnectTable, RTObj, errbuf)== nvmFAILURE)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "ConnectTable not created");
		return NULL;
	}

	//it creates the exchangeBufferPool
	if (arch_CreateExbufPool(RTObj, NUM_EX_BUF, PKT_SIZE, INFO_SIZE, errbuf)== nvmFAILURE)
	{
		printf("errore nella create exbufpool\n");
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Exchange buffer pool not created");
		return NULL;
	}


	arch_CreatePhysInterfList(RTObj, errbuf);
	// creates the PEStates, HandlerStates, Physical interface list and exchange buffer pool
	// all the state local to the runtime environment object should be allocated through the arch_AllocRTObject() API
	// Such function keeps track of all the allocated data inside the RTObj->AllocData list
	// When destroying the RTObj all the runtime memory is freed at once

	RTObj->execution_option = flags;
	#ifdef RTE_PROFILE_COUNTERS
	RTObj->Tot =calloc (1, sizeof(nvmCounter));
	#endif


	return RTObj;
}

int32_t  nvmCreatePEStates(nvmNetPE *PE,nvmRuntimeEnvironment *RTObj, uint32_t *shd_size, char *errbuf)
{
	tmp_nvmPEState *PEState = NULL;
	uint32_t i=0;
	PEState = arch_AllocRTObject(RTObj, sizeof(tmp_nvmPEState), errbuf);
	if (PEState == NULL)
		return nvmFAILURE;

	/* Allocate data memory */
	if (PE->DataMemSize > 0)
		PEState->DataMem = arch_AllocPEMemory(RTObj, PE->DataMemSize, errbuf);

	/* Allocate shared memory */
	PEState->ShdMem = arch_AllocPEMemory(RTObj, *shd_size, errbuf);

	/* Allocate space for the inited memory segment */
	/* [GM] Maybe I should't use memcpy here? I didn't find an arch_CopyMemory(), so I did it this way.
	 * Someone please fix, in the case! */
	if (PE->InitedMemSize > 0) {
		PEState->InitedMem = arch_AllocPEMemory(RTObj, PE->InitedMemSize, errbuf);
		memcpy (PEState->InitedMem->Base, PE->InitedMem, PE->InitedMemSize);
	} else {
		PEState->InitedMem = NULL;
	}

	/* Setup coprocessors table */
	PEState->NCoprocRefs=PE->NCopros;
	PEState->CoprocTable = (nvmCoprocessorState *) malloc ((PEState->NCoprocRefs) * sizeof (nvmCoprocessorState));
	if (PEState->CoprocTable == NULL)
	{
		printf("It was impossible to allocate the coprocessors table for PE\n");
		return nvmFAILURE;
	}
	for (i=0; i<PEState->NCoprocRefs; i++)
	{
		nvm_copro_map[PE->Copros[i]].copro_create(&PEState->CoprocTable[i]);
	}

	//PEState->ConnTable=nvmCreatePortState(RTObj, errbuf);

	PEState->ConnTable = arch_AllocRTObject(RTObj, PE->NPorts * sizeof(nvmPortState), errbuf);
	if (PEState->ConnTable == NULL)
		return nvmFAILURE;
	//At the begin each PEState is connect at the interpreter function
	for (i=0; i < PE->NPorts; i++)
	{
		PEState->ConnTable[i].CtdHandlerFunct=arch_Execute_Handlers;
	}

	PEState->Nports= PE->NPorts;
	PEState->RTEnv = RTObj;
	PE->PEState = PEState;

	if (SLLst_Add_Item_Tail(RTObj->PEStates, PEState) == nvmFAILURE)
		return nvmFAILURE;


	//for each PEHandler it creates a PEHandlerState
	if (nvmCreateHandlerStates(RTObj, PE, PE->InitHandler,PEState, errbuf) == nvmFAILURE)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Init HandlerState not created");
		return nvmFAILURE;
	}
	if (nvmCreateHandlerStates(RTObj, PE, PE->PushHandler,PEState, errbuf) == nvmFAILURE)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Push HandleState not created");
		return nvmFAILURE;
	}

	if (nvmCreateHandlerStates(RTObj, PE, PE->PullHandler,PEState, errbuf) == nvmFAILURE)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Pull HandlerState not created");
		return nvmFAILURE;
	}

	return nvmSUCCESS;
}


nvmPortState  *nvmCreatePortState(nvmRuntimeEnvironment *RTObj,char *errbuf)
{
	nvmPortState *portState = NULL;
	portState = arch_AllocRTObject(RTObj, sizeof(nvmPortState), errbuf);
	if (portState == NULL)
		return NULL;
	portState->CtdHandlerFunct=arch_Execute_Handlers;

	return portState;
}


int32_t nvmCreateHandlerStates(nvmRuntimeEnvironment *RTObj,nvmNetPE *PE, nvmPEHandler *xHandler, tmp_nvmPEState *PEState,char *errbuf)
{
	nvmHandlerState * handlerState = NULL;
#ifdef RTE_DYNAMIC_PROFILE
	char dataFileName[256];
#endif
	handlerState = arch_AllocRTObject(RTObj,sizeof(nvmHandlerState), errbuf);
	if (handlerState == NULL)
		return nvmFAILURE;

	handlerState->Handler=xHandler;
	handlerState->NLocals=xHandler->NumLocals;
  	handlerState->StackSize=xHandler->MaxStackSize;
  	handlerState->PEState=PEState;
#ifdef RTE_PROFILE_COUNTERS
  	handlerState->ProfCounters=calloc (1, sizeof(nvmCounter));
	handlerState->ProfCounters->TicksDelta= arch_SampleDeltaTicks();
  	//printf("Delta:%llu\n",handlerState->ProfCounters->TicksDelta);
	handlerState->temp= calloc (1, sizeof(nvmCounter));
	handlerState->temp->TicksDelta= arch_SampleDeltaTicks();
#endif
#ifdef RTE_DYNAMIC_PROFILE
	snprintf(dataFileName, 255, "%s.%s.txt", handlerState->Handler->OwnerPE->Name, handlerState->Handler->Name);
	handlerState->MemDataFile = fopen(dataFileName, "w");
	if (handlerState->MemDataFile == NULL)
	{
		errsnprintf(errbuf, sizeof(errbuf), "Unable to create dynamic profiling data file: %s", dataFileName);
		return nvmFAILURE;
	}
	printf("created file %s\n", dataFileName);
	handlerState->t = 0;
#endif
	xHandler->HandlerState= handlerState;

	if (SLLst_Add_Item_Tail(RTObj->HandlerStates, handlerState) == nvmFAILURE)
		return nvmFAILURE;

	return nvmSUCCESS;

}


int32_t nvmCreateConnectTable(tmp_nvmPEState *PEState,nvmRuntimeEnvironment *RTObj,char *errbuf)
{
		if (SLLst_Iterate_2Args(RTObj->HandlerStates, (nvmIteratefunct2Args *)nvmFindHandlerState, PEState,errbuf) == nvmFAILURE)
			return nvmFAILURE;

		return nvmSUCCESS;
}


int32_t nvmFindHandlerState(nvmHandlerState *HandlerState, tmp_nvmPEState *PEState, char *errbuf)
{
uint32_t i=0;
nvmNetPE *ourPE, *ctdPE;
uint32_t ctdport;
nvmPEPort portTab;

	//it takes only the HandlerState that refers to the PEState
	if(HandlerState->PEState == PEState)
	{
		for (i=0; i<PEState->Nports; i++)
		{
			ourPE=HandlerState->Handler->OwnerPE;
			portTab=ourPE->PortTable[i];
		
			if (PORT_IS_CONN_PE(portTab.PortFlags))
			{
				if (PORT_IS_COLLECTOR(portTab.PortFlags))
				{
#ifdef ENABLE_NETVM_LOGGING
					logdata(LOG_RUNTIME_CREATE_PEGRAPH, "DIR OUT Port %d is connected with port %d of another NetPE", i, ourPE->PortTable[i].CtdPort);
#endif
					ctdPE=portTab.CtdPE;
					ctdport=ourPE->PortTable[i].CtdPort;
					PEState->ConnTable[i].CtdHandler=ctdPE->PortTable[ctdport].Handler->HandlerState;
				}
			}

		}

	}

	return nvmSUCCESS;
}
int32_t nvmOutCallback(nvmExchangeBuffer **exbuf, uint32_t dummy, nvmHandlerState *HandlerState)
{

#if defined(__APPLE__)
/*
	On Mac Intel machines we MUST realign the stack to 16 bits.
	The user defined callback could call some external functions residing in a shared library,
	whose calling operations are managed by the dynamic linker.
	If the stack is not aligned to 16 bits, we get an EXC_BAD_INSTRUCTION exception in the function
	__dyld_stub_binding_helper_interface(), causing the program to crash.
*/
__asm__("movl %esp,%esi\n\t"	/* copy esp in esi */
		"andl $0x0F,%esi\n\t"	/* modulo 16 */
		"subl %esi,%esp");		/* realign the stack */
#endif

//todo profiling stop HandlerState

#ifdef RTE_PROFILE_COUNTERS
	HandlerState->ProfCountersCallb->TicksStart= nbProfilerGetTime();
#endif

	HandlerState->Callback(*exbuf);

#ifdef RTE_PROFILE_COUNTERS
	HandlerState->ProfCountersCallb->TicksEnd= nbProfilerGetTime();
	HandlerState->ProfCountersCallb->NumTicks += HandlerState->ProfCountersCallb->TicksEnd - HandlerState->ProfCountersCallb->TicksStart - HandlerState->ProfCountersCallb->TicksDelta;
#endif

	return nvmSUCCESS;
}


int32_t nvmOutPhysicIn(nvmExchangeBuffer **exbuf, uint32_t dummy, nvmHandlerState *HandlerState)
{
	arch_OutPhysicIn(*exbuf,dummy,HandlerState);
	return nvmSUCCESS;
}


nvmAppInterface *nvmCreateAppInterfacePushIN(nvmRuntimeEnvironment *RTObj, char *errbuf)
{

	nvmAppInterface *AppInterface = NULL;

	AppInterface = arch_AllocRTObject(RTObj, sizeof(nvmAppInterface), errbuf);
	if (AppInterface == NULL)
		return NULL;
	AppInterface->InterfDir= INTERFACE_DIR_IN;
	AppInterface->CtdHandlerType = HANDLER_TYPE_INTERF_IN;
	AppInterface->RTEnv = RTObj;
#ifdef RTE_PROFILE_COUNTERS
  	AppInterface->ProfCounterTot=calloc (1, sizeof(nvmCounter));
	AppInterface->ProfCounterTot->TicksDelta= arch_SampleDeltaTicks();
#endif
	if (SLLst_Add_Item_Tail(RTObj->ApplInterfaces, AppInterface) == nvmFAILURE)
		return NULL;

	return AppInterface;
}


nvmAppInterface *nvmCreateAppInterfacePushOUT(nvmRuntimeEnvironment *RTObj, nvmCallBackFunct *CtdCallback, char *errbuf)
{

	nvmAppInterface *AppInterface = NULL;

	AppInterface = arch_AllocRTObject(RTObj, sizeof(nvmAppInterface), errbuf);
	if (AppInterface == NULL)
		return NULL;

	AppInterface->InterfDir= INTERFACE_DIR_OUT;
	AppInterface->CtdHandlerType = HANDLER_TYPE_CALLBACK;
	AppInterface->Handler.CtdCallback= CtdCallback;
	AppInterface->RTEnv = RTObj;

	if (SLLst_Add_Item_Tail(RTObj->ApplInterfaces, AppInterface) == nvmFAILURE)
		return NULL;


	return AppInterface;
}


nvmAppInterface *nvmCreateAppInterfacePullIN(nvmRuntimeEnvironment *RTObj,nvmPollingFunct *PollingFunct, char *errbuf)
{

	nvmAppInterface *AppInterface = NULL;

	AppInterface = arch_AllocRTObject(RTObj, sizeof(nvmAppInterface), errbuf);
	if (AppInterface == NULL)
		return NULL;

	AppInterface->InterfDir= INTERFACE_DIR_IN;
	AppInterface->CtdHandlerType = HANDLER_TYPE_POLLING;
	AppInterface->Handler.CtdPolling = PollingFunct;
	AppInterface->RTEnv = RTObj;

	if (SLLst_Add_Item_Tail(RTObj->ApplInterfaces, AppInterface) == nvmFAILURE)
		return NULL;


	return AppInterface;
}


nvmAppInterface *nvmCreateAppInterfacePullOUT(nvmRuntimeEnvironment *RTObj, char *errbuf)
{

	nvmAppInterface *AppInterface = NULL;

	AppInterface = arch_AllocRTObject(RTObj, sizeof(nvmAppInterface), errbuf);
	if (AppInterface == NULL)
		return NULL;

	AppInterface->InterfDir= INTERFACE_DIR_OUT;
	AppInterface->CtdHandlerType = HANDLER_TYPE_INTERF_OUT;
	AppInterface->RTEnv = RTObj;

	if (SLLst_Add_Item_Tail(RTObj->ApplInterfaces, AppInterface) == nvmFAILURE)
		return NULL;


	return AppInterface;
}


int32_t nvmBindAppInterf2Socket(nvmAppInterface *AppInterface,nvmSocket *Socket)
{
	uint32_t port;
	uint32_t flags;
	char errbuf[250];

	port= Socket->CtdPort;
	flags= Socket->CtdPE->PortTable[port].PortFlags;
	Socket->InterfaceType = APPINTERFACE;
	Socket->AppInterface = AppInterface;



	//PUSH IN
	if( PORT_IS_EXPORTER(flags) && PORT_IS_DIR_PUSH(flags) && AppInterface->CtdHandlerType == HANDLER_TYPE_INTERF_IN )
	{
		AppInterface->CtdHandler = Socket->CtdPE->PortTable[port].Handler->HandlerState;
		AppInterface->CtdPort = port;
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_RUNTIME_CREATE_PEGRAPH, "Executed bind push in");
#endif
		return nvmSUCCESS;
	}

	//PUSH OUT
	if( PORT_IS_COLLECTOR(flags) && PORT_IS_DIR_PUSH(flags) && AppInterface->CtdHandlerType == HANDLER_TYPE_CALLBACK )
	{
		//creo HandlerState fittizio
		nvmHandlerState * HandlerState = NULL;
		HandlerState = arch_AllocRTObject(AppInterface->RTEnv,sizeof(nvmHandlerState), errbuf);
		if (HandlerState == NULL)
			return nvmFAILURE;
		if (SLLst_Add_Item_Tail(AppInterface->RTEnv->HandlerStates, HandlerState) == nvmFAILURE)
			return nvmFAILURE;
		HandlerState->Callback = AppInterface->Handler.CtdCallback;
#ifdef RTE_PROFILE_COUNTERS
  	HandlerState->temp=calloc (1, sizeof(nvmCounter));
	HandlerState->temp->TicksDelta= arch_SampleDeltaTicks();
  	HandlerState->ProfCounters=calloc (1, sizeof(nvmCounter));
	HandlerState->ProfCounters->TicksDelta= arch_SampleDeltaTicks();
  	HandlerState->ProfCountersCallb=calloc (1, sizeof(nvmCounter));
	HandlerState->ProfCountersCallb->TicksDelta= arch_SampleDeltaTicks();
#endif
		//collego l'HandlerState fittizio e registro la funz wrapper
		Socket->CtdPE->PEState->ConnTable[port].CtdHandler = HandlerState;
		Socket->CtdPE->PEState->ConnTable[port].CtdHandlerFunct = nvmOutCallback;

		AppInterface->CtdPort = port;
		PORT_SET_CONN_PE(Socket->CtdPE->PortTable[port].PortFlags);
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_RUNTIME_CREATE_PEGRAPH, "Executed bind push out");
#endif
		return nvmSUCCESS;
	}


	//PULL IN
	if( PORT_IS_EXPORTER(flags) && PORT_IS_DIR_PULL(flags) && AppInterface->CtdHandlerType == HANDLER_TYPE_POLLING )
	{
		AppInterface->CtdPort = port;
		Socket->CtdPE->PEState->ConnTable[port].CtdPolling = AppInterface->Handler.CtdPolling;
		Socket->CtdPE->PEState->ConnTable[port].CtdHandlerType = HANDLER_TYPE_POLLING;
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_RUNTIME_CREATE_PEGRAPH, "Executed bind pull in");
#endif
		return nvmSUCCESS;
	}

	//PULL OUT
	if( PORT_IS_COLLECTOR(flags) && PORT_IS_DIR_PULL(flags) && AppInterface->CtdHandlerType == HANDLER_TYPE_INTERF_OUT )
	{
		AppInterface->CtdHandler = Socket->CtdPE->PortTable[port].Handler->HandlerState;
		AppInterface->CtdPort = port;
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_RUNTIME_CREATE_PEGRAPH, "Executed bind pull out");
#endif
		return nvmSUCCESS;
	}

	return nvmFAILURE;


}
int32_t nvmBindPhysInterf2Socket(nvmPhysInterface *PhysInterface, nvmSocket *Socket)
{
	char errbuf[250];
	uint32_t port;

	port=Socket->CtdPort;
	PhysInterface->CtdPort = port;
	Socket->InterfaceType = PHYSINTERFACE;
	Socket->PhysInterface = PhysInterface;


	if(PhysInterface->PhysInterfInfo->InterfDir == INTERFACE_DIR_IN)
	{
		Socket->CtdPE->PEState->ConnTable[port].CtdInterf = PhysInterface;
		PhysInterface->CtdHandler = Socket->CtdPE->PortTable[port].Handler->HandlerState;
	}

	if(PhysInterface->PhysInterfInfo->InterfDir == INTERFACE_DIR_OUT)
	{
		nvmHandlerState * HandlerState = NULL;
		HandlerState = arch_AllocRTObject(PhysInterface->RTEnv,sizeof(nvmHandlerState), errbuf);
		if (HandlerState == NULL)
			return nvmFAILURE;
		if (SLLst_Add_Item_Tail(PhysInterface->RTEnv->HandlerStates, HandlerState) == nvmFAILURE)
			return nvmFAILURE;
		//collego l'HandlerState fittizio e registro la funz wrapper
		Socket->CtdPE->PEState->ConnTable[port].CtdHandler = HandlerState;
		HandlerState->CtdInterf=PhysInterface;
		Socket->CtdPE->PEState->ConnTable[port].CtdHandlerFunct = nvmOutPhysicIn;
		PORT_SET_CONN_PE(Socket->CtdPE->PortTable[port].PortFlags);
		Socket->CtdPE->PEState->ConnTable[port].CtdInterf = PhysInterface;
		Socket->CtdPE->PEState->ConnTable[port].CtdHandlerType = HANDLER_TYPE_INTERF_OUT;
		arch_OpenPhysIn(PhysInterface,errbuf);
	}

	return nvmSUCCESS;
}

nvmPhysInterfaceInfo *nvmEnumeratePhysInterfaces(nvmRuntimeEnvironment *RTObj,char *errbuf)
{
	nvmPhysInterfaceInfo * info = NULL;
	nvmPhysInterfaceInfo * head = NULL;
	SLLstElement *phys;
	if (RTObj == NULL)
		return NULL;

	if (RTObj->PhysInterfaces == NULL)
		return NULL;

	//scorro la lista RTObj->PhysInterfaces
	for (phys=RTObj->PhysInterfaces->Head; phys != RTObj->PhysInterfaces->Tail && phys != NULL; phys=phys->Next)
	{
		info=((nvmPhysInterface *) phys->Item)->PhysInterfInfo;
		info->Next =((nvmPhysInterface *) ((SLLstElement *) phys->Next)->Item)->PhysInterfInfo;
		if(phys ==RTObj->PhysInterfaces->Head )
			head=info;
		printf("name: %s\n",info->Name);
		printf("description: %s\n",info->Description);
	}
	if (phys == RTObj->PhysInterfaces->Tail)
	{
		info=((nvmPhysInterface *) phys->Item)->PhysInterfInfo;
		info->Next=NULL;
		printf("name: %s\n",info->Name);
		printf("description: %s\n",info->Description);

	}

	return head;
}




int32_t nvmStartPhysInterface(nvmPhysInterface *PhysInterface,void *userData, char *errbuf)
{
	nvmExchangeBuffer *exbuf;
	if (PhysInterface->RTEnv->execution_option == nvmRUNTIME_COMPILEONLY)
	{
		return nvmSUCCESS;
	}

	if (PhysInterface->PhysInterfInfo->InterfDir != INTERFACE_DIR_IN)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Wrong type of interface\n");
		return nvmFAILURE;
	}



	exbuf = arch_GetExbuf(PhysInterface->RTEnv);
	if (exbuf == NULL)
		return nvmFAILURE;

	exbuf->UserData = userData;


	if (arch_SetTStamp(exbuf,errbuf) == nvmFAILURE)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Impossible to set Time stamp\n");
		return nvmFAILURE;
	}


	if (arch_ReadFromPhys(exbuf, PhysInterface,errbuf) == nvmFAILURE)
		return nvmFAILURE;


return nvmSUCCESS;

}

//use this funct in push mode
int32_t nvmWriteAppInterface(nvmAppInterface *AppInterface, uint8_t *pkt, uint32_t PktLen, void *userData, char * errbuf)
{
nvmTStamp ts;

	//Ivano: the following code was into another function (nvmWriteAppInterfaceTS), but it was useless and less effient.. Then I have merged all
	//into this one

	nvmHandlerFunction *f;
	nvmExchangeBuffer *exbuf;

	uint32_t porta;
	nvmHandlerState *handler;

	ts.sec = 0;
	ts.usec = 0;

#ifdef RTE_PROFILE_COUNTERS
uint32_t lung;
//	printf("num pkt: %d\n",AppInterface->ProfCounterTot->NumPkts );
	//uint64_t *stp;
	//uint64_t *sto = &AppInterface->ProfCounterTot->TicksEnd;
	//uint64_t *star = &AppInterface->ProfCounterTot->TicksStart;
	//uint64_t *stop;

	AppInterface->ProfCounterTot->NumPkts ++;
	lung=sizeof(nvmCounter);
    memset(&jitProfCounters,0,lung);

//	callbjitstop=&jitProfCounters.TicksEnd;

	jitProfCounters.NumTicks=0;

	AppInterface->ProfCounterTot->TicksStart= nbProfilerGetTime();
#endif

	//todo metterlo come assert e toglierlo dal runtime
	/*
	if (AppInterface->CtdHandlerType != HANDLER_TYPE_INTERF_IN)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Wrong type of interface\n");
		return nvmFAILURE;
	}
	*/

	exbuf = arch_GetExbuf(AppInterface->RTEnv);
	if (exbuf == NULL)
		return nvmFAILURE;

	exbuf->UserData = userData;
#ifdef	ARCH_RUNTIME_OCTEON
	memcpy(exbuf->PacketBuffer,pkt,PktLen);
#else
	exbuf->PacketBuffer = pkt;

	exbuf->TStamp_s = ts.sec;
	exbuf->TStamp_us = ts.usec;

#endif


	exbuf->PacketLen = PktLen;
	//memset(exbuf->InfoData, 0, exbuf->InfoLen);
	//exbuf->InfoData = &AppInterface->RTEnv->ExbufPool->InfoData[exbuf->ID * AppInterface->RTEnv->ExbufPool->InfoLen];

//#ifdef RTE_PROFILE_COUNTERS
//	AppInterface->CtdHandler->ProfCounters->NumPkts ++;
//#endif


#ifdef RTE_PROFILE_COUNTERS
	//AppInterface->CtdHandler->ProfCounters->TicksDelta= arch_SampleDeltaTicks();
	AppInterface->CtdHandler->ProfCounters->TicksStart= nbProfilerGetTime();
#endif
	
	f=AppInterface->CtdHandler->PEState->ConnTable[AppInterface->CtdPort].CtdHandlerFunct;
	porta=AppInterface->CtdPort;
	handler=AppInterface->CtdHandler;

#ifdef RTE_PROFILE_COUNTERS
	AppInterface->ProfCounterTot->TicksEnd= nbProfilerGetTime();
	AppInterface->ProfCounterTot->NumTicks += AppInterface->ProfCounterTot->TicksEnd - AppInterface->ProfCounterTot->TicksStart - AppInterface->ProfCounterTot->TicksDelta;
#endif

#ifdef RTE_PROFILE_COUNTERS
	jitProfCounters.TicksDelta= AppInterface->ProfCounterTot->TicksDelta;
	jitProfCounters.TicksStart= nbProfilerGetTime();
#endif

#ifdef CODE_PROFILING
{
int i;
	AllocateProfilerExecTime();

	for (i=0; i < CODEXEC_SAMPLES; i++)
	{
	int j;
	uint64_t Start, End;

		Start= nbProfilerGetTime();
		for (j= 0; j < CODEXEC_RUNS_PER_SAMPLE; j++)
		{
#endif
			f(&exbuf,porta,handler);//rdi, rsi, rdx on linux
#ifdef CODE_PROFILING
		}
		End= nbProfilerGetTime();
		//printf("100 esecuzioni ci mettono %llu\n",End-Start);
		ProfilerStoreSample(Start, End);
	}
	ProfilerProcessData();
	printf("\n======================= Code Execution Time profiling ======================\n");

	ProfilerPrintSummary();
	ProfilerExecTimeCleanup();
}
#endif

#ifdef RTE_PROFILE_COUNTERS
	jitProfCounters.TicksEnd= nbProfilerGetTime();

	jitProfCounters.NumTicks = jitProfCounters.TicksEnd - jitProfCounters.TicksStart - jitProfCounters.TicksDelta;

	AppInterface->ProfCounterTot->TicksStart= nbProfilerGetTime();
#endif

	arch_ReleaseExbuf(AppInterface->RTEnv, exbuf);

#ifdef RTE_PROFILE_COUNTERS
	AppInterface->ProfCounterTot->TicksEnd= nbProfilerGetTime();
	AppInterface->ProfCounterTot->NumTicks += AppInterface->ProfCounterTot->TicksEnd - AppInterface->ProfCounterTot->TicksStart - AppInterface->ProfCounterTot->TicksDelta;
#endif

	return nvmSUCCESS;
}

//use this function in pull mode
int32_t nvmReadAppInterface(nvmAppInterface *AppInterface,nvmExchangeBuffer **exbuf, void *userData, char *errbuf)
{
	if (AppInterface->RTEnv->execution_option == nvmRUNTIME_COMPILEONLY)
	{
		return nvmSUCCESS;
	}
	if (AppInterface->CtdHandlerType != HANDLER_TYPE_INTERF_OUT)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Wrong type of interface\n");
		return nvmFAILURE;
	}

		if(arch_SetTStamp(*exbuf,errbuf) == nvmFAILURE)
		{
			errsnprintf(errbuf, nvmERRBUF_SIZE, "Impossible to set Time stamp\n");
			return nvmFAILURE;
		}

	if(arch_ReadExBuff(exbuf, AppInterface->CtdPort,AppInterface->CtdHandler) == nvmFAILURE)
	{
		errsnprintf(errbuf, nvmERRBUF_SIZE, "Impossible to write on the exchange buffer\n");
		return nvmFAILURE;
	}



	return nvmSUCCESS;
}

int32_t nvmInit()
{

	arch_nvmInit();

return nvmSUCCESS;
}


int32_t nvmNetStart(
	nvmNetVM *NetVM,
	nvmRuntimeEnvironment *RTObj,
	int32_t  UseJit,
	uint32_t JitFlags,
	uint32_t OptLevel,
	char *Errbuf)
{

	uint32_t res = nvmSUCCESS;

	useJIT_flag= UseJit;

	if (RTObj->execution_option == nvmRUNTIME_COMPILEONLY)
	{
		return nvmSUCCESS;
	}

	if (SLLst_Iterate_1Arg(NetVM->NetPEs, (nvmIteratefunct1Arg *)nvmExecute_Init, Errbuf) == nvmFAILURE)
			return nvmFAILURE;

	if (!UseJit)
	{
#ifdef _DEBUG
		VerbOut(RTObj, 0, "Verifying the NetVM Application\n");
#endif

#ifdef CODEVERIFY_PROFILING
	#define CODEVERIFY_RUNS_PER_SAMPLE 1
	#define CODEVERIFY_SAMPLES CODEXEC_SAMPLES
#endif

#ifdef CODEVERIFY_PROFILING
		{
		int i;
                AllocateProfilerExecTime();

			for (i=0; i < CODEVERIFY_SAMPLES; i++)
			{
				int j;
                                uint64_t start, end;

                                start = nbProfilerGetTime();
				for (j= 0; j < CODEVERIFY_RUNS_PER_SAMPLE; j++)
				{
#endif
					res = nvmVerifyApplication(NetVM, RTObj, JitFlags | nvmDO_NATIVE, OptLevel,  NULL, Errbuf);

#ifdef CODEVERIFY_PROFILING
				}
                                end = nbProfilerGetTime();
                                ProfilerStoreSample(start, end);
			}

                        ProfilerProcessData();
			printf("\n======================= Code Verify Time profiling ======================\n");

                        ProfilerPrintSummary();
                        ProfilerExecTimeCleanup();
		}
#endif
	}


	if (UseJit)
	{
#ifdef ENABLE_NETVM_LOGGING
		logdata(LOG_RUNTIME_CREATE_PEGRAPH, "Using the jit");
#endif

#ifdef _DEBUG
		VerbOut(RTObj, 0, "JIT Compiling the NetVM Application\n");
#endif
		res = nvmNetCompileApplication(NetVM, RTObj, 0 /* First backend available */, JitFlags | nvmDO_NATIVE, OptLevel, NULL, Errbuf);
	}

	return res;
}


//FULVIO per OLIVIER: qui ci sono un po' di funzioni che usano ErrBuf, ma non viene passato come parametro il size del buffer, quindi
// si rischia un overflow
int32_t nvmCompileApplication(
	nvmNetVM *NetVM,
	nvmRuntimeEnvironment *RTObj,
	uint32_t BackendID,
	uint32_t JitFlags,
	uint32_t OptLevel,
	const char* OutputFilePrefix,
	char *ErrBuf)
{

	uint32_t res = nvmFAILURE;
	useJIT_flag= 1;

	if (RTObj->execution_option == nvmRUNTIME_COMPILEANDEXECUTE)
	{
		return nvmSUCCESS;
	}

	if (BackendID > TARGETS_NUM)
	{
		//FULVIO per OLIVIER: non sarebbe meglio un errorsnprintf , che ritorna anche l'errore nel buffer passato dal chiamante?
		errorprintf(__FILE__, __FUNCTION__, __LINE__, "Wrong target index\n");
		return nvmFAILURE;
	}

	res = nvmNetCompileApplication(NetVM, RTObj, BackendID, JitFlags, OptLevel, OutputFilePrefix, ErrBuf);

	return res;
}


int32_t nvmExecute_Init(nvmNetPE *PE, char * errbuf)
{
	if (arch_Execute_Handlers(NULL , 0, PE->InitHandler ->HandlerState) == nvmFAILURE)
		return nvmFAILURE;


	return nvmSUCCESS;
}


int32_t nvmWriteExBuff(nvmExchangeBuffer *exbuf, uint32_t port,  nvmHandlerState *HandlerState)
{
uint32_t ctdPort;
//#ifdef RTE_PROFILE_COUNTERS

//	arch_WriteExBuff(exbuf,port,HandlerState);
//#endif
//todo profiling stop HandlerState

#ifdef RTE_PROFILE_COUNTERS
//	HandlerState->ProfCounters->TicksEnd= nbProfilerGetTime();
//	HandlerState->ProfCounters->NumTicks += HandlerState->ProfCounters->TicksEnd - HandlerState->ProfCounters->TicksStart - HandlerState->ProfCounters->TicksDelta;

//	HandlerState->temp->TicksStart= nbProfilerGetTime();
#endif

	nvmHandlerFunction *f = HandlerState->PEState->ConnTable[port].CtdHandlerFunct;
	nvmHandlerState * h   =HandlerState->PEState->ConnTable[port].CtdHandler;
	ctdPort =HandlerState->Handler->OwnerPE->PortTable[port].CtdPort;

//todo profiling start HandlerState->PEState->ConnTable[port].CtdHandler

#ifdef RTE_PROFILE_COUNTERS
//	HandlerState->temp->TicksEnd= nbProfilerGetTime();
//	HandlerState->temp->NumTicks += HandlerState->temp->TicksEnd - HandlerState->temp->TicksStart - HandlerState->temp->TicksDelta;
//	HandlerState->PEState->ConnTable[port].CtdHandler->ProfCounters->TicksDelta= arch_SampleDeltaTicks();
//	HandlerState->PEState->ConnTable[port].CtdHandler->ProfCounters->TicksStart= nbProfilerGetTime();
#endif

	f(&exbuf,ctdPort,h);

	//HandlerState->PEState->ConnTable[port].CtdHandlerFunct(&exbuf,ctdPort,HandlerState->PEState->ConnTable[port].CtdHandler);
	return nvmSUCCESS;
}

int32_t nvmNetPacket_Recive(nvmExchangeBuffer **exbuf,   uint32_t port,  nvmHandlerState *HandlerState)
{
	return arch_ReadExBuff(exbuf, port,HandlerState);
}


nvmExchangeBuffer *nvmGetExbuf(nvmRuntimeEnvironment *RTObj)
{
nvmExchangeBuffer *exbuf;

	exbuf= arch_GetExbuf(RTObj);

	return exbuf;
}

void nvmReleaseExbuf(nvmRuntimeEnvironment *RTObj, nvmExchangeBuffer *exbuf)
{
	arch_ReleaseExbuf(RTObj, exbuf);
}

nvmRESULT nvmNetPacketSendDup (nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState)
{
nvmExchangeBuffer *new;

	new= arch_GetExbuf(HandlerState->PEState->RTEnv);
	memcpy(new, exbuf, sizeof(nvmExchangeBuffer));

	return arch_WriteExBuff(new, port, HandlerState);
}


uint32_t nvmRuntimeHasStats(void)
{
#ifdef RTE_PROFILE_COUNTERS
	return 1;
#else
	return 0;
#endif
}

nvmNetPEHandlerStats *nvmGetPEHandlerStats(nvmRuntimeEnvironment *RTObj, char *errbuf)
{
#ifdef RTE_PROFILE_COUNTERS
	SLLstElement *currListItem = RTObj->HandlerStates->Head;
	nvmHandlerState *HandlerState = NULL;
	nvmNetPEHandlerStats *currStat = NULL;
	nvmNetPEHandlerStats *lastStat = NULL;
	nvmNetPEHandlerStats *head = NULL;
	uint32_t	i = 0;
	char *handlerName;
	while (currListItem != NULL)
	{

		HandlerState = (nvmHandlerState*)currListItem->Item;
		if (HandlerState->Callback !=NULL || HandlerState->CtdInterf != NULL)
			break;

		if (HandlerState->Handler->HandlerType != INIT_HANDLER)
		{
			switch(HandlerState->Handler->HandlerType)
			{
				case PUSH_HANDLER:
					handlerName = "PUSH";
					break;
				case PULL_HANDLER:
					handlerName = "PULL";
					break;
			}
			currStat = arch_AllocRTObject(RTObj, sizeof(nvmNetPEHandlerStats), errbuf);
			if (currStat == NULL)
				return NULL;
			if (i == 0)
				head = currStat;
			//ultoa(i, currStat->PEName, 10);
			currStat->PEName[0] = '\0';
			currStat->HandlerName = handlerName;
			currStat->NumPkts = HandlerState->ProfCounters->NumPkts;
			currStat->NumPktsFwd = HandlerState->ProfCounters->NumFwdPkts;
			currStat->NumTicks = HandlerState->ProfCounters->NumTicks;

			currStat->Next = NULL;
			if (lastStat != NULL)
				lastStat->Next = currStat;
			lastStat = currStat;
			i++;
		}
		currListItem = currListItem->Next;
	}
	return head;
#else
	errsnprintf(errbuf, nvmERRBUF_SIZE, "Handler statistics are not available");
	return NULL;
#endif
}


void nvmPrintStatistics(nvmRuntimeEnvironment *RTObj)
{
#ifndef CODE_PROFILING

#ifdef RTE_PROFILE_COUNTERS
	printf("\nNetVM profiling\n");
	printf("=================================================================\n");
	SLLst_Iterate_1Arg(RTObj->HandlerStates, (nvmIteratefunct1Arg *)nvmStatistics,RTObj);
    SLLst_Iterate_1Arg(RTObj->ApplInterfaces,(nvmIteratefunct1Arg *)nvmAppStatistics,RTObj);
	printf("=================================================================\n");
	//printf("Somma dei ticks di tutti gli handler: %llu \n",RTObj->Tot->NumTicks);
#endif

#endif
}

#ifdef RTE_PROFILE_COUNTERS
nvmRESULT nvmAppStatistics (nvmAppInterface *AppInterface, nvmRuntimeEnvironment *RTObj)
{
	if (AppInterface->InterfDir == INTERFACE_DIR_IN)
	printf("Ticks spent in framework: %Lu\n",AppInterface->ProfCounterTot->NumTicks);
	return nvmSUCCESS;
}
nvmRESULT nvmStatistics (nvmHandlerState *HandlerState, nvmRuntimeEnvironment *RTObj)
{
unsigned int i= 0;
int j=0, max_op=0;

if (HandlerState->CtdInterf != NULL)
		return nvmSUCCESS;

	if (HandlerState->Callback != 0)
	{
		printf("Ticks spent in callback: %llu\n", HandlerState->ProfCountersCallb->NumTicks );
		//printf("Num of ticks for wrapper callback : %llu\n", HandlerState->ProfCounters->NumTicks );
		//printf("Num of ticks del pezzo temp : %llu\n", HandlerState->temp->NumTicks );
		//printf("Delta ticks : %llu\n", HandlerState->ProfCounters->TicksDelta );
	}
	else if (HandlerState->Handler->HandlerType == PUSH_HANDLER )
	{
		printf("Ticks spent in PE %s : %llu\nNumber of coprocessor for PE %s : %d\n", HandlerState->Handler->OwnerPE->Name, HandlerState->ProfCounters->NumTicks,HandlerState->Handler->OwnerPE->Name, HandlerState->PEState->NCoprocRefs);
		for (i=0; i < HandlerState->PEState->NCoprocRefs; i++)
		{
			printf("Ticks spent in coprocessor %s : %llu\n", HandlerState->PEState->CoprocTable[i].name, HandlerState->PEState->CoprocTable[i].ProfCounter_tot->NumTicks);

			if (strcmp("lookup",HandlerState->PEState->CoprocTable[i].name )== 0)
				max_op=5;
			else
				max_op=4;
			printf("Number of operation for coprocessor %s :%d\n ",HandlerState->PEState->CoprocTable[i].name,max_op);
			for (j=0;j<max_op; j++)
				{
					printf("operation:%d ticks: %llu\n ",j,HandlerState->PEState->CoprocTable[i].ProfCounter[j]->NumTicks  );
				}
			//	printf("\n");
		}
		printf("\n");
	//	printf("Num pkt in: %lu\n", HandlerState->ProfCounters->NumPkts );
	//	printf("Num pkt out: %lu\n", HandlerState->ProfCounters->NumFwdPkts );
	//	printf(" %llu\n", HandlerState->ProfCounters->NumTicks );
		//printf("Num of ticks del pezzo temp : %llu\n", HandlerState->temp->NumTicks );
		//printf("Delta ticks : %llu\n", HandlerState->ProfCounters->TicksDelta );
		RTObj->Tot->NumTicks += HandlerState->ProfCounters->NumTicks;
		//for (i=0; i < HandlerState->PEState->NCoprocRefs; i++)
		//{
		//	printf("Coprocessore %s : %llu\n",HandlerState->PEState->CoprocTable[i].name, HandlerState->PEState->CoprocTable[i].ProfCounter->NumTicks );
		//}
	}
	else if (HandlerState->Handler->HandlerType == PULL_HANDLER )
	{
		//printf("\nHandler Pull: no statistic\n");
	//	printf("Num pkt in: %lu\n", HandlerState->ProfCounters->NumPkts );
	//	printf("Num pkt out: %lu\n", HandlerState->ProfCounters->NumFwdPkts );
	//	printf("Num of ticks : %lu\n", HandlerState->ProfCounters->NumTicks );
	}

return nvmSUCCESS;
}

#endif

//EXPORT

void nvmDestroyRTEnv(nvmRuntimeEnvironment *RTObj)
{
#ifdef RTE_PROFILE_COUNTERS
	nvmPrintStatistics(RTObj);
#endif
	if (RTObj->TargetCode)
		free(RTObj->TargetCode);
	arch_ReleaseRTObject(RTObj);
}


void nvmDestroyRTEnvFromInt(nvmAppInterface *interface)
{
//TODO
#ifdef RTE_PROFILE_COUNTERS
	nvmPrintStatistics(interface->RTEnv);
#endif
}

void VerbOut(nvmRuntimeEnvironment *RTObj, uint32_t level, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	if (level < RTObj->VerbosityLevel)
		vfprintf(RTObj->VerboseOutput, format, args);
	va_end(args);
}



char *nvmGetTargetCode(nvmRuntimeEnvironment *RTObj)
{
	return RTObj->TargetCode;
}


uint32_t nvmHash(uint8_t *data, uint8_t len)
{
	uint32_t hash = len, tmp = 0;
	int rem = 0;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 2;
    hash += hash >> 15;
    hash ^= hash << 10;

    return hash;
}
