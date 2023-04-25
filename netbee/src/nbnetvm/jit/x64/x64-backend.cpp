/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file x64-backend.cpp
 * \brief implementation o the function exported by the class x64Backend
 */


#include "x64-backend.h"
#include "x64-asm.h"
#include "x64-emit.h"
#include "x64-opt.h"
#include "udis86.h"
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

#include "opt/bcheck_remove.h"

#include "gc_regalloc.h"
#include "x64-regalloc.h"
#include "inssel-x64.h"
#include "insselector.h"

#include <string>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace jit;
using namespace x64;
using namespace opt;
using namespace std;

static void addPrologue(CFG<x64Instruction>& cfg, GCRegAlloc<CFG<x64Instruction> >& regAlloc);
static void addEpilogue(CFG<x64Instruction>& cfg, GCRegAlloc<CFG<x64Instruction> >& regAlloc);

x64TargetDriver::x64TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
:
	TargetDriver(netvm, RTObj, options)
{
}

TargetDriver* x64_getTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
{
	return new x64TargetDriver(netvm, RTObj, options);
}

void x64TargetDriver::init(CFG<MIRNode>& cfg)
{
	algorithms.push_back(new CFGEdgeSplitter<MIRNode>(cfg));

	//jump to jump elimination ?

#ifdef _DEBUG_CFG_BUILD
	{
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, NonSSAPrinter<MIRNode> >(cfg);
		ostringstream print_name;
		print_name << "Dopo lo split dei critical edge " << cfg.getName();
		string outstring("stdout");
		algorithms.push_back( new DoPrint<MIRNode>(outstring, codeprinter, print_name.str()));
	}
#endif

	algorithms.push_back(new BasicBlockElimination<CFG<MIRNode> >(cfg));
	//algorithms.push_back( new MemTranslator(cfg));

	algorithms.push_back(new ComputeDominance<CFG<MIRNode> >(cfg));
	algorithms.push_back(new SSA< CFG<MIRNode> >(cfg));

#ifdef _DEBUG_OPTIMIZER
	{
		ostringstream print_name;
		print_name << "Dopo forma SSA " << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back(new DoPrint<MIRNode>(string("stdout"), codeprinter, print_name.str()));
	}
#endif

	if(options->OptLevel > -1)
	{
#ifdef ENABLE_COMPILER_PROFILING
		cout << "NetVM Optimizer enabled" << endl;
#endif
		algorithms.push_back(new Optimizer< CFG<MIRNode> >(cfg));
#ifdef _DEBUG_OPTIMIZER

		ostringstream print_name;
		print_name << "After opt" << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(string("stdout"), codeprinter, print_name.str()));
#endif
	}
	else
	{
#ifdef ENABLE_COMPILER_PROFILING
		cout << "NetVM Optimizer disabled" << endl;
#endif

		algorithms.push_back(new CanonicalizationStep<CFG<MIRNode> >(cfg));
	}


	if( nvmFLAG_ISSET(options->Flags, nvmDO_BCHECK ) && (options->OptLevel > 1) )
	{
		algorithms.push_back(new Boundscheck_remover< CFG<MIRNode> >(cfg, options));
	
		#ifdef _DEBUG_SSA
	
		ostringstream print_name;
		print_name << "Uscita da Boundscheck_remover " << cfg.getName() << std::endl;
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(string("stdout"), codeprinter, print_name.str()));
	
		#endif
	}

	algorithms.push_back(new UndoSSA< CFG<MIRNode> >(cfg));

#ifdef _DEBUG_SSA
	{
		ostringstream print_name;
		print_name << "Uscita da forma SSA " << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(string("stdout"), codeprinter, print_name.str()));
	}
#endif

	algorithms.push_back(new BasicBlockElimination< CFG<MIRNode> >(cfg));
	x64Checker* checker = new x64Checker();
	algorithms.push_back(new Fold_Copies< CFG<MIRNode> >(cfg, checker));
	algorithms.push_back(new KillRedundantCopy< CFG<MIRNode> >(cfg));

#ifdef _DEBUG_SSA
	{
		ostringstream print_name;
		print_name << "Uscita da copy folding " << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(string("stdout"), codeprinter, print_name.str()));
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
		algorithms.push_back( new DoPrint<MIRNode>(string("stdout"), codeprinter, print_name.str()));
	}
#endif


#ifdef _DEBUG_CFG_BUILD
	{
		ostringstream filename;
		filename << "cfg_before_loopAnalyzer" << cfg.getName() << ".dot";
		CFGPrinter<MIRNode>* codeprinter = new DotPrint<MIRNode>(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(filename.str(), codeprinter, ""/*filename.str()*/));
	}
#endif
	algorithms.push_back( new BasicBlockElimination< CFG<MIRNode> >(cfg));
	algorithms.push_back( new ComputeDominance< CFG<MIRNode> >(cfg));
	algorithms.push_back( new LoopAnalyzer<MIRNode>(cfg));

	algorithms.push_back( new JumpToJumpElimination<CFG<MIRNode> >(cfg) );
	algorithms.push_back( new EmptyBBElimination< CFG<MIRNode> >(cfg));
	algorithms.push_back( new BasicBlockElimination< CFG<MIRNode> >(cfg));

#ifdef _DEBUG_CFG_BUILD
	{
		ostringstream print_name;
		print_name << "Dopo rimozione dei jump e dei basic block vuoti!" << cfg.getName();
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, NonSSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(string("stdout"), codeprinter, print_name.str()));
	}
#endif

#ifdef _DEBUG_CFG_BUILD
	{
		ostringstream filename;
		filename << "cfg_before_backend" << cfg.getName() << ".dot";
		CFGPrinter<MIRNode>* codeprinter = new DotPrint<MIRNode>(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(filename.str(), codeprinter, ""/*filename.str()*/));
	}
#endif
}

GenericBackend* x64TargetDriver::get_genericBackend(CFG<MIRNode>& cfg)
{
	return new x64Backend(cfg);
}

x64Checker::x64Checker()
{
	copro_space = Application::getCoprocessorRegSpace();
}

bool x64Checker::operator()(RegType& a, RegType& b)
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

/*!
 * \param cfg the source CFG
 */
x64Backend::x64Backend(CFG<MIRNode>& cfg)
: MLcfg(cfg), LLcfg(cfg.getName()),
  code_created(false), buffer(NULL),
  trace_builder(LLcfg) {}

x64Backend::~x64Backend() {
}

static void	init_machine_registers(std::list<RegisterInstance>& machineRegisters)
{
	static const x64Regs_64 precolored[] = {RSP, RBP /*function parameters follow */, EXCHANGE_BUFFER_REGISTER, INPUT_PORT_REGISTER, HANDLER_STATE_REGISTER};
	static const int n = 5;

	for(int i = 0; i < n; i++)
	{
		machineRegisters.push_back(RegisterInstance(MACH_SPACE, (uint32_t)precolored[i]));
	}
}

static void	init_virtual_registers(std::list<RegisterInstance>& virtualRegisters)
{
	int latest_virt =  RegisterModel::get_latest_name(VIRT_SPACE);

	for(int i = 0; i <= latest_virt; i++)
	{
		virtualRegisters.push_back(RegisterInstance(VIRT_SPACE, i));
	}
}

static void init_colors(std::list<RegisterInstance>& colors)
{
#ifdef _WIN64
	static const x64Regs_32 avaible[] = {EAX, /* ECX, EDX,*/ ESI, EDI, /*R8D,*/  R9D,  R10D,  R11D,  R12D,  R13D,  R14D, R15D, EBX};
#else
	static const x64Regs_32 avaible[] = {EAX, ECX /*, EDX, ESI, EDI*/, R8D,  R9D,  R10D,  R11D,  R12D,  R13D,  R14D, R15D, EBX};
#endif

	static const int n = 11;

	for(int i = 0; i < n; i++)
	{
		colors.push_back(RegisterInstance(MACH_SPACE, (uint32_t)avaible[i]));
	}
}

static void	rename_regs(std::list<RegisterInstance>& virtualRegisters, GCRegAlloc<CFG<x64Instruction> >& regAlloc)
{
	typedef std::list<pair<RegisterInstance, RegisterInstance> > list_t;
	typedef std::list<pair<RegisterInstance, RegisterInstance> >::iterator iterator_t;
	list_t registers = regAlloc.getColors();

	for(iterator_t reg = registers.begin(); reg != registers.end(); reg++)
	{
		(*reg).first.get_model()->rename((*reg).second.get_model()->get_space(), (*reg).second.get_model()->get_name());
	}
}
#if 0
static bool is_useless(x64Instruction *actual, x64Instruction *prev)
{
	assert(actual != NULL && prev != NULL);
	if(actual->getOpcode() != prev->getOpcode() && actual->getOpcode() != X64_MOV)
		return false;
	//operations have the same opcode
	if(actual->NOperands == 2)
	{
	if(actual->Operands[0] == prev->Operands[0]
			&& actual->Operands[1] == prev->Operands[1])
		return true;

	if(actual->Operands[0] == prev->Operands[1]
			&& actual->Operands[1] == prev->Operands[0])
		return true;
	}
	else if (actual->NOperands == 1)
	{
		if(actual->Operands[0] == prev->Operands[0])
			return true;
	}
	return false;
}

static void delete_copy_pair(jit::CFG<x64Instruction> &cfg)
{
	typedef std::list<BasicBlock<x64Instruction> *> bb_list_t;
	typedef bb_list_t::iterator bb_list_it_t;
	typedef std::list<x64Instruction*> code_list_t;
	typedef code_list_t::iterator code_list_it_t;

	x64Instruction *prev_stmt;

	bb_list_t *bl = cfg.getBBList();
	for(bb_list_it_t i = bl->begin(); i != bl->end(); i++)
	{
		prev_stmt = NULL;
		code_list_t &code_l = (*i)->getCode();
		for(code_list_it_t j = code_l.begin(); j != code_l.end();)
		{
			if(prev_stmt == NULL)
			{
				prev_stmt = *j;
				j++;
				continue;
			}

			if(is_useless(*j, prev_stmt) && !(*j)->has_side_effects())
			{
#ifdef _DEBUG_X64_BACKEND
				std::cout << **i << "*********Elimino l'istruzione ridondante"<< std::endl;
				std::cout << "Prev: " << *prev_stmt << std::endl;

				std::cout << "---> Next: " << **j << std::endl;
#endif
				j = code_l.erase(j);
			}
			else
			{
				prev_stmt = (*j);
				j++;
			}
		}
	}
}

#endif

x64TraceBuilder::x64TraceBuilder(CFG<x64Instruction>& cfg)
:
	TraceBuilder<CFG<x64Instruction> >(cfg)
{
}

void x64TraceBuilder::handle_no_succ_bb(bb_t *bb)
{
	assert(bb != NULL && bb->getId() == BasicBlock<x64Instruction>::EXIT_BB);
	return;
}

void x64TraceBuilder::handle_one_succ_bb(bb_t *bb)
{
	assert(bb != NULL);

	bb_t* next = bb->getProperty< bb_t* >(next_prop_name);
	std::list< CFG<x64Instruction>::GraphNode* > successors(bb->getSuccessors());

	bb_t *target = successors.front()->NodeInfo;

	if(bb->getCode().size() == 0)
	{
		//empty BB should be eliminated from the cfg!!!
		//for now emit a jump to the target if necessary
		if(!next || next->getId() != target->getId())
		{
			x64_Asm_JMP_Label(bb->getCode(), target->getId());
		}
	}
	else
	{
		x64Instruction* insn = bb->getCode().back();
		uint16_t opcode = OP_ONLY(insn->getOpcode());

		//if not followed by next bb in emission
		if(!next || next->getId() != target->getId())
		{
			//if has not explicit jump
			if(opcode != X64_JMP && opcode != X64_J && opcode != X64_JECXZ)
			{
				//add the jump
				x64_Asm_JMP_Label(bb->getCode(), target->getId());
			}
		}
		else
		{
			//we can remove last useless jump
			if(opcode == X64_JMP)
			{
				bb->getCode().pop_back();
				delete insn;
			}
		}
	}
	return;
}

void x64TraceBuilder::handle_two_succ_bb(bb_t *bb)
{
	assert(bb != NULL);

	bb_t* next = bb->getProperty< bb_t* >(TraceBuilder<CFG<x64Instruction> >::next_prop_name);
	std::list< CFG<x64Instruction>::GraphNode* > successors(bb->getSuccessors());

	x64Instruction* insn = bb->getCode().back();
	
	uint16_t jt = LBL_OPERAND(insn->Operands[0].Op)->Label;
	successors.remove(cfg.getBBById(jt)->getNodePtr());
	uint16_t jf = successors.front()->NodeInfo->getId();


	if(!next || (next->getId() != jt && next->getId() != jf))
	{
		x64_Asm_JMP_Label(bb->getCode(), jf);
		return;
	}

	if(insn->getOpcode() == X64_JMP)
		return;

	if(jt == next->getId())
	{
		//invert instruction
		uint8_t cc = CC_ONLY(insn->getOpcode());

		cc += (cc % 2 == 0 ? 1 : -1);

		LBL_OPERAND(insn->Operands[0].Op)->Label = jf;
		insn->setOpcode(OP_CC(X64_J, cc));
	}

	//else bb is followed by is false target so it's correct
}

bool jit::x64::x64Backend::create_code()
{
	if(code_created)
		return true;
	{
		CFGCopy<MIRNode,x64Instruction> copier(MLcfg, LLcfg);
		copier.buildCFG();
	}

	list<RegisterInstance> machineRegisters;
	list<RegisterInstance> virtualRegisters;
	list<RegisterInstance> colors;

	#ifdef _DEBUG_X64_BACKEND
	{
		//create a .dot file with the cfg
		ostringstream filename;
		filename << "cfg_copy_LIRgraph_" << LLcfg.getName() << ".dot";

		ofstream dot(filename.str().c_str());
		DotPrint<x64Instruction> printer(LLcfg);
		dot << printer;
		dot.close();
	}
	{
		//create a .txt file with the MIR code organized as a cfg
		ostringstream codefilename2;
		codefilename2 << "cfg_regalloc_MIRcode_" << MLcfg.getName() << ".txt";

		ofstream code2(codefilename2.str().c_str());
		CodePrint<MIRNode> codeprinter2(MLcfg);
		code2 << codeprinter2;
		code2.close();
	}
	#endif

	{
		x64_offsets.init();
		base_manager.reset();
		dim_manager.reset();
		X64InsSelector IAsel;
		InsSelector<X64InsSelector, x64Instruction> selector(IAsel,MLcfg, LLcfg);
		selector.instruction_selection(MB_NTERM_stmt);
	}

	#ifdef _DEBUG_X64_BACKEND
	{
		ostringstream codefilename;
		codefilename << "cfg_x64LIRcode_" << LLcfg.getName() << ".txt";

		ofstream code(codefilename.str().c_str());
		CodePrint<x64Instruction> codeprinter(LLcfg);
		code << codeprinter;
		code.close();
	}
	#endif
	
	{
	  jit::x64::x64Optimizations opt(LLcfg);
	  opt.run();
	}

	init_machine_registers(machineRegisters);
	init_virtual_registers(virtualRegisters);
	init_colors(colors);

	{
	jit::opt::KillRedundantCopy< CFG<x64Instruction> > krc(LLcfg);
	krc.run();
	}

	x64RegSpiller regSp(LLcfg);
	GCRegAlloc<jit::CFG<x64Instruction> > regAlloc(LLcfg, virtualRegisters, machineRegisters, colors, regSp);

	#ifdef ENABLE_COMPILER_PROFILING
		std::cout << "numero di registri prima della regalloc in "<< LLcfg.getName() << ": " << virtualRegisters.size() << std::endl;
	#endif

	if(!regAlloc.run())
		throw "register allocation failed\n";

	#ifdef ENABLE_COMPILER_PROFILING
		std::cout << "numero di registri spillati in " << LLcfg.getName() << ": " << regSp.getNumSpilledReg() << std::endl;
	#endif

	rename_regs(virtualRegisters, regAlloc);


	#ifdef _DEBUG_X64_BACKEND
	{
		ostringstream codefilename2;
		codefilename2 << "cfg_regalloc_x64LIRcode_" << LLcfg.getName() << ".txt";

		ofstream code2(codefilename2.str().c_str());
		CodePrint<x64Instruction> codeprinter2(LLcfg);
		code2 << codeprinter2;
		code2.close();
	}
	#endif

	{
	jit::opt::KillRedundantCopy< CFG<x64Instruction> > krc(LLcfg);
	krc.run();
	}

	trace_builder.build_trace();
	// we make a scan of the code to delete some useless instructions
	//delete_copy_pair(LLcfg);

	#ifdef ENABLE_COMPILER_PROFILING
		cout << "Number of insn for " << LLcfg.getName() << "after backend: " << LLcfg.get_insn_num() << endl;
	#endif

	addPrologue(LLcfg, regAlloc);
	addEpilogue(LLcfg, regAlloc);

	#ifdef _DEBUG_X64_BACKEND
	{
		ostringstream codefilename3;
		codefilename3 << "cfg_before_emission_x64LIRcode_" << LLcfg.getName() << ".txt";

		ofstream code3(codefilename3.str().c_str());
		CodePrint<x64Instruction> codeprinter3(LLcfg);
		code3 << codeprinter3;
		code3.close();
	}
	#endif


	x64_Emitter emitter(LLcfg, regAlloc, trace_builder);
	buffer = emitter.emit();
	actual_buff_sz = emitter.getActualBufferSize();
	code_created = true;	
	
	return true;
}

static void addPrologue(CFG<x64Instruction>& cfg, GCRegAlloc<CFG<x64Instruction> >& regAlloc)
{
	uint32_t calleeSaveMask[X64_NUM_REGS] = X64_CALLEE_SAVE_REGS_MASK;
	uint32_t i;

	BasicBlock<x64Instruction>* entry = cfg.getBBById(BasicBlock<x64Instruction>::ENTRY_BB);
	list<x64Instruction*> code;

	x64_Asm_Comment(code, "function prologue");

	//x64_Asm_Op_Reg(code, X64_PUSH, X64_MACH_REG(RBP), x64_QWORD);
	for (i = 0; i < X64_NUM_REGS; i++){
		if (i == RBP || (calleeSaveMask[i] && regAlloc.isAllocated(X64_MACH_REG(i)))){
			x64_Asm_Op_Reg(code, X64_PUSH, X64_MACH_REG(i), x64_QWORD);
		}
	}
	x64_Asm_Op_Reg_To_Reg(code, X64_MOV, X64_MACH_REG(RSP), X64_MACH_REG(RBP), x64_QWORD);
	//x64-regalloc return only register spilled in memory and not the one spilled in xmm registers 
	uint32_t numSpilled = regAlloc.getNumSpilledReg();
	if(numSpilled != 0)
		x64_Asm_Op_Imm_To_Reg(code, X64_SUB, numSpilled * 8, X64_MACH_REG(RSP), x64_QWORD);


#ifdef JIT_RTE_PROFILE_COUNTERS
	x64_Asm_Comment(code, "profiling instructions");
	x64_Asm_Op_Imm_To_Reg(code, X64_SUB, 8, X64_MACH_REG(RSP), x64_DWORD);

	nvmHandlerState *HandlerState = Application::getApp(entry->getId()).getCurrentPEHandler()->HandlerState;
	uint64_t* addr = &HandlerState->ProfCounters->TicksDelta;

	x64_Asm_Comment(code, "Delta ticks");
	x64_Asm_Op(code, X64_RDTSC);
	x64_Asm_Op_Reg_To_Mem_Base(code, X64_MOV, X64_MACH_REG(EAX), X64_MACH_REG(RSP), 0, x64_DWORD);
	x64_Asm_Op_Reg_To_Mem_Base(code, X64_MOV, X64_MACH_REG(EDX), X64_MACH_REG(RSP), 4, x64_DWORD);
	x64_Asm_Op(code, X64_RDTSC);
	x64_Asm_Op_Mem_Base_To_Reg(code, X64_SUB, X64_MACH_REG(RSP), 0, X64_MACH_REG(EAX), x64_DWORD);
	x64_Asm_Op_Mem_Base_To_Reg(code, X64_SBB, X64_MACH_REG(RSP), 4, X64_MACH_REG(EDX), x64_DWORD);
	x64_Asm_Op_Reg_To_Mem_Displ(code, X64_MOV, X64_MACH_REG(EAX), (uint64_t)addr, x64_DWORD);
	x64_Asm_Op_Reg_To_Mem_Displ(code, X64_MOV, X64_MACH_REG(EDX), (uint64_t)(addr) + 4, x64_DWORD);

	x64_Asm_Comment(code, "Now sample starting time");
	x64_Asm_Op(code, X64_RDTSC);
	x64_Asm_Op_Reg_To_Mem_Base(code, X64_MOV, X64_MACH_REG(EAX), X64_MACH_REG(RSP), 0, x64_DWORD);
	x64_Asm_Op_Reg_To_Mem_Base(code, X64_MOV, X64_MACH_REG(EDX), X64_MACH_REG(RSP), 4, x64_DWORD);
#endif

#ifdef _DEBUG_X64_CODE
	x64_Asm_Op_Imm(code, X64_PUSH, (uint32_t)Application::getApp(0).getCurrentSegment()->Name);
	x64_Asm_Op_Imm(code, X64_CALL, (uint32_t) puts);
	x64_Asm_Op_Imm(code, X64_POP, X64_MACH_REG(EAX));
#endif
	x64_Asm_Comment(code, "function prologue ends");

	list<x64Instruction*>& old_code = entry->getCode();

	old_code.insert( old_code.begin(), code.begin(), code.end());
}

static void addEpilogue(CFG<x64Instruction>& cfg, GCRegAlloc<CFG<x64Instruction> >& regAlloc)
{
	uint32_t calleeSaveMask[X64_NUM_REGS] = X64_CALLEE_SAVE_REGS_MASK;
	int32_t i;

	BasicBlock<x64Instruction>* exit = cfg.getBBById(BasicBlock<x64Instruction>::EXIT_BB);
	list<x64Instruction*>& code = exit->getCode();

	x64_Asm_Comment(code, "function epilogue");

#ifdef JIT_RTE_PROFILE_COUNTERS
	x64_Asm_Op_Imm_To_Reg(code, X64_ADD, 8, X64_MACH_REG(RSP), x64_DWORD);
#endif

	uint32_t numSpilled = regAlloc.getNumSpilledReg();
	if(numSpilled != 0)
		x64_Asm_Op_Imm_To_Reg(code, X64_ADD, numSpilled * 8, X64_MACH_REG(RSP), x64_QWORD);
		
	//x64_Asm_Op_Reg_To_Reg(code, X64_MOV, X64_MACH_REG(RBP), X64_MACH_REG(RSP), x64_QWORD);
	for (i = X64_NUM_REGS - 1; i >= 0; i--){
		if (i == RBP || (calleeSaveMask[i] && regAlloc.isAllocated(X64_MACH_REG(i))))
			x64_Asm_Op_Reg(code, X64_POP, X64_MACH_REG(i), x64_QWORD);
	}
	//x64_Asm_Op_Reg(code, X64_POP, X64_MACH_REG(RBP), x64_QWORD);
	x64_Asm_Op(code, X64_RET);

}

uint8_t* x64Backend::emitNativeFunction()
{
	if(!create_code())
		return NULL;
#ifdef _DEBUG_X64_BACKEND	
	std::cout << "disassembly code to file" << std::endl;
	emitNativeAssembly("x64_");
	std::cout << "disassembly code to file: DONE" << std::endl;
#endif
	return buffer;
}

void x64::x64Backend::emitNativeAssembly(std::string prefix)
{
	//printf("Emitting native assembly:\n");
	if(!create_code())
		return;

	string filename(prefix + LLcfg.getName() + ".asm");

	ofstream os(filename.c_str());

	emitNativeAssembly(os);
	os.close();
	
	cout << "Assembly code for '"  << LLcfg.getName() << "' written to file: " <<  filename << endl;

}



void x64::x64Backend::emitNativeAssembly(std::ostream &os)
{
	//printf("Emitting native assembly:\n");
	if(!create_code())
		return;

	//Invoke the disassembler...
	ud_t ud_obj;

	ud_init(&ud_obj);
	ud_set_input_buffer(&ud_obj, buffer, actual_buff_sz);
	//ud_set_input_file(&ud_obj, stdin);
	ud_set_mode(&ud_obj, 64);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);

	//printf("binary disassembly (still experimental):\n");
	while (ud_disassemble(&ud_obj)) {
		os << setw(8) << setfill('0') << hex << ud_insn_off(&ud_obj) << "  ";
		os << ud_insn_asm(&ud_obj) << endl;
//		printf("%08x  ", ud_insn_off(&ud_obj));
//		printf("%s\n", ud_insn_asm(&ud_obj));
	}

}
