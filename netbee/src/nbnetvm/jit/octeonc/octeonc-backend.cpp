/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include <sstream>
#include <fstream>

#include "application.h"
#include "cfg_copy.h"
#include "cfgdom.h"
#include "cfg_edge_splitter.h"
#include "cfg.h"
#include "cfg_loop_analyzer.h"
#include "cfg_printer.h"
#include "cfg_ssa.h"
#include "inssel-octeonc.h"
#include "insselector.h"
#include "int_structs.h"
#include "mem_translator.h"
#include "mirnode.h"
#include "octeonc-backend.h"
#include "octeonc-emit.h"
#include "opt/controlflow_simplification.h"
#include "opt/deadcode_elimination_2.h"
#include "opt/nvm_optimizer.h"

using namespace jit;
using namespace octeonc;
using namespace std;
using namespace opt;

TargetDriver* octeonc_getTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
{
	return new octeoncTargetDriver(netvm, RTObj, options);
}

int32_t octeoncTargetDriver::check_options(uint32_t index)
{
	options->Flags |= nvmDO_INIT;
	return TargetDriver::check_options(index);
}

octeoncTargetDriver::octeoncTargetDriver(
nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
:
	TargetDriver(netvm, RTObj, options)
{
}

/*!
 * \brief this function pre-compile PEs
 *
 */
DiGraph<nvmNetPE*>* 
octeoncTargetDriver::precompile(DiGraph<nvmNetPE*>* pe_graph)
{
	{
        // emit common header
        ofstream header("octeonc_precompile.h");
		DiGraph<nvmNetPE *>::NodeIterator n = pe_graph->FirstNode();
        // include netvm header
        header << "#ifndef OCTEONC_PRECOMPILE_H\n"
		          "#define OCTEONC_PRECOMPILE_H\n"
                  "#include \"octeon_coprocessors.h\"\n"
		          "#include \"nbnetvm.h\"\n";

        // handler function prototype for every pe and coprocessors
		for(; n != pe_graph->LastNode(); n++)
		{
			nvmNetPE* pe = (*n)->NodeInfo;
            // function prototypes
			header << "int32_t  "<< pe->Name << "_init(nvmExchangeBuffer **, uint32_t, nvmHandlerState*);\n";
			header << "int32_t "<< pe->Name << "_push(nvmExchangeBuffer **, uint32_t, nvmHandlerState*);\n";
            // data memory and coprocessors references
            header << "extern uint8_t " << pe->Name << "_data[];";
            // coprocessor structures
            tmp_nvmPEState* pe_state = pe->PEState;
            for(uint32_t i = 0; i < pe_state->NCoprocRefs; i++)
            {
                char* name = pe_state->CoprocTable[i].name;
                header << "extern nvmCoprocessorState "
                   << pe->Name << "_" << name << ";" << endl;
            }
            header << endl;
		}

        // packet dropping and output functions
        header << "void nvmDropPacket(nvmExchangeBuffer**, uint32_t, void*);\n"
                  "void nvmSendOut(nvmExchangeBuffer**, uint32_t, void*);\n"
                  "extern nvmHandlerFunction *nvmStartFirstPE;\n"
                  "#endif\n";
        header.close();
	}

    // actual precompilation code
    ofstream code("octeonc_precompile.c");
    code << "#include \"octeonc_precompile.h\"\n";

#ifdef OCTEONC_MULTICORE
// experimental code for multicore on octeon
    {
        code << endl
        << "typedef struct _myPortTable {"<< endl
        << "    char name[32];"<< endl
        << "    int32_t (*handler)(nvmExchangeBuffer **, uint32_t, nvmHandlerState *);"
        << endl 
        << "    int32_t (*init_handler)(void *, uint32_t, void*);"<< endl
        << "    int next_cpu[16];"<< endl
        << "} myPortTable;"<< endl;

        code << "myPortTable ptable [] = {" << endl;
        DiGraph<nvmNetPE *>::NodeIterator ni;
        for (ni = pe_graph->FirstNode(); ni != pe_graph->LastNode(); ++ni) {
            code << "{\"" << (*ni)->NodeInfo->Name << "\", ";
            code << "&" << (*ni)->NodeInfo->Name << "_push\", ";
            code << "&" << (*ni)->NodeInfo->Name << "_init\", ";
            code << "{PORT_INVALID,";
        }
        code << "};" << endl << endl;

    }
#endif

    code << "nvmHandlerFunction *nvmStartFirstPE;\n";
	code << "void nvmExecuteInit()\n"
			"{\n";

	DiGraph<nvmNetPE *>::NodeIterator ni = pe_graph->FirstNode();
	code << "nvmStartFirstPE =" << (*ni)->NodeInfo->Name <<"_push;\n";
	for(; ni != pe_graph->LastNode(); ni++)
	{
		nvmNetPE* pe = (*ni)->NodeInfo;
		code << pe->Name << "_init(0,0,0);\n";
	}
	code <<"}\n";
	code.close();

	return pe_graph;
}

void octeoncTargetDriver::init(CFG<MIRNode>& cfg)
{
	algorithms.push_back(new CFGEdgeSplitter<MIRNode>(cfg));

	//jump to jump elimination ?

#ifdef _DEBUG_CFG_BUILD
	{
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, NonSSAPrinter<MIRNode> >(cfg);
		ostringstream print_name;
		print_name << "Dopo lo split dei critical edge " << cfg.getName();
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	}
#endif

	algorithms.push_back(new BasicBlockElimination<CFG<MIRNode> >(cfg));

#ifdef _DEBUG_CFG_BUILD
	{
		ostringstream print_name;
		print_name << "Dopo la linearizzazione della memoria " << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, NonSSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	}
#endif

	algorithms.push_back(new ComputeDominance<CFG<MIRNode> >(cfg));
	algorithms.push_back(new SSA< CFG<MIRNode> >(cfg));

#ifdef _DEBUG_SSA
	{
		ostringstream print_name;
		print_name << "Dopo forma SSA " << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back(new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	}
#endif

	if(options->OptLevel > 0)
	{
		algorithms.push_back(new Optimizer< CFG<MIRNode> >(cfg));
#ifdef _DEBUG_OPTIMIZER

		ostringstream print_name;
		print_name << "Dopo Ottimizzazioni" << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
#endif
	}
	else
	{
		algorithms.push_back(new CanonicalizationStep< CFG<MIRNode> >(cfg));
	}

	algorithms.push_back(new UndoSSA< CFG<MIRNode> >(cfg));

#ifdef _DEBUG_SSA
	{
		ostringstream print_name;
		print_name << "Uscita da forma SSA " << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	}
#endif

	algorithms.push_back(new BasicBlockElimination<CFG<MIRNode> >(cfg));
	octeoncChecker* checker = new octeoncChecker();
	algorithms.push_back(new Fold_Copies< CFG<MIRNode> >(cfg, checker));
	algorithms.push_back(new KillRedundantCopy< CFG<MIRNode> >(cfg));

#ifdef _DEBUG_SSA
	{
		ostringstream print_name;
		print_name << "Uscita da copy folding " << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	}
#endif

	{
		// Register mapping to a dense space
		set<uint32_t> reg_set;
		reg_set.insert(Application::getCoprocessorRegSpace());
		algorithms.push_back( new Register_Mapping<CFG<MIRNode> >(cfg, 1, reg_set));
	}

#ifdef _DEBUG_SSA
	{
		ostringstream print_name;
		print_name << "Uscita da register mapping" << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	}
#endif

	algorithms.push_back( new EmptyBBElimination<CFG<MIRNode> >(cfg));
	algorithms.push_back( new BasicBlockElimination<CFG<MIRNode> >(cfg));

	algorithms.push_back( new ComputeDominance<CFG<MIRNode> >(cfg));
	algorithms.push_back( new LoopAnalyzer<MIRNode>(cfg));

#ifdef _DEBUG_CFG_BUILD
	{
		ostringstream filename;
		filename << "cfg_before_backend" << cfg.getName() << ".dot";
		CFGPrinter<MIRNode>* codeprinter = new DotPrint<MIRNode>(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(filename.str(), codeprinter, filename.str()));
	}
#endif
}

GenericBackend* 
octeoncTargetDriver::get_genericBackend(CFG<MIRNode>& cfg)
{
	return new octeoncBackend(cfg);
}

octeoncTraceBuilder::octeoncTraceBuilder(CFG<octeonc_Insn>& cfg)
:
	TraceBuilder<CFG<octeonc_Insn> >(cfg)
{
}

void octeoncTraceBuilder::build_trace()
{
	TraceBuilder<CFG<octeonc_Insn> >::build_trace();

	uint32_t count(0);

	for(trace_iterator_t i = begin(); i != end(); i++)
	{
		bb_t* current = *i;

		current->setProperty<uint32_t> (count_prop_name, count);
		count += current->get_insn_num();
	}

	transform_long_jumps();
}

octeoncBackend::octeoncBackend(CFG<MIRNode>& cfg)
:
	MLcfg(cfg),
	LLcfg(MLcfg.getName()),
	trace_builder(LLcfg)
{}

octeoncBackend::~octeoncBackend()
{ }


void octeoncBackend::emitNativeAssembly(std::ostream &str)
{
	{
		CFGCopy<MIRNode, octeonc_Insn> copier(MLcfg, LLcfg);
		copier.buildCFG();
	}

	{
		OcteoncInsSelector octeoncSel;
		InsSelector<OcteoncInsSelector, octeonc_Insn> selector(
            octeoncSel, MLcfg, LLcfg);
		selector.instruction_selection(MB_NTERM_stmt);
	}

	#ifdef _DEBUG_OCTEON_BACKEND
	{
		ostringstream codefilename;
		codefilename << "cfg_octeoncCode_" << LLcfg.getName() << ".txt";
		ofstream code(codefilename.str().c_str());
		CodePrint<octeonc_Insn> codeprinter(LLcfg);
		code << codeprinter;
		code.close();
	}
	#endif

    trace_builder.build_trace();

    octeoncEmitter em(LLcfg, trace_builder);
    em.emit(str);

	return;
}

void octeoncBackend::emitNativeAssembly(std::string prefix)
{
    string filename(prefix + MLcfg.getName() + ".c");
    std::ofstream os(filename.c_str());
    emitNativeAssembly(os);
    os.close();
}

octeoncChecker::octeoncChecker()
{
	copro_space = Application::getCoprocessorRegSpace();
}

bool octeoncChecker::operator()(RegType& a, RegType& b)
{
	uint32_t a_space(a.get_model()->get_space());
	uint32_t b_space(b.get_model()->get_space());

	if(a_space == b_space)
	{
		if(a_space == copro_space)
		{
			return a.get_model()->get_name() == b.get_model()->get_name();
		}
	} else if(a_space == copro_space || b_space == copro_space)
		return false;

	return true;
}

