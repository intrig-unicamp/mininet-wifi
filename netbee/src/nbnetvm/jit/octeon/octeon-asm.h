/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file octeon-asm.h
 * \brief this file contains declaration of classes representing an octeon instruction
 */
#ifndef OCTEON_ASM_H
#define OCTEON_ASM_H

#include "irnode.h"
#include "registers.h"
#include "octeon-offset.h"
#include "gc_regalloc.h"
#include <list>
#include <map>

namespace jit {
	//!namespace containing all definitions of octeon backend
	namespace octeon {

		class octeon_Insn; //forward declaration

		//!enumeration of octeon registers
		typedef enum{
			ZERO = 0,
			AT,
			V0, V1,
			A0, A1, A2, A3,
			T0, T1, T2, T3, T4, T5, T6, T7,
			S0, S1, S2, S3, S4, S5, S6, S7,
			T8, T9,
			KT0, KT1,
			GP,
			SP,
			FP,
			RA,
			HI, LO //results of multiply between 64 bit values
		} octeonRegs;

		//!type used to represent a register
		class octeonRegType : public RegisterInstance
		{
			public:

			octeonRegType(const RegisterInstance& reg = RegisterInstance::invalid);
			octeonRegType(uint32_t space, uint32_t name, uint32_t version = 0);

			static octeonRegType mach_reg(octeonRegs reg);
			static octeonRegType virt_reg(uint32_t name);
			static octeonRegType new_virt_reg();
			static octeonRegType spill_reg(uint32_t name);
			static octeonRegType new_spill_reg();
			static void init_machine_regs(std::list<octeonRegType>& machine_regs);
			static void init_virtual_regs(std::list<octeonRegType>& virtual_regs);
			static void init_colors(std::list<octeonRegType>& colors);
			static void rename_regs(std::list<octeonRegType>& virtualRegisters, GCRegAlloc<CFG<octeon_Insn> >& regAlloc);

			bool is_spill_register() const;
			bool is_mach_register() const;
			bool is_virt_register() const;

			//!number of registers on octeon machine
			static const uint32_t num_mach_regs;
			//!registers a called procedure should save
			static const octeonRegs calle_save_regs[];
			//!registers a calling procedure should save
			static const octeonRegs caller_save_regs[];
			static const octeonRegs avaible[];
			static const octeonRegs precolored[];

			friend std::ostream& operator<<(std::ostream& os, const octeonRegType& reg);

			//!number of the space of machine registers
			static const uint32_t MachSpace;

			//!number of the space of spilled registers
			static const uint32_t SpillSpace;

			//!number of the space of virtual registers
			static const uint32_t VirtSpace;

			//!registers names
			static const char* octeonRegsNames[];
		};

		
		//!flags of a instruction
		typedef enum
		{
			DEF_OP		= 0x0001,	//!<The operation defines an operand	
			IMM         = 0x0002,	//!<The operation uses an immediate
			LBL			= 0x0004,	//!<The operation uses a label operand
			DEF_H		= 0x0008,	//!<The operation defines register HI
			DEF_L		= 0x000a,	//!<The operation defines register LO
			USE_H		= 0x000c,	//!<The operation uses register HI
			USE_L		= 0x000e,	//!<The operation uses register LO
			DEF_RA		= 0x0010	//!<The operation defines register RA
		} octeonOpDescrFlags;

		//!<struct containg information about an octeon instruction
		struct octeonInsDescr
		{
			char		*Name;			//!<The literal name of the opcode								
			uint32_t	Flags;			//!<Detail flags
			uint8_t	Defs;			//!<Defined operands
			uint8_t	Uses;			//!<used operands
		};

		//!vector containing information about all octeon instruction
		extern const struct octeonInsDescr octeonInsDescription[];

		//!property name of the instruction number counter
		extern const std::string count_prop_name;

		typedef enum _octeon_opcodes
		{
#define OCTEON_ASM(code, mnemonic, flags, defs, uses, description)	code,
#include "octeon-asm.def"
#undef OCTEON_ASM
		} octeonOpcodes;

		//!enumeration of operands types
		typedef enum _octeon_operand_type
		{
			REG_OP,
			IMM_OP,
			MEM_OP,
			LABEL_OP
		} octeonOpType;

		//!enumeration of operands sizes
		typedef enum _octeon_operand_size
		{
			OC_BYTE,
			OC_HWORD,
			OC_WORD,
			OC_DWORD,
			OC_NOSIZE
		} octeonOpSize;

		//!class representing a generic octeon instruction operand
		class octeonOperand
		{
			public:

			//!constructor
			octeonOperand(octeonOpType type, octeonOpSize size = OC_NOSIZE);
			//!destructor
			virtual ~octeonOperand() {}

			//!get this operand type
			octeonOpType get_type() const;
			//!get this operand size
			octeonOpSize get_size() const;

			virtual std::ostream& print(std::ostream& os) const = 0;

			friend std::ostream& operator<<(std::ostream& os, const octeonOperand& operand);

			private:
			octeonOpType type; //!<type of this operand
			octeonOpSize size; //!<size of this operand
		};

		class octeonLabelOp: public octeonOperand
		{
			public:

			octeonLabelOp(uint32_t target = 0);
			std::ostream& print(std::ostream& os) const;

			uint32_t get_label() const;

			private:
			uint32_t label;
		};

		//!class representin a immediate operand
		class octeonImmOp : public octeonOperand
		{
			public:

			octeonImmOp(uint32_t value);
			std::ostream& print(std::ostream& os) const;

			uint32_t get_value() const;

			private:
			int32_t value;
		};

		//!class representin a register operand
		class octeonRegOp : public octeonOperand
		{
			public:

			octeonRegOp(octeonRegType& reg, octeonOpSize size);
			octeonRegOp();
			std::ostream& print(std::ostream& os) const;

			const octeonRegType& get_reg() const;

			private:
			octeonRegType reg;
		};

		//!class representing a memory operand
		class octeonMemOp : public octeonOperand
		{
			public:
			octeonMemOp(uint32_t displ, octeonRegType& base, octeonOpSize size);
			octeonMemOp(std::string& displ, octeonRegType& base, octeonOpSize size);
			std::ostream& print(std::ostream& os) const;
			
			uint32_t get_displ() const;
			const std::string& get_str_displ() const;
			const octeonRegType& get_base() const;

			private:
			uint32_t displ;
			std::string str_displ;
			octeonRegType base;
		};

		class octeonJumpTable
		{
			public:
				typedef std::list<octeonLabelOp> target_list_t;
				octeonJumpTable(std::string& name);
				std::ostream& print(std::ostream& os) const;
				const std::string& getName() const;
				void addTarget(uint16_t target);
				
			private:
				target_list_t targets;
				std::string name;
		};

		//!class representing a generic octeon instruction
		class octeon_Insn : public  TableIRNode<octeonRegType, octeonOpcodes>
		{
			public:
				static const uint32_t max_bytes_per_insn;
				static const uint32_t max_near_jump_offset;

				bool test_flag(octeonOpDescrFlags flag) const;

				class IRNodeIterator;

				IRNodeIterator nodeBegin();
				IRNodeIterator nodeEnd();

				virtual std::ostream& print(std::ostream& os) const;
				std::ostream& printNode(std::ostream& os, bool inSSA = false) const;

				virtual bool isLongJump(CFG<octeon_Insn>& cfg, uint32_t count) const;
				virtual void toLongJump();

				virtual bool has_side_effects();
				virtual bool isCopy() const;
				virtual std::list< std::pair <RegType, RegType> > getCopiedPair() const;
				virtual RegType get_to() const;
				virtual RegType get_from() const;
				virtual std::set<octeonRegType> getUses();
				virtual std::set<octeonRegType> getDefs();

				octeon_Insn(octeonOpcodes opcode = OCTEON_INVALID);

				friend std::ostream& operator<<(std::ostream& os, const octeon_Insn& insn);

				//function for register mapping. see todo in register mapping
				void rewrite_destination(uint16_t, uint16_t) { assert(1 == 0 && "not implemented"); }
				void rewrite_use(RegType oldreg, RegType newreg) { assert (1 == 0 && "not_implemented"); }
				RegType* getOwnReg() { assert(1 == 0 && "not_implemented"); return NULL;}
				void setDefReg(RegType r) {assert(1 == 0 && "not implemented"); }
		};

		class octeon_Insn::IRNodeIterator
		{
			private: 
				octeon_Insn *ptr;
			public:
				typedef octeon_Insn* value_type;
				typedef ptrdiff_t difference_type;
				typedef octeon_Insn** pointer;
				typedef octeon_Insn*& reference;
				typedef std::forward_iterator_tag iterator_category;

				IRNodeIterator(octeon_Insn* insn = NULL);
				IRNodeIterator(const IRNodeIterator &it);
				value_type& operator*();
				value_type& operator->();
				IRNodeIterator& operator++(int);
				bool operator==(const IRNodeIterator& it) const;
				bool operator!=(const IRNodeIterator& it) const;
		};

		class octeonStartTicks : public octeon_Insn
		{
			public:
				octeonStartTicks(const std::string& pe_name);
				std::ostream& print(std::ostream& os) const;
			private:
				std::string pe_name;
		};

		class octeonStopTicks : public octeon_Insn
		{
			public:
				octeonStopTicks(const std::string& pe_name, octeonRegType& reg);
				std::ostream& print(std::ostream& os) const;
				std::set<octeonRegType> getDefs();
			private:
				std::string pe_name;
				octeonRegOp op;
		};

		class octeonLabel: public octeon_Insn
		{
			public:
				octeonLabel(const std::string& label);
				std::ostream& print(std::ostream& os) const;
			private:
			std::string label;
		};

		class octeonBranchJumpTable: public octeon_Insn
		{
			public:
			std::ostream& print(std::ostream& os) const;
			octeonBranchJumpTable(octeonRegType& offset, std::string& table_name);
			std::set<octeonRegType> getUses();

			private:
			octeonRegType offset;
			std::string table_name;
		};

		class octeonUnOp: public octeon_Insn
		{
			public:
			std::ostream& print(std::ostream& os) const;
			octeonUnOp(octeonOpcodes opcode, octeonRegType& op, octeonOpSize size = OC_DWORD);
			std::set<octeonRegType> getUses();

			private:
			octeonRegOp operand;
		};

		class octeonRet: public octeon_Insn
		{
			public:
				std::ostream& print(std::ostream& os) const;
				octeonRet();
		};

		class octeonSendPkt: public octeon_Insn
		{
			public:
				std::ostream& print(std::ostream& os) const;
				octeonSendPkt(uint32_t port_number);

			private:
				uint32_t port_number;
		};

		//!class representing a load instruction
		class octeonLoad : public octeon_Insn
		{
			public:
			std::set<octeonRegType> getUses();
			std::set<octeonRegType> getDefs();
			std::ostream& print(std::ostream& os) const;

			octeonLoad(octeonOpcodes opcode, octeonRegType& base, uint32_t displ, octeonRegType& dst, octeonOpSize size = OC_DWORD);
			octeonLoad(octeonOpcodes opcode, octeonRegType& base, std::string& displ, octeonRegType& dst, octeonOpSize size = OC_DWORD);

			private:
			octeonMemOp from;
			octeonRegOp to;
		};

		//!class representing a store instruction
		class octeonStore: public octeon_Insn
		{
			public:
			std::set<octeonRegType> getUses();
			std::ostream& print(std::ostream& os) const;

			octeonStore(octeonOpcodes opcode, octeonRegType& base, uint32_t displ, octeonRegType& src, octeonOpSize size = OC_DWORD);

			private:
			octeonMemOp to;
			octeonRegOp from;
		};

		//!class representing an alu operation with 3 operands 
		class octeonAlu3op: public octeon_Insn
		{
			public:
			std::set<octeonRegType> getUses();
			std::set<octeonRegType> getDefs();
			std::ostream& print(std::ostream& os) const;

			octeonAlu3op(octeonOpcodes opcode, octeonRegType& dst, octeonRegType& src1, octeonRegType& src2, octeonOpSize size = OC_DWORD);
			octeonAlu3op(octeonOpcodes opcode, octeonRegType& dst, octeonRegType& src1, uint32_t imm, octeonOpSize size = OC_DWORD);

			private:
			octeonRegOp operands[3];
			octeonImmOp imm;
		};

		//!class representin an alu operation with two operands
		class octeonAlu2Op: public octeon_Insn
		{
			public:
			std::set<octeonRegType> getUses();
			std::set<octeonRegType> getDefs();
			std::ostream& print(std::ostream& os) const;

			octeonAlu2Op(octeonOpcodes opcode, octeonRegType& dst, octeonRegType& src, octeonOpSize size = OC_DWORD);
			octeonAlu2Op(octeonOpcodes opcode, octeonRegType& dst, uint32_t imm, octeonOpSize size = OC_DWORD);

			private:
			octeonRegOp operands[2];
			octeonImmOp imm;
		};

		//!class representin an mul or div operation
		class octeonMulDivOp: public octeon_Insn
		{
			public:
			std::set<octeonRegType> getUses();
			std::ostream& print(std::ostream& os) const;

			octeonMulDivOp(octeonOpcodes opcode, octeonRegType& src1, octeonRegType& src2, octeonOpSize size = OC_DWORD);

			private:
			octeonRegOp operands[2];
		};

		class octeonBinaryTreeBranch: public octeon_Insn
		{
			public:
			std::set<octeonRegType> getUses();
			std::ostream& print(std::ostream& os) const;

			octeonBinaryTreeBranch(octeonRegType reg, uint32_t value, std::string& my_name, std::string& target_name);

			private:
			std::string myname;
			std::string target_name;
			uint32_t value;
			octeonRegType reg;
		};

		//!class representing a branch with label or indirect target
		class octeonBranchLblOp: public octeon_Insn
		{
			public:
			virtual std::ostream& print(std::ostream& os) const;

			octeonBranchLblOp(octeonOpcodes opcode, uint32_t target, bool long_jump = false);
			octeonBranchLblOp(octeonOpcodes opcode, octeonRegType& target, bool long_jump = false);
			octeonBranchLblOp();

			uint32_t get_target() const;
			bool isLongJump(CFG<octeon_Insn>& cfg, uint32_t count) const;
			void toLongJump();

			static std::string long_jump_to_target(const std::string& target);
			static std::string long_jump_to_target(const octeonLabelOp& target);

			protected:
			octeonLabelOp target;
			octeonRegOp   reg_target;
			bool epilogue;
			bool long_jump;
		};

		//!common interface for condititional jump instructions
		class octeonCondBranch: public octeonBranchLblOp
		{
			public:
			octeonCondBranch(octeonOpcodes opcode, uint32_t jt, uint32_t jf, bool long_jump = false);

			virtual void invert();

			uint32_t get_false_target() const;

			protected:
			octeonLabelOp false_target;
			octeonOpcodes get_inverted_opcode() const;
		};

		//!class representing a conditional jump with an operand
		class octeonBranchCmp1Op: public octeonCondBranch
		{
			public:
			std::set<octeonRegType> getUses();
			std::ostream& print(std::ostream& os) const;

			octeonBranchCmp1Op(octeonOpcodes opcode, octeonRegType& src, uint32_t jt, uint32_t jf, octeonOpSize size = OC_DWORD);
			void invert();

			private:
			octeonRegOp op;
		};

		//!class representing a conditional jump with two operands
		class octeonBranchCmp2Op: public octeonCondBranch
		{
			public:
			std::set<octeonRegType> getUses();
			std::ostream& print(std::ostream& os) const;

			octeonBranchCmp2Op(octeonOpcodes opcode, octeonRegType& src1, octeonRegType& src2, uint32_t jt, uint32_t jf, octeonOpSize size = OC_DWORD);
			void invert();

			private:
			octeonLabelOp target_false;
			octeonRegOp ops[2];
		};

		//!class representing a move operation
		class octeonMoveOp : public octeon_Insn
		{
			public:
			std::set<octeonRegType> getUses();
			std::set<octeonRegType> getDefs();
			std::ostream& print(std::ostream& os) const;

			octeonMoveOp(octeonRegType& src, octeonRegType& dst, octeonOpSize size = OC_DWORD);

			bool isCopy() const;
			std::list< std::pair <RegType, RegType> > getCopiedPair() const;
			octeonRegType get_from() const;
			octeonRegType get_to() const;

			private:
			octeonRegOp src;
			octeonRegOp dst;
		};
		
		template<typename IR>
		std::string getFunctionName(const CFG<IR>& cfg)
		{
			std::string res = std::string(cfg.getName());
			uint32_t sub = res.find_first_of('.');
			res.replace(sub, 1, "_");
			return res;
		}

		class octeonCFG: public CFG<octeon_Insn>
		{
			public:
				typedef std::list<octeonJumpTable*> jump_table_list_t;

				octeonCFG(const std::string& name);
				~octeonCFG();
				void addJumpTable(octeonJumpTable* jump_table);
				
				const jump_table_list_t& getJumpTables() const;

			private:
				std::list< octeonJumpTable* > jumpTables;
		};

	}//namespace octeon
}//namespace jit

#include "octeon-asm.inl"
#endif
