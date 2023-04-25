/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*!
 * \file octeonc-asm.h
 * \brief this file contains declaration of classes representing an octeonc instruction
 */
#ifndef OCTEONC_ASM_H
#define OCTEONC_ASM_H

#include <memory>
#include <list>
#include <map>
#include "cfg.h"
#include "irnode.h"
#include "mirnode.h"
#include "registers.h"

namespace jit {
	//!namespace containing all definitions of octeonc backend
	namespace octeonc {

        extern const std::string count_prop_name;
		//!type used to represent a register
		class octeoncRegType : public RegisterInstance
		{
			public:

			octeoncRegType(const RegisterInstance& reg = 
                RegisterInstance::invalid);
			octeoncRegType(
                uint32_t space, uint32_t name, uint32_t version = 0);

			static octeoncRegType virt_reg(uint32_t name);
			static octeoncRegType new_virt_reg();

			friend std::ostream& operator<<(
                std::ostream& os, const octeoncRegType& reg);

			//!number of the space of virtual registers
			static const uint32_t VirtSpace;
		};

        class octeoncOperand
        {
            public:
            virtual std::ostream& print(std::ostream& os) const = 0;

            friend std::ostream& operator<<(
                std::ostream& os, const octeoncOperand& op);
        };

        class octeoncRegOperand : public octeoncOperand
        {
            public:
            octeoncRegOperand(octeoncRegType reg);
            std::ostream& print(std::ostream& os) const;

            private:
            octeoncRegType reg_;
        };

        class octeoncLitOperand : public octeoncOperand
        {
            public:
            octeoncLitOperand(int32_t val);
            std::ostream& print(std::ostream& os) const;

            private:
            int32_t val_;
        };

		//!class representing a generic octeonc instruction
		class octeonc_Insn : public IRNode<octeoncRegType>
		{
			public:
            // constructor
            octeonc_Insn();
            // destructor
            virtual ~octeonc_Insn() {}

            // iterator
            class IRNodeIterator;
            IRNodeIterator nodeBegin();
            IRNodeIterator nodeEnd();

            // printing functions
            virtual std::ostream& print(std::ostream& os) const;
            std::ostream& printNode(
                std::ostream& os, bool inSSA = false) const;
            friend std::ostream& operator<<(
                std::ostream& os, const octeonc_Insn& insn);

            virtual std::set<RegType> getDefs();
            virtual std::set<RegType> getUses();

            // function for register mapping. 
            // see todo in register mapping
            void rewrite_destination(uint16_t, uint16_t) 
                { assert(1 == 0 && "not implemented"); }
            void rewrite_use(RegType oldreg, RegType newreg) 
                { assert (1 == 0 && "not_implemented"); }
            RegType* getOwnReg() 
                { assert(1 == 0 && "not_implemented"); return NULL;}
            void setDefReg(RegType r) 
                {assert(1 == 0 && "not implemented"); }
		};

        //!generic intruction
        class octeonc_GenerigInsn : public octeonc_Insn
        {
            public:
            octeonc_GenerigInsn(const std::string& insn);
            std::ostream& print(std::ostream& os) const;

            private:
            std::string insn_;
        };

        class octeonc_Unaryexpr : public octeonc_Insn
        {
            public:
            octeonc_Unaryexpr(
                MIRNode* insn,
                octeoncOperand* dst,
                octeoncOperand* src);

            static const char* get_expr_op(MIRNode *insn);
            std::ostream& print(std::ostream& os) const;

            private:
            std::string op_;
            std::auto_ptr<octeoncOperand> dst_;
            std::auto_ptr<octeoncOperand> src_;
        };

        //!bynary expression instruction
        class octeonc_Binaryexpr : public octeonc_Insn
        {
            public:
            octeonc_Binaryexpr(
                MIRNode* insn,      // the operator symbol
                octeoncOperand* dst,     // destination
                octeoncOperand* src1,   // src1 reg
                octeoncOperand* src2);

            static const char* get_expr_op(MIRNode* insn);

            std::ostream& print(std::ostream& os) const;

            private:
                std::string op_;
                std::auto_ptr<octeoncOperand> dst_;    // destination
                std::auto_ptr<octeoncOperand> src1_;   // src reg
                std::auto_ptr<octeoncOperand> src2_;   // src2 reg
        };

        class octeonc_Jump : public octeonc_Insn
        {
            public:
            octeonc_Jump(
                JumpMIRNode* insn, uint32_t jt, uint32_t jf,
                octeoncOperand* src1, octeoncOperand* src2);

            static const char* get_cond_op(JumpMIRNode* insn);
            std::ostream& print(std::ostream& os) const;

            private:
            std::string op_;
            uint32_t jt_;
            uint32_t jf_;
            std::auto_ptr<octeoncOperand> src1_;
            std::auto_ptr<octeoncOperand> src2_;
        };

        class octeonc_MemOp : public octeonc_Insn
        {
            public:
            typedef enum {
                PKT,
                INFO,
                DATA,
                INVALID_TYPE
            } octeonc_MemType;

            typedef enum {
                INT8,  INT16,  INT32,
                UINT8, UINT16, UINT32,
                INVALID_DATATYPE
            } octeonc_DataType;

            octeonc_MemOp(MIRNode* insn);

            const char* get_base() const;
            const char* get_type() const;

            private:
            static octeonc_MemType get_memtype(MIRNode* insn);
            static octeonc_DataType get_datatype(MIRNode* insn);

            // to store the data memory name
            octeonc_MemType  memtype_;
            octeonc_DataType datatype_;
            std::string data_name_;
        };

        class octeonc_Load : public octeonc_MemOp
        {
            public:
            octeonc_Load(MIRNode* insn, 
                octeoncOperand* offset, octeoncOperand* dst);

            std::ostream& print(std::ostream& os) const;

            private:
            std::auto_ptr<octeoncOperand> offset_;
            std::auto_ptr<octeoncOperand> dst_;
        };

        class octeonc_Store : public octeonc_MemOp
        {
            public:
            octeonc_Store(MIRNode* insn,
                octeoncOperand *offset, octeoncOperand *src);

            std::ostream& print(std::ostream& os) const;

            private:
            std::auto_ptr<octeoncOperand> offset_;
            std::auto_ptr<octeoncOperand> src_;
        };

		class octeonc_Insn::IRNodeIterator
		{
			private: 
				octeonc_Insn *ptr;
			public:
				typedef octeonc_Insn* value_type;
				typedef ptrdiff_t difference_type;
				typedef octeonc_Insn** pointer;
				typedef octeonc_Insn*& reference;
				typedef std::forward_iterator_tag iterator_category;

				IRNodeIterator(octeonc_Insn* insn = NULL);
				IRNodeIterator(const IRNodeIterator &it);
				value_type& operator*();
				value_type& operator->();
				IRNodeIterator& operator++(int);
				bool operator==(const IRNodeIterator& it) const;
				bool operator!=(const IRNodeIterator& it) const;
		};

		template<typename IR>
		std::string getFunctionName(const CFG<IR>& cfg)
		{
			std::string res = std::string(cfg.getName());
			uint32_t sub = res.find_first_of('.');
			res.replace(sub, 1, "_");
			return res;
		}

		class octeoncCFG: public CFG<octeonc_Insn>
		{
			public:
				octeoncCFG(const std::string& name);
		};

	}//namespace octeonc
}//namespace jit

#include "octeonc-asm.inl"
#endif
