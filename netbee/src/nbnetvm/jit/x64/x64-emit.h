/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#ifndef __X64_EMIT_FUNCTIONS_H__
#define __X64_EMIT_FUNCTIONS_H__

#include "basicblock.h"
#include "irnode.h"
#include "cfg.h"
#include "x64-asm.h"
#include "tracebuilder.h"
#include "gc_regalloc.h"

/** @file x64-emit.h
 * \brief This file contains the prototypes of the functions that the Jit uses to emit x64 code in memory
 *
 */

namespace jit{
namespace x64{

	//!a class wich emits into a buffer a cfg of x64Instruction
	class x64_Emitter
	{
		public:
		typedef BasicBlock<x64Instruction> bb_t;

		struct patch_info;

		//!this function emits the code to a buffer
		uint8_t* emit();

		/*!
		 * \brief contructor
		 * \param cfg The cfg to emit
		 */
		x64_Emitter(CFG<x64Instruction>& cfg, GCRegAlloc<CFG<x64Instruction> >& regAlloc, TraceBuilder<jit::CFG<x64Instruction> >& trace_builder);

		/*!
		 * \brief this function gets the actual size of the emitted code buffer
		 * \return the size of the emitted binary code
		 */

		uint32_t getActualBufferSize(void);

		private:

		static const std::string prop_name; //!<holds the name of the property in bb of the address of emission in memory

		/*!
		 * \brief enable/disable page protection
		 */
		void setPageProtection(void *address, size_t size, bool executableFlag, bool writeableFlag);

		/*!
		 * \brief map some space for generation code
		 */
		void* allocCodePages(size_t size);

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
		void emitBB(BasicBlock<x64Instruction> *bb);

		/*!
		 * \brief add a jump to the list of instruction to patch
		 * \param insn pointer to the instruction to patch
		 * \param emission_address address in the buffer where the instruction is emitted
		 */
		void add_jump(x64Instruction* insn, uint8_t* emission_address);

		/*!
		 * \brief add a jump to the list of instruction to patch
		 * \param opcode opcode of the instruction to patch
		 * \param emission_address address in the buffer where the instruction is emitted
		 * \param destination_id id of the destination BasicBlock
		 */
		void add_jump(x64OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id);

		/*!
		 * \brief add a jump to the list of instruction to patch
		 * \param insn instruction of the entry to patch
		 * \param emission_address address in the buffer where the instruction is emitted
		 */
		void add_entry(x64Instruction* insn, uint8_t* emission_address);

		/*!
		 * \brief emit the last jump from the exitBB to the function epilogue
		 * \param emission_address address in the buffer when the instruction is emitted
		 */
		void add_last_jump(uint8_t* emission_address);

		/*!
		 * \brief function thath emits an instruction in memory
		 * \param insn pointer to the instruction to emit
		 */
		void emit_insn(x64Instruction* insn,x64Instruction* prev);

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

		CFG<x64Instruction>& cfg; //!<cfg to emit
		GCRegAlloc<CFG<x64Instruction> >& regAlloc; //!<register allocation information
		TraceBuilder<CFG<x64Instruction> >& trace_builder; //!<traces of the cfg
		std::list<patch_info> jumps; //!<list of instruction and information for patching them
		std::list<patch_info> entries; //!<list of switch table entries and information for patching them

		public:

		//!structure to hold information about patching jump target addresses
		struct patch_info
		{
			x64OpCodesEnum opcode; //!<opcode of this jump
			uint8_t *emission_address; //!<address of emission
			uint16_t destination_id; //!<id of the destination bb
			bool last; //!<is this the last jump which must go to the epilogue

			//!construct a patch info for jumps
			patch_info(x64OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id, bool last = false);
			//!construct a patch info for table entries
			patch_info(uint8_t* emission_address, uint16_t destination_id);
		};
	};

	void x64_Emit_ADC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_ADD(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_AND(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_BSF(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_BSR(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_BSWAP(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_BT(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_BTC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_BTR(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_BTS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CALL(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CBW(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CDQ(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CLC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CLD(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CMC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CMOV(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CMP(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CMPS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CMPXHG(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CWD(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_CWDE(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_DEC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_DIV(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_IDIV(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_IMUL(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_INC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_J(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_JECXZ(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_JMP(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_LEA(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_LODS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_LOOP(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_LOOPNZ(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_LOOPZ(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_MOV(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_MOVS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_MOVSX(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_MOVZX(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_MOVQ(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_MUL(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_NEG(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_NOP(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_NOT(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_INC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_OR(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_POP(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_POPFD(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_PUSH(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_PUSHFD(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_RET(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_ROL(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_ROR(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SAL(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SAR(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SBB(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SCAS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SET(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SHL(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SHR(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_STC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_STD(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_STOS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SUB(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_TEST(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_XADD(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_XCHG(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_XOR(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_SAVEREGS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_LOADREGS(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_RDTSC(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_Sw_Table_Entry(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_Sw_Table_Entry_Start(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_REPE(Px64Instruction insn, uint8_t **codeBuf);
	void x64_Emit_Comment(Px64Instruction insn, uint8_t **codeBuf);

} //namespace x64
} //namespace jit

#endif
