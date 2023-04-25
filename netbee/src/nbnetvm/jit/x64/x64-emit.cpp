/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/** @file x64-emit.cpp
 * \brief This file contains the functions that emit x64 code in memory
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <limits.h>

#include <application.h>
#include "x64-asm.h"
#include "x64-emit.h"
#include "netvmjitglobals.h"
#include "../../../nbee/globals/debug.h"
#include "tracebuilder.h"
#include "application.h"

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
	namespace x64{


		//Alu operation opcodes
		typedef enum
		{
			X64_ALU_ADD = 0,
			X64_ALU_OR  = 1,
			X64_ALU_ADC = 2,
			X64_ALU_SBB = 3,
			X64_ALU_AND = 4,
			X64_ALU_SUB = 5,
			X64_ALU_XOR = 6,
			X64_ALU_CMP = 7
		} x64AluOpEnum;


		//Shift operation opcodes
		typedef enum
		{
			X64_SHF_ROL = 0,
			X64_SHF_ROR = 1,
			X64_SHF_RCL = 2,
			X64_SHF_RCR = 3,
			X64_SHF_SHL = 4,
			X64_SHF_SHR = 5,
			X64_SHF_SAR = 7
		} x64ShiftOpEnum;


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

		void x64_Emit_Prefix(uint8_t prefix, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = prefix;
		}
		
		
		/** 
		* Emit REX Prefix, used to access registers r8-r15 and to set operator size to 64bit
		*
		* @param src pointer to src register or 0 (for none), it will be modified as needed to be emitted in next bytes
		* @param dst pointer to dst register or 0 (for none), it will be modified as needed to be emitted in next bytes
		* @param idx pointer to ids register or 0 (for none), it will be modified as needed to be emitted in next bytes
		* @param size operator size
		* @param bufPtr pointer to the buffer where emit the prefix
		*
		* @return void, all register and buffer pointer are updated.
		*/
		void x64_Emit_REX_Prefix(uint8_t* reg, uint8_t* base, uint8_t* idx, x64OpndSz size, uint8_t **bufPtr)
		{
		    uint8_t rex = 0x40;
		    if (size == x64_QWORD){
		      rex |= 0x8;
		    }
		    if (reg != 0 && *reg >= R8){
		      rex |= 0x4;
		      *reg -= 0x8;
		    }
		    if (base != 0 && *base >= R8){
		      rex |= 0x1;
		      *base -= 0x8;
		    }
		    if (idx != 0 && *idx >= R8){
		      rex |= 0x2;
		      *idx -= 0x8;
		    }
		    if (rex != 0x40)
		      x64_Emit_Prefix(rex, bufPtr);
		}

		void x64_Emit_Imm64(uint64_t immVal, uint8_t **bufPtr)
		{
				
			uint8_t valBuf[8];

			(*(uint64_t*)valBuf) = immVal;
			(*(*bufPtr)++) = valBuf[0];
			(*(*bufPtr)++) = valBuf[1];
			(*(*bufPtr)++) = valBuf[2];
			(*(*bufPtr)++) = valBuf[3];
			(*(*bufPtr)++) = valBuf[4];
			(*(*bufPtr)++) = valBuf[5];
			(*(*bufPtr)++) = valBuf[6];
			(*(*bufPtr)++) = valBuf[7];
		}

		void x64_Emit_Imm32(uint32_t immVal, uint8_t **bufPtr)
		{		
			uint8_t valBuf[4];

			(*(uint32_t*)valBuf) = immVal;
			(*(*bufPtr)++) = valBuf[0];
			(*(*bufPtr)++) = valBuf[1];
			(*(*bufPtr)++) = valBuf[2];
			(*(*bufPtr)++) = valBuf[3];
		}
		
		/*
		*	This function considers the value as a 32 bit integer
		*/
		void x64_Emit_Imm32_signed(int32_t immVal, uint8_t **bufPtr)
		{
			uint8_t valBuf[4];

			(*(int32_t*)valBuf) = immVal;
			(*(*bufPtr)++) = valBuf[0];
			(*(*bufPtr)++) = valBuf[1];
			(*(*bufPtr)++) = valBuf[2];
			(*(*bufPtr)++) = valBuf[3];
		}



		void x64_Emit_Imm16(uint16_t immVal, uint8_t **bufPtr)
		{
			uint8_t valBuf[2];

			(*(uint16_t*)valBuf) = immVal;
			(*(*bufPtr)++) = valBuf[0];
			(*(*bufPtr)++) = valBuf[1];
		}


		void x64_Emit_Imm8(uint8_t immVal, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = immVal;
		}


		void x64_Emit_ModRM_Byte(uint8_t mod, uint8_t reg, uint8_t rm, uint8_t **bufPtr)
		{
		
			(*(*bufPtr)++) = ((mod & MOD_MASK) << 6) | ((reg & REG_MASK) << 3) | (rm & RM_MASK);
		}


		void x64_Emit_SIB_Byte(uint8_t scale, uint8_t index, uint8_t base, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = ((scale & SCALE_MASK) << 6) | ((index & INDEX_MASK) << 3) | (base & BASE_MASK);
		}


		void x64_Emit_Reg_Reg(uint8_t reg1, uint8_t reg2, uint8_t **bufPtr)
		{
			x64_Emit_ModRM_Byte(REG_ONLY, reg1, reg2, bufPtr);
		}


		void x64_Emit_Reg_Mem_Base(uint8_t reg, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			if (base == ESP){		// Correct addressing needs the SIB byte
				if (displ == 0){
					x64_Emit_ModRM_Byte(0x0, reg, 0x4, bufPtr);
					x64_Emit_SIB_Byte(0x0, 0x04, ESP, bufPtr);
				}
				else if (Is_Imm8(displ)){
					x64_Emit_ModRM_Byte(0x1, reg, 0x4, bufPtr);
					x64_Emit_SIB_Byte(0, 0x04, ESP, bufPtr);
					x64_Emit_Imm8(displ, bufPtr);
				}
				else{
					x64_Emit_ModRM_Byte(0x02, reg, 0x04, bufPtr);
					x64_Emit_SIB_Byte(0, 0x04, ESP, bufPtr);
					x64_Emit_Imm32(displ, bufPtr);
				}
				return;
			}
			if (displ == 0 && base != EBP){
				x64_Emit_ModRM_Byte(0, reg, base, bufPtr);
				return;
			}

			if (Is_Imm8(displ)){
				x64_Emit_ModRM_Byte(0x1, reg, base, bufPtr);
				x64_Emit_Imm8(displ, bufPtr);
			}
			else{
				x64_Emit_ModRM_Byte(0x02, reg, base, bufPtr);
				x64_Emit_Imm32(displ, bufPtr);
			}

		}



		void x64_Emit_Reg_Mem_Index(uint8_t reg, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			if (index != EBP && displ == 0){
				x64_Emit_ModRM_Byte(0, reg, 0x04, bufPtr);
				x64_Emit_SIB_Byte(scale, index, base, bufPtr);
			}
			else if (Is_Imm8(displ)){
				x64_Emit_ModRM_Byte(1, reg, 0x04, bufPtr);
				x64_Emit_SIB_Byte(scale, index, base, bufPtr);
				x64_Emit_Imm8(displ, bufPtr);
			}
			else{
				x64_Emit_ModRM_Byte(2, reg, 0x04, bufPtr);
				x64_Emit_SIB_Byte(scale, index, base, bufPtr);
				x64_Emit_Imm32(displ, bufPtr);
			}

		}

		void x64_Emit_ALU(uint8_t aluOpcode, x64OpndSz operandsSize, uint8_t dir, uint8_t **bufPtr)
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

			if (operandsSize == x64_DWORD || operandsSize == x64_QWORD)
				opcodeByte |= 1;

			opcodeByte |= aluOpcode << 3;

			(*(*bufPtr)++) = opcodeByte;

		}


		void x64_Emit_ALU_Reg_Reg(uint8_t aluOpcode, uint8_t src, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			
			x64_Emit_REX_Prefix(&src, &dst, 0, size, bufPtr);

			x64_Emit_ALU(aluOpcode, size, 0, bufPtr);
			x64_Emit_Reg_Reg(src, dst, bufPtr);

		}

		void x64_Emit_ALU_Reg_Displ(uint8_t aluOpcode, uint8_t src, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{ 	
			x64_Emit_REX_Prefix(&src, 0, 0, size, bufPtr);
			(*(*bufPtr)++) = aluOpcode;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x64_Emit_ModRM_Byte(BASE_ONLY, src, 5, bufPtr);
			x64_Emit_Imm32(displ, bufPtr);
		}

		void x64_Emit_ALU_Displ_Reg(uint8_t aluOpcode, uint32_t displ, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &dst, 0, size, bufPtr);
			(*(*bufPtr)++) = aluOpcode;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x64_Emit_ModRM_Byte(BASE_ONLY, dst, 5, bufPtr);
			x64_Emit_Imm32(displ, bufPtr);
		}

		void x64_Emit_ALU_Imm_Reg(uint8_t aluOpcode, uint32_t immVal, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			// We suppose to have always DWORD immediate values to avoid the management of byte/word parts of regs
			// so we put the s bit to 0 (no sign extension) and the w bit to 1 (size = 32 bit)
			x64_Emit_REX_Prefix(0, &dst, 0, size, bufPtr);
			(*(*bufPtr)++) = 0x81;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x64_Emit_ModRM_Byte(3, aluOpcode, dst, bufPtr);
			x64_Emit_Imm32(immVal, bufPtr);

		}

		void x64_Emit_ALU_Imm_Displ(uint8_t aluOpcode, x64OpndSz size, uint32_t immVal, uint32_t displ, uint8_t **bufPtr)
		{
			//(*(*bufPtr)++) = aluOpcode;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)

			uint8_t szBit = 0;

			if(size != x64_BYTE)
				szBit = 1;

			if(size == x64_WORD )
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			x64_Emit_REX_Prefix(0, 0, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x80 | szBit;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)

			x64_Emit_ModRM_Byte(BASE_ONLY,  aluOpcode, 5, bufPtr);

			x64_Emit_Imm32(displ, bufPtr);

			if(size == x64_BYTE)
				x64_Emit_Imm8(immVal, bufPtr);
			else if(size == x64_WORD)
				x64_Emit_Imm16(immVal, bufPtr);
			else if(size == x64_DWORD)
				x64_Emit_Imm32(immVal, bufPtr);
		}

		void x64_Emit_ALU_Imm_Mem_Base(uint8_t aluOpcode, x64OpndSz size, uint32_t immVal, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if(size != x64_BYTE)
				szBit = 1;

			if(size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			x64_Emit_REX_Prefix(0, &base, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x80 | szBit;	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)

			x64_Emit_Reg_Mem_Base(aluOpcode, base, displ, bufPtr);

			if(size == x64_BYTE)
				x64_Emit_Imm8(immVal, bufPtr);
			else if(size == x64_WORD)
				x64_Emit_Imm16(immVal, bufPtr);
			else if(size == x64_DWORD)
				x64_Emit_Imm32(immVal, bufPtr);
		}



		void x64_Emit_ALU_Reg_Mem_Base(uint8_t aluOpcode,  x64OpndSz size, uint8_t src, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			//direction bit = 0: register to memory

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			x64_Emit_REX_Prefix(&src, &base, 0, size, bufPtr);

			x64_Emit_ALU(aluOpcode, size, 0, bufPtr);
			x64_Emit_Reg_Mem_Base(src, base, displ, bufPtr);


		}


		void x64_Emit_ALU_Mem_Base_Reg(uint8_t aluOpcode,  x64OpndSz size, uint8_t dst, uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
			//direction bit = 1: memory to register

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			x64_Emit_REX_Prefix(&dst, &base, 0, size, bufPtr);

			x64_Emit_ALU(aluOpcode, size, 1, bufPtr);
			x64_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}


		void x64_Emit_ALU_Imm_Mem_Index(uint8_t aluOpcode, x64OpndSz size, uint32_t immVal, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if(size == x64_WORD || size == x64_DWORD || size == x64_QWORD)
				szBit = 1;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			x64_Emit_REX_Prefix(0, &base, &index, size, bufPtr);

			(*(*bufPtr)++) = (0x80 | szBit);	// follows ModR/M byte with opcode and the 4 bytes of immVal (w bit = 1, s bit = 0)
			x64_Emit_Reg_Mem_Index(aluOpcode, base, displ, index, scale, bufPtr);
			switch(size)
			{
				case x64_BYTE:
					x64_Emit_Imm8(immVal, bufPtr);
					break;
				case x64_WORD:
					x64_Emit_Imm16(immVal, bufPtr);
					break;
				case x64_DWORD:
					x64_Emit_Imm32(immVal, bufPtr);
					break;
				case x64_QWORD:
					x64_Emit_Imm64(immVal, bufPtr);
					break;
			}
			//x64_Emit_Imm32(immVal, bufPtr);
		}



		void x64_Emit_ALU_Reg_Mem_Index(uint8_t aluOpcode,  x64OpndSz size, uint8_t src, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			//direction bit = 0: register to memory

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			x64_Emit_REX_Prefix(&src, &base, &index, size, bufPtr);
			
			x64_Emit_ALU(aluOpcode, size, 0, bufPtr);
			x64_Emit_Reg_Mem_Index(src, base, displ, index, scale, bufPtr);


		}


		void x64_Emit_ALU_Mem_Index_Reg(uint8_t aluOpcode,  x64OpndSz size, uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, uint8_t **bufPtr)
		{
			//direction bit = 1: memory to register

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			x64_Emit_REX_Prefix(&dst, &base, &index, size, bufPtr);
			
			x64_Emit_ALU(aluOpcode, size, 1, bufPtr);
			x64_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}



		void x64_Emit_SHIFT_Imm_Reg(x64ShiftOpEnum shiftOpcode, uint32_t immVal, uint8_t dst, uint8_t **bufPtr)
		{
			if (immVal == 0)
				return;

			if (immVal == 1){
				(*(*bufPtr)++) = 0xD1;
				x64_Emit_ModRM_Byte(3, shiftOpcode, dst, bufPtr);
				return;
			}


			x64_Emit_REX_Prefix(0, &dst, 0, x64_DWORD, bufPtr);
			(*(*bufPtr)++) = 0xC1;
			x64_Emit_ModRM_Byte(3, shiftOpcode, dst, bufPtr);
			x64_Emit_Imm8(immVal, bufPtr);

		}


		void x64_Emit_SHIFT_CL_Reg(x64ShiftOpEnum shiftOpcode, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;

			if (size == x64_WORD){
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
				szBit = 1;
			}
			else if (size == x64_DWORD || size == x64_QWORD)
				szBit = 1;

			x64_Emit_REX_Prefix(0, &dst, 0, size, bufPtr);
			(*(*bufPtr)++) = 0xD2 | szBit;
			x64_Emit_ModRM_Byte(3, shiftOpcode, dst, bufPtr);
		}


		void x64_Emit_SHIFT_Imm_Mem_Base(x64ShiftOpEnum shiftOpcode, uint32_t immVal, uint8_t base, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;

			if (immVal == 0)
				return;

			if (size == x64_WORD){
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x64_DWORD || size == x64_QWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			x64_Emit_REX_Prefix(0, &base, 0, size, bufPtr);
			if (immVal == 1){
				(*(*bufPtr)++) = 0xD0 | szBit;
				x64_Emit_Reg_Mem_Base(shiftOpcode, base, displ, bufPtr);
				return;
			}

			(*(*bufPtr)++) = 0xC0 | szBit;
			x64_Emit_Reg_Mem_Base(shiftOpcode, base, displ, bufPtr);
			x64_Emit_Imm8(immVal, bufPtr);
		}





		void x64_Emit_SHIFT_CL_Mem_Base(x64ShiftOpEnum shiftOpcode, uint8_t base, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;

			if (size == x64_WORD){
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x64_DWORD || size == x64_QWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			x64_Emit_REX_Prefix(0, &base, 0, size, bufPtr);
			(*(*bufPtr)++) = 0xD2 | szBit;
			x64_Emit_Reg_Mem_Base(shiftOpcode, base, displ, bufPtr);
		}



		void x64_Emit_SHIFT_Imm_Mem_Index(x64ShiftOpEnum shiftOpcode, uint32_t immVal, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, x64OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;

			if (immVal == 0)
				return;

			if (size == x64_WORD){
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x64_DWORD || size == x64_QWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			x64_Emit_REX_Prefix(0, &base, &index, size, bufPtr);
			if (immVal == 1){
				(*(*bufPtr)++) = 0xD0 | szBit;
				x64_Emit_Reg_Mem_Index(shiftOpcode, base, displ, index, scale, bufPtr);
				return;
			}

			(*(*bufPtr)++) = 0xC0 | szBit;
			x64_Emit_Reg_Mem_Index(shiftOpcode, base, displ, index, scale, bufPtr);
			x64_Emit_Imm8(immVal, bufPtr);
		}



		void x64_Emit_SHIFT_CL_Mem_Index(x64ShiftOpEnum shiftOpcode, uint8_t src, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, x64OpndSz size, uint8_t **bufPtr)
		{
			uint32_t szBit = 0;

			if (size == x64_WORD){
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
	            szBit = 1;
            }
			else if (size == x64_DWORD || size == x64_QWORD)		/* [GM] Changed "=" to "=="!!! */
				szBit = 1;

			x64_Emit_REX_Prefix(&src, &base, &index, size, bufPtr);
			(*(*bufPtr)++) = 0xD2 | szBit;
			x64_Emit_Reg_Mem_Index(shiftOpcode, base, displ, index, scale, bufPtr);
		}





		void x64_Emit_BSWAP_Reg(uint8_t reg, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &reg, 0, x64_DWORD, bufPtr);
			(*(*bufPtr)++) = 0x0F;
			x64_Emit_ModRM_Byte(3, 1, reg, bufPtr);
		}

		void x64_Emit_JMP_Indirect_Mem_Base(uint8_t base, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &base, 0, size, bufPtr);
			(*(*bufPtr)++) = 0xFF;
			x64_Emit_ModRM_Byte(BASE_DISP32, 4, base, bufPtr);
			x64_Emit_Imm32(displ, bufPtr);
		}
		
		void x64_Emit_JMP_Indirect_Mem_Index(uint8_t base, uint8_t index, x64OpndSz size, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &base, &index, size, bufPtr);
			(*(*bufPtr)++) = 0xFF;
			x64_Emit_Reg_Mem_Index(0x4, base, 0, index, 1, bufPtr);
		}

		void x64_Emit_JMP_Relative32(uint32_t displ, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xE9;
			x64_Emit_Imm32(displ, bufPtr);
		}


		void x64_Emit_MOV_Reg_Reg(uint8_t src, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x64_DWORD || size == x64_QWORD){
				szBit = 1;
			}
			x64_Emit_REX_Prefix(&src, &dst, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x88 | szBit;
			x64_Emit_ModRM_Byte(3, src, dst, bufPtr);

		}


		void x64_Emit_MOV_Imm_Reg(uint64_t immVal, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &dst, 0, size, bufPtr);
		
			(*(*bufPtr)++) = 0xB8 + dst;
			if (size == x64_QWORD)
			  x64_Emit_Imm64(immVal, bufPtr);
			else
			  x64_Emit_Imm32(immVal, bufPtr);

		}

		void x64_Emit_MOV_Imm_Displ(uint32_t imm, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			
// 			uint8_t szBit = 0;
// 
// 			if (size == x64_WORD)
// 				//x64_Emit_Prefix(OPER_SIZE, bufPtr);
// 
// 			if (size == x64_DWORD || size == x64_WORD)
// 				szBit = 1;
			
			x64_Emit_REX_Prefix(0, 0, 0, size, bufPtr);
			//(*(*bufPtr)++) = 0xC6 | szBit;
			(*(*bufPtr)++) = 0xC7;
			x64_Emit_ModRM_Byte(0, 0, 5, bufPtr);
			x64_Emit_Imm32(displ, bufPtr);
			x64_Emit_Imm32(imm, bufPtr);
		}

		void x64_Emit_MOV_Displ_Reg(uint32_t displ, uint8_t dstReg, x64OpndSz size, uint8_t **bufPtr)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);

			if (size == x64_DWORD || size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(0, &dstReg, 0, size, bufPtr);
			
			(*(*bufPtr)++) = 0x8A | szBit;
			x64_Emit_ModRM_Byte(0, dstReg, 5, bufPtr);
			x64_Emit_Imm32(displ, bufPtr);
		}


		void x64_Emit_MOV_Reg_Displ(uint8_t src, uint64_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);

			if (size == x64_DWORD || size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				szBit = 1;

			x64_Emit_REX_Prefix(&src, 0, 0, size, bufPtr);
			
			(*(*bufPtr)++) = 0x88 | szBit;
			x64_Emit_ModRM_Byte(BASE_ONLY, src, 4, bufPtr);
			x64_Emit_SIB_Byte(0x0, 0x04, 0x05, bufPtr);
			
			if (size == x64_QWORD)
			  x64_Emit_Imm64(displ, bufPtr);
			else
			  x64_Emit_Imm32(displ, bufPtr);
		}

		void x64_Emit_MOV_Imm_Mem_Base(uint32_t immVal, uint8_t base, uint32_t displ,  x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);

			if (size == x64_DWORD || size == x64_WORD || size == x64_QWORD){
				szBit = 1;
			}
			
			x64_Emit_REX_Prefix(0, &base, 0, size, bufPtr);

			(*(*bufPtr)++) = 0xC6 | szBit;
			x64_Emit_Reg_Mem_Base(0, base, displ, bufPtr);
			if (size == x64_BYTE)
				x64_Emit_Imm8((uint8_t)immVal, bufPtr);
			else if (size == x64_WORD)
				x64_Emit_Imm16((uint16_t)immVal, bufPtr);
			else
				x64_Emit_Imm32(immVal, bufPtr);
		}

		void x64_Emit_MOV_Imm_Mem_Index(uint32_t immVal, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x64_DWORD || size == x64_QWORD){
				szBit = 1;
			}
			x64_Emit_REX_Prefix(0, &base, &index, size, bufPtr);

			(*(*bufPtr)++) = 0xC6 | szBit;
			x64_Emit_Reg_Mem_Index(0, base, displ, index, scale, bufPtr);
			if (size == x64_BYTE)
				x64_Emit_Imm8((uint8_t)immVal, bufPtr);
			else if (size == x64_WORD)
				x64_Emit_Imm16((uint16_t)immVal, bufPtr);
			else
				x64_Emit_Imm32(immVal, bufPtr);

		}

		void x64_Emit_MOV_Reg_Mem_Base(uint8_t src, uint8_t base, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			if (size == x64_DWORD || size == x64_WORD || size == x64_QWORD){
				szBit = 1;
			}
			
			x64_Emit_REX_Prefix(&src, &base, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x88 | szBit;	//dir bit = 0  opcode: 1000 100w
			x64_Emit_Reg_Mem_Base(src, base, displ, bufPtr);


		}


		void x64_Emit_MOV_Mem_Base_Reg(uint8_t dst, uint8_t base, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x64_DWORD || size == x64_QWORD){
			  szBit = 1;
			}
			x64_Emit_REX_Prefix(&dst, &base, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x8A | szBit;	//dir bit = 0  opcode: 1000 101w
			x64_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}

		void x64_Emit_MOV_Reg_Mem_Index(uint8_t src, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x64_DWORD || size == x64_QWORD){
				szBit = 1;
			}
			x64_Emit_REX_Prefix(&src, &base, &index, size, bufPtr);

			(*(*bufPtr)++) = 0x88 | szBit;	//dir bit = 0  opcode: 1000 100w
			x64_Emit_Reg_Mem_Index(src, base, displ, index, scale, bufPtr);


		}


		void x64_Emit_MOV_Mem_Index_Reg(uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				x64_Emit_Prefix(OPER_SIZE, bufPtr);
			else if (size == x64_DWORD || size == x64_QWORD){
				szBit = 1;
			}
			x64_Emit_REX_Prefix(&dst, &base, &index, size, bufPtr);

			(*(*bufPtr)++) = 0x8A | szBit;	//dir bit = 0  opcode: 1000 101w
			x64_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}


		void x64_Emit_MOVSX_Displ_Reg(uint32_t displ, uint8_t dstReg, x64OpndSz size, uint8_t **bufPtr)
		{

			uint8_t szBit = 0;

			if (size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
				
			x64_Emit_REX_Prefix(0, &dstReg, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x64_Emit_ModRM_Byte(0, dstReg, 5, bufPtr);
			x64_Emit_Imm32(displ, bufPtr);


		}

		void x64_Emit_MOVSX_Reg_Reg(uint8_t src, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(&src, &dst, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x64_Emit_ModRM_Byte(3, dst, src, bufPtr);

		}

		void x64_Emit_MOVSX_Mem_Base_Reg(uint8_t dst, uint8_t base, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(&dst, &base, 0, size, bufPtr);
			
			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x64_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}

		void x64_Emit_MOVSX_Mem_Index_Reg(uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(&dst, &base, &index, size, bufPtr);
			
			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xBE | szBit;
			x64_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}

		void x64_Emit_MOVZX_Reg_Reg(uint8_t src, uint8_t dst, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(&src, &dst, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x64_Emit_ModRM_Byte(3, dst, src, bufPtr);

		}

		void x64_Emit_MOVZX_Displ_Reg(uint32_t displ, uint8_t dstReg, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD){
				szBit = 1;
				//x64_Emit_Prefix(OPER_SIZE, bufPtr);
			}
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(0, &dstReg, 0, size, bufPtr);

			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x64_Emit_ModRM_Byte(0, dstReg, 5, bufPtr);
			x64_Emit_Imm32(displ, bufPtr);
		}

		void x64_Emit_MOVZX_Mem_Base_Reg(uint8_t dst, uint8_t base, uint32_t displ, x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(&dst, &base, 0, size, bufPtr);
			
			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x64_Emit_Reg_Mem_Base(dst, base, displ, bufPtr);
		}

		void x64_Emit_MOVZX_Mem_Index_Reg(uint8_t dst, uint8_t base, uint32_t displ, uint8_t index, uint8_t scale,  x64OpndSz size, uint8_t **bufPtr)
		{
			uint8_t szBit = 0;

			if (size == x64_WORD)
				szBit = 1;
			if (size == x64_QWORD)
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

			x64_Emit_REX_Prefix(&dst, &base, &index, size, bufPtr);
			
			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0xB6 | szBit;
			x64_Emit_Reg_Mem_Index(dst, base, displ, index, scale, bufPtr);
		}

		void x64_Emit_MOVQ_XMM_Reg(uint8_t src, uint8_t dst, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x66;
			x64_Emit_REX_Prefix(&src, &dst, 0, x64_QWORD, bufPtr);
			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0x7E;
			x64_Emit_ModRM_Byte(3, src, dst, bufPtr);
		}
		
		void x64_Emit_MOVQ_Reg_XMM(uint8_t src, uint8_t dst, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x66;
			x64_Emit_REX_Prefix(&dst, &src, 0, x64_QWORD, bufPtr);
			(*(*bufPtr)++) = 0x0F;
			(*(*bufPtr)++) = 0x6E;
			x64_Emit_ModRM_Byte(3, dst, src, bufPtr);
		}
		
		void x64_Emit_PUSH_Reg(uint8_t reg, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &reg, 0, x64_QWORD, bufPtr);
			(*(*bufPtr)++) = 0x50 | (reg & REG_MASK);
		}

		void x64_Emit_PUSH_Mem_Base(uint8_t base, uint32_t displ, uint8_t **bufPtr)
		{
		  //FIXME: we need to push 64bit or 32bit ???
			x64_Emit_REX_Prefix(0, &base, 0, x64_QWORD, bufPtr);
			(*(*bufPtr)++) = 0xFF;
			x64_Emit_Reg_Mem_Base(6, base, displ, bufPtr);
		}


		void x64_Emit_PUSH_Imm(uint32_t immVal, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, 0, 0, x64_QWORD, bufPtr);
			(*(*bufPtr)++) = 0x68;
			x64_Emit_Imm32(immVal, bufPtr);
		}


		void x64_Emit_POP_Reg(uint8_t reg, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &reg, 0, x64_QWORD, bufPtr);
			(*(*bufPtr)++) = 0x58 | (reg & REG_MASK);
		}

		void x64_Emit_CALL_Relative32(uint32_t displ, uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xE8;
			x64_Emit_Imm32(displ, bufPtr);

		}
		
		void x64_Emit_CALL_Indirect(uint8_t reg, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &reg, 0, x64_QWORD, bufPtr);
			(*(*bufPtr)++) = 0xFF;
			x64_Emit_ModRM_Byte(REG_ONLY, 2, reg, bufPtr);
		}

		void x64_Emit_Ret(uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0xC3;
		}


		void x64_Emit_NOT_Reg(uint8_t src, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &src, 0, x64_DWORD, bufPtr);
			(*(*bufPtr)++) = 0xF7;
			x64_Emit_ModRM_Byte(3, 2, src, bufPtr);
		}

		void x64_Emit_NEG_Reg(uint8_t src, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &src, 0, x64_DWORD, bufPtr);
			(*(*bufPtr)++) = 0xF7;
			x64_Emit_ModRM_Byte(3, 3, src, bufPtr);
		}

		void x64_Emit_INC_Reg(uint8_t src, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &src, 0, x64_DWORD, bufPtr);
			(*(*bufPtr)++) = 0xFF;
			x64_Emit_ModRM_Byte(3, 0, src, bufPtr);
		}


		void x64_Emit_DEC_Reg(uint8_t src, uint8_t **bufPtr)
		{
			x64_Emit_REX_Prefix(0, &src, 0, x64_DWORD, bufPtr);
			(*(*bufPtr)++) = 0xFF;
			x64_Emit_ModRM_Byte(3, 1, src, bufPtr);
		}


		void x64_emit_RDTSC(uint8_t **bufPtr)
		{
			(*(*bufPtr)++) = 0x0f;
			(*(*bufPtr)++) = 0x31;

		}
		
		uint32_t Is_XMM_Reg_Op(Px64Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == REG_OP && insn->Operands[1].Type == XMM_OP)
				return TRUE;

			return FALSE;
		}
		
		uint32_t Is_Reg_XMM_Op(Px64Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == XMM_OP && insn->Operands[1].Type == REG_OP)
				return TRUE;

			return FALSE;
		}

		uint32_t Is_Reg_Reg_Op(Px64Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == REG_OP && insn->Operands[1].Type == REG_OP)
				return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Reg_Op(Px64Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == REG_OP && insn->Operands[1].Type == IMM_OP)
				return TRUE;

			return FALSE;
		}

		uint32_t Is_Reg_Displ_Op(Px64Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == REG_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & DISPL && !(MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Displ_Op(Px64Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[1].Type == IMM_OP && insn->Operands[0].Type == MEM_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & DISPL && !(MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Displ_Reg_Op(Px64Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[1].Type == MEM_OP && insn->Operands[0].Type == REG_OP)
				if (MEM_OPERAND(insn->Operands[1].Op)->Flags & DISPL && !(MEM_OPERAND(insn->Operands[1].Op)->Flags & BASE))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Mem_Base_Op(Px64Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == IMM_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE &&
				    !(MEM_OPERAND(insn->Operands[0].Op)->Flags & INDEX))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Imm_Mem_Base_Index_Op(Px64Instruction insn)
		{
			if(insn->NOperands != 2)
				return FALSE;

			if(insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == IMM_OP)
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE &&
				    (MEM_OPERAND(insn->Operands[0].Op)->Flags & INDEX))
					return TRUE;

			return FALSE;
		}

		uint32_t Is_Reg_Mem_Base_Op(Px64Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == REG_OP){
				if (MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE)
					return TRUE;
			}

			return FALSE;
		}



		uint32_t Is_Reg_Mem_Base_Index_Op(Px64Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[0].Type == MEM_OP && insn->Operands[1].Type == REG_OP){
				if ((MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE) && (MEM_OPERAND(insn->Operands[0].Op)->Flags & INDEX))
					return TRUE;
			}

			return FALSE;
		}




		uint32_t Is_Mem_Base_Reg_Op(Px64Instruction insn)
		{
			if (insn->NOperands != 2)
				return FALSE;

			if (insn->Operands[1].Type == MEM_OP && insn->Operands[0].Type == REG_OP){
				if (MEM_OPERAND(insn->Operands[1].Op)->Flags & BASE)
					return TRUE;
			}

			return FALSE;
		}



		uint32_t Is_Mem_Base_Index_Reg_Op(Px64Instruction insn)
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
		 X64 EMIT FUNCTIONS used in x64-asm.def
		 *********************************************************************************
		 */

		void x64_Emit_ADC(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_ADC, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x64_Emit_ALU_Imm_Reg(X64_ALU_ADC, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x64_Emit_ALU_Reg_Mem_Base(
					X64_ALU_ADC,
					insn->Operands[0].Size,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x64_Emit_ALU_Mem_Base_Reg(
					X64_ALU_ADC,
					insn->Operands[0].Size,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ,
					codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x64_Emit_ALU_Displ_Reg(0x13, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x64_Emit_ALU_Reg_Displ(0x11, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, insn->Operands[0].Size, codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_ADD(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_ADD, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x64_Emit_ALU_Imm_Reg(X64_ALU_ADD, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Mem_Base_Op(insn))
				x64_Emit_ALU_Imm_Mem_Base(X64_ALU_ADD, insn->Operands[0].Size, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), 0, codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x64_Emit_ALU_Displ_Reg(X64_ALU_ADD, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x64_Emit_ALU_Reg_Mem_Base(
					X64_ALU_ADD,
					insn->Operands[0].Size,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if(Is_Imm_Displ_Op(insn))
				x64_Emit_ALU_Imm_Displ(
					X64_ALU_ADD,
					insn->Operands[0].Size,
					IMM_OPERAND(insn->Operands[1].Op)->Value,
					MEM_OPERAND(insn->Operands[0].Op)->Displ, 
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x64_Emit_ALU_Mem_Base_Reg(
					X64_ALU_ADD,
					insn->Operands[0].Size,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ,
					codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x64_Emit_ALU_Reg_Displ(X64_ALU_ADD, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_AND(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_AND, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x64_Emit_ALU_Imm_Reg(X64_ALU_AND, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_BSF(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_BSR(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_BSWAP(Px64Instruction insn, uint8_t **codeBuf)
		{
			x64_Emit_BSWAP_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
		}

		void x64_Emit_BT(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_BTC(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_BTR(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_BTS(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CALL(Px64Instruction insn, uint8_t **codeBuf)
		{
			int64_t relAddr;

			if (insn->Operands[0].Type == IMM_OP)
			{
				relAddr = IMM_OPERAND(insn->Operands[0].Op)->Value - (int64_t) *codeBuf - 5;
				
				if (relAddr > INT_MAX || relAddr < INT_MIN)
				    NETVM_ASSERT(1 == 0, "Can't emit relative call")

				x64_Emit_CALL_Relative32(relAddr, codeBuf);
			}

			else if (insn->Operands[0].Type == REG_OP)
			{
				x64_Emit_CALL_Indirect(REG_OPERAND_NAME(insn, 0), codeBuf);
			}
		}

		void x64_Emit_CBW(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CDQ(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CLC(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CLD(Px64Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0xFC;
		}

		void x64_Emit_CMC(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CMOV(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CMP(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_CMP, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if(Is_Imm_Reg_Op(insn)) {
				x64_Emit_ALU_Imm_Reg(X64_ALU_CMP, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			}
			else if(Is_Imm_Mem_Base_Op(insn))
			{
				uint8_t baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				uint32_t displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x64_Emit_ALU_Imm_Mem_Base(X64_ALU_CMP, insn->Operands[0].Size, IMM_OPERAND(insn->Operands[1].Op)->Value, baseReg, displ, codeBuf);
			}
			else if(Is_Imm_Mem_Base_Index_Op(insn))
			{
				uint8_t baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				uint32_t displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				uint8_t indexReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Index.REG_NAME);
				x64_Emit_ALU_Imm_Mem_Index(X64_ALU_CMP, insn->Operands[0].Size, IMM_OPERAND(insn->Operands[1].Op)->Value, baseReg, displ, indexReg, 0, codeBuf);
			}
			else if(Is_Reg_Mem_Base_Op(insn))
			{
				uint8_t srcReg = (uint8_t)REG_OPERAND_NAME(insn, 1);
				uint8_t baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				uint32_t displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x64_Emit_ALU_Reg_Mem_Base(X64_ALU_CMP, insn->Operands[0].Size,srcReg,baseReg, displ, codeBuf);
			}
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CMPS(Px64Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0xF3;
			(*(*codeBuf)++) = 0xa6;
			return;
		}

		void x64_Emit_CMPXHG(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CWD(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_CWDE(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_DEC(Px64Instruction insn, uint8_t **codeBuf)
		{
		   //FIXME: this may conflict with rex.w
			if (insn->Operands[0].Type == REG_OP)
				x64_Emit_DEC_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_DIV(Px64Instruction insn, uint8_t **codeBuf)
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
			x64_Emit_ModRM_Byte(REG_ONLY, 6, REG_OPERAND_NAME(insn, 0), codeBuf);
		}

		void x64_Emit_IDIV(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_IMUL(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_INC(Px64Instruction insn, uint8_t **codeBuf)
		{
		  //FIXME: this may conflict with rex.w
			if (insn->Operands[0].Type == REG_OP)
				x64_Emit_INC_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_J(Px64Instruction insn, uint8_t **codeBuf)
		{
			uint8_t cc = CC_ONLY(insn->getOpcode());
			
			(*(*codeBuf)++) = 0x0F;
			(*(*codeBuf)++) = 0x80 | (cc & 0x0F);	/* [GM] Added parentheses, I hope the correct way ;) */
			x64_Emit_Imm32(0, codeBuf);
		}

		void x64_Emit_JECXZ(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_JMP_Indirect(Px64Instruction insn, uint8_t **bufPtr)
		{

			if (insn->Operands[0].Type == MEM_OP){
				if ((MEM_OPERAND(insn->Operands[0].Op)->Flags & BASE)){
					uint8_t base = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
					uint64_t displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
					x64_Emit_JMP_Indirect_Mem_Base(base, displ, insn->Operands[0].Size, bufPtr);
				}
				else
					NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
			}
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_JMP(Px64Instruction insn, uint8_t **codeBuf)
		{
			if(insn->Operands[0].Type == LBL_OP)
			{
				x64_Emit_JMP_Relative32(0, codeBuf);
			}
			else if (insn->Operands[0].Type == MEM_OP)
			{
				x64_Emit_JMP_Indirect(insn, codeBuf);
			}
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_LEA(Px64Instruction insn, uint8_t **codeBuf)
		{
			uint8_t dst = REG_OPERAND_NAME(insn, 0);
			uint8_t baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
			uint8_t indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
			uint32_t displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
			uint8_t scale =  MEM_OPERAND(insn->Operands[1].Op)->Scale;
			x64_Emit_REX_Prefix(&dst, &baseReg, &indexReg, insn->Operands[0].Size, codeBuf);
			(*(*codeBuf)++) = 0x8D;
			x64_Emit_Reg_Mem_Index(dst, baseReg, displ, indexReg, scale, codeBuf);
		}

		void x64_Emit_LODS(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_LOOP(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_LOOPNZ(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_LOOPZ(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_MOV(Px64Instruction insn, uint8_t **codeBuf)
		{
			uint8_t srcReg, dstReg, baseReg, indexReg;
			uint64_t displ;

			if (Is_Reg_Reg_Op(insn)){
				x64_Emit_MOV_Reg_Reg(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			}
			else if (Is_Reg_Mem_Base_Index_Op(insn)){
				srcReg = (uint8_t)REG_OPERAND_NAME(insn, 1);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[0].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x64_Emit_MOV_Reg_Mem_Index(srcReg, baseReg, displ, indexReg, 0, insn->Operands[1].Size, codeBuf);

			}
			else if (Is_Mem_Base_Index_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOV_Mem_Index_Reg(dstReg, baseReg, displ, indexReg, 0, insn->Operands[0].Size, codeBuf);
			}
			else if (Is_Imm_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				if (insn->load_current_address)
				    IMM_OPERAND(insn->Operands[1].Op)->Value = (uint64_t) *codeBuf;
				x64_Emit_MOV_Imm_Reg(IMM_OPERAND(insn->Operands[1].Op)->Value, dstReg, insn->Operands[0].Size, codeBuf);
			}
			else if (Is_Mem_Base_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOV_Mem_Base_Reg(dstReg, baseReg, displ, insn->Operands[0].Size, codeBuf);

			}
			else if (Is_Reg_Mem_Base_Op(insn)){
				srcReg = (uint8_t)REG_OPERAND_NAME(insn, 1);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x64_Emit_MOV_Reg_Mem_Base(srcReg, baseReg, displ, insn->Operands[0].Size, codeBuf);
			}
			else if(Is_Imm_Mem_Base_Op(insn)){
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x64_Emit_MOV_Imm_Mem_Base(IMM_OPERAND(insn->Operands[1].Op)->Value,	baseReg, displ,	insn->Operands[0].Size, codeBuf);
			} else if(Is_Reg_Displ_Op(insn)) {
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				srcReg = (uint8_t)REG_OPERAND_NAME(insn, 1);
				x64_Emit_MOV_Reg_Displ(srcReg, displ, insn->Operands[1].Size, codeBuf);
			} else if(Is_Displ_Reg_Op(insn)) {
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOV_Displ_Reg(displ, dstReg, insn->Operands[0].Size, codeBuf);
			} else if(Is_Imm_Displ_Op(insn)) {
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x64_Emit_MOV_Imm_Displ(IMM_OPERAND(insn->Operands[1].Op)->Value, displ, insn->Operands[0].Size, codeBuf);
			} else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")

		}

		void x64_Emit_MOVS(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_MOVSX(Px64Instruction insn, uint8_t **codeBuf)
		{
			uint8_t dstReg, baseReg, indexReg;
			uint32_t displ;

			if (Is_Reg_Reg_Op(insn)){
				x64_Emit_MOVSX_Reg_Reg(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[1].Size, codeBuf);
			}
			else if (Is_Mem_Base_Index_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOVSX_Mem_Index_Reg(dstReg, baseReg, displ, indexReg, 0, insn->Operands[1].Size, codeBuf);
			}
			else if(Is_Mem_Base_Reg_Op(insn)){
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOVSX_Mem_Base_Reg(REG_OPERAND_NAME(insn, 0), baseReg, displ, insn->Operands[1].Size, codeBuf);

			} else if(Is_Imm_Displ_Op(insn)) {
				displ = MEM_OPERAND(insn->Operands[0].Op)->Displ;
				x64_Emit_MOV_Imm_Displ(IMM_OPERAND(insn->Operands[1].Op)->Value, displ, insn->Operands[0].Size, codeBuf);
			}
			else if(Is_Displ_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOVSX_Displ_Reg(displ, dstReg, insn->Operands[0].Size, codeBuf);


			}
			else
				NETVM_ASSERT(1 == 0, " CANNOT BE HERE!")
		}

		void x64_Emit_MOVZX(Px64Instruction insn, uint8_t **codeBuf)
		{
			uint8_t dstReg, baseReg, indexReg;
			uint32_t displ;

			if (Is_Reg_Reg_Op(insn)){
				x64_Emit_MOVZX_Reg_Reg(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[1].Size, codeBuf);
			}
			else if (Is_Mem_Base_Index_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				indexReg = (uint8_t) (MEM_OPERAND(insn->Operands[1].Op)->Index.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOVZX_Mem_Index_Reg(dstReg, baseReg, displ, indexReg, 0, insn->Operands[1].Size, codeBuf);
			}
			else if(Is_Mem_Base_Reg_Op(insn)){
				baseReg = (uint8_t)(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOVZX_Mem_Base_Reg(REG_OPERAND_NAME(insn, 0), baseReg, displ, insn->Operands[1].Size, codeBuf);
			}
			else if(Is_Displ_Reg_Op(insn)){
				dstReg = (uint8_t)REG_OPERAND_NAME(insn, 0);
				displ = MEM_OPERAND(insn->Operands[1].Op)->Displ;
				x64_Emit_MOVZX_Displ_Reg(displ, dstReg, insn->Operands[0].Size, codeBuf);


			}
			else
				NETVM_ASSERT(1 == 0, " CANNOT BE HERE!")
		}
		
		void x64_Emit_MOVQ(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_XMM_Reg_Op(insn)){
				x64_Emit_MOVQ_XMM_Reg(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), codeBuf);
			}else if (Is_Reg_XMM_Op(insn)){
				x64_Emit_MOVQ_Reg_XMM(REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), codeBuf);
			}else{
				NETVM_ASSERT(1 == 0, " CANNOT BE HERE!")
			}
		}
			
			

		void x64_Emit_MUL(Px64Instruction insn, uint8_t **codeBuf)
		{
			/*
			 * From Intel Manual
			 * MUL F7 /4
			 * Mod R/M
			 *  MOD = REG_ONLY
			 *  R/M = 4
			 *  r32 = registro
			 */
			uint8_t reg = REG_OPERAND_NAME(insn, 0);
			x64_Emit_REX_Prefix(0, &reg, 0, x64_DWORD, codeBuf);
			(*(*codeBuf)++) = 0xF7;
			x64_Emit_ModRM_Byte(REG_ONLY, 4, reg, codeBuf);
		}

		void x64_Emit_NEG(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
			{
				uint8_t szBit = 0;

				x64OpndSz size = insn->Operands[0].Size;

				if(size == x64_WORD)
					x64_Emit_Prefix(OPER_SIZE, codeBuf);
				else if (size == x64_DWORD)
					szBit = 1;

				(*(*codeBuf)++) = 0xF6 | szBit;
				x64_Emit_ModRM_Byte(REG_ONLY, 3, REG_OPERAND_NAME(insn, 0), codeBuf);
			}
			else
			NETVM_ASSERT(1 == 0, "X64 INSTRUCTION NOT IMPLEMENTED")
		}

		void x64_Emit_NOP(Px64Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0x90;
		}

		void x64_Emit_NOT(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x64_Emit_NOT_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_OR(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_OR, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x64_Emit_ALU_Imm_Reg(X64_ALU_OR, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_POP(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x64_Emit_POP_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_POPFD(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_PUSH(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (insn->Operands[0].Type == REG_OP)
				x64_Emit_PUSH_Reg(REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (insn->Operands[0].Type == MEM_OP)
				x64_Emit_PUSH_Mem_Base(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME, MEM_OPERAND(insn->Operands[0].Op)->Displ, codeBuf);
			else if (insn->Operands[0].Type == IMM_OP)
				x64_Emit_PUSH_Imm(IMM_OPERAND(insn->Operands[0].Op)->Value, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_PUSHFD(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_RET(Px64Instruction insn, uint8_t **codeBuf)
		{
			x64_Emit_Ret(codeBuf);
		}

		void x64_Emit_ROL(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Imm_Reg_Op(insn))
				x64_Emit_SHIFT_Imm_Reg(X64_SHF_ROL, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_ROR(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SAL(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SAR(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_SHIFT_CL_Reg(X64_SHF_SAR, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x64_Emit_SHIFT_Imm_Reg(X64_SHF_SAR, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SBB(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_SBB, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x64_Emit_ALU_Imm_Reg(X64_ALU_SBB, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x64_Emit_ALU_Reg_Mem_Base(
					X64_ALU_SBB,
					x64_DWORD,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ,
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x64_Emit_ALU_Mem_Base_Reg(
					X64_ALU_SBB,
					x64_DWORD,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ,
					codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x64_Emit_ALU_Displ_Reg(0x1B, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x64_Emit_ALU_Reg_Displ(0x19, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, insn->Operands[0].Size, codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SCAS(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SET(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SHL(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Imm_Reg_Op(insn))
				x64_Emit_SHIFT_Imm_Reg(X64_SHF_SHL, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Reg_Op(insn))
				x64_Emit_SHIFT_CL_Reg(X64_SHF_SHL, REG_OPERAND_NAME(insn, 0),  insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SHR(Px64Instruction insn, uint8_t **codeBuf)
		{


			if (Is_Imm_Reg_Op(insn))
				x64_Emit_SHIFT_Imm_Reg(X64_SHF_SHR, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), codeBuf);
			else if (Is_Reg_Reg_Op(insn))
				x64_Emit_SHIFT_CL_Reg(X64_SHF_SHR, REG_OPERAND_NAME(insn, 0),  insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!! " )
		}

		void x64_Emit_STC(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_STD(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_STOS(Px64Instruction insn, uint8_t **codeBuf)
		{
			(*(*codeBuf)++) = 0xF3;
			x64_Emit_REX_Prefix(0,0,0,x64_QWORD, codeBuf);
			(*(*codeBuf)++) = 0xAA;

			//NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_SUB(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_SUB, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Imm_Reg_Op(insn))
				x64_Emit_ALU_Imm_Reg(X64_ALU_SUB, IMM_OPERAND(insn->Operands[1].Op)->Value, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Reg_Mem_Base_Op(insn))
				x64_Emit_ALU_Reg_Mem_Base(
					X64_ALU_SUB,
					insn->Operands[0].Size,
					REG_OPERAND_NAME(insn, 1),
					(MEM_OPERAND(insn->Operands[0].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[0].Op)->Displ, 
					codeBuf);
			else if (Is_Mem_Base_Reg_Op(insn))
				x64_Emit_ALU_Mem_Base_Reg(
					X64_ALU_SUB,
					insn->Operands[0].Size,
					REG_OPERAND_NAME(insn, 0),
					(MEM_OPERAND(insn->Operands[1].Op)->Base.REG_NAME),
					MEM_OPERAND(insn->Operands[1].Op)->Displ, 
					codeBuf);
			else if (Is_Displ_Reg_Op(insn))
				x64_Emit_ALU_Displ_Reg(0x2B, MEM_OPERAND(insn->Operands[1].Op)->Displ, REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else if (Is_Reg_Displ_Op(insn))
				x64_Emit_ALU_Reg_Displ(0x29, REG_OPERAND_NAME(insn, 1), MEM_OPERAND(insn->Operands[0].Op)->Displ, insn->Operands[0].Size, codeBuf);
			else

				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_TEST(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_XADD(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_XCHG(Px64Instruction insn, uint8_t **codeBuf)
		{
			NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_XOR(Px64Instruction insn, uint8_t **codeBuf)
		{
			if (Is_Reg_Reg_Op(insn))
				x64_Emit_ALU_Reg_Reg(X64_ALU_XOR, REG_OPERAND_NAME(insn, 1), REG_OPERAND_NAME(insn, 0), insn->Operands[0].Size, codeBuf);
			else
				NETVM_ASSERT(1 == 0, " X64_INSTRUCTION NOT IMPLEMENTED!!!")
		}

		void x64_Emit_Sw_Table_Entry(Px64Instruction insn, uint8_t **codeBuf)
		{
			(*codeBuf) += 8;
		}

		void x64_Emit_Sw_Table_Entry_Start(Px64Instruction insn, uint8_t **codeBuf)
		{
			Px64Instruction entry = insn->switch_entry;

			x64_Emit_Imm64((uint64_t)(&codeBuf), &entry->emission_address);
		}

		void x64_Emit_SAVEREGS(Px64Instruction insn, uint8_t **codeBuf)
		{
			x64_Emit_PUSH_Reg(HANDLER_STATE_REGISTER, codeBuf);
			x64_Emit_PUSH_Reg(INPUT_PORT_REGISTER, codeBuf);
			x64_Emit_PUSH_Reg(EXCHANGE_BUFFER_REGISTER, codeBuf);
/*			uint32_t caller_save_mask[] = X64_CALLER_SAVE_REGS_MASK;

			for(uint8_t reg = 0; reg < X64_NUM_REGS; reg++)
			{
				if(caller_save_mask[reg] == TRUE) //FIXME: we only have to save live regs
					x64_Emit_PUSH_Reg(reg, codeBuf);
			}*/
		}

		void x64_Emit_LOADREGS(Px64Instruction insn, uint8_t **codeBuf)
		{
			x64_Emit_POP_Reg(EXCHANGE_BUFFER_REGISTER, codeBuf);
			x64_Emit_POP_Reg(INPUT_PORT_REGISTER, codeBuf);
			x64_Emit_POP_Reg(HANDLER_STATE_REGISTER, codeBuf);
/*			uint32_t caller_save_mask[] = X64_CALLER_SAVE_REGS_MASK;

			for(int8_t reg = X64_NUM_REGS - 1; reg >= 0; reg--)
			{
				if(caller_save_mask[reg] == TRUE) //FIXME: we only have to pop live regs
					x64_Emit_POP_Reg(reg, codeBuf);
			}*/
		}

		void x64_Emit_Comment(Px64Instruction insn, uint8_t **codeBuf)
		{
			//do nothing
			return;
		}

        void x64_Emit_RDTSC(Px64Instruction insn, uint8_t **codeBuf)
		{
			x64_emit_RDTSC(codeBuf);
		}

		void emit_prof_timer_sampling(uint8_t **codeBuf, uint64_t *counter)
		{
#ifdef RTE_PROFILE_COUNTERS
			x64_emit_RDTSC(codeBuf);
                        // dark: I am not really sure if x64_DWORD is correct in the first call
			x64_Emit_MOV_Imm_Reg((uint64_t)counter, ECX, x64_DWORD, codeBuf);
			x64_Emit_MOV_Reg_Mem_Base(EAX, ECX, 0, x64_DWORD, codeBuf);
			x64_Emit_MOV_Reg_Mem_Base(EDX, ECX, 4, x64_DWORD, codeBuf);
#endif
		}

		void emit_inc_pkt_counter(uint8_t **codeBuf, uint32_t *counter)
		{
#ifdef RTE_PROFILE_COUNTERS
                        // dark: I am not really sure if x64_DWORD is correct in the first call
                        x64_Emit_MOV_Imm_Reg((uint64_t)counter, ECX, x64_DWORD, codeBuf);
			x64_Emit_ALU_Imm_Mem_Base(X64_ALU_ADD, x64_DWORD, 1, ECX, 0, codeBuf);
#endif
		}

		const std::string x64_Emitter::prop_name("x64_start_offset");

		x64_Emitter::x64_Emitter(CFG<x64Instruction>& cfg, GCRegAlloc<CFG<x64Instruction> >& regAlloc, TraceBuilder<jit::CFG<x64Instruction> >& trace_builder)
			: buffer(NULL), current(NULL),
			cfg(cfg), regAlloc(regAlloc), trace_builder(trace_builder)
		{
		}

		void x64_Emitter::patch_entry(patch_info& pinfo)
		{
			uint8_t* targetAddr = cfg.getBBById(pinfo.destination_id)->getProperty<uint8_t*>(prop_name);
			x64_Emit_Imm64((uint64_t)targetAddr, &pinfo.emission_address);
		}

		void x64_patch_instruction(uint8_t* instrToPatch, uint8_t* targetAddr)
		{
			int32_t relAddr;

			if (*instrToPatch == 0xE9){	//JMP: have to skip 1 byte and rewrite the target
				relAddr =(int32_t) ((int64_t) targetAddr - (int64_t) instrToPatch - 5);
				instrToPatch++;
				x64_Emit_Imm32(relAddr, &instrToPatch);

			}
			else if (*instrToPatch == 0x0F){	//Jxx: have to skip 2 bytes and rewrite the target
				uint8_t *aux = NULL;
				aux = instrToPatch;
				aux++;
				relAddr =(int32_t)((int64_t) targetAddr - (int64_t) instrToPatch - 6);
				instrToPatch += 2;
				x64_Emit_Imm32(relAddr, &instrToPatch);

			}

			assert(1 != 0 && "Wrong instruction to patch pointer");
		}

		void x64_Emitter::patch_jump(patch_info& pinfo)
		{
			uint8_t *instrToPatch, *targetAddr;

			instrToPatch = pinfo.emission_address;
			targetAddr = (pinfo.last ? epilogue : cfg.getBBById(pinfo.destination_id)->getProperty<uint8_t*>(prop_name));

			assert(targetAddr != NULL);

			x64_patch_instruction(instrToPatch, targetAddr);
		}

		uint8_t* x64_Emitter::emit()
		{
			uint32_t npages = (((cfg.get_insn_num() + 200)* MAX_BYTES_PER_INSN) / 4096) + 1;
			uint32_t nbytes = npages * 4096; //Davide: la pagina è sempre da 4096 ?
			current = buffer = (uint8_t*) allocCodePages(nbytes);

			TraceBuilder<jit::CFG<x64Instruction> >::trace_iterator_t t = trace_builder.begin();

			while(t != trace_builder.end())
			{
				emitBB(*t);
				t++;
			}

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

		
		void* x64_Emitter::allocCodePages(size_t size) {
		  #ifdef WIN32
			  void *addr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		      return addr;
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
		
		void x64_Emitter::setPageProtection(void *address, size_t size, bool executableFlag, bool writeableFlag) {
			#ifdef WIN32
			  //On windows the page is already allocated with execution privileges, nothing to do here.
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

		uint32_t x64_Emitter::getActualBufferSize(void)
		{
			if (buffer == NULL)
				return 0;
			return current-buffer;
		}

		x64_Emitter::patch_info::patch_info(x64OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id, bool last)
			: opcode(opcode), emission_address(emission_address), destination_id(destination_id), last(last)
		{
		}

		x64_Emitter::patch_info::patch_info(uint8_t* emission_address, uint16_t destination_id)
			: opcode(x64OpCodesEnum(0)), emission_address(emission_address), destination_id(destination_id), last(false)
		{
		}

		void x64_Emitter::add_jump(x64OpCodesEnum opcode, uint8_t* emission_address, uint16_t destination_id)
		{
			jumps.push_back(patch_info(opcode, emission_address, destination_id));
		}

		void x64_Emitter::add_last_jump(uint8_t* emission_address)
		{
			jumps.push_back(patch_info(X64_JMP, emission_address, 0, true));
		}

		void x64_Emitter::add_jump(x64Instruction* insn, uint8_t* emission_address)
		{
			x64OpCodesEnum opcode = (x64OpCodesEnum)OP_ONLY(insn->getOpcode());

			assert(OP_ONLY(insn->getOpcode()) == X64_J ||
					OP_ONLY(insn->getOpcode()) == X64_JMP ||
					OP_ONLY(insn->getOpcode()) == X64_JECXZ);

			uint16_t destination_id = LBL_OPERAND(insn->Operands[0].Op)->Label;
			add_jump(opcode, emission_address, destination_id);
		}

		void x64_Emitter::add_entry(x64Instruction* insn, uint8_t* emission_address)
		{
			x64OpCodesEnum opcode = (x64OpCodesEnum)OP_ONLY(insn->getOpcode());

			assert(opcode == X64_SW_TABLE_ENTRY);
			entries.push_back(patch_info(emission_address, insn->switch_target));
		}

		void x64_Emitter::emit_insn(x64Instruction* insn, x64Instruction* prev)
		{
			uint16_t opcode = OP_ONLY(insn->getOpcode());
		
		    #ifdef _DEBUG_X64_BACKEND
				std::cout << "emitting instruction at " << (void*)current << " (IR " << (void*)insn << ") " << *insn << " (opcode: " << opcode << " )" << std::endl;
		    #endif

			if(opcode == X64_J || opcode == X64_JMP || opcode == X64_JECXZ)
			{
				if(insn->NOperands >= 1 && (insn->Operands[0].Type == REG_OP || insn->Operands[0].Type == LBL_OP))
				{ 
					//this is not a switch table indirect jump
					if(insn->switch_entry)
					{   
						//this is a target of a jump in a bynary search tree need to patch previous branch
						NETVM_ASSERT(*(current-6) == 0x81, "There is a bug! The previous instruction was not a 'cmp'.")	
						uint8_t* addr = insn->switch_entry->emission_address;					
						uint8_t prevRegister = REG_OPERAND_NAME(prev, 0);
						/*
						*	we have to fix the previous jump, which is 2 instruction above
						*
						*	cmp reg, imm
						*	jge t1, t2		;instruction to patch
						*	cmp reg, imm2	;previous instruction
						*	jge t3, t4		;current instruction
						*
						*	Depends on the fact that the previous cmp has or not the REX, we must go to 7 or 6 bytes above
						*/
						
						uint8_t* target_addr = NULL;
						if(prevRegister>= R8)
							target_addr = current - 7; //the previous instruction usases a register >= R8. As consequence, it has the REX and uses 7B
						else
							target_addr = current -6; //the previous instruction doesn't uses a register >= R8. As consequence, it hasn't the REX and uses 6B
						x64_patch_instruction(addr, target_addr);
					}
					if(!insn->binary_switch_jump)
					{
						//this is a normal jump we need to patch it later
						add_jump(insn, current);
					}
				}
			}

			if(opcode == X64_SW_TABLE_ENTRY && insn->switch_target)
			{
				add_entry(insn, current);
			}

			insn->emission_address = current;
			//cout << "Now emit instruction: " << endl;
			insn->OpDescr->EmitFunct(insn, &current);
		}

		void x64_Emitter::emitBB(bb_t *bb)
		{
			//std::cout << "**********Emitting BB************** " << *bb << std::endl;
			typedef std::list<x64Instruction*>::iterator code_iterator_t;

			//set this node starting address
			bb->setProperty(prop_name, current);

			//emit code
			std::list<x64Instruction*>& code = bb->getCode();
			x64Instruction* insn = NULL, *prev = NULL;
			for(code_iterator_t c = code.begin(); c != code.end(); c++)
			{
						
				insn = *c;
				emit_insn(insn,prev);
				prev = *c;
			}

		}
	} //namespace x64
} //namespace jit


