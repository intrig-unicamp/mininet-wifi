/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

#include <nbprotodb_elements_internal.h>
#include <nbpflcompiler.h>

#ifdef NETPDL_EXPORTS
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT __declspec(dllexport)
	#endif
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif

//! \brief This class can be used to parse the list of fields extracted from a network packet.
class nbExtractedFieldsReader;

struct _nvmTStamp;
struct _nvmRuntimeEnvironment;

/*!
	\brief The nbPacketEngine class provides a rich set of methods and properties to initialize 
	and use the netil compiler and the NetVM engine. 
	The nbPacketEngine class allows to perform processing of the packet and to get information about extracted field, 
	using the nbExtractedFieldsReader class and some other structures such as _nbFieldDescriptor. 
	
	To initialize the compiler and the NetVM engine use the following methods:
	- If the compiler is called for the first time in the program, use the Compile() method that allows to set the filter. 
		If a previous compiler object is allocated and a filter is already set, use the CompileEx() method to add other rules to the compiler (not implemented).
	- If the compiler is initialized use the InitNVM method that allows to initialize the environment and the socket variables.
	- If a compiled filter is available use the Initialize method to set 'extractfields' structures.
	- To set a COMPILE_ONLY mode for the NetVM engine use GenerateBackendCode method.
	
	To process a packet with the compiler and the netVM:
	- Call the ProcessPacket method that returns nbSUCCESS if the filter is successful and no error occurred, nbFAILURE otherwise. 
		The return code strictly depends on the result of the packet filter, even if the processing program involves also the 'extractfields' capability of this engine.
	
	To get the structures containing the field extraction information:
	- Call the GetExtractedFieldsReader method that returns a nbExtractedFieldsReader object 
	that will be used to parse the extracted fields in order to print their values and such. 
		This method must be used when the nbPacketEngine class is configured to extract fields from the packet. 
		If in the filter there isn't the ExtractFields action this method returns a nbExtractedFieldsReader object with a DescriptorVector object empty.
	
	The nbPacketEngine class allows you to configure the debug compiler options using the SetXXX methods. 
	
	To get compiling information and structures:
	- The messages created by the compilation are available with the GetCompMessageList  that returns a pointer to a CompilerMessage structures. 
	- To get the last error message call the GetLastError method.
	- The NETIL compiled code  is available with the GetCompiledCode method.
	- The _nvmRuntimeEnvironment is available with the GetNetVMRuntimeEnvironment method.
	
	The nbPacketEngine class must be allocated using the nbAllocatePacketEngine function because it’s an abstract class. The de-allocation is done nbDeallocatePacketEngine function.

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
		The creationFlag is used to initialize the NetVM engine 
		with a COMPILEANDEXECUTE mode or in the COMPILEONLY mode.
		
		\param pflCode			filter string in the Netpfl language
		\param opt				flag for set the optimization using
		\param initOnly         flag for set only the compiler initialization step
		
		\return nbSUCCESS if in the initialization no error occurred and the compile of
		the filter is successfull. 
		nbFAILURE if there are some error in the initialization or if the compile fails.
	*/
	virtual int Compile(char* pflCode, bool opt=true, bool initOnly=false)=0;

	/*!
		\brief Adds a new rule to the compiler object.

		This method can be used to add another instance at the NetVM. 
        
		\note
		not implemented
	*/
	virtual int CompileEx(char* pflCode)=0;

	/*!
		\brief	Initializes and starts the NetVM Engine. 
		
		It creates and it inizializes the Virtual Machine.
		Then it connects the input and the output socket to the NetPE ports, 
		It creates the NetVM Runtime environment, an input and an ouput Application Interface.
		It makes the binding of these interfaces with the sockets and
		finally it starts the netVM.

		It must be called after the Compile method and before the Process one. 
		The creationFlag is used to initialize the NetVM engine 
		with a COMPILEANDEXECUTE mode or in the COMPILEONLY mode.

		\param creationFlag     flag for the initialization of the NetVM

		\return nbSUCCESS if in the initialization no error occurred,nbFAILURE otherwise
	*/
	virtual int InitNVM(uint32 creationFlag)=0;

	/*!
		\brief Returns the last nbExtractedFieldsReader object allocated in the Compile method.

		This method must be used when the nbPacketEngine class is configured to extract fields from
		the packet. This method returns a class that will be used to parse the extracted fields
		in order to print their values and such.

		\return A nbExtractedFieldsReader object if no error occured, NULL otherwise.

		\note if in the filter there isn't the ExtractFields action this method returns
		a nbExtractedFieldsReader with a DescriptorVector object empty.
	*/
	virtual nbExtractedFieldsReader* GetExtractedFieldsReader()=0;

	/*!
		\brief Processes a packet (winpcap structure)

		This method can be used to process a packet, if no error occured and if it's
		valid for the filter compiled with the 'Compile(char* pflCode)' method.

		\param PktData		pointer to the buffer containing the packet, according to the WinPcap definition.
		\param pktLen		length of the packet

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
	virtual int ProcessPacket(const unsigned char *PktData, int pktLen)=0;

	/*!
		\brief Gets the list of error/warning messages generated during the initialization or compilation phases

		\return a list of CompilerMessage structures
	*/

	virtual CompilerMessage *GetCompMessageList(void)=0;

	/*!
		\brief Returns a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	virtual char *GetLastError()=0;

	/*!
	\brief Initializes the compiler with a precompiled code and the nbExtractedFieldsReader with a DescriptorVector.

	It must be called before the processing of the packets.

	\param generatedCode    generated code compiled (NETIL code)
	\param DesVector		   DescriptorVector of fields that will be extracted

	\return nbSUCCESS if in the initialization no error occurred
	nbFAILURE if there are some error in the initializations.
	*/
	virtual int InjectCode(nbNetPDLLinkLayer_t LinkLayer, char *NetILCode,  _nbExtractFieldInfo *ExtractedFieldsDescriptorInfo, int num_entry)=0;
	
	/*!
	\brief Gets the compiled code of the filter (NETIL code)

	This method can be used to get the compiled code of the filter that result from the
	last call to the Compile(char* pflCode) method.

	\return a pointer to the compiled code if it exists, NULL otherwise
	*/
	virtual char* GetCompiledCode()=0;
	
	/*!
	\brief Initializes the netVM engine for the COMPILEONLY mode. 
	Set the file where the dump of the backend code will be write.

	\param Id			the Id of the possible Backend
    \param OptCycles	Optimization cycles flag
	\param Inline		inline option if is supported by the specified backend
	\param DumpFileName	name of the prefix of the file where the coded is dumped

	\return nbSUCCESS if in the initialization no error occurred
	nbFAILURE if there are some error in the initializations.
	*/
	virtual int GenerateBackendCode(int Id, bool Opt, unsigned char Inline, char* DumpFileName)=0;

	/*!
		\brief set the flow information detail level
		\param flowInformation		detail level (0,1 or 2)
	*/
	virtual void SetFlowInformation(const unsigned int flowInformation)=0;
	
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
		\brief Gets the nvmRuntimeEnvironment structure initialized by the call to InitNVM() method.
		
		\return pointer to the nvmRuntimeEnvironment structure if it's initialized, NULL otherwise
	*/
	virtual struct _nvmRuntimeEnvironment *GetNetVMRuntimeEnvironment(void)=0;
};


/*!
	\brief A pointer to real object that has the same interface of the nbPacketEngine abstract class, or NULL in case of error.

	\param	NetPDLDatabase  NetPDL protocol database to be used by the compiler
	\param	useJit			flag for setting the NetVM Engine to use jitted code.

	\return a nbPacketEngine object if no errors occurred, NULL otherwise.
*/
nbPacketEngine DLL_EXPORT *nbAllocatePacketEngine(struct _nbNetPDLDatabase *NetPDLDatabase, unsigned char useJit);


/*!
	\brief  De-allocates the nbPacketEngine Object

	\param	packetEngine pointer to the object that has to be deallocated
*/
void DLL_EXPORT nbDeallocatePacketEngine(nbPacketEngine *packetEngine);
