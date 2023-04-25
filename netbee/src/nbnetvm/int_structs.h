/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "utils/lists.h"
#include "rt_environment.h"

#ifdef __cplusplus
extern "C" {
#endif


// TODO [OM] comment all these macros and structures!

//!NetIL Handler Kinds
typedef enum
{
	PULL_HANDLER,
	PUSH_HANDLER,
	INIT_HANDLER,
} HandlerKind;

typedef uint32_t nvmPORT_FLAGS;
/*
struct _nvmNetVM;
struct _nvmNetPE;
struct _nvmSocket;
struct _nvmPEPort;
struct _nvmPEHandler;
struct _nvmExchBuffer;

typedef struct _nvmNetVM nvmNetVM;
typedef struct _nvmNetPE nvmNetPE;
typedef struct _nvmSocket nvmSocket;
typedef struct _nvmPEPort nvmPEPort;
typedef struct _nvmPEHandler nvmPEHandler;
typedef struct _nvmExchBuffer nvmExchBuffer;
*/

#define FLAG_CLEAR(fl, bitm) (fl &= ~(bitm))
#define FLAG_SET(fl, bitm)	(fl |= bitm)

#define PORT_TYPE_FLAG		0x00000001
#define PORT_DIR_FLAG		0x00000002
#define PORT_CONN_FLAG		0x0000000c
#define PORT_DIR(fl)		(fl & PORT_DIR_FLAG)
#define PORT_TYPE(fl)		(fl & PORT_TYPE_FLAG)
#define PORT_CONN(fl)		(fl & PORT_CONN_FLAG)
#define PORT_TYPE_COLLECTOR	0x00000000
#define PORT_TYPE_EXPORTER	PORT_TYPE_FLAG
#define PORT_DIR_PUSH		0x00000000
#define PORT_DIR_PULL		PORT_DIR_FLAG
#define PORT_CONN_PE		0x00000004
#define PORT_CONN_SOCK		0x00000008
#define PORT_CONN_NOTHING	0x00000000


#define PORT_IS_COLLECTOR(fl) (PORT_TYPE(fl) == PORT_TYPE_COLLECTOR)
#define PORT_IS_EXPORTER(fl) (PORT_TYPE(fl) == PORT_TYPE_EXPORTER)
#define PORT_IS_DIR_PUSH(fl)	(PORT_DIR(fl) == PORT_DIR_PUSH)
#define PORT_IS_DIR_PULL(fl) (PORT_DIR(fl) == PORT_DIR_PULL)
#define PORT_IS_CONN_PE(fl) (PORT_CONN(fl) == PORT_CONN_PE)
#define PORT_IS_CONN_SOCK(fl) (PORT_CONN(fl) == PORT_CONN_SOCK)
#define PORT_IS_CONN_NOTHING(fl) (PORT_CONN(fl) == PORT_CONN_NOTHING)
#define PORT_SET_TYPE_COLLECTOR(fl) (FLAG_CLEAR(fl, PORT_TYPE_FLAG))
#define PORT_SET_TYPE_EXPORTER(fl) (FLAG_SET(fl, PORT_TYPE_FLAG))
#define PORT_SET_DIR_PUSH(fl) (FLAG_CLEAR(fl, PORT_DIR_FLAG))
#define PORT_SET_DIR_PULL(fl) (FLAG_SET(fl, PORT_DIR_FLAG))
#define PORT_SET_CONN_NOTHING(fl) (FLAG_CLEAR(fl, PORT_CONN_FLAG))
#define PORT_SET_CONN_PE(fl) (FLAG_SET(fl, PORT_CONN_PE))
#define PORT_SET_CONN_SOCK(fl) (FLAG_SET(fl, PORT_CONN_SOCK))


//TODO [OM for MB]: are you sure these macros should be kept here?

//!\brief Macros for bytecode pointers and arguments

//!Macro that gets a LONG given its address in memory
#define PTR_TO_LONG(ADDR)	(*(int32_t *)(ADDR))

//!Macro that gets the BYTE argument of a NetIL Instruction in the bytecode
#define BYTE_ARG(PC)	(*(int8_t *)(bytecode + PC + 1))	//!< the code at bytecode[pc+1] is treated as a char argument

//!Macro that gets the SHORT argument of a NetIL Instruction in the bytecode
#define SHORT_ARG(PC)	(*(int16_t *)(bytecode + PC + 1))	//!< the 2 codes at bytecode[pc+1] are treated as a short int argument

//!Macro that gets the LONG argument of a NetIL Instruction in the bytecode
#define LONG_ARG(PC)	(*(int32_t *)(bytecode + PC + 1))	//!< the 4 codes at bytecode[pc+1] are treated as a long int argument


/*!\brief Macros for absolute branch targets
	*
	* (+3) and (+5) are because the address argument of a branch is relative to the next
	* opcode, so for the translation to an absolute address in the bytecode we have to do
	* this kind of adjustment
*/
//!Macro that gets the index in the bytecode of a short jump or branch target
#define SHORT_JMP_TARGET(PC)	(BYTE_ARG(PC) + PC + 3)
//!Macro that gets the index in the bytecode of a long jump or branch target
#define LONG_JMP_TARGET(PC)		(LONG_ARG(PC) + PC + 5)

/* Maximum length of a PE name. If you change this, change it also in compiler.h */
#define MAX_NETPE_NAME 64

/* Code segments */
#define SEGMENT_HEADER_LEN	8		//!< 12 bytes, see below
#define MAX_STACK_SIZE_OFFS	0		//!< displacement in the bytecode of the Max Stack Size
#define LOCALS_SIZE_OFFS	4		//!< displacement in the bytecode of the Locals Size

/* This is for the .port segment */
#define PORT_TABLE_OFFS		4		//!< displacement in the bytecode of the port table


#define EXTRACT_BC_SIGNATURE(dst,src)	do{strncpy( (dst) , (src) , 4 ); (dst)[4]='\0';}while(0)

enum InterfaceTypes
{
	APPINTERFACE,
	PHYSINTERFACE

};

struct _nvmNetVM
{
	SLinkedList *NetPEs;
	SLinkedList *Sockets;
};


struct _nvmPEPort
{
	union
	{
		nvmNetPE	*CtdPE;
		nvmSocket	*CtdSocket;
	};
	uint32_t		CtdPort;
	nvmNetPE		*OwnerPE;
	nvmPEHandler	*Handler; //handler del pe in uso nello state c'e' l'handler collegato
	nvmPORT_FLAGS	PortFlags;
};


struct _nvmSocket
{
	nvmNetVM	*OwnerVM;
	nvmNetPE	*CtdPE;
	uint32_t	CtdPort;
	uint32_t	InterfaceType;
	union
	{
		nvmAppInterface	*AppInterface;
		nvmPhysInterface	*PhysInterface;
	};

};


struct _nvmAppInterface
{
	uint32_t 	InterfDir;
//	union _CtdHandlerState
//	{
//		nvmHandlerState		*CtdHandler;	//!< Connected Handler
//		nvmCallBackFunct	*CtdCallback;	//!< Connected Callback function
//		nvmPollingFunct		*CtdPolling;	//!< Connected Polling function
//	}Handler;
	CtdHandlerState Handler;
	nvmHandlerState		*CtdHandler;	//!< Connected Handler
	uint32_t			CtdHandlerType;	//!< Type of the connected handler (one element of the \ref nvmHandlerTypes enumeration)
	nvmRuntimeEnvironment	*RTEnv;
	uint32_t				CtdPort;
	//aggiungere la ctdhandlerfunct e registarrla nella bind la funz interprete e nel compilebytecode la funz del jit
#ifdef RTE_PROFILE_COUNTERS
  	nvmCounter	*ProfCounterTot;		//!< Profiling counters
#endif

};


/*!
  \brief Descriptor of a NetVM handler: {.init, .push, .pull}
*/
struct _nvmPEHandler
{

	char			*Name;			//!< The name of the bytecode segment
	uint8_t		*ByteCode;		//!< Pointer to the bytecode of the handler
	uint32_t		CodeSize;		//!< Size of the bytecode segment in bytes
	uint32_t		NumLocals;		//!< Number of Locals
	uint32_t		MaxStackSize;	//!< Maximum Stack Size allowed
	HandlerKind		HandlerType;	//!< Kind of handler.
	nvmNetPE		*OwnerPE;
	nvmHandlerState	*HandlerState;
	uint8_t		*Insn2LineTable;
	uint32_t		Insn2LineTLen;

	uint16_t		start_bb;		//!< TEMPORARY FIXME basic block where this PE starts
};


struct _nvmNetPE
{
	char			Name[MAX_NETPE_NAME];
	char			FileName[MAX_NETPE_NAME];
	uint32_t		NPorts;
	nvmPEPort		*PortTable;
	nvmPEHandler	*InitHandler;
	nvmPEHandler	*PushHandler;
	nvmPEHandler	*PullHandler;
	nvmNetVM		*OwnerVM;
	tmp_nvmPEState	*PEState;
	uint32_t		*Copros;
	uint32_t		NCopros;
	uint32_t		DataMemSize;
	uint8_t		*InitedMem;
	uint32_t		InitedMemSize;
	uint32_t		InfoPartitionSize;
#if 0
	void			*SymTable;
	void			*ConstPool;
#endif
};



#ifdef __cplusplus
}
#endif

