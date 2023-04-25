/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*!
	\file nbnetvm.h
	\brief This file contains prototypes, definitions and data structures exported by the NetVM framework.
*/




/*!
	\defgroup ExportedFunc		NetVM Exported Functions
*/

/*!
	\defgroup ExportedStruct		NetVM Exported Structures
*/


/*! \mainpage NetVM Exported Interface
 *
 *  \section intro Introduction
 *
 *  This document describes data structures and functions that compose the API of the NetVM Framework.
 *
 *  The NetVM API allows to create and manage NetVM applications and to instantiate a Runtime Environment for their
 *  execution
 *
 *	This module includes the following sections:
 *  <ul>
 *	<li> \ref ExportedFunc
 *  <li> \ref ExportedStruct
 * </ul>
 *
 */


/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifdef NETVM_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif


#define ALLOC_FAILURE "MEMORY ALLOCATION FAILURE"

#define nvmERRBUF_SIZE 256			//!< Minimum size of the error message buffer
#define nvmTRUE		((uint32_t)1)	//!< Boolean True
#define nvmFALSE	((uint32_t)0)	//!< Boolean False


//! Highest bit of a value of 32 bits: sign bit
#define SIGN(a)		((a) & (1<<31))

//! Macro that checks if the bit corresponding to flag 'flag' is set in variable 'var'.
#define nvmFLAG_ISSET(var,flag) (flag & var)
//! Macro that set the bit corresponding to flag 'flag' in variable 'var'.
#define nvmFLAG_SET(var,flag) {var=var|flag;}


#if !defined (get16bits)
#define get16bits(d) ((((const uint8_t *)(d))[1] << 8)\
                      +((const uint8_t *)(d))[0])
#endif


/*************************** BYTE SWAPPING MACROS ***************************/
// WORDS_BIGENDIAN is defined by the AC_C_BIGENDIAN autoconf macro, in case. On Windows please define manually.
#ifdef WORDS_BIGENDIAN		//!< Swapping macros on big endian systems (Where host b. o. = network b. o.)
	/* This machine's order to big endian */
#define bo_my2big_16(a) ((uint16_t)(a))
#define bo_my2big_32(a) ((uint32_t)(a))

	/* This machine's order to little endian */
#define bo_my2little_16(a) ( \
		((((uint16_t) (a)) & 0x00FF) << 8) + \
		(((uint16_t) (a)) >> 8) \
	)
#define bo_my2little_32(a) ( \
	        ((((uint32_t) (a)) & 0x000000FF) << 24) + \
		((((uint32_t) (a)) & 0x0000FF00) << 8) + \
		((((uint32_t) (a)) & 0x00FF0000) >> 8) + \
		(((uint32_t) (a)) >> 24) \
	)
#else				//!< Swapping macros on little endian systems
	/* This machine's order to big endian */
#define bo_my2big_16(a) ( \
		((((uint16_t) (a)) & 0x00FF) << 8) + \
		(((uint16_t) (a)) >> 8) \
	)
#define bo_my2big_32(a) ( \
		((((uint32_t) (a)) & 0x000000FF) << 24) + \
		((((uint32_t) (a)) & 0x0000FF00) << 8) + \
		((((uint32_t) (a)) & 0x00FF0000) >> 8) + \
		(((uint32_t) (a)) >> 24) \
	)

	/* This machine's order to little endian */
#define bo_my2little_16(a) ((uint16_t)(a))
#define bo_my2little_32(a) ((uint32_t)(a))
#endif

/* These will be handy */
/* Big endian to this machine's order */
#define bo_big2my_16(x) bo_my2big_16(x)
#define bo_big2my_32(x) bo_my2big_32(x)

/* Little endian to this machine's order */
#define bo_little2my_16(x) bo_my2little_16(x)
#define bo_little2my_32(x) bo_my2little_32(x)

/* There are the most useful ones */
#define nvm_htons(x)	bo_my2big_16(x)
#define nvm_htonl(x)	bo_my2big_32(x)
#define nvm_ntohs(x)	nvm_htons(x)
#define nvm_ntohl(x)	nvm_htonl(x)

/************************ END OF BYTE SWAPPING MACROS ***********************/


/*! \addtogroup ExportedStruct
	\{
*/

typedef int32_t		nvmRESULT; 
typedef uint32_t	nvmBOOL;
typedef uint32_t	nvmNETVMFLAGS;

struct _nvmNetVM;
struct _nvmNetPE;
struct _nvmSocket;
struct _nvmPEPort;
struct _nvmPEHandler;
struct _nvmTStamp;
struct _nvmExchangeBuffer;
struct _nvmPEMemInfo;
struct _nvmRuntimeEnvironment;
struct _nvmPhysInterface;
struct _nvmAppInterface;
struct _nvmNetPEHandlerStats;
typedef struct _nvmNetVM nvmNetVM;
typedef struct _nvmNetPE nvmNetPE;
typedef struct _nvmSocket nvmSocket;
typedef struct _nvmPEPort nvmPEPort;
typedef struct _nvmPEHandler nvmPEHandler;
typedef struct _nvmTStamp nvmTStamp;
typedef struct _nvmExchangeBuffer nvmExchangeBuffer;
typedef struct _nvmRuntimeEnvironment nvmRuntimeEnvironment;
typedef struct _nvmPhysInterface nvmPhysInterface;
typedef struct _nvmAppInterface nvmAppInterface;
typedef struct _nvmHandlerState nvmHandlerState;
typedef	struct _nvmPhysInterfaceInfo nvmPhysInterfaceInfo;
typedef struct _nvmByteCode nvmByteCode; 
typedef struct _nvmNetPEHandlerStats nvmNetPEHandlerStats;
typedef int32_t (nvmHandlerFunct)(nvmExchangeBuffer *, uint32_t);
typedef int32_t (nvmPollingFunct)(nvmExchangeBuffer *, uint32_t);
typedef int32_t (nvmCallBackFunct)(nvmExchangeBuffer *);
typedef int32_t (nvmInterfOutFunct)(nvmExchangeBuffer *, uint32_t);
typedef int32_t (nvmInterfInFunct)(nvmExchangeBuffer *);
typedef int32_t (nvmHandlerFunction)(nvmExchangeBuffer **, uint32_t, nvmHandlerState *);	


//! Return Value.
typedef enum
{
	nvmSUCCESS		=  0,	//!< Return value to indicate that the operation was succesfull
	nvmFAILURE		= -1,	//!< Return value to indicate that the operation failed
	nvmSTACKEX		=  1, //!<Return value to indicate stack out-of-bound access
	nvmPKTEX		=  2, //!<Return value to indicate packet out-of-bound access
	nvmINFOEX 		=  3,  //!<Return value to indicate info out-of-bound access
	nvmDATAEX		=  4,  //!<Return value to indicate data out-of-bound access
	nvmCOPROCERR	=  5,  //!<Return value to indicate coprocessor check error
	nvmINITEDMEMERR	=  6,  //!<Return value to indicate initedmem out-of-bound access
	nvmJUMPERR		=  7,  //!<Return value to indicate jump error
} nvmRETValue;


//! Jit Flags; can select some options in the code emission for that backend
typedef enum
{
	nvmDO_ASSEMBLY = 0x1,  //!< The backend can do assembly emission
	nvmDO_NATIVE   = 0x2,  //!< The backend can do native code emission
	nvmDO_INLINE   = 0x4,  //!< The backend can do netPE inlining
	nvmDO_INIT	   = 0x8,  //!< The backend needs to compile also init segments
	nvmDO_BCHECK   = 0x10  //!< The backend can do boundschecking. Please note that bounds checking are implemented only if the optimization level is > 1.
} nvmJITFlags;


//! IDs of the supported native backends (not including the NetIL interpreter)
typedef enum nvmBackendID_t
{
	nvmBACKEND_X86= 1,		//!< BackedID associated to the Intel x86 (32 bits) platform
	nvmBACKEND_X64,			//!< BackedID associated to the Intel x64 (64 bits) platform
	nvmBACKEND_X11,			//!< BackedID associated to the Xelerated x11 platform
	nvmBACKEND_OCTEON,		//!< BackedID associated to the Cavium Octeon (64 bits) platform
	nvmBACKEND_OCTEONC,		//!< BackedID associated to the Cavium Octeon C compiler

} nvmBackendID_t;


//!Runtime instantation flags
typedef enum
{
	nvmRUNTIME_COMPILEONLY       = 0x1, //!<The runtime is for compilation only
	nvmRUNTIME_COMPILEANDEXECUTE = 0x2  //!<The runtime is for execution of jit functions
} nvmRuntimeOptions;


//! Contains some general information on a given compiler backend
typedef struct _nvmBackendDescriptor
{
	char *Name;				//!< Name of the backend target (e.g. x86, X11, octeon)
	nvmBackendID_t BackendID;		//!< Backend ID (e.g. x86, X11, octeon), according to the values provided in #nvmBackendID_t
	uint8_t MaxOptLevel; //!< Maximum optimization level allowed for this target
	uint32_t Flags;		//!< Flags specifying what the backend can do 
} nvmBackendDescriptor;


/*! 
	\brief Structure handling the runtime statistics relative to a PE Handler (push or pull)
*/
struct _nvmNetPEHandlerStats
{
	char		PEName[256];	//!< Name of the PE
	char		*HandlerName;	//!< Type of handler (push or pull)
	uint32_t	NumPkts;		//!< Total numer of packets processed during the execution of the handler
	uint32_t	NumPktsFwd;		//!< Number of forwarded packets
	uint64_t	NumTicks;		//!< Total numer of clock ticks elapsed during the execution of the handler
	nvmNetPEHandlerStats *Next;	//!< Pointer to the next element in the list
};

/*!
	\brief Generic interface Types
*/

enum nvmIntefDir
{
	INTERFACE_DIR_IN,		//!< Input interface
	INTERFACE_DIR_OUT		//!< Output interface
};

/*!
	\brief NetVM port types
*/

enum PortDirection
{
	nvmPORT_COLLECTOR = 1,	//!< Port that accepts data (e.g. packets) from the outside world.
	nvmPORT_EXPORTER = 2	//!< Port that sends data (e.g. packets) to the outside world.
};


/*!
	\brief NetVM Connection types
*/
enum ConnectionType
{
	nvmCONNECTION_PULL = 4,	//!< Pull connection (i.e. the packet transfer is initiated by a read operation)
	nvmCONNECTION_PUSH = 8	//!< Push connection (i.e. the packet transfer is initiated by a write operation)
};

/*!
	\brief This structure provides information about a physical interface
*/
struct _nvmPhysInterfaceInfo
{
	char		*Name;				//!< Interface name (target specific)
	char		*Description;		//!< Interface description (target specific)
	uint32_t   InterfDir;			//!< Interface type
	nvmPhysInterface	*PhysInterface;//!< Actual physical interface
	nvmPhysInterfaceInfo	*Next;	
};

/*!
	\brief Time stamp
*/
struct _nvmTStamp
{
	uint32_t	sec;			//!< Packet Timestamp (seconds)
	uint32_t	usec;			//!< Packet Timestamp (microseconds)
};

/*!
	\brief This structure represents an Exchange Buffer (i.e. the basic unit of information that flows between NetPEs)

	The exchange buffer can be considered as a meta-packet that flows between PEs and that triggers the execution of 
	a NetIL code handler. It is composed of two partitions, the first containing packet data and the second containing
	additional information (i.e. packet metadata), whose semantic must be defined by the user application.
	TimeStamp information ideally represents the time when the packet buffer contained in the exchange buffer has been 
	received on a physicalal interface
	The ArchData member has no particular meaning for the user, since it is used by the target-architecture dependent part
	of the runtime environment
*/

struct _nvmExchangeBuffer
{
	uint32_t	ID;					//!< Exchange Buffer ID
	uint8_t	*PacketBuffer;		//!< Packet buffer
	uint32_t	PacketLen;			//!< Packet buffer lenght
	uint8_t	*InfoData;			//!< Info (metadata) buffer
	uint32_t	InfoLen;			//!< Info buffer length
	uint32_t	TStamp_s;			//!< Packet Timestamp (seconds) C compiler
	uint32_t	TStamp_us;			//!< Packet Timestamp (microseconds)
	void		*UserData;			//!< Data for the callback
	void		*ArchData;			//!< (Reserved) Target specific information
};

//#define CODE_PROFILING


/*!
 	\}
*/

/*! \addtogroup ExportedFunc
	\{
*/


/*!
  \brief	Instantiate an object representing a NetVM application (i.e. a set of NetPEs and Sockets interconnected between them)
  \param	flags	NetVM Creation flags
  \param	ErrBuf		error buffer	
  \return	A pointer to the NetVM object
*/
DLL_EXPORT nvmNetVM *nvmCreateVM(nvmNETVMFLAGS flags, char* ErrBuf);

/*!
  \brief	Destroy the NetVM object as well as all its internal components (i.e. NetPEs and Sockets)
  \param	VMObj		Pointer to the NetVM object
  \return	nothing
*/
DLL_EXPORT void nvmDestroyVM(nvmNetVM *VMObj);



/*!
  \brief	Creates a NetPE into a NetVM Object by reading a bytecode image from file
  \param	VMObj		Pointer to the NetVM Object
  \param	filename	Path and name of the bytecode file	
  \param	ErrBuf		error buffer	
  \return	A pointer to the NetPE object 
*/
DLL_EXPORT nvmNetPE *nvmLoadPE(nvmNetVM *VMObj, char *filename, char* ErrBuf);

/*!
  \brief	Creates a NetPE inside a NetVM Object from a bytecode image residing in memory
  \param	VMObj		Pointer to the NetVM Object
  \param	bytecode	Pointer to the bytecode image
  \param	ErrBuf		error buffer	
  \return	A pointer to the NetPE object 
*/
DLL_EXPORT nvmNetPE *nvmCreatePE(nvmNetVM *VMObj, nvmByteCode *bytecode, char* ErrBuf);


/*!
  \brief	Associates a filename to a NetPE
  \param	netPE		Pointer to a NetPE object
  \param	filename	Filename
  \return	nothing
*/
DLL_EXPORT void nvmSetPEFileName(nvmNetPE *netPE, char *filename);

/*!
  \brief	Connects an output port of a NetPE to an input port of another NetPE
  \param	VMObj		Pointer to the NetVM Object
  \param	PE1			pointer to the first PE
  \param	port1       port of the first PE
  \param	PE2			pointer to the second PE
  \param	port2       port of the second PE
  \param	ErrBuf		error buffer	
  \return	A pointer to the NetPE object 
*/
DLL_EXPORT nvmRESULT nvmConnectPE2PE(nvmNetVM *VMObj, nvmNetPE *PE1, uint32_t port1, nvmNetPE *PE2, uint32_t port2, char *ErrBuf);

/*!
  \brief	Creates a nvmSocket object by registering it inside a NetVM Object 
  \param	VMObj		Pointer to the NetVM Object
  \param	ErrBuf		error buffer	
  \return	A pointer to the nvmSocket object 
*/
DLL_EXPORT nvmSocket *nvmCreateSocket(nvmNetVM *VMObj, char* ErrBuf);

/*!
  \brief	Connects a Socket to a port of a NetPE 
  \param	VMObj		Pointer to the NetVM Object
  \param	Socket		pointer to a Socket
  \param	PE			pointer to a PE
  \param	port		number of the port to be connected
  \param	ErrBuf			error buffer	
  \return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT nvmRESULT nvmConnectSocket2PE(nvmNetVM *VMObj, nvmSocket *Socket, nvmNetPE *PE, uint32_t port, char *ErrBuf);

/*!
  \brief  Compile a NetIL assembler program into bytecode. The source program is read from a memory buffer
  \param  NetVMAsmBuffer	Name of the buffer containing the NetIL program.
  \param  ErrBuf	error buffer
  \return A pointer to bytecode or NULL 
*/
DLL_EXPORT nvmByteCode *nvmAssembleNetILFromBuffer(char *NetVMAsmBuffer, char *ErrBuf);

/*!
  \brief  Compile a NetIL assembler program into bytecode. The source program is read from a file
  \param  FileName	Name of the text file containing the NetIL program.
  \param  ErrBuf	error buffer
  \return A pointer to bytecode or NULL 
*/
DLL_EXPORT nvmByteCode *nvmAssembleNetILFromFile(char *FileName, char *ErrBuf);

/*!
  \brief  Loads a bytecode image
  \param  FileName	Pointer to the file containing the bytecode.
  \param  ErrBuf	error buffer
  \return A pointer to bytecode or NULL 
*/
DLL_EXPORT nvmByteCode *nvmLoadBytecodeImage(char *FileName, char *ErrBuf);

/*!
  \brief  Save a bytecode image into a file.
  \param  bytecode	Pointer to the bytecode image
  \param  FileName	Name of the file that will be created.
  \param  ErrBuf	error buffer
  \return nvmSUCCESS if the file was created correctly.
*/
DLL_EXPORT int32_t nvmSaveBinaryFile(nvmByteCode *bytecode, char *FileName, char *ErrBuf);

/*!
  \brief	Destroy a nvmByteCode object.
  \param	bytecode	the object tou want to deallocate
*/
DLL_EXPORT void nvmDestroyBytecode(nvmByteCode *bytecode);

/*!
  \brief  Create a runtime environment object for the execution of a NetVM application
  \param  netVMApp		Pointer to the NetVM Object representing the application
  \param  flags			options for the Runtime Environment instantiation (e.g. execution or AOT compilation). Allowed values are in \ref nvmRuntimeOptions.
  \param  ErrBuf		error buffer
  \return A pointer to the nvmRuntimeEnvironment object 
*/
DLL_EXPORT nvmRuntimeEnvironment *nvmCreateRTEnv(nvmNetVM *netVMApp, uint32_t flags, char *ErrBuf);


/*!
 	\brief Destroy the runtime environment
	\param RTObj	pointer to nvmRuntimeEnvironment Object
*/
DLL_EXPORT void nvmDestroyRTEnv(nvmRuntimeEnvironment *RTObj);

/*!
  \brief	Create an Application Input Interface (Push mode)  

			The created interface allows the user application to send packets to the NetVM
  \param	RTObj		Runtime Environment Object
  \param	ErrBuf		error buffer	
  \return	A pointer to the nvmAppInterface object
*/
DLL_EXPORT nvmAppInterface *nvmCreateAppInterfacePushIN(nvmRuntimeEnvironment *RTObj, char *ErrBuf);


/*!
  \brief	Create an Application Output Interface (Push mode)
	
			The created interface allows the NetVM to communicate with the user application by calling a callback function that must be provided by the user
  \param	RTObj		Runtime Environment Object
  \param	CtdCallback	callback function to register on the output application interface
  \param	ErrBuf		error buffer	
  \return	A pointer to the nvmAppInterface object
*/

DLL_EXPORT nvmAppInterface *nvmCreateAppInterfacePushOUT(nvmRuntimeEnvironment *RTObj,nvmCallBackFunct *CtdCallback, char *ErrBuf);
/*!
  \brief	Create an Application Input Interface (Pull mode) 

  The created interface allows the NetVM to receive packets from the user application, by calling a polling function that must be provided by the user
  \param	RTObj		Runtime Environment Object
  \param	PollingFunct	polling function to register on the input application interface
  \param	ErrBuf			error buffer	
  \return	A pointer to the nvmAppInterface object
*/
DLL_EXPORT nvmAppInterface *nvmCreateAppInterfacePullIN(nvmRuntimeEnvironment *RTObj,nvmPollingFunct *PollingFunct, char *ErrBuf);
/*!
  \brief	Create an Application Output Interface (Pull mode) 

  The created interface allows the user application to receive packets from the NetVM
  \param	RTObj		Runtime Environment Object
  \param	ErrBuf		error buffer	
  \return	A pointer to the nvmAppInterface object
*/
DLL_EXPORT nvmAppInterface *nvmCreateAppInterfacePullOUT(nvmRuntimeEnvironment *RTObj, char *ErrBuf);

/*!
  \brief	Bind an application interface to a NetVM socket  
  \param	AppInterface	Application Interface object to bind	
  \param	Socket 			nvmSocket object to bind	
  \return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT int32_t nvmBindAppInterf2Socket(nvmAppInterface *AppInterface, nvmSocket *Socket);


/*!
  \brief	Send a packet into NetVM through an application interface

  \param	AppInterface	Application Interface object (must have input direction)	
  \param	pkt				pointer to packet	
  \param	PktLen			size of the packet	
  \param	userData		user data to be passed to the callback function	
  \param	ErrBuf			error buffer	
  \return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT int32_t nvmWriteAppInterface(nvmAppInterface *AppInterface, uint8_t *pkt, uint32_t PktLen, void *userData, char * ErrBuf);


/*!
  \brief	Send a packet into NetVM through an application interface with user-provided timestamp   
  \param	AppInterface	Application Inteface object (must have input direction)
  \param	pkt				pointer to packet	
  \param	PktLen			size of the packet	
  \param	tstamp			user provided time stamp
  \param	userData		user data to be passed to the callback function	
  \param	ErrBuf			error buffer	
  \return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT int32_t nvmWriteAppInterfaceTS(nvmAppInterface *AppInterface, uint8_t *pkt, uint32_t PktLen, nvmTStamp *tstamp, void *userData, char * ErrBuf);



/*!
  \brief	Start the NetVM application 

			This function initializes the application. If JIT compilation is selected, the program is compiled and 
			all the init handlers are executed
  \param	NetVM			pointer to NetVM object	
  \param	RTObj			pointer to Runtime Environment object	
  \param	UseJit			'true' if we have to use JIT (false if we want to use the interpreter)
  \param	JitFlags		flags for the jit compiler
  \param	OptLevel		optimization level for the compiler
  \param	ErrBuf			error buffer	
  \return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT int32_t nvmNetStart(
	nvmNetVM *NetVM, 
	nvmRuntimeEnvironment *RTObj, 
	int32_t UseJit, 
	uint32_t JitFlags,     // FULVIO shall we change this into an 'enum nvmJITFlags' ?
	uint32_t OptLevel, 
	char *ErrBuf);


/*!
	\brief	Receive packets from an application interface 
	\param	AppInterface	nvmAppInterface object must have dir=in	
	\param	exbuf			pointer to a pointer to nvmExchangeBuffer object	
	\param	userData		something to pass at the callback function	
	\param	ErrBuf			error buffer	
	\return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT int32_t nvmReadAppInterface(nvmAppInterface *AppInterface, nvmExchangeBuffer **exbuf, void *userData, char *ErrBuf);


/*!
	\brief		Get an exchange buffer
	\param		RTObj	Runtime Environment object
	\return	return a empty nvmExchangeBuffer. If there are some error return NULL
*/
DLL_EXPORT nvmExchangeBuffer *nvmGetExbuf(nvmRuntimeEnvironment *RTObj);


/*!
	\brief		Release the exchange buffer
	\param		RTObj	Runtime Environment object
	\param		exbuf	pointer to the exchange buffer to release
	\return	nvmSUCCESS or nvmFAILURE
 */
DLL_EXPORT void nvmReleaseExbuf(nvmRuntimeEnvironment *RTObj, nvmExchangeBuffer *exbuf);


/*!
 	\brief Print runtime statistics for all the handlers of the Runtime Environment
 	\param		RTObj	Runtime Environment object
 */
DLL_EXPORT void nvmPrintStatistics(nvmRuntimeEnvironment *RTObj);

/*!
 	\brief Dump a bytecode code segment on a file
 	\param		outFile		output file 	
 	\param		bytecode	bytecode segment
	\param		bytecodeLen	length of bytecode segment
 */
DLL_EXPORT void nvmDumpBytecode(FILE *outFile, uint8_t *bytecode, uint32_t bytecodeLen);


/*!
 	\brief Dump all the contents of a bytecode image on a file (code segments and metadata)
 	\param		file	output file
 	\param		binary	nvmByteCode object
 */
DLL_EXPORT void nvmDumpBytecodeInfo(FILE *file,nvmByteCode *binary);



/*!
  \brief Enumarate the available physical interfaces
  
  		This function creates a list of interface descriptors that contain information on the available physical interfaces
		
  \param	RTObj		nvmRuntimeEnvironment Object
  \param	ErrBuf			error buffer	
  \return	pointer to nvmPhysInterfaceInfo 
*/
DLL_EXPORT nvmPhysInterfaceInfo *nvmEnumeratePhysInterfaces(nvmRuntimeEnvironment *RTObj,char *ErrBuf);


/*!
  \brief	Bind a physical interface to a socket	   
  \param	PhysInterface		pointer to a nvmPhysInterface object
  \param	Socket		socket to bind	
  \return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT int32_t nvmBindPhysInterf2Socket(nvmPhysInterface *PhysInterface, nvmSocket *Socket);

/*!
  \brief	Read packets from a physical interface through an infinite loop and send them into NetVM	   
  \param	PhysInterface	pointer to a nvmPhysInterface obj
  \param	userData		something to pass at the callback function	
  \param	ErrBuf			error buffer	
  \return	nvmSUCCESS or nvmFAILURE 
*/
DLL_EXPORT int32_t nvmStartPhysInterface(nvmPhysInterface *PhysInterface,void *userData, char *ErrBuf);

/*!
  \brief	Tell if the current build of the NetVM Runtime Environment supports runtime statistics   	
  \return	0 if the statistics are not available, 1 otherwise
*/
DLL_EXPORT uint32_t nvmRuntimeHasStats(void);


/*!
  \brief	Get a list containing the runtime statistics (number of packets processed and ticks) for every PE Handler 
  \param	RTObj	the current runtime environment
  \param	errbuf	the error buffer
  \return	a list of nvmNetPEHandlerStats objects

  \note	the user has not to free the list returned by this function
*/
DLL_EXPORT nvmNetPEHandlerStats *nvmGetPEHandlerStats(nvmRuntimeEnvironment *RTObj, char *errbuf);


/*!
 * \brief	Enumerate the available compiler backends for generating NetVM native code.

 This function does not consider the 'netil' backend; only backends that can generate
 native code are considered.

 * \param   Num Variable that, upon return, will contain the number of backends avalable on this platform
 * \return  A pointer to the array of target descriptors; 'Num' backends will be available.
 */
DLL_EXPORT nvmBackendDescriptor *nvmGetBackendList(uint32_t *Num);


/*!
 * \brief   Compiles a netvm application
 * \param 	NetVM pointer to the netvm object
 * \param 	RTObj pointer to the runtime object
 * \param	BackendID number identifying the target used to compile the application
 * \param 	JitFlags flags for the compiler selecting what to do
 * \param	OptLevel optimization level for the compiler
 * \param	OutputFilePrefix string to prepend to filenames emitted
 * \param	ErrBuf buffer of for error messages
 */
DLL_EXPORT int32_t nvmCompileApplication(
	nvmNetVM *NetVM, 
	nvmRuntimeEnvironment *RTObj, 
	uint32_t BackendID, 
	uint32_t JitFlags , // FULVIO shall we change this into an 'enum nvmJITFlags' ?
	uint32_t OptLevel, 
	const char* OutputFilePrefix, 
	char *ErrBuf);


/*!
 * \brief   Retrieves the target assembler code generated through nvmCompile_Application()
 * \param 	RTObj pointer to the runtime object
 */
DLL_EXPORT char *nvmGetTargetCode(nvmRuntimeEnvironment *RTObj);

/*
 \}
*/




/** \} */

#ifdef __cplusplus
}
#endif
