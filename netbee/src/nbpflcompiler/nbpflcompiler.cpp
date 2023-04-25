/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "nbpflcompiler.h"
#include "netpflfrontend.h"
#include "errors.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"




nbNetPFLCompiler *nbAllocateNetPFLCompiler(_nbNetPDLDatabase *NetPDLProtoDB)
{
	if (!NetPDLProtoDB)
		return NULL;

	return new nbNetPFLCompiler(*NetPDLProtoDB);
}

void nbDeallocateNetPFLCompiler(nbNetPFLCompiler *NetPFLCompiler)
{	
	if (NetPFLCompiler)
	{
		delete NetPFLCompiler;
	}
}


nbNetPFLCompiler::nbNetPFLCompiler(_nbNetPDLDatabase &protoDB)
: NumMsg(0), MsgList(0), PDLInited(0), ProtoDB(protoDB), PFLFrontEnd(0), GenCode(0), CodeSize(0)
{
#ifdef _DEBUG
	m_debugLevel= nbNETPFLCOMPILER_DEBUG_DETAIL_LEVEL;
	dumpHIRCodeFilename= nbNETPFLCOMPILER_DEBUG_HIR_CODE_FILENAME;
	dumpLIRCodeFilename= NULL;
	dumpLIRGraphFilename= NULL;
	dumpLIRNoOptGraphFilename= nbNETPFLCOMPILER_DEBUG_LIR_NOOPT_GRAPH_FILENAME;
	dumpNetILCodeFilename= NULL;
	dumpNetILGraphFilename= NULL;
	dumpNoCodeGraphFilename= NULL;
	dumpProtoGraphFilename= nbNETPFLCOMPILER_DEBUG_PROTOGRAH_DUMP_FILENAME;
	dumpFilterAutomatonFilename= nbNETPFLCOMPILER_DEBUG_FILTERAUTOMATON_DUMP_FILENAME;
#else
	m_debugLevel= nbNETPFLCOMPILER_DEBUG_DETAIL_LEVEL;
	dumpHIRCodeFilename= NULL;
	dumpLIRCodeFilename= NULL;
	dumpLIRGraphFilename= NULL;
	dumpLIRNoOptGraphFilename= NULL;
	dumpNetILCodeFilename= NULL;
	dumpNetILGraphFilename= NULL;
	dumpNoCodeGraphFilename= NULL;
	dumpProtoGraphFilename= NULL;
	dumpFilterAutomatonFilename= NULL;
#endif

}


nbNetPFLCompiler::~nbNetPFLCompiler()
{
	if (GenCode)
		delete []GenCode;
	
	NetPDLCleanup();
}

void nbNetPFLCompiler::NetPDLCleanup(void)
{
	if (this->m_debugLevel > 2)
		nbPrintDebugLine("Cleaning NetPFL Front End", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 2);
	if (PFLFrontEnd)
		delete PFLFrontEnd;
	PFLFrontEnd= NULL;
	PDLInited= false;
}


int nbNetPFLCompiler::FillMsgList(ErrorRecorder &errRecorder)
{
	NumMsg = 0;
	if (errRecorder.TotalErrors() > 0)
	{
		list<ErrorInfo> &compErrList = errRecorder.GetErrorList();
		list<ErrorInfo>::iterator i = compErrList.begin();
		MsgList = 0;
		_nbNetPFLCompilerMessages *current=0;
		char errType[20];

		while (i != compErrList.end())
		{
			_nbNetPFLCompilerMessages *message = new _nbNetPFLCompilerMessages;
			if (message == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
				return nbFAILURE;
			}
			memset(message->MessageString, 0, nbNETPFLCOMPILER_MAX_MESSAGE);
			switch ((*i).Type) {
				case ERR_PFL_ERROR:
					sprintf(errType, "PFL error");
					break;
				case ERR_PDL_ERROR:
					sprintf(errType, "PDL error");
					break;
				case ERR_PFL_WARNING:
					sprintf(errType, "PFL warning");
					break;
				case ERR_PDL_WARNING:
					sprintf(errType, "PDL warning");
					break;
				case ERR_FATAL_ERROR:
					sprintf(errType, "Fatal error");
					break;
			}
			snprintf(message->MessageString, nbNETPFLCOMPILER_MAX_MESSAGE - 1, "[%s] %s", errType, (*i).Msg.c_str());
			message->Next = 0;
			if (MsgList == NULL)
				MsgList = message;
			else
				current->Next=message;

			current=message;

			NumMsg++;
			i++;
		}
	}
	return nbSUCCESS;
}


void nbNetPFLCompiler::ClearMsgList(void)
{
	NumMsg = 0;
	_nbNetPFLCompilerMessages *next = MsgList;
	while(next)
	{
		_nbNetPFLCompilerMessages *current = next;
		next = current->Next;
		delete current;
	}
	MsgList = NULL;
}


int nbNetPFLCompiler::IsInitialized(void)
{
	if (PDLInited)
		return nbSUCCESS;

	return nbFAILURE;
}

void nbNetPFLCompiler::SetDebugLevel(const unsigned int DebugLevel)
{
	m_debugLevel= DebugLevel;
}

void nbNetPFLCompiler::SetNetILCodeFilename(const char *DumpNetILCodeFilename)
{
	dumpNetILCodeFilename= (char*)DumpNetILCodeFilename;
}

void nbNetPFLCompiler::SetHIRCodeFilename(const char *DumpHIRCodeFilename)
{
	dumpHIRCodeFilename= (char*)DumpHIRCodeFilename;
}

void nbNetPFLCompiler::SetLIRGraphFilename(const char *DumpLIRGraphFilename)
{
	dumpLIRGraphFilename= (char*)DumpLIRGraphFilename;
}

void nbNetPFLCompiler::SetLIRCodeFilename(const char *DumpLIRCodeFilename)
{
	dumpLIRCodeFilename= (char*) DumpLIRCodeFilename;
}

void nbNetPFLCompiler::SetLIRNoOptGraphFilename(const char *DumpLIRNoOptGraphFilename)
{
	dumpLIRNoOptGraphFilename= (char*)DumpLIRNoOptGraphFilename;
}

void nbNetPFLCompiler::SetNoCodeGraphFilename(const char *DumpNoCodeGraphFilename)
{
	dumpNoCodeGraphFilename= (char*)DumpNoCodeGraphFilename;
}

void nbNetPFLCompiler::SetNetILGraphFilename(const char *DumpNetILGraphFilename)
{
	dumpNetILGraphFilename= (char*)DumpNetILGraphFilename;
}

void nbNetPFLCompiler::SetProtoGraphFilename(const char *DumpProtoGraphFilename)
{
	dumpProtoGraphFilename= (char*)DumpProtoGraphFilename;
}

void nbNetPFLCompiler::SetFilterAutomatonFilename(const char *DumpFilterAutomatonFilename)
{
	dumpFilterAutomatonFilename= (char*)DumpFilterAutomatonFilename;
}


int nbNetPFLCompiler::NetPDLInit(nbNetPDLLinkLayer_t LinkLayer)
{
	ClearMsgList();

	if (PDLInited || PFLFrontEnd)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL Compiler Engine has already been initialized, please use NetPDLCleanup()");
		return nbFAILURE;
	}

	try
	{
		PFLFrontEnd = new NetPFLFrontEnd(ProtoDB, LinkLayer, m_debugLevel, dumpHIRCodeFilename, dumpLIRNoOptGraphFilename, dumpProtoGraphFilename, dumpFilterAutomatonFilename);
		if (PFLFrontEnd == 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
			NetPDLCleanup();
			return nbFAILURE;
		}
		ErrorRecorder &errRecorder = PFLFrontEnd->GetErrRecorder();
		if (FillMsgList(errRecorder) != nbSUCCESS)
		{
			NetPDLCleanup();
			return nbFAILURE;
		}

		if (errRecorder.NumErrors() > 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed to compile the NetPDL description");
			NetPDLCleanup();
			return nbFAILURE;
		}
		PDLInited = true;
		return nbSUCCESS;
	}

	catch (ErrorInfo &e)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Compiler Internal Error: ", e.Msg.c_str());
		NetPDLCleanup();
		return nbFAILURE;
	}
}

_nbNetPFLCompilerMessages *nbNetPFLCompiler::GetCompMessageList(void)
{
	return MsgList;
}

void nbNetPFLCompiler::PrintFSA(const char *AutomatonFilename)
{
	PFLFrontEnd->PrintFinalAutomaton(AutomatonFilename);
}

int nbNetPFLCompiler::CheckFilter(const char *NetPFLFilterString)
{
bool RetVal;

	if (m_debugLevel > 1)
		nbPrintDebugLine("Checking filter syntax...", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 1);

	ClearMsgList();

	if (!(PDLInited && PFLFrontEnd))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL Compiler Engine has not been initialized, please use NetPDLInit()");
		return nbFAILURE;
	}

	if (!NetPFLFilterString)
		RetVal= PFLFrontEnd->CheckFilter("");
	else
		RetVal= PFLFrontEnd->CheckFilter(NetPFLFilterString);

	ErrorRecorder &errRecorder = PFLFrontEnd->GetErrRecorder();
	FillMsgList(errRecorder);

	if ((errRecorder.NumErrors() > 0) || !RetVal)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed to compile the NetPFL filter");
		return nbFAILURE;
	}

	return nbSUCCESS;
}

int nbNetPFLCompiler::CreateAutomatonFromFilter(const char *NetPFLFilterString)
{
	int RetVal;

	if (m_debugLevel > 1)
		nbPrintDebugLine("Compiling filter...", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 1);

	ClearMsgList();

	if (!(PDLInited && PFLFrontEnd))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL Compiler Engine has not been initialized, please use NetPDLInit().");
		return nbFAILURE;
	}

	if (NetPFLFilterString == NULL)
		RetVal= PFLFrontEnd->CreateAutomatonFromFilter(""); //RetVal can be: nbSUCCESS, nbFAILURE or nbWARNING
	else
		RetVal= PFLFrontEnd->CreateAutomatonFromFilter(NetPFLFilterString);

	ErrorRecorder &errRecorder = PFLFrontEnd->GetErrRecorder();
	FillMsgList(errRecorder);

	if ((errRecorder.NumErrors() > 0) || (RetVal==nbFAILURE))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed to compile the NetPFL filter");
		return nbFAILURE;
	}

	return nbSUCCESS;
}

int nbNetPFLCompiler::CompileFilter(const char *NetPFLFilterString, char **NetILCode, bool optimizationCycles)
{
int RetVal;

	if (m_debugLevel > 1)
		nbPrintDebugLine("Compiling filter...", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 1);

	ClearMsgList();
	if (GenCode != NULL)
	{
		delete []GenCode;
		GenCode = NULL;
	}
	*NetILCode= NULL;

	if (!(PDLInited && PFLFrontEnd))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL Compiler Engine has not been initialized, please use NetPDLInit().");
		return nbFAILURE;
	}

	if (NetPFLFilterString == NULL)
		RetVal= PFLFrontEnd->CompileFilter("", optimizationCycles); //RetVal can be: nbSUCCESS, nbFAILURE or nbWARNING
	else
		RetVal= PFLFrontEnd->CompileFilter(NetPFLFilterString, optimizationCycles);

	ErrorRecorder &errRecorder = PFLFrontEnd->GetErrRecorder();
	FillMsgList(errRecorder);

	if ((errRecorder.NumErrors() > 0) || (RetVal==nbFAILURE))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed to compile the NetPFL filter");
		return nbFAILURE;
	}

	string &netIL = PFLFrontEnd->GetNetILFilter();
	unsigned int codeStrLen = netIL.size() + 1;
	GenCode = new char[codeStrLen];
	if (GenCode == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
		return nbFAILURE;
	}
	strncpy(GenCode, netIL.c_str(), codeStrLen - 1);
	GenCode[codeStrLen-1] = '\0';

	if (this->dumpNetILCodeFilename!=NULL)
	{
		if (this->dumpNetILCodeFilename[0] == 0)
			printf("%s", GenCode);
		else
		{
			ofstream netILFile(this->dumpNetILCodeFilename);
			PFLFrontEnd->DumpFilter(netILFile, true);
			netILFile.close();
		}
	}

#ifdef ENABLE_PFLFRONTEND_DBGFILES
	if (this->dumpLIRCodeFilename!=NULL)
	{
		ofstream irFile(this->dumpLIRCodeFilename);
		PFLFrontEnd->DumpFilter(irFile, false);
		irFile.close();
	}
	if (this->dumpNetILGraphFilename!=NULL)
	{
		ofstream netILCFG(this->dumpNetILGraphFilename);
		PFLFrontEnd->DumpCFG(netILCFG, false, true);
		netILCFG.close();
	}
	if (this->dumpNoCodeGraphFilename!=NULL)
	{
		ofstream cfgNoCode(this->dumpNoCodeGraphFilename);
		PFLFrontEnd->DumpCFG(cfgNoCode, true, true);
		cfgNoCode.close();
	}
	if (this->dumpLIRGraphFilename!=NULL)
	{
		ofstream irCFG(this->dumpLIRGraphFilename);
		PFLFrontEnd->DumpCFG(irCFG, false, false);
		irCFG.close();
	}
#endif

	*NetILCode = GenCode;
	return nbSUCCESS;
}


_nbExtractFieldInfo* nbNetPFLCompiler::GetExtractField(int *num_entry)
{
	int j=0;

	exFieldList= PFLFrontEnd->GetExField();
	//Descriptors= new _nbExtractedFieldsDescriptor[exFieldList.size()];
	_nbExtractFieldInfo *ExtractedField = new _nbExtractFieldInfo[exFieldList.size()];
	*num_entry = exFieldList.size();
	for (FieldsList_t::iterator i = exFieldList.begin(); i != exFieldList.end(); i++,j++)
	{
		SymbolField *field=(*i);
		if(field->FieldType!=PDL_FIELD_ALLFIELDS)
		{
													
			//Position
			//uint32 position = field->Positions.front();
			//Descriptors->FieldDescriptor[j].Position = position;
			//field->Positions.pop_front();
			//field->Positions.push_back(position);
			
			//multi field
			bool multiField = field->IsMultiField.front();
			field->IsMultiField.pop_front();
			field->IsMultiField.push_back(multiField);
				
			if (field->MultiProto)
			{
				ExtractedField[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO;
				ExtractedField[j].Name = field->Name.c_str();
				ExtractedField[j].Proto = field->Protocol->Name.c_str();
				ExtractedField[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
			}
			else if (multiField/*field->MultiField*/)
			{
				ExtractedField[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD;
				ExtractedField[j].Name = field->Name.c_str();
				ExtractedField[j].Proto = field->Protocol->Name.c_str();
				ExtractedField[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
			}
			else
			{
				ExtractedField[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_FIELD;
				ExtractedField[j].Name = field->Name.c_str();
				ExtractedField[j].Proto = field->Protocol->Name.c_str();
				ExtractedField[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
			}
		}
		else
		{
			nbASSERT(field->Positions.size() == 1, "Ops! There is a BUG!");
			ExtractedField[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_FIELDLIST;
			ExtractedField[j].Name = "allfields";
			ExtractedField[j].Proto = field->Protocol->Name.c_str();
			//Descriptors->FieldDescriptor[j].Position = *(field->Positions.begin());
			ExtractedField[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
		}
	}
	
	return ExtractedField;
}

int nbNetPFLCompiler::GetFieldInfo(const string protoName, uint32_t id, _nbExtractFieldInfo* des)
{
	SymbolField* field=PFLFrontEnd->GetFieldbyId(protoName,(int)id);

	if ((field==NULL) || (field->IsDefined==false))
		return nbFAILURE;

	des->FieldType= (nbExtractedFieldsFieldType_t) field->FieldType;
	des->Name= field->Name.c_str();
	des->Proto= field->Protocol->Name.c_str();
	des->DataFormatType= nbNETPFLCOMPILER_DATAFORMAT_FIELD;

	return nbSUCCESS;
}
