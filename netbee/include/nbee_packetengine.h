/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once


/*!
	\file nbee_packetengine.h

	Keeps the definitions and exported functions related to the Packet Engine module of the NetBee library.
*/


#ifdef NBEE_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif


#include <nbprotodb_exports.h>
#include <nbpflcompiler.h>


// Forward declarations
class nbExtractedFieldsReader;
struct _nvmTStamp;
struct _nvmRuntimeEnvironment;


/*!
	\defgroup NetBeePacketEngine NetBee classes for packet capture and field extraction
	
	\brief This module allows filtering and extracting selected fields from network packets.

	This module provides a set classes, methods and properties that allows processing packets using
	the NetPDL technology (that defines protocol headers), the NetPFL language (that defines what to do
	on network packets in terms of filtering and field extraction), and the NetVM engine (that does the
	actual job).
*/

/*! \addtogroup NetBeePacketEngine
	\{
*/


//! \brief Defines the code that allow to select if the NetVM has only to compile the code, or also start executing it
typedef enum nbNetVMCreationFlag_t
{
	nbNETVM_CREATION_FLAG_COMPILEANDEXECUTE,		//!< Configures the NetVM to compile the code and execute it
	nbNETVM_CREATION_FLAG_COMPILEONLY				//!< Configures the NetVM only to compile the code
} nbNetVMCreationFlag_t;


/*!
	\brief This class allows filtering and extracting selected fields from network packets.
	
	This class hides all the details of the NetPFL compiler, exporting a clean and easy interface.
	In case this is needed, the actual NetPDL compiler can be accessed by looking at the documentation
	of the <a href="../nbpflcompiler/index.htm" target="_top">NetPFL Compiler module</a>.

	Some additional classes (and structures) are required in order to parse the data returned by the NetVM
	engine. For instance, a call to the GetExtractedFieldsReader() method returns a nbExtractedFieldsReader
	object that will be used to parse the extracted fields and print their values and such.

	In case this class is used to simply filter packets, the GetExtractedFieldsReader() will return
	and nbExtractedFieldsReader object with an empty _nbExtractedFieldsDescriptorVector object.

	The nbPacketEngine class allows also to configure the debug compiler options using the SetXXX methods.
	This is usually required only in case of debugging.
	
	To get compiling information and structures:
	- The messages created by the compilation are available with the GetCompMessageList  that returns a pointer to a _nbNetPFLCompilerMessages structures. 
	- To get the last error message call the GetLastError method.
	- The NETIL compiled code  is available with the GetCompiledCode method.
	- The _nvmRuntimeEnvironment is available with the GetNetVMRuntimeEnvironment method.
	
	The nbPacketEngine class must be allocated using the nbAllocatePacketEngine() function because is an abstract class.
	The de-allocation is done through the nbDeallocatePacketEngine() function.
*/

class DLL_EXPORT nbPacketEngine
{

public:

	/*!
		\brief	Object constructor

	*/
	nbPacketEngine() {};

	/*!
		\brief	Object destructor
	*/
	virtual ~nbPacketEngine(void) {};

	/*!
		\brief Compiles a NetPFL filter using the nbpflcompiler class
		This method initializes the compiler and call the CompileFilter method of the nbpflcompiler,
		then it allocates the nbExtractedFieldsReader object for the extraction action.

		This method can be used to compile a filter in the Netpfl langauge.
		It must be called before the processing of the packets. 
		
		\param NetPFLFilterString			filter string in the NetPFL language

		\param LinkLayer		Link layer type of the packets we want to filter (e.g., Ethernet).
		This value can be read from the dump file that contains the packets we want to filter
		or from the network interface actually in use for capturing packets. Allowed values are
		listed in #nbNetPDLLinkLayer_t.

		\param Opt				flag that turns the frontend optimizations on
		
		\return nbSUCCESS if in the initialization no error occurred and the compile of
		the filter is successfull. 
		nbFAILURE if there are some error in the initialization or if the compile fails.
	*/
	virtual int Compile(const char* NetPFLFilterString, nbNetPDLLinkLayer_t LinkLayer, bool Opt = true)=0;

	/*!
		\brief	Initializes and starts the NetVM Engine. 
		
		It creates and it inizializes the Virtual Machine.
		Then it connects the input and the output socket to the NetPE ports, 
		It creates the NetVM Runtime environment, an input and an ouput Application Interface.
		It makes the binding of these interfaces with the sockets and
		finally it starts the netVM.

		It must be called after the Compile method and before the Process one. 

		\param CreationFlag     Flag for the initialization of the NetVM, according to
		the values defined in \ref nbNetVMCreationFlag_t

		\return nbSUCCESS if in the initialization no error occurred,nbFAILURE otherwise
	*/
	virtual int InitNetVM(nbNetVMCreationFlag_t CreationFlag)=0;

	/*!
		\brief Returns the last nbExtractedFieldsReader object allocated in the Compile method.

		This method must be used when the nbPacketEngine class is configured to extract fields from
		the packet. This method returns a class that will be used to parse the extracted fields
		in order to print their values and such.

		\return A nbExtractedFieldsReader object if no error occured, NULL otherwise.

		\note if in the filter there isn't the ExtractFields action this method returns
		a nbExtractedFieldsReader with a _nbExtractedFieldsDescriptorVector object empty.
	*/
	virtual nbExtractedFieldsReader* GetExtractedFieldsReader()=0;


	/*!
		\brief Processes a packet (winpcap structure)

		This method can be used to process a packet, if no error occured and if it's
		valid for the filter compiled with the 'Compile(char* NetPFLCode)' method.

		\param PktData		pointer to the buffer containing the packet, according to the WinPcap definition.
		\param PktLen		length of the packet

		\return nbSUCCESS if the filter is succesful and no error occurred,
		nbFAILURE if the filter fails or there are some other errors. For instance, 
		if the processing program sets the "ip" filter and the packet is not IP, this
		function returns nbFAILURE.
		
		\note The return code strictly depends on the result of the packet filter, even
		if the processing program involves also the 'extractfields' capability of this engine.
		For instance, if the filter is satisfied but the packet does not contain the fields
		that have to be extracted, this function still returns nbSUCCESS, but the returned
		field list does not contain any valid value.
	*/
	virtual int ProcessPacket(const unsigned char *PktData, int PktLen)=0;

	/*!
		\brief Gets the list of error/warning messages generated during the initialization or compilation phases

		\return a list of _nbNetPFLCompilerMessages structures
	*/

	virtual _nbNetPFLCompilerMessages *GetCompMessageList(void)=0;

	/*!
		\brief Returns a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	virtual char *GetLastError()=0;

	/*!
		\brief Initializes the compiler with a precompiled code and the nbExtractedFieldsReader with a _nbExtractedFieldsDescriptorVector.

		It must be called before the processing of the packets.

		\param NetILCode    Code already compiled for the NetVM (NETIL code)

		\param LinkLayer		Link layer type of the packets we want to filter (e.g., Ethernet).
		This value can be read from the dump file that contains the packets we want to filter
		or from the network interface actually in use for capturing packets. Allowed values are
		listed in #nbNetPDLLinkLayer_t.

		\param ExtractedFieldsDescriptorVector		   _nbExtractedFieldsDescriptorVector of fields that will be extracted

		\return nbSUCCESS if in the initialization no error occurred
		nbFAILURE if there are some error in the initializations.
		*/
	virtual int InjectCode(nbNetPDLLinkLayer_t LinkLayer, char *NetILCode, _nbExtractFieldInfo *ExtractedFieldsDescriptorInfo, int num_entry)=0;
	
	/*!
		\brief Gets the compiled code of the filter (NETIL code)

		This method can be used to get the compiled code of the filter that result from the
		last call to the Compile(char* NetPFLCode) method.

		\return a pointer to the compiled code if it exists, NULL otherwise
	*/
	virtual char* GetCompiledCode()=0;
	
	/*!
		\brief Initializes the netVM engine for the COMPILEONLY mode. 
		Set the file where the dump of the backend code will be write.

		\param BackendId	The Id of the wanted compiler backend.
		\param Opt			Turn compiler optimizations on/off.
		\param Inline		Inline option, if supported by the specified backend.
		\param DumpFileName	Name of the prefix of the file where the coded is dumped (if NULL the code is kept in a string).

		\return nbSUCCESS if in the initialization no error occurred
		nbFAILURE if there are some error in the initializations.
	*/
	virtual int GenerateBackendCode(int BackendId, bool Opt, bool Inline, const char* DumpFileName)=0;

	/*!
		\brief Extracts a C string containing the assembly code generated by the NetVM backend
		\return a C string containing the target assembly code, NULL if not present
	*/

	virtual char * GetAssemblyCode() = 0;
	

	/*!
		\brief Set the flow information detail level

		\param DebugLevel Activate debug messages during execution:
		- 0 (default): no debugging is enabled at run-time
		- 1: NetPDL initialization messages and simple execution information
		- 2: NetPDL initialization messages and full execution information
	*/
	virtual void SetDebugLevel(const unsigned int DebugLevel)=0;
	
	/*!
		\brief set the dump file for the NETIL code
		\param dumpNetILCodeFilename		file name
	*/
	virtual void SetNetILCodeFilename(const char *dumpNetILCodeFilename)=0;
	
	/*!
		\brief set the dump file for the HIR code
		\param dumpHIRCodeFilename		file name
	*/
	virtual void SetHIRCodeFilename(const char *dumpHIRCodeFilename)=0;

	/*!
		\brief set the dump file for the LIR code
		\param dumpLIRCodeFilename		file name
	*/
	virtual void SetLIRCodeFilename(const char *dumpLIRCodeFilename)=0;

	/*!
		\brief set the dump file for the LIR graph
		\param dumpLIRGraphFilename	file name
	*/
	virtual void SetLIRGraphFilename(const char *dumpLIRGraphFilename)=0;
	
	/*!
		\brief set the dump file for the NoOpt graph
		\param dumpLIRNoOptGraphFilename		file name
	*/
	virtual void SetLIRNoOptGraphFilename(const char *dumpLIRNoOptGraphFilename)=0;
	
	/*!
		\brief set the dump file for the NoCode graph
		\param dumpNoCodeGraphFilename		file name
	*/
	virtual void SetNoCodeGraphFilename(const char *dumpNoCodeGraphFilename)=0;
	
	/*!
		\brief set the dump file for the NETIL graph
		\param dumpNetILGraphFilename		file name
	*/
	virtual void SetNetILGraphFilename(const char *dumpNetILGraphFilename)=0;
	
	/*!
		\brief set the dump file for the Proto graph
		\param dumpProtoGraphFilename		file name
	*/
	virtual void SetProtoGraphFilename(const char *dumpProtoGraphFilename)=0;

	/*!
		\brief set the dump file for the Filter automaton
		\param dumpFilterAutomatonFilename		file name
	*/
	virtual void SetFilterAutomatonFilename(const char *dumpFilterAutomatonFilename)=0;

	/*!
		\brief Gets the nvmRuntimeEnvironment structure initialized by the call to InitNVM() method.
		
		\return pointer to the nvmRuntimeEnvironment structure if it's initialized, NULL otherwise
	*/
	virtual struct _nvmRuntimeEnvironment *GetNetVMRuntimeEnvironment(void)=0;
};


/*!
	\brief A pointer to real object that has the same interface of the nbPacketEngine abstract class, or NULL in case of error.

	\param	UseJit			Sets the NetVM for using jitted (native) code.
	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will eventually 
	keep an error message (if one).

	\param ErrBufSize: the length of the buffer that keeps the error message.

	\return A nbPacketEngine object if no errors occurred, NULL otherwise.
	In case of failure, the error message is returned into the ErrBuf buffer.
*/
DLL_EXPORT nbPacketEngine *nbAllocatePacketEngine(bool UseJit, char *ErrBuf, int ErrBufSize);


/*!
	\brief  De-allocates the nbPacketEngine object

	\param	PacketEngine Pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocatePacketEngine(nbPacketEngine *PacketEngine);


/*!
	\}
*/
