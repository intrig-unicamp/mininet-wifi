/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

#include "irnode.h"
#include "registers.h"
#include "netvmjitglobals.h"

#include <cstddef>	// Added for catching an error on missing def for ptrdiff_t on Ubuntu 11.10
#include <list>
#include <set>
#include <iostream>

#define X64_COMMENT_LEN 30

namespace jit{
namespace x64 {

	//x64 Registers:

	typedef enum{
		AL = 0x00, CL = 0x01, DL = 0x02, BL = 0x03,
		AH = 0x04, CH = 0x05, DH = 0x06, BH = 0x07,
		R8L  = 0x08, R9L  = 0x09, R10L = 0x0A, R11L = 0x0B,
		R12L = 0x0C, R13L = 0x0D, R14L = 0x0E ,R15L = 0x0F
	} x64Regs_8;

	typedef enum{
		AX = 0x00, CX = 0x01, DX = 0x02, BX = 0x03,
		SP = 0x04, BP = 0x05, SI = 0x06, DI = 0x07,
		R8W  = 0x08, R9W  = 0x09, R10W = 0x0A, R11W = 0x0B,
		R12W = 0x0C, R13W = 0x0D, R14W = 0x0E ,R15W = 0x0F
	} x64Regs_16;

	typedef enum
	{
		EAX = 0x00, ECX = 0x01, EDX = 0x02, EBX = 0x03,
		ESP = 0x04, EBP = 0x05, ESI = 0x06, EDI = 0x07,
		R8D  = 0x08, R9D  = 0x09, R10D = 0x0A, R11D = 0x0B,
		R12D = 0x0C, R13D = 0x0D, R14D = 0x0E ,R15D = 0x0F
	} x64Regs_32;


	typedef enum
	{
		RAX = 0x00, RCX = 0x01, RDX = 0x02, RBX = 0x03,
		RSP = 0x04, RBP = 0x05, RSI = 0x06, RDI = 0x07,
		R8  = 0x08, R9  = 0x09, R10 = 0x0A, R11 = 0x0B,
		R12 = 0x0C, R13 = 0x0D, R14 = 0x0E ,R15 = 0x0F
	} x64Regs_64;

#define X64_NUM_REGS		16

#ifdef _WIN64
	#define EXCHANGE_BUFFER_REGISTER RCX
	#define COPRO_STATE_REGISTER RCX
    #define INPUT_PORT_REGISTER RDX
	#define COPRO_OPERATION_REGISTER RDX
    #define HANDLER_STATE_REGISTER R8
	#define X64_CALLER_SAVE_REGS_MASK {TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE,FALSE, FALSE, \
		                               TRUE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE }
	#define X64_CALLEE_SAVE_REGS_MASK {FALSE, FALSE, FALSE, TRUE,  FALSE, TRUE,  TRUE,  TRUE, \
									   FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,  TRUE,  TRUE}
#else
	#define EXCHANGE_BUFFER_REGISTER RDI
	#define COPRO_STATE_REGISTER RDI
    #define INPUT_PORT_REGISTER	RSI
	#define COPRO_OPERATION_REGISTER RSI
    #define HANDLER_STATE_REGISTER RDX
	#define X64_CALLER_SAVE_REGS_MASK {TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, TRUE,  TRUE, \
		                               TRUE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE }
	#define X64_CALLEE_SAVE_REGS_MASK {FALSE, FALSE, FALSE, TRUE,  FALSE, TRUE,  FALSE, FALSE, \
									   FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,  TRUE,  TRUE}
#endif


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
	} x64ConditionCodes;

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
	} x64RegSpaces;

#define X64_MACH_REG(R)       (jit::RegisterInstance((uint32_t)MACH_SPACE, (uint32_t)(R), 0))
#define X64_IS_MACH_REG(R)    ((R).get_model()->get_space() == MACH_SPACE)
#define X64_SPILLED_REG(R)    (jit::RegisterInstance((uint32_t)SPILL_SPACE, (uint32_t)(R), 0))
#define X64_IS_SPILLED_REG(R) ((R).get_model()->get_space() == SPILL_SPACE)
#define X64_NEW_VIRT_REG      (jit::RegisterInstance::get_new((uint32_t)VIRT_SPACE))
#define X64_NEW_SPILL_REG     (jit::RegisterInstance::get_new((uint32_t)SPILL_SPACE))

	// X64 opcode prefixes: (usually not used in the machine code emitted by the NetVM JIT)
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
		SS_OVRRIDE	= 0x36, //Stack Segment Selector Override
		REXW		= 0x48 //64bit register Prefix
	} x64PrefixBytes;


	// Macros used by the x64_Asm_MOVSZX_... functions to discriminate between zero or sign extension
	typedef enum
	{
		ZERO_XTEND	= 0,
		SIGN_XTEND	= 1
	}x64Xtend;

	typedef enum
	{
		M_EAX			= 0x0001, 	//The operator modifies EAX
		M_ECX			= 0x0002, 	//The operator modifies ECX
		M_EDX			= 0x0004, 	//The operator modifies EDX
		M_EDI			= 0x0008, 	//The operator modifies EDI
		M_ESI			= 0x0010, 	//The operator modifies ESI
		M_R8			= 0x1000,	//The operator modifies R8
		M_R9			= 0x2000,	//The operator modifies R9
		M_R10			= 0x4000,	//The operator modifies R10
		M_R11			= 0x8000,	//The operator modifies R11
		U_EAX			= 0x0020, 	//The operator uses EAX
		U_ECX			= 0x0040, 	//The operator uses ECX
		U_EDX			= 0x0080, 	//The operator uses EDX
		U_EDI			= 0x0100, 	//The operator uses EDI
		U_ESI			= 0x0200, 	//The operator uses ESI
		NEED_CC			= 0x0400	//Operation based on Condition Code Suffix
	} x64OpDescrFlags;

	typedef class _X64_INSTRUCTION x64Instruction, *Px64Instruction;

	//type definition for the function that will emit the x64 machine code to memory:
	typedef void (*X64EmitFunct)(x64Instruction*, uint8_t**);

	//x64 Opcode Descriptor
	typedef struct _X64_OP_DESCR
	{
		char*		Name;			//The literal name of the opcode
		x64OpDescrFlags	Flags;			//Detail flags
		uint8_t		Defs;			//Maximum number of potentially defined regs
		uint8_t		Uses;			//Maximum number of potentially used regs
		X64EmitFunct    EmitFunct;		//Function that will emit the x64 machine code to memory

	} x64OpDescr, *Px64OpDescr;

#define x64_MAX_DEF_REGS	3
#define x64_MAX_USED_REGS	3
	/*
	   x64 Operand's Addressing:
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
		x64_QWORD	= 3,
		x64_DWORD	= 2,
		x64_WORD	= 1,
		x64_BYTE	= 0
	}x64OpndSz;

	//Flags for Memory Operands. They tell which members to consider for the operand addressing
	typedef enum
	{
		BASE	= 0x0001,
		INDEX	= 0x0002,
		DISPL	= 0x0004,
		SCALE	= 0x0008

	}x64MemFlags;

	//Register operand
	typedef RegisterInstance x64RegOpnd, *Px64RegOpnd;

	//Immediate operand
	typedef struct _X64_IMM_OP
	{
		uint64_t	Value;
	} x64ImmOpnd, *Px64ImmOpnd;


	bool operator==(x64ImmOpnd a, x64ImmOpnd b);

	//Label operand (for jumps and branches)
	typedef struct _X64_LBL_OP
	{
		uint32_t	Label;
	} x64LblOpnd, *Px64LblOpnd;

	bool operator==(x64LblOpnd a, x64LblOpnd b);

	//Memory operand
	typedef struct _X64_MEM_OP
	{
		x64RegOpnd	Base;
		x64RegOpnd	Index;
		int64_t		Displ;
		uint8_t	Scale;
		x64MemFlags Flags;
	} x64MemOpnd, *Px64MemOpnd;

	bool operator==(x64MemOpnd a, x64MemOpnd b);

	//X64 operand types:
	typedef enum
	{
		REG_OP	= 0x0001,
		IMM_OP	= 0x0002,
		MEM_OP	= 0x0004,
		LBL_OP	= 0x0008,
		XMM_OP	= 0x0010 
	} x64OperandTypeEnum;


	//Generic X64 operand. The Type member is used to select the right cast for the Op member
	typedef struct _x64_OPERAND
	{
		x64OpndSz			Size;	//operand size: one between X64_BYTE, X64_WORD, X64_DWORD
		x64OperandTypeEnum	Type;	//operand type: one between REG_OP, IMM_OP, MEM_OP, LBL_OP
		void				*Op;	//pointer to the effective operand description

	} x64Operand, *Px64Operand;

	bool operator==(x64Operand a, x64Operand b);

	std::ostream& operator<<(std::ostream& os, x64Operand& op);

	//Macros that perform the cast to the correct operand pointer from the generic operand
#define MEM_OPERAND(OPND) ((Px64MemOpnd)OPND)
#define REG_OPERAND(OPND) ((Px64RegOpnd)OPND)
#define IMM_OPERAND(OPND) ((Px64ImmOpnd)OPND)
#define LBL_OPERAND(OPND) ((Px64LblOpnd)OPND)

	typedef enum
	{
#define X64_ASM(code, mnemonic, flags, defs, uses, emitfunction, description)	code,
#include "x64-asm.def"
#undef X64_ASM

	}x64OpCodesEnum;

	extern x64OpDescr x64OpDescriptions[];
	extern char *x64CC[];
	extern char *x64RegNames[];
	extern x64OpDescr x64OpDescriptions[];

	//!Generic X64 instruction
	class _X64_INSTRUCTION : public TableIRNode<x64RegOpnd, uint16_t >
	{
		public:

			Px64OpDescr		OpDescr;		//!<Pointer to the operation's Description
			x64Operand		Operands[2];	//!<Array of operands
			uint8_t		NOperands;		//!<Number of operands
			uint8_t* 		emission_address; //!<address of this instruction in memory
			char			Comment[X64_COMMENT_LEN];	//!<optional comment used for debug purposes

			x64Instruction*	switch_entry; //!<pointer to the entry in the jump table
			uint32_t		switch_target; //!<label of the target of this case of the switch
			bool			binary_switch_jump; //!<true if this is an binary switch jump
			bool			load_current_address; //!<true if this is a mov that load current emission address added in a register (used in switch)

			std::set<RegType> getUses();
			std::set<RegType> getDefs();

			_X64_INSTRUCTION(uint16_t opcode)
			: TableIRNode<x64RegOpnd, uint16_t > (opcode, 0),
			  OpDescr(&x64OpDescriptions[OP_ONLY(opcode)]),
			  NOperands(0),
			  emission_address(0),
			  switch_entry(NULL),
			  switch_target(0),
			  binary_switch_jump(false),
			  load_current_address(false)
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

			class _X64_INSTRUCTION_IT
			{
				private:
					_X64_INSTRUCTION *ptr;
				public:
					typedef _X64_INSTRUCTION* value_type;
					typedef ptrdiff_t difference_type;
					typedef _X64_INSTRUCTION** pointer;
					typedef _X64_INSTRUCTION*& reference;
					typedef std::forward_iterator_tag iterator_category;

					_X64_INSTRUCTION_IT(_X64_INSTRUCTION* insn = NULL): ptr(insn) { }
					_X64_INSTRUCTION_IT(const _X64_INSTRUCTION_IT &it): ptr(it.ptr) {}
					value_type& operator*();
					value_type& operator->();
					_X64_INSTRUCTION_IT& operator++(int);
					bool operator==(const _X64_INSTRUCTION_IT& it) const;
					bool operator!=(const _X64_INSTRUCTION_IT& it) const;
			};

			typedef class _X64_INSTRUCTION_IT IRNodeIterator;

			_X64_INSTRUCTION_IT nodeBegin();
			_X64_INSTRUCTION_IT nodeEnd();

			virtual bool has_side_effects();
	};

	std::ostream& operator<<(std::ostream& os, _X64_INSTRUCTION& insn);

	typedef std::list<Px64Instruction>& Px64InsnSequence;

	Px64Instruction x64_Asm_Comment(Px64InsnSequence x64InsnSeq, const char *comment);

	//only an operand
	Px64Instruction x64_Asm_Op(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode);
	Px64Instruction x64_Asm_Op_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Imm(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal);
	Px64Instruction x64_Asm_Op_Mem_Base(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, uint32_t displ, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Mem_Index(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, x64RegOpnd indexReg, x64OpndSz opSize);

	//operand -> reg
	Px64Instruction x64_Asm_Op_Reg_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64RegOpnd dstReg, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Imm_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint64_t immVal, x64RegOpnd dstReg, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Mem_Displ_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t displ, x64RegOpnd dstReg, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Mem_Base_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, uint32_t displ, x64RegOpnd dstReg, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Mem_Index_To_Reg(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd baseReg, x64RegOpnd indexReg, uint32_t displ, uint8_t scale, x64RegOpnd dstReg, x64OpndSz opSize);

	//operand -> mem_base
	Px64Instruction x64_Asm_Op_Reg_To_Mem_Base(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64RegOpnd baseReg, uint32_t displ, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Imm_To_Mem_Base(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal, x64RegOpnd baseReg, uint32_t displ, x64OpndSz opSize);

	//operand -> mem_index
	Px64Instruction x64_Asm_Op_Reg_To_Mem_Index(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, x64RegOpnd baseReg, x64RegOpnd indexReg, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Imm_To_Mem_Index(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal, x64RegOpnd baseReg, x64RegOpnd indexReg, x64OpndSz opSize);

	//operand -> mem_displ
	Px64Instruction x64_Asm_Op_Reg_To_Mem_Displ(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, x64RegOpnd srcReg, uint64_t displ, x64OpndSz opSize);
	Px64Instruction x64_Asm_Op_Imm_To_Mem_Displ(Px64InsnSequence x64CodeSeq, x64OpCodesEnum x64OpCode, uint32_t immVal, uint64_t displ, x64OpndSz opSize);

	//jumps
	Px64Instruction x64_Asm_JMP_Label(Px64InsnSequence x64CodeSeq, uint32_t basicBlockLabel);
	Px64Instruction x64_Asm_J_Label(Px64InsnSequence x64CodeSeq, uint8_t condCode, uint32_t basicBlockLabel);
	Px64Instruction x64_Asm_CALL_NetVM_API(Px64InsnSequence x64CodeSeq, void *functAddress);
	Px64Instruction x64_Asm_JMP_Indirect(Px64InsnSequence x64CodeSeq, x64RegOpnd baseReg, uint32_t displ, x64OpndSz size);

	//table entries
	//Px64Instruction x64_Asm_JMP_Table_Entry(Px64InsnSequence x64CodeSeq, x64RegOpnd baseReg, uint32_t displ, x64OpndSz size);
	
	Px64Instruction x64_Asm_Load_Current_Memory_Location(Px64InsnSequence x64CodeSeq, x64RegOpnd dstReg);
	Px64Instruction x64_Asm_Switch_Table_Entry(Px64InsnSequence x64CodeSeq, uint32_t target = 0);
	Px64Instruction x64_Asm_Switch_Table_Entry_Start(Px64InsnSequence x64CodeSeq, Px64Instruction entry);

	int32_t x64_Asm_Append_RegOp(Px64Instruction insn, x64RegOpnd reg, x64OpndSz size);
	int32_t x64_Asm_Append_XMMOp(Px64Instruction insn, x64RegOpnd reg);
	int32_t x64_Asm_Append_MemOp(Px64Instruction insn, x64RegOpnd base, x64RegOpnd index, uint64_t displ, uint8_t scale,
			x64OpndSz size, x64MemFlags memFlags);

	Px64Instruction x64_Asm_New_Op(x64OpCodesEnum code);

	void x64_Asm_Append_Comment(Px64Instruction insn, const char *comment);

} //namespace x64
} //namespace jit

