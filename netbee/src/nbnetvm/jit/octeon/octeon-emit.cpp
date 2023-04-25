/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include <nbnetvm.h>
#include "int_structs.h"
#include "octeon-emit.h"
#include "octeon-asm.h"
#include "application.h"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace jit;
using namespace octeon;
using namespace std;

const uint32_t jit::octeon::octeonEmitter::stack_space = 48;

octeonEmitter::octeonEmitter(octeonCFG& cfg, GCRegAlloc<jit::CFG<octeon_Insn> >& regAlloc, TraceBuilder<jit::CFG<octeon_Insn> >& trace_builder)
:
	cfg(cfg),
	regAlloc(regAlloc),
	trace_builder(trace_builder)
{
	function_name = getFunctionName(cfg);
}

void octeonEmitter::emit_bb(ostream& os, BasicBlock<octeon_Insn>* bb)
{
	os << ".L" << bb->getId() <<  ":" << endl;

	list<octeon_Insn *>& code(bb->getCode());

	list<octeon_Insn *>::iterator i;
	for(i = code.begin(); i != code.end(); i++)
	{
		os << "\t" << **i << endl;
	}
}

void octeonEmitter::emit(string filename)
{
	ofstream os(filename.c_str());
	emit(os);
	os.close();
}


void octeonEmitter::emit(std::ostream &os)
{
	emit_prologue(os);

	for(TraceBuilder<CFG<octeon_Insn> >::trace_iterator_t i = trace_builder.begin();
		i != trace_builder.end(); i++)
	{
		emit_bb( os, *i );
	}

	emit_epilogue(os);
}

void octeonEmitter::emit_jump_tables(std::ostream& os)
{
	octeonCFG::jump_table_list_t::const_iterator i;
	const octeonCFG::jump_table_list_t& tables(cfg.getJumpTables());

	os << ".section\t.data.rel,\"aw\",@progbits" << endl;
	os << ".align 3" << endl;
	for(i = tables.begin(); i != tables.end(); i++)
	{
		(*i)->print(os) << endl;
	}
}

/*!
 * \brief emit function prologue
 *
 * function prologue looks like this
 * <code>
 * .text
 * .align 2
 * .globl functionname
 * .ent functionname
 * .type functionname, @function
 * functionname:
 *	daddi	$sp, $sp, -56
 *	sd		$31, 48($sp) #$ra
 *	sd		$sp, 32($sp)
 *	sd		$28, 24($sp) #$gp
 *	sd		$4,  16($sp)#save a2
 *	sd		$4,   8($sp)#save a1
 *	sd		$4,   0($sp)#save a0
 *	move	$fp, $sp #prologo della funzione
 *	</code>
 */

uint32_t octeonEmitter::get_stack_space()
{
	uint32_t space = (stack_space + 24 + 8 * regAlloc.getNumSpilledReg());
#ifdef RTE_PROFILE_COUNTERS
	if(Application::getCurrentSegmentType() == PUSH_SEGMENT) space += 8;
#endif
	return space;
}

void octeonEmitter::emit_prologue(ostream& os)
{
	nvmNetPE* pe = Application::getCurrentPEHandler()->OwnerPE;
	tmp_nvmPEState* pe_state = pe->PEState;

	emit_jump_tables(os);

	os << ".text" << endl;
	os << ".align 2" << endl;

	if(Application::getCurrentSegmentType() == INIT_SEGMENT)
	{
		//allocate data memory
		os << ".comm " << pe->Name << "_data, " << pe->DataMemSize << ", 8" << endl;

		//allocate memory for coprocessors structures
		for(uint32_t i = 0; i < pe_state->NCoprocRefs; i++)
		{
			char* name = pe_state->CoprocTable[i].name;
			os << ".comm " << pe->Name << "_" << name << ", " << offsets.CoproStateSize << ", 8" << endl;
		}
	}

	os << ".globl " << function_name << endl;
	os << ".ent "   << function_name << endl;
	os << ".type "  << function_name << ", @function" << endl << endl;
	os << ".set noat" << endl;
	os << get_function_name() << ":" << endl;

	os << "\tdaddiu $sp, $sp, -" << get_stack_space() << endl;

#ifdef RTE_PROFILE_COUNTERS
	if(Application::getCurrentSegmentType() == PUSH_SEGMENT)
		os << "\tsd $0, 48($sp) #profiling" << endl;
#endif
 	os << "\tsd $31, 40($sp) #$ra" << endl;
 	os << "\tsd $fp, 32($sp) #$fp" << endl;
 	os << "\tsd $28, 24($sp) #$gp" << endl;
 	os << "\tsd $6,  16($sp) #save a2" << endl;
 	os << "\tsd $5,   8($sp) #save a1" << endl;
 	os << "\tsd $4,   0($sp) #save a0" << endl;
	os << "\tlui $28, %hi(%neg(%gp_rel("<< function_name << ")))\n";
	os << "\tdaddu $28, $28, $25\n";
	os << "\tdaddiu $28, $28,%lo(%neg(%gp_rel("<< function_name <<")))\n";

	if(Application::getCurrentSegmentType() == INIT_SEGMENT)
	{
		//call create_copro for every coprocessor
		for(uint32_t i = 0; i < pe_state->NCoprocRefs; i++)
		{
			char* name = pe_state->CoprocTable[i].name;
			uint32_t index = pe->Copros[i];
			os << "ld $4, %got_disp(" << pe->Name << "_" << name << ")($28)" << endl;
			os << "ld $25, %got_disp(nvm_copro_map)($28)" << endl;
			os << "daddiu $25, $25, "<< offsets.octeon_copro_map_entry.size * index << endl;
			os << "ld $25, "<< offsets.octeon_copro_map_entry.create << "($25)" << endl;
			os << "jal $25" << endl;
			os << "nop" << endl;
		}
	}

#ifdef RTE_PROFILE_COUNTERS
	octeonStartTicks st(pe->Name);
	st.print(os);
#endif
}

/*!
 * \brief emit function epilogue
 *
 * function epilogue looks like this
 * <code>
 * move 	$sp, $fp
 * ld		$31, 32($sp) #$ra
 * ld		$sp, 24($sp)
 * ld		$28, 16($sp) #$gp
 * daddiu	$sp, $sp, 48
 * j		$31
 * nop
 * .end functionname
 * </code>
 */
void octeonEmitter::emit_epilogue(ostream& os)
{
  os << ".Lepilogue:" << endl;
  os << "\tld	$31, 40($sp) #$ra" << endl;
  os << "\tld	$fp, 32($sp) #$fp" << endl;
  os << "\tld	$28, 24($sp) #$gp" << endl;
  os << "\tdaddiu	$sp, $sp, " << get_stack_space() <<endl;
  os << "\tj $31" << endl;
  os << ".end " << function_name << endl;
}
