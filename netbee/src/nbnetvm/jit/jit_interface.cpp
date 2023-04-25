/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \page netvmjit
 *
 * \section netvmjit NetVMjit overview
 *
 * <p> To write an overview of the jit compilation process.
 * </p>
 * \section toread More details
 *
 * <p>See \ref todo and \ref inssel.
 */

/*!
 * \file genericfrontend.cpp
 * \brief this file contains the function to be called by the NetVM framework to compile the netil code
 */

#include "nbnetvm.h"

#include "helpers.h"
#include "rt_environment.h"
#include "int_structs.h"

#include "jit_interface.h"
#include "jit_internals.h"

#include "codetable.h"
#include "bytecode_segments.h"
#include "bytecode_analyse.h"
#include "../../nbee/globals/debug.h"

#include "digraph.h"
#include "cfg.h"
#include "cfg_bytecode_builder.h"
#include "cfg_printer.h"
#include "cfg_liveness.h"
#include "irnode.h"
#include "mirnode.h"
#include "iltranslator.h"
#include "application.h"
#include "opt/deadcode_elimination_2.h"
#include "opt/optimizer_statistics.h"

#include "registers.h"
#include "register_mapping.h"

#ifdef WIN32
#define snprintf _snprintf
#define strdup _strdup
#endif

#ifdef ENABLE_X86_BACKEND
  #include "x86-backend.h"
#endif
#ifdef ENABLE_X64_BACKEND
  #include "x64/x64-backend.h"
#endif
#ifdef ENABLE_X11_BACKEND
  #include "x11/x11-backend.h"
#endif
#ifdef ENABLE_OCTEON_BACKEND
  #include "octeon-backend.h"
#endif
#ifdef ENABLE_OCTEONC_BACKEND
  #include "octeonc-backend.h"
#endif

#define TARGETS_NUM ((sizeof(targets_func) / sizeof(TargetInterfaceFunc *)) - 1)

#include <fstream>
#include <sstream>
#include <memory>

using namespace jit;
using namespace std;

static void nvmNet_JitFill_Segments_Info(nvmByteCodeSegmentsInfo *segmentsInfo, nvmHandlerState *HandlerState);

static int32_t Perform_Linkage(CFG<MIRNode> &cfg);
static int32_t Delete_edges_from_entry(CFG<MIRNode>& cfg, DiGraph<nvmNetPE*>* pe_graph);

static TargetInterfaceFunc *targets_func[] =
{
#ifdef ENABLE_X86_BACKEND
  x86_getTargetDriver,
#endif
#ifdef ENABLE_X64_BACKEND
  x64_getTargetDriver,
#endif
#ifdef ENABLE_X11_BACKEND
  x11_getTargetDriver,
#endif
#ifdef ENABLE_OCTEON_BACKEND
  octeon_getTargetDriver,
#endif
#ifdef ENABLE_OCTEONC_BACKEND
  octeonc_getTargetDriver,
#endif
   NULL
};

//!create a graph of the PE of this netvm instance
DiGraph<nvmNetPE *>* createPEGraph(nvmNetVM *NetVM,	nvmRuntimeEnvironment* RTObj);

//!build a cfg from a handler and bytecode segment
void build_cfg(CFG<MIRNode>& cfg, nvmPEHandler* handler, nvmByteCodeSegment* segment, uint16_t* BBCount, TargetOptions* options);


TargetDriver::TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
:
	netvm(netvm),
	options(options),
	RTObj(RTObj)
{
}

/*!
 * \brief this function pre-compile PEs
 *
 */
DiGraph<nvmNetPE*>* TargetDriver::precompile(DiGraph<nvmNetPE*>* pe_graph)
{
//do nothing
	return pe_graph;
}
/*!
 * \brief this function creates a Digraph of PEs
 *
 */
DiGraph<nvmNetPE*>* createPEGraph(nvmNetVM *netvm, nvmRuntimeEnvironment* RTObj)
{
	typedef DiGraph<nvmNetPE*> graph_t;
	typedef graph_t::GraphNode graph_node_t;
	typedef map<nvmNetPE* , graph_node_t*> map_t;
	typedef map_t::iterator map_it_t;

	SLLstElement *iter;
	iter = netvm->NetPEs->Head;
	graph_t *graph = new graph_t();

	//fill the graph with all nodes
	//int counter = 0;

	map<nvmNetPE* , graph_node_t*> nodes_map;
#ifdef _DEBUG_CFG_BUILD
	string filename("PE_Graph.dot");
	ofstream outf(filename.c_str());
	outf << "digraph G {" << endl;
#endif
	int counter = 0;
	while(iter != NULL)
	{
		nvmNetPE *currentPE = (nvmNetPE*)iter->Item;
		graph_node_t *current_node;

		if(currentPE->Name[0] == '\0')
//			snprintf(currentPE->Name, MAX_NETPE_NAME, "NetPE%d", counter);
			snprintf(currentPE->Name, MAX_NETPE_NAME, "netpe%d", counter);

		map_it_t ins = nodes_map.find(currentPE);
		if(ins != nodes_map.end())
			current_node = (*ins).second;
		else
		{
			current_node = &graph->AddNode(currentPE);
			nodes_map[currentPE] = current_node;
		}
#ifdef _DEBUG_CFG_BUILD
		outf << currentPE->Name << " [ label= \"" << currentPE->Name << "\"]" << endl;
#endif

		for(uint32_t i = 0; i < currentPE->NPorts; i++)
		{
			nvmPEPort pe_port = currentPE->PortTable[i];
			if(PORT_IS_CONN_PE(pe_port.PortFlags) && PORT_IS_COLLECTOR(pe_port.PortFlags))
			{
				nvmNetPE* connected = pe_port.CtdPE;
				graph_node_t *newnode;
				map_it_t it = nodes_map.find(connected);
				if( it == nodes_map.end() ) // element is not present
				{
					newnode = &graph->AddNode(connected);
					nodes_map[connected] = newnode;
				}
				else
				{
					newnode = (*it).second;
				}
				graph->AddEdge(*current_node, *newnode);
#ifdef _DEBUG_CFG_BUILD
				cout << "Aggiunto un arco tra " << currentPE->Name << " e " << connected->Name << endl;
				outf << currentPE->Name << " -> " << connected->Name << endl;
#endif
			}
		}
		iter = (SLLstElement*)iter->Next;
		counter ++;
	}
#ifdef _DEBUG_CFG_BUILD
	outf << "}" << endl;
	outf.close();
#endif
	assert(graph->GetNodeCount() == netvm->NetPEs->NumElems);

	return graph;
}


int32_t TargetDriver::check_options(uint32_t index)
{
uint32_t BackendID;
nvmBackendDescriptor *BackendList;

	BackendList= nvmGetBackendList(&BackendID);

	uint32_t valid_flags = BackendList[index].Flags;
	uint32_t current_flags = options->Flags;

	if( (current_flags & valid_flags) != current_flags)
	{
		return nvmFAILURE;
	}

	return nvmSUCCESS;
}


int32_t nvmVerifyApplication(nvmNetVM* NetVM,
	nvmRuntimeEnvironment* RTObj,
	uint32_t JitFlags,
	uint32_t OptLevel,
	const char *OutputFilePrefix,
	char *Errbuf)
{

	std::stringstream targetCodeStream;
	TargetOptions options(OptLevel, OutputFilePrefix, JitFlags, targetCodeStream);

	if (NetVM == NULL)
	{
		cerr << "NetVM cannot be NULL" << endl;
		return nvmFAILURE;
	}

	if (RTObj == NULL)
	{
		cerr << "RTObj cannot be NULL" << endl;
		return nvmFAILURE;
	}

	nvmInitOpcodeTable();

	//Application::setCurrentNetVM(NetVM);
	//Application::setCurrentRuntime(RTObj);

	auto_ptr< DiGraph<nvmNetPE*> > pe_graph(createPEGraph(NetVM, RTObj));

	//DiGraph<nvmNetPE*>* pe_graph = pe_graph2.get();

	pe_graph->SortPostorder(**pe_graph->FirstNode());
	DiGraph<nvmNetPE *>::SortedIterator n = pe_graph->FirstNodeSorted();

	for(; n != pe_graph->LastNodeSorted(); n++)
	{
		//!\todo handle pull segments
		nvmByteCodeSegmentsInfo segmentInfo;
		nvmNetPE* pe = (*n)->NodeInfo;

		//compile push_segment
		if (strlen(pe->Name) > 0)
#ifdef _DEBUG
			VerbOut(RTObj, 0, "Verifying NetPE: %s\n", pe->Name);
#endif
		{
			nvmHandlerState* push = pe->PushHandler->HandlerState;
			nvmNet_JitFill_Segments_Info(&segmentInfo, push);
			RegisterModel::reset();

			CFG<MIRNode> cfg(segmentInfo.PushSegment.Name);
			uint16_t BBCount(0);

			try
			{
				build_cfg(cfg, push->Handler, &segmentInfo.PushSegment, &BBCount, &options);
			}
			catch (string msg)
			{
				errsnprintf(Errbuf, nvmERRBUF_SIZE, "%s", msg.c_str());
				return nvmFAILURE;
			}
		}

	}

	return nvmSUCCESS;
}


int32_t nvmNetCompileApplication(
	nvmNetVM *NetVM,
	nvmRuntimeEnvironment* RTObj,
	uint32_t BackendID,
	uint32_t JitFlags,
	uint32_t OptLevel,
	const char *OutputFilePrefix,
	char *Errbuf)
{

	std::stringstream targetCodeStream;
	TargetOptions options(OptLevel, OutputFilePrefix, JitFlags, targetCodeStream);

	if (BackendID >= TARGETS_NUM)
	{
		cerr << "Invalid backend ID " << BackendID << endl;
		return nvmFAILURE;
	}

	if (NetVM == NULL)
	{
		cerr << "NetVM cannot be NULL" << endl;
		return nvmFAILURE;
	}

	if (RTObj == NULL)
	{
		cerr << "RTObj cannot be NULL" << endl;
		return nvmFAILURE;
	}



	auto_ptr<TargetDriver> driver (targets_func[BackendID](NetVM, RTObj, &options));
	if(driver->check_options(BackendID) != nvmSUCCESS)
	{
		cerr << "Wrong flags for the backend" << endl;
		return nvmFAILURE;
	}

	try {
		driver->compile();
	}
	catch (string msg)
	{
		errsnprintf(Errbuf, nvmERRBUF_SIZE, "%s", msg.c_str());
		return nvmFAILURE;
	}

	VerbOut(RTObj, 0, "compilations succeeded\n");

	if (OutputFilePrefix == NULL)
	{
		if (RTObj->TargetCode)
			free(RTObj->TargetCode);

		std::string code = targetCodeStream.str();
		RTObj->TargetCode = (char*)calloc(code.size() + 1, sizeof(char));
		strncpy(RTObj->TargetCode, code.c_str(), code.size() + 1);
	}

	return nvmSUCCESS;
}

TargetDriver::~TargetDriver()
{
	clear_algorithm_list();
}

void TargetDriver::compile()
{
	nvmInitOpcodeTable();
	Application::setCurrentNetVM(netvm);
	Application::setCurrentRuntime(RTObj);

	auto_ptr< DiGraph<nvmNetPE*> > pe_graph(createPEGraph(netvm, RTObj));

	precompile(pe_graph.get());

	if(nvmFLAG_ISSET(options->Flags, nvmDO_INLINE))
	{
		compileInline(pe_graph.get());
	}
	else
	{
		compilePEs(pe_graph.get());
	}
}

void TargetDriver::connectFunctionToHandler(uint8_t* functPushBuffer, nvmHandlerState* HandlerState)
{
	uint32_t k, n;
	uint32_t flags_port;
    nvmNetPE *CtdPE;
	nvmSocket *Socket;

	assert(functPushBuffer != NULL && HandlerState != NULL && HandlerState->Handler->HandlerType == PUSH_HANDLER);

	for (k=0; k < HandlerState->PEState->Nports; k++)
	{
		if (PORT_IS_CONN_PE(HandlerState->Handler->OwnerPE->PortTable[k].PortFlags))
		{
			CtdPE = HandlerState->Handler->OwnerPE->PortTable[k].CtdPE;
			for (n = 0; n < CtdPE->NPorts; n++)
			{
				if(CtdPE->PortTable[n].CtdPE == HandlerState->Handler->OwnerPE)
				{
					flags_port =CtdPE->PortTable[n].PortFlags;
					if(PORT_IS_COLLECTOR(flags_port) && PORT_IS_DIR_PUSH(flags_port))
						CtdPE->PEState->ConnTable[n].CtdHandlerFunct = (nvmHandlerFunction *)functPushBuffer;
				}

			}

		}
		else if (PORT_IS_CONN_SOCK(HandlerState->Handler->OwnerPE->PortTable[k].PortFlags))
		{
			Socket = HandlerState->Handler->OwnerPE->PortTable[k].CtdSocket;
			if(Socket->InterfaceType == APPINTERFACE)
			{

				flags_port= Socket->CtdPE->PortTable[k].PortFlags;
				if( PORT_IS_EXPORTER(flags_port) && PORT_IS_DIR_PUSH(flags_port) && Socket->AppInterface->CtdHandlerType == HANDLER_TYPE_INTERF_IN )
				{
					HandlerState->PEState->ConnTable[k].CtdHandlerFunct = (nvmHandlerFunction *)functPushBuffer;
					Socket->AppInterface->Handler.CtdHandlerFunct = (nvmHandlerFunction *) functPushBuffer;
				}
			}
			else if(Socket->InterfaceType == PHYSINTERFACE)
			{
				if(Socket->PhysInterface->PhysInterfInfo->InterfDir == INTERFACE_DIR_IN)
				{
					printf("setta la funzione porta:%d\n",k);
					HandlerState->PEState->ConnTable[k].CtdHandlerFunct = (nvmHandlerFunction *)functPushBuffer;
					Socket->PhysInterface->Handler.CtdHandlerFunct = (nvmHandlerFunction *)functPushBuffer;
				}
			}
		}

	}
}

char errors[2048];


void build_cfg(CFG<MIRNode>& cfg, nvmPEHandler* handler, nvmByteCodeSegment* segment, uint16_t* BBCount, TargetOptions* options)
{
	nvmJitByteCodeInfo *BCInfo;
	ErrorList *analysisErrList;
	std::stringstream errStream;

	Application::setCurrentPEHandler(handler);
	Application::setCurrentSegment(segment);

	errStream << "In ";

	if (segment->PEFileName)
	{
		errStream << "file " << string(segment->PEFileName) << ", ";
	}
	if (segment->PEName)
	{
		errStream << "PE " << string(segment->PEName) << ": " << std::endl;
	}

	errStream << "Bytecode verification error - ";

	analysisErrList = nvmJitNew_Error_List();
	if (analysisErrList == NULL)
	{
		throw std::string("Error creating NetVM JIT analysis error list");
	}

	BCInfo = nvmJitAnalyse_ByteCode_Segment_Ex(segment, analysisErrList, BBCount);

	if (BCInfo == NULL)
	{

		nvmJitDump_Analysis_Errors(errors, 2048, analysisErrList);
		nvmJitFree_Error_List(analysisErrList);
		errStream << string(errors) << std::endl;
		throw errStream.str();
	}

	//fill cfg
	ByteCode2CFG<MIRNode> builder(cfg, BCInfo, handler);
	builder.buildCFG();
	ILTranslator::translate(cfg, BCInfo, handler->start_bb, options);

	Liveness<CFG<MIRNode> > liveness(cfg);
	liveness.run();
	std::set< RegisterInstance > liveIn = liveness.get_LiveInSet(cfg.getEntryBB());
	if (!liveIn.empty())
	{
		//we have some undefined variable
		errStream << "undefined local variables: ";
		std::set< MIRNode::RegType >::iterator i = liveIn.begin();
		for (; i != liveIn.end(); i++)
		{
			errStream << "local " << i->get_model()->get_name() << "; ";
		}
		errStream << std::endl;

		throw errStream.str();
	}
	
	nvmJitFree_Error_List(analysisErrList);

#ifdef _DEBUG_CFG_BUILD
	{
		cout << "Dopo la traduzione " << cfg.getName() << endl;
		CodePrint<MIRNode, NonSSAPrinter<MIRNode> > codeprinter(cfg);
		cout << codeprinter;
	}
#endif

	free(BCInfo->BCInfoArray);
	free(BCInfo);
}

void TargetDriver::compilePEs(DiGraph<nvmNetPE*>* pe_graph)
{
	pe_graph->SortPostorder(**pe_graph->FirstNode());
	DiGraph<nvmNetPE *>::SortedIterator n = pe_graph->FirstNodeSorted();

	for(; n != pe_graph->LastNodeSorted(); n++)
	{
		//!\todo handle pull segments
		nvmByteCodeSegmentsInfo segmentInfo;
		nvmNetPE* pe = (*n)->NodeInfo;

		// Compile push_segment
#ifdef _DEBUG
		VerbOut(RTObj, 0, "Compiling");
		if (strlen(pe->Name) > 0)
			VerbOut(RTObj, 0, " NetPE %s", pe->Name);
		if (strlen(pe->FileName) > 0)
			VerbOut(RTObj, 0, " from file: %s", pe->FileName);
		VerbOut(RTObj, 0, "...\n");
#endif
		{
			nvmHandlerState* push = pe->PushHandler->HandlerState;
			nvmNet_JitFill_Segments_Info(&segmentInfo, push);
			RegisterModel::reset();

			CFG<MIRNode> cfg(segmentInfo.PushSegment.Name);
			uint16_t BBCount(0);
			build_cfg(cfg, push->Handler, &segmentInfo.PushSegment, &BBCount, options);
		
#ifdef _DEBUG_COMMON_BACKEND
			{
				ostringstream name;
				name << "cfg_first_MIR_backend_" << cfg.getName() << ".txt";
				ofstream mircode(name.str().c_str());
				CodePrint<MIRNode> codeprinter(cfg);
				mircode << codeprinter;
				mircode.close();
			}
#endif
			
#ifdef ENABLE_COMPILER_PROFILING
			uint32_t statements_before = cfg.get_insn_num();
#endif
			init(cfg);
			compileCFG(cfg);

#ifdef ENABLE_COMPILER_PROFILING
			jit::opt::OptimizerStatistics<CFG<MIRNode> > optstat("After JIT");
			cout << optstat << endl;
				cout << "Number of statement before optimizations for " << pe->Name << ": " << statements_before << " and before backend: " << cfg.get_insn_num() << endl;
#endif

			auto_ptr<GenericBackend> backend(get_genericBackend(cfg));

			if(nvmFLAG_ISSET(options->Flags, nvmDO_ASSEMBLY))
			{
				if (options->OutputFilePrefix == NULL)
				{
					options->assembly_stream << "NetPE " << pe->Name << std::endl;
					options->assembly_stream << "Push Segment: " << std::endl;
					options->assembly_stream << "--------------------------------" << std::endl;
					backend->emitNativeAssembly(options->assembly_stream);
					options->assembly_stream << std::endl;
				}
				else
				{
					backend->emitNativeAssembly(string(options->OutputFilePrefix));
				}
				VerbOut(RTObj, 0, ">> .push segment compilation succeeded!\n");
			}
			
			
			else if(nvmFLAG_ISSET(options->Flags, nvmDO_NATIVE))
			{
				//ivano
				backend->emitNativeAssembly("asm_debug_asm");
			
				uint8_t* functPushBuffer = backend->emitNativeFunction();
				VerbOut(RTObj, 0, ">> .push segment compilation succeeded!\n");
				connectFunctionToHandler(functPushBuffer, push);
			}
			else
			{
				throw "compilation mode not define\n";
			}

			clear_algorithm_list();
		}

		//compile init segment if needed
		if(nvmFLAG_ISSET(options->Flags, nvmDO_ASSEMBLY) && nvmFLAG_ISSET(options->Flags, nvmDO_INIT) && options->OutputFilePrefix)
		{
			nvmHandlerState* init_h = pe->InitHandler->HandlerState;
			nvmNet_JitFill_Segments_Info(&segmentInfo, init_h);
			RegisterModel::reset();

			CFG<MIRNode> cfg(segmentInfo.InitSegment.Name);
			uint16_t BBCount(0);
			build_cfg(cfg, init_h->Handler, &segmentInfo.InitSegment, &BBCount, options);

			init(cfg);
			compileCFG(cfg);

#ifdef _DEBUG_OPTIMIZER
			{
				std::cout << "Prima di backend " << cfg.getName() << std::endl;
				jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(cfg);
				std::cout << codeprinter;
				std::cout.flush();
			}
	#endif

			#ifdef ENABLE_COMPILER_PROFILING
				cout << "Number of statement before backend for " << pe->Name << ": " << cfg.get_insn_num() << endl;
			#endif

			auto_ptr<GenericBackend> backend(get_genericBackend(cfg));

			if(nvmFLAG_ISSET(options->Flags, nvmDO_ASSEMBLY))
			{
				backend->emitNativeAssembly(string(options->OutputFilePrefix));
				VerbOut(RTObj, 0, ">> .init segment compilation succeeded!\n");
			}
			else
			{
				throw "compilation mode not define\n";
			}

			clear_algorithm_list();
		}
	} //for every pe
}

void TargetDriver::clear_algorithm_list()
{
	func_list_t::iterator i;
	for(i = algorithms.begin(); i != algorithms.end(); i++)
	{
#ifdef _DEBUG_JIT_IF
		clog << "elimino l'algoritmo: ";
		clog << (*i)->getName() << endl;
#endif
		delete *i;
	}
	algorithms.clear();
}

void TargetDriver::compileInline(DiGraph<nvmNetPE* >* pe_graph)
{
	RegisterModel::reset();

	uint16_t BBCount(0);
	int i = 0;

	//create cfg
	RegisterModel::reset();
	CFG<MIRNode> cfg("globalCFG");

	DiGraph<nvmNetPE *>::NodeIterator n = pe_graph->FirstNode();

	for(; n != pe_graph->LastNode(); n++, i++)
	{
		nvmByteCodeSegmentsInfo segmentInfo;
		nvmNetPE* pe = (*n)->NodeInfo;
		nvmHandlerState* push = pe->PushHandler->HandlerState;

		nvmNet_JitFill_Segments_Info(&segmentInfo, push);

		build_cfg(cfg, push->Handler, &segmentInfo.PushSegment, &BBCount, options);
	} // end for

#ifdef _DEBUG_CFG_BUILD
	{
		cout << "Before inlining " << cfg.getName() << endl;
		CodePrint<MIRNode> codeprinter(cfg);
		cout << codeprinter;
	}
#endif

	// Link the inlined cfgs together
	Perform_Linkage(cfg);

#ifdef _DEBUG_CFG_BUILD
	{
		cout << "After inlining " << cfg.getName() << endl;
		CodePrint<MIRNode> codeprinter(cfg);
		cout << codeprinter;

	}
#endif

	Delete_edges_from_entry(cfg, pe_graph);

#ifdef _DEBUG_CFG_BUILD
	{
		ostringstream filename;
		filename << "cfgInline_" << cfg.getName() << ".dot";

		ofstream dot(filename.str().c_str());
		DotPrint<MIRNode> printer(cfg);
		dot << printer;
		dot.close();
	}
#endif

	init(cfg);
	compileCFG(cfg);

#ifdef _DEBUG_OPTIMIZER
			{
				std::cout << "Prima di backend " << cfg.getName() << std::endl;
				jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(cfg);
				std::cout << codeprinter;
				std::cout.flush();
			}
	#endif

	auto_ptr<GenericBackend> backend(get_genericBackend(cfg));

	if(nvmFLAG_ISSET(options->Flags, nvmDO_ASSEMBLY))
	{
		if (options->OutputFilePrefix == NULL)
		{
			options->assembly_stream << "--------------------------------" << std::endl;
			backend->emitNativeAssembly(options->assembly_stream);
			options->assembly_stream << std::endl;
		}
		else
		{
			backend->emitNativeAssembly(string(options->OutputFilePrefix));
		}
	}
	else if(nvmFLAG_ISSET(options->Flags, nvmDO_NATIVE))
	{
		uint8_t* functPushBuffer = backend->emitNativeFunction();
		VerbOut(RTObj, 0, ">> .push segment compilation succeeded!\n");
		assert(pe_graph->GetNodeCount() > 0 && "PeGraph contains no nodes!!");
		nvmHandlerState* push = (*pe_graph->FirstNode())->NodeInfo->PushHandler->HandlerState;
		connectFunctionToHandler(functPushBuffer, push);
	}
	else
		assert(1 == 0 && "compilation mode not defined");

	return;
}

void TargetDriver::compileCFG(CFG<MIRNode>& cfg)
{
	func_list_t::iterator i;

#ifdef _DEBUG_COMMON_BACKEND
	int counter = 0;
#endif

	for(i = algorithms.begin(); i != algorithms.end(); i++)
	{
		//!\todo check returned value of the algoritms
		#ifdef _DEBUG_JIT_IF
			clog << "running algorithm " << (*i)->getName() << " on " << cfg.getName() << endl;
		#endif

		if(!(*i)->run())
		{
			ostringstream error;
			error << "Algorithm " << (*i)->getName() << " failed";
		//	throw error.str().c_str();
		}
		
		#ifdef _DEBUG_COMMON_BACKEND
		{
			ostringstream name;
			name << "cfg_MIR_backend_" << cfg.getName() << "_after_(" << counter << ")_" << (*i)->getName() << ".asm";
			ofstream mircode(name.str().c_str());
			CodePrint<MIRNode> codeprinter(cfg);
			mircode << codeprinter;
			mircode.close();
			counter++;
		}
		#endif
	}

#ifdef _DEBUG_COMMON_BACKEND
	//print the MIR code, in form of cfg and without registers and constants. 
	//in the output you can see the operations
	ofstream of((cfg.getName() + "_sign.txt").c_str());
	CodePrint<MIRNode, SignaturePrinter<MIRNode> > codeprinter(cfg);
	of << codeprinter;
	of.close();
#endif
}

int32_t Delete_edges_from_entry(CFG<MIRNode>& cfg, DiGraph<nvmNetPE *>* pe_graph)
{

	// Delete the edges between 0 and every BB that is not 1.
	BasicBlock<MIRNode> *zero(cfg.getBBById(0)), *current;
	int i = 0;

	DiGraph<nvmNetPE *>::NodeIterator m = pe_graph->FirstNode();
	for(; m != pe_graph->LastNode(); m++, i++)
	{
		nvmHandlerState* push = (*m)->NodeInfo->PushHandler->HandlerState;
		if(push->Handler->start_bb > 1) {
			current = cfg.getBBById(push->Handler->start_bb);

			current->removeFromPredecessors(0);
			zero->removeFromSuccessors(push->Handler->start_bb);

			// Modify the graph structure
#ifdef _DEBUG_CFG_BUILD
			cout << "Il " << *current << "Sta per essere eliminato" << endl;
#endif
			cfg.DeleteEdge(*(zero->getNode()), *(current->getNode()));
		}
	}

	return nvmSUCCESS;
}

int32_t Perform_Linkage(CFG<MIRNode> &cfg)
{
	typedef CFG<MIRNode>	CFG;
	typedef MIRNode			IRType;
	typedef BasicBlock<IRType>	BasicBlock;
	typedef IRType::RegType			RegType;


	list<BasicBlock *>::iterator i;

	bool done;

	do {
		done = true;
		list<BasicBlock *> *bbList(cfg.getBBList());

		for(i = bbList->begin(); i != bbList->end(); ++i) {
			list<IRType *> &code((*i)->getCode());
			list<IRType *>::iterator j;

			for(j = code.begin(); j != code.end(); ++j) {
				if((*j)->getOpcode() == SNDPKT) {
					SndPktNode *snd(dynamic_cast<SndPktNode *>(*j));
					assert(snd != 0);

					// Tour: pehandler->handlerstate->pestate->conntable[port].ctdhandler->handler

					// Fetch this PE's port table and port handler
					nvmPEHandler *handler((*i)->getProperty<nvmPEHandler *>("handler"));
					assert(handler != 0);

					nvmHandlerState *hs(handler->HandlerState);
					assert(hs != 0);

					// Check if we are connected to another PE
					if(!PORT_IS_CONN_PE(hs->Handler->OwnerPE->PortTable[snd->getPort_number()].PortFlags))
						continue;

					/*
					 * FIXME: Marco dice che bisogna aggiungere questo, ed io gli credo sulla parola
					 *if (HandlerState->PEState->ConnTable[port].CtdHandlerType == HANDLER_TYPE_CALLBACK)
					 */

					// If so, proceed with the inlining

					// FIXME: we assume that PEs are ONLY connected with push handlers!

					done = false;

					tmp_nvmPEState *pes(hs->PEState);
					nvmPortState port(pes->ConnTable[snd->getPort_number()]);
					nvmHandlerState *dst_hs(port.CtdHandler);
					nvmPEHandler *dst_handler(dst_hs->Handler);

#if 0
					nvmNetPE *pe(handler->OwnerPE);
					assert(pe != 0);

					// Sanity check on the port number
					assert(snd->getPort_number() < pe->NPorts);

					nvmPEPort *port = pe->PortTable + snd->getPort_number();
					assert(port != 0);

					// Fetch the next PE's head node
					nvmPEHandler *dst_handler(port->Handler);
					assert(dst_handler != 0);
#endif

					uint16_t dst_bb(dst_handler->start_bb);

					// Replace the sndpkt with a jump.w
					// I assume there can be only one pkt.send per basic block (as it should be regarded as a ret)
					JumpMIRNode * jump = new JumpMIRNode(JUMPW, (*i)->getId(), dst_bb, 0, RegType::invalid, 0, 0);
					code.insert(j, jump);

					// Delete the send packet node
					delete(*j);
					code.erase(j);

					// The CFG structure has changed. This BB now lead only to dst_bb.
					// We remove this basic block from the successor's predecessors lists.
					list<BasicBlock::node_t *> successors((*i)->getSuccessors());
					list<BasicBlock::node_t *>::iterator k;
					BasicBlock::node_t *l((*i)->getNode());

					for(k = successors.begin(); k != successors.end(); ++k) {
						(*i)->removeFromSuccessors((*k)->NodeInfo->getId());
						(*k)->NodeInfo->removeFromPredecessors((*i)->getId());

						// Update the CFG as well
						cfg.DeleteEdge(*l, **k);
					}

					// Update the CFG so it reflects the new structure
					cfg.AddEdge(*((*i)->getNode()), *(cfg.getBBById(dst_bb)->getNode()));

					// Restart the visit from the outer loop.
					break;
				}
			}
		}

		delete bbList;
	} while(!done);

	return nvmSUCCESS;
}

void nvmNet_JitFill_Segments_Info(nvmByteCodeSegmentsInfo *segmentsInfo, nvmHandlerState *HandlerState)
{
	char section_name[MAX_NETPE_NAME + 10];

	if(HandlerState->Handler->HandlerType == INIT_HANDLER)
	{
		segmentsInfo->InitSegment.LocalsSize = (HandlerState->Handler->NumLocals) *4 ;
		segmentsInfo->InitSegment.MaxStackSize = HandlerState->Handler->MaxStackSize;
		segmentsInfo->InitSegment.ByteCode = HandlerState->Handler->ByteCode;
		segmentsInfo->InitSegment.SegmentSize = HandlerState->Handler->CodeSize;
		snprintf(section_name, MAX_NETPE_NAME + 10, "%s%s", HandlerState->Handler->OwnerPE->Name, HandlerState->Handler->Name);
		segmentsInfo->InitSegment.Name = strdup(section_name);
		segmentsInfo->InitSegment.SegmentType = INIT_SEGMENT;
		uint32_t j = 0;
		uint8_t *byte_stream = HandlerState->Handler->Insn2LineTable;
		while(j < HandlerState->Handler->Insn2LineTLen)
		{
			uint32_t ip, line;
			ip = *(uint32_t*)&byte_stream[j];
			j += 4;
			line = *(uint32_t*)&byte_stream[j];
			j += 4;
			segmentsInfo->InitSegment.Insn2LineMap.insert(std::pair<uint32_t,uint32_t>(ip, line));
		}


	}
	else if(HandlerState->Handler->HandlerType == PUSH_HANDLER)
	{
		segmentsInfo->PushSegment.LocalsSize = (HandlerState->Handler->NumLocals) *4 ;
		segmentsInfo->PushSegment.MaxStackSize = HandlerState->Handler->MaxStackSize;
		segmentsInfo->PushSegment.ByteCode = HandlerState->Handler->ByteCode;
		segmentsInfo->PushSegment.SegmentSize = HandlerState->Handler->CodeSize;
		snprintf(section_name, MAX_NETPE_NAME + 10, "%s%s", HandlerState->Handler->OwnerPE->Name, HandlerState->Handler->Name);
		segmentsInfo->PushSegment.Name = strdup(section_name);
		segmentsInfo->PushSegment.SegmentType = PUSH_SEGMENT;
		uint32_t j = 0;
		uint8_t *byte_stream = HandlerState->Handler->Insn2LineTable;
		while(j < HandlerState->Handler->Insn2LineTLen)
		{
			uint32_t ip, line;
			ip = *(uint32_t*)&byte_stream[j];
			j += 4;
			line = *(uint32_t*)&byte_stream[j];
			j += 4;
			segmentsInfo->PushSegment.Insn2LineMap.insert(std::pair<uint32_t,uint32_t>(ip, line));
		}


	}
	else if(HandlerState->Handler->HandlerType == PULL_HANDLER)
	{
		segmentsInfo->PullSegment.LocalsSize = (HandlerState->Handler->NumLocals) *4 ;
		segmentsInfo->PullSegment.MaxStackSize = HandlerState->Handler->MaxStackSize;
		segmentsInfo->PullSegment.ByteCode = HandlerState->Handler->ByteCode;
		segmentsInfo->PullSegment.SegmentSize = HandlerState->Handler->CodeSize;
		snprintf(section_name, MAX_NETPE_NAME + 10, "%s%s", HandlerState->Handler->OwnerPE->Name, HandlerState->Handler->Name);
		segmentsInfo->PullSegment.Name = strdup(section_name);
		segmentsInfo->PullSegment.SegmentType = PULL_SEGMENT;
		uint32_t j = 0;
		uint8_t *byte_stream = HandlerState->Handler->Insn2LineTable;
		while(j < HandlerState->Handler->Insn2LineTLen)
		{
			uint32_t ip, line;
			ip = *(uint32_t*)&byte_stream[j];
			j += 4;
			line = *(uint32_t*)&byte_stream[j];
			j += 4;
			segmentsInfo->PullSegment.Insn2LineMap.insert(std::pair<uint32_t,uint32_t>(ip, line));
		}
	}

	char *PEName = (strlen(HandlerState->Handler->OwnerPE->Name) > 0 ? HandlerState->Handler->OwnerPE->Name : NULL);
	char *PEFileName = (strlen(HandlerState->Handler->OwnerPE->FileName) > 0 ? HandlerState->Handler->OwnerPE->FileName : NULL);

	segmentsInfo->PushSegment.PEName = PEName;
	segmentsInfo->PushSegment.PEFileName = PEFileName;
	segmentsInfo->PullSegment.PEName = PEName;
	segmentsInfo->PullSegment.PEFileName = PEFileName;
	segmentsInfo->InitSegment.PEName = PEName;
	segmentsInfo->InitSegment.PEFileName = PEFileName;
}


