/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "irnode.h"
#include "registers.h"
#include "netvmjitglobals.h"

#include <list>
#include <set>
#include <iostream>

#define X86_COMMENT_LEN 30

namespace jit{
namespace ia32 {

	//x86 Registers:

	typedef enum{
		AL = 0x00, CL = 0x01, DL = 0x02, BL = 0x03,
		AH = 0x04, CH = 0x05, DH = 0x06, BH	= 0x07
	} x86Regs_8;

	typedef enum{
		AX = 0x00, CX = 0x01, DX = 0x02, BX	= 0x03,
		SP = 0x04, BP = 0x05, SI = 0x06, DI = 0x07
	} x86Regs_16;

	typedef enum
	{
		EAX = 0x00, ECX = 0x01, EDX = 0x02, EBX = 0x03,
		ESP = 0x04, EBP = 0x05, ESI = 0x06, EDI = 0x07
	} x86Regs_32;

#define X86_NUM_REGS		8

#define X86_CALLER_SAVE_REGS_MASK {TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE}
#define X86_CALLEE_SAVE_REGS_MASK {FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, TRUE,  TRUE}

	typedef enum
	{
		O	= 0,		//Overflow
		NO	= 1,		//No Overflow
		B	= 2,		//Below
		AE	= 3,		//Above Or Equal = Not Below
		E	= 4,		//Equal
		NE	= 5,		//Not Equal
		BE	= 6,		//Below Or Equal
		A	= 7,		//Above
		S	= 8,		//Sign
		NS	= 9,		//Not Sign
		P	= 10,		//Parity
		NP	= 11,		//Not Parity
		L	= 12,		//Less Than
		GE	= 13,		//Greater Or Equal
		LE	= 14,		//Less Or Equal
		G	= 15		//Greater
	} x86ConditionCodes;

	// The following macros are used to manage the Jcc opcodes:
	// condition codes are encoded in the 4th byte of the opcode
#define CC_MASK			0xF000
#define OP_CC(OP,CC)	(OP | (CC << 12))
#define OP_ONLY(OP)		(OP & ~CC_MASK)
#define CC_ONLY(OP)		((OP & CC_MASK) >> 12)

	//!register spaces
	typedef enum
	{
		VIRT_SPACE  = 1,
		MACH_SPACE  = 86,
		SPILL_SPACE = 87
	} x86RegSpaces;

#define X86_MACH_REG(R)       (jit::RegisterInstance((uint32_t)MACH_SPACE, (uint32_t)(R), 0))
#define X86_IS_MACH_REG(R)    ((R).get_model()->get_space() == MACH_SPACE)
#define X86_SPILLED_REG(R)    (jit::RegisterInstance((uint32_t)SPILL_SPACE, (uint32_t)(R), 0))
#define X86_IS_SPILLED_REG(R) ((R).get_model()->get_space() == SPILL_SPACE)
#define X86_NEW_VIRT_REG      (jit::RegisterInstance::get_new((uint32_t)VIRT_SPACE))
#define X86_NEW_SPILL_REG     (jit::RegisterInstance::get_new((uint32_t)SPILL_SPACE))

	// X86 opcode prefixes: (usually not used in the machine code emitted by the NetVM JIT)
	typedef enum
	{
		ADDR_SIZE	= 0x67,	//Address Size Prefix
		LOCK		= 0x70, //Lock Prefix
		OPER_SIZE	= 0x66,	//Operand Size Prefix
		CS_OVRRIDE	= 0x2E,	//Code Segment Selector Override
		DS_OVRRIDE	= 0x3E,	//Data Segment Selector Override
		ES_OVRRIDE	= 0x26, //Ext Segment Selector Override
		FS_OVRRIDE	= 0x64, //FS Selector Override
		GS_OVRRIDE	= 0x65, //GS Selector Override
		SS_OVRRIDE	= 0x36  //Stack Segment Selector Override
	} x86PrefixBytes;


	// Macros used by the x86_Asm_MOVSZX_... functions to discriminate between zero or sign extension
	typedef enum
	{
		ZERO_XTEND	= 0,
		SIGN_XTEND	= 1
	}x86Xtend;

	typedef enum
	{
		M_EAX			= 0x0001, 	//The operator modifies EAX
		M_ECX			= 0x0002, 	//The operator modifies ECX
		M_EDX			= 0x0004, 	//The operator modifies EDX
		M_EDI			= 0x0008, 	//The operator modifies EDI
		M_ESI			= 0x0010, 	//The operator modifies ESI
		U_EAX			= 0x0020, 	//The operator uses EAX
		U_ECX			= 0x0040, 	//The operator uses ECX
		U_EDX			= 0x0080, 	//The operator uses EDX
		U_EDI			= 0x0100, 	//The operator uses EDI
		U_ESI			= 0x0200, 	//The operator uses ESI
		NEED_CC			= 0x0400	//Operation based on Condition Code Suffix
/*		DST_OP			= 0x0040,	//Predefined destination operand
		M_EDX			= 0x0080,	//Operation modify EDX reg (32bit div or mul operation)
		DEF_OP			= 0x0100,	//The operation defines an operand
		DEF_2OP			= 0x0200,	//The operation defines two operands
		SHOP			= 0x0400,	//The operation is a shift, so uses ECX
		M_EDI_ECX  		= 0x0800
*/
	} x86OpDescrFlags;

	typedef class _X86_INSTRUCTION x86Instruction, *Px86Instruction;

	//type definition for the function that will emit the x86 machine code to memory:
	typedef void (*X86EmitFunct)(x86Instruction*, uint8_t**);

	//x86 Opcode Descriptor
	typedef struct _X86_OP_DESCR
	{
		char*		Name;			//The literal name of the opcode
		x86OpDescrFlags	Flags;			//Detail flags
		uint8_t		Defs;			//Maximum number of potentially defined regs
		uint8_t		Uses;			//Maximum number of potentially used regs
		X86EmitFunct    EmitFunct;		//Function that will emit the x86 machine code to memory

	} x86OpDescr, *Px86OpDescr;

#define x86_MAX_DEF_REGS	3
#define x86_MAX_USED_REGS	3
	/*
	   x86 Operand's Addressing:
	   Immediate					-> Immediate value
	   Register					-> Value In Register
	   [Displ]						-> Value in Memory at location pointed by immediate value Displ
	   [BaseReg]					-> Value in Memory at location pointed by BaseReg
	   [BaseReg+Displ]				-> Value in Memory at location pointed by BaseReg + Displ
	   [Index*Scale]				-> Value in Memory at location pointed by Index * Scale (Note: Scale can only be 1, 2, 4, 8)
	   [BaseReg+Index+Displ]		-> Value in Memory at location pointed by BaseReg + Index + Displ
	   [BaseReg+Index*Scale+Displ]	-> Value in Memory at location pointed by BaseReg + Index * Scale + Displ
	   */

	//Operand Sizes:
	typedef enum
	{
		x86_DWORD	= 2,
		x86_WORD	= 1,
		x86_BYTE	= 0
	}x86OpndSz;

	//Flags for Memory Operands. They tell which members to consider for the operand addressing
	typedef enum
	{
		BASE	= 0x0001,
		INDEX	= 0x0002,
		DISPL	= 0x0004,
		SCALE	= 0x0008

	}x86MemFlags;

	//Register operand
	typedef RegisterInstance x86RegOpnd, *Px86RegOpnd;

	//Immediate operand
	typedef struct _X86_IMM_OP
	{
		uint32_t	Value;
	} x86ImmOpnd, *Px86ImmOpnd;

	bool operator==(x86ImmOpnd a, x86ImmOpnd b);

	//Label operand (for jumps and branches)
	typedef struct _X86_LBL_OP
	{
		uint32_t	Label;
	} x86LblOpnd, *Px86LblOpnd;

	bool operator==(x86LblOpnd a, x86LblOpnd b);

	//Memory operand
	typedef struct _X86_MEM_OP
	{
		x86RegOpnd	Base;
		x86RegOpnd	Index;
		int32_t		Displ;
		uint8_t	Scale;
		x86MemFlags Flags;
	} x86MemOpnd, *Px86MemOpnd;

	bool operator==(x86MemOpnd a, x86MemOpnd b);

	//X86 operand types:
	typedef enum
	{
		REG_OP	= 0x0001,
		IMM_OP	= 0x0002,
		MEM_OP	= 0x0004,
		LBL_OP	= 0x0008
	} x86OperandTypeEnum;


	//Generic X86 operand. The Type member is used to select the right cast for the Op member
	typedef struct _x86_OPERAND
	{
		x86OpndSz			Size;	//operand size: one between X86_BYTE, X86_WORD, X86_DWORD
		x86OperandTypeEnum	Type;	//operand type: one between REG_OP, IMM_OP, MEM_OP, LBL_OP
		void				*Op;	//pointer to the effective operand description

	} x86Operand, *Px86Operand;

	bool operator==(x86Operand a, x86Operand b);

	std::ostream& operator<<(std::ostream& os, x86Operand& op);

	//Macros that perform the cast to the correct operand pointer from the generic operand
#define MEM_OPERAND(OPND) ((Px86MemOpnd)OPND)
#define REG_OPERAND(OPND) ((Px86RegOpnd)OPND)
#define IMM_OPERAND(OPND) ((Px86ImmOpnd)OPND)
#define LBL_OPERAND(OPND) ((Px86LblOpnd)OPND)

	typedef enum
	{
#define X86_ASM(code, mnemonic, flags, defs, uses, emitfunction, description)	code,
#include "x86-asm.def"
#undef X86_ASM

	}x86OpCodesEnum;

	extern x86OpDescr x86OpDescriptions[];
	extern char *x86CC[];
	extern char *x86RegNames[];
	extern x86OpDescr x86OpDescriptions[];

	//!Generic X86 instruction
	class _X86_INSTRUCTION : public TableIRNode<x86RegOpnd, uint16_t >
	{
		public:

			Px86OpDescr		OpDescr;		//!<Pointer to the operation's Description
			x86Operand		Operands[2];	//!<Array of operands
			uint8_t		NOperands;		//!<Number of operands
			uint8_t* 		emission_address; //!<address of this instruction in memory
			char			Comment[X86_COMMENT_LEN];	//!<optional comment used for debug purposes

			x86Instruction*	switch_entry; //!<pointer to the entry in the jump table
			uint32_t		switch_default_target; //!<label of the default target of this switch
			bool			table_jump; //!<true if this is an indirect jump using a table

			std::set<RegType> getUses();
			std::set<RegType> getDefs();

			_X86_INSTRUCTION(uint16_t opcode)
			: TableIRNode<x86RegOpnd, uint16_t > (opcode, 0),
			  OpDescr(&x86OpDescriptions[OP_ONLY(opcode)]),
			  NOperands(0),
			  emission_address(0),
			  switch_entry(NULL),
			  switch_default_target(0),
			  table_jump(false)
			  //Removed(false)
			{
				Comment[0] = '\0';
			}

			void rewrite_destination(uint16_t, uint16_t) { assert(1 == 0 && "not implemented"); }
			void rewrite_use(RegType oldreg, RegType newreg) { assert (1 == 0 && "not_implemented"); }
			RegType* getOwnReg() { assert(1 == 0 && "not_implemented"); return NULL;}
			void setDefReg(RegType r) {assert(1 == 0 && "not implemented"); }

			void rewrite_Reg(RegType oldreg , RegType * newreg);

			//method exported for the register allocation algorithm
			bool isCopy() const;
			RegType get_from() const;
			RegType get_to() const;

			std::list< std::pair<RegType, RegType> > getCopiedPair();
			void printNode(std::ostream& os, bool SSAform = false);

			class _X86_INSTRUCTION_IT
			{
				private:
					_X86_INSTRUCTION *ptr;
				public:
					typedef _X86_INSTRUCTION* value_type;
					typedef ptrdiff_t difference_type;
					typedef _X86_INSTRUCTION** pointer;
					typedef _X86_INSTRUCTION*& reference;
					typedef std::forward_iterator_tag iterator_category;

					_X86_INSTRUCTION_IT(_X86_INSTRUCTION* insn = NULL): ptr(insn) { }
					_X86_INSTRUCTION_IT(const _X86_INSTRUCTION_IT &it): ptr(it.ptr) {}
					value_type& operator*();
					value_type& operator->();
					_X86_INSTRUCTION_IT& operator++(int);
					bool operator==(const _X86_INSTRUCTION_IT& it) const;
					bool operator!=(const _X86_INSTRUCTION_IT& it) const;
			};

			typedef class _X86_INSTRUCTION_IT IRNodeIterator;

			_X86_INSTRUCTION_IT nodeBegin();
			_X86_INSTRUCTION_IT nodeEnd();

			virtual bool has_side_effects();
	};

	std::ostream& operator<<(std::ostream& os, _X86_INSTRUCTION& insn);

	typedef std::list<Px86Instruction>& Px86InsnSequence;

	Px86Instruction x86_Asm_Comment(Px86InsnSequence x86InsnSeq, const char *comment);

	//only an operand
	Px86Instruction x86_Asm_Op(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode);
	Px86Instruction x86_Asm_Op_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Imm(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal);
	Px86Instruction x86_Asm_Op_Mem_Base(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, uint32_t displ, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Mem_Index(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, x86RegOpnd indexReg, x86OpndSz opSize);

	//operand -> reg
	Px86Instruction x86_Asm_Op_Reg_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86RegOpnd dstReg, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Imm_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, x86RegOpnd dstReg, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Mem_Displ_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t displ, x86RegOpnd dstReg, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Mem_Base_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, uint32_t displ, x86RegOpnd dstReg, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Mem_Index_To_Reg(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd baseReg, x86RegOpnd indexReg, uint32_t displ, uint8_t scale, x86RegOpnd dstReg, x86OpndSz opSize);

	//operand -> mem_base
	Px86Instruction x86_Asm_Op_Reg_To_Mem_Base(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86RegOpnd baseReg, uint32_t displ, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Imm_To_Mem_Base(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, x86RegOpnd baseReg, uint32_t displ, x86OpndSz opSize);

	//operand -> mem_index
	Px86Instruction x86_Asm_Op_Reg_To_Mem_Index(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, x86RegOpnd baseReg, x86RegOpnd indexReg, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Imm_To_Mem_Index(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, x86RegOpnd baseReg, x86RegOpnd indexReg, x86OpndSz opSize);

	//operand -> mem_displ
	Px86Instruction x86_Asm_Op_Reg_To_Mem_Displ(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, x86RegOpnd srcReg, uint32_t displ, x86OpndSz opSize);
	Px86Instruction x86_Asm_Op_Imm_To_Mem_Displ(Px86InsnSequence x86CodeSeq, x86OpCodesEnum x86OpCode, uint32_t immVal, uint32_t displ, x86OpndSz opSize);

	//jumps
	Px86Instruction x86_Asm_JMP_Label(Px86InsnSequence x86CodeSeq, uint32_t basicBlockLabel);
	Px86Instruction x86_Asm_J_Label(Px86InsnSequence x86CodeSeq, uint8_t condCode, uint32_t basicBlockLabel);
	Px86Instruction x86_Asm_CALL_NetVM_API(Px86InsnSequence x86CodeSeq, void *functAddress);
	Px86Instruction x86_Asm_JMP_Indirect(Px86InsnSequence x86CodeSeq, x86RegOpnd baseReg, uint32_t displ, x86OpndSz size);

	//table entries
	Px86Instruction x86_Asm_JMP_Table_Entry(Px86InsnSequence x86CodeSeq, x86RegOpnd baseReg, uint32_t displ, x86OpndSz size);
	Px86Instruction x86_Asm_Switch_Table_Entry(Px86InsnSequence x86CodeSeq, uint32_t target = 0);
	Px86Instruction x86_Asm_Switch_Table_Entry_Start(Px86InsnSequence x86CodeSeq, Px86Instruction entry);

	int32_t x86_Asm_Append_RegOp(Px86Instruction insn, x86RegOpnd reg, x86OpndSz size);
	int32_t x86_Asm_Append_MemOp(Px86Instruction insn, x86RegOpnd base, x86RegOpnd index, uint32_t displ, uint8_t scale,
			x86OpndSz size, x86MemFlags memFlags);

	Px86Instruction x86_Asm_New_Op(x86OpCodesEnum code);

	void x86_Asm_Append_Comment(Px86Instruction insn, const char *comment);

} //namespace ia32
} //namespace jit


