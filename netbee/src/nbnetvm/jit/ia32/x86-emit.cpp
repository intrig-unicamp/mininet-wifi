/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





/** @file x86-emit.cpp
 * \brief This file contains the functions that emit x86 code in memory
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <application.h>
#include "x86-asm.h"
#include "x86-emit.h"
#include "netvmjitglobals.h"
#include "../../../nbee/globals/debug.h"
#include "tracebuilder.h"
//#include "application.h"

#ifndef WIN32
// page protection management (only unix)
#include <sys/mman.h>
#include <unistd.h>
#endif


#define MAX_BYTES_PER_INSN 10

//Masks for the ModR/M byte:
#define MOD_MASK	0x03	//2 bits	Mod field
#define REG_MASK	0x07	//3 bits	Reg/Opcode field
#define RM_MASK		0x07	//3 bits	R/M field

//Masks for the SIB (scale/index/base) byte:
#define SCALE_MASK	0x03	//2 bits	Scale field
#define INDEX_MASK	0x07	//3 bits	Index field
#define BASE_MASK	0x07	//3 bits	Base field

//Defs for the mod field in the ModR/M byte
#define BASE_ONLY	0x00
#define BASE_DISP8	0x01
#define BASE_DISP32	0x02
#define REG_ONLY	0x03

#define REG_NAME get_model()->get_name()
#define REG_OPERAND_NAME(INSN, INDEX) (REG_OPERAND((INSN)->Operands[(INDEX)].Op)->REG_NAME)

#undef NETVM_ASSERT
#define NETVM_ASSERT(A,B) assert(A && B);

namespace jit{
	namespace ia32{


		//Alu operation opcodes
		typedef enum
		{
			X86_ALU_ADD = 0,
			X86_ALU_OR  = 1,
			X86_ALU_ADC = 2,
			X86_ALU_SBB = 3,
			X86_ALU_AND = 4,
			X86_ALU_SUB = 5,
			X86_ALU_XOR = 6,
			X86_ALU_CMP = 7
		} x86AluOpEnum;


		//Shift operation opcodes
		typedef enum
		{
			X86_SHF_ROL = 0,
			X86_SHF_ROR = 1,
			X86_SHF_RCL = 2,
			X86_SHF_RCR = 3,
			X86_SHF_SHL = 4,
			X86_SHF_SHR = 5,
			X86_SHF_SAR = 7
		} x86ShiftOpEnum;


		void emit_prof_timer_sampling(uint8_t **codeBuf, uint64_t *counter);

		void emit_inc_pkt_counter(uint8_t **codeBuf, uint32_t *counter);


		uint32_t Is_Imm8(uint32_t immVal)
		{
			if (((int32_t)immVal) >= -128 && ((int32_t)immVal) <= 127)
				return TRUE;
			else
				return FALSE;
		}

		uint32_t Is_Imm16(uint32_t immVal)
		{
			if (((int32_t)immVal) >= -(1 << 16) && ((int32_t)immVal) <= ((1 << 16) - 1))
				return TRUE;
			else
				return FALSE;
		}

		void x86_Emit_Prefix(uint8_t prefix, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = prefix;
		}

		void x86_Emit_Imm32(uint32_t immVal, uint8_t **bufPtr)
		{
			uint8_t valBuf[4];


			(*(uint32_t*)valBuf) = immVal;
			(*(*bufPtr)++) = valBuf[0];
			(*(*bufPtr)++) = valBuf[1];
			(*(*bufPtr)++) = valBuf[2];
			(*(*bufPtr)++) = valBuf[3];
		}


		void x86_Emit_Imm16(uint16_t immVal, uint8_t **bufPtr)
		{
			uint8_t valBuf[2];

			(*(uint16_t*)valBuf) = immVal;
			(*(*bufPtr)++) = valBuf[0];
			(*(*bufPtr)++) = valBuf[1];
		}


		void x86_Emit_Imm8(uint8_t immVal, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = immVal;
		}


		void x86_Emit_ModRM_Byte(uint8_t mod, uint8_t reg, uint8_t rm, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = ((mod & MOD_MASK) << 6) | ((reg & REG_MASK) << 3) | (rm & RM_MASK);
		}


		void x86_Emit_SIB_Byte(uint8_t scale, uint8_t index, uint8_t base, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = ((scale & SCALE_MASK) << 6) | ((index & INDEX_MASK) << 3) | (base & BASE_MASK);
		}


		void x86_Emit_Reg_Reg(uint8_t reg1, uint8_t reg2, uint8_t **bufPtr)
		{
			x86_Emit_ModRM_Byte(REG_ONLY, reg1, reg2, bufPtr);
		}


		void x86_Emit_Reg_Mem_Base(uint8_t reg, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			if (base == ESP){		// Correct addressing needs the SIB byte
				if (displ == 0){
					x86_Emit_ModRM_Byte(0x0, reg, 0x4, bufPtr);
					x86_Emit_SIB_Byte(0x0, 0x04, ESP, bufPtr);
				}
				else if (Is_Imm8(displ)){
					x86_Emit_ModRM_Byte(0x1, reg, 0x4, bufPtr);
					x86_Emit_SIB_Byte(0, 0x04, ESP, bufPtr);
					x86_Emit_Imm8(displ, bufPtr);
				}
				else{
					x86_Emit_ModRM_Byte(0x02, reg, 0x04, bufPtr);
					x86_Emit_SIB_Byte(0, 0x04, ESP, bufPtr);
					x86_Emit_Imm32(displ, bufPtr);
				}
				return;
			}
			if (displ == 0 && base != EBP){
				x86_Emit_ModRM_Byte(0, reg, base, bufPtr);
				return;
			}

			if (Is_Imm8(displ)){
				x86_Emit_ModRM_Byte(0x1, reg, base, bufPtr);
				x86_Emit_Imm8(displ, bufPtr);
			}
			else{
				x86_Emit_ModRM_Byte(0x02, reg, base, bufPtr);
				x86_Emit_Imm32(displ, bufPtr);
			}

		}



		void x86_Emit_Reg_Mem_Index(uint8_t reg, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			if (index != EBP && displ == 0){
				x86_Emit_ModRM_Byte(0, reg, 0x04, bufPtr);
				x86_Emit_SIB_Byte(scale, index, base, bufPtr);
			}
			else if (Is_Imm8(displ)){
				x86_Emit_ModRM_Byte(1, reg, 0x04, bufPtr);
				x86_Emit_SIB_Byte(scale, index, base, bufPtr);
				x86_Emit_Imm8(displ, bufPtr);
			}
			else{
				x86_Emit_ModRM_Byte(2, reg, 0x04, bufPtr);
				x86_Emit_SIB_Byte(scale, index, base, bufPtr);
				x86_Emit_Imm32(displ, bufPtr);
			}

		}

		void x86_Emit_ALU(uint8_t aluOpcode, x86OpndSz operandsSize, uint8_t dir, uint8_t **bufPtr)
		{
			uint8_t opcodeByte = 0;

			/*
			   ALU operations are ADD, OR, ADC, SBB, AND, SUB, XOR, CMP with opcodes in the range 0..7 (3 bits)
			   the more general basic format is the following:

			   00 OPC 0dw

			   where
			 * OPC is the 3-bit ALU opcode
			 * d is the direction bit
			 * w is the size of the operands (0 -> 8 bit, 1 -> 32 bit)
			 */

			if (dir == 1)
				opcodeByte = 2;

			if (operandsSize == x86_DWORD)
				opcodeByte |= 1;

			opcodeByte |= aluOpcode << 3;

			(*(*bufPtr)++) = opcodeByte;

		}


		void x86_Emit_ALU_Reg_Reg(uint8_t aluOpcode, uint8_t src, uint8_t dst, x86OpndSz size, uint8_t **bufPtr)
		{
			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			x86_Emit_ALU(aluOpcode, size, 0, bufPtr);
			x86_Emit_Reg_Reg(src, dst, bufPtr);

		}

		void x86_Emit_ALU_Reg_Displ(uint8_t aluOpcode, uint8_t src, uint32_t displ, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = aluOpcode;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x86_Emit_ModRM_Byte(BASE_ONLY, src, 5, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);
		}

		void x86_Emit_ALU_Displ_Reg(uint8_t aluOpcode, uint32_t displ, uint8_t dst, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = aluOpcode;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x86_Emit_ModRM_Byte(BASE_ONLY, dst, 5, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);
		}

		void x86_Emit_ALU_Imm_Reg(uint8_t aluOpcode, uint32_t immVal, uint8_t dst, uint8_t **bufPtr)
		{
			// We suppose to have always DWORD immediate values to avoid the management of byte/word parts of regs
			// so we put the s bit to 0 (no sign extension) and the w bit to 1 (size = 32 bit)

			(*(*bufPtr)++) = 0x81;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x86_Emit_ModRM_Byte(3, aluOpcode, dst, bufPtr);
			x86_Emit_Imm32(immVal, bufPtr);

		}

		void x86_Emit_ALU_Imm_Displ(uint8_t aluOpcode, x86OpndSz size, uint32_t immVal, uint32_t displ, uint8_t **bufPtr)
		{
			//(*(*bufPtr)++) = aluOpcode;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)


			uint8_t szBit = 0;

			if(size != x86_BYTE)
				szBit = 1;

			if(size == x86_WORD )
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			(*(*bufPtr)++) = 0x80 | szBit;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)

			x86_Emit_ModRM_Byte(BASE_ONLY,  aluOpcode, 5, bufPtr);

			x86_Emit_Imm32(displ, bufPtr);

			if(size == x86_BYTE)
				x86_Emit_Imm8(immVal, bufPtr);
			else if(size == x86_WORD)
				x86_Emit_Imm16(immVal, bufPtr);
			else if(size == x86_DWORD)
				x86_Emit_Imm32(immVal, bufPtr);
		}

		void x86_Emit_ALU_Imm_Mem_Base(uint8_t aluOpcode, x86OpndSz size, uint32_t immVal, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if(size != x86_BYTE)
				szBit = 1;

			if(size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			(*(*bufPtr)++) = 0x80 | szBit;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)

			x86_Emit_Reg_Mem_Base(aluOpcode, base, displ, bufPtr);

			if(size == x86_BYTE)
				x86_Emit_Imm8(immVal, bufPtr);
			else if(size == x86_WORD)
				x86_Emit_Imm16(immVal, bufPtr);
			else if(size == x86_DWORD)
				x86_Emit_Imm32(immVal, bufPtr);
		}



		void x86_Emit_ALU_Reg_Mem_Base(uint8_t aluOpcode,  x86OpndSz size, uint8_t src, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			//direction bit = 0: register to memory

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			x86_Emit_ALU(aluOpcode, size, 0, bufPtr);
			x86_Emit_Reg_Mem_Base(src, base, displ, bufPtr);


		}


		void x86_Emit_ALU_Mem_Base_Reg(uint8_t aluOpcode,  x86OpndSz size, uint8_t dst, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			//direction bit = 1: memory to register

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			x86_Emit_ALU(aluOpcode, size, 1, bufPtr);
			x86_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}


		void x86_Emit_ALU_Imm_Mem_Index(uint8_t aluOpcode, x86OpndSz size, uint32_t immVal, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if(size == x86_WORD || size == x86_DWORD)
				szBit = 1;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			(*(*bufPtr)++) = (0x80 | szBit);	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x86_Emit_Reg_Mem_Index(aluOpcode, base, displ, index, scale, bufPtr);
			switch(size)
			{
				case x86_BYTE:
					x86_Emit_Imm8(immVal, bufPtr);
					break;
				case x86_WORD:
					x86_Emit_Imm16(immVal, bufPtr);
					break;
				case x86_DWORD:
					x86_Emit_Imm32(immVal, bufPtr);
					break;
			}
			//x86_Emit_Imm32(immVal, bufPtr);
		}



		void x86_Emit_ALU_Reg_Mem_Index(uint8_t aluOpcode,  x86OpndSz size, uint8_t src, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			//direction bit = 0: register to memory

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			x86_Emit_ALU(aluOpcode, size, 0, bufPtr);
			x86_Emit_Reg_Mem_Index(src, base, displ, index, scale, bufPtr);


		}


		void x86_Emit_ALU_Mem_Index_Reg(uint8_t aluOpcode,  x86OpndSz size, uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			//direction bit = 1: memory to register

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			x86_Emit_ALU(aluOpcode, size, 1, bufPtr);
			x86_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}



		void x86_Emit_SHIFT_Imm_Reg(x86ShiftOpEnum shiftOpcode, uint32_t immVal, uint8_t dst, uint8_t **bufPtr)
		{
			if (immVal == 0)
				return;

			if (immVal == 1){
				(*(*bufPtr)++) = 0xD1;
				x86_Emit_ModRM_Byte(3, shiftOpcode, dst, bufPtr);
				return;
			}


			(*(*bufPtr)++) = 0xC1;
			x86_Emit_ModRM_Byte(3, shiftOpcode, dst, bufPtr);
			x86_Emit_Imm8(immVal, bufPtr);

		}


		void x86_Emit_SHIFT_CL_Reg(x86ShiftOpEnum shiftOpcode, uint8_t dst, x86OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;

			if (size == x86_WORD){
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x86_DWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			(*(*bufPtr)++) = 0xD2 | szBit;
			x86_Emit_ModRM_Byte(3, shiftOpcode, dst, bufPtr);
		}


		void x86_Emit_SHIFT_Imm_Mem_Base(x86ShiftOpEnum shiftOpcode, uint32_t immVal, uint8_t base, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;



			if (immVal == 0)
				return;

			if (size == x86_WORD){
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x86_DWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			if (immVal == 1){
				(*(*bufPtr)++) = 0xD0 | szBit;
				x86_Emit_Reg_Mem_Base(shiftOpcode, base, displ, bufPtr);
				return;
			}

			(*(*bufPtr)++) = 0xC0 | szBit;
			x86_Emit_Reg_Mem_Base(shiftOpcode, base, displ, bufPtr);
			x86_Emit_Imm8(immVal, bufPtr);
		}





		void x86_Emit_SHIFT_CL_Mem_Base(x86ShiftOpEnum shiftOpcode, uint8_t base, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{

			uint32_t szBit = 0;

			if (size == x86_WORD){
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x86_DWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			(*(*bufPtr)++) = 0xD2 | szBit;
			x86_Emit_Reg_Mem_Base(shiftOpcode, base, displ, bufPtr);
		}



		void x86_Emit_SHIFT_Imm_Mem_Index(x86ShiftOpEnum shiftOpcode, uint32_t immVal, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, x86OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;

			if (immVal == 0)
				return;

			if (size == x86_WORD){
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x86_DWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			if (immVal == 1){
				(*(*bufPtr)++) = 0xD0 | szBit;
				x86_Emit_Reg_Mem_Index(shiftOpcode, base, displ, index, scale, bufPtr);
				return;
			}

			(*(*bufPtr)++) = 0xC0 | szBit;
			x86_Emit_Reg_Mem_Index(shiftOpcode, base, displ, index, scale, bufPtr);
			x86_Emit_Imm8(immVal, bufPtr);
		}



		void x86_Emit_SHIFT_CL_Mem_Index(x86ShiftOpEnum shiftOpcode, uint8_t src, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, x86OpndSz size, uint8_t **bufPtr)
		{

			uint32_t szBit = 0;

			if (size == x86_WORD){
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x86_DWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			(*(*bufPtr)++) = 0xD2 | szBit;
			x86_Emit_Reg_Mem_Index(shiftOpcode, base, displ, index, scale, bufPtr);
		}





		void x86_Emit_BSWAP_Reg(uint8_t reg, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x0F;
			x86_Emit_ModRM_Byte(3, 1, reg, bufPtr);
		}

		void x86_Emit_JMP_Indirect_Mem_Base(uint8_t base, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xFF;
			x86_Emit_ModRM_Byte(BASE_DISP32, 4, base, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);
		}

		void x86_Emit_JMP_Relative32(uint32_t displ, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xE9;
			x86_Emit_Imm32(displ, bufPtr);
		}


		void x86_Emit_J_Relative32(uint8_t cc, uint32_t displ, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0x80 | (cc & 0x0F);		/* [GM] Added parentheses, I hope the correct way ;) */
			x86_Emit_Imm32(displ, bufPtr);
		}

		void x86_Emit_MOV_Reg_Reg(uint8_t src, uint8_t dst, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x86_DWORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x88 | szBit;
			x86_Emit_ModRM_Byte(3, src, dst, bufPtr);

		}


		void x86_Emit_MOV_Imm_Reg(uint32_t immVal, uint8_t dst, uint8_t **bufPtr)
		{
			//we consider only transfer of DWORD values to registers

			(*(*bufPtr)++) = 0xC7;
			x86_Emit_ModRM_Byte(3, 0, dst, bufPtr);
			x86_Emit_Imm32(immVal, bufPtr);

		}

		void x86_Emit_MOV_Imm_Displ(uint32_t imm, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				//x86_Emit_Prefix(OPER_SIZE, bufPtr);

			if (size == x86_DWORD || size == x86_WORD)
				szBit = 1;

			//(*(*bufPtr)++) = 0xC6 | szBit;
			(*(*bufPtr)++) = 0xC7;
			x86_Emit_ModRM_Byte(0, 0, 5, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);
			x86_Emit_Imm32(imm, bufPtr);
		}

		void x86_Emit_MOV_Displ_Reg(uint32_t displ, uint8_t dstReg, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			if (size == x86_DWORD || size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x8A | szBit;
			x86_Emit_ModRM_Byte(0, dstReg, 5, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);
		}


		void x86_Emit_MOV_Reg_Displ(uint8_t srcReg, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			if (size == x86_DWORD || size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x88 | szBit;
			x86_Emit_ModRM_Byte(BASE_ONLY, srcReg, 5, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);
		}

		void x86_Emit_MOV_Imm_Mem_Base(uint32_t immVal, uint8_t base, uint32_t displ,  x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);

			if (size == x86_DWORD || size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0xC6 | szBit;
			x86_Emit_Reg_Mem_Base(0, base, displ, bufPtr);
			if (size == x86_BYTE)
				x86_Emit_Imm8((uint8_t)immVal, bufPtr);
			else if (size == x86_WORD)
				x86_Emit_Imm16((uint16_t)immVal, bufPtr);
			else
				x86_Emit_Imm32(immVal, bufPtr);
		}

		void x86_Emit_MOV_Imm_Mem_Index(uint32_t immVal, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x86_DWORD)
				szBit = 1;

			(*(*bufPtr)++) = 0xC6 | szBit;
			x86_Emit_Reg_Mem_Index(0, base, displ, index, scale, bufPtr);
			if (size == x86_BYTE)
				x86_Emit_Imm8((uint8_t)immVal, bufPtr);
			else if (size == x86_WORD)
				x86_Emit_Imm16((uint16_t)immVal, bufPtr);
			else
				x86_Emit_Imm32(immVal, bufPtr);

		}

		void x86_Emit_MOV_Reg_Mem_Base(uint8_t src, uint8_t base, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
			if (size == x86_DWORD || size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x88 | szBit;	//dir bit = 0  opcode: 1000 100w
			x86_Emit_Reg_Mem_Base(src, base, displ, bufPtr);


		}


		void x86_Emit_MOV_Mem_Base_Reg(uint8_t dst, uint8_t base, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x86_DWORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x8A | szBit;	//dir bit = 0  opcode: 1000 101w
			x86_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}






		void x86_Emit_MOV_Reg_Mem_Index(uint8_t src, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x86_DWORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x88 | szBit;	//dir bit = 0  opcode: 1000 100w
			x86_Emit_Reg_Mem_Index(src, base, displ, index, scale, bufPtr);


		}


		void x86_Emit_MOV_Mem_Index_Reg(uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				x86_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x86_DWORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x8A | szBit;	//dir bit = 0  opcode: 1000 101w
			x86_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}


		void x86_Emit_MOVSX_Displ_Reg(uint32_t displ, uint8_t dstReg, x86OpndSz size, uint8_t **bufPtr)
		{

			uint8_t szBit = 0;

			if (size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x86_Emit_ModRM_Byte(0, dstReg, 5, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);


		}



		void x86_Emit_MOVSX_Reg_Reg(uint8_t src, uint8_t dst, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				szBit = 1;


			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x86_Emit_ModRM_Byte(3, dst, src, bufPtr);

		}


		void x86_Emit_MOVSX_Mem_Base_Reg(uint8_t dst, uint8_t base, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x86_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}




		void x86_Emit_MOVSX_Mem_Index_Reg(uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x86_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}



		void x86_Emit_MOVZX_Reg_Reg(uint8_t src, uint8_t dst, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				szBit = 1;


			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x86_Emit_ModRM_Byte(3, dst, src, bufPtr);

		}

		void x86_Emit_MOVZX_Displ_Reg(uint32_t displ, uint8_t dstReg, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD){
				szBit = 1;
				//x86_Emit_Prefix(OPER_SIZE, bufPtr);
			}


			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x86_Emit_ModRM_Byte(0, dstReg, 5, bufPtr);
			x86_Emit_Imm32(displ, bufPtr);
		}

		void x86_Emit_MOVZX_Mem_Base_Reg(uint8_t dst, uint8_t base, uint32_t displ, x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x86_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}

		void x86_Emit_MOVZX_Mem_Index_Reg(uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x86OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x86_WORD)
				szBit = 1;

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x86_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}



		void x86_Emit_PUSH_Reg(uint8_t src, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x50 | (src & REG_MASK);
		}



		void x86_Emit_PUSH_Mem_Base(uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xFF;
			x86_Emit_Reg_Mem_Base(6, base, displ, bufPtr);
		}


		void x86_Emit_PUSH_Imm(uint32_t immVal, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x68;
			x86_Emit_Imm32(immVal, bufPtr);
		}


		void x86_Emit_POP_Reg(uint8_t src, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x58 | (src & REG_MASK);
		}

		void x86_Emit_CALL_Relative32(uint32_t displ, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xE8;
			x86_Emit_Imm32(displ, bufPtr);

		}


		void x86_Emit_Ret(uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xC3;
		}


		void x86_Emit_NOT_Reg(uint8_t src, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xF7;
			x86_Emit_ModRM_Byte(3, 2, src, bufPtr);
		}

		void x86_Emit_NEG_Reg(uint8_t src, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xF7;
			x86_Emit_ModRM_Byte(3, 3, src, bufPtr);
		}

		void x86_Emit_INC_Reg(uint8_t src, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xFF;
			x86_Emit_ModRM_Byte(3, 0, src, bufPtr);
		}


		void x86_Emit_DEC_Reg(uint8_t src, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xFF;
			// FIXME: this is a long shot, I do not know if it is correct.
			x86_Emit_ModRM_Byte(3, 1, src, bufPtr);
		}


		void x86_emit_RDTSC(uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x0f;
			(*(*bufPtr)++) = 0x31;

		}

		uint32_t Is_Reg_Reg_Op(Px86Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == REG_OP && insn->Operands[1].Type == REG_OP)
				return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Reg_Op(Px86Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == REG_OP && insn->Operands[1].Type == IMM_OP)
				return TRUE;

			return FALSE;
		}

		uint32_t Is_Reg_Displ_Op(Px86Instruction insn)

		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == REG_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & DISPL && !(MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Displ_Op(Px86Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[1].Type == IMM_OP && insn->Operands[0].Type == MEM_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & DISPL && !(MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Displ_Reg_Op(Px86Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[1].Type == MEM_OP && insn->Operands[0].Type == REG_OP)
				if (MEM_OPERAND(insn->Operands[1].Op)->Flags & DISPL && !(MEM_OPERAND(insn->Operands[1].Op)->Flags & BASE))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Mem_Base_Op(Px86Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == IMM_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE &&
				    !(MEM_OPERAND(insn->Operands[0].Op)->Flags & INDEX))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Mem_Base_Index_Op(Px86Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == IMM_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE &&
				    (MEM_OPERAND(insn->Operands[0].Op)->Flags & INDEX))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Reg_Mem_Base_Op(Px86Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == REG_OP){
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE)
					return TRUE;
			}

			return FALSE;
		}



		uint32_t Is_Reg_Mem_Base_Index_Op(Px86Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == REG_OP){
				if ((MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE) && (MEM_OPERAND(insn->Operands[0].Op)->Flags & INDEX))
					return TRUE;
			}

			return FALSE;
		}




		uint32_t Is_Mem_Base_Reg_Op(Px86Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[1].Type == MEM_OP && insn->Operands[0].Type == REG_OP){
				if (MEM_OPERAND(insn->Operands[1].Op)->Flags & BASE)
					return TRUE;
			}

			return FALSE;
		}



		uint32_t Is_Mem_Base_Index_Reg_Op(Px86Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[1].Type == MEM_OP && insn->Operands[0].Type == REG_OP){
				if ((MEM_OPERAND(insn->Operands[1].Op)->Flags & BASE) && (MEM_OPERAND(insn->Operands[1].Op)->Flags & INDEX))
					return TRUE;
			}

			return FALSE;
		}

		/*
		 *********************************************************************************
		 X86 EMIT FUNCTIONS used in x86-asm.def
		 *********************************************************************************
		 */

		void x86_Emit_ADC(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_ADC, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x86_Emit_ALU_Imm_Reg(X86_ALU_ADC, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x86_Emit_ALU_Reg_Mem_Base(
					X86_ALU_ADC,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x86_Emit_ALU_Mem_Base_Reg(
					X86_ALU_ADC,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ,
					codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x86_Emit_ALU_Displ_Reg(0x13, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x86_Emit_ALU_Reg_Displ(0x11, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_ADD(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_ADD, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x86_Emit_ALU_Imm_Reg(X86_ALU_ADD, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Imm_Mem_Base_Op(insn))
				x86_Emit_ALU_Imm_Mem_Base(X86_ALU_ADD, insn->Operands[0].Size, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), 0, codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x86_Emit_ALU_Displ_Reg(X86_ALU_ADD, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x86_Emit_ALU_Reg_Mem_Base(
					X86_ALU_ADD,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if(Is_Imm_Displ_Op(insn))
				x86_Emit_ALU_Imm_Displ(
					X86_ALU_ADD,
					insn->Operands[0].Size,
					IMM_OPERAND(insn->Operands[1].Op)->Value,
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x86_Emit_ALU_Mem_Base_Reg(
					X86_ALU_ADD,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ,
					codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x86_Emit_ALU_Reg_Displ(X86_ALU_ADD, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_AND(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_AND, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x86_Emit_ALU_Imm_Reg(X86_ALU_AND, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_BSF(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_BSR(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_BSWAP(Px86Instruction insn, uint8_t **codeBuf)
		{
			x86_Emit_BSWAP_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
		}

		void x86_Emit_BT(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_BTC(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_BTR(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_BTS(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CALL(Px86Instruction insn, uint8_t **codeBuf)
		{
			int32_t relAddr;

			if (insn->Operands[0].Type == IMM_OP)
			{
				relAddr = IMM_OPERAND(insn->Operands[0].Op)->Value - (intptr_t) *codeBuf - 5;

				x86_Emit_CALL_Relative32(relAddr, codeBuf);
			}

			else if (insn->Operands[0].Type == REG_OP)
			{
				/*
				 * From Intel Manual
				 * CALL near absolute indirect FF /2
				 * Mod R/M
				 *  MOD = REG_ONLY
				 *  R/M = 2
				 *  r32 = registro
				 */
				(*(*codeBuf)++) = 0xFF;
				x86_Emit_ModRM_Byte(REG_ONLY, 2, REG_OPERAND_NAME(insn, 0), codeBuf);
			}
		}

		void x86_Emit_CBW(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CDQ(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CLC(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CLD(Px86Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0xFC;
		}

		void x86_Emit_CMC(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CMOV(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CMP(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_CMP, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if(Is_Imm_Reg_Op(insn)) {
				x86_Emit_ALU_Imm_Reg(X86_ALU_CMP, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			}
			else if(Is_Imm_Mem_Base_Op(insn))
			{
				uint8_t baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				uint32_t displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x86_Emit_ALU_Imm_Mem_Base(X86_ALU_CMP, insn->Operands[0].Size, IMM_OPERAND(insn->Operands[1].Op)->Value, baseReg, displ, codeBuf);
			}
			else if(Is_Imm_Mem_Base_Index_Op(insn))
			{
				uint8_t baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				uint32_t displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				uint8_t indexReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Index.REG_NAME);
				x86_Emit_ALU_Imm_Mem_Index(X86_ALU_CMP, insn->Operands[0].Size, IMM_OPERAND(insn->Operands[1].Op)->Value, baseReg, displ, indexReg, 0, codeBuf);
			}
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CMPS(Px86Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0xF3;
			(*(*codeBuf)++) = 0xa6;
			return;
		}

		void x86_Emit_CMPXHG(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CWD(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_CWDE(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_DEC(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x86_Emit_DEC_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_DIV(Px86Instruction insn, uint8_t **codeBuf)
		{
			/*
			 * From Intel Manual
			 * MUL F7 /6
			 * Mod R/M
			 *  MOD = REG_ONLY
			 *  R/M = 6
			 *  r32 = registro
			 */
			(*(*codeBuf)++) = 0xF7;
			x86_Emit_ModRM_Byte(REG_ONLY, 6, REG_OPERAND_NAME(insn, 0), codeBuf);
		}

		void x86_Emit_IDIV(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_IMUL(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_INC(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x86_Emit_INC_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_J(Px86Instruction insn, uint8_t **codeBuf)
		{
			x86_Emit_J_Relative32(CC_ONLY(insn->getOpcode()), 0, codeBuf);
		}

		void x86_Emit_JECXZ(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_JMP_Indirect(Px86Instruction insn, uint8_t **bufPtr)
		{
			if (insn->Operands[0].Type == MEM_OP){
				if ((MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE)){
					uint8_t base = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
					uint32_t displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
					if(insn->table_jump)
						displ = ((intptr_t)*bufPtr) + 6;
					x86_Emit_JMP_Indirect_Mem_Base(base, displ, insn->Operands[1].Size, bufPtr);
				}
				else
					NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
			}
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_JMP(Px86Instruction insn, uint8_t **codeBuf)
		{
			if(insn->Operands[0].Type == LBL_OP)
			{
				x86_Emit_JMP_Relative32(0, codeBuf);
			}
			else if (insn->Operands[0].Type == MEM_OP)
			{
				x86_Emit_JMP_Indirect(insn, codeBuf);
			}
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_LEA(Px86Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0x8D;
			uint8_t dst = REG_OPERAND_NAME(insn, 0);
			uint8_t baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
			uint8_t indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
			uint32_t displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
			uint8_t scale =  MEM_OPERAND(insn->Operands[1].Op)->Scale;
			x86_Emit_Reg_Mem_Index(dst, baseReg, displ, indexReg, scale, codeBuf);
		}

		void x86_Emit_LODS(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_LOOP(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_LOOPNZ(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_LOOPZ(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_MOV(Px86Instruction insn, uint8_t **codeBuf)
		{
			uint8_t srcReg, dstReg, baseReg, indexReg;
			uint32_t displ;

			if (Is_Reg_Reg_Op(insn)){
				x86_Emit_MOV_Reg_Reg(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			}
			else if (Is_Reg_Mem_Base_Index_Op(insn)){
				srcReg = (uint8_t)REG_OPERAND_NAME(insn, 1);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[0].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x86_Emit_MOV_Reg_Mem_Index(srcReg, baseReg, displ, indexReg, 0, insn->Operands[1].Size, codeBuf);

			}
			else if (Is_Mem_Base_Index_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOV_Mem_Index_Reg(dstReg, baseReg, displ, indexReg, 0, insn->Operands[0].Size, codeBuf);
			}
			else if (Is_Imm_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				x86_Emit_MOV_Imm_Reg(IMM_OPERAND(insn->Operands[1].Op)->Value, dstReg, codeBuf);
			}
			else if (Is_Mem_Base_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOV_Mem_Base_Reg(dstReg, baseReg, displ, insn->Operands[0].Size, codeBuf);

			}
			else if (Is_Reg_Mem_Base_Op(insn)){
				srcReg = (uint8_t)REG_OPERAND_NAME(insn, 1);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x86_Emit_MOV_Reg_Mem_Base(srcReg, baseReg, displ, insn->Operands[0].Size, codeBuf);
			}
			else if(Is_Imm_Mem_Base_Op(insn)){
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x86_Emit_MOV_Imm_Mem_Base(IMM_OPERAND(insn->Operands[1].Op)->Value,	baseReg, displ,	insn->Operands[0].Size, codeBuf);
			} else if(Is_Reg_Displ_Op(insn)) {
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				srcReg = (uint8_t)REG_OPERAND_NAME(insn, 1);
				x86_Emit_MOV_Reg_Displ(srcReg, displ, insn->Operands[1].Size, codeBuf);
			} else if(Is_Displ_Reg_Op(insn)) {
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOV_Displ_Reg(displ, dstReg, insn->Operands[0].Size, codeBuf);
			} else if(Is_Imm_Displ_Op(insn)) {
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x86_Emit_MOV_Imm_Displ(IMM_OPERAND(insn->Operands[1].Op)->Value, displ, insn->Operands[0].Size, codeBuf);
			} else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")

		}

		void x86_Emit_MOVS(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_MOVSX(Px86Instruction insn, uint8_t **codeBuf)
		{
			uint8_t dstReg, baseReg, indexReg;
			uint32_t displ;

			if (Is_Reg_Reg_Op(insn)){
				x86_Emit_MOVSX_Reg_Reg(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[1].Size, codeBuf);
			}
			else if (Is_Mem_Base_Index_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOVSX_Mem_Index_Reg(dstReg, baseReg, displ, indexReg, 0, insn->Operands[1].Size, codeBuf);
			}
			else if(Is_Mem_Base_Reg_Op(insn)){
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOVSX_Mem_Base_Reg(REG_OPERAND_NAME(insn, 0), baseReg, displ, insn->Operands[1].Size, codeBuf);

			} else if(Is_Imm_Displ_Op(insn)) {
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x86_Emit_MOV_Imm_Displ(IMM_OPERAND(insn->Operands[1].Op)->Value, displ, insn->Operands[0].Size, codeBuf);
			}
			else if(Is_Displ_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOVSX_Displ_Reg(displ, dstReg, insn->Operands[0].Size, codeBuf);


			}
			else
				NETVM_ASSERT(1 == 0, " CANNOT BE HERE!")
		}

		void x86_Emit_MOVZX(Px86Instruction insn, uint8_t **codeBuf)
		{
			uint8_t dstReg, baseReg, indexReg;
			uint32_t displ;

			if (Is_Reg_Reg_Op(insn)){
				x86_Emit_MOVZX_Reg_Reg(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[1].Size, codeBuf);
			}
			else if (Is_Mem_Base_Index_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOVZX_Mem_Index_Reg(dstReg, baseReg, displ, indexReg, 0, insn->Operands[1].Size, codeBuf);
			}
			else if(Is_Mem_Base_Reg_Op(insn)){
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOVZX_Mem_Base_Reg(REG_OPERAND_NAME(insn, 0), baseReg, displ, insn->Operands[1].Size, codeBuf);
			}
			else if(Is_Displ_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x86_Emit_MOVZX_Displ_Reg(displ, dstReg, insn->Operands[0].Size, codeBuf);


			}
			else
				NETVM_ASSERT(1 == 0, " CANNOT BE HERE!")
		}

		void x86_Emit_MUL(Px86Instruction insn, uint8_t **codeBuf)
		{
			/*
			 * From Intel Manual
			 * MUL F7 /4
			 * Mod R/M
			 *  MOD = REG_ONLY
			 *  R/M = 4
			 *  r32 = registro
			 */
			(*(*codeBuf)++) = 0xF7;
			x86_Emit_ModRM_Byte(REG_ONLY, 4, REG_OPERAND_NAME(insn, 0), codeBuf);
		}

		void x86_Emit_NEG(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
			{
				uint8_t szBit = 0;

				x86OpndSz size = insn->Operands[0].Size;

				if(size == x86_WORD)
					x86_Emit_Prefix(OPER_SIZE, codeBuf);
				else if (size == x86_DWORD)
					szBit = 1;

				(*(*codeBuf)++) = 0xF6 | szBit;
				x86_Emit_ModRM_Byte(REG_ONLY, 3, REG_OPERAND_NAME(insn, 0), codeBuf);
			}
			else
			NETVM_ASSERT(1 == 0, "X86 INSTRUCTION NOT IMPLEMENTED")
		}

		void x86_Emit_NOP(Px86Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0x90;
		}

		void x86_Emit_NOT(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x86_Emit_NOT_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_OR(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_OR, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x86_Emit_ALU_Imm_Reg(X86_ALU_OR, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_POP(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x86_Emit_POP_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_POPFD(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_PUSH(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x86_Emit_PUSH_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (insn->Operands[0].Type == MEM_OP)
				x86_Emit_PUSH_Mem_Base(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME, MEM_OPERAND(insn->Operands[0].Op)->Displ, codeBuf);
			else if (insn->Operands[0].Type == IMM_OP)
				x86_Emit_PUSH_Imm(IMM_OPERAND(insn->Operands[0].Op)->Value, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_PUSHFD(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_RET(Px86Instruction insn, uint8_t **codeBuf)
		{
			x86_Emit_Ret(codeBuf);
		}

		void x86_Emit_ROL(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Imm_Reg_Op(insn))
				x86_Emit_SHIFT_Imm_Reg(X86_SHF_ROL, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_ROR(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SAL(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SAR(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_SHIFT_CL_Reg(X86_SHF_SAR, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x86_Emit_SHIFT_Imm_Reg(X86_SHF_SAR, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SBB(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_SBB, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x86_Emit_ALU_Imm_Reg(X86_ALU_SBB, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x86_Emit_ALU_Reg_Mem_Base(
					X86_ALU_SBB,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x86_Emit_ALU_Mem_Base_Reg(
					X86_ALU_SBB,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ,
					codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x86_Emit_ALU_Displ_Reg(0x1B, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x86_Emit_ALU_Reg_Displ(0x19, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SCAS(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SET(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SHL(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Imm_Reg_Op(insn))
				x86_Emit_SHIFT_Imm_Reg(X86_SHF_SHL, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Reg_Op(insn))
				x86_Emit_SHIFT_CL_Reg(X86_SHF_SHL, REG_OPERAND_NAME(insn, 0),  insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SHR(Px86Instruction insn, uint8_t **codeBuf)
		{


			if (Is_Imm_Reg_Op(insn))
				x86_Emit_SHIFT_Imm_Reg(X86_SHF_SHR, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Reg_Op(insn))
				x86_Emit_SHIFT_CL_Reg(X86_SHF_SHR, REG_OPERAND_NAME(insn, 0),  insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!! " )
		}

		void x86_Emit_STC(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_STD(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_STOS(Px86Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0xF3;
			(*(*codeBuf)++) = 0xAA;

			//NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_SUB(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_SUB, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x86_Emit_ALU_Imm_Reg(X86_ALU_SUB, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x86_Emit_ALU_Reg_Mem_Base(
					X86_ALU_SUB,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x86_Emit_ALU_Mem_Base_Reg(
					X86_ALU_SUB,
					x86_DWORD,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ,
					codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x86_Emit_ALU_Displ_Reg(0x2B, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x86_Emit_ALU_Reg_Displ(0x29, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_TEST(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_XADD(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_XCHG(Px86Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_XOR(Px86Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x86_Emit_ALU_Reg_Reg(X86_ALU_XOR, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X86_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x86_Emit_Sw_Table_Entry(Px86Instruction insn, uint8_t **codeBuf)
		{
			(*codeBuf) += 4;
		}

		void x86_Emit_Sw_Table_Entry_Start(Px86Instruction insn, uint8_t **codeBuf)
		{
			Px86Instruction entry = insn->switch_entry;

			x86_Emit_Imm32((intptr_t)(*codeBuf), &entry->emission_address);
		}

		void x86_Emit_SAVEREGS(Px86Instruction insn, uint8_t **codeBuf)
		{
			uint32_t caller_save_mask[] = X86_CALLER_SAVE_REGS_MASK;

			for(uint8_t reg = 0; reg < X86_NUM_REGS; reg++)
			{
				if(caller_save_mask[reg] == TRUE)
					x86_Emit_PUSH_Reg(reg, codeBuf);
			}
		}

		void x86_Emit_LOADREGS(Px86Instruction insn, uint8_t **codeBuf)
		{
			uint32_t caller_save_mask[] = X86_CALLER_SAVE_REGS_MASK;

			for(int8_t reg = X86_NUM_REGS - 1; reg >= 0; reg--)
			{
				if(caller_save_mask[reg] == TRUE)
					x86_Emit_POP_Reg(reg, codeBuf);
			}
		}

		void x86_Emit_Comment(Px86Instruction insn, uint8_t **codeBuf)
		{
			//do nothing
			return;
		}

        void x86_Emit_RDTSC(Px86Instruction insn, uint8_t **codeBuf)
		{
			x86_emit_RDTSC(codeBuf);
		}

		void emit_prof_timer_sampling(uint8_t **codeBuf, uint64_t *counter)
		{
#ifdef RTE_PROFILE_COUNTERS
			x86_emit_RDTSC(codeBuf);
			x86_Emit_MOV_Imm_Reg((uint32_t)counter, ECX, codeBuf);
			x86_Emit_MOV_Reg_Mem_Base(EAX, ECX, 0, x86_DWORD, codeBuf);
			x86_Emit_MOV_Reg_Mem_Base(EDX, ECX, 4, x86_DWORD, codeBuf);
#endif
		}

		void emit_inc_pkt_counter(uint8_t **codeBuf, uint32_t *counter)
		{
#ifdef RTE_PROFILE_COUNTERS
			x86_Emit_MOV_Imm_Reg((uint32_t)counter, ECX, codeBuf);
			x86_Emit_ALU_Imm_Mem_Base(X86_ALU_ADD, x86_DWORD, 1, ECX, 0, codeBuf);
#endif
		}

		const std::string x86_Emitter::prop_name("x86_start_offset");

		x86_Emitter::x86_Emitter(CFG<x86Instruction>& cfg, GCRegAlloc<CFG<x86Instruction> >& regAlloc, TraceBuilder<jit::CFG<x86Instruction> >& trace_builder)
			: buffer(NULL), current(NULL),
			cfg(cfg), regAlloc(regAlloc), trace_builder(trace_builder)
		{
		}

		void x86_Emitter::patch_entry(patch_info& pinfo)
		{
			uint8_t* targetAddr = cfg.getBBById(pinfo.destination_id)->getProperty<uint8_t*>(prop_name);
			x86_Emit_Imm32((intptr_t)targetAddr, &pinfo.emission_address);
		}

		void x86_patch_instruction(uint8_t* instrToPatch, uint8_t* targetAddr)
		{
			uint32_t relAddr;

			if (*instrToPatch == 0xE9){	//JMP: have to skip 1 byte and rewrite the target
				relAddr = (intptr_t) targetAddr - (intptr_t) instrToPatch - 5;
				instrToPatch++;
				x86_Emit_Imm32(relAddr, &instrToPatch);

			}
			else if (*instrToPatch == 0x0F){	//Jxx: have to skip 2 bytes and rewrite the target
				relAddr =(intptr_t) targetAddr - (intptr_t) instrToPatch - 6;
				instrToPatch += 2;
				x86_Emit_Imm32(relAddr, &instrToPatch);

			}

			assert(1 != 0 && "Wrong instruction to patch pointer");
		}

		void x86_Emitter::patch_jump(patch_info& pinfo)
		{
			uint8_t *instrToPatch, *targetAddr;

			instrToPatch = pinfo.emission_address;
			targetAddr = (pinfo.last ? epilogue : cfg.getBBById(pinfo.destination_id)->getProperty<uint8_t*>(prop_name));

			assert(targetAddr != NULL);

			x86_patch_instruction(instrToPatch, targetAddr);
		}

		uint8_t* x86_Emitter::emit()
		{
			uint32_t npages = (((cfg.get_insn_num() + 200)* MAX_BYTES_PER_INSN) / 4096) + 1;
			uint32_t nbytes = npages * 4096; //Davide: la pagina è sempre da 4096 ?
			current = buffer = (uint8_t*) allocCodePages(nbytes);
			
			//emit_prologue();

			TraceBuilder<jit::CFG<x86Instruction> >::trace_iterator_t t = trace_builder.begin();

			while(t != trace_builder.end())
			{
				emitBB(*t);
				t++;
			}

			//emit_epilogue();

			for (std::list<patch_info>::iterator j = jumps.begin(); j != jumps.end(); j++)
			{
				patch_jump(*j);
			}

			for (std::list<patch_info>::iterator j = entries.begin(); j != entries.end(); j++)
			{
				patch_entry(*j);
			}

			#ifdef RTE_PROFILE_COUNTERS
			std::cout << cfg.getName() <<" numero di instruzioni: " << cfg.get_insn_num() << std::endl;
			std::cout << cfg.getName() <<" dimensione del codice: " << current - buffer << std::endl;
			#endif
			
			setPageProtection(buffer, nbytes, true, false);
			
			return buffer;
		}
		
		void* x86_Emitter::allocCodePages(size_t size) {
		  #ifdef WIN32
		      return (void*) calloc(size, sizeof (uint8_t));
		  #else 
		      void *addr = mmap(NULL,
					size,
					PROT_NONE,
#if defined(__APPLE__)
					MAP_PRIVATE | MAP_ANON,
#else
					MAP_PRIVATE | MAP_ANONYMOUS,
#endif
					-1, 0);
		      if (addr == MAP_FAILED) {
			  return NULL;
		      }

		      return mmap(addr,
							 size,
							 PROT_READ | PROT_WRITE,
#if defined(__APPLE__)
							 MAP_PRIVATE | MAP_FIXED | MAP_ANON,
#else
							 MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
#endif
							 -1, 0);
		  #endif
		}
		
		void x86_Emitter::setPageProtection(void *address, size_t size, bool executableFlag, bool writeableFlag) {
			#ifdef WIN32
			  return;
			#else
			  int bitmask = 4096 -1; // sysconf(_SC_PAGESIZE) - 1;
			  // mprotect requires that the addresses be aligned on page boundaries
			  void *endAddress = (void*) ((char*)address + size);
			  void *beginPage = (void*) ((size_t)address & ~bitmask);
			  void *endPage   = (void*) (((size_t)endAddress + bitmask) & ~bitmask);
			  size_t sizePaged = (size_t)endPage - (size_t)beginPage;
			  
			  int flags = PROT_READ;
			  if (executableFlag) {
				flags |= PROT_EXEC;
			  }
			  if (writeableFlag) {
				flags |= PROT_WRITE;
			  }
			  int retval = mprotect(beginPage, (unsigned int)sizePaged, flags);
			  if(retval != 0){
			    std::cout << "Failed to change page protection";
			  }
			#endif
		}


		uint32_t x86_Emitter::getActualBufferSize(void)
		{
			if (buffer == NULL)
				return 0;
			return current-buffer;
		}

		x86_Emitter::patch_info::patch_info(x86OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id, bool last)
			: opcode(opcode), emission_address(emission_address), destination_id(destination_id), last(last)
		{
		}

		x86_Emitter::patch_info::patch_info(uint8_t* emission_address, uint16_t destination_id)
			: opcode(x86OpCodesEnum(0)), emission_address(emission_address), destination_id(destination_id), last(false)
		{
		}

		void x86_Emitter::add_jump(x86OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id)
		{
			jumps.push_back(patch_info(opcode, emission_address, destination_id));
		}

		void x86_Emitter::add_last_jump(uint8_t* emission_address)
		{
			jumps.push_back(patch_info(X86_JMP, emission_address, 0, true));
		}

		void x86_Emitter::add_jump(x86Instruction* insn, uint8_t* emission_address)
		{
			x86OpCodesEnum opcode = (x86OpCodesEnum)OP_ONLY(insn->getOpcode());

			assert(OP_ONLY(insn->getOpcode()) == X86_J ||
					OP_ONLY(insn->getOpcode()) == X86_JMP ||
					OP_ONLY(insn->getOpcode()) == X86_JECXZ);

			uint16_t destination_id = LBL_OPERAND(insn->Operands[0].Op)->Label;
			add_jump(opcode, emission_address, destination_id);
		}

		void x86_Emitter::add_entry(x86Instruction* insn, uint8_t* emission_address)
		{
			OP_ONLY(insn->getOpcode());
			// x86OpCodesEnum opcode = (x86OpCodesEnum)OP_ONLY(insn->getOpcode());

			entries.push_back(patch_info(emission_address, insn->switch_default_target));
		}

		void x86_Emitter::emit_insn(x86Instruction* insn)
		{
			//std::cout << "emitting instruction " << *insn << std::endl;
			uint16_t opcode = OP_ONLY(insn->getOpcode());

			if(opcode == X86_J || opcode == X86_JMP || opcode == X86_JECXZ)
			{
				if(insn->NOperands >= 1 && (insn->Operands[0].Type == REG_OP || insn->Operands[0].Type == LBL_OP))
				{ //this is not a switch table indirect jump
					if(insn->switch_entry)
					{   //this is a target of a jump in a bynary search tree need to patch previous branch
						uint8_t* addr = insn->switch_entry->emission_address;
						uint8_t* target_addr = current - 6;
						x86_patch_instruction(addr, target_addr);
					}

					if(!insn->table_jump)
					{
						//this is a normal jump we need to patch it later
						add_jump(insn, current);
					}
					//else this a jump in a binary search tree will be patched during emission
				}
			}

			if(opcode == X86_SW_TABLE_ENTRY && insn->switch_default_target)
			{
				add_entry(insn, current);
			}

			insn->emission_address = current;
			insn->OpDescr->EmitFunct(insn, &current);
		}

		void x86_Emitter::emitBB(bb_t *bb)
		{
			//std::cout << "Emitting BB " << *bb << std::endl;
			typedef std::list<x86Instruction*>::iterator code_iterator_t;

			//set this node starting address
			bb->setProperty(prop_name, current);

			//emit code
			std::list<x86Instruction*>& code = bb->getCode();
			x86Instruction* insn = NULL;
			for(code_iterator_t c = code.begin(); c != code.end(); c++)
			{
				insn = *c;
				emit_insn(insn);
			}

			//if(bb->getId() == bb_t::EXIT_BB)
			//{
			//	add_last_jump(current);
			//	x86_Emit_JMP_Relative32(0, &current);
			//}
		}

		/*!
		 * Function Epilogue looks Like this
		 *
		 * pop		ebx			; restore saved registers (if used)
		 * pop		edi
		 * pop		esi
		 * mov		esp, ebp	; restore ESP
		 * pop		ebp			; restore EBP
		 */
		void x86_Emitter::emit_epilogue()
		{
			uint32_t calleeSaveMask[X86_NUM_REGS] = X86_CALLEE_SAVE_REGS_MASK;
			int32_t i;

			epilogue = current;
			//x86_Emit_ALU_Reg_Reg(X86_ALU_XOR, EAX, EAX, x86_DWORD, codeBuf);
			//
#ifdef JIT_RTE_PROFILE_COUNTERS
			x86_Emit_ALU_Imm_Reg(X86_ALU_ADD, 8, ESP, &current);
#endif
			//

			for (i = X86_NUM_REGS - 1; i >= 0; i--){
				if (calleeSaveMask[i] && regAlloc.isAllocated(X86_MACH_REG(i)))
					x86_Emit_POP_Reg(i, &current);
			}

			uint32_t numSpilled = regAlloc.getNumSpilledReg();
			if(numSpilled != 0)
				x86_Emit_ALU_Imm_Reg(X86_ALU_ADD, numSpilled * 4, ESP, &current);

			x86_Emit_MOV_Reg_Reg(EBP, ESP, x86_DWORD, &current);
			x86_Emit_POP_Reg(EBP, &current);

			x86_Emit_Ret(&current);
		}

		/*!
		 * Function Prologue looks Like this
		 *
		 * push	ebp
		 * mov		ebp, esp
		 * sub		esp, XXX	; create spill area
		 * push	esi			; push callee save registers (if used)
		 * push	edi
		 * push	ebx
		 */
		void x86_Emitter::emit_prologue()
		{
			uint32_t calleeSaveMask[X86_NUM_REGS] = X86_CALLEE_SAVE_REGS_MASK;
			uint32_t i;


			x86_Emit_PUSH_Reg(EBP, &current);
			x86_Emit_MOV_Reg_Reg(ESP, EBP, x86_DWORD, &current);

			uint32_t numSpilled = regAlloc.getNumSpilledReg();

			if(numSpilled != 0)
				x86_Emit_ALU_Imm_Reg(X86_ALU_SUB, numSpilled * 4, ESP, &current);

			for (i = 0; i < X86_NUM_REGS; i++){
				if (calleeSaveMask[i] && regAlloc.isAllocated(X86_MACH_REG(i))){
					x86_Emit_PUSH_Reg(i, &current);
				}
			}

#ifdef JIT_RTE_PROFILE_COUNTERS
			x86_emit_RDTSC(&current);
			x86_Emit_ALU_Imm_Reg(X86_ALU_SUB, 8, ESP, &current);
			x86_Emit_MOV_Reg_Mem_Base(EAX, ESP, 0, x86_DWORD, &current);
			x86_Emit_MOV_Reg_Mem_Base(EDX, ESP, 4, x86_DWORD, &current);
#endif

#ifdef _DEBUG_X86_CODE
			x86_Emit_PUSH_Imm((uint32_t)Application::getApp(0).getCurrentSegment()->Name, &current);
			x86_Emit_CALL_Relative32(((int32_t)puts) - (int32_t) current - 5, &current);
			x86_Emit_POP_Reg(0, &current);
#endif

		}


	} //namespace ia32
} //namespace jit


