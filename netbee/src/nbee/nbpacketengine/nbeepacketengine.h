/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <nbee_packetengine.h>
#include <nbee_extractedfieldreader.h>
#include "../globals/debug.h"

#include <stdlib.h>
#include <string.h>
#include <pcap.h>

#include <nbee.h>
#include <nbnetvm.h>


#define DATAINFO_BUF_SIZE 2048


//! This structure is used for communicating with the NetVM Application callback function
struct ExBufInfo
{
	int *Result;
	unsigned char *DataInfo;
	int *N;

	ExBufInfo(int *m_Result, unsigned char* datainfo,int *n):Result(m_Result), DataInfo(datainfo),N(n){}
};


//! Class for filtering and extracting selected fields from network packets.
class nbeePacketEngine:public nbPacketEngine
{

private:

	struct _nbNetPDLDatabase *m_NetPDLDatabase;	//!< Netpdl structure for the compiler initialization
	bool				m_UseJit;				//!< Jit Flag

	char				m_errbuf[2048];			//!< Buffer that keeps the last error message (if any)
	unsigned int		m_NumMsg;				//!< Number of error/warning messages generated during the initialization or filter compilation phases
	_nbNetPFLCompilerMessages		*m_MsgList;				//!< List of error/warning messages


	nbNetPFLCompiler	*m_Compiler;
	char				*m_GeneratedCode;
	int					n_field;
	ExBufInfo			*m_exbufinfo;

	//!\todo THIS SHOULD BE ALLOCATED AT RUNTIME
	unsigned char		m_Info[DATAINFO_BUF_SIZE];
	int					m_Result;
	nbExtractedFieldsReader	*m_fieldReader;
	_nbExtractedFieldsDescriptor *ExtractedFieldsDescriptorVector;
	_nbExtractFieldInfo *ExtractedField;
	_nbExtractFieldInfo *AllFieldsInfo;

	//NetVM related structures
	char				netvmErrBuf[nvmERRBUF_SIZE];
	nvmNetVM			*NetVM;
	nvmNetPE			*NetPE;
	nvmSocket			*SocketIn;
	nvmSocket			*SocketOut;
	nvmRuntimeEnvironment *NetVMRTEnv;
	nvmAppInterface		*InInterf;
	nvmAppInterface		*OutInterf;
	nvmByteCode			*BytecodeHandle;
	nbNetVMCreationFlag_t	m_creationFlag;


public:

	/*!
		\brief	Object constructor

		\param	NetPDLDatabase  pointer to the structure that contains the NetPDL Description
		\param	UseJit			flag for setting the NetVM Engine.

	*/
	nbeePacketEngine(struct _nbNetPDLDatabase *NetPDLDatabase, bool UseJit);

	/*!
		\brief	Object destructor
	*/
	~nbeePacketEngine(void);

	int Compile(const char* NetPFLFilterString, nbNetPDLLinkLayer_t LinkLayer, bool Opt);

	int InitNetVM(nbNetVMCreationFlag_t creationFlag);

	nbExtractedFieldsReader *GetExtractedFieldsReader();

	int ProcessPacket(const unsigned char *PktData, int PktLen);

	_nbNetPFLCompilerMessages *GetCompMessageList(void);

	char *GetLastError()
	{
		return m_errbuf;
	}

	char * GetCompiledCode();

	int GenerateBackendCode(int BackendId, bool Opt, bool Inline, const char* DumpFileName);

	char * GetAssemblyCode();

	int InjectCode(nbNetPDLLinkLayer_t LinkLayer, char *NetILCode, _nbExtractFieldInfo *ExtractedFieldsDescriptorInfo, int num_entry);

	void SetDebugLevel(const unsigned int FlowInformation= nbNETPFLCOMPILER_DEBUG_DETAIL_LEVEL);

	void SetNetILCodeFilename(const char *DumpNetILCodeFilename= nbNETPFLCOMPILER_DEBUG_NETIL_CODE_FILENAME);

	void SetHIRCodeFilename(const char *DumpHIRCodeFilename= nbNETPFLCOMPILER_DEBUG_HIR_CODE_FILENAME);

	void SetLIRCodeFilename(const char *DumpLIRCodeFilename= nbNETPFLCOMPILER_DEBUG_LIR_CODE_FILENAME);

	void SetLIRGraphFilename(const char *DumpLIRGraphFilename= nbNETPFLCOMPILER_DEBUG_LIR_GRAPH_FILENAME);

	void SetLIRNoOptGraphFilename(const char *DumpLIRNoOptGraphFilename= nbNETPFLCOMPILER_DEBUG_LIR_NOOPT_GRAPH_FILENAME);

	void SetNoCodeGraphFilename(const char *DumpNoCodeGraphFilename= nbNETPFLCOMPILER_DEBUG_NO_CODE_GRAPH_FILENAME);

	void SetNetILGraphFilename(const char *DumpNetILGraphFilename= nbNETPFLCOMPILER_DEBUG_NETIL_GRAPH_FILENAME);

	void SetProtoGraphFilename(const char *DumpProtoGraphFilename= nbNETPFLCOMPILER_DEBUG_PROTOGRAH_DUMP_FILENAME);
	
	void SetFilterAutomatonFilename(const char *DumpFilterAutomatonFilename=  nbNETPFLCOMPILER_DEBUG_FILTERAUTOMATON_DUMP_FILENAME);

	nvmRuntimeEnvironment *GetNetVMRuntimeEnvironment(void);
};

