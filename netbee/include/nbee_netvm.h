/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#ifdef NETPDL_EXPORTS
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT __declspec(dllexport)
	#endif
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif



#include "../../Modules/NetVM/Include/netvm.h"
#include <nbnetvmports.h>


/*!
	\defgroup NetBeeNetVM NetBee interface for the NetVM
*/

/*! \addtogroup NetBeeNetVM 
	\{
*/


#ifdef __cplusplus
extern "C" {
#endif

/*!
	\brief It accepts a user-friendly filter (e.g. 'ip.src') and it returns an executable in the NetVM format.

	This method is invoked by external programs to submit a user level filter description.

	\param LinkLayerType: Link layer type of the interface.

	\param FilterSpecChars: pointer to a NULL-terminated buffer that contains the user level filter specification.

	\param SourceCode: pointer to a char buffer containing the assembly code according to the instruction
	set of the NetVM virtual machine.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will eventually 
	keep an error message (if one).

	\param ErrBufSize: the length of the buffer that keeps the error message.

	\return nbFAILURE if the function fails, nbSUCCESS otherwise. In case of failure, the error
	message is returned into the ErrBuf buffer, which is always '0' terminated.
*/
int nbPacketFilterCompiler(int LinkLayerType, char *FilterSpecChars, char *SourceCode, char *ErrBuf, int ErrBufSize);

#ifdef __cplusplus
}
#endif


// Please note that the same macros are in NetVMinternals.h
//! Macro that checks if the bit corresponding to flag 'flag' is set in variable 'var'.
#define nbFLAG_ISSET(var,flag) (flag & var)

//! Macro that set the bit corresponding to flag 'flag' in variable 'var'.
#define nbFLAG_SET(var,flag) {var=var|flag;}



//class nbNetVmByteCode;

/*!
	\brief It holds the NetVm environment
*/
class DLL_EXPORT nbNetVm
{

friend class nbNetVmPortApplication;

public:
	nbNetVm(bool UseJit); //!< create a NetVM istance (Jit or Interpreter) 
	~nbNetVm(void);
	int AttachPort(nbNetVmPortDataDevice  * NetVmExternalPort, int Mode); 		//!< Attach port to NetVM
	int ConnectPorts(nvmExternalPort *firstPort, nvmExternalPort *secondPort);	//!< Connect two ports (internal or external to NetVM)
	int ConnectPorts(nvmExternalPort *firstPort, nvmInternalPort *secondPort);	//!< Connect two ports (internal or external to NetVM)
	int ConnectPorts(nvmInternalPort *firstPort, nvmExternalPort *secondPort);	//!< Connect two ports (internal or external to NetVM)
	int ConnectPorts(nvmInternalPort *firstPort, nvmInternalPort *secondPort);	//!< Connect two ports (internal or external to NetVM)
	int Start(void);	//!< Start the Virtual Machine
	void Stop(void);	//!< Stop the Virtual Machine
	nvmNetVMState  * GetNetVmPointer() {return VmState;} //!< return the pointer to the istance of Virtual Machine
	char *GetLastError() {return m_errbuf;} //!< Return the error messages (if any)

	void SetProfilingResultsFile(FILE *file);	//!< Return NetVM profiling result into a given file
	
private:
	nvmNetVMState * VmState;		//!< Virtual Machine State
	char m_errbuf[2048];			//!< Buffer that keeps the error message (if any)
};


/*!
	\brief It holds the NetPe informations
*/
class DLL_EXPORT nbNetPe
{
friend class nbNetVmPortApplication;
friend class nbNetVmByteCode;

public:
	nbNetPe(nbNetVm *virtual_machine); //!< Create a NetPE of the virtual_machine istance
	~nbNetPe(void);
	nvmInternalPort * GetPEPort(int PortId); //!< Return the pointer to the NetPE Internal port with PortID identification
	nvmPEState * GetPEPointer() {return NetPeState;} //!< Return the pointer to the NetPE 
	// PASCAL: the following function was commented out
	nbNetVmByteCode * CompileProgram(char *source_code); //!< Not Implemented
	// PASCAL: the following function was commented out
	int InjectCode(nbNetVmByteCode *NetVmByteCode);  //!< Inject NetVM ByteCode in the NetPE
	char *GetLastError() {return m_errbuf;}  //!< Return the error messages (if any)

private:
	nvmPEState * NetPeState;		//!< NetPE state
	nvmNetVMState * VmState;		//!< Virtual Machine State
	char m_errbuf[2048];			//!< Buffer that keeps the error message (if any)
};


/*!
	\brief Class for managing NetVM bytecode.
*/
class DLL_EXPORT nbNetVmByteCode
{

public:
	nbNetVmByteCode();
	~nbNetVmByteCode();
	int OpenBinaryFile(char *FileName); 				//!< Open ByteCode binary file  
	int CreateBytecodeFromAsmBuffer(char* NetVMAsmBuffer);		//!< Create ByteCode from ASM buffer
	int CreateBytecodeFromAsmFile(char* FileName);			//!< Create ByteCode ASM file
	int SaveBinaryFile(char* FileName);  				//!< Save ByteCode binary file  
	void close(void);  						//!< Close the netVM ByteCode
	nvmByteCode  * GetNetVmByteCodePointer() {return ByteCode;}  	//!< Return the pointer to the ByteCode structure
	char *GetLastError() {return m_errbuf;}  			//!< Return the error messages (if any)
	void DumpBytecode(FILE *file);						//!< Dump the bytecode to file

private:
	nvmByteCode * ByteCode;		//!< ByteCode pointer
	char m_errbuf[2048];		//!< Buffer that keeps the error message (if any)
};


/*!
	\}
*/

