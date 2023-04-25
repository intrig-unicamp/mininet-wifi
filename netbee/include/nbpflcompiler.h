/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once




/*!
	\mainpage NetPFL Compiler Module

	\section intro Introduction
 
	This document describes the data structures and functions that are exported externally by the NetPFL
	compiler module. This module is able to compile a NetPFL (Network Packet Filtering Language) string
	into a set of NetIL instructions for the NetVM; filters are based on the protocol definition contained
	in a NetPDL database.

	Please note that most programmers do not need to link against this library, because functions exported here
	are pretty low-level. A more user-friendly wrapper is available in the NetBee library; programmers are encouraged
	in using that library instead of this one.

	This module exports the nbNetPFLCompiler class, which can be used to create the NetPFL compiler.
	Additional data structures are also exported in order to facilitate the parsing of the data
	produced internally by the NetVM engine (e.g. the set of values related to the fields extracted
	from the packet, and more).

	The nbNetPFLCompiler must be created using the nbAllocateNetPFLCompiler() function, and it will be
	deallocated through the nbDeallocateNetPFLCompiler().
 */



#include <nbprotodb_exports.h>
#include <nbpflcompiler_exports.h>


#include <string>
#include <list>
using namespace std;


/*!
	\file nbpflcompiler.h

	This file defines the classes and the macros that are exported by this module.
*/


class NetPFLFrontEnd;	//forward declaration
class ErrorRecorder;	//forward declaration


#define N_ALLFIELDS 128 //!< Size of the Allfields DescriptorVector
#define INFO_FIELDS_SIZE 4 //!< number of bytes for each fields in the info-partition
#define INFO_FIELDS_SIZE_ALL 6 //!< number of bytes for each "allfields" fields in the info-partition


#define nbNETPFLCOMPILER_MAX_ALLFIELDS 128		//!< Size of the _nbExtractedFieldsDescriptorVector when the 'allfields' keyword is used for extractinf data
#define nbNETPFLCOMPILER_INFO_FIELDS_SIZE 4		//!< Number of bytes dedicated to each fields in the info-partition
#define nbNETPFLCOMPILER_INFO_FIELDS_SIZE_ALL 6 //!< Number of bytes dedicated to each 'allfields' fields in the info-partition

#define nbNETPFLCOMPILER_MAX_PROTO_INSTANCES 5 //!< Maximum number of occurrences of a single protocol handled in the info-partition
#define nbNETPFLCOMPILER_MAX_FIELD_INSTANCES 5 //!< Maximum number of occurrences of the same field handled in the info-partition




// Please remember that the same #define are present in the "Globals.h" header
// So, please be careful to keep them aligned
#ifndef nbSUCCESS
	#define nbSUCCESS 0		//!< Return code for 'success'
	#define nbFAILURE -1	//!< Return code for 'failure'
	#define nbWARNING -2	//!< Return code for 'warning' (we can go on, but something wrong occurred)
#endif


#ifdef PFLCOMPILER_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif

// class 'std::list<_Ty>' needs to have dll-interface to be used by clients of class (nbNetPFLCompiler::exFieldList)
#ifdef WIN32
#pragma warning(disable: 4251)
#endif


struct SymbolField;     //forward declaration
//struct _nbExtractedFieldsDescriptorVector; //forward declaration
typedef list<SymbolField*> FieldsList_t;

/*!
	\brief This class implements the NetPFL Compiler
	
	This component is a frontent to the NetVM framework and it generates the 
	NetIL code for packet filters and field extractors.
	This component implements only the first step of teh code generation: for optimizations,
	code lowering, etc, other components already available in the NetVM framework are used.
*/

class DLL_EXPORT nbNetPFLCompiler
{

private:
	unsigned int		NumMsg;					//!< Number of error/warning messages generated during the initialization or filter compilation phases
	_nbNetPFLCompilerMessages		*MsgList;	//!< List of error/warning messages
	bool				PDLInited;				//!< Tells if the compiler has been initialized or not
	_nbNetPDLDatabase	&ProtoDB;				//!< Reference to the NetPDL database used to compile the filter
	NetPFLFrontEnd		*PFLFrontEnd;			//!< Reference to the Compiler's front end
	char				*GenCode;				//!< Char string holding the generated NetIL code
	unsigned int		CodeSize;
	char					m_errbuf[2048];	//!< Buffer that keeps the last error message (if any)

	FieldsList_t			exFieldList;

	/*!
		\brief Fills the message list

		\param errRecorder	reference to the internal Error Recorder

		\return nbSUCCESS if no error occurred, nbFAILURE otherwise

	*/
	int FillMsgList(ErrorRecorder &errRecorder);

	/*!
		\brief Empties the message list
	*/
	void ClearMsgList(void);

	/*!
		\brief Destroys the internal Front End object
	*/
	void NetPDLCleanup(void);

	// TODO [OM] I think this method is not useful
	int IsInitialized(void);


	unsigned int m_debugLevel;
	char *dumpHIRCodeFilename;
	char *dumpLIRCodeFilename;
	char *dumpNetILCodeFilename;
	char *dumpLIRGraphFilename;
	char *dumpLIRNoOptGraphFilename;
	char *dumpNetILGraphFilename;
	char *dumpNoCodeGraphFilename;
	char *dumpProtoGraphFilename;
	char *dumpFilterAutomatonFilename;

public:
	/*!
		\brief	Object constructor

		\param	protoDB	reference to the NetPDL protocol database

		\return nothing
	*/

	nbNetPFLCompiler(_nbNetPDLDatabase &protoDB);



	/*!
		\brief	Object destructor
	*/
	~nbNetPFLCompiler();

	/*!
		\brief Returns a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	char *GetLastError()
	{
		return m_errbuf;
	}


	/*!
		\brief Initializes the NetPFL Compiler

		During the initialization phase, the NetPDL database is parsed and its correctness is verified

		\param LinkLayer		Link layer type of the packets we want to filter (e.g., Ethernet).
		This value can be read from the dump file that contains the packets we want to filter
		or from the network interface actually in use for capturing packets. Allowed values are
		listed in #nbNetPDLLinkLayer_t.

		\return nbSUCCESS if no error occurred, nbFAILURE otherwise
	*/

	int NetPDLInit(nbNetPDLLinkLayer_t LinkLayer);

	/*!
		\brief Checks the syntax of a NetPFL filter, withouth full compilation.

		This function checks only if the NetPFL filter syntax is correct, but
		it does not generate any code.

		\param NetPFLFilterString		Filter string, in the NetPFL language
		\return nbSUCCESS if no error occurred, nbFAILURE otherwise
	*/

	int CheckFilter(const char *NetPFLFilterString);
	
		/*!
		\brief Prints the final state automaton related to the filtering expression

		\param AutomatonFilename	Name of the .dot output file
	*/

	
	void PrintFSA(const char *AutomatonFilename);

	/*!
		\brief Compiles a NetPFL filter

		\param NetPFLFilterString	Filter string, in the NetPFL language
		\param NetILCode			String holding the generated NetIL code
		\param optimizationCycles	Flag to set the optimizations
		\return nbSUCCESS if no error occurred, nbFAILURE otherwise
	*/

	int CompileFilter(const char *NetPFLFilterString, char **NetILCode, bool optimizationCycles=true);
	
	/*!
		\brief Create the final state automaton for a NetPFL filter

		\param NetPFLFilterString	Filter string, in the NetPFL language
	*/
	
	int CreateAutomatonFromFilter(const char *NetPFLFilterString);

	/*!
		\brief Returns the length of the character array containing the generated NetIL filter
		\return The size of the NetIL code string.
	*/
	unsigned int GetCodeSize(void)
	{
		return CodeSize;
	}
	/*!
		\brief Gets the list of error/warning messages generated during the initialization or compilation phases

		\return a list of _nbNetPFLCompilerMessages structures.
	*/

	_nbNetPFLCompilerMessages *GetCompMessageList(void);

	/*!
		\brief Gets the vector of descriptors generated during compilation phases,
		it fills the vector with the basic information such as kind, name, protocol, type and
		if it's a field descriptor also the length.

		\return A _nbExtractedFieldsDescriptorVector structure.

		\note The _nbExtractedFieldsDescriptorVector will be empty in case the ExtractFields action
		is not present in the filter string.
	*/
	_nbExtractFieldInfo* GetExtractField(int *num_entry);

	/*!
		\brief Fills the basic information of the field, indexed by id, in the descriptor structure

		\param protoName		Protocol name
		\param id				Field id
		\param des				Descriptor field

		\return nbSUCCESS if the field's id is valid for the protocol, nbFAILURE otherwise
	*/
	int GetFieldInfo(const string protoName, uint32_t id, _nbExtractFieldInfo* des);
	
	/*!
		\brief Sets the flow information detail level
		\param FlowInformation Debug detail level (0,1 or 2)
	*/
	void SetDebugLevel(const unsigned int DebugLevel= nbNETPFLCOMPILER_DEBUG_DETAIL_LEVEL);

	/*!
		\brief Sets the dump file for the NETIL code
		\param DumpNetILCodeFilename File name.
	*/
	void SetNetILCodeFilename(const char *DumpNetILCodeFilename= nbNETPFLCOMPILER_DEBUG_NETIL_CODE_FILENAME);
	
	/*!
		\brief Sets the dump file for the HIR code
		\param DumpHIRCodeFilename File name.
	*/
	void SetHIRCodeFilename(const char *DumpHIRCodeFilename= nbNETPFLCOMPILER_DEBUG_HIR_CODE_FILENAME);

	/*!
		\brief Sets the dump file for the LIR code
		\param DumpLIRCodeFilename File name.
	*/
	void SetLIRCodeFilename(const char *DumpLIRCodeFilename= nbNETPFLCOMPILER_DEBUG_LIR_CODE_FILENAME);

	/*!
		\brief Sets the dump file for the LIR graph
		\param DumpLIRGraphFilename File name.
	*/
	void SetLIRGraphFilename(const char *dumpLIRGraphFilename= nbNETPFLCOMPILER_DEBUG_LIR_GRAPH_FILENAME);
	
	/*!
		\brief Sets the dump file for the NoOpt graph
		\param DumpLIRNoOptGraphFilename File name.
	*/
	void SetLIRNoOptGraphFilename(const char *DumpLIRNoOptGraphFilename= nbNETPFLCOMPILER_DEBUG_LIR_NOOPT_GRAPH_FILENAME);
	
	/*!
		\brief Sets the dump file for the NoCode graph
		\param DumpNoCodeGraphFilename File name.
	*/
	void SetNoCodeGraphFilename(const char *DumpNoCodeGraphFilename= nbNETPFLCOMPILER_DEBUG_NO_CODE_GRAPH_FILENAME);

	/*!
		\brief Sets the dump file for the NETIL graph
		\param DumpNetILGraphFilename File name.
	*/
	void SetNetILGraphFilename(const char *DumpNetILGraphFilename= nbNETPFLCOMPILER_DEBUG_NETIL_GRAPH_FILENAME);
	
	/*!
		\brief Sets the dump file for the Proto graph
		\param DumpProtoGraphFilename File name.
	*/
	void SetProtoGraphFilename(const char *dumpProtoGraphFilename= nbNETPFLCOMPILER_DEBUG_PROTOGRAH_DUMP_FILENAME);
	
	/*!
		\brief Sets the dump file for the atuomaton representing the filter
		\param DumpFitlerAutomatonFilename File name.
	*/
	void SetFilterAutomatonFilename(const char *dumpFilterAutomatonFilename=  nbNETPFLCOMPILER_DEBUG_FILTERAUTOMATON_DUMP_FILENAME);

};


/*!
	\brief Creates a nbNetPFLCompiler object
	\param NetPDLProtoDB NetPDL protocol database to be used by the compiler.
	\return A nbNetPFLCompiler object.
*/
DLL_EXPORT nbNetPFLCompiler *nbAllocateNetPFLCompiler(_nbNetPDLDatabase *NetPDLProtoDB);


/*!
	\brief Deallocates the nbNetPFLCompiler object
	\param NetPFLCompiler Pointer to a nbNetPFLCompiler object.
*/
DLL_EXPORT void nbDeallocateNetPFLCompiler(nbNetPFLCompiler *NetPFLCompiler);


