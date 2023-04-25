/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "visit.h"


#include "netpflfrontend.h"
#include "statements.h"
#include "ircodegen.h"
#include "compile.h"
#include "dump.h"
#include "encapfsa.h"
#include "sft/sft_writer.hpp"
#include "compilerconsts.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "cfgwriter.h"
#include "../nbee/globals/debug.h"
#include "protographwriter.h"
#include "filtersubgraph.h"
#include "protographshaper.h"
#include "cfg_edge_splitter.h"
#include "cfgdom.h"
#include "cfg_ssa.h"
#include "deadcode_elimination_2.h"
#include "constant_propagation.h"
#include "copy_propagation.h"
#include "constant_folding.h"
#include "controlflow_simplification.h"
#include "redistribution.h"
#include "reassociation.h"
#include "irnoderegrenamer.h"
#include "../nbnetvm/jit/copy_folding.h"
#include "register_mapping.h"
#include "reassociation_fixer.h"
#include "optimizer_statistics.h"
#include "nbpflcompiler.h"
#include <map>


#ifdef ENABLE_FIELD_OPT
	#include "compfields.h"
#endif




#ifdef ENABLE_TEST_REPORTS
	#include <nbee_profiler.h>
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
	#include <nbee_profiler.h>
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING_USEC
	#include <nbee_profiler.h>
#endif

//IF WANT TO SPLIT CODE BY PROTOCOL ENABLE THIS DEFINE!
//#define DIVIDE_PROTO_FILES true

using namespace std;


void NetPFLFrontEnd::SetupCompilerConsts(CompilerConsts &compilerConsts, uint16 linkLayer)
{
	compilerConsts.AddConst("$linklayer", linkLayer);
}

//TODO OM: maybe it would be better to keep a smaller constructor and to provide some "Set" methods for
// setting all those filenames. Moreover, the unsigned char flags (like dumpHIRCode) are useless if we check
// the presence of the corresponding filename

NetPFLFrontEnd::NetPFLFrontEnd(_nbNetPDLDatabase &protoDB, nbNetPDLLinkLayer_t LinkLayer,
										 const unsigned int DebugLevel,
										 const char *dumpHIRCodeFilename,
			 							 const char *dumpMIRNoOptGraphFilename,
										 const char *dumpProtoGraphFilename,
										 const char *dumpFilterAutomatonFilename
										 )
										 :m_ProtoDB(&protoDB), m_GlobalInfo(protoDB), m_GlobalSymbols(m_GlobalInfo),
										  m_CompUnit(0), m_MIRCodeGen(0), m_NetVMIRGen(0),  m_StartProtoGenerated(0),
									      NetPDLParser(protoDB, m_GlobalSymbols, m_ErrorRecorder),
									      HIR_FilterCode(0), NetVMIR_FilterCode(0), PFL_Tree(0),
										 NetIL_FilterCode(""), m_ManageStatesExtraInfo(true), m_WalkForVisit(false), m_Lower(true),
										 m_AutomatonWithCode(false)
										 
#ifdef OPTIMIZE_SIZED_LOOPS
										 , m_ReferredFieldsInFilter(0), m_ReferredFieldsInCode(0), m_ContinueParsingCode(false)
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
									  	 , m_Profiling("NetPFLFfontendProfiling.txt")
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING_USEC
										 , m_ProfilingUsec("NetPFLfrontendProfilingUsec.txt")
#endif

#ifdef ENABLE_PEG_AND_xpFSA_PROFILING
										 , m_PegAndXPFSAProfiling("PegAndXPFSAProfiling.txt")
#endif

{

#ifdef ENABLE_PFLFRONTEND_PROFILING
	uint64_t TicksBefore, TicksAfter, TicksTotal, MeasureCost;

	TicksTotal= 0;
	
	//Get the starting time of the execution of the frontend		
	m_StartingTicks = nbProfilerGetTime(); 
	
	MeasureCost= nbProfilerGetMeasureCost();
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING_USEC
	m_StartingUsec = nbProfilerGetMicro(); 
#endif
	

	m_GlobalInfo.Debugging.DebugLevel= DebugLevel;

	if (dumpHIRCodeFilename!=NULL)
		m_GlobalInfo.Debugging.DumpHIRCodeFilename=(char *)dumpHIRCodeFilename;
	if (dumpMIRNoOptGraphFilename!=NULL)
		m_GlobalInfo.Debugging.DumpMIRNoOptGraphFilename=(char *)dumpMIRNoOptGraphFilename;
	if (dumpProtoGraphFilename!=NULL)
		m_GlobalInfo.Debugging.DumpProtoGraphFilename=(char *)dumpProtoGraphFilename;	
	if (dumpFilterAutomatonFilename!=NULL)
		m_GlobalInfo.Debugging.DumpFilterAutomatonFilename=(char *)dumpFilterAutomatonFilename;	

	if (m_GlobalInfo.Debugging.DebugLevel > 0)
		nbPrintDebugLine("Initializing NetPFL Front End...", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 1);

	if (!m_GlobalInfo.IsInitialized())
		throw ErrorInfo(ERR_FATAL_ERROR, "startproto protocol not found!!!");

	CompilerConsts &compilerConsts = m_GlobalInfo.GetCompilerConsts();
	SetupCompilerConsts(compilerConsts, LinkLayer);

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksBefore= nbProfilerGetTime();
#endif

	HIRCodeGen ircodegen(m_GlobalSymbols, &(m_GlobalSymbols.GetProtoCode(m_GlobalInfo.GetStartProtoSym())));
	NetPDLParser.ParseStartProto(ircodegen, PARSER_VISIT_ENCAP);


	SymbolProto **protoList = m_GlobalInfo.GetProtoList();
	
	
	if((NetPDLParser.GetErrRecorder()).PDLErrorsOccurred())
	{
		m_ErrorRecorder = NetPDLParser.GetErrRecorder();
		return;
	}

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	TicksTotal= TicksTotal + (TicksAfter - TicksBefore - MeasureCost);

	protoList[0]->ParsingTime= TicksAfter - TicksBefore - MeasureCost;
#endif

#ifdef ENABLE_PFLFRONTEND_DBGFILES   //[mligios] - PARSE & SAVE - FORMAT SECTION ON NETPDL (p1_2 , only for startproto)
	ostringstream my_filename;
	if (m_GlobalInfo.Debugging.DumpHIRCodeFilename)
	{
		#ifdef DIVIDE_PROTO_FILES
			my_filename << m_GlobalInfo.Debugging.DumpHIRCodeFilename << "_" << protoList[0]->Name << ".asm";
			ofstream my_ir(my_filename.str().c_str());
			CodeWriter my_cw(my_ir);
			my_cw.DumpCode(&m_GlobalSymbols.GetProtoCode(protoList[0])); //dumps the HIR code
			my_ir.close();
		#endif
		#ifndef DIVIDE_PROTO_FILES
		//It can be useful to keep all hir code together in one file
			ofstream my_ir2(m_GlobalInfo.Debugging.DumpHIRCodeFilename);
			CodeWriter my_cw2(my_ir2);
			my_cw2.DumpCode(&m_GlobalSymbols.GetProtoCode(protoList[0])); //dumps the HIR code
			my_ir2.close();
		#endif
	}
#endif

	//create the graph
	for (uint32 i = 1; i < m_GlobalInfo.GetNumProto(); i++)
	{
#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksBefore= nbProfilerGetTime();
#endif
		HIRCodeGen protocodegen(m_GlobalSymbols, &(m_GlobalSymbols.GetProtoCode(protoList[i])));
		NetPDLParser.ParseProtocol(*protoList[i], protocodegen, PARSER_VISIT_ENCAP);		

#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksAfter= nbProfilerGetTime();
		TicksTotal= TicksTotal + (TicksAfter - TicksBefore - MeasureCost);

		protoList[i]->ParsingTime= TicksAfter - TicksBefore - MeasureCost;
#endif

		if((NetPDLParser.GetErrRecorder()).PDLErrorsOccurred())
		{
			m_ErrorRecorder = NetPDLParser.GetErrRecorder();
			return;
		}

#ifdef ENABLE_PFLFRONTEND_DBGFILES  
		if (m_GlobalInfo.Debugging.DumpHIRCodeFilename)
		{
		#ifdef DIVIDE_PROTO_FILES
			my_filename.str("");					
			my_filename << m_GlobalInfo.Debugging.DumpHIRCodeFilename << "_" << protoList[i]->Name << ".asm";
			ofstream my_ir(my_filename.str().c_str());
			CodeWriter my_cw(my_ir);
			my_cw.DumpCode(&m_GlobalSymbols.GetProtoCode(protoList[i])); //dumps the HIR code
			my_ir.close();
		#endif
		#ifndef DIVIDE_PROTO_FILES
		//It can be useful to keep all hir code together in one file
			ofstream my_ir2(m_GlobalInfo.Debugging.DumpHIRCodeFilename, ios::app);
			CodeWriter my_cw2(my_ir2);
			my_cw2.DumpCode(&m_GlobalSymbols.GetProtoCode(protoList[i])); //dumps the HIR code
			my_ir2.close();
		#endif
		}
#endif
/*
	if(protoList[i]->NoSession || protoList[i]->Name=="http")
		{
	//cout<<"PROTOCOL: "<<protoList[i]->Name<<" NoSession "<<protoList[i]->NoSession <<endl;
	//cout<<"LOOKUP declaration: "<<protoList[i]->LookupDecl<< " #occ "<<protoList[i]->LookupOccurrences<<endl;
		}
		
*/
	}

#ifdef ENABLE_FIELD_OPT
	//Print Information about ALL the fields of ALL the protocols
	#ifdef ENABLE_FIELD_DEBUG 
		PrintFieldsInfo(0);
	#endif
#endif


#ifdef STATISTICS
	ofstream fieldStats;
		fieldStats.open("fieldstats.txt");
		m_GlobalSymbols.DumpProtoFieldStats(fieldStats);
	printf("\nTotal protocols number: %u", m_GlobalInfo.GetNumProto());

	uint32 notSupportedProto=0;
	FILE *f=fopen("statistics.csv", "w");
	if (f!=NULL)
	{
		fprintf(f, "Protocol name;Supported;Encapsulable;Parsing time (ticks);" \
			"ParsedFields;Inner fields;Field unsupported;Ref field unknown;" \
			"Expression unsupported;Var declared;Var unsupported;Var occurrences;Var tot occurrences;" \
			"Encap declared;Encap tentative;Encap successful;" \
			"Lookup declared;Lookup occurrences" \
			"\n");
		for (uint32 i = 0; i < m_GlobalInfo.GetNumProto(); i++)
		{
			fprintf(f, "%s", protoList[i]->Name.c_str());

			if (protoList[i]->IsDefined==false || protoList[i]->IsSupported==false)
				fprintf(f, ";no");
			else
				fprintf(f, ";yes");

			if (protoList[i]->VerifySectionSupported==false || protoList[i]->BeforeSectionSupported==false)
				fprintf(f, ";no");
			else
				fprintf(f, ";yes");

			fprintf(f, ";-");
			fprintf(f, ";%u", protoList[i]->DeclaredFields);
			fprintf(f, ";%u", protoList[i]->InnerFields);
			fprintf(f, ";%u", protoList[i]->UnsupportedFields);
			fprintf(f, ";%u", protoList[i]->UnknownRefFields);
			fprintf(f, ";%u", protoList[i]->UnsupportedExpr);
			fprintf(f, ";%u", protoList[i]->VarDeclared);
			fprintf(f, ";%u", protoList[i]->VarUnsupported);
			fprintf(f, ";%u", protoList[i]->VarOccurrences);
			fprintf(f, ";%u", protoList[i]->VarTotOccurrences);
			fprintf(f, ";%u", protoList[i]->EncapDecl);
			fprintf(f, ";%u", protoList[i]->EncapTent);
			fprintf(f, ";%u", protoList[i]->EncapSucc);
			fprintf(f, ";%u", protoList[i]->LookupDecl);
			fprintf(f, ";%u", protoList[i]->LookupOccurrences);
			fprintf(f, "\n");
			if (protoList[i]->IsDefined==false || protoList[i]->IsSupported==false)
				notSupportedProto++;
		}
	}

	printf("\n\tTotal unsupported protocols number: %u\n", notSupportedProto);
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
	//Prints the time needed to the PDLParser to analyze each procol, and the time required to parse all the protocols of the NetPDL
	
	m_Profiling << "Protocol name\tParsing time (ticks)" << endl;
	for (uint32 i = 0; i < m_GlobalInfo.GetNumProto(); i++)
	{
		//printf("\t%s  %ld\n", protoList[i]->Name.c_str(), protoList[i]->ParsingTime);
		m_Profiling << protoList[i]->Name.c_str() << "\t\t" << protoList[i]->ParsingTime << endl;
	}
	m_Profiling << endl << "Total protocol parsing time " << TicksTotal << " (avg per protocol " << (TicksTotal/m_GlobalInfo.GetNumProto()) <<")";
#endif

#ifdef ENABLE_PEG_AND_xpFSA_PROFILING
	ProfilePeg();
#endif

#ifndef ENABLE_PFLFRONTEND_PROFILING
	if (m_GlobalInfo.Debugging.DumpProtoGraphFilename)
	{
		//print the complete graph
		ofstream protograph(m_GlobalInfo.Debugging.DumpProtoGraphFilename);
		m_GlobalInfo.DumpProtoGraph(protograph);
		protograph.close();
		
		//print the preferred graph
		string sname = m_GlobalInfo.Debugging.DumpProtoGraphFilename;
		char *name;
		name = (char*)malloc(sizeof(char)*(strlen(sname.c_str())+strlen("_Preferred")));
		strcpy(name,sname.c_str());
		name[strlen(name)-4]='\0';
		strcpy(&name[strlen(name)],"_Preferred.dot");
		ofstream protographPreferred(name);
		m_GlobalInfo.DumpProtoGraphPreferred(protographPreferred);
		protographPreferred.close();
	}
	
#endif

	if (m_GlobalInfo.Debugging.DebugLevel > 0)
		nbPrintDebugLine("Initialized NetPFL Front End", DBG_TYPE_INFO, __FILE__,
		__FUNCTION__, __LINE__, 1);
}


#ifdef ENABLE_FIELD_OPT
	#ifdef ENABLE_FIELD_DEBUG
void NetPFLFrontEnd::PrintFieldsInfo(bool flag)
{
	SymbolProto **protoList = m_GlobalInfo.GetProtoList();
	ofstream *my_ir;
	if(flag==0){
		my_ir = new ofstream("Debug_ALL_Fields_Before_Optimizations.txt");
	}else{
		my_ir = new ofstream("Debug_ALL_Fields_After_Optimizations.txt");
	}

	(*my_ir) << "*************************   FRONTEND -> PROTOCOL LIST (extracted from GlobalSymbols)" <<endl; 

		for (uint32 i = 0; i < m_GlobalInfo.GetNumProto(); i++)
		{
			
			//protoList[i]->Name	Protocollo i-esimo
			//serve x ogni protocollo la lista dei campi!

			(*my_ir) <<" Protocol: ( " << protoList[i]->Name << " ) FIELDS"<<endl;
			SymbolField **tmpf=m_GlobalSymbols.GetFieldList(protoList[i]);
			for (uint32 k = 0; k < m_GlobalSymbols.GetNumField(protoList[i]); k++)
			{
				switch(tmpf[k]->FieldType)
				{
				case PDL_FIELD_FIXED:
					(*my_ir) << "FIXED ";
				break;	
				case PDL_FIELD_VARLEN:
					(*my_ir) << "VARLEN ";
				break;
				case PDL_FIELD_BITFIELD:
					(*my_ir) << "BIT ";
				break;
				default:	
					(*my_ir) << "DEFAULT ";
				break;
				}
				(*my_ir) << "  Name: "<< tmpf[k]->Name << " Compattable? (1=yes) " << tmpf[k]->Compattable <<" Used? (1=yes) "<<tmpf[k]->Used <<endl; 
			}
			(*my_ir) << "****** END of Protocol: ( " << protoList[i]->Name << " )"<<endl;
		 }	
	(*my_ir)  << "*********************   FRONTEND -> End of PROTOCOL LIST" <<endl; 
	(*my_ir).close();
	delete(my_ir);
}
	#endif
#endif


#ifdef ENABLE_PEG_AND_xpFSA_PROFILING
void NetPFLFrontEnd::ProfilePeg()
{
	//This code has been written by Ivano Cerrato to perform some tests for his paper
	unsigned int totalEdges = 0;
	unsigned int preferredEdges = 0;
	unsigned int preferredNodes = 0;
	unsigned int selfLoops = 0;
	unsigned int preferredSelfLoops = 0;
	EncapGraph &peg = m_GlobalInfo.GetProtoGraph();
	EncapGraph &pegPreferred = m_GlobalInfo.GetProtoGraphPreferred();
	
	m_PegAndXPFSAProfiling << "PEG fullencap" << endl;
	m_PegAndXPFSAProfiling << "\tNodes: " << m_GlobalInfo.GetNumProto() << endl;
	for (uint32 i = 0; i < m_GlobalInfo.GetNumProto(); i++)
	{	
		SymbolProto *proto = m_GlobalSymbols.GetProto(i);
		
		EncapGraph::GraphNode *node = peg.GetNode(proto);
		list<EncapGraph::GraphNode*> &Successors = node->GetSuccessors();
		totalEdges += Successors.size();
		for(list<EncapGraph::GraphNode*>::iterator it = Successors.begin(); it != Successors.end(); it++)
		{
			if(*it == node)
				selfLoops++;
		}
		
		EncapGraph::GraphNode *nodePreferred = pegPreferred.GetNode(proto);
		list<EncapGraph::GraphNode*> &SuccessorsPreferred = nodePreferred->GetSuccessors();
		list<EncapGraph::GraphNode*> &PredecessorsPreferred = nodePreferred->GetPredecessors();		
		if(SuccessorsPreferred.size() == 0 && PredecessorsPreferred.size() == 0)
			continue;
		preferredEdges += SuccessorsPreferred.size();
		preferredNodes++;
		for(list<EncapGraph::GraphNode*>::iterator it = SuccessorsPreferred.begin(); it != SuccessorsPreferred.end(); it++)
		{
			if(*it == nodePreferred)
				preferredSelfLoops++;
		}
	}
	m_PegAndXPFSAProfiling << "\tEdges: " << totalEdges << endl;
	m_PegAndXPFSAProfiling << "\tSelf loops: " << selfLoops << endl;
	m_PegAndXPFSAProfiling << "\tLayers: " << endl;
	AssignProtocolLayer assignerFullEncap(peg); 
	assignerFullEncap.Classify();
	assignerFullEncap.PrintLayers(m_PegAndXPFSAProfiling);

	m_PegAndXPFSAProfiling << "PEG preferred" << endl;
	m_PegAndXPFSAProfiling << "\tNodes: " << preferredNodes << endl;
	m_PegAndXPFSAProfiling << "\tEdges: " << preferredEdges << endl;
	m_PegAndXPFSAProfiling << "\tSelf loops: " << preferredSelfLoops << endl;
	m_PegAndXPFSAProfiling << "\tLayers: " << endl;	
	AssignProtocolLayer assignerPreferred(pegPreferred);
	assignerPreferred.Classify();
	assignerPreferred.PrintLayers(m_PegAndXPFSAProfiling);
}
#endif

NetPFLFrontEnd::~NetPFLFrontEnd()
{
	if (m_CompUnit != NULL)
	{
		delete m_CompUnit;
		m_CompUnit = NULL;
	}
}

void NetPFLFrontEnd::DumpPFLTreeDotFormat(PFLExpression *expr, ostream &stream)
{
	stream << "Digraph PFL_Tree{" << endl;
	expr->PrintMeDotFormat(stream);
	stream << endl << "}" << endl;
}

/*
* this method is used by the sample "filtercompiler" with the option "-compileonly"
*/
bool NetPFLFrontEnd::CheckFilter(string filter)
{
	m_ErrorRecorder.Clear();

	ParserInfo parserInfo(m_GlobalSymbols, m_ErrorRecorder);
	compile(&parserInfo, filter.c_str(), 0);

	if (parserInfo.Filter == NULL)
		return false;
	else
		return true;
}

/*
* this method is used by the sample "filtercompiler" with the option "-automaton"
*/
int NetPFLFrontEnd::CreateAutomatonFromFilter(string filter)
{
	m_ErrorRecorder.Clear();

	PFLStatement *filterExpression = ParseFilter(filter);  
	
	if (filterExpression == NULL)
		return nbFAILURE;

	m_FullEncap = filterExpression->IsFullEncap(); // true in case of the NetPFL keyword "fullencap" has been specifid
	PFLAction *action = filterExpression->GetAction();
	bool fieldExtraction = (action->GetType() == PFL_EXTRACT_FIELDS_ACTION)? true : false;
	NodeList_t toExtract;

	m_fsa = VisitFilterExpression(filterExpression->GetExpression(),false);//false: we are not generating the automaton related to the field extraction

	if(m_fsa==NULL)
		return nbFAILURE; //an error occurred during the creation of the automaton
		
	if (fieldExtraction)
	{   	  	
		m_ManageStatesExtraInfo = false;	//we want only to generate the automaton, and we are not interested in the extra
											//info associated with the action states
		EncapFSA *fsaExtractFields = VisitFieldExtraction((PFLExtractFldsAction*)action);
		if(fsaExtractFields==NULL) //an error occurrend during the creation of the automaton
			return nbFAILURE;
	
		m_fsa->ResetFinals();
	    
	    //fsa_filter OR fsa_extraction
	   	m_fsa = EncapFSA::BooleanOR(m_fsa,fsaExtractFields,false);
	   	m_fsa->FixTransitions();
	}   
	m_fsa->fixStateProtocols();	
	return nbSUCCESS;		
}


PFLStatement *NetPFLFrontEnd::ParseFilter(string filter)
{
	
	if (filter.size() == 0)
	{
		PFLReturnPktAction *retpkt = new PFLReturnPktAction(1);
		CHECK_MEM_ALLOC(retpkt);
		PFLStatement *stmt = new PFLStatement(NULL, retpkt, NULL);
		m_FullEncap = stmt->IsFullEncap();
		CHECK_MEM_ALLOC(stmt);
		return stmt;
	}
	
	ParserInfo parserInfo(m_GlobalSymbols, m_ErrorRecorder); //this struct is defined in compile.h
	compile(&parserInfo, filter.c_str(), 0);
		
	return parserInfo.Filter;//return the filtering expression
}

int NetPFLFrontEnd::CompileFilter(string filter, bool optimizationCycles)
{

	m_ErrorRecorder.Clear();
	if (m_CompUnit != NULL)
	{
		delete m_CompUnit;
		m_CompUnit = NULL;
	}

	PFLStatement *filterStmt = ParseFilter(filter);  //now we have the statement related to the filtering expression 
	if (filterStmt == NULL)
		return nbFAILURE;
		
	m_FullEncap = filterStmt->IsFullEncap();	
		
	m_CompUnit = new CompilationUnit(filter);
	CHECK_MEM_ALLOC(m_CompUnit);

#ifdef ENABLE_PFLFRONTEND_PROFILING
	int64_t TicksBefore, TicksAfter, MeasureCost;

	MeasureCost= nbProfilerGetMeasureCost();
#endif

	if (filter.size() > 0) //i.e. if the filter is not empty
	{
#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksBefore= nbProfilerGetTime();
#endif	


//MIR Code is going to be generated!!
//1. VISIT PROTOCOLS & CHECK IF (lookuptable are used && no-Empty is set) 
//2. ADD CODE TO AVOID use of lookup-table
			//FSA!?
/*
			cout<<"START HERE"<<endl;
			std::list<EncapFSA::State*> stateList = fsa->SortRevPostorder();
			std::list<EncapFSA::State*>::iterator i;
			for(i = stateList.begin() ; i != stateList.end(); i++)
			{	
				set<SymbolProto*> protocols;
				protocols = GetProtocols(*i,fsa);
	
				for(set<SymbolProto*>::iterator p = protocols.begin(); p != protocols.end(); p++)
				{
					cout<<"PROTO: "<<(*p)->Name<<endl;
					CodeList *code = &(m_GlobalSymbols.GetProtoCode(*p));
					
					
				CodeList *code = m_CodeList;
					if(code)
					{
						for(StmtBase *stmt = code->Front(); stmt != code->Back(); stmt=stmt->Next)
						{
							if (stmt->Kind == STMT_GEN)
								if (stmt->Forest->Kids[0]->Op == IR_ASGNI)	   
									cout << "FOUND! " << stmt->Forest->Kids[0]->Value <<endl;	
						}
					}

				}
				cout<<"END HERE"<<endl;
			}
*/

		MIRCodeGen mirCodeGen(m_GlobalSymbols, &m_CompUnit->IRCode);
		m_MIRCodeGen = &mirCodeGen;
		m_CompUnit->OutPort = 1;
		if(filterStmt->GetAction()!=NULL)
			VisitAction(filterStmt->GetAction()); 

#ifdef ENABLE_TEST_REPORTS
                int64_t prof_start_ticks, prof_end_ticks, prof_start_us, prof_end_us, MeasureCost;
                int64_t opt_start_ticks, opt_end_ticks, opt_start_us, opt_end_us;
                MeasureCost= nbProfilerGetMeasureCost();
                prof_start_us = nbProfilerGetMicro();
                prof_start_ticks = nbProfilerGetTime();
#endif

		try
		{	
			PRINT_DOT_LEGEND();
			if(GenCode(filterStmt)==nbFAILURE) 
				return nbFAILURE;
		}
		catch(ErrorInfo &e)
		{
			if (e.Type != ERR_FATAL_ERROR)
			{
				m_ErrorRecorder.PDLError(e.Msg);
				return nbFAILURE;
			}
			else
			{
				throw e;
			}
		}

#ifdef ENABLE_TEST_REPORTS
                prof_end_ticks = nbProfilerGetTime();
                prof_end_us = nbProfilerGetMicro();
                printf("\nCODEGEN-REPORT: ticks(%ld), us(%ld)\n", prof_end_ticks - prof_start_ticks - MeasureCost, prof_end_us - prof_start_us);
#endif

		delete filterStmt;


#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	//The time required to generate the MIR code includes that needed to build the automaton. 
	m_Profiling << endl << endl <<  "MIR generation used " << (TicksAfter - TicksBefore - MeasureCost) << " ticks";
	TicksBefore= nbProfilerGetTime();
#endif

#ifdef ENABLE_PFLFRONTEND_DBGFILES
	{
		ofstream ir("mir_code.asm");		
		CodeWriter cw(ir);
		cw.DumpCode(&m_CompUnit->IRCode); //dumps the MIR code
		ir.close();
	}
#endif

	//creates the control flow graph - defined in pflcfg.h - and translates it from MIR code to MIRO code
	CFGBuilder builder(m_CompUnit->Cfg); 
	builder.Build(m_CompUnit->IRCode);//m_CompUnit->IRCode contains MIR statements	//ERROR HERE!!! MIR CHANGE!! 


#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	m_Profiling << endl << endl <<  "MIRO generation used " << (TicksAfter - TicksBefore - MeasureCost) << " ticks";
	TicksBefore= nbProfilerGetTime();
#endif

#ifdef ENABLE_PFLFRONTEND_DBGFILES
		{
			ofstream irCFG("cfg_miro_no_opt.dot");
			DumpCFG(irCFG, false, false);
			irCFG.close();
		}
#endif

		GenRegexEntries(); 
		GenStringMatchingEntries(); 


		// generate initialization code (i.e. code for the initialization processors)
		MIRCodeGen mirInitCodeGen(m_GlobalSymbols, &m_CompUnit->InitIRCode);
		m_MIRCodeGen = &mirInitCodeGen;
		try
		{
			int locals = m_CompUnit->NumLocals;
			GenInitCode(); 
			m_CompUnit->NumLocals = locals;
		}
		catch(ErrorInfo &e)
		{
			if (e.Type != ERR_FATAL_ERROR)
			{
				m_ErrorRecorder.PDLError(e.Msg);
				return nbFAILURE;
			}
			else
			{
				throw e;
			}
		}

		
#ifdef ENABLE_PFLFRONTEND_DBGFILES
	{
		ofstream ir_init("mir_code_initialization.asm");		
		CodeWriter cw_init(ir_init);
		cw_init.DumpCode(&m_CompUnit->InitIRCode); //dumps the MIR code to initialize the coprocessors
		ir_init.close();
	}

#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	m_Profiling << endl << endl << "MIR generation for initialization required: " << (TicksAfter - TicksBefore - MeasureCost) << " ticks";
	TicksBefore= nbProfilerGetTime();
#endif
	
	//creates the control flow graph about the initialization code and translates it from MIR code to MIRO code
	CFGBuilder initBuilder(m_CompUnit->InitCfg);
	initBuilder.Build(m_CompUnit->InitIRCode);

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	m_Profiling << endl << endl << "MIRO generation for initialization required: " << (TicksAfter - TicksBefore - MeasureCost) << " ticks";
	TicksBefore= nbProfilerGetTime();
#endif

#ifdef ENABLE_TEST_REPORTS
	opt_start_us = nbProfilerGetMicro();
    opt_start_ticks = nbProfilerGetTime();
#endif


#ifdef ENABLE_PFLFRONTEND_DBGFILES
	{
		ofstream ir("cfg_before_opt.dot");
		DumpCFG(ir, false, false);
		ir.close();
	}
#endif

		/*******************************************************************
		*		HERE IT BEGINS THE OPTIMIZATIONS		   *
		*******************************************************************/
#ifdef ENABLE_PFLFRONTEND_PROFILING
	uint64_t TicksBeforeOpt, TicksAfterOpt; //used to measure the time required by each optimization algorithm		
#endif
		

#ifdef _DEBUG_OPTIMIZER
		std::cout << "Running setCFG" << std::endl;
#endif
		MIRONode::setCFG(&m_CompUnit->Cfg);

//*****************************************  Simple Reduce of CFG  ***********************************//
		bool changed = true;
		bool reass_changed = true;

	#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksBeforeOpt= nbProfilerGetTime();
	#endif

		while(reass_changed)
		{
			reass_changed = false;
			changed = true;

			while (changed)
			{
			
			changed = false;

			/**************** BasicBlockElimination ****************/

	#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running Basic block elimination (Simple Reduce)" << std::endl;
	#endif
			jit::opt::BasicBlockElimination<PFLCFG> bbs(m_CompUnit->Cfg);
			bbs.start(changed);

				jit::opt::JumpToJumpElimination<PFLCFG> j2je(m_CompUnit->Cfg);
				jit::opt::EmptyBBElimination<PFLCFG> ebbe(m_CompUnit->Cfg);
				jit::opt::RedundantJumpSimplification<PFLCFG> rds(m_CompUnit->Cfg);

			bool cfs_changed = true;
				while(cfs_changed)
				{
					cfs_changed = false;
					cfs_changed |= ebbe.run();
					bbs.start(changed);
					cfs_changed |= rds.run();
					bbs.start(changed);
					cfs_changed |= j2je.run();
					bbs.start(changed);
				}
			}
		}

	#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksAfterOpt= nbProfilerGetTime();
		m_Profiling << endl << endl << "Simple Reduce of CFG required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
	#endif

	#ifdef ENABLE_PFLFRONTEND_DBGFILES
		{
			ofstream ir("cfg_after_simple_reduction.dot");
			DumpCFG(ir, false, false);
			ir.close();
		}
	#endif		
	

//************************************** END of Simple Reduce of CFG  *********************************//

//while(false){
		if(optimizationCycles)
		{
		
			/**************** NodeRenamer ****************/
		
#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running NodeRenamer" << std::endl;
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksBeforeOpt= nbProfilerGetTime();
#endif
			IRNodeRegRenamer inrr(m_CompUnit->Cfg);
			inrr.run();
			
#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "NodeRenamer required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif

			/**************** EdgeSplitter ****************/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running EdgeSPlitter" << std::endl;
#endif
			CFGEdgeSplitter<MIRONode> cfges(m_CompUnit->Cfg);
			cfges.run();
			
#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "EdgeSplitter required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif


#ifdef ENABLE_PFLFRONTEND_DBGFILES
			{
				ofstream ir("cfg_edge_splitting.dot");
				DumpCFG(ir, false, false);
				ir.close();
			}
#endif

			/**************** ComputeDominance ****************/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running ComputeDominance" << std::endl;
#endif
			jit::ComputeDominance<PFLCFG> cd(m_CompUnit->Cfg);
			cd.run();
			
#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "ComputeDominance required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif
			
			/**************** SSA ****************/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running SSA" << std::endl;
#endif
			jit::SSA<PFLCFG> ssa(m_CompUnit->Cfg);
			ssa.run();

#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "SSA required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif

#ifdef ENABLE_PFLFRONTEND_DBGFILES
			{
				ofstream ir("cfg_after_SSA.dot");
				DumpCFG(ir, false, false);
				ir.close();
			}
#endif

			reass_changed = true;

#ifdef ENABLE_PFLFRONTEND_DBGFILES
			int counter = 0;
#endif

			while(reass_changed)
			{
	
				reass_changed = false;
				changed = true;

				while (changed)
				{
					changed = false;
					/**************** CopyPropagation ****************/

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Copy propagation" << std::endl;
#endif
					jit::opt::CopyPropagation<PFLCFG> copyp(m_CompUnit->Cfg);
					changed |= copyp.run();
				
#ifdef ENABLE_PFLFRONTEND_PROFILING
					TicksAfterOpt= nbProfilerGetTime();
					m_Profiling << endl << endl << "CopyPropagation required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
					TicksBeforeOpt= nbProfilerGetTime();
#endif

					
					/**************** ConstantPropagation ****************/					
					
#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Constant propagation" << std::endl;
#endif
					jit::opt::ConstantPropagation<PFLCFG> cp(m_CompUnit->Cfg);
					changed |= cp.run();
					
#ifdef ENABLE_PFLFRONTEND_PROFILING
					TicksAfterOpt= nbProfilerGetTime();
					m_Profiling << endl << endl << "ConstantPropagation required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
					TicksBeforeOpt= nbProfilerGetTime();
#endif

					/**************** ConstantFolding ****************/

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running ConstantFolding" << std::endl;
#endif
					jit::opt::ConstantFolding<PFLCFG> cf(m_CompUnit->Cfg);
					changed |= cf.run();

#ifdef ENABLE_PFLFRONTEND_PROFILING
					TicksAfterOpt= nbProfilerGetTime();
					m_Profiling << endl << endl << "ConstantFolding required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
					TicksBeforeOpt= nbProfilerGetTime();
#endif
					
					/**************** DeadCodeElimination ****************/

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running DeadCodeElimination" << std::endl;
#endif
					jit::opt::DeadcodeElimination<PFLCFG> dce_b(m_CompUnit->Cfg);
					changed |= dce_b.run();

#ifdef ENABLE_PFLFRONTEND_PROFILING
					TicksAfterOpt= nbProfilerGetTime();
					m_Profiling << endl << endl << "DeadCodeElimination required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
					TicksBeforeOpt= nbProfilerGetTime();
#endif

					/**************** BasicBlockElimination ****************/

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Basic block elimination" << std::endl;
#endif
					jit::opt::BasicBlockElimination<PFLCFG> bbs(m_CompUnit->Cfg);
					bbs.start(changed);
					
#ifdef ENABLE_PFLFRONTEND_PROFILING
					TicksAfterOpt= nbProfilerGetTime();
					m_Profiling << endl << endl << "BasicBlockElimination required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
					TicksBeforeOpt= nbProfilerGetTime();
#endif
					
					/**************** Redistribution ****************/				

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Redistribution" << std::endl;
#endif
					jit::opt::Redistribution<PFLCFG> rdst(m_CompUnit->Cfg);
					rdst.start(changed);
#ifdef ENABLE_PFLFRONTEND_PROFILING
					TicksAfterOpt= nbProfilerGetTime();
					m_Profiling << endl << endl << "Redistribution required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
					TicksBeforeOpt= nbProfilerGetTime();
#endif

				}
				
				#ifdef ENABLE_PFLFRONTEND_DBGFILES
				{
					ostringstream filename;
					filename << "cfg_before_reass_" << counter << ".dot";
					ofstream ir(filename.str().c_str());
					DumpCFG(ir, false, false);
					ir.close();
				}
#endif
				
				/**************** Reassociation ****************/
				
#ifdef _DEBUG_OPTIMIZER
				std::cout << "Running reassociation" << std::endl;
#endif
				{
					jit::Reassociation<PFLCFG> reass(m_CompUnit->Cfg);
					reass_changed = reass.run();
					
#ifdef ENABLE_PFLFRONTEND_PROFILING
					TicksAfterOpt= nbProfilerGetTime();
					m_Profiling << endl << endl << "Reassociation required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
					TicksBeforeOpt= nbProfilerGetTime();
#endif
					
				}
#ifdef ENABLE_PFLFRONTEND_DBGFILES
				{
					ostringstream filename;
					filename << "cfg_after_reass_" << counter++ << ".dot";
					ofstream ir(filename.str().c_str());
					DumpCFG(ir, false, false);
					ir.close();
				}
#endif
			}

			/**************** ReassociationFixer ****************/
				
#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running reassociation  fixer" << std::endl;
#endif

			ReassociationFixer rf(m_CompUnit->Cfg);
			rf.run();

#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "Reassociation fixer " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif

#ifdef ENABLE_PFLFRONTEND_DBGFILES
			{
				ofstream ir("cfg_after_opt.dot");
				DumpCFG(ir, false, false);
				ir.close();
			}


#endif

			/**************** UndoSSA ****************/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running UndoSSA" << std::endl;
#endif
			jit::UndoSSA<PFLCFG> ussa(m_CompUnit->Cfg);
			ussa.run();
			
#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "UndoSSA required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif

			/**************** FoldCopies ****************/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running Foldcopies" << std::endl;
#endif
			jit::Fold_Copies<PFLCFG> fc(m_CompUnit->Cfg);
			fc.run();
			
#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "FoldCopies required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif
			
			/**************** KillRedundant ****************/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running KillRedundant" << std::endl;
#endif
			jit::opt::KillRedundantCopy<PFLCFG> krc(m_CompUnit->Cfg);
			krc.run();
			
#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "KillRedundant required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif
			
			/**************** ControlFlowSimplification **********/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running ControlFlow simplification" << std::endl;
#endif
			jit::opt::ControlFlowSimplification<PFLCFG> cfs(m_CompUnit->Cfg);
			cfs.start(changed);

#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "ControlFlowSimplification required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif

			/**************** BasicBlockElimination ****************/

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running Basic block elimination" << std::endl;
#endif
			jit::opt::BasicBlockElimination<PFLCFG> bbs(m_CompUnit->Cfg);
			bbs.start(changed);

			jit::opt::JumpToJumpElimination<PFLCFG> j2je(m_CompUnit->Cfg);
			jit::opt::EmptyBBElimination<PFLCFG> ebbe(m_CompUnit->Cfg);
			jit::opt::RedundantJumpSimplification<PFLCFG> rds(m_CompUnit->Cfg);

			bool cfs_changed = true;
			while(cfs_changed)
			{
				cfs_changed = false;
				cfs_changed |= ebbe.run();
				bbs.start(changed);
				cfs_changed |= rds.run();
				bbs.start(changed);
				cfs_changed |= j2je.run();
				bbs.start(changed);
			}
			
#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "BasicBlockElimination required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
			TicksBeforeOpt= nbProfilerGetTime();
#endif

		}	//optimization Cycle?
		//}	//while false?? (commented)
		
		
		/**************** RegisterMapping ****************/
		
#ifdef _DEBUG_OPTIMIZER
		std::cout << "Running RegisterMapping" << std::endl;
#endif
		std::set<uint32_t> ign_set;
		jit::Register_Mapping<PFLCFG> rm(m_CompUnit->Cfg, 1, ign_set);
		rm.run();

#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksAfterOpt= nbProfilerGetTime();
		m_Profiling << endl << endl << "RegisterMapping required: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
#endif

		m_CompUnit->NumLocals = rm.get_manager().getCount();


		/*******************************************************************
		*		HERE IT ENDS  					   *
		*******************************************************************/

#ifdef ENABLE_PFLFRONTEND_DBGFILES
		{
			ofstream irCFG("cfg_ir_opt.dot");
			DumpCFG(irCFG, false, false);
			irCFG.close();

		}
#endif

#ifdef ENABLE_COMPILER_PROFILING
		jit::opt::OptimizerStatistics<PFLCFG> optstats("After PFL");
		std::cout << optstats << std::endl;
#endif

#ifdef ENABLE_TEST_REPORTS
                opt_end_ticks = nbProfilerGetTime();
                opt_end_us = nbProfilerGetMicro();
                printf("\nOPTIMIZATION-REPORT: ticks(%ld), us(%ld)\n", opt_end_ticks - opt_start_ticks - MeasureCost, opt_end_us - opt_start_us);
#endif

	}	//if(filter.size>0)?
#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	m_Profiling << endl << endl << "MIRO optimized required: " << (TicksAfter - TicksBefore - MeasureCost) << " ticks";
	TicksBefore= nbProfilerGetTime();
#endif

	ostringstream netIL;
	if (this->m_GlobalSymbols.GetUsefulTablesNumber()>0)
		m_CompUnit->UsingCoproLookupEx=true;
	else
		m_CompUnit->UsingCoproLookupEx=false;
		
	if (this->m_GlobalSymbols.GetStringMatchingEntriesCountUseful()>0)
		m_CompUnit->UsingCoproStringMatching=true;
	else
		m_CompUnit->UsingCoproStringMatching=false;
	

	if (this->m_GlobalSymbols.GetRegExEntriesCountUseful()>0)
		m_CompUnit->UsingCoproRegEx=true;
	else
		m_CompUnit->UsingCoproRegEx=false;

	m_CompUnit->DataItems=this->m_GlobalSymbols.GetDataItems();

	m_CompUnit->GenerateNetIL(netIL); //generates the NetIL bytecode
	
	NetIL_FilterCode = netIL.str();

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	m_Profiling << endl << endl << "NetIL generation required: " << (TicksAfter - TicksBefore - MeasureCost) << " ticks";
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter = nbProfilerGetTime();
	m_Profiling << endl << endl << "All the operations of the NetPFL fronted used " << (TicksAfter - m_StartingTicks - MeasureCost) << " ticks" << endl; 
	
	cout << "The Profiling information was writtend in the file \"NetPFLFfontendProfiling.txt\"" << endl;
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING_USEC
	uint64_t usecAfter = nbProfilerGetMicro();
	m_ProfilingUsec << "All the operations of the NetPFL frontend used " <<  (usecAfter - m_StartingUsec) << " microseconds" << endl;
	cout << "The Profiling information was writtend in the file \"NetPFLFfontendProfilingUsec.txt\"" << endl;
#endif

	if (m_GlobalInfo.Debugging.DebugLevel > 1)   
		nbPrintDebugLine("Generated NetIL code", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 2);

	m_MIRCodeGen = NULL;
	m_NetVMIRGen = NULL;
	
	return nbSUCCESS;
}


void NetPFLFrontEnd::DumpCFG(ostream &stream, bool graphOnly, bool netIL)
{
	CFGWriter cfgWriter(stream);
	cfgWriter.DumpCFG(m_CompUnit->Cfg, graphOnly, netIL);
}

void NetPFLFrontEnd::DumpFilter(ostream &stream, bool netIL)
{
	if (netIL)
	{
		stream << NetIL_FilterCode;
	}
	else
	{
		CodeWriter cw(stream);
		cw.DumpCode(&m_CompUnit->IRCode);
	}
}

int NetPFLFrontEnd::GenCode(PFLStatement *filterExpression)
{
	#ifdef ENABLE_PFLFRONTEND_PROFILING
		//mesure the time required for the construction of the automaton
		int64_t TicksAutomatonBefore, TicksAutomatonAfter, MeasureCost;
		MeasureCost= nbProfilerGetMeasureCost();
	#endif
	
	stmts = 0;
	SymbolLabel *filterFalseLbl = m_MIRCodeGen->NewLabel(LBL_CODE, "DISCARD");
	SymbolLabel *sendPktLbl = m_MIRCodeGen->NewLabel(LBL_CODE, "SEND_PACKET");
	SymbolLabel *actionLbl = m_MIRCodeGen->NewLabel(LBL_CODE, "ACTION");

	// [ds] created to bypass the if inversion bug
	SymbolLabel *filterExitLbl = m_MIRCodeGen->NewLabel(LBL_CODE, "EXIT");
	
	//here we set the protocol encapsulation graph to the EncapFSA class
	EncapGraph &protograph = (m_FullEncap)? m_GlobalInfo.GetProtoGraph() : m_GlobalInfo.GetProtoGraphPreferred();
	EncapFSA::SetGraph(&protograph);

	PFLAction *action = filterExpression->GetAction();
	nbASSERT(action, "action cannot be null")
	bool fieldExtraction = (action->GetType() == PFL_EXTRACT_FIELDS_ACTION)? true : false;
	
	m_StartProtoGenerated = false;

	#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksAutomatonBefore = nbProfilerGetTime();
	#endif
	
	#ifdef ENABLE_PFLFRONTEND_PROFILING_USEC
		uint64_t usecBefore = nbProfilerGetMicro();
	#endif

	//create the automaton representing the header chain
	EncapFSA *fsa = VisitFilterExpression(filterExpression->GetExpression(),false);//false: we are not generating the automaton related to the field extraction
	if(fsa==NULL)
		return nbFAILURE; //an error occurred during the creation of the automaton
	
    PRINT_DOT(fsa,"complete filter expression","completeFilter");    
       
    //initialize the info partition and create the automaton representing the field extraction
    if (fieldExtraction)
   	{   	  				
		EncapFSA *fsaExtractFields = VisitFieldExtraction((PFLExtractFldsAction*)action);
		if(fsaExtractFields==NULL) //an error occurrend during the creation of the automaton
			return nbFAILURE;
					
		PRINT_DOT(fsaExtractFields,"complete field extraction","completeExtraction");
		
		fsa->ResetFinals();
        
        //fsa_filter OR fsa_extraction
       	fsa = EncapFSA::BooleanOR(fsa,fsaExtractFields,false);
       	fsa->FixTransitions();
       	
	}   
	
	PRINT_DOT(fsa,"before reductions","before_reduction_fsa");
	
	/* If you need to apply further optimizations, please
	* do it before this call, maybe inside the
	* VisitFilterExpression call chain. This method must
	* be called immediately before the code generation.
	*/
	
	fsa = fixAutomaton(fsa);

	#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksAutomatonAfter= nbProfilerGetTime();
		m_Profiling << endl << endl << "The generation of the complete FSA used " << TicksAutomatonAfter - TicksAutomatonBefore - MeasureCost << " ticks";
	#endif
	
	#ifdef ENABLE_PFLFRONTEND_PROFILING_USEC
		uint64_t usecAfter= nbProfilerGetMicro();
		m_ProfilingUsec << "The generation of the complete FSA used " << usecAfter - usecBefore << " microseconds" << endl;
	#endif

    PRINT_DOT(fsa,"complete fsa representing the NetPFL rule","complete");
    
    //initialize the infoPartition to return the extracted values to the user application
	if(fieldExtraction)
	{
		if(!InitInfoPartition(sendPktLbl, sendPktLbl))
			return nbFAILURE;	//an error occurred during the initialization of the info partition
	}
    
    //we have to initialize the variables for the header indexing and the tunneled, i.e. the code associated with the input symbols
	GenHeaderIndexingCode(fsa);  
	GenTunneledCode(fsa); 
    
    m_fsa = fsa;
    
#ifndef ENABLE_PFLFRONTEND_PROFILING
	if (m_GlobalInfo.Debugging.DumpFilterAutomatonFilename)
		//print the automaton
		PrintFinalAutomaton(m_GlobalInfo.Debugging.DumpFilterAutomatonFilename);
#endif

    
	GenFSACode(fsa, actionLbl, filterFalseLbl, fieldExtraction); 

	m_MIRCodeGen->LabelStatement(actionLbl);
	m_MIRCodeGen->LabelStatement(sendPktLbl);
    // default outport == 1
	m_MIRCodeGen->GenStatement(m_MIRCodeGen->TermNode(SNDPKT, 1));

	// [ds] created to bypass the if inversion bug
	m_MIRCodeGen->JumpStatement(JUMPW, filterExitLbl);

	m_MIRCodeGen->LabelStatement(filterFalseLbl);

	// [ds] created to bypass the if inversion bug	
	m_MIRCodeGen->LabelStatement(filterExitLbl);

	m_MIRCodeGen->GenStatement(m_MIRCodeGen->TermNode(RET));
	//fprintf(stderr, "number of statements: %u\n", stmts);
	return nbSUCCESS;
}

/* 
* Executes further operations on the complete automaton 
*/		
EncapFSA *NetPFLFrontEnd::fixAutomaton(EncapFSA *fsa)
{	
	//FIXME: mettere protoGraph come variabile di classe, così tutte le volte evito di riprenderlo
	EncapGraph &protoGraph = (m_FullEncap)? m_GlobalInfo.GetProtoGraph() : m_GlobalInfo.GetProtoGraphPreferred();
    EncapFSA::Alphabet_t fsaAlphabet;
    EncapFSABuilder fsaBuilder(protoGraph, fsaAlphabet);    
    fsaBuilder.ReduceAutomaton(fsa);
	fsa->fixStateProtocols();
	fsa->fixActionStates();
	
	PRINT_DOT(fsa,"after fixing","after_fixing");
	
    return fsa;
}

void NetPFLFrontEnd::ReorderStates(list<EncapFSA::State*> *stateList)
{
	list<EncapFSA::State*>::iterator it = stateList->begin();
	it++; //the first state is not startproto
	for(; it != stateList->end(); it++)
	{
		if((*it)->GetInfo()->Name == "startproto")
			break;
	}
	
	nbASSERT(it!=stateList->end(),"Is this automaton without the state related to startprot?");
	list<EncapFSA::State*>::iterator aux = it;
	stateList->erase(it);
	stateList->push_front(*aux);

}

/*
 * Translates the complete automaton in MIR code
 * If fieldExtractTest != NULL, we're doing field extraction
 */
void NetPFLFrontEnd::GenFSACode(EncapFSA *fsa, SymbolLabel *trueLabel, SymbolLabel *falseLabel, bool fieldExtraction)
{
	IRLowering NetVMIRGen(*m_CompUnit, *m_MIRCodeGen, falseLabel,this);
	m_NetVMIRGen = &NetVMIRGen;

	FilterCodeGenInfo protoFilterInfo(*fsa, trueLabel, falseLabel);

	std::list<EncapFSA::State*> stateList = fsa->SortRevPostorder();
	
	std::list<EncapFSA::State*>::iterator i;

    // initialize all the additional state information we need
    sInfoMap_t statesInfo;

    //the first state translated in code must be the initial state, which is related to "startproto"
    if(stateList.size()!=0)
    {   
	    if((*(stateList.begin()))->GetInfo()==NULL || !((*(stateList.begin()))->GetInfo()->Name == "startproto"))
    		ReorderStates(&stateList);
    }
    
    std::list<EncapFSA::State*> l = fsa->GetFinalStates();
    
    //creates a label for each state
    for(i = stateList.begin() ; i != stateList.end(); ++i)
    {
          StateCodeInfo *info = new StateCodeInfo(*i);
          SymbolLabel *l_complete = m_MIRCodeGen->NewLabel(LBL_CODE, info->toString() + "_complete");
          SymbolLabel *l_fast = m_MIRCodeGen->NewLabel(LBL_CODE, info->toString() + "_fast");
          info->setLabels(l_complete, l_fast);
          statesInfo.insert( sInfoPair_t(*i, info) ); 
     }
        
        
    /* Emit a label at the beginning of the FSA code. This will
     * never be reached, but it works around some nasty bugs
     * dealing with empty filters.
     * The bottom line is: "there must always be at least a label in your code".
     */
    m_MIRCodeGen->LabelStatement(m_MIRCodeGen->NewLabel(LBL_CODE, "FSAroot"));
	
    
#ifdef OPTIMIZE_SIZED_LOOPS     
	/*
	*	Ivano:
	*	OPTIMIZED_SIZED_LOOPS currently is not defined, because this code is wrong.. It uses a data structure never initialized, and causes
	*	the "non-generation" of each sized loop defined in the NetPDL.
	*/
	for(i = stateList.begin() ; i != stateList.end(); i++)
	{
		EncapFSA::State *stateFrom = *i;	
		
		SymbolProto *stateProto = stateFrom->GetInfo();

		if(stateProto != NULL)
		{
			m_Protocol = stateProto; //current SymbolProto

			// get referred fields in ExtractField list
			if(m_GlobalSymbols.GetFieldExtractCode(m_Protocol).Front() != NULL)
			this->GetReferredFieldsExtractField(m_Protocol);

			// get referred fields in the protocol code
			this->GetReferredFields( &(m_GlobalSymbols.GetProtoCode(m_Protocol)) );

			// set sized loops to preserve
			for (FieldsList_t::iterator i = m_ReferredFieldsInFilter.begin(); i != m_ReferredFieldsInFilter.end(); i++)
			{
				StmtBase *loop = m_Protocol->Field2SizedLoopMap[*i];

				if (loop != NULL)
				{
					m_Protocol->SizedLoopToPreserve.push_front(loop);

					StmtBase *extLoop = m_Protocol->SizedLoop2SizedLoopMap[loop];
					while (extLoop != NULL)
					{
						m_Protocol->SizedLoopToPreserve.push_front(loop);
						extLoop = m_Protocol->SizedLoop2SizedLoopMap[extLoop];
					}
				}
			}

			// set sized loops to preserve
			for (FieldsList_t::iterator i = m_ReferredFieldsInCode.begin(); i != m_ReferredFieldsInCode.end(); i++)
			{
				StmtBase *loop = m_Protocol->Field2SizedLoopMap[*i];

				if (loop != NULL)
				{
					m_Protocol->SizedLoopToPreserve.push_front(loop);
					StmtBase *extLoop = m_Protocol->SizedLoop2SizedLoopMap[loop];
					while (extLoop != NULL)
					{
						m_Protocol->SizedLoopToPreserve.push_front(loop);
						extLoop = m_Protocol->SizedLoop2SizedLoopMap[extLoop];
					}
				}
			}
  		}
	}
#endif	

#ifdef ENABLE_PFLFRONTEND_PROFILING
	uint64_t TicksBeforeOpt, TicksAfterOpt; //used to measure the time required by each optimization algorithm		
	uint64_t MeasureCost;
#endif


#ifdef ENABLE_PFLFRONTEND_PROFILING
		MeasureCost    = nbProfilerGetMeasureCost();
		TicksBeforeOpt = nbProfilerGetTime();
#endif

#ifdef ENABLE_FIELD_OPT
//	set<SymbolProto*> alreadyCompacted; 
	#ifdef ENABLE_FIELD_DEBUG	
		ofstream my_ir("Debug_Fields_and_Loop_Optimizations_Stats.txt");
		my_ir <<"*** Loop & Field Optimization ***"<<endl;
	#endif

	//cout<<"BEFORE states-PROTOCOL-"<<endl;
	for(i = stateList.begin() ; i != stateList.end(); i++)
	{
	#ifdef ENABLE_FIELD_DEBUG
		CompField my_cf(m_GlobalSymbols,NetPDLParser.getUsedSymbMap(),&my_ir);
	#endif
	#ifndef ENABLE_FIELD_DEBUG
		CompField my_cf(m_GlobalSymbols,NetPDLParser.getUsedSymbMap());
	#endif
		SymbolProto *current = (*i)->GetInfo();
		set<SymbolProto*> protocols;
		if(current != NULL)
			protocols.insert(current);
		else
			/*
			*	This happens when multiple protocols are associated with the same state
			*/
			protocols = GetProtocols(*i,fsa);

		//cout<<"BEFORE PROTOCOL -"<<endl;		
		for(set<SymbolProto*>::iterator p = protocols.begin(); p != protocols.end(); p++)
		{
			//cout<<"PROTOCOL: "<<(*p)->Name<<" NO-SESSION: "<<(*p)->NoSession<<endl;

		#ifdef ENABLE_FIELD_DEBUG
			my_ir <<" Protocol(Before Loop & Field Optimization): "<<(*p)->Name;
			my_ir <<" Number of fields: "<<m_GlobalSymbols.GetNumField((*p))<<endl;
		#endif

/*			Ivano: i commented this code because the compiler is more efficient without it
			if(alreadyCompacted.count(*p) != 0)
				continue;
			cout << "New: " << (*p)->Name << endl;
			alreadyCompacted.insert(*p); */
	#ifdef ENABLE_FIELD_DEBUG
			my_ir <<" Protocol(Start-Visit for Loop & Field Optimization): "<<(*p)->Name<<endl;
	#endif
			my_cf.VisitCode(&(m_GlobalSymbols.GetProtoCode(*p))); 
		}
		
	}

	#ifdef ENABLE_FIELD_DEBUG
	my_ir.close();
	//Print Information about ALL the fields remained
	PrintFieldsInfo(1);
	#endif

#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
			TicksAfterOpt= nbProfilerGetTime();
			m_Profiling << endl << endl << "Field & Loop Optimization: " << (TicksAfterOpt - TicksBeforeOpt - MeasureCost) << " ticks";
#endif

	// start parsing the states and generating the code
	// the final not accepting state is not considered
	for(i = stateList.begin() ; i != stateList.end(); i++)
	{	
		GenProtoCode(*i, protoFilterInfo, statesInfo, fieldExtraction); 
	}
	
#ifdef ENABLE_PEG_AND_xpFSA_PROFILING
	ProfileAutomaton(stateList,fsa);
#endif		
	
	m_NetVMIRGen = NULL;
	
    m_MIRCodeGen->CommentStatement(""); // spacing
    m_MIRCodeGen->CommentStatement("defaulting the whole filter to FAILURE, if no accepting state was reached");
    m_MIRCodeGen->JumpStatement(JUMPW, protoFilterInfo.FilterFalseLbl);
    m_MIRCodeGen->CommentStatement("FSA implementation ends here");
    m_MIRCodeGen->CommentStatement(""); // spacing
 
}



	//TEMP
	//MIR Code is going to be generated!!
	//1. VISIT PROTOCOLS & CHECK IF (lookuptable are used && no-Empty is set) 
	//2. ADD CODE TO AVOID use of lookup-table
	/*
				set<SymbolProto*> protocols;
				protocols = GetProtocols(*i,fsa);	
				for(set<SymbolProto*>::iterator p = protocols.begin(); p != protocols.end(); p++)
				{
					//if( (*p)->FirstEncapsulationItem)
					if( (*p)->NoAck == true || (*p)->Name=="tcp" )  //TEMP -> we gotta check ENCAP (only?)!!
					{
					
						cout<<"PROTOCOL: "<<(*p)->Name<<endl;
						CodeList *code = &(m_GlobalSymbols.GetProtoCode(*p));
						if(code)
						{
						}
							cout<<"PROTOCOL END HERE"<<endl;		
					}
				}

	*/	
/*
bool NetPFLFrontEnd::CheckLabel(SymbolLabel *label)
{
	nbASSERT(label!=NULL,"Label can't be NULL! (CheckLabel)");
	return true;

	if(label->Proto->NoAck)
	{
		return true;	
	
	}	
	//return false;
}
*/

void NetPFLFrontEnd::CheckifCodeGotLookup(CodeList *code)
{
	nbASSERT(code!=NULL,"Code can't be NULL! (CheckIfCode)");
	Visit encapCode(code,m_GlobalSymbols);
	encapCode.VisitCode(code);
}


/*
* Given a state, this method returns a set containing the protocols associated with the state itself.
* To do this, the method analyzes the second protocol of the label on its incoming transitions.
* Note that the self loop is not considered.
*/
set<SymbolProto*> NetPFLFrontEnd::GetProtocols(EncapFSA::State* state, EncapFSA *fsa)
{
	std::list<EncapFSA::Transition*> trlist = fsa->getTransEnteringTo(state);
	set<SymbolProto*> protocols;
	
	for (std::list<EncapFSA::Transition*>::iterator tr_iter = trlist.begin();tr_iter != trlist.end();++tr_iter) 
	{  
		trans_pair_t trinfo = (*tr_iter)->GetInfo();
		if ((*tr_iter)->IsComplementSet())
	    {
	    	continue; 
	    	/*
	    	* Situation: final accepting state associated with multiple protocols and having at least an incoming
	    	* transition with a complement set of labels. This situation happens only in case of field extraction.
	    	* Here, when an accepting but non-final state does not lead to at least an action state, it goes
	    	* to an eatall state using this complement set transition. Being the eatall state a final accepting state,
	    	* only the eventual before and verify sections of the protocol associated with it should be executed.
	    	* Since these sections are usually used only in application layer protocols, and I'm not able to figure 
	    	* application layer protocols on this transition, I do not optimize this case. In any case, if the protocols
	    	* belong to the application layer, the code works, but their before and verify sections are not optimized.
	    	*/
	    }
	    else
	    {
	    	for(EncapFSA::Alphabet_t::iterator sym = trinfo.first.begin();sym != trinfo.first.end();++sym)
	      		protocols.insert((*sym).second);
		}	
	}  
  return protocols;
}


#ifdef ENABLE_PEG_AND_xpFSA_PROFILING
void NetPFLFrontEnd::ProfileAutomaton(std::list<EncapFSA::State*> stateList, EncapFSA *fsa)
{
	unsigned int statesCounter = 0;
	unsigned int totalEdges = 0;
	unsigned int selfLoopsCounter = 0;
	unsigned int towardLowerEqual = 0;
	unsigned int extendedTr	= 0;
	
	if(m_FullEncap)
	{
		/*
		*	This is needed because, into the method ProfilePeg(), we first profile the full peg and then the preferred one.
		*	Since the SymbolProto structure has only one attribute for the layer, and the profiling of the preferred peg could
		*	change the layer of a protocol, we profile the fullencap peg again in case it is the one referred into the creation 
		*	of the automaton.
		*/
		EncapGraph &protoGraph = (m_FullEncap)? m_GlobalInfo.GetProtoGraph() : m_GlobalInfo.GetProtoGraphPreferred();
		AssignProtocolLayer assignerFullEncap(protoGraph);
		assignerFullEncap.Classify();
	}
	
	// start parsing the states and generating the code
    // the final not accepting state is not considered
    stringstream ss;
	for(std::list<EncapFSA::State*>::iterator  i = stateList.begin() ; i != stateList.end(); i++)	
	{
		statesCounter++;
		list<EncapFSA::Transition*> transitions = fsa->getTransExitingFrom(*i);
		if(EncapFSA::ThereIsSelfLoop(transitions))
			selfLoopsCounter++;	
		SymbolProto *current = (*i)->GetInfo();
		if(current == NULL)
			continue;
		for(list<EncapFSA::Transition*>::iterator tr = transitions.begin(); tr != transitions.end(); tr++)
		{
			if((*tr)->getIncludingET())
				extendedTr++;
		
			EncapStIterator_t dest = (*tr)->ToState();
			SymbolProto *next = (&(*dest))->GetInfo();
			if(next==NULL)
				continue;
			if(current->Level >= next->Level)
			{
				ss << "\t\t" << current->Name << "(" << current->Level << ")" << " --> " << next->Name << "(" << next->Level << ")" << endl;
				towardLowerEqual++;
			}
			
		}
		totalEdges += transitions.size();
	}
	nbASSERT((extendedTr%2)==0,"You counted an odd number of extended transitions. How is it possible?");
	
	m_PegAndXPFSAProfiling << "Automaton" << endl;
	m_PegAndXPFSAProfiling << "\tStates: " << (statesCounter+1) << endl; 
	m_PegAndXPFSAProfiling << "\tTotal Transitions: " << (totalEdges+1) << endl;
	m_PegAndXPFSAProfiling << "\t\tnormal: " << (totalEdges+1-extendedTr/2) << endl;
	m_PegAndXPFSAProfiling << "\t\textended: " << (extendedTr/2) << endl; 
	m_PegAndXPFSAProfiling << "\tSelf transitions: " << selfLoopsCounter << endl;
	//If you want to count also the self loop over the final non-accepting state, print selfLoopsCounter+1
	m_PegAndXPFSAProfiling << "\tTransitions toward a protocol of a lower/equal layer: " << towardLowerEqual << endl;
	m_PegAndXPFSAProfiling << ss.str();
	m_PegAndXPFSAProfiling.flush();
	
	cout << "The Profiling information was written in the file \"PegAndXPFSAProfiling.txt\"" << endl;
}
#endif	

EncapFSA *NetPFLFrontEnd::VisitFilterBinExpr(PFLBinaryExpression *expr, bool fieldExtraction)
{
	EncapFSA *fsa1 = NULL;
	EncapFSA *fsa2 = NULL;
    EncapFSA *result = NULL;

	switch(expr->GetOperator()) {
	case BINOP_BOOLAND:
            fsa1 = VisitFilterExpression(expr->GetLeftNode(), fieldExtraction);
            if(fsa1==NULL)
            	return NULL; //an error occurred during the creation of the automaton

            fsa2 = VisitFilterExpression(expr->GetRightNode(), fieldExtraction);
            if(fsa2==NULL)
            	return NULL; //an error occurred during the creation of the automaton
          
            result = EncapFSA::BooleanAND(fsa1,fsa2);
            break;
          
	case BINOP_BOOLOR:
            fsa1 = VisitFilterExpression(expr->GetLeftNode(), fieldExtraction); 
            if(fsa1==NULL)
            	return NULL; //an error occurred during the creation of the automaton
            
            fsa2 = VisitFilterExpression(expr->GetRightNode(), fieldExtraction);
            if(fsa2==NULL)
            	return NULL; //an error occurred during the creation of the automaton

            PRINT_DOT(fsa1,"Before OR: FSA1","before_or_1");
            PRINT_DOT(fsa2,"Before OR: FSA2","before_or_2");

            result = EncapFSA::BooleanOR(fsa1, fsa2);

            PRINT_DOT(result,"After determinization","after_nfa2dfa");

            break;          

	default:
          nbASSERT(false, "CANNOT BE HERE");
          break;
	}

        PRINT_DOT(result,"Just before returning from VisitFilterBinExpr","visitfilter_end");
	return result;
}


EncapFSA *NetPFLFrontEnd::VisitFilterUnExpr(PFLUnaryExpression *expr, bool fieldExtraction)
{
	EncapFSA *fsa = NULL;
	switch(expr->GetOperator())
	{
	case UNOP_BOOLNOT:
		fsa = VisitFilterExpression(expr->GetChild(), fieldExtraction);
		if(fsa==NULL)
	            	return NULL; //an error occurred during the creation of the automaton
		fsa->BooleanNot();
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	return fsa;
}

EncapFSA *NetPFLFrontEnd::VisitFilterRegExpr(PFLRegExpExpression *expr, bool fieldExtraction)
{
	EncapFSA *regExpDFA = NULL;
	switch(expr->GetOperator())
	{
		case REGEXP_OP:
		{
				EncapGraph &protoGraph = (m_FullEncap)? m_GlobalInfo.GetProtoGraph() : m_GlobalInfo.GetProtoGraphPreferred();
                m_GlobalSymbols.UnlinkProtoLabels();
                protoGraph.RemoveUnconnectedNodes();
                EncapFSA::Alphabet_t fsaAlphabet;
                EncapFSABuilder fsaRegExpBuilder(protoGraph, fsaAlphabet);     
                EncapFSA *regExp = fsaRegExpBuilder.BuildRegExpFSA(expr->GetInnerList(),(fieldExtraction)? ACTION_STATE : ACCEPTING_STATE);
                
                //Set the fields involved in predicates as "non-compattable"
                //This way, they will not be merged with others in order to reduce the generated MIR code
#ifdef ENABLE_FIELD_OPT
                set<pair<string,string> > fields = fsaRegExpBuilder.GetPredicatesOnFields();
                for(set<pair<string,string> >::iterator current = fields.begin(); current != fields.end(); current++)
                {
                	SymbolField *field = m_GlobalSymbols.LookUpProtoFieldByName((*current).first,(*current).second);
                	field->Compattable = false;
			field->Used = true;

			#ifdef ENABLE_FIELD_DEBUG
				cout<<"Set Field Info (PFL) "<<field->Name<<endl;
			#endif
				NetPDLParser.SetFieldInfo(field);
                }
#endif               
                if(regExp==NULL)
                {
                	m_ErrorRecorder = fsaRegExpBuilder.GetErrRecorder();
                	//an error occurred and the automaton has not been built
					return NULL;
                }
                regExpDFA = EncapFSA::NFAtoDFA(regExp,false);
                std::list<EncapFSA::State*> action2 = regExpDFA->GetActionStates();	
                PRINT_DOT(regExpDFA,"After regexpfsa determinization","regexpfsa_after_nfa2dfa");
                
              	fsaRegExpBuilder.FixAutomaton(regExpDFA,(fieldExtraction)? ACTION_STATE : ACCEPTING_STATE);  
             	PRINT_DOT(regExpDFA,"The regexp FSA is complete!","regexpfsa_complete");
           
                return regExpDFA;
			break;
		}
		default:
			nbASSERT(false, "CANNOT BE HERE");
	}
	return regExpDFA;
}

/* 
* creates the automaton representing the extire extraction list
*/
EncapFSA *NetPFLFrontEnd::VisitFieldExtraction(PFLExtractFldsAction* action)
{	
	//we have to create the inner lists, in order to reuse the same algorithm defined for the header chain
	FieldsList_t &fields = action->GetFields();
	FieldsList_t::iterator f = fields.begin();

	set<pair<SymbolProto*,uint32> > alreadyFound; 
	list<PFLRegExpExpression*> headerChains;
	/*
	*	extraction, for each instance of a protocol, contains the set of
	*	fields to extract.
	*	extractfields(ip%1.src,ip%1.dst,ip%2.nextproto)
	*		->	<ip,1>: src, dst 
	*		->	<ip,2>: nextproto
	*/
	map< pair<SymbolProto*,uint32>, list<SymbolField*> > extraction;
	map< pair<SymbolProto*,uint32>, list<uint32> > where;
	map< pair<SymbolProto*,uint32>, list<bool> > multifield;

	for (; f != fields.end(); f++)
	{
		SymbolField *field = *f;
			
		SymbolProto *protocol = field->Protocol; 			//proto.field[*]
		list<uint32> headerIndexList = field->HeaderIndex; 	//proto%n.field[*]
		bool multiproto = field->MultiProto; 				//proto*.field
		uint32 headerIndex = 0;
		uint32 position = 0;		
		bool isMulti = false;
		
		if(!multiproto)
		{
			headerIndex = headerIndexList.front();
			headerIndexList.pop_front();
			headerIndexList.push_back(headerIndex);
			field->HeaderIndex = headerIndexList;
		}
		
		if(m_ManageStatesExtraInfo)
		{								
			position = field->Positions.front();
			field->Positions.pop_front();
			field->Positions.push_back(position);

			isMulti = field->IsMultiField.front();
			field->IsMultiField.pop_front();
			field->IsMultiField.push_back(isMulti);
		}	
				
		pair<SymbolProto*, uint32> p(protocol,headerIndex); //headerIndex == 0 in case of multiproto
		map< pair<SymbolProto*,uint32>, list<SymbolField*> >::iterator it = extraction.find(p);
		map< pair<SymbolProto*,uint32>, list<uint32> >::iterator wIt = where.find(p);
		map< pair<SymbolProto*,uint32>, list<bool> >::iterator mIt = multifield.find(p);
				
		if(alreadyFound.count(p) != 0)
		{
			//the new field must be added to the list
			nbASSERT(it != extraction.end(), "There is a BUG!");
			nbASSERT(wIt != where.end(), "There is a BUG!");
			nbASSERT(mIt != multifield.end(), "There is a BUG!");
			list<SymbolField*> fieldList = (*it).second;
			list<uint32> positionList = (*wIt).second;
			list<bool>	multifieldList = (*mIt).second;
			fieldList.push_back(field);
			positionList.push_back(position);
			multifieldList.push_back(isMulti);
			extraction[p] = fieldList;
			where[p] = positionList;
			multifield[p] = multifieldList;
			continue;
		}
		
		//the new field must be added to the list
		list<SymbolField*> fieldList;
		fieldList.push_back(field);
		extraction[p] = fieldList;
		list<uint32> positionList;
		positionList.push_back(position);
		where[p] = positionList;
		list<bool> multifieldList;
		multifieldList.push_back(isMulti);
		multifield[p] = multifieldList;
		//create the terminal expression
		PFLTermExpression *expression = new PFLTermExpression(protocol,headerIndex);
		//create the set expression
		list<PFLExpression*> *elements = new list<PFLExpression*>();
		elements->push_back(expression);
		PFLSetExpression *setExpr = new PFLSetExpression(elements,SET_OP,false); //tunneled is false
		setExpr->SetAnyPlaceholder(false);
		//create the regexp expression
		list<PFLSetExpression*> *sets = new list<PFLSetExpression*>();
		sets->push_back(setExpr);
		PFLRegExpExpression *expr = new PFLRegExpExpression(sets, REGEXP_OP); 
		if (expr == NULL)
			return NULL;
		headerChains.push_back(expr);
		alreadyFound.insert(p);
	}	
	

				
	list<PFLRegExpExpression*>::iterator hc = headerChains.begin();
	EncapFSA *fsa = VisitFilterExpression(*hc,true);
	if(fsa == NULL)
		return NULL;	
	fsa = AssignFields(fsa,*hc,extraction,where,multifield);    	
	fsa->ResetFinals();
	PRINT_DOT(fsa,"FSA representing the first element of the extraction list","first_extraction_fsa");
	
	hc++;
	for(;hc != headerChains.end(); hc++)
	{
		EncapFSA *currentFSA = VisitFilterExpression(*hc,true);
		if(currentFSA == NULL)
			return NULL;
		currentFSA = AssignFields(currentFSA,*hc,extraction,where,multifield);
		
		currentFSA->ResetFinals();
		PRINT_DOT(currentFSA,"FSA representing an element of the extraction list","extraction_fsa");
		fsa = EncapFSA::BooleanOR(fsa, currentFSA);
        PRINT_DOT(fsa,"FSA representing the or of two automata for the extraction","or_of_extraction_fsa");
	}
	return fsa;
}

/*
*	Assign to an action state, the fields to extract, the related position in the info partition, and the information about multifield
*/
EncapFSA *NetPFLFrontEnd::AssignFields(EncapFSA *fsa, PFLRegExpExpression* hc,map< pair<SymbolProto*,uint32>, list<SymbolField*> > extraction, map< pair<SymbolProto*,uint32>, list<uint32> >  position, map< pair<SymbolProto*,uint32>, list<bool> >  multiField)
{
	uint32 hi = hc->GetHeaderIndex();
	SymbolProto *proto = hc->GetProtocol();
	pair<SymbolProto*,uint32> p(proto,hi);
		
	//fields
	{
		map< pair<SymbolProto*,uint32>, list<SymbolField*> >::iterator it = extraction.find(p);
		nbASSERT(it != extraction.end(), "There is a BUG!");
		list<SymbolField*> fieldList = (*it).second;
		fsa->SetExtraInfoToAction(fieldList);
	}
	
	//positions
	{
		map< pair<SymbolProto*,uint32>, list<uint32> >::iterator it = position.find(p);
		nbASSERT(it != position.end(), "There is a BUG!");
		list<uint32> positionList = (*it).second;
		fsa->SetExtraInfoPlusToAction(positionList);
	}
	
	//multifield
	{
		map< pair<SymbolProto*,uint32>, list<bool> >::iterator it = multiField.find(p);
		nbASSERT(it != multiField.end(), "There is a BUG!");
		list<bool> mfList = (*it).second;
		fsa->SetOtherInfoToAction(mfList);
	}
	
	return fsa;
}

/*
*	Initialize the info partition to return the extracted values to the user application
*/
bool NetPFLFrontEnd::InitInfoPartition(SymbolLabel *trueLabel, SymbolLabel *falseLabel)
{
	IRLowering NetVMIRGen(*m_CompUnit, *m_MIRCodeGen, falseLabel,this);
	m_NetVMIRGen = &NetVMIRGen;

	set<SymbolField*> already;

	for (FieldsList_t::iterator i = m_FieldsList.begin(); i != m_FieldsList.end(); i++)
	{	
		if(already.count(*i))
			continue;
		already.insert(*i);
	
		list<uint32> positions = (*i)->Positions;
		list<bool> multiField = (*i)->IsMultiField;
		
		switch((*i)->FieldType)
		{
			case PDL_FIELD_ALLFIELDS:
				{
					/*
					*	allfields can be specified only on the last protocol of the list. So we must check if this is true in the filter
					*/
					FieldsList_t::iterator aux = i;
					aux++;
					if(aux!=m_FieldsList.end())
					{
						//there is an error in the filter
						m_ErrorRecorder.PFLError("You can specify \"allfields\" only as last element of the filter, e.g. \"extracfields(proto1.x,proto2.allfields)\"");
						return false;
					}
					for(list<uint32>::iterator p = positions.begin(); p != positions.end(); p++)
					{
						m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p)));
						m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p+2)));
					}
				}
			break;

			case PDL_FIELD_BITFIELD:
				for(list<uint32>::iterator p = positions.begin(); p != positions.end(); p++)
				{
					m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p)));
					m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p+2)));
				}
			break;
			
			case PDL_FIELD_FIXED:
			{
				//FIXME: ivano - questa cosa me la sono trovata: perché il multiproto e multifield si guardano solo per i campi a lunghezza fissa?
				nbASSERT(positions.size() == multiField.size(),"Very strange! There should be a BUG!");
				list<bool>::iterator m = multiField.begin();
				for(list<uint32>::iterator p = positions.begin(); p != positions.end(); p++, m++)
				{
					if((*i)->MultiProto)
					{
						nbASSERT(*m == false,"There is a BUG!");
						m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p)));
						m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p+2)));
					}
					else if(*m)
					{
						m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p)));
						m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p+2)));
					}
					else
					{
							m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p)));
							m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(IISTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p+2)));
					}
				}
			}
			break;

			default:
				for(list<uint32>::iterator p = positions.begin(); p != positions.end(); p++)
				{
					m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(ISSTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p)));
					m_MIRCodeGen->GenStatement(m_MIRCodeGen->BinOp(ISSTR, m_MIRCodeGen->TermNode(PUSH,(uint32)0), m_MIRCodeGen->TermNode(PUSH,*p+2)));
				}
			break;
		}	
	}
	
	return true;
	
}

/*
*	Creates an fsa with only the startproto
*/ 
EncapFSA *NetPFLFrontEnd::VisitNullExpr() 
{
	EncapGraph &protoGraph = (m_FullEncap)? m_GlobalInfo.GetProtoGraph() : m_GlobalInfo.GetProtoGraphPreferred();
	EncapFSA::Alphabet_t fsaAlphabet;
	EncapFSABuilder fsaBuilder(protoGraph, fsaAlphabet);
	return fsaBuilder.BuildSingleNodeFSA();
}

EncapFSA *NetPFLFrontEnd::VisitFilterExpression(PFLExpression *expr, bool fieldExtraction)
{
	EncapFSA *fsa = NULL;
	
	if(expr != NULL)
	{
		switch(expr->GetType())
		{
		case PFL_BINARY_EXPRESSION:
			fsa = VisitFilterBinExpr((PFLBinaryExpression*)expr, fieldExtraction);
			break;
		case PFL_UNARY_EXPRESSION:
			fsa = VisitFilterUnExpr((PFLUnaryExpression*)expr, fieldExtraction);
			break;
		case PFL_REGEXP_EXPRESSION:
			fsa = VisitFilterRegExpr((PFLRegExpExpression*)expr, fieldExtraction);
			break;
		default:
			nbASSERT(false,"You cannot be here!");
		}
	}
	else
		//this happens if the filter has not been specified. However an automaton is created also in this case, and it has the state related to the startproto which is also the final accepting state of the FSA
		fsa = VisitNullExpr();
	return fsa;
}

/*
*	Initializes variables needed for the field extraction.
*	Menwhile, it marks as non-compattable the fields invovled into the extraction.
*/
void NetPFLFrontEnd::ParseExtractField(FieldsList_t *fldList)
{
	uint32 count=0;
	
	map<SymbolField*,uint32> counter; //counts the times a certain field has already been encountered in the extraction list
									  //needed to identify the instance which wants "multifields"
	for(FieldsList_t::iterator i = fldList->begin(); i!= fldList->end();i++)
	{	
		SymbolField *fld=m_GlobalSymbols.LookUpProtoField((*i)->Protocol,(*i));
		nbASSERT((*i)->Protocol != NULL,"There is a BUG!");
		nbASSERT(fld != NULL,"There is a BUG!");
		fld->Positions.push_back(count);
		
#ifdef ENABLE_FIELD_OPT
		//fld, being used into the NetPFL filter, cannot be merged with others in order to reduce the MIR code that will be generated
		if(fld->Name != "allfields")
		{
			fld->Compattable = false;
			fld->Used=true;
			#ifdef ENABLE_FIELD_DEBUG
				cout <<"Set Field Info (PFL) "<<fld->Name<<endl;
			#endif
			NetPDLParser.SetFieldInfo(fld);
		}
		else
		{
			//no field of the current protocol can be merged, since the filtering expression requires the extraction from "allfields"
			uint32_t numFields = m_GlobalSymbols.GetNumField(fld->Protocol);
			SymbolField **fields = m_GlobalSymbols.GetFieldList(fld->Protocol); 
			for(uint32_t i = 0; i < numFields; i++)
			{
				SymbolField *current = fields[i];
				current->Compattable = false;
				current->Used= true;

			#ifdef ENABLE_FIELD_DEBUG
				cout <<"Set Field Info (PFL) "<<current->Name<<endl;
			#endif
			NetPDLParser.SetFieldInfo(current);
			}
		}
#endif		
		
		map<SymbolField*,uint32>::iterator c = counter.find(fld);
		uint32 cntr = 0;
		if(c != counter.end())
			cntr = (*c).second;
		cntr++;
		counter[fld] = cntr;
		
		if(fld->MultiFields.count(cntr)>0)
			fld->IsMultiField.push_back(true);
		else
			fld->IsMultiField.push_back(false);
		
		if((*i)->FieldType==PDL_FIELD_ALLFIELDS)
		{	
			/* 
			* ALLFIELDS 
			* 	proto.allfields
			* 	proto*.allfields
			* 	proto%n.allfields
			*	
			*	two variables are needed:
			*		allfields_position indicates the current position in the infopartition
			*		allfields_count counts the number of extracted fields
			*/
			StmtFieldInfo *fldInfoStmt= new StmtFieldInfo((*i),count);
			m_GlobalSymbols.AddFieldExtractStatment((*i)->Protocol,fldInfoStmt);
			(*i)->Protocol->position= m_MIRCodeGen->NewTemp(string("allfields_position"), m_CompUnit->NumLocals);
			(*i)->Protocol->beginPosition=count;
			this->m_MIRCodeGen->CommentStatement(string("Initializing allfields position and counter"));
			this->m_MIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_allfields")));
			m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(LOCST,m_MIRCodeGen->TermNode(PUSH,count),(*i)->Protocol->position));
			(*i)->Protocol->NExFields= m_MIRCodeGen->NewTemp(string("allfields_count"), m_CompUnit->NumLocals);
			m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(LOCST,m_MIRCodeGen->TermNode(PUSH,(uint32)0),(*i)->Protocol->NExFields));
		}
		
		StmtFieldInfo *fldInfoStmt= new StmtFieldInfo(fld,count);
		m_GlobalSymbols.AddFieldExtractStatment((*i)->Protocol,fldInfoStmt);
				
		if((*i)->FieldType!=PDL_FIELD_ALLFIELDS)
		{
			if(fld->MultiProto && fld->Protocol->ExtractG == NULL)
			{
				/*
				*	proto_ExtractG: counts the number of time an header related to proto has been found into the packet
				*	It is needed in case of multiproto without allfields
				*/
				fld->Protocol->ExtractG = m_MIRCodeGen->NewTemp(fld->Protocol->Name + string("_ExtractG"), m_CompUnit->NumLocals);
				this->m_MIRCodeGen->CommentStatement(string("Initializing Header Counter for protocol ") + fld->Protocol->Name);
				this->m_MIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_") + fld->Protocol->Name + string("_ExtractG")));
				m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(LOCST,m_MIRCodeGen->TermNode(PUSH,(uint32)0), fld->Protocol->ExtractG)); //proto_ExtractG = 0
			}

			/*
			*	field_counter: counts the number of time field has been encountered in the current header
			*	It is always needed, except in case of allfields
			*/
			if(fld->FieldCount == NULL)
			{
				fld->FieldCount = m_MIRCodeGen->NewTemp(fld->Name + string("_counter"), m_CompUnit->NumLocals);
				this->m_MIRCodeGen->CommentStatement(string("Initializing Field Counter for field ") + fld->Name);
				this->m_MIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_") + fld->Protocol->Name + string("_") + fld->Name + string("_Counter")));
				m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(LOCST,m_MIRCodeGen->TermNode(PUSH,(uint32)0), fld->FieldCount)); //field_counter = 0
	
				fld->Protocol->fields_ToExtract.push_back(fld);
			}
		}
		
		/*
		*	Calculates the position of the next field in the info partition
		*/		
		if (fld->MultiProto || fld->MultiFields.count(cntr) > 0)
			count += nbNETPFLCOMPILER_INFO_FIELDS_SIZE * (1 + nbNETPFLCOMPILER_MAX_PROTO_INSTANCES);
		else
			count += nbNETPFLCOMPILER_INFO_FIELDS_SIZE;
	}

}


/*
*	Understands the action specified in the NetPFL rule
*/
void NetPFLFrontEnd::VisitAction(PFLAction *action)
{
	switch(action->GetType())
	{
	case PFL_EXTRACT_FIELDS_ACTION:
		{
			PFLExtractFldsAction * ExFldAction = (PFLExtractFldsAction *)action;
			m_FieldsList = ExFldAction->GetFields();
			ParseExtractField(&m_FieldsList);
		}
		break;
	case PFL_CLASSIFY_ACTION:
		//not yet implemented
		break;
	case PFL_RETURN_PACKET_ACTION:
		break;
	}
}

/*
* initialize the variables for the header indexing
*/
void NetPFLFrontEnd::GenHeaderIndexingCode(EncapFSA *fsa)
{	
	EncapFSA::Instruction_t instructions = fsa->m_Instruction1;
	
	if(instructions.size()>0)
	{
		this->m_MIRCodeGen->CommentStatement(string("Initializing counters for header indexing"));
		this->m_MIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_counters_for_header_indexing")));		
		m_AutomatonWithCode = true;
	}
	
	for(EncapFSA::Instruction_t::iterator i = instructions.begin(); i != instructions.end(); i++)
	{
		list<Code*> codeList = (*i).second;
		nbASSERT(codeList.size()==1,"This case has not been considered!");
		for(list<Code*>::iterator code = codeList.begin(); code != codeList.end(); code++)
		{
			SymbolTemp *var = m_MIRCodeGen->NewTemp(string("indexing_")+((*i).first)->ToString(), m_CompUnit->NumLocals); //create a new variable...
			nbASSERT(var->SymKind==SYM_TEMP,"There is a bug!");
			m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(LOCST,m_MIRCodeGen->TermNode(PUSH,(uint32)0), var)); //... and initialize it to zero
			(*code)->SetVariable(var);//add the variable to the automaton
		}
	}
}	

/*
* Initialize the variables for the tunneled
* For each layer wa have to count, a single variable is created.
*/
void NetPFLFrontEnd::GenTunneledCode(EncapFSA *fsa)
{
	map<double,SymbolTemp*> layers;	//for each layer, contains the related counter
	
	EncapFSA::Instruction_t instructions = fsa->m_Instruction2;

	if(instructions.size()>0)
	{
		m_AutomatonWithCode = true;
		this->m_MIRCodeGen->CommentStatement(string("Initializing counters for tunneled"));
		this->m_MIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_counters_for_tunneled")));
	}
	
	for(EncapFSA::Instruction_t::iterator i = instructions.begin(); i != instructions.end(); i++)
	{
		list<Code*> codeList = (*i).second;
		for(list<Code*>::iterator code = codeList.begin(); code != codeList.end(); code++)
		{
			string c = (*code)->GetCode();		
			SymbolTemp *var = NULL;
			//we have to get the level, from the code
			double level;					
			size_t dot_pos = c.rfind('.');
 	 		nbASSERT(dot_pos!=string::npos, "The name is wrong!");
 			string strLevel = c.substr(0, dot_pos);
  			char aux;
			sscanf(strLevel.c_str(),"%c%lf",&aux,&level);
			
			if(layers.count(level)==0)
			{
				//this protocol increments a counter not initialized yet.		
				stringstream s;
				s << level;
				var = m_MIRCodeGen->NewTemp(string("tunneled_level")+s.str(), m_CompUnit->NumLocals); //create a new variable...
				nbASSERT(var->SymKind==SYM_TEMP,"There is a bug!");
				m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(LOCST,m_MIRCodeGen->TermNode(PUSH,(uint32)0), var)); //... and initialize it to zero
				layers[level] = var;
			}
			else
			{
				//the counter has already been created
				map<double,SymbolTemp*>::iterator exist = layers.find(level);
				var = (*exist).second;
			}
			(*code)->SetVariable(var);
		}
	}
}	

void NetPFLFrontEnd::GenStringMatchingEntries(void)
{
	RegExList_t list = this->m_GlobalSymbols.GetStringMatchingListUseful();
	int smEntries= this->m_GlobalSymbols.GetStringMatchingEntriesCountUseful();
	
	if (smEntries>0)
	{
		int index = /*0*/1 + smEntries;
		char bufIndex[10];
		char sensitiveStr[10];
		char bufPatLen[10];
		char bufEntriesCount[10];

		//itoa(regexEntries, bufEntriesCount, 10);
		snprintf(bufEntriesCount, 9, "%d", smEntries);

		SymbolDataItem *smData = new SymbolDataItem(string("stringmatching_data"), DATA_TYPE_WORD, string(bufEntriesCount), index);
		this->m_GlobalSymbols.StoreDataItem(smData);
		
		
		index = 1;
		for (RegExList_t::iterator i=list.begin(); i!=list.end();i++,index++)
		{
			snprintf(bufIndex, 9, "%d", index);

			int sensitive=( (*i)->CaseSensitive? 0 : 1 );
			//int optLenValue=( (*i)->CaseSensitive? 2 : 3 );
			snprintf(sensitiveStr, 9, "%d", sensitive);

			//int escapes=0;
			int slashes=0;
			int length=(*i)->Pattern->Size;
			for (int cIndex=0; cIndex<length; cIndex++)
			{
				if ((*i)->Pattern->Name[cIndex]=='\\')
				{
					slashes++;
				}
			}

			length-=slashes/2;
			snprintf(bufPatLen, 9, "%d", length);
	
			SymbolDataItem *pathNo = new SymbolDataItem(string("path_no_").append(bufIndex), DATA_TYPE_WORD, string("1"), index);
			this->m_GlobalSymbols.StoreDataItem(pathNo); 		
			
			SymbolDataItem *pathLen = new SymbolDataItem(string("path_length_").append(bufIndex), DATA_TYPE_WORD, string(bufPatLen), index);
			this->m_GlobalSymbols.StoreDataItem(pathLen);
		

			SymbolDataItem *caseSensitive = new SymbolDataItem(string("case_").append(bufIndex), DATA_TYPE_WORD,sensitiveStr, index);
			this->m_GlobalSymbols.StoreDataItem(caseSensitive);
		
			SymbolDataItem *dataRet = new SymbolDataItem(string("data_").append(bufIndex), DATA_TYPE_DOUBLE, string("0"), index);
			this->m_GlobalSymbols.StoreDataItem(dataRet);
		

			SymbolDataItem *pattern = new SymbolDataItem(string("pattern_").append(bufIndex), DATA_TYPE_BYTE, string("\"").append(string((*i)->Pattern->Name)).append("\""), index);
			this->m_GlobalSymbols.StoreDataItem(pattern);
		}
	}
}

void NetPFLFrontEnd::GenRegexEntries(void)
{
	RegExList_t list = m_GlobalSymbols.GetRegExListUseful();
	int regexEntries=  this->m_GlobalSymbols.GetRegExEntriesCountUseful();
	if (regexEntries>0)
	{
		int index = /*0*/1 + regexEntries;
		char bufIndex[10];
		char bufOptLen[10];
		char bufOpt[10]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		char bufPatLen[10];

		char bufEntriesCount[10];

		//itoa(regexEntries, bufEntriesCount, 10);
		snprintf(bufEntriesCount, 9, "%d", regexEntries);
		SymbolDataItem *regexData = new SymbolDataItem(string("regexp_data"), DATA_TYPE_WORD, string(bufEntriesCount), index);
		//m_CodeGen->GenStatement(m_CodeGen->TermNode(IR_DATA, regexData));
		this->m_GlobalSymbols.StoreDataItem(regexData);
	

		index = 1;
		for (RegExList_t::iterator i=list.begin(); i!=list.end(); i++, index++)
		{
			snprintf(bufIndex, 9, "%d", index);

			int optLenValue=1;
			snprintf(bufOptLen, 9, "%d", optLenValue);

			//int escapes=0;
			int slashes=0;
			int length=(*i)->Pattern->Size;
			for (int cIndex=0; cIndex<length; cIndex++)
			{
				if ((*i)->Pattern->Name[cIndex]=='\\')
				{
					slashes++;
				}
			}

			length-=slashes/2;
			snprintf(bufPatLen, 9, "%d", length);

			if ((*i)->CaseSensitive)
				strncpy(bufOpt, "s", 1);
			else
				strncpy(bufOpt, "i", 1);

			SymbolDataItem *optLen = new SymbolDataItem(string("opt_len_").append(bufIndex), DATA_TYPE_WORD, string(bufOptLen), index);
			this->m_GlobalSymbols.StoreDataItem(optLen);

			SymbolDataItem *opt = new SymbolDataItem(string("opt_").append(bufIndex), DATA_TYPE_BYTE, string("\"").append(string(bufOpt)).append("\""), index);
			this->m_GlobalSymbols.StoreDataItem(opt);

			SymbolDataItem *patternLen = new SymbolDataItem(string("pat_len_").append(bufIndex), DATA_TYPE_WORD, string(bufPatLen), index);
			this->m_GlobalSymbols.StoreDataItem(patternLen);

			SymbolDataItem *pattern = new SymbolDataItem(string("pat_").append(bufIndex), DATA_TYPE_BYTE, string("\"").append(string((*i)->Pattern->Name)).append("\""), index);

			this->m_GlobalSymbols.StoreDataItem(pattern);	
		}	
	}

}


//this method generates initialization MIR code for the lookup_ex, the regex and the string matching coprocessors (if are needed)
void NetPFLFrontEnd::GenInitCode()
{
	this->m_MIRCodeGen->CommentStatement(string("INITIALIZATION"));
	this->m_MIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init")));

	//REGEXP
	if (this->m_GlobalSymbols.GetRegExEntriesCountUseful()>0)
	{
		//the regex coprocessor is needed
		this->m_MIRCodeGen->CommentStatement(string("Initialize Regular Expression coprocessor"));
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(POP,
			m_MIRCodeGen->UnOp(COPINIT, new SymbolLabel(LBL_ID, 0, "regexp"), 0, new SymbolLabel(LBL_ID, 0, "regexp_data"))));
	}

	//STRINGMATCHING
	if (this->m_GlobalSymbols.GetStringMatchingEntriesCountUseful()>0)
	{
		//the stringmatching coprocessor is needed
		this->m_MIRCodeGen->CommentStatement(string("Initialize String Matching coprocessor"));
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(POP,
			m_MIRCodeGen->UnOp(COPINIT, new SymbolLabel(LBL_ID, 0, "stringmatching"), 0, new SymbolLabel(LBL_ID, 0, "stringmatching_data"))));
	}
	
	//LOOKUP
	LookupTablesList_t lookupTablesList = this->m_GlobalSymbols.GetLookupTablesList();
	int usefulTables = m_GlobalSymbols.GetUsefulTablesNumber();
	
	if(usefulTables>0)
	{
		//the lookup_ex coprocessor is needed
		m_MIRCodeGen->CommentStatement(string("Initialize Lookup_ex coprocessor"));	
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(COPOUT,
			m_MIRCodeGen->TermNode(PUSH, usefulTables),
			new SymbolLabel(LBL_ID, 0, "lookup_ex"), LOOKUP_EX_OUT_TABLE_ID));
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, "lookup_ex"), LOOKUP_EX_OP_INIT));
	}
	for (LookupTablesList_t::iterator i = lookupTablesList.begin(); i!=lookupTablesList.end();i++)
	{
		if(!m_GlobalSymbols.IsLookUpTableUseful((*i)->Name))
			continue;
			
		SymbolLookupTable *table = (*i);
		
		m_MIRCodeGen->CommentStatement(string("Initialize the table ").append(table->Name));

		// set table id
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(COPOUT,
			m_MIRCodeGen->TermNode(PUSH, table->Id),
			table->Label,
			LOOKUP_EX_OUT_TABLE_ID));

		// set entries number
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(COPOUT,
			m_MIRCodeGen->TermNode(PUSH, table->MaxExactEntriesNr+table->MaxMaskedEntriesNr),
			table->Label,
			LOOKUP_EX_OUT_ENTRIES));


		// set data size
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(COPOUT,
			m_MIRCodeGen->TermNode(PUSH, table->KeysListSize),
			table->Label,
			LOOKUP_EX_OUT_KEYS_SIZE));

		// set value size
		uint32 valueSize = table->ValuesListSize;
		if (table->Validity==TABLE_VALIDITY_DYNAMIC)
		{
			// hidden values
			for (int i=0; i<HIDDEN_LAST_INDEX; i++)
				valueSize += (table->HiddenValuesList[i]->Size/sizeof(uint32));
		}
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(COPOUT,
			m_MIRCodeGen->TermNode(PUSH, valueSize),
			table->Label,
			LOOKUP_EX_OUT_VALUES_SIZE));

		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(COPRUN, table->Label, LOOKUP_EX_OP_INIT_TABLE));
	}//end iteration over the lookup tables		
}


/*
 * Emit the MIR code of stateFrom
 */
void NetPFLFrontEnd::GenProtoCode(EncapFSA::State *stateFrom, FilterCodeGenInfo &protoFilterInfo, const sInfoMap_t statesInfo, bool fieldExtraction)
{	
  // safety check
  if (stateFrom->isFinal())
    nbASSERT(stateFrom->isAccepting(), "A final, non-accepting state should not have got so far.");

  sInfoMap_t::const_iterator infoPtr = statesInfo.find(stateFrom);
  nbASSERT(infoPtr!=statesInfo.end(), "The info about me should be already initialized");
 
  StateCodeInfo *curStateInfo = infoPtr->second;
  EncapFSA *fsa = &(protoFilterInfo.fsa);
  
  // 1) emit the "complete" label
  m_MIRCodeGen->CommentStatement(""); // spacing
  m_MIRCodeGen->LabelStatement(curStateInfo->getLabelComplete());

  // emit the before and format sections only if they are useful (i.e. they were not generated on entering ET or we are
  // doing field extraction
  if(NoFormat.count(stateFrom)==0)
  {
	  // 2) emit the before section for the protocol, if it is the only one of the current state  
	  if(stateFrom->GetInfo()!=NULL)
	  	GenerateBeforeCode( stateFrom->GetInfo() );

	  // 3) emit the FORMAT section, in case of this state is not final (or we are doing field extraction) and it is related to a single protocol. 
	  //	Note that the code to extract the required fields (if any) are generated here
	  bool action = stateFrom->isAction();
	  if( (stateFrom->GetInfo()!=NULL) && (!stateFrom->isFinal() || action))
	  {
	  	manageRegExStringMarchCopros(stateFrom->GetInfo());

	  	m_NetVMIRGen->LowerHIRCode(&(m_GlobalSymbols.GetProtoCode(stateFrom->GetInfo())),
						               stateFrom->GetInfo(),
						               action,
						               stateFrom->GetExtraInfo(),
						               stateFrom->GetExtraInfoPlus(),
						               stateFrom->GetOtherInfo(),
						               "emitting FORMAT for proto inside " + curStateInfo->toString());
		m_MIRCodeGen->CommentStatement("DONE emitting the proto FORMAT");

	  }
   }

  // 4) emit the middle ("fast") label
  m_MIRCodeGen->LabelStatement(curStateInfo->getLabelFast());
  
  // 5) if we are in a final accepting state, we're done. Emit a jump and return
  if(stateFrom->isFinal())
  {
    m_MIRCodeGen->JumpStatement(JUMPW, protoFilterInfo.FilterTrueLbl );
    return;
  }

  // 6) cycle on all transitions and prepare the data structures
  std::map<string, SymbolLabel*> toStateMap; 									/* map of the states reachable from this one
  																				 */
  std::map<EncapFSA::ExtendedTransition*, SymbolLabel*> et_tohandle;		    /* map of the ET that must be handled. The mapped value is a
																				  temporary label at which position all the code for the ET will be
																				  generated.*/


 //FIXME: Ivano - this variable should be useless
  /*std::map<EncapFSA::ExtendedTransition*, bool> endFinal;*/				/* for each ET that must be handled, this map says if at least one of the destination state is final or not*/

  std::map<string, EarlyCodeEmission_t> early_protos;							/* map of protocols for which we need to emit early the before code
																				 * (needed when the destination state does not identify a unique
																				 * protocol, but the before code for it must be emitted)
																				 */	
   std::map<EncapLabel_t,SymbolLabel*> symbols_code_tohandle;					/* map of the symbols having code that must be executed before the reaching of the next state */
  std::map<EncapLabel_t,SymbolLabel*> after_symbols_code;						/* map of the labels to jump after the handling of the code related to a symlo*/
  
  std::list<EncapFSA::Transition*> trlist = fsa->getTransExitingFrom(stateFrom);


//	bool flag=false;



  	for (std::list<EncapFSA::Transition*>::iterator tr_iter = trlist.begin();tr_iter != trlist.end();++tr_iter) 
	{  
      

    // GetInfo() returns a trans_pair_t containing a Alphabet_t and a PType object
    trans_pair_t trinfo = (*tr_iter)->GetInfo();
    
    if((*tr_iter)->getIncludingET() == NULL) 
    { 
      if((&(*((*tr_iter)->ToState())))->isFinal() && !(&(*((*tr_iter)->ToState())))->isAccepting())
      /*
      * Since the transition goes toward a final non-accepting state, we do not insert any protocol here. This way,
      * we avoid to generate:
      *	- complemented transitions: jumps related to nextproto belonging to the used peg (preferred or fullencap)
      	   (all the code related to the nextproto-candidate is avoided in another way)
      *	- non-complemented transitions: jumps related to nextproto - jumps, verify (and before) related to 
      *    nextproto-candidate belonging to the used peg (preferred or fullencap) - Note that this casa should happen
      *	   only in case of field extraction.
      */		  
    	continue;
    
      // if the transition has no predicate, then simply set the target state as the destination of the jump

      sInfoMap_t::const_iterator to_s = statesInfo.find(&* ((*tr_iter)->ToState()) );
      if (to_s == statesInfo.end()) {

        // jump to fail
        if ((*tr_iter)->IsComplementSet())
        	toStateMap[LAST_RESORT_JUMP] = protoFilterInfo.FilterFalseLbl;
        else
          for(EncapFSA::Alphabet_t::iterator sym = trinfo.first.begin();sym != trinfo.first.end();++sym)
          	toStateMap[(*sym).second->Name] = protoFilterInfo.FilterFalseLbl;

      } 
      else {

        if ((*tr_iter)->IsComplementSet())
          toStateMap[LAST_RESORT_JUMP] = (*to_s).second->getLabelComplete();
        else
          for(EncapFSA::Alphabet_t::iterator sym = trinfo.first.begin();
            sym != trinfo.first.end();
            ++sym)
          {
            // for each symbol, check if we can directly jump to the destination state,
            // or if we need to emit early its before (and eventually its format and encapsulation) code
          	if( to_s->first->GetInfo() == NULL && (  (*sym).second->FirstExecuteBefore)) 
 	           	toStateMap[(*sym).second->Name] = setupEarlyCode(&early_protos,(*sym).second,(*to_s).second->getLabelComplete());
            else
           	{
           		if(m_fsa->HasCode(*sym))
           		{
	           		//we have to manage the counters for the header indexing and/or the tunneled
	           		SymbolLabel *tmp_label = m_MIRCodeGen->NewLabel(LBL_CODE, "symbol_code_tmp");
	           		symbols_code_tohandle[*sym] = tmp_label;
	           		after_symbols_code[*sym] = (*to_s).second->getLabelComplete();       
	           		toStateMap[(*sym).second->Name] = tmp_label;
	           	}           		
           		else
	              	toStateMap[(*sym).second->Name] = (*to_s).second->getLabelComplete();          
            }
          }

      }
    }	//end ((*tr_iter)->getIncludingET() == NULL)
    else 
    {
      // otherwise, handle the predicate

      // sanity checks
      nbASSERT(!(*tr_iter)->IsComplementSet(), "A predicate on a complemented-set transition?");
      nbASSERT(trinfo.first.size()==1, "A predicate on a transition with more than one symbol?");
      EncapLabel_t label = *(trinfo.first.begin());
      SymbolProto *proto = label.second;

      // extract the ExtendedTransition that handles this predicate
      EncapFSA::ExtendedTransition *et = (*tr_iter)->getIncludingET();

      // see if it has been already scheduled for post-inspection...
      std::map<EncapFSA::ExtendedTransition*, SymbolLabel*>::iterator et_check = et_tohandle.find(et);
      //..if not, do it
      if (et_check == et_tohandle.end()){
      	//it is possible that, before the execution of the code of the ET, we have to execute the code related to the symbol
      	bool hasCode = m_fsa->HasCode(label);
      	if(hasCode)
      	{
      		SymbolLabel *tmp_label = m_MIRCodeGen->NewLabel(LBL_CODE, "symbol_code_tmp");
	        symbols_code_tohandle[label] = tmp_label;
	        toStateMap[(label).second->Name] = tmp_label;
      	}
      
        SymbolLabel *tmp_label = m_MIRCodeGen->NewLabel(LBL_CODE, "ET_tmp");
        et_tohandle.insert(std::pair<EncapFSA::ExtendedTransition*, SymbolLabel*>(et,tmp_label));
        toStateMap.insert(std::pair<string,SymbolLabel*>(proto->Name,tmp_label));
        
        if(hasCode)
        	after_symbols_code[label] = tmp_label;       
        
      }
      EncapStIterator_t dest = (*tr_iter)->ToState();
/*   	  if((&(*dest))->isAccepting())
      {

			endFinal.insert(pair<EncapFSA::ExtendedTransition*,bool>(et,true));
      }*/
  	}
  }
     
   // ended scanning the outgoing transitions
/*
	if(endFinal.find(et)!=endFinal.end())
	(*endFinal.find(et)).second=true;
	else      	
	endFinal.insert(pair<EncapFSA::ExtendedTransition*,bool>(et,true));
	cout<<"insert true"<<endl;
//	flag=true;
      }
      else
     {
	if(endFinal.find(et)==endFinal.end())
//	if(flag==false)
	{
	cout<<"not true -> set false"<<endl;
  	 endFinal.insert(pair<EncapFSA::ExtendedTransition*,bool>(et,false));
	}
	cout<<"really set?"<<endl;
     }


    }
	cout<<"ET&: "<<(*tr_iter)->getIncludingET();
	cout<<"end of loop"<<endl;
  }
  // ended scanning the outgoing transitions

  
	//endFinal.insert(pair<EncapFSA::ExtendedTransition*,bool>(et,flag));
*/


  // 7) parse and emit the encapsulation section, if the current state is related to a single protocol
  if(stateFrom->GetInfo()!=NULL)
  {
	CodeList *encapCode = m_GlobalSymbols.NewCodeList(true);		//Build New list
	HIRCodeGen encapCodeGen(m_GlobalSymbols, encapCode);			//m_CodeList = encapCode

	NetPDLParser.ParseEncapsulation(stateFrom->GetInfo(), toStateMap, fsa, stateFrom->isAccepting(),
		                              protoFilterInfo.FilterFalseLbl, protoFilterInfo.FilterTrueLbl, encapCodeGen, !m_FullEncap/*indicates if we are using or not the preferred graph*/);

	#ifdef ENABLE_PFLFRONTEND_DBGFILES   //[mligios] - PARSE & SAVE - ENCAPSULATION SECTION ON NETPDL
	ostringstream my_filename;

	if (m_GlobalInfo.Debugging.DumpHIRCodeFilename)
	{
		#ifdef DIVIDE_PROTO_FILES
			my_filename << m_GlobalInfo.Debugging.DumpHIRCodeFilename << "_" << stateFrom->GetInfo()->Name << ".asm";
			ofstream my_ir(my_filename.str().c_str(), ios::app );
			CodeWriter my_cw(my_ir);
			my_cw.DumpCode(encapCode); //dumps the HIR code  (encapcode) -> get by m_GlobalSymbols
			my_ir.close();
		#endif
		#ifndef DIVIDE_PROTO_FILES
		//It can be useful to keep all hir code together in one file
		ofstream my_ir2(m_GlobalInfo.Debugging.DumpHIRCodeFilename, ios::app);
		CodeWriter my_cw2(my_ir2);
		my_cw2.DumpCode(encapCode); //dumps the HIR code
		my_ir2.close();
		#endif
	}
	#endif


	for(map<string, SymbolLabel*>::iterator it = toStateMap.begin(); it != toStateMap.end(); it++)
	{
		SymbolProto *current = m_GlobalSymbols.LookUpProto((*it).first);
		if(current == NULL) 
			continue;
			
		if(current->NoAck)		//es "http noack"
		 	CheckifCodeGotLookup(encapCodeGen.m_CodeList);                              
			
		/*In practice, here we consider the copros used in the after-format-verify by the current nextproto*/
		manageRegExStringMarchCopros(current);
		/* Find the required lookup coprocessors */
		manageLookUpTables(current);				
		
	} 

		                              
	// emit the ENCAPSULATION code
	m_NetVMIRGen->LowerHIRCode(encapCode, "emitting ENCAPSULATION for proto inside " + curStateInfo->toString());
	m_MIRCodeGen->CommentStatement("DONE emitting the proto ENCAPSULATION");
   }


  // 8) emit the symbol code
  if(!symbols_code_tohandle.empty())
  {
	m_MIRCodeGen->CommentStatement("BEGIN handling symbols code");
	std::map<EncapLabel_t, SymbolLabel*>:: iterator c = symbols_code_tohandle.begin();
	std::map<EncapLabel_t, SymbolLabel*>:: iterator d = after_symbols_code.begin();
  	for (; c != symbols_code_tohandle.end();++c,++d) 
  	{
  		m_MIRCodeGen->LabelStatement(c->second); // generate the proper temporary label
  		GenerateSymbolsCode(c->first,d->second);
  	} 
  	m_MIRCodeGen->CommentStatement("DONE handling symbols code"); 
  }


  // 9) emit the ExtendedTransition code
  if (!et_tohandle.empty()) {
    m_MIRCodeGen->CommentStatement("BEGIN handling ExtendedTransitions");
    for (std::map<EncapFSA::ExtendedTransition*, SymbolLabel*>:: iterator et_iter = et_tohandle.begin();
         et_iter != et_tohandle.end();++et_iter) 
    {
      m_MIRCodeGen->LabelStatement(et_iter->second); // generate the proper temporary label
      CodeList *ETcode = m_GlobalSymbols.NewCodeList(true);
      HIRCodeGen ETcodeGen(m_GlobalSymbols, ETcode);
      EncapFSA::Alphabet_t alphEt = (*et_iter).first->GetLabels();
      nbASSERT(alphEt.size() == 1,"An exended transition cannot have more than one symbol");

     GenerateETCode(et_iter->first, statesInfo, protoFilterInfo.FilterFalseLbl, &ETcodeGen, fieldExtraction || !(*(endFinal.find((*et_iter).first))).second,*(alphEt.begin()));

      m_NetVMIRGen->LowerHIRCode(ETcode, "");
    }
    m_MIRCodeGen->CommentStatement("DONE handling ExtendedTransitions");
  }
  
  // 9) emit the early code for those states that need it
  m_MIRCodeGen->CommentStatement("");
  for(std::map<string, EarlyCodeEmission_t>::iterator i = early_protos.begin();
      i != early_protos.end();
      ++i)
  {
  	/*
  	* This situation happens for filters like "http or ftp"
  	* The automaton representing it has a single final accepting state, which is associated with both
  	* the protocols. Here we have to emit the before of http and ftp.
  	*/
  	
  	// emit the start label
    m_MIRCodeGen->LabelStatement(i->second.start_lbl);
    SymbolProto *protocol = i->second.protocol;
    m_MIRCodeGen->CommentStatement("EARLY BEFORE SECTION FOR PROTOCOL: " +protocol->Name+ " START");
    GenerateBeforeCode(protocol); 
    m_MIRCodeGen->CommentStatement("EARLY BEFORE SECTION FOR PROTOCOL: " +protocol->Name+ " END"); 
    m_MIRCodeGen->JumpStatement(JUMPW, i->second.where_to_jump_after);
  }
 
  return;
}

void NetPFLFrontEnd::manageRegExStringMarchCopros(SymbolProto *current)
{
  	{
	  	//Instantiate regexp copro
	  	RegExList_t list = this->m_GlobalSymbols.GetRegExList();
	  	RegExList_t::iterator reIt = list.begin();
	  	ProtoList_t protocols = this->m_GlobalSymbols.GetRegExProtoList();
	  	for(ProtoList_t::iterator it = protocols.begin(); it != protocols.end();)
	  	{
	  		if(*it == current)
	  		{
	  			(*reIt)->Id = m_GlobalSymbols.GetRegExEntriesCountUseful();
	  			m_GlobalSymbols.StoreRegExEntryUseful(*reIt);
				
				//update elements		  			
	  			RegExList_t::iterator auxRegEx = reIt;
	  			reIt++;
	  			list.erase(auxRegEx);
	  			ProtoList_t::iterator auxProto = it;
	  			it++;
	  			protocols.erase(auxProto);
	  			continue;
	  		}
	  		reIt++;
	  		it++;
	  	}
	  	m_GlobalSymbols.SetRegExList(list);
	  	m_GlobalSymbols.SetRegExProtoList(protocols);
	}
	{
	  	//Instantiate stringmatching copro
	  	RegExList_t list = this->m_GlobalSymbols.GetStringMatchingList();
	  	RegExList_t::iterator reIt = list.begin();
	  	ProtoList_t protocols = this->m_GlobalSymbols.GetStringMatchingProtoList();
	  	for(ProtoList_t::iterator it = protocols.begin(); it != protocols.end();)
	  	{	  	
	   		if(*it == current)
	  		{
	  			(*reIt)->Id = m_GlobalSymbols.GetStringMatchingEntriesCountUseful();
	  			m_GlobalSymbols.StoreStringMatchingEntryUseful(*reIt);
	  			
	  			//update elements
	  			RegExList_t::iterator auxRegEx = reIt;
	  			reIt++;
	  			list.erase(auxRegEx);
	  			ProtoList_t::iterator auxProto = it;
	  			it++;
	  			protocols.erase(auxProto);
	  			continue;
	  		}
	  		 it++;
	  		 reIt++;
	  	}
	  	m_GlobalSymbols.SetStringMatchingList(list);
	  	m_GlobalSymbols.SetStringMatchingProtoList(protocols);
	}
}

void NetPFLFrontEnd::manageLookUpTables(SymbolProto *current)        
{
	//Get the copros used by current
	set<string> tables =  m_GlobalSymbols.GetAssociationProtosTable(current);  //Returns the set of tables used by protocol 
	//Remove tables from the other protocols using them
	for(set<string>::iterator t = tables.begin(); t != tables.end(); t++)
	{
		set<SymbolProto*> protocols = m_GlobalSymbols.GetAssociationTableProtos(*t);
		for(set<SymbolProto*>::iterator p = protocols.begin(); p != protocols.end(); p++)
			m_GlobalSymbols.RemoveProtoTable(*p,*t);
		m_GlobalSymbols.StoreUsefulTable(*t);		//store the name of this useful table
	}
	
}

SymbolLabel* NetPFLFrontEnd::setupEarlyCode(std::map<string, EarlyCodeEmission_t>* m,
                                            SymbolProto *p, SymbolLabel *jump_after)
{
  std::map<string, EarlyCodeEmission_t>::iterator i = m->find(p->Name);
  if(i != m->end())
    return i->second.start_lbl;

  struct EarlyCodeEmission_t code;  
  code.start_lbl = m_MIRCodeGen->NewLabel(LBL_CODE, p->Name + "_earlybefore");
  code.protocol = p;
  code.where_to_jump_after = jump_after;

  (*m)[p->Name] = code;
  return code.start_lbl;
}

/*
*	Generates the code assocaited with symbols
*/
void NetPFLFrontEnd::GenerateSymbolsCode(EncapLabel_t symbol, SymbolLabel* destination)
{
	list<SymbolTemp*> variables = m_fsa->GetVariables(symbol);
	nbASSERT(variables.size()!=0,"There is a BUG! This symbol has code and has not a variable O_o");
	nbASSERT(destination!=NULL,"There is a bug! The lable cannot be NULL");
	
	//increment each variable - (*it)++
	for(list<SymbolTemp*>::iterator it = variables.begin(); it != variables.end(); it++)		
		m_MIRCodeGen->GenStatement(m_MIRCodeGen->UnOp(LOCST, m_MIRCodeGen->BinOp(ADD, m_MIRCodeGen->TermNode(LOCLD, (*it)), m_MIRCodeGen->TermNode(PUSH, 1)),(*it)));
	
	//jump to the next code to execute
	m_MIRCodeGen->JumpStatement(JUMPW,destination);
}

/*
*	Generates the code for an et
*	fieldExtraction is true if, after the parsing of the format of the destination protocol, the offset into the packet must be set before the format.
*	This reset is needed in case of the destination state of the et is not the final accepting state
*/
void NetPFLFrontEnd::GenerateETCode(EncapFSA::ExtendedTransition *et, const sInfoMap_t stateMappings,
                                    SymbolLabel *defaultFailure, HIRCodeGen *codeGenerator,
                                    bool fieldExtraction, EncapLabel_t input_symbol)
{
	
  SymbolProto *target_proto = (*(et->GetInfo().first.begin())).second;
 
  manageRegExStringMarchCopros(target_proto);   
  if(m_AutomatonWithCode)
  	 VisitET(et,codeGenerator);
  
  if(m_Lower)
  {
  	 //Since this et requires to parse the format of the protocol, we also execute the before section
  	 // generate and emit the "before" code for the protocol we're reaching		
	 GenerateBeforeCode(target_proto);
	 // generate and emit the "format" code for the protocol we're reaching
	 // FIXME: le prossime 3 variabili sembra solo per fare compilare. creare un LowerHIRCode senza ste tre cose, e che non prenda false tanto è scontato
	 list<SymbolField*> aux;
  	 list<uint32> aux2; 
     list<bool> aux3; 
	 m_NetVMIRGen->LowerHIRCode(&m_GlobalSymbols.GetProtoCode(target_proto), target_proto,
		                         false, //we do not want to emit extraction code, even if this state could be action. This is because the offset will be reset, and the state parsed again
		                         aux, aux2, aux3, "[ET] Parsing format for target protocol", fieldExtraction);  
		                           
	if(!fieldExtraction)
	{
		//We are generated the before and the format of the nextprotocol. Hence, if we are not doing field extraction, we insert it into the set
		//"NoFormat", since its format and before section should not be generated (since useless).
		list<EncapFSA::StateIterator> destinations = et->ToStates();
		for(list<EncapFSA::StateIterator>::iterator it = destinations.begin(); it != destinations.end(); it++)
			NoFormat.insert(&(*(*it)));
	}
  }
  else
 	/*
 	* This ET evaluates only the value of counters, hence we do not emit the early format.
 	*/
 	 m_MIRCodeGen->CommentStatement("[ET] only on counters");

  struct ETCodegenCB_info_t info;
  info.instance = this;
  info.dfl_fail_label = defaultFailure;
  // retrieve generic info about the substates...
  map<unsigned long, EncapFSA::State*> substates = et->getSubstatesInfo();
  // 	.. and generate their labels
  for(map<unsigned long, EncapFSA::State*>::iterator i = substates.begin();
      i != substates.end();
      ++i)
    info.substates[i->first] = codeGenerator->NewLabel(LBL_CODE, "ET_internal");

  // start the recursive walk
  info.codeGens.push(codeGenerator); 

  et->walk(ETCodegenCB_newlabel, ETCodegenCB_range, ETCodegenCB_punct, ETCodegenCB_jump,ETCodegenCB_special, &info); //walk is sft.hpp

  // complete the work by generating jump code from the output gates
  for(map<unsigned long, EncapFSA::State*>::iterator i = substates.begin();
      i != substates.end();
      ++i)
    if(i->second) {
      // emit the label
      map<unsigned long,SymbolLabel *>::iterator l_iter = info.substates.find(i->first);
      nbASSERT(l_iter != info.substates.end(), "I should have a created a label before");
      codeGenerator->LabelStatement(l_iter->second);

      // jump to the outer-level state
      sInfoMap_t::const_iterator infoPtr = stateMappings.find(i->second);

      if (infoPtr == stateMappings.end()) // This gate points to a State that does not lead to an accepting state
        codeGenerator->JumpStatement(defaultFailure);
      else
      {
     	if(!fieldExtraction && m_Lower)
	{
	       codeGenerator->JumpStatement(infoPtr->second->getLabelFast()); /* here I use the "fast" label because
                                                                          * I will emit 'before' and 'format' directly
                                                                          * inside the ET
                                                                                */	
	}
	else
	{
       		codeGenerator->JumpStatement(infoPtr->second->getLabelComplete());
	}
      }
    }
}

/*
* Given an ET, this method visit (but not generate code) the ET itself, in order to understand if it requires the lowering of the 
* format of the destination protocol, or not. Note that the lowering of the format is not required on an ET in case it only evaluates
* the value of a counter. On the other hand, an ET that must fire if a specific field assumes a certain value requires the lowering of 
* the format.
*/
void NetPFLFrontEnd::VisitET(EncapFSA::ExtendedTransition *et, HIRCodeGen *codeGenerator)
{
	m_Lower = false;
	struct ETCodegenCB_info_t infoAux;
	infoAux.instance = this;

	infoAux.codeGens.push(codeGenerator);

	m_WalkForVisit = true; 
	// start the recursive visit
	et->walk(ETCodegenCB_newlabel, ETCodegenCB_range, ETCodegenCB_punct, ETCodegenCB_jump,ETCodegenCB_special, &infoAux); //walk is sft.hpp
	m_WalkForVisit = false;
}

bool NetPFLFrontEnd::GenerateBeforeCode(SymbolProto *sp) 
{
  nbASSERT(sp, "protocol cannot be NULL inside GenerateBeforeCode");
  
  _nbNetPDLElementProto *element=sp->Info;
  if (sp->FirstExecuteBefore)
  {
    CodeList *beforeCode = m_GlobalSymbols.NewCodeList(true);
    HIRCodeGen beforeCodeGen(m_GlobalSymbols, beforeCode);
    NetPDLParser.ParseBeforeSection(element->FirstExecuteBefore,beforeCodeGen,sp);
 
	#ifdef ENABLE_PFLFRONTEND_DBGFILES   //[mligios] - PARSE & SAVE - (complete hir code for goal states) 

	ostringstream my_filename;
	if (m_GlobalInfo.Debugging.DumpHIRCodeFilename)
	{
		#ifdef DIVIDE_PROTO_FILES
			my_filename.str("");
			my_filename << m_GlobalInfo.Debugging.DumpHIRCodeFilename << "_" << sp->Name << ".asm";
			ofstream my_ir(my_filename.str().c_str(), ios::app );
			CodeWriter my_cw(my_ir);
			my_cw.DumpCode(beforeCode); //dumps the HIR code
			my_ir.close();
		#endif
		#ifndef DIVIDE_PROTO_FILES
		//It can be useful to keep all hir code together in one file
		ofstream my_ir2(m_GlobalInfo.Debugging.DumpHIRCodeFilename, ios::app);
		CodeWriter my_cw2(my_ir2);
		my_cw2.DumpCode(beforeCode); //dumps the HIR code
		my_ir2.close();
		#endif

	}
	#endif

  
    /* 
    * Move all the regexp and string matching copros used by the current protocol
    * In practice, here we consider the copros used in the before by the current proto
    */  
	manageRegExStringMarchCopros(sp);
    
    // emit the before code
    m_NetVMIRGen->LowerHIRCode(beforeCode, "emitting before code for protocol " + sp->Name);    //THIS.PROTOCOL IN MIR - HERE START <BEFORE> Section 
    m_MIRCodeGen->CommentStatement("DONE emitting before code");				//THIS.PROTOCOL IN MIR - HERE END   <BEFORE> Section 

    return true;
  }

  return false;
}

/*
*	The next code emits the extended transitions
*/

HIROpcodes NetPFLFrontEnd::rangeOpToIROp(RangeOperator_t op)
{
	switch(op)
	{
		case LESS_THAN: return IR_LTI;
		case LESS_EQUAL_THAN: return IR_LEI;
		case GREAT_THAN: return IR_GTI;
		case GREAT_EQUAL_THAN: return IR_GEI;
		case EQUAL: return IR_EQI;
		case INVALID:
		default:
			nbASSERT(false, "Congratulations, you have found a bug");
			// Return the invalid opcode; it doesn't matter (the program stops at the previous
			// instruction) but it avoids a warning in the C compiler ('not all control paths return a value)
			return HIR_LAST_OP;
	}
}


void NetPFLFrontEnd::ETCodegenCB_newlabel(unsigned long id, string proto_field_name, void *ptr)
{
  struct ETCodegenCB_info_t *info = (struct ETCodegenCB_info_t*) ptr;

  // safety check
  nbASSERT(info->codeGens.size() == 1, "There should be only one codegen on the stack");
 
  // save the field symbol that will be used until the next invocation of this method
  size_t dot_pos = proto_field_name.find('.');
  nbASSERT(dot_pos!=string::npos, "I should find a dot in the proto-field name!");
  string proto_name = proto_field_name.substr(0, dot_pos);
  string field_name = proto_field_name.substr(dot_pos+1);
    
  //extract the eventual third part. This part there is only in case of tunneled
  dot_pos = field_name.rfind('.');
  string third_str = field_name.substr(dot_pos+1);
   
  if(field_name == string("HeaderCounter"))
  {
  	//et on a variable related to the symbols - we want to check: variable==n
	SymbolProto *proto = info->instance->m_GlobalSymbols.LookUpProto(proto_name);
	EncapLabel_t label = std::make_pair<SymbolProto*, SymbolProto*>(NULL, proto);//label representing all the symbols leading to proto
	SymbolTemp *st = info->instance->m_fsa->GetVariableToCheck1(label); //get the variable to test
	nbASSERT(st!=NULL,"This symbol cannot be NULL. There is a bug!");
	nbASSERT(st->SymKind==SYM_TEMP,"There is a bug! This symbol should be SYM_TEMP");
  	info->cur_sym = st;
  }
  else if(third_str == string("LevelCounter"))
  {
  	//et on a variable related to the symbols - we want to check: variable>n
  	stringstream ss;
  	ss << proto_field_name << "++";
  	SymbolTemp *st = info->instance->m_fsa->GetVariableFromCode2(ss.str()); //get the variable to test
  	nbASSERT(st!=NULL,"This symbol cannot be NULL. There is a bug!");
	nbASSERT(st->SymKind==SYM_TEMP,"There is a bug! This symbol should be SYM_TEMP");
  	info->cur_sym = st;
  }
  else
  	//et on a protocol field
  	info->cur_sym = info->instance->m_GlobalSymbols.LookUpProtoFieldByName(proto_name,field_name);

  // Duplicate the codegen at the top of the stack.
  // It will be used for the current substate.
  HIRCodeGen *codegen = static_cast<HIRCodeGen*>(info->codeGens.top());
  info->codeGens.push(codegen);

  if(!info->instance->m_WalkForVisit)
  {
  	/*
  	* The label is emitted only if we are not walking for visit (i.e., for understanding if the ET contains at least a 
  	* predicate on a protocol field), but only of we are actually generating the code implementing the ET
  	*/
	map<unsigned long,SymbolLabel *>::const_iterator m = info->substates.find(id);
  	nbASSERT(m != info->substates.end(), "I should have a created a label before");
  	codegen->LabelStatement(m->second);
  }
}


void NetPFLFrontEnd::ETCodegenCB_range(RangeOperator_t r, uint32 sep, void *ptr)
{
//	cout << "RANGE" << endl;

  struct ETCodegenCB_info_t *info = (struct ETCodegenCB_info_t*) ptr;

  // use the codegen at the top of the stack, then pop it
  HIRCodeGen *codegen = static_cast<HIRCodeGen*>(info->codeGens.top());
  info->codeGens.pop();
  
  Symbol *symbol = info->cur_sym;
  StmtIf *ifStmt = NULL;
  
  if(symbol->SymKind==SYM_FIELD)
  {
  	//et on a protocol field
  	if(info->instance->m_WalkForVisit)
  	{
	  	/*
  		* We are not walking for visit (i.e., for understanding if the ET contains at least a 
  		* predicate on a protocol field)
  		*/
  		info->instance->m_Lower = true; //The transition should we lowered
  		HIRCodeGen *branchCodeGen = NULL;
		info->codeGens.push(branchCodeGen);
  		info->codeGens.push(branchCodeGen);
  		return;
    }
  	ifStmt = codegen->IfStatement(codegen->BinOp(rangeOpToIROp(r),codegen->UnOp(IR_CINT,codegen->TermNode(IR_FIELD, static_cast<SymbolField*>(symbol))),codegen->TermNode(IR_ICONST, codegen->ConstIntSymbol(sep))));
   }
  else if (symbol->SymKind==SYM_TEMP)
  {
  	//et on a variable
    if(info->instance->m_WalkForVisit)
  	{
  		/*
  		* We are not walking for visit (i.e., for understanding if the ET contains at least a 
  		* predicate on a protocol field)
  		*/
  		HIRCodeGen *branchCodeGen = NULL;
		info->codeGens.push(branchCodeGen);
  		info->codeGens.push(branchCodeGen);
		return;
  	}
	ifStmt = codegen->IfStatement(codegen->BinOp(IR_GEI,codegen->UnOp(IR_IVAR,symbol),codegen->TermNode(IR_ICONST,codegen->ConstIntSymbol(2))));
   }
  else
	nbASSERT(false,"There is a bug!");

  // generate the true and false branches, push them into the stack (watch for the order!)
  HIRCodeGen *branchCodeGen = new HIRCodeGen(info->instance->m_GlobalSymbols, ifStmt->TrueBlock->Code);
  HIRCodeGen *falseCodeGen = new HIRCodeGen(info->instance->m_GlobalSymbols, ifStmt->FalseBlock->Code);

  info->codeGens.push(falseCodeGen);
  info->codeGens.push(branchCodeGen);
}

void NetPFLFrontEnd::ETCodegenCB_punct(RangeOperator_t r, const map<uint32,unsigned long> &mappings, void *ptr)
{
	struct ETCodegenCB_info_t *info = (struct ETCodegenCB_info_t*) ptr;

	// use the codegen at the top of the stack, then pop it
	HIRCodeGen *codegen = static_cast<HIRCodeGen*>(info->codeGens.top());
	info->codeGens.pop();

  	HIRStmtSwitch *swStmt = NULL;
	
	Symbol *symbol = info->cur_sym;

	nbASSERT(symbol!=NULL,"There is a BUG!");
	
	if(symbol->SymKind==SYM_FIELD)
	{
		//et on a protocol field
		if(info->instance->m_WalkForVisit)
	  	{
	  		/*
	  		* We are not walking for visit (i.e., for understanding if the ET contains at least a 
	  		* predicate on a protocol field)
	  		*/
	  		info->instance->m_Lower = true; //The transition should be lowered
	  		HIRCodeGen *dflCaseCodeGen = NULL;
			info->codeGens.push(dflCaseCodeGen);
	  		return;
	  	}
	  	swStmt = codegen->SwitchStatement(codegen->UnOp(IR_CINT,codegen->TermNode(IR_FIELD, static_cast<SymbolField*>(symbol))));
	}
	else if (symbol->SymKind==SYM_TEMP)
	{
		//et on a variable
		if(info->instance->m_WalkForVisit)
	  	{
	  		/*
	  		* We are not walking for visit (i.e., for understanding if the ET contains at least a 
	  		* predicate on a protocol field)
	  		*/
	  		HIRCodeGen *dflCaseCodeGen = NULL;
			info->codeGens.push(dflCaseCodeGen);
			return;
	  	}
		swStmt = codegen->SwitchStatement(codegen->UnOp(IR_IVAR,symbol));
	}
	else
		nbASSERT(false,"There is a bug!");

	swStmt->SwExit = codegen->NewLabel(LBL_CODE, "unused");

  // generate the code for the cases
  for(map<uint32,unsigned long>::const_iterator i = mappings.begin();
      i != mappings.end();
      ++i) {
    // prepare the jump for the current case ...
    HIRNode *value = codegen->TermNode(IR_ICONST, codegen->ConstIntSymbol(i->first));
    StmtCase *caseStmt = codegen->CaseStatement(swStmt, value);
    HIRCodeGen subCode(info->instance->m_GlobalSymbols, caseStmt->Code->Code);

    // ... and handle it
    map<unsigned long,SymbolLabel *>::const_iterator m = info->substates.find(i->second);
    nbASSERT(m != info->substates.end(), "Callback for a substate that I did not know of!");
    subCode.JumpStatement(m->second); 

    swStmt->NumCases++;
  }

  // ensure correctness and avoid runtime failures
  codegen->JumpStatement(info->dfl_fail_label); 
  codegen->CommentStatement("The label and jump just above me should never be reached");

  // handle the default case by creating a new IRCodeGen and pushing it onto the stack
  StmtCase *defaultCaseStmt = codegen->DefaultStatement(swStmt);
  HIRCodeGen *dflCaseCodeGen = new HIRCodeGen(info->instance->m_GlobalSymbols, defaultCaseStmt->Code->Code);
  info->codeGens.push(dflCaseCodeGen);
}


void NetPFLFrontEnd::ETCodegenCB_jump(unsigned long id, void *ptr)
{
  struct ETCodegenCB_info_t *info = (struct ETCodegenCB_info_t*) ptr;
  
  if(info->instance->m_WalkForVisit)
  {
  	/*
	* We are not walking for visit (i.e., for understanding if the ET contains at least a 
	* predicate on a protocol field)
	*/
  	info->codeGens.pop();
  	return;
  }

  // use the codegen at the top of the stack, then pop it
  HIRCodeGen *codegen = static_cast<HIRCodeGen*>(info->codeGens.top());
  info->codeGens.pop();

  // jump!
  map<unsigned long,SymbolLabel *>::const_iterator m = info->substates.find(id);
  nbASSERT(m != info->substates.end(), "Callback for a substate that I did not know of!");
  codegen->JumpStatement( m->second); 
}


FieldsList_t NetPFLFrontEnd::GetExField(void)
{
  return m_FieldsList;
}


SymbolField* NetPFLFrontEnd::GetFieldbyId(const string protoName, int index)
{
	return m_GlobalSymbols.LookUpProtoFieldById(protoName,(uint32)index);
}

void NetPFLFrontEnd::ETCodegenCB_special(RangeOperator_t r, std::string val, void *ptr)
{
	struct ETCodegenCB_info_t *info = (struct ETCodegenCB_info_t*) ptr;
	  
	if(info->instance->m_WalkForVisit)
	{		
		/*
  		* We are not walking for visit (i.e., for understanding if the ET contains at least a 
  		* predicate on a protocol field)
  		*/
		info->instance->m_Lower = true; // The transition should be lowered
		info->codeGens.pop();
		HIRCodeGen *branchCodeGen = NULL;
		info->codeGens.push(branchCodeGen);
		info->codeGens.push(branchCodeGen);
		return;
	}

 	// use the codegen at the top of the stack
 	HIRCodeGen *codegen = static_cast<HIRCodeGen*>(info->codeGens.top());
 	info->codeGens.pop();
 	
 	Symbol *symbol = info->cur_sym;
  	nbASSERT(symbol->SymKind==SYM_FIELD,"There is an error! A special is supported only on et related to fields");

 	SymbolField *sf = static_cast<SymbolField*>(symbol);
 	
 	PDLFieldType fieldType = sf->FieldType;
 	 	
 	if((r==MATCH)||(r==CONTAINS)){//we have to match a regular expression
 	
	 	SymbolStrConst *patternSym = (SymbolStrConst *)codegen->ConstStrSymbol(val);
	 	int id=(r==MATCH)? info->instance->m_GlobalSymbols.GetRegExEntriesCountUseful() : info->instance->m_GlobalSymbols.GetStringMatchingEntriesCountUseful();
 		SymbolTemp *size(0);
 		SymbolTemp *offset(0);
 		bool sensitive;
 		
 		switch(fieldType){
			case PDL_FIELD_VARLEN:
			{
	 			SymbolFieldVarLen *varlenFieldSym = static_cast<SymbolFieldVarLen*>(sf);
 				size  = varlenFieldSym->LenTemp;
 				offset= varlenFieldSym->IndexTemp;	 			
 				sensitive = varlenFieldSym->Sensitive;
			}
			break;
			case PDL_FIELD_TOKEND:
			{
 				SymbolFieldTokEnd *tokEndFieldSym = static_cast<SymbolFieldTokEnd*>(sf);
 				size  = tokEndFieldSym->LenTemp;
 				offset= tokEndFieldSym->IndexTemp;	
 				sensitive = tokEndFieldSym->Sensitive; 			
 			}
			break;
            case PDL_FIELD_LINE:
 			{
				SymbolFieldLine *lineFieldSym = static_cast<SymbolFieldLine*>(sf);
	 			size  = lineFieldSym->LenTemp;
 				offset= lineFieldSym->IndexTemp;	
 				sensitive = lineFieldSym->Sensitive; 			
	 		}	
			break;
			case PDL_FIELD_PATTERN:
			{
	 			SymbolFieldPattern *patternFieldSym = static_cast<SymbolFieldPattern*>(sf);
	 			size  = patternFieldSym->LenTemp;
	 			offset= patternFieldSym->IndexTemp;	
	 			sensitive = patternFieldSym->Sensitive; 			
	 		}		
			break;
			case PDL_FIELD_EATALL:
			{
	 			SymbolFieldEatAll *eatAllFieldSym = static_cast<SymbolFieldEatAll*>(sf);
 				size  = eatAllFieldSym->LenTemp;
 				offset= eatAllFieldSym->IndexTemp;	
 				sensitive = eatAllFieldSym->Sensitive; 			
 			}	
			break;
	 		case PDL_FIELD_FIXED:
			{		
	 			SymbolFieldFixed *fixedFieldSym = static_cast<SymbolFieldFixed*>(sf);	
 				offset = fixedFieldSym->IndexTemp;
  				size  = fixedFieldSym->LenTemp;
  				sensitive = fixedFieldSym->Sensitive;
			}
			break;
 			default:
				nbASSERT(false,"Only fixed, variable, tokenended, line, pattern and eatall fields are supported with the NetPFL keywords \"matches\" and \"contains\"");
 		}
	 	
	 	nbASSERT(offset != NULL, "The offset of a symbol cannot be NULL! There is a BUG somewhere!");
	 	SymbolRegExPFL *regExpSym = new SymbolRegExPFL(patternSym,sensitive,id,offset,size); 
	 		
	 	HIRNode *exp;
	 	if(r==MATCH)
	 	{
		 	exp = codegen->TermNode(IR_REGEXFND, regExpSym);
		 	info->instance->m_GlobalSymbols.StoreRegExEntryUseful(regExpSym);
		 	}
		else 
		{
			//we are sure that r==CONTAINS
		 	exp = codegen->TermNode(IR_STRINGMATCHINGFND, regExpSym);
		 	info->instance->m_GlobalSymbols.StoreStringMatchingEntryUseful(regExpSym);
		 	}
	 	
	 	
	 	StmtIf *ifStmt = codegen->IfStatement(exp);

		 // generate the true and false branches, push them into the stack (watch for the order!)
		 HIRCodeGen *branchCodeGen = new HIRCodeGen(info->instance->m_GlobalSymbols, ifStmt->TrueBlock->Code);
		 HIRCodeGen *falseCodeGen = new HIRCodeGen(info->instance->m_GlobalSymbols, ifStmt->FalseBlock->Code);

		 info->codeGens.push(falseCodeGen);
		 info->codeGens.push(branchCodeGen);
		 
	}  
 	else
 		nbASSERT(false,"You cannot be here!");
  
}
