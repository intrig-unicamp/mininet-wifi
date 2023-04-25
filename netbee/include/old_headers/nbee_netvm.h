/*
 * Copyright (c) 2002 - 2004
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Politecnico di Torino nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#ifndef __NETBEENETVM_H__
#define __NETBEENETVM_H__


#ifdef NETPDL_EXPORTS
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
		#define DLL_EXPORT __declspec(dllexport)
		#define DLL_FILE_NAME "NetBee.dll"
	#endif
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
		#define DLL_EXPORT
		#define DLL_FILE_NAME ""
	#endif
#endif


#include "NetVMStructs.h"
#include "NetVMExport.h"
#include "NetBeeNetVMPorts.h"


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
	\brief GV_NOT_TOUCHED It accepts a user-friendly filter (e.g. 'ip.src') and it returns an executable in the NetVM format.

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



class nbNetVmByteCode;

/*!
	\brief It holds the NetPe informations
*/
class DLL_EXPORT nbNetPe
{
friend class nbNetVm;

protected:
	/*!
		\brief Creates a NetPe

		It creates a new NetPe in the NetVm environment.

		\param virtual_machine data structure that represent the handle for the whole virtual machine
	*/
	nbNetPe(nbNetVm *virtual_machine);

	/*!
		\brief Destroys a NetPe and its ports

		It destroys a new NetPe in the NetVm environment. It removes all connections from current NetPe and then free all resources used by a NetPe.
		THIS IS NOT CLEAR: what does it do? does it deregister the PE from the father???
*/
	~nbNetPe(void);

public:

	/*!
		\brief Get a netPe port handle

		It returns a port handle of one port of current netPe identified by a label

		\param PortId: id of the port

		\return A pointer to the port if everything is fine, NULL in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	nbPort *GetPort(int PortId);

	/*!
		\brief Get a netPe port handle

		It returns a port handle of one port of current netPe identified by a label

		\param portName: label of the port

		\return A pointer to the port if everything is fine, NULL in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	nbPort *GetPort(char* portName);

	/*!
		\brief Compile high level source program into a binary code for NetVM

		\param source_code pointer to a char buffer containing the NetPe source code

		\return A pointer to a binary form of the code to be executed on NetVM if everything is fine, NULL in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	nbNetVmByteCode* CompileProgram(char *source_code);

	
	/*!
		\brief Download the source code into a NetPe

		It writes the binary code of a NetPe. It deploys the binary code in NetPe running environment

		\param NetVmByteCode: pointer to a nbNetVmByteCode object, that contains the code to be executed by this NetPe

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	int InjectCode(nbNetVmByteCode *NetVmByteCode);

	/*!
		\brief Return a string keeping the last error message that occurred within the current instance of the class.

		\note This method, as the whole class, is not thread safe, so calling this method
		from different threads can cause unexpectable behaviors.

		\return The error message.
	*/
	char *GetLastError() {return m_errbuf;}


private:
	nvmPEState m_NetPeState;		//!< NetPE state
	int m_PortNumber;				//!< Total number of ports in the NetPE
	nbNetVmPortNetPE **m_PortTable; //!< Table of the NetPE ports
	nbNetVm *m_OwnerNetVm;			//!< Pointer to the virtual machine 
	char m_errbuf[2048];			//!< Buffer that keeps the error message (if any)
};



/*!
	\brief It holds the NetVm environment
*/
class DLL_EXPORT nbNetVm
{
friend class nbNetPe;

public:
	/*!
		\brief Creates the NetVm instance.

		It creates the NetVm instance and returns a handle of the whole NetVm
		that should be used in next library calls.
	*/
	nbNetVm();


	/*!
		\brief Destroys the NetVm instance.

		It destroys the NetVm instance. This function should be called only 
		after that nbNetVm::Stop() has been invoked.

	*/
	virtual ~nbNetVm();
	
	/*!
		\brief Starts the NetVm instance.

		It starts all the internal workings (e.g. the NetPes) of the NetVm 
		instance.

		\return 
			- \ref nbSUCCESS if everything is fine, 
			- \ref nbFAILURE in case of errors.
			In case of error, the error message can be retrieved with the 
			GetLastError() method.
	*/
	int Start(void);


	/*!
		\brief Stops the NetVm instance.

		It stops all the internal workings (e.g. the NetPes) of the NetVm 
		instance.

		\return 
			- \ref nbSUCCESS if everything is fine, 
			- \ref nbFAILURE in case of errors.
			In case of error, the error message can be retrieved with the 
			GetLastError() method.
		
	*/
	int Stop(void);


	/*!
		\brief Get an address of writeable shared memory.

		It returns a NetVm shared mem address where application could write 
		its data.

		\return the address of writeable shared memory
	*/
	void *GetWriteableMem(void);

	/*!
		\brief Returns a string keeping the last error message that occurred 
			within the current instance of the class.

		\note This method, as the whole class, is not thread safe, so calling 
			this method from different threads can cause an unexpectable 
			behavior.

		\return The error message.
	*/
	char *GetLastError() {return m_errbuf;}

	/*!
		\brief It creates a new NetPe inside the NetVm instance.

		\return A new \ref nbNetPe object, or NULL. The object returned by this
			method should not be destroyed. It will be destroyed when the NetVm
			instance is released.
		*/
	
	nbNetPe *CreateNetPe();

private:
	nvmNetVMState m_VmState;	//!< Virtual Machine State

	char m_errbuf[2048];	//!< Buffer that keeps the error message (if any)
};

/*!
	\brief Class for managing NetVM bytecode.

	This class is devoted to open, save and create bytecode. For instance, this class can be used to open 
	a file containing a (binary) bytecode and store it in memory.

	This class provides also some methods that allow to load a file contaning the bytecode in assembly and
	to create the (binary) bytecode out of it.

	The so created bytecode object can be injected within a NetPE in order to customize its execution.
*/
class DLL_EXPORT nbNetVmByteCode
{
friend class nbNetPe;

public:
	//! Default constructor
	nbNetVmByteCode();

	//! Closes a NetVM binary and releases all the memory buffers associated to it.
	~nbNetVmByteCode();


	/*!
		\brief Opens an assembled (binary) bytecode from a file.

		\param FileName: name of the file containing the bytecode (in binary format) to open

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	int OpenBinaryFile(char *FileName);


	/*!
		\brief Closes the underlying assebled bytecode.

		This function must be used in order to clear an existing bytecode before 
		loading / creating a new one.
	*/
	void Close();


	/*!
		\brief Creates an assembled (binary) bytecode out of a string buffer in NetVM assembly.

		The buffer contains the NetVM program in assembly language, which is the
		final step before getting the bytecode in binary format.

		\param NetVMAsmBuffer: string containing the bytecode to be assembled

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	int CreateBytecodeFromAsmBuffer(char* NetVMAsmBuffer);

	/*!
		\brief Creates an assembled (binary) bytecode out of a file to be assembled (".asm" file).

		The file contains the NetVM program in assembly language, which is the
		final step before getting the bytecode in binary format.

		\param FileName: name of the file containing the bytecode to be assembled.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	int CreateBytecodeFromAsmFile(char* FileName);


	/*!
		\brief Saves an assembled bytecode (in binary format) in a given file.

		The class must contain a valid bytecode, otherwise this call will fail.

		\param FileName: name of the file that will contain the assembled bytecode.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message can be retrieved with the GetLastError() method.
	*/
	int SaveBinaryFile(char* FileName);

	//! Return a string keeping the last error message that occurred within the current instance of the class.
	char *GetLastError() {return m_errbuf;}


private:

	/*! 
		\brief Adds a section to a (binary) bytecode.

		This function is used to create the bytecode in memory.
		This function adds a section to a NetVM bytecode.

		\param SaveBin pointer to the bytecode structure.
		\param SectName name of the section in the bytecode (e.g. ".push")
		\param SectFlags flags of the section.
		\param SectBody body of the section.
		\param SectSize size of the section.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message is stored in the m_errbuf internal variable.
	*/
	u_int32_t AddSection(nvmByteCode * SaveBin, char* SectName, u_int32_t SectFlags, char* SectBody, u_int32_t SectSize);


	/*! 
		\brief sets the entry point for the Init function .

		This function is used to create the bytecode in memory.

		\param SaveBin a pointer to the bytecode.
		\param SectName a pointer to the name of the section.
		\param addr the offset of the section.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message is stored in the m_errbuf internal variable.
	*/
	u_int32_t SetInitEntryPoint(nvmByteCode * SaveBin, char* SectName, u_int32_t addr);


	/*! 
		\brief sets the entry point for the Push function .

		This function is used to create the bytecode in memory.

		\param SaveBin a pointer to the bytecode.
		\param SectName a pointer to the name of the section.
		\param addr the offset of the section.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message is stored in the m_errbuf internal variable.
	*/
	u_int32_t SetPushEntryPoint(nvmByteCode * SaveBin, char* SectName, u_int32_t addr);

	/*! 
		\brief sets the entry point for the Pull function.

		This function is used to create the bytecode in memory.

		\param SaveBin a pointer to the bytecode.
		\param SectName a pointer to the name of the section.
		\param addr the offset of the section.

		\return nbSUCCESS if everything is fine, nbFAILURE in case of errors.
		In case of error, the error message is stored in the m_errbuf internal variable.
	*/
	u_int32_t SetPullEntryPoint(nvmByteCode * SaveBin, char* SectName, u_int32_t addr);



	/*! 
		\brief Initialize the structures needed to contain the bytecode.

		This function is used to create the bytecode in memory.
		This function creates a new bytecode structure and fills the structure's fields with the values passed as parameters
		and other standard values.

		\param DefaultDataMemSize the dimension of the data memory that will be setted in the bytecode.
		\param DefaultExchangeBufferSize the dimension of the exchange buffer memory that will be setted in the bytecode.
		\param DefaultStackSize the dimension of the memory allocated for the stack that will be setted in the bytecode.

		\return a pointer to the bytecode created.
		In case of error, the error message is stored in the m_errbuf internal variable.
	*/
	nvmByteCode *Create(u_int32_t DefaultDataMemSize, u_int32_t DefaultExchangeBufferSize, u_int32_t DefaultStackSize);


private:
// FULVIO PER GIANLUCA: verificare se non e' il caso di creare un altro wrapper, come per le altre funzioni NetBee, in modo che venga nascosto
// il dettaglio relativo a questa struttura (quindi non sia piu' necessario linkare i files esportati dalla NetVM)
	//! Pointer to a valid bytecode, in binary format.
	nvmByteCode *m_ByteCode;

	//! Buffer that keeps the error message (if any)
	char m_errbuf[2048];
};



/*!
	\}
*/


#endif // __NETBEENETVM__