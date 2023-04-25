/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "octeon-backend.h"
#include "inssel-octeon.h"
#include "insselector.h"
#include "octeon-regalloc.h"
#include "octeon-emit.h"
#include "application.h"
#include "opt/deadcode_elimination_2.h"
#include <sstream>
#include <fstream>

#include "mirnode.h"
#include "cfg.h"

#include "cfg_copy.h"
#include "cfg_edge_splitter.h"
#include "cfg_loop_analyzer.h"
#include "cfg_printer.h"
#include "cfgdom.h"
#include "cfg_ssa.h"
#include "mem_translator.h"
#include "opt/controlflow_simplification.h"
#include "opt/deadcode_elimination_2.h"
#include "opt/nvm_optimizer.h"

using namespace jit;
using namespace octeon;
using namespace std;
using namespace opt;

TargetDriver* octeon_getTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
{
	return new octeonTargetDriver(netvm, RTObj, options);
}

int32_t octeonTargetDriver::check_options(uint32_t index)
{
	options->Flags |= nvmDO_INIT;
	return TargetDriver::check_options(index);
}

octeonTargetDriver::octeonTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
:
	TargetDriver(netvm, RTObj, options)
{
}

/*!
 * \brief this function pre-compile PEs
 *
 */
DiGraph<nvmNetPE*>* octeonTargetDriver::precompile(DiGraph<nvmNetPE*>* pe_graph)
{

    // precompile header
    {
        ofstream code("octeon_precompile.h");

        code << "#ifndef OCTEON_PRECOMPILE\n"
                "#define OCTEON_PRECOMPILE\n";

		DiGraph<nvmNetPE *>::NodeIterator n = pe_graph->FirstNode();
		code << "#include \"nbnetvm.h\"\n";
		for(; n != pe_graph->LastNode(); n++)
		{
			nvmNetPE* pe = (*n)->NodeInfo;
			code << "int32_t  "<< pe->Name << "_init(void *,int ,void *);\n";
			code << "int32_t "<< pe->Name << "_push(nvmExchangeBuffer **,int ,nvmHandlerState *);\n";
		}
        code << "extern nvmHandlerFunction *nvmStartFirstPE;\n";
        code << "#endif\n";
        code.close();
    }   
	ofstream code("octeon_precompile.c");
    code << "#include \"octeon_precompile.h\"\n";

#ifdef RTE_PROFILE_COUNTERS
	{
		ostringstream funct;
		funct << "void octeon_print_PE_Statics() {" << endl;

		DiGraph<nvmNetPE *>::NodeIterator n = pe_graph->FirstNode();
		for(; n != pe_graph->LastNode(); n++)
		{
			nvmNetPE* pe = (*n)->NodeInfo;
			code << "uint64_t " << pe->Name << "_ticks = 0;\n";

			funct << "printf(\"Ticks for PE "<< pe->Name << " : %llu\\n\", " <<  pe->Name << "_ticks);" << endl;
		}

		funct<< "}" << endl;

		code << funct.str();
	}

#endif

#ifdef OCTEON_MULTICORE
    {
        //TODO SV add the ports
        code << endl
        << "typedef struct _myPortTable {"<< endl
        << "    char name[32];"<< endl
        << "    int32_t (*handler)(nvmExchangeBuffer **, int, nvmHandlerState *);"
        << endl 
        << "    int32_t (*init_handler)(void *, int, void*);"<< endl
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

void octeonTargetDriver::init(CFG<MIRNode>& cfg)
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
	octeonChecker* checker = new octeonChecker();
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

GenericBackend* octeonTargetDriver::get_genericBackend(CFG<MIRNode>& cfg)
{
	return new octeonBackend(cfg);
}

octeonChecker::octeonChecker()
{
	copro_space = Application::getCoprocessorRegSpace();
}

bool octeonChecker::operator()(RegType& a, RegType& b)
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

octeonTraceBuilder::octeonTraceBuilder(CFG<octeon_Insn>& cfg)
:
	TraceBuilder<CFG<octeon_Insn> >(cfg)
{
}

void octeonTraceBuilder::handle_no_succ_bb(bb_t *bb)
{
	assert(bb != NULL && bb->getId() == bb_t::EXIT_BB);
	bb->getCode().push_back(new octeonBranchLblOp());
	return;
}

void octeonTraceBuilder::handle_one_succ_bb(bb_t *bb)
{
	assert(bb != NULL);

	bb_t* next = bb->getProperty< bb_t* >(next_prop_name);
	std::list< CFG<octeon_Insn>::GraphNode* > successors(bb->getSuccessors());

	bb_t *target = successors.front()->NodeInfo;

	if(bb->getCode().size() == 0)
	{
		//empty BB should be eliminated from the cfg!!!
		//for now emit a jump to the target if necessary
		if(!next || next->getId() != target->getId())
		{
			bb->getCode().push_back( new octeonBranchLblOp(OCTEON_J, target->getId()) );
		}
	}
	else
	{
		octeon_Insn* insn = bb->getCode().back();
		uint16_t opcode = insn->getOpcode();

		//if not followed by next bb in emission
		if(!next || next->getId() != target->getId())
		{
			//if has not explicit jump
			if(opcode != OCTEON_J)
			{
				bb->getCode().push_back( new octeonBranchLblOp(OCTEON_J, target->getId()) );
			}
		}
		else
		{
			//we can remove last useless jump
			if(opcode == OCTEON_J || opcode == OCTEON_JR)
			{
				bb->getCode().pop_back();
				delete insn;
			}
		}
	}
	return;
}

void octeonTraceBuilder::handle_two_succ_bb(bb_t *bb)
{
	assert(bb != NULL);

	bb_t* next = bb->getProperty< bb_t* >(TraceBuilder<CFG<octeon_Insn> >::next_prop_name);
	//std::list< CFG<octeon_Insn>::GraphNode* > successors(bb->getSuccessors());

	octeon_Insn* insn = bb->getCode().back();
	octeonCondBranch* branch = dynamic_cast<octeonCondBranch *>(insn);
	assert(branch != NULL);

	uint16_t jt = branch->get_target();
	//successors.remove(cfg.getBBById(jt)->getNodePtr());
	uint16_t jf = branch->get_false_target();


	if(!next || (next->getId() != jt && next->getId() != jf))
	{
		bb->getCode().push_back( new octeonBranchLblOp(OCTEON_J, jf) );
		return;
	}

	if(insn->getOpcode() == OCTEON_J)
		return;

	if(jt == next->getId())
	{
		branch->invert();
	}

	//else bb is followed by is false target so it's correct
}

void octeonTraceBuilder::transform_long_jumps()
{
	for(trace_iterator_t i = begin(); i != end(); i++)
	{
		bb_t* current = *i;
		uint32_t count = current->getProperty<uint32_t>(count_prop_name);
		list< IR* >& code = current->getCode();
		list< IR*>::iterator it;

		for(it = code.begin(); it != code.end(); it++)
		{
			octeon_Insn* insn = *it;
			if (insn->isLongJump(cfg, count))
			{
				insn->toLongJump();
			}
			count++;
		}

	}
}

void octeonTraceBuilder::build_trace()
{
	TraceBuilder<CFG<octeon_Insn> >::build_trace();

	uint32_t count(0);

	for(trace_iterator_t i = begin(); i != end(); i++)
	{
		bb_t* current = *i;

		current->setProperty<uint32_t> (count_prop_name, count);
		count += current->get_insn_num();
	}

	transform_long_jumps();
}

octeonBackend::octeonBackend(CFG<MIRNode>& cfg)
:
	MLcfg(cfg),
	LLcfg(MLcfg.getName()),
	trace_builder(LLcfg)
{}

octeonBackend::~octeonBackend()
{ }


void octeonBackend::emitNativeAssembly(std::ostream &str)
{
	{
		CFGCopy<MIRNode, octeon_Insn> copier(MLcfg, LLcfg);
		copier.buildCFG();
	}

	{
		offsets.init();
		base_man.reset();
		OcteonInsSelector octeonSel;
		InsSelector<OcteonInsSelector, octeon_Insn> selector(octeonSel, MLcfg, LLcfg);
		selector.instruction_selection(MB_NTERM_stmt);
	}

	{
		jit::opt::KillRedundantCopy< CFG<octeon_Insn> > krc(LLcfg);
		krc.run();
	}

	#ifdef _DEBUG_OCTEON_BACKEND
	{
		ostringstream codefilename;
		codefilename << "cfg_octeonCode_" << LLcfg.getName() << ".txt";
		ofstream code(codefilename.str().c_str());
		CodePrint<octeon_Insn> codeprinter(LLcfg);
		code << codeprinter;
		code.close();
	}
	#endif

	{
		list<octeonRegType> machine_regs;
		list<octeonRegType> virtual_regs;
		list<octeonRegType> colors;

		octeonRegType::init_machine_regs(machine_regs);
		octeonRegType::init_virtual_regs(virtual_regs);
		octeonRegType::init_colors(colors);

		octeonRegSpiller reg_spiller(LLcfg);
		GCRegAlloc<jit::CFG<octeon_Insn> > regAlloc(LLcfg, virtual_regs, machine_regs, colors, reg_spiller);

		if(!regAlloc.run())
			throw "Register allocation failed\n";

		octeonRegType::rename_regs(virtual_regs, regAlloc);
		{
			jit::opt::KillRedundantCopy< CFG<octeon_Insn> > krc(LLcfg);
			krc.run();
		}

		trace_builder.build_trace();

		{
			octeonEmitter em(LLcfg, regAlloc, trace_builder);
			em.emit(str);
		}

	}

	return;
}

void octeonBackend::emitNativeAssembly(std::string prefix)
{
	{
		CFGCopy<MIRNode, octeon_Insn> copier(MLcfg, LLcfg);
		copier.buildCFG();
	}

	{
		offsets.init();
		base_man.reset();
		OcteonInsSelector octeonSel;
		InsSelector<OcteonInsSelector, octeon_Insn> selector(octeonSel, MLcfg, LLcfg);
		selector.instruction_selection(MB_NTERM_stmt);
	}

	{
		jit::opt::KillRedundantCopy< CFG<octeon_Insn> > krc(LLcfg);
		krc.run();
	}

	#ifdef _DEBUG_OCTEON_BACKEND
	{
		ostringstream codefilename;
		codefilename << "cfg_octeonCode_" << LLcfg.getName() << ".txt";
		ofstream code(codefilename.str().c_str());
		CodePrint<octeon_Insn> codeprinter(LLcfg);
		code << codeprinter;
		code.close();
	}
	#endif

	{
		list<octeonRegType> machine_regs;
		list<octeonRegType> virtual_regs;
		list<octeonRegType> colors;

		octeonRegType::init_machine_regs(machine_regs);
		octeonRegType::init_virtual_regs(virtual_regs);
		octeonRegType::init_colors(colors);

		octeonRegSpiller reg_spiller(LLcfg);
		GCRegAlloc<jit::CFG<octeon_Insn> > regAlloc(LLcfg, virtual_regs, machine_regs, colors, reg_spiller);

		if (!regAlloc.run())
			throw "Register allocation failed\n";

		octeonRegType::rename_regs(virtual_regs, regAlloc);
		{
			jit::opt::KillRedundantCopy< CFG<octeon_Insn> > krc(LLcfg);
			krc.run();
		}

		trace_builder.build_trace();

		{
			string filename(prefix + LLcfg.getName() + ".s");
			octeonEmitter em(LLcfg, regAlloc, trace_builder);
			em.emit(filename);
		}

	}

}
