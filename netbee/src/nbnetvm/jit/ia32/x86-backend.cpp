/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
 * \file x86-backend.cpp
 * \brief implementation o the function exported by the class x86Backend
 */


#include "x86-backend.h"
#include "x86-asm.h"
#include "x86-emit.h"
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
#include "x86-regalloc.h"
#include "inssel-ia32.h"
#include "insselector.h"

#include <string>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

#define _DEBUG_X86_BACKEND



using namespace jit;
using namespace ia32;
using namespace opt;
using namespace std;

static void addPrologue(CFG<x86Instruction>& cfg, GCRegAlloc<CFG<x86Instruction> >& regAlloc);
static void addEpilogue(CFG<x86Instruction>& cfg, GCRegAlloc<CFG<x86Instruction> >& regAlloc);

x86TargetDriver::x86TargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
:
	TargetDriver(netvm, RTObj, options)
{
}

TargetDriver* x86_getTargetDriver(nvmNetVM* netvm, nvmRuntimeEnvironment* RTObj, TargetOptions* options)
{
	return new x86TargetDriver(netvm, RTObj, options);
}

void x86TargetDriver::init(CFG<MIRNode>& cfg)
{
	algorithms.push_back(new CFGEdgeSplitter<MIRNode>(cfg));

	//jump to jump elimination ?

#ifdef _DEBUG_CFG_BUILD
	{
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, NonSSAPrinter<MIRNode> >(cfg);
		ostringstream print_name;
		print_name << "Dopo lo split dei critical edge " << cfg.getName();
		string outstring("stdout");
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/outstring, codeprinter, print_name.str()));
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
		algorithms.push_back(new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	}
#endif

	if(options->OptLevel > 0)
	{
//#ifdef ENABLE_COMPILER_PROFILING
		cout << "NetVM Optimizer enabled" << endl;
//#endif
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
//#ifdef ENABLE_COMPILER_PROFILING
		cout << "NetVM Optimizer disabled" << endl;
//#endif
		algorithms.push_back(new CanonicalizationStep<CFG<MIRNode> >(cfg));
	}


	if( nvmFLAG_ISSET(options->Flags, nvmDO_BCHECK ) && (options->OptLevel > 1) )
	{
		algorithms.push_back(new Boundscheck_remover< CFG<MIRNode> >(cfg, options));
	
		#ifdef _DEBUG_SSA
	
		ostringstream print_name;
		print_name << "Uscita da Boundscheck_remover " << cfg.getName() << std::endl;
		CFGPrinter<MIRNode>* codeprinter = new CodePrint<MIRNode, SSAPrinter<MIRNode> >(cfg);
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
	
		#endif
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

	algorithms.push_back(new BasicBlockElimination< CFG<MIRNode> >(cfg));
	x86Checker* checker = new x86Checker();
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
		algorithms.push_back( new DoPrint<MIRNode>(/*cout,*/string("stdout"), codeprinter, print_name.str()));
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

GenericBackend* x86TargetDriver::get_genericBackend(CFG<MIRNode>& cfg)
{
	return new x86Backend(cfg);
}

x86Checker::x86Checker()
{
	copro_space = Application::getCoprocessorRegSpace();
}

bool x86Checker::operator()(RegType& a, RegType& b)
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
x86Backend::x86Backend(CFG<MIRNode>& cfg)
: MLcfg(cfg), LLcfg(cfg.getName()),
  code_created(false), buffer(NULL),
  trace_builder(LLcfg) {}

x86Backend::~x86Backend() {
}

static void	init_machine_registers(std::list<RegisterInstance>& machineRegisters)
{
	static const x86Regs_32 precolored[] = {ESP, EBP};
	static const int n = 2;

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
	static const x86Regs_32 avaible[] = {EAX, EBX, ECX, EDX, ESI, EDI};
	static const int n = 6;

	for(int i = 0; i < n; i++)
	{
		colors.push_back(RegisterInstance(MACH_SPACE, (uint32_t)avaible[i]));
	}
}

static void	rename_regs(std::list<RegisterInstance>& virtualRegisters, GCRegAlloc<CFG<x86Instruction> >& regAlloc)
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
static bool is_useless(x86Instruction *actual, x86Instruction *prev)
{
	assert(actual != NULL && prev != NULL);
	if(actual->getOpcode() != prev->getOpcode() && actual->getOpcode() != X86_MOV)
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

static void delete_copy_pair(jit::CFG<x86Instruction> &cfg)
{
	typedef std::list<BasicBlock<x86Instruction> *> bb_list_t;
	typedef bb_list_t::iterator bb_list_it_t;
	typedef std::list<x86Instruction*> code_list_t;
	typedef code_list_t::iterator code_list_it_t;

	x86Instruction *prev_stmt;

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
#ifdef _DEBUG_X86_BACKEND
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

x86TraceBuilder::x86TraceBuilder(CFG<x86Instruction>& cfg)
:
	TraceBuilder<CFG<x86Instruction> >(cfg)
{
}

void x86TraceBuilder::handle_no_succ_bb(bb_t *bb)
{
	assert(bb != NULL && bb->getId() == BasicBlock<x86Instruction>::EXIT_BB);
	return;
}

void x86TraceBuilder::handle_one_succ_bb(bb_t *bb)
{
	assert(bb != NULL);

	bb_t* next = bb->getProperty< bb_t* >(next_prop_name);
	std::list< CFG<x86Instruction>::GraphNode* > successors(bb->getSuccessors());

	bb_t *target = successors.front()->NodeInfo;

	if(bb->getCode().size() == 0)
	{
		//empty BB should be eliminated from the cfg!!!
		//for now emit a jump to the target if necessary
		if(!next || next->getId() != target->getId())
		{
			x86_Asm_JMP_Label(bb->getCode(), target->getId());
		}
	}
	else
	{
		x86Instruction* insn = bb->getCode().back();
		uint16_t opcode = OP_ONLY(insn->getOpcode());

		//if not followed by next bb in emission
		if(!next || next->getId() != target->getId())
		{
			//if has not explicit jump
			if(opcode != X86_JMP && opcode != X86_J && opcode != X86_JECXZ)
			{
				//add the jump
				x86_Asm_JMP_Label(bb->getCode(), target->getId());
			}
		}
		else
		{
			//we can remove last useless jump
			if(opcode == X86_JMP)
			{
				bb->getCode().pop_back();
				delete insn;
			}
		}
	}
	return;
}

void x86TraceBuilder::handle_two_succ_bb(bb_t *bb)
{
	assert(bb != NULL);

	bb_t* next = bb->getProperty< bb_t* >(TraceBuilder<CFG<x86Instruction> >::next_prop_name);
	std::list< CFG<x86Instruction>::GraphNode* > successors(bb->getSuccessors());

	x86Instruction* insn = bb->getCode().back();

	uint16_t jt = LBL_OPERAND(insn->Operands[0].Op)->Label;
	successors.remove(cfg.getBBById(jt)->getNodePtr());
	uint16_t jf = successors.front()->NodeInfo->getId();


	if(!next || (next->getId() != jt && next->getId() != jf))
	{
		x86_Asm_JMP_Label(bb->getCode(), jf);
		return;
	}

	if(insn->getOpcode() == X86_JMP)
		return;

	if(jt == next->getId())
	{
		//invert instruction
		uint8_t cc = CC_ONLY(insn->getOpcode());

		cc += (cc % 2 == 0 ? 1 : -1);

		LBL_OPERAND(insn->Operands[0].Op)->Label = jf;
		insn->setOpcode(OP_CC(X86_J, cc));
	}

	//else bb is followed by is false target so it's correct
}

bool jit::ia32::x86Backend::create_code()
{
	if(code_created)
		return true;
	{
		CFGCopy<MIRNode,x86Instruction> copier(MLcfg, LLcfg);
		copier.buildCFG();
	}

	list<RegisterInstance> machineRegisters;
	list<RegisterInstance> virtualRegisters;
	list<RegisterInstance> colors;

	#ifdef _DEBUG_X86_BACKEND
	{
		ostringstream filename;
		filename << "cfg_copy_" << LLcfg.getName() << ".dot";

		ofstream dot(filename.str().c_str());
		DotPrint<x86Instruction> printer(LLcfg);
		dot << printer;
		dot.close();
	}
	#endif

	{
		x86_offsets.init();
		base_manager.reset();
		dim_manager.reset();
		IA32InsSelector IAsel;
		InsSelector<IA32InsSelector, x86Instruction> selector(IAsel,MLcfg, LLcfg);
		selector.instruction_selection(MB_NTERM_stmt);
	}

	#ifdef _DEBUG_X86_BACKEND
	{
		ostringstream codefilename;
		codefilename << "cfg_x86code_" << LLcfg.getName() << ".txt";

		ofstream code(codefilename.str().c_str());
		CodePrint<x86Instruction> codeprinter(LLcfg);
		code << codeprinter;
		code.close();
	}
	#endif

	init_machine_registers(machineRegisters);
	init_virtual_registers(virtualRegisters);
	init_colors(colors);

	{
	jit::opt::KillRedundantCopy< CFG<x86Instruction> > krc(LLcfg);
	krc.run();
	}

	x86RegSpiller regSp(LLcfg);
	GCRegAlloc<jit::CFG<x86Instruction> > regAlloc(LLcfg, virtualRegisters, machineRegisters, colors, regSp);

	#ifdef ENABLE_COMPILER_PROFILING
	std::cout << "numero di registri prima della regalloc in "<< LLcfg.getName() << ": " << virtualRegisters.size() << std::endl;
	#endif

	if(!regAlloc.run())
		throw "register allocation failed\n";

	#ifdef ENABLE_COMPILER_PROFILING
	std::cout << "numero di registri spillati in " << LLcfg.getName() << ": " << regSp.getNumSpilledReg() << std::endl;
	#endif

	rename_regs(virtualRegisters, regAlloc);


	#ifdef _DEBUG_X86_BACKEND
	{
		ostringstream codefilename2;
		codefilename2 << "cfg_regalloc_x86code_" << LLcfg.getName() << ".txt";

		ofstream code2(codefilename2.str().c_str());
		CodePrint<x86Instruction> codeprinter2(LLcfg);
		code2 << codeprinter2;
		code2.close();
	}
	#endif

	{
	jit::opt::KillRedundantCopy< CFG<x86Instruction> > krc(LLcfg);
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

	x86_Emitter emitter(LLcfg, regAlloc, trace_builder);
	buffer = emitter.emit();
	actual_buff_sz = emitter.getActualBufferSize();
	code_created = true;
	return true;
}

static void addPrologue(CFG<x86Instruction>& cfg, GCRegAlloc<CFG<x86Instruction> >& regAlloc)
{
	uint32_t calleeSaveMask[X86_NUM_REGS] = X86_CALLEE_SAVE_REGS_MASK;
	uint32_t i;

	BasicBlock<x86Instruction>* entry = cfg.getBBById(BasicBlock<x86Instruction>::ENTRY_BB);
	list<x86Instruction*> code;

	x86_Asm_Comment(code, "function prologue");

	x86_Asm_Op_Reg(code, X86_PUSH, X86_MACH_REG(EBP), x86_DWORD);
	x86_Asm_Op_Reg_To_Reg(code, X86_MOV, X86_MACH_REG(ESP), X86_MACH_REG(EBP), x86_DWORD);

	uint32_t numSpilled = regAlloc.getNumSpilledReg();
	if(numSpilled != 0)
		x86_Asm_Op_Imm_To_Reg(code, X86_SUB, numSpilled * 4, X86_MACH_REG(ESP), x86_DWORD);

	for (i = 0; i < X86_NUM_REGS; i++){
		if (calleeSaveMask[i] && regAlloc.isAllocated(X86_MACH_REG(i))){
			x86_Asm_Op_Reg(code, X86_PUSH, X86_MACH_REG(i), x86_DWORD);
		}
	}

#ifdef JIT_RTE_PROFILE_COUNTERS
	x86_Asm_Comment(code, "profiling instructions");
	x86_Asm_Op_Imm_To_Reg(code, X86_SUB, 8, X86_MACH_REG(ESP), x86_DWORD);

	nvmHandlerState *HandlerState = Application::getApp(entry->getId()).getCurrentPEHandler()->HandlerState;
	uint64_t* addr = &HandlerState->ProfCounters->TicksDelta;

	x86_Asm_Comment(code, "Delta ticks");
	x86_Asm_Op(code, X86_RDTSC);
	x86_Asm_Op_Reg_To_Mem_Base(code, X86_MOV, X86_MACH_REG(EAX), X86_MACH_REG(ESP), 0, x86_DWORD);
	x86_Asm_Op_Reg_To_Mem_Base(code, X86_MOV, X86_MACH_REG(EDX), X86_MACH_REG(ESP), 4, x86_DWORD);
	x86_Asm_Op(code, X86_RDTSC);
	x86_Asm_Op_Mem_Base_To_Reg(code, X86_SUB, X86_MACH_REG(ESP), 0, X86_MACH_REG(EAX), x86_DWORD);
	x86_Asm_Op_Mem_Base_To_Reg(code, X86_SBB, X86_MACH_REG(ESP), 4, X86_MACH_REG(EDX), x86_DWORD);
	x86_Asm_Op_Reg_To_Mem_Displ(code, X86_MOV, X86_MACH_REG(EAX), (uint32_t)addr, x86_DWORD);
	x86_Asm_Op_Reg_To_Mem_Displ(code, X86_MOV, X86_MACH_REG(EDX), (uint32_t)(addr) + 4, x86_DWORD);

	x86_Asm_Comment(code, "Now sample starting time");
	x86_Asm_Op(code, X86_RDTSC);
	x86_Asm_Op_Reg_To_Mem_Base(code, X86_MOV, X86_MACH_REG(EAX), X86_MACH_REG(ESP), 0, x86_DWORD);
	x86_Asm_Op_Reg_To_Mem_Base(code, X86_MOV, X86_MACH_REG(EDX), X86_MACH_REG(ESP), 4, x86_DWORD);
#endif

#ifdef _DEBUG_X86_CODE
	x86_Asm_Op_Imm(code, X86_PUSH, (uint32_t)Application::getApp(0).getCurrentSegment()->Name);
	x86_Asm_Op_Imm(code, X86_CALL, (uint32_t) puts);
	x86_Asm_Op_Imm(code, X86_POP, X86_MACH_REG(EAX));
#endif
	x86_Asm_Comment(code, "function prologue ends");

	list<x86Instruction*>& old_code = entry->getCode();

	old_code.insert( old_code.begin(), code.begin(), code.end());
}

static void addEpilogue(CFG<x86Instruction>& cfg, GCRegAlloc<CFG<x86Instruction> >& regAlloc)
{
	uint32_t calleeSaveMask[X86_NUM_REGS] = X86_CALLEE_SAVE_REGS_MASK;
	int32_t i;

	BasicBlock<x86Instruction>* exit = cfg.getBBById(BasicBlock<x86Instruction>::EXIT_BB);
	list<x86Instruction*>& code = exit->getCode();

	x86_Asm_Comment(code, "function epilogue");

#ifdef JIT_RTE_PROFILE_COUNTERS
	x86_Asm_Op_Imm_To_Reg(code, X86_ADD, 8, X86_MACH_REG(ESP), x86_DWORD);
#endif

	for (i = X86_NUM_REGS - 1; i >= 0; i--){
		if (calleeSaveMask[i] && regAlloc.isAllocated(X86_MACH_REG(i)))
			x86_Asm_Op_Reg(code, X86_POP, X86_MACH_REG(i), x86_DWORD);
	}

	uint32_t numSpilled = regAlloc.getNumSpilledReg();
	if(numSpilled != 0)
		x86_Asm_Op_Imm_To_Reg(code, X86_ADD, numSpilled * 4, X86_MACH_REG(ESP), x86_DWORD);

	x86_Asm_Op_Reg_To_Reg(code, X86_MOV, X86_MACH_REG(EBP), X86_MACH_REG(ESP), x86_DWORD);
	x86_Asm_Op_Reg(code, X86_POP, X86_MACH_REG(EBP), x86_DWORD);
	x86_Asm_Op(code, X86_RET);

}

uint8_t* x86Backend::emitNativeFunction()
{
	if(!create_code())
		return NULL;

	return buffer;
}

void ia32::x86Backend::emitNativeAssembly(std::string prefix)
{
	//printf("Emitting native assembly:\n");
	if(!create_code())
		return;

	string filename(prefix + LLcfg.getName() + ".s");

	ofstream os(filename.c_str());

	emitNativeAssembly(os);
	os.close();
}



void ia32::x86Backend::emitNativeAssembly(std::ostream &os)
{
	//printf("Emitting native assembly:\n");
	if(!create_code())
		return;


//#ifdef OLD_X86_OUTPUT
//	for(TraceBuilder<CFG<x86Instruction> >::trace_iterator_t t = trace_builder.begin();
//		t != trace_builder.end();
//		t++)
//	{
//		if((*t)->getCode().empty())
//			continue;
//
//		os << BBPrinter<x86Instruction, NormalPrinter<x86Instruction> >(**t);
//		//if((*t)->getId() == BasicBlock<x86Instruction>::EXIT_BB)
//		//{
//		//	os << "jump to function epilogue\n";
//		//}
//	}
//#else

	//Invoke the disassembler...
	ud_t ud_obj;

	ud_init(&ud_obj);
	ud_set_input_buffer(&ud_obj, buffer, actual_buff_sz);
	//ud_set_input_file(&ud_obj, stdin);
	ud_set_mode(&ud_obj, 32);
	ud_set_syntax(&ud_obj, UD_SYN_INTEL);

	//printf("binary disassembly (still experimental):\n");
	while (ud_disassemble(&ud_obj)) {
		os << setw(8) << setfill('0') << hex << ud_insn_off(&ud_obj) << "  ";
		os << ud_insn_asm(&ud_obj) << endl;
		//printf("%08x  ", ud_insn_off(&ud_obj));
		//printf("%s\n", ud_insn_asm(&ud_obj));
	}
//#endif

}
