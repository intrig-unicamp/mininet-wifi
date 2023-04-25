/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "nbeepacketengine.h"
#include "nbeefieldreader.h"




/*
 * The callback function registered onto the output interface of NetVM
 * sets to true the "packet accepted" flag and copies the entire info
 * partition in a private buffer of the packet-engine.
 * The "packet accepted" flag is held by the ExBufInfo structure, which is passed
 * through the UserData member of the exchange buffer when the packet is pushed into NetVM.
 *
 */


int32_t ResultInfoCallback(nvmExchangeBuffer *xbuffer)
{
	ExBufInfo *exbufInfo= (ExBufInfo*)xbuffer->UserData;
	*(exbufInfo->Result)=nbSUCCESS;
	memcpy(exbufInfo->DataInfo,xbuffer->InfoData,xbuffer->InfoLen);
	return nbSUCCESS;
}

nbeePacketEngine::nbeePacketEngine(struct _nbNetPDLDatabase *NetPDLDatabase, bool UseJit):
m_NetPDLDatabase(NetPDLDatabase), m_UseJit(UseJit)
{
	m_Compiler = nbAllocateNetPFLCompiler(m_NetPDLDatabase);
	m_fieldReader=NULL;
	netvmErrBuf[0] = '\0';
	NetVM=NULL;
	NetPE=NULL;
	SocketIn=NULL;
	SocketOut=NULL;
	NetVMRTEnv=NULL;
	BytecodeHandle = NULL;
	n_field=1;
	ExtractedFieldsDescriptorVector = NULL;
	ExtractedField = NULL;
	AllFieldsInfo = NULL;
	m_exbufinfo= new ExBufInfo(&m_Result,m_Info,&n_field);
}


nbeePacketEngine::~nbeePacketEngine(void)
{
	if (m_fieldReader)
	{
		delete m_fieldReader;
		m_fieldReader= NULL;
	}
	
	if(ExtractedFieldsDescriptorVector)
	{
		for(int i = 0; i < n_field; i++)
		{
			if(ExtractedFieldsDescriptorVector[i].DVct)
				delete []ExtractedFieldsDescriptorVector[i].DVct;
		}
		if(AllFieldsInfo)
		{
			delete []ExtractedFieldsDescriptorVector[n_field].DVct;
			delete []AllFieldsInfo;
			AllFieldsInfo = NULL;
		}	
		delete []ExtractedFieldsDescriptorVector;
		ExtractedFieldsDescriptorVector = NULL;
	}
	
	if (ExtractedField)
	{
		delete []ExtractedField;
		ExtractedField =NULL;
	}
	
	if(NetVM)
		nvmDestroyVM(NetVM);

	if(NetVMRTEnv)
		nvmDestroyRTEnv(NetVMRTEnv);

	nbCleanup();
}

int32_t nbeePacketEngine::InitNetVM(nbNetVMCreationFlag_t CreationFlag)
{
	m_creationFlag= CreationFlag;

	if (NetVM)
		nvmDestroyVM(NetVM);

	if (NetVMRTEnv)
		nvmDestroyRTEnv(NetVMRTEnv);

	NetVM = nvmCreateVM(0, netvmErrBuf);
	if(NetVM == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	BytecodeHandle = nvmAssembleNetILFromBuffer(m_GeneratedCode, netvmErrBuf);
	if (BytecodeHandle == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	NetPE = nvmCreatePE(NetVM, BytecodeHandle, netvmErrBuf);
	if (NetPE == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	free(BytecodeHandle);

	SocketIn = nvmCreateSocket(NetVM, netvmErrBuf);
	if (SocketIn == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	SocketOut = nvmCreateSocket(NetVM, netvmErrBuf);
	if (SocketOut == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	//connect the Input socket to the port 0 of the NetPE
	if (nvmConnectSocket2PE(NetVM, SocketIn, NetPE, 0, netvmErrBuf) == nvmFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	//connect the Output socket to the port 1 of the NetPE
	if (nvmConnectSocket2PE(NetVM, SocketOut, NetPE, 1, netvmErrBuf) == nvmFAILURE)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	//create the NetVM Runtime environment

	NetVMRTEnv = nvmCreateRTEnv(NetVM, CreationFlag, netvmErrBuf);
	if (NetVMRTEnv == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	//create an input Application Interface (i.e. a control plane input interface where we can collect packets from)
	InInterf = nvmCreateAppInterfacePushIN(NetVMRTEnv, netvmErrBuf);
	if (InInterf == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	//create an output Application Interface (i.e. a control plane output interface where we can write packets on)
	OutInterf= nvmCreateAppInterfacePushOUT(NetVMRTEnv, ResultInfoCallback, netvmErrBuf);
	if (OutInterf == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
		return nbFAILURE;
	}

	//bind the input interface to the input socket
	if (nvmBindAppInterf2Socket(InInterf,SocketIn)!= nvmSUCCESS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Input Interface binding failed");
		return nbFAILURE;
	}
	//bind the output interface to the output socket
	if (nvmBindAppInterf2Socket(OutInterf,SocketOut)!= nvmSUCCESS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Output Interface binding failed");
		return nbFAILURE;
	}

	if (m_creationFlag == nbNETVM_CREATION_FLAG_COMPILEANDEXECUTE)
	{
		if (nvmNetStart(NetVM, NetVMRTEnv, m_UseJit, nvmDO_NATIVE | nvmDO_BCHECK, 3, netvmErrBuf) != nvmSUCCESS)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), netvmErrBuf);
			return nbFAILURE;
		}
	}

	return nbSUCCESS;

}


int nbeePacketEngine::Compile(const char *NetPFLFilterString, nbNetPDLLinkLayer_t LinkLayer, bool Opt)
{
	int RetVal, num_entry;

	if (m_Compiler == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler Allocation Failed");
		return nbFAILURE;
	}

	RetVal= m_Compiler->NetPDLInit(LinkLayer);

	if (RetVal != nbSUCCESS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), m_Compiler->GetLastError());
		return nbFAILURE;
	}

	m_GeneratedCode= NULL;

	RetVal = m_Compiler->CompileFilter(NetPFLFilterString, &m_GeneratedCode, Opt);
	if (RetVal != nbSUCCESS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), m_Compiler->GetLastError());
		return nbFAILURE;
	}

	ExtractedField= m_Compiler->GetExtractField(&num_entry);
	if(num_entry > 0)
	{
		if (m_fieldReader)
		{
			delete m_fieldReader;
			m_fieldReader=NULL;
		}
	   ExtractedFieldsDescriptorVector = new _nbExtractedFieldsDescriptor[num_entry];
	   for(int i = 0; i < num_entry; i++)
	   {
	   	if(ExtractedField[i].FieldType == PDL_FIELD_TYPE_ALLFIELDS)
	   	{
	   		ExtractedFieldsDescriptorVector[i].DVct = new _nbExtractedFieldsDescriptor[nbNETPFLCOMPILER_MAX_ALLFIELDS];
	   		AllFieldsInfo = new _nbExtractFieldInfo[nbNETPFLCOMPILER_MAX_ALLFIELDS];
	  			ExtractedFieldsDescriptorVector[i].num_entries = nbNETPFLCOMPILER_MAX_ALLFIELDS;
	   	}else if(ExtractedField[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
	   	{
	   		ExtractedFieldsDescriptorVector[i].DVct = new _nbExtractedFieldsDescriptor[nbNETPFLCOMPILER_MAX_FIELD_INSTANCES];
	   		ExtractedFieldsDescriptorVector[i].num_entries = nbNETPFLCOMPILER_MAX_FIELD_INSTANCES;
	   	}else if(ExtractedField[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO)
	   	{
	   		ExtractedFieldsDescriptorVector[i].DVct = new _nbExtractedFieldsDescriptor[nbNETPFLCOMPILER_MAX_PROTO_INSTANCES];
	   		ExtractedFieldsDescriptorVector[i].num_entries = nbNETPFLCOMPILER_MAX_PROTO_INSTANCES;
	   	}
	   }
		m_fieldReader= new nbeeFieldReader(ExtractedFieldsDescriptorVector, ExtractedField, AllFieldsInfo, num_entry, m_Info, m_Compiler);

		// Store the number of fields to extract (except allfields)
		if (ExtractedField[num_entry - 1].FieldType == PDL_FIELD_TYPE_ALLFIELDS)
			n_field= num_entry - 1;
		else
			n_field= num_entry;
	}

	return RetVal;
}


int nbeePacketEngine::ProcessPacket(const unsigned char *PktData, int PktLen)
{
	m_Result=nbFAILURE;

	nvmWriteAppInterface(InInterf, (uint8_t*)PktData, (uint32_t)PktLen, m_exbufinfo , netvmErrBuf);
	
	
	if(m_fieldReader && m_Result!= nbFAILURE)
		m_fieldReader->FillDescriptors();
		

	return m_Result;
}


nbExtractedFieldsReader *nbeePacketEngine::GetExtractedFieldsReader()
{
 return m_fieldReader;
}


char *nbeePacketEngine::GetCompiledCode()
{
	return m_GeneratedCode;
}


int nbeePacketEngine::GenerateBackendCode(int BackendId, bool Opt, bool Inline, const char* DumpFileName)
{
	if (m_creationFlag == nbNETVM_CREATION_FLAG_COMPILEONLY)
	{
	int OptLevel= 0;
	int NetVMFlags;

		if (Opt == true)
			OptLevel= 3;

		// Let's determine if we support bound checking or not
		uint32_t NumBackends;
		nvmBackendDescriptor *BackendList= nvmGetBackendList(&NumBackends);
		
		if ((BackendId < 1) || (NumBackends > (unsigned int) BackendId))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetVM internal error: requested a backend with a wrong code.");
			return nbFAILURE;
		}

		// Set the bound checking if the backend supports it
		if (BackendList[BackendId-1].Flags & nvmDO_BCHECK)
			NetVMFlags= nvmDO_ASSEMBLY | nvmDO_BCHECK;
		else
			NetVMFlags= nvmDO_ASSEMBLY;

		if (Inline)
			NetVMFlags |= nvmDO_INLINE;

		if (nvmCompileApplication(NetVM,
					NetVMRTEnv,
					BackendId - 1,
					NetVMFlags,
					OptLevel,
					DumpFileName,
					netvmErrBuf) != nvmSUCCESS)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetVM Runtime - Error initializing the runtime environment");
			return nbFAILURE;
		}
	}
	else
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "GenerateBackendCode can be used only if the CreationFlag is COMPILEONLY");
		return nbFAILURE;
	}


	return nbSUCCESS;

}



char *nbeePacketEngine::GetAssemblyCode()
{
	return nvmGetTargetCode(NetVMRTEnv);
}



int nbeePacketEngine::InjectCode(nbNetPDLLinkLayer_t LinkLayer, char *NetILCode, _nbExtractFieldInfo *ExtractedFieldsDescriptorInfo, int num_entry)
{
int RetVal;

	if (m_Compiler == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler Allocation Failed");
		return nbFAILURE;
	}

	RetVal= m_Compiler->NetPDLInit(LinkLayer);

	if (RetVal != nbSUCCESS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), m_Compiler->GetLastError());
		return nbFAILURE;
	}

	m_GeneratedCode= NetILCode;

	if (m_fieldReader)
	{	
		delete m_fieldReader;
		m_fieldReader= NULL;
	}
	
	if(ExtractedFieldsDescriptorVector != NULL)
	{
		for(int i = 0; i < n_field; i++)
		{
			if(ExtractedFieldsDescriptorVector[i].num_entries > 0)
				delete []ExtractedFieldsDescriptorVector[i].DVct;
		}
		
		if(AllFieldsInfo != NULL)
		{
			delete []ExtractedFieldsDescriptorVector[n_field].DVct;
			delete []AllFieldsInfo;
			AllFieldsInfo = NULL;
		}
			
		delete []ExtractedFieldsDescriptorVector;
		ExtractedFieldsDescriptorVector = NULL;
	}
	
	if(ExtractedField != NULL)
	{
		delete []ExtractedField;
		ExtractedField = NULL;
	}
	
	ExtractedField= ExtractedFieldsDescriptorInfo;
	if(num_entry > 0)
	{
	   ExtractedFieldsDescriptorVector = new _nbExtractedFieldsDescriptor[num_entry];
	   for(int i = 0; i < num_entry; i++)
	   {
	   	if(ExtractedField[i].FieldType == PDL_FIELD_TYPE_ALLFIELDS)
	   	{
	   		ExtractedFieldsDescriptorVector[i].DVct = new _nbExtractedFieldsDescriptor[nbNETPFLCOMPILER_MAX_ALLFIELDS];
	   		AllFieldsInfo = new _nbExtractFieldInfo[nbNETPFLCOMPILER_MAX_ALLFIELDS];
	   		ExtractedFieldsDescriptorVector[i].num_entries = nbNETPFLCOMPILER_MAX_ALLFIELDS;
	   	}else if(ExtractedField[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
	   	{
	   		ExtractedFieldsDescriptorVector[i].DVct = new _nbExtractedFieldsDescriptor[nbNETPFLCOMPILER_MAX_FIELD_INSTANCES];
	   		ExtractedFieldsDescriptorVector[i].num_entries = nbNETPFLCOMPILER_MAX_FIELD_INSTANCES;
	   	}else if(ExtractedField[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO)
	   	{
	   		ExtractedFieldsDescriptorVector[i].DVct = new _nbExtractedFieldsDescriptor[nbNETPFLCOMPILER_MAX_PROTO_INSTANCES];
	   		ExtractedFieldsDescriptorVector[i].num_entries = nbNETPFLCOMPILER_MAX_PROTO_INSTANCES;
	   	}
	   }
	   
		m_fieldReader= new nbeeFieldReader(ExtractedFieldsDescriptorVector, ExtractedField, AllFieldsInfo, num_entry, m_Info, m_Compiler);
	}

	if (ExtractedFieldsDescriptorInfo[num_entry - 1].FieldType == PDL_FIELD_TYPE_ALLFIELDS)
		n_field= num_entry - 1;
	else
		n_field= num_entry;

	return RetVal;
}


void nbeePacketEngine::SetDebugLevel(const unsigned int DebugLevel)
{
	if (m_Compiler)
		m_Compiler->SetDebugLevel(DebugLevel);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");

}

void nbeePacketEngine::SetNetILCodeFilename(const char *dumpNetILCodeFilename)
{
	if (m_Compiler)
		m_Compiler->SetNetILCodeFilename(dumpNetILCodeFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetHIRCodeFilename(const char *dumpHIRCodeFilename)
{
	if (m_Compiler)
		m_Compiler->SetHIRCodeFilename(dumpHIRCodeFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetLIRGraphFilename(const char *dumpLIRGraphFilename)
{
	if (m_Compiler)
		m_Compiler->SetLIRGraphFilename(dumpLIRGraphFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetLIRCodeFilename(const char *dumpLIRCodeFilename)
{
	if (m_Compiler)
		m_Compiler->SetLIRCodeFilename(dumpLIRCodeFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetLIRNoOptGraphFilename(const char *dumpLIRNoOptGraphFilename)
{
	if (m_Compiler)
		m_Compiler->SetLIRNoOptGraphFilename(dumpLIRNoOptGraphFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetNoCodeGraphFilename(const char *dumpNoCodeGraphFilename)
{
	if(m_Compiler!=NULL)
		m_Compiler->SetNoCodeGraphFilename(dumpNoCodeGraphFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetNetILGraphFilename(const char *dumpNetILGraphFilename)
{
	if (m_Compiler)
		m_Compiler->SetNetILGraphFilename(dumpNetILGraphFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetProtoGraphFilename(const char *dumpProtoGraphFilename)
{
	if (m_Compiler)
		m_Compiler->SetProtoGraphFilename(dumpProtoGraphFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}

void nbeePacketEngine::SetFilterAutomatonFilename(const char *dumpFilterAutomatonFilename)
{
	if (m_Compiler)
		m_Compiler->SetFilterAutomatonFilename(dumpFilterAutomatonFilename);
	else
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "m_Compiler not initialized");
}


_nbNetPFLCompilerMessages *nbeePacketEngine::GetCompMessageList(void)
{
	return m_Compiler->GetCompMessageList();
}

nvmRuntimeEnvironment *nbeePacketEngine::GetNetVMRuntimeEnvironment(void)
{
  return NetVMRTEnv;
}


