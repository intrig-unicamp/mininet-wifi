/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "x11-backend.h"
#include "x11-emit.h"
#include "x11-optimization.h"
#include "inssel-x11.h"

#include "insselector.h"
#include "cfg_copy.h"
#include "cfg_ssa.h"
#include "cfg_printer.h"
#include "cfgdom.h"
#include "cfg_edge_splitter.h"
#include "cfg_loop_analyzer.h"
#include "register_mapping.h"
#include "copy_folding.h"
#include "opt/deadcode_elimination.h"
#include "op_size.h"

#include <iostream>
#include <fstream>

using namespace std;
using namespace opt;

jit::TargetDriver* x11_getTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, jit::TargetOptions* options)
{
	return new jit::X11::X11TargetDriver(netvm, RTObj, options);
}

namespace jit {
namespace X11 {

X11TargetDriver::X11TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
:
	TargetDriver(netvm, RTObj, options)
{
}

int32_t X11TargetDriver::check_options(uint32_t index)
{
	options->Flags |= nvmDO_INLINE;
	return TargetDriver::check_options(index);
}

void X11TargetDriver::init(CFG<MIRNode>& cfg)
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

	algorithms.push_back(new BasicBlockElimination< CFG<MIRNode> >(cfg));

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
		algorithms.push_back(new CanonicalizationStep<CFG<MIRNode> >(cfg));
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
	algorithms.push_back(new Fold_Copies< CFG<MIRNode> >(cfg));
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
		set<uint32_t> empty_set;
		algorithms.push_back( new Register_Mapping<CFG<MIRNode> >(cfg, 1, empty_set));
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

GenericBackend* X11TargetDriver::get_genericBackend(CFG<MIRNode>& cfg)
{
	return new X11Backend(cfg);
}

X11Backend::X11Backend(CFG<MIRNode>& cfg) : MLcfg(cfg), LLcfg(cfg.getName())
{
	// Nop
	return;
}

X11Backend::~X11Backend()
{
	// Nop
	return;
}

void X11Backend::emitNativeAssembly(std::string file_name)
{
	ofstream os(file_name.c_str());

	emitNativeAssembly(os);
	os.close();
}

void X11Backend::emitNativeAssembly(std::ostream &str)
{
	// Map every register space to virtual registers (for now)
	std::set<uint32_t> empty_set;
	jit::Register_Mapping<jit::CFG<jit::MIRNode> > reg_mapping(MLcfg, sp_virtual, empty_set);
	reg_mapping.run();

	{
		std::cout << "After register mapping: " << MLcfg.getName() << std::endl;
		jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(MLcfg);
		std::cout << codeprinter;
	}

	jit::Infer_Size infer(MLcfg);
	infer.run();
	sizes = infer.getMap();

	{
		std::cout << "After operand size inference: " << MLcfg.getName() << std::endl;
		jit::CodePrint<jit::MIRNode, jit::SSAPrinter<jit::MIRNode> > codeprinter(MLcfg);
		std::cout << codeprinter;
	}

	// Create a new CFG with the same structure as the old one but with empty BBs.
	CFGCopy<MIRNode,X11IRNode> copier(MLcfg, LLcfg);
	copier.buildCFG();

	{
		std::cout << "Low-level CFG: " << LLcfg.getName() << std::endl;
		jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
		std::cout << codeprinter;
	}

//	// Re-Split critical edges
//	jit::CFGEdgeSplitter<X11IRNode> es(LLcfg);
//	es.run();

	// Set up the code emitter
	X11EmissionContext ec(LLcfg, str);
	ctx = &ec;

	// Perform instruction selection
	X11InsSelector X11sel;
	InsSelector<X11InsSelector, X11IRNode> selector(X11sel,MLcfg, LLcfg);
	selector.instruction_selection(MB_NTERM_stmt);

	// Split the BBs containing coprocessor instructions
	X11SplitCoprocessors sc(LLcfg);
	sc.buildCFG();

	// Print the new CFG
	{
		std::cout << "After instruction selection: " << LLcfg.getName() << std::endl;
		jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
		std::cout << codeprinter;
	}

	// Recompute dominance
	jit::computeDominance<CFG<X11IRNode> >(LLcfg);

	// Copy folding
	jit::Fold_Copies<CFG<X11IRNode> > copy_folding(LLcfg);
	copy_folding.run();

	{
		cout << "Uscita da copy folding " << LLcfg.getName() << std::endl;
		CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
		std::cout << codeprinter;
	}

	// Eliminate coalesced copies
	{
		jit::opt::KillRedundantCopy< CFG<X11IRNode> > krc(LLcfg);
		krc.run();
	}


	// Remove redundancy + Dead Code Elimination
	Simple_Redundancy_Elimination sre(LLcfg);
	sre.run();

	{
		// Zero all version numbers
		Zero_Versions zv(LLcfg);
		zv.run();
	}

	// Eliminate coalesced copies
	{
		jit::opt::KillRedundantCopy< CFG<X11IRNode> > krc(LLcfg);
		krc.run();
	}

	{
		std::cout << "After simple redundancy elimination: " << LLcfg.getName() << std::endl;
		jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
		std::cout << codeprinter;
	}

	// Perform VLIW merging
	Merge merge(LLcfg);
	merge.run();

	// Print the new CFG
	{
		std::cout << "After instruction MERGE: " << LLcfg.getName() << std::endl;
		jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
		std::cout << codeprinter;
	}

#if 0

//	// Recompute dominance and liveness information
//	computeDominance<X11IRNode>(LLcfg);


	// Copy folding
	jit::Fold_Copies<CFG<X11IRNode> > copy_folding(LLcfg);
	copy_folding.run();

	{
		cout << "Uscita da copy folding " << LLcfg.getName() << std::endl;
		CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
		std::cout << codeprinter;
	}

	// Eliminate coalesced copies
	jit::opt::KillRedundantCopy<X11IRNode>(LLcfg);

	// Consolidate the virtual register space and the switch helpers without changing anything else.
	reg_space_t never_touch[] = {sp_const, sp_flags, sp_pkt_mem, sp_reg_mem, sp_offset, sp_sw_help, sp_sw_table};
	reg_space_t consolidate[] = {sp_virtual};
	std::set<uint32_t> do_not_touch;

	{
		for(unsigned i(0); i < sizeof(consolidate)/sizeof(*consolidate); ++i) {
			do_not_touch.clear();
			do_not_touch.insert(never_touch, never_touch+sizeof(never_touch)/sizeof(*never_touch));
			for(unsigned j(0); j < sizeof(consolidate)/sizeof(*consolidate); ++j) {
				if(i != j) {
					do_not_touch.insert(consolidate[j]);
				}
			}

			Register_Mapping<CFG<X11IRNode> > reg_mapping(LLcfg, consolidate[i], do_not_touch);
			reg_mapping.run();
		}
	}

	{
		// Zero all version numbers
		Zero_Versions zv(LLcfg);
		zv.run();
	}

	{
		std::cout << "After first optimization pass: " << LLcfg.getName() << std::endl;
		jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
		std::cout << codeprinter;
	}

#if 0
	// ----- Dead Code Elimination

	{
		// Recompute dominance and liveness information
		computeDominance<X11IRNode>(LLcfg);

		// Remove redundancy
		Dead_Code_Elimination dce(LLcfg);
		dce.run();

		{
			std::cout << "After dead code elimination: " << LLcfg.getName() << std::endl;
			jit::CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
			std::cout << codeprinter;
		}

		exit(0);

		// Copy folding
		jit::Fold_Copies<CFG<X11IRNode> > copy_folding(LLcfg);
		copy_folding.run();

		{
			cout << "Uscita da copy folding " << LLcfg.getName() << std::endl;
			CodePrint<X11IRNode, jit::SSAPrinter<X11IRNode> > codeprinter(LLcfg);
			std::cout << codeprinter;
		}

		// Consolidate the virtual register space and the switch helpers without changing anything else.
		reg_space_t never_touch[] = {sp_const, sp_flags, sp_pkt_mem, sp_offset};
		reg_space_t consolidate[] = {sp_virtual, sp_sw_help};
		std::set<uint32_t> do_not_touch;

		{
			unsigned i(0);
//	for(unsigned i(0); i < sizeof(consolidate)/sizeof(*consolidate); ++i) {
			do_not_touch.clear();
			do_not_touch.insert(never_touch, never_touch+sizeof(never_touch)/sizeof(*never_touch));
			unsigned j(1);
//		for(unsigned j(0); j < sizeof(consolidate)/sizeof(*consolidate); ++j) {
//			if(i != j) {
					do_not_touch.insert(consolidate[j]);
//			}
//		}

			Register_Mapping<CFG<X11IRNode> > reg_mapping(LLcfg, consolidate[i], do_not_touch);
			reg_mapping.run();
//	}
		}

		// Zero all version numbers
		Zero_Versions zv(LLcfg);
		zv.run();
	}
#endif

#endif

	// Perform instruction emission
	ec.run();
	ctx = 0;

	return;
}

X11SplitCoprocessors::X11SplitCoprocessors(CFG<X11IRNode> &c) : CFGBuilder<X11IRNode>(c)
{
	// Nop
	return;
}

X11SplitCoprocessors::~X11SplitCoprocessors()
{
	// Nop
	return;
}

void X11SplitCoprocessors::run()
{
	bool changes;

	// FIXME: assumes that the BB numbering is contiguous
	uint16_t highest(CFGBuilder<X11IRNode>::cfg.BBListSize() - 1);

	do {
		changes = false;

		// Iterate over every BB - order is not relevant
		CFG<X11IRNode>::NodeIterator i(CFGBuilder<X11IRNode>::cfg.FirstNode());

		for(; i != CFGBuilder<X11IRNode>::cfg.LastNode(); ++i) {
			BasicBlock<X11IRNode> *bb((*i)->NodeInfo);

			cout << "esamino il BB " << bb->getId() << '\n';

			list<X11IRNode *> &code(bb->getCode());
			list<X11IRNode *>::iterator ci;

			// Scan the whole instruction list and get the first coprocessor instruction (if any)
			for(ci = code.begin(); ci != code.end(); ++ci) {
				if(dynamic_cast<Prepare *>(*ci) != 0)
					break;
			}

			if(ci == code.end()) {
				cout << "non trovo un'op. dei coprocessori\n";
				continue;
			}

			++ci; // point to the node following the coprocessor instruction
			if(ci == code.end()) {
				cout << "l'istruzione e' gia' l'ultima\n";
				continue;
			}

			// iterate over successors
			list<BasicBlock<X11IRNode>::node_t *> l = bb->getSuccessors();
			list<BasicBlock<X11IRNode>::node_t *>::iterator li;

			for(li = l.begin(); li != l.end(); ++li) {
				BasicBlock<X11IRNode> *ss((*li)->NodeInfo);

				cout << "Considero il successore " << ss->getId() << '\n';

				// delete edge (current, successor)
				CFGBuilder<X11IRNode>::cfg.DeleteEdge(**i, **li);

				// insert new edge (current, new)
				addEdge(bb->getId(), highest);

				// insert new edge (new, successor)
				addEdge(highest, ss->getId());
			}

			// Get the new BB structiore
			BasicBlock<X11IRNode> *new_bb(CFGBuilder<X11IRNode>::cfg.getBBById(highest));

			// Now copy the instructions to the new BB
			copy(ci, code.end(), insert_iterator<list<X11IRNode *> >(new_bb->getCode(),
				new_bb->getCode().begin()));
//			new_bb->getCode().insert(ci, code.end());

			// For each instruction, rewrite the basic block it belongs to
			list<X11IRNode *>::iterator t;
			for(t = new_bb->getCode().begin(); t != new_bb->getCode().end(); ++t)
				(*t)->setBBId(highest);

			// Now delete those instructions from the old bb
			code.erase(ci, code.end());

			// Mark that we have actually made a split.
			++highest;
			changes = true;
			break;
		}
//		delete bblist;
	} while(changes);

	return;
}


} /* namespace X11 */
} /* namespace jit */

