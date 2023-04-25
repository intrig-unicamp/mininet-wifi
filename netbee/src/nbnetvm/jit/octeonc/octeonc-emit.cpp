/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include <nbnetvm.h>
#include "int_structs.h"
#include "octeonc-emit.h"
#include "octeonc-asm.h"
#include "application.h"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace jit;
using namespace octeonc;
using namespace std;


octeoncEmitter::octeoncEmitter(
octeoncCFG& cfg, TraceBuilder<jit::CFG<octeonc_Insn> >& trace_builder)
:
	cfg(cfg),
	trace_builder(trace_builder)
{
	function_name = getFunctionName(cfg);
}

void octeoncEmitter::emit_bb(ostream& os, BasicBlock<octeonc_Insn>* bb)
{
	os << "\nL" << bb->getId() <<  ":" << endl;
    os << "\t{\n"; 

	list<octeonc_Insn *>& code(bb->getCode());

	list<octeonc_Insn *>::iterator i;
	for(i = code.begin(); i != code.end(); i++)
	{
		os << "\t" << **i << endl;
	}

    os << "\t}\n";
}

void octeoncEmitter::emit(string filename)
{
	ofstream os(filename.c_str());
	emit(os);
	os.close();
}


void octeoncEmitter::emit(std::ostream &os)
{
	emit_prologue(os);

	for(TraceBuilder<CFG<octeonc_Insn> >::trace_iterator_t 
        i = trace_builder.begin();
		i != trace_builder.end(); i++)
	{
		emit_bb( os, *i );
	}

	emit_epilogue(os);
}

void octeoncEmitter::emit_prologue(ostream& os)
{
	nvmNetPE* pe = Application::getCurrentPEHandler()->OwnerPE;
	tmp_nvmPEState* pe_state = pe->PEState;

    // include common header
    os << "#include \"octeonc_precompile.h\"\n";

	if(Application::getCurrentSegmentType() == INIT_SEGMENT)
	{
        // include for coprocessor functions
        os << "#include \"octeon_coprocessors.h\"\n";
		// allocate data memory
        os << "uint8_t " << pe->Name << "_data[" 
           << pe->DataMemSize << "];" << endl;

		// allocate memory for coprocessors structures
		for(uint32_t i = 0; i < pe_state->NCoprocRefs; i++)
		{
			char* name = pe_state->CoprocTable[i].name;

            os << "nvmCoprocessorState "
               << pe->Name << "_" << name << ";" << endl;
		}
	}

    // function definition
	os << "int32_t " << get_function_name() 
       << "(nvmExchangeBuffer** exbuf, uint32_t port, nvmHandlerState* dummy){" 
       << endl;

    // all register declaration at the top of the function
    uint32_t maxreg = RegisterModel::get_latest_name(octeoncRegType::VirtSpace);
    for (uint32_t i = 0; i <= maxreg; ++i) {
        octeoncRegType reg(octeoncRegType::VirtSpace, i);
        os << "\tint32_t " << reg << ";\n" ;
    }

	if(Application::getCurrentSegmentType() == INIT_SEGMENT)
	{
		// call create_copro for every coprocessor
		for(uint32_t i = 0; i < pe_state->NCoprocRefs; i++)
		{
			uint32_t index = pe->Copros[i];
			char* name = pe_state->CoprocTable[i].name;
            os << "\tnvm_copro_map[" << index << "].copro_create(&"
               << pe->Name << "_" << name << ");" << endl;
		}
	}
}

void octeoncEmitter::emit_epilogue(ostream& os)
{
    if(Application::getCurrentSegmentType() != INIT_SEGMENT) {
        os << "\tnvmDropPacket(exbuf, 0, 0);\n";
    }
    os << "\treturn 0;"
          "\n}";
}
