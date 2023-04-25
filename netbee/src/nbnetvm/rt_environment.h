/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "./utils/lists.h"
#include "../nbee/globals/debug.h"
#include "coprocessor.h"
#include <stdlib.h>
#include "netvmprofiling.h"

extern int32_t useJIT_flag;

struct _nvmMemDescriptor;
struct _nvmHandlerState;
struct _nvmPortState;
struct _nvmPEState;
struct _nvmTargetInfo;
struct _nvmExbufPool;
typedef struct _nvmMemDescriptor nvmMemDescriptor;
typedef struct _nvmPortState nvmPortState;
typedef struct _nvmPEState tmp_nvmPEState;
typedef struct _nvmTargetInfo nvmTargetInfo;
typedef struct _nvmExbufPool nvmExbufPool;
//typedef struct _nvmCounterTot nvmCounterTot;





/*! \addtogroup RuntimeInternalStructs
	\{
*/

/*!
 \brief There are different kind of function that you can connect to a handler
*/

typedef	union _CtdHandlerState
	{
		nvmHandlerFunction	*CtdHandlerFunct;//!< Interpreter function or Jit function
		nvmCallBackFunct	*CtdCallback;	//!< Connected Callback function
		nvmPollingFunct		*CtdPolling;	//!< Connected Polling function
	}CtdHandlerState;




/*!
	\brief This structure holds information about a NetVM memory (PE data-mem, or shared-mem) allocated at runtime

	Since some platforms provide several kinds of physical memory, the Flags member can used by the target back-end in order to recognize where
	the buffer is actually allocated
*/
struct _nvmMemDescriptor
{
	uint8_t	*Base;	//!< Base address of the allocated buffer
	uint32_t	Size;	//!< Size of the allocated buffer
	uint32_t	Flags;	//!< Target specific flags
};


/*!
	\brief Enumeration of runtime handler types
*/
enum nvmHandlerTypes
{
	HANDLER_TYPE_HANDLERFUNCT,  //!< The handler is a interpreter o jit function
	HANDLER_TYPE_CALLBACK,		//!< The handler is a callback function
	HANDLER_TYPE_POLLING,		//!< The handler is a polling function
	HANDLER_TYPE_INTERF_OUT,	//!< The handler is an interface output writing function
	HANDLER_TYPE_INTERF_IN		//!< The handler is an interface input reading function
};

enum nvmPktTypes
{
	FORWARDED,
	DISCARDED,
	START
};
/*
struct _nvmCounterTot{
	uint64_t LenghtTotHandlersPush;
	uint64_t LenghtTotHandlersPull;
	uint64_t LenghtTotHandlersInit;
 };
*/

/*!
	\brief This structure holds information associated to a physicalal interface
*/
struct _nvmPhysInterface
{
	CtdHandlerState Handler;		//!<Connected function
	nvmHandlerState		*CtdHandler;	//!< Connected Handler
	nvmRuntimeEnvironment *RTEnv;		//!<Relative RT object
	uint32_t 	CtdPort;				//!Connected port
	nvmPhysInterfaceInfo	*PhysInterfInfo;	//!< (Reserved) Target specific interface descriptor
	void 	*ArchData;
};


/*!
	\brief This structure holds information about PE ports at runtime

	This structure is mainly used as a caching mechanism in order to reduce the levels of indirection necessary for getting the next code handler to be
	executed
*/

struct _nvmPortState
{

//la union sparisce al posto suo rimane ctdhandlerfunct e basta
	union
	{
		nvmCallBackFunct	*CtdCallback;	//!< Connected Callback function
		nvmPollingFunct		*CtdPolling;	//!< Connected Polling function
		nvmHandlerFunction *CtdHandlerFunct;//!<Connected handler function (interpreter or jit)
	};
	nvmHandlerState		*CtdHandler;	//!< Connected Handler
	//aggiungere union con genericinterface (physic-app) e ctdhandler
	//
	uint32_t			CtdHandlerType;	//!< Type of the connected handler (one element of the \ref nvmHandlerTypes enumeration)
	nvmPhysInterface	*CtdInterf;		//!< Connected interface (if applicable)
};

/*!
	\brief This structure holds the state associated to a runtime code handler of a PE
*/
struct _nvmHandlerState
{
	nvmPEHandler *Handler;		//!< PE handler corresponding to a .push, .pull, or .init NetIL segment
	uint32_t	*Locals;			//!< Locals pool
	uint32_t	NLocals;			//!< Number of locals
	uint32_t	*Stack;				//!< Operands stack
	uint32_t	StackSize;			//!< Size of the stack
	tmp_nvmPEState *PEState;			//!< Reference to the PE State
#ifdef RTE_PROFILE_COUNTERS
	nvmCounter	*ProfCounters;		//!< Profiling counters
	nvmCounter	*temp;		//!< Profiling counters
	nvmCounter	*ProfCountersCallb;		//!< Profiling counters
#endif
#ifdef RTE_DYNAMIC_PROFILE
  FILE			*MemDataFile;
  uint64_t		t;
#endif

  nvmCallBackFunct *Callback;

	nvmPhysInterface	*CtdInterf;		//!< Connected interface (if applicable)

};


/*!
	\brief This structure holds the state associated to a PE
*/
struct _nvmPEState
{
	nvmMemDescriptor	*DataMem;		//!< Data-memory descriptor
	nvmMemDescriptor	*ShdMem;		//!< Shared-memory descriptor
	nvmMemDescriptor	*InitedMem;		//!< Initialized-memory descriptor
	nvmCoprocessorState	*CoprocTable;	//!< Coprocessor Table
	uint32_t			NCoprocRefs;	//!< Number of coprocessor objects referenced by the PE
	nvmPortState		*ConnTable;		//!< Connection Table
	uint32_t			Nports;			//!< Number of ports
	nvmRuntimeEnvironment *RTEnv;


#if 0
	void			*SymTable;
	void			*ConstPool;
#endif
};


/*!
	\brief This structure holds the pool of exchange buffer that the runtime object allocates
*/

struct _nvmExbufPool
{
	nvmExchangeBuffer	**Pool;		//!< LIFO array of exchange buffers
	uint8_t			*PktData;	//!< Packet Data memory chunks
	uint8_t			*InfoData;	//!< Info Data memory chunks
	uint32_t			Size;		//!< Size of the pool (i.e. number of available ExBufs)
	uint32_t			Top;		//!< Top of the stack
	uint32_t			PacketLen;	//!< Default packet-len
	uint32_t			InfoLen;	//!< Default info-len
};

/*!
	\brief This structure holds the state associated to a NetVM
*/

struct _nvmRuntimeEnvironment
{

	SLinkedList			*PEStates;	//!<List of PEState
	SLinkedList			*HandlerStates;	//!<List of HandlerState
	SLinkedList			*PhysInterfaces;//!<List of physic interface
	SLinkedList			*ApplInterfaces;//!<List of application interface
	nvmExbufPool		*ExbufPool;		//!<Pool of exchenge buffer
	uint32_t			VerbosityLevel; //!<Loudness of the output messages
	FILE				*VerboseOutput;
	SLinkedList			*AllocData;		//!<(RESERVED)
	void				*ArchData;		//!<used for keeping architecture specific information
	uint32_t			execution_option;
	char 				*TargetCode;
#ifdef RTE_PROFILE_COUNTERS
  	nvmCounter			*Tot;		//!< Profiling counters
#endif
};

#ifdef _WIN32
#ifdef _WIN64
#define MMSET(ptr,zero,length) { int mmsetCounter=0; \
   for(; mmsetCounter < length; mmsetCounter++) ptr[mmsetCounter] = zero; \
}
#else
#define MMSET(ptr,zero,lenght) __asm{\
							__asm mov ecx, lenght\
							__asm mov al, zero\
							__asm mov edi, ptr\
							__asm rep stosb\
							}
#endif
#else
#ifdef __amd64__
#define MMSET(ptr,zero,len) asm("mov %0,%%ecx; mov $0, %%eax; movq %1, %%rdi; rep stosb"\
   		: \
        : "r"(len),"r"(ptr)\
        : "ecx", "eax","rdi");
#else
#define MMSET(ptr,zero,len) asm("mov %0,%%ecx; mov $0, %%eax; mov %1, %%edi; rep stosb"\
   		: \
        : "r"(len),"r"(ptr)\
        : "ecx", "eax","edi");

#endif

#endif




/*!
 	\brief Function that creates the PEStates associated to a PE 1:1 in a RT
	\param PE relative to the pestate you want to create
	\param RT object
	\param	pointer to size of data
	\param	pointer to size of shd
	\param	error buffer
	\return	nvmSUCCESS or nvmFAILURE
*/
int32_t  nvmCreatePEStates(nvmNetPE *PE,nvmRuntimeEnvironment *RTObj, uint32_t *shd_size, char *errbuf);
/*!
 	\brief Function that creates the PortStates
	\param RT object
	\param	error buffer
	\return	nvmSUCCESS or nvmFAILURE
*/
nvmPortState * nvmCreatePortState(nvmRuntimeEnvironment *RTObj,char *errbuf);
/*!
 	\brief Function that creates the HandlerStates, one for each Handler
*/
int32_t nvmCreateHandlerStates(nvmRuntimeEnvironment *RTObj,nvmNetPE *PE, nvmPEHandler *xHandler, tmp_nvmPEState *PEState,char *errbuf);
/*!
 	\brief Functions that creates the ConnectTable for a PEStates, CreateConnectTable iterate on the PEState list and call the FindHandlerState
*/
int32_t nvmCreateConnectTable(tmp_nvmPEState *PEState,nvmRuntimeEnvironment *RTObj,char *errbuf);
/*!
 	\brief This function is called by the nvmCreateConnectTable. You must use this 2 function together
 */
int32_t nvmFindHandlerState(nvmHandlerState *HandlerState, tmp_nvmPEState *PEState, char *errbuf);
/*!
 	\brief Function that is called by the PE's push netil code
 */
//int32_t nvmNetPacket_Send(nvmExchangeBuffer *exbuf, uint32_t port,  nvmHandlerState *HandlerState);
#define nvmNetPacket_Send nvmWriteExBuff
//((EXBUF), (PORT), (HSTATE))


/*!
 	\brief Function that destroy the RT
*/
//void nvmDestroyRTEnv(nvmRuntimeEnvironment *RTObj);//TODO devo ancora implementarla
/*!
 	\brief Function that executes the Init Handler for a PE, called by the nvmNetStart
*/
int32_t nvmExecute_Init(nvmNetPE *PE, char * errbuf);


nvmRESULT nvmNetPacketSendDup (nvmExchangeBuffer *exbuf, uint32_t port, nvmHandlerState *HandlerState);


/*!
 	\brief Function that is called by the PE's pull netil code
 */
int32_t nvmNetPacket_Recive(nvmExchangeBuffer **exbuf,   uint32_t port,  nvmHandlerState *HandlerState);


/*!
 	\brief Function that prints statistics for one handler
 */
nvmRESULT nvmStatistics (nvmHandlerState *HandlerState, nvmRuntimeEnvironment *RTObj/* uint64_t LenghtTotHandlersPush,uint64_t LenghtTotHandlersPull,uint64_t LenghtTotHandlersInit*/);

nvmRESULT nvmAppStatistics (nvmAppInterface *AppInterface, nvmRuntimeEnvironment *RTObj);


int32_t nvmOutCallback(nvmExchangeBuffer **exbuf, uint32_t dummy, nvmHandlerState *HandlerState);


/*!
 	\brief Function for printing messages at the desired verbosity level
 */
void VerbOut(nvmRuntimeEnvironment *RTObj, uint32_t level, const char *format, ...);


uint32_t nvmHash(uint8_t *data, uint8_t len);

int32_t nvmWriteExBuff(nvmExchangeBuffer *exbuf, uint32_t port,  nvmHandlerState *HandlerState);


/** \} */

#ifdef __cplusplus
}
#endif

