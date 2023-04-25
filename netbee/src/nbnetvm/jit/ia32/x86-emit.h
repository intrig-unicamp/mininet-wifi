/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#ifndef __X86_EMIT_FUNCTIONS_H__
#define __X86_EMIT_FUNCTIONS_H__

#include "basicblock.h"
#include "irnode.h"
#include "cfg.h"
#include "x86-asm.h"
#include "tracebuilder.h"
#include "gc_regalloc.h"

/** @file x86-emit.h
 * \brief This file contains the prototypes of the functions that the Jit uses to emit x86 code in memory
 * 
 */

namespace jit{
namespace ia32{

	//!a class wich emits into a buffer a cfg of x86Instruction
	class x86_Emitter
	{
		public:
		typedef BasicBlock<x86Instruction> bb_t;

		struct patch_info;

		//!this function emits the code to a buffer
		uint8_t* emit();

		/*!
		 * \brief contructor
		 * \param cfg The cfg to emit
		 */
		x86_Emitter(CFG<x86Instruction>& cfg, GCRegAlloc<CFG<x86Instruction> >& regAlloc, TraceBuilder<jit::CFG<x86Instruction> >& trace_builder);
		
		/*!
		 * \brief this function gets the actual size of the emitted code buffer
		 * \return the size of the emitted binary code
		 */

		uint32_t getActualBufferSize(void);
		
		private:

		static const std::string prop_name; //!<holds the name of the property in bb of the address of emission in memory

		/*!
		 * \brief map some space for generation code
		 */
		void* allocCodePages(size_t size);
		
		/*!
		 * \brief enable/disable page protection
		 */
		void setPageProtection(void *address, size_t size, bool executableFlag, bool writeableFlag);
		
		/*!
		 * \brief emits function prologue in buffer
		 */
		void emit_prologue();

		/*!
		 * \brief emit function prologue in buffer
		 */
		void emit_epilogue();

		/*!
		 * \brief recursive function that emits all the BB graph
		 * \param bb pointer to the current bb to emit
		 */
		void emitBB(BasicBlock<x86Instruction> *bb);

		/*!
		 * \brief add a jump to the list of instruction to patch
		 * \param insn pointer to the instruction to patch
		 * \param emission_address address in the buffer where the instruction is emitted
		 */
		void add_jump(x86Instruction* insn, uint8_t* emission_address);

		/*!
		 * \brief add a jump to the list of instruction to patch
		 * \param opcode opcode of the instruction to patch
		 * \param emission_address address in the buffer where the instruction is emitted
		 * \param destination_id id of the destination BasicBlock
		 */
		void add_jump(x86OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id);

		/*!
		 * \brief add a jump to the list of instruction to patch
		 * \param insn instruction of the entry to patch
		 * \param emission_address address in the buffer where the instruction is emitted
		 */
		void add_entry(x86Instruction* insn, uint8_t* emission_address);

		/*!
		 * \brief emit the last jump from the exitBB to the function epilogue
		 * \param emission_address address in the buffer when the instruction is emitted
		 */
		void add_last_jump(uint8_t* emission_address);

		/*!
		 * \brief function thath emits an instruction in memory
		 * \param insn pointer to the instruction to emit
		 */
		void emit_insn(x86Instruction* insn);

		/*!
		 * \brief patch a jump instruction
		 * \param pinfo refence to the information for patching
		 */
		void patch_jump(patch_info& pinfo);

		/*!
		 * \brief patch a switch entry
		 * \param pinfo reference to the information for patching
		 */
		void patch_entry(patch_info& pinfo);

		uint8_t *buffer; //!<buffer allocated for the emission
		uint8_t *current; //!<current offset in the buffer
		uint8_t *epilogue; //!<address of the epilogue

		CFG<x86Instruction>& cfg; //!<cfg to emit
		GCRegAlloc<CFG<x86Instruction> >& regAlloc; //!<register allocation information
		TraceBuilder<CFG<x86Instruction> >& trace_builder; //!<traces of the cfg
		std::list<patch_info> jumps; //!<list of instruction and information for patching them 
		std::list<patch_info> entries; //!<list of switch table entries and information for patching them 

		public:

		//!structure to hold information about patching jump target addresses
		struct patch_info
		{
			x86OpCodesEnum opcode; //!<opcode of this jump
			uint8_t *emission_address; //!<address of emission
			uint16_t destination_id; //!<id of the destination bb
			bool last; //!<is this the last jump which must go to the epilogue

			//!construct a patch info for jumps
			patch_info(x86OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id, bool last = false);
			//!construct a patch info for table entries
			patch_info(uint8_t* emission_address, uint16_t destination_id);
		};
	};

	void x86_Emit_ADC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_ADD(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_AND(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_BSF(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_BSR(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_BSWAP(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_BT(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_BTC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_BTR(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_BTS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CALL(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CBW(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CDQ(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CLC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CLD(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CMC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CMOV(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CMP(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CMPS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CMPXHG(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CWD(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_CWDE(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_DEC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_DIV(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_IDIV(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_IMUL(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_INC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_J(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_JECXZ(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_JMP(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_LEA(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_LODS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_LOOP(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_LOOPNZ(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_LOOPZ(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_MOV(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_MOVS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_MOVSX(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_MOVZX(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_MUL(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_NEG(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_NOP(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_NOT(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_INC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_OR(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_POP(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_POPFD(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_PUSH(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_PUSHFD(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_RET(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_ROL(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_ROR(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SAL(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SAR(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SBB(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SCAS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SET(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SHL(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SHR(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_STC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_STD(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_STOS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SUB(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_TEST(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_XADD(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_XCHG(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_XOR(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_SAVEREGS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_LOADREGS(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_RDTSC(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_Sw_Table_Entry(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_Sw_Table_Entry_Start(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_REPE(Px86Instruction insn, uint8_t **codeBuf);
	void x86_Emit_Comment(Px86Instruction insn, uint8_t **codeBuf);

} //namespace ia32
} //namespace jit

#endif
