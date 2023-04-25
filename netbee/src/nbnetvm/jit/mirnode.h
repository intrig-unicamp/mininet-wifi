/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef MLIRNODE_H
#define MLIRNODE_H

#include "irnode.h"
#include "codetable.h"
#include "../opcodes.h"
#include "basicblock.h"
#include "registers.h"
#include "application.h"
#include <algorithm>
#include <list>
#include <set>
#include <iterator>
#include <iostream>
#include <cassert>

/** @file mirnode.h
 * \brief This file contains the definitions for the NetVM JIT of the basic structure type to store the instruction in a tree form
 */
namespace jit {
	
	
	//template<class RegType, class OpCodeType> 	//Forward declaration needed for typedef
	//class MIRNode;

	//typedef class MIRNode MIRNode;
	
	class PhiMIRNode; // Forward declaration for the phi builder method
	class ConstNode;
	class SwitchMIRNode;
	class JumpMIRNode;
	class LDRegNode;

	/*!
	 * \brief this class is used as a middle level representation. It inherits from the IRNode class and it is a list of trees. 
	 */
	class MIRNode : public TableIRNode<RegisterInstance , uint8_t >
	{
		protected:
			MIRNode* kids[2]; //!<The kids of this node
			uint32_t numRefs; //!<Number of references to this node (can be >1 if DAGs are allowed in the IR)
			/*!
			 * This register id the virtual register written by this node. It is returned 
			 * by getDefs only if this node is a store register.
			 */
			RegType defReg; //!<The register defined by this node

			std::list<MIRNode*> ordered_list; //!<the list of this node kids in post order ???

			bool isNotValid;

			bool isLinked;

			bool isDepend;

			MIRNode* depend;

			MIRNode* linked; // if this node depend on another node



		public:
			// ------ THIS INTERFACE IS REQUIRED FOR THE SSA ALGORITHMS -------------------------
			typedef PhiMIRNode PhiType; //!< Required interface for the SSA algorithm.
			static PhiMIRNode *make_phi(uint16_t, RegType);

			typedef MIRNode CopyType;	//!< Type representing a copy operation
			static MIRNode *make_copy(uint16_t, RegType src, RegType dst); //!< ctor for a node that implements a register-to-register copy
			
			virtual void rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id);
			virtual void rewrite_use(RegType old, RegType new_reg);
			
			//! Needed for exit from SSA - true if this is a control flow operation, of any type
			bool isJump() const {
				return isBranch() || isSwitch();
			}
			// ------ END OF INTERFACE REQUIRED BY THE SSA ALGORITHMS ---------------------------
			
			// ------ THIS INTERFACE IS REQUIRED FOR THE COPY FOLDING ALGORITHM -----------------
			typedef std::list<MIRNode *> ContType;
			typedef LDRegNode LoadType;
			ContType get_reverse_instruction_list();
			virtual bool isCopy() const;

			// ------ END OF INTERFACE REQUIRED BY THE COPY FOLDING ALGORITHM -------------------
			// ------ INTERFACE REQUIRED BY CONSTANT PROPAGATION ALGORITHM --------------------------
			typedef ConstNode ConstType;
			typedef int32_t ValueType;

			static ConstType * make_const(uint16_t bbID, ValueType const_value, RegType defReg = RegType::invalid);

			virtual bool isConst(); 
			virtual bool isStore();
			virtual bool isLoad();
			


			//virtual bool isPFLStmt();{ return false;};
/*#######################################################################
 * Reassociation algorithm interface
 * #####################################################################*/
			virtual bool isOpMemPacket();
			virtual bool isOpMemData();
			virtual bool isMemLoad();
			virtual bool isMemStore();

			static uint8_t num_kids;
/*#######################################################################
 * END reassociation algorithm interface
 * #####################################################################*/

/*#######################################################################
 * Control flow opt
 * #####################################################################*/
			typedef SwitchMIRNode SwitchType;
			typedef JumpMIRNode JumpType;
			virtual bool isStateChanger() { return true; }
/*#######################################################################
 * END Control flow opt
 * #####################################################################*/

			virtual ValueType getValue() { //assert(1 == 0 && "GetValue called on a non-constant node"); 
			return 0;
			};

			MIRNode * getTree() { return this; }

			typedef MIRNode StoreType;
			static StoreType * make_store(uint16_t bbID, MIRNode *kid, RegType defReg);
			static LoadType *make_load(uint16_t bbID, MIRNode::RegType loadReg);
			// ------ END OF INTERFACE REQUIRED BY THE CONSTANT PROPAGATION ALGORITHM -------------------

			virtual uint32_t get_defined_count();
			
//			virtual void set_preferred_size(operand_size_t size);
			
			OpCodeType getOpcode() const;
			void setOpcode(OpCodeType op);

			void setIsNotValid(bool cond) { isNotValid = cond;}
			bool getIsNotValid() { return isNotValid; }

			void setIsLinked(bool cond) { isLinked = cond;}
			bool getIsLinked() { return isLinked; }

			void setIsDepend(bool cond) { isDepend = cond;}
			bool getIsDepend() { return isDepend; }

			void setLinked(MIRNode * node) { linked = node;}
			MIRNode * getLinked() { return linked; }

			void setDepend(MIRNode * node) { depend = node;}
			MIRNode * getDepend() { return depend; }
		
			bool inlist; //!<true if this node is in a BB's list
			//TODO
			//!\todo find a better place to store State information for burg
			void *state; //!<burg's information about this node

			//!destructor
			virtual ~MIRNode();
			//!print this node in SSA form
			virtual std::ostream& printNode(std::ostream &os, bool SSAForm);

			//!printf this node signature
			virtual std::ostream& printSignature(std::ostream &os);

			//!get the registers defined by this node
			virtual std::set<RegType> getDefs();
			
			//!get the registers defined by this tree
			virtual std::set<RegType> getAllDefs();

			//!get the registers used by this node
			virtual std::set<RegType> getUses();
			
			//!get the registers used by this tree
			virtual std::set<RegType> getAllUses();

			/*!
			 * \brief get copy done by this node
			 * \return A list of pair<dest, src>. Register copied by this node
			 */
			virtual std::list<std::pair<RegType,RegType> > getCopiedPair();

			//!constructor
			MIRNode(OpCodeType opcode, uint32_t BBId, RegType defReg = RegisterInstance::invalid, bool inlist = false);

			//!copy constructor
			MIRNode(const MIRNode& n);

			//!get the register written by this node
			virtual RegType getDefReg() { return defReg; }
			virtual void setDefReg(RegType t) { defReg = t; }
			
			//!< get the register associated with this node, if any.
			virtual RegType *getOwnReg() {
				if(!(defReg == RegType::invalid))
					return &defReg;
				return 0;
			};

			//!create a tree which has a kid
			static MIRNode* unOp(uint8_t opcode, uint32_t BBId, MIRNode *kid,
				RegType defReg = RegisterInstance::invalid);
			
			//!create a tree which has two kids
			static MIRNode* binOp(uint8_t opcode, uint16_t BBId, MIRNode *kid1, MIRNode *kid2,
				MIRNode::RegType defReg = RegisterInstance::invalid);

			//!create a tree which has no kids
			static MIRNode* leafNode(uint8_t opcode, uint32_t BBId,
				MIRNode::RegType defReg = RegisterInstance::invalid);

			/*!
			 	\brief Returns the specified child
				\param pos the kid's number
				\return Return a pointer to the child
			*/
			virtual
			MIRNode *getKid(uint8_t pos) const
			{
				assert(pos <= 1);
				return kids[pos];
			}
			
			/*!
			 	\brief sets a parent child
				\param node the node to set as a child
				\param pos the position of the child
			*/
			virtual
			void setKid(MIRNode *node, uint8_t pos)
			{
				assert(pos <= 1);
				kids[pos] = node;
				if (node != NULL)
					kids[pos]->addRef();
			}

			virtual
			void deleteKid(uint8_t pos)
			{
				assert(pos <= 1);
				delete kids[pos];
			}

			virtual
			MIRNode *unlinkKid(uint8_t pos)
			{
				assert(pos <= 1);
				MIRNode *kid = kids[pos];
				kid->removeRef();
				kids[pos] = NULL;
				return kid;
			}

			virtual
			void addRef(void)
			{
				numRefs++;
				//assert(numRefs <= 1);
			}

			virtual
			void removeRef(void)
			{
				assert(numRefs > 0);
				numRefs--;
			}

			void swapKids()
			{
				MIRNode *tmp;
				tmp = kids[0];
				kids[0] = kids[1];
				kids[1] = tmp;
			}

			
			//!true if this is a memory operation
			bool isMemoryOperation() const
			{
				uint32_t op = nvmOpCodeTable[this->OpCode].Consts & OPCODE_MASK;
				if ( (op == OP_LOAD) || (op == OP_STORE) || (op == OP_LLOAD) || (op == OP_LSTORE) )
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			

			//!true if this node is a statement
			bool isStmt() const
			{
				return (nvmOpCodeTable[this->OpCode].Consts & OPTYPE_MASK) == OP_TYPE_STMT;
			}

			//!true if this node is a branch
			bool isBranch() const
			{
				if( (nvmOpCodeTable[this->OpCode].Consts & OPCODE_MASK) == OP_JUMP 
				    || (nvmOpCodeTable[this->OpCode].Consts & OPCODE_MASK) == OP_PTNMTC)	
					return true;
				return false;
			}

			//!true if this node is a branch
			bool isBoundsCheck() const
			{
				if( this->OpCode == PCHECK || this->OpCode == ICHECK || this->OpCode == DCHECK)
				  
					return true;
				return false;
			}
			//!true if this node is a switch
			bool isSwitch() const
			{
				if( this->OpCode == SWITCH )
					return true;
				return false;
			}

			//!true if this node is a phi function
			virtual bool isPhi() const
			{
				assert(getOpcode() != PHI);
				return false;
			}

			//!true if this node is a stack operation
			bool isStackOp() const
			{
				if( (nvmOpCodeTable[this->OpCode].Consts & OPCODE_MASK) == OP_STACK)	
					return true;
				return false;
			}

			//!true if this node is a arith operation
			bool isArithOp() const
			{
				if( (nvmOpCodeTable[this->OpCode].Consts & OPCODE_MASK) == OP_ARITH)	
					return true;
				return false;
			}
	

			//!virtual method to obtain the effect of a virtual copy constructor
			virtual MIRNode *copy() 
			{
				return new MIRNode(*this);
			}

			//!struct to order this tree in post order
			struct Orderer
			{
				std::list<MIRNode*> &list;
				Orderer(std::list<MIRNode*> &l): list(l) {}

				void operator()(MIRNode *n)
				{
					list.push_back(n);
				}
			};


			//!do an action on every node in this tree in post order
			template<class Function>
			void visitPostOrder(Function action)
			{
				if(kids[0])
					kids[0]->visitPostOrder(action);
				if(kids[1])
					kids[1]->visitPostOrder(action);

				action(this);
			}
			
			//!do an action on every node in this tree in order
			template<class Function>
			void visitPreOrder(Function action)
			{
				action(this);

				if(kids[0])
					kids[0]->visitPostOrder(action);
				if(kids[1])
					kids[1]->visitPostOrder(action);

			}

			//!sort this tree in post order
			void sortPostOrder()
			{
				visitPostOrder(Orderer(ordered_list));
			}

			//!sort this tree in post order
			void sortPreOrder()
			{
				visitPreOrder(Orderer(ordered_list));
			}

			//!printing operation
			friend std::ostream& operator<<(std::ostream& os, MIRNode& node);

			/*!
			 * \brief class that implements an iterator on the nodes of this tree
			 */
			class MIRNodeIterator
			{
				private:
					std::list<MIRNode*> &nodes; //!<ordered list containing the nodes of this tree
					std::list<MIRNode*>::iterator _it; //!<wrapped iterator on the list
				public:

				typedef MIRNode* value_type;
				typedef ptrdiff_t difference_type;
				typedef MIRNode** pointer;
				typedef MIRNode*& reference;
				typedef std::forward_iterator_tag iterator_category;
			
				MIRNodeIterator(std::list<MIRNode*> &l): nodes(l) { _it = nodes.begin();}
				MIRNodeIterator(std::list<MIRNode*> &l, bool dummy):nodes(l), _it(nodes.end()) { }

				MIRNode*& operator*()
				{
					return *_it;
				}

				MIRNode*& operator->()
				{
					return *_it;
				}

				MIRNodeIterator& operator++(int)
				{
					_it++;
					return *this;
				}


				bool operator==(const MIRNodeIterator& bb) const
				{
					return bb._it == _it;
				}

				bool operator!=(const MIRNodeIterator& bb) const
				{
					return bb._it != _it;
				}
			};
			
			typedef MIRNodeIterator IRNodeIterator; //!< Required for the SSA algorithm interface.
			friend class MIRNodeIterator;

			//!get an iterator on the nodes of this tree in post order
			virtual MIRNodeIterator nodeBegin()
			{
				ordered_list.clear();
				sortPostOrder();
				return MIRNodeIterator(ordered_list);
			}

			virtual //!get an iterarot to the end of the list of nodes
			MIRNodeIterator nodeEnd()
			{
				return MIRNodeIterator(ordered_list, true);
			}

	}; /* class MIRNode */
	
	
	/*
	 * Dummy node that uses and defines some registers, in order to comply
	 * with existing interfaces that assume that every node defines at most 1
	 * register */
	class Nodelet : public MIRNode {
	  private:
		std::set<RegType> uses;
		std::map<RegType, RegType> use_mapping;
		std::map<RegType, RegType> reverse;
		RegType original;
				
	  public:
		Nodelet(uint16_t bb_id, RegType def, std::set<RegType> uses);
		Nodelet(const Nodelet &n);
		virtual ~Nodelet();
		
		RegType getOriginal() const {return original;};
		RegType getUseMap(RegType o) {
			return use_mapping[o];
		}
		
//		virtual void setDefReg(RegType r);
		
		virtual Nodelet *copy();
				
		virtual void rewrite_use(RegType old, RegType new_reg);
		
		virtual std::set<RegType> getUses();
		virtual std::set<RegType> getDefs();
		
		virtual void set_preferred_size(operand_size_t size);
		
		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);
	}; /* class Nodelet */


	//!effectors to print a register in SSA form
	template<typename RegType>
	struct SSARegPrinter
	{
		RegType _reg;

//		template<typename RegType>
//		friend std::ostream& operator<<(std::ostream& os, SSARegPrinter<RegType> & rp);
		SSARegPrinter(RegType reg): _reg(reg) {}
	};

	/*!
	 	\brief This class is a MIRNode plus some extra field to manage branches
	*/
	class JumpMIRNode : public MIRNode
	{
	private:
		typedef BasicBlock<MIRNode> basic_block_t; //!< type of a basic block
		uint32_t jt;//!<true target of this jump
		uint32_t jf;//!<false target of this jump
	public:
		JumpMIRNode(uint8_t opcode, uint32_t BBId)
			:MIRNode(opcode, BBId) {}

		JumpMIRNode(const JumpMIRNode& n)
			:MIRNode(n.OpCode, n.BBid) {}

		JumpMIRNode(uint8_t opcode, 
				uint32_t BBId,
				uint32_t jt,
				uint32_t jf,
				MIRNode::RegType defReg,
				MIRNode* param1,
				MIRNode* param2)
			:MIRNode(opcode, BBId, defReg), jt(jt), jf(jf)
		{
			kids[0] = param1;
			kids[1] = param2;
		}

		virtual MIRNode *copy() { return new JumpMIRNode(*this); }
		
		virtual bool has_side_effects() {return true;}

		/*!
		 * \brief get the true target
		 * \return the true target
		 */
		uint32_t getTrueTarget() { return jt; }
		/*!
		 * \brief get the false target
		 * \return the false target
		 */
		uint32_t getFalseTarget() { return jf; }
		
		virtual void rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id);

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);
	};

	//!class representing a coprocessor operation
	class CopMIRNode : public MIRNode
	{
		private:
		uint32_t coproId; //!<the coprocessor Id
//		uint32_t coproReg; //!<the coprocessor register written/read
		uint32_t coproOp; //!<the coprocessor operation invoked
		uint32_t coproInitOffset;
		
		nvmCoprocessorState *state;
		uint32_t base;
		uint32_t space;
		
//		std::set<RegType> uses;
		
		static bool emission_mode;
		
		std::map<RegisterInstance, uint32_t> reverse_mapping;
		std::map<uint32_t, RegisterInstance> reg_mapping;

		public:
		typedef BasicBlock<MIRNode> basic_block_t;
		
		CopMIRNode(uint8_t opcode, uint16_t BBId, uint32_t coproId, uint32_t opId, nvmCoprocessorState *cop);
		CopMIRNode(const CopMIRNode& n);
		virtual ~CopMIRNode();
		
		nvmCoprocessorState *get_state();
		uint32_t get_base() const;
		uint32_t get_space() const;

		void setcoproInitOffset(uint32_t coproInitOffset) {this->coproInitOffset = coproInitOffset;}
//		uint32_t getcoproReg() const {return getDefReg();}
		uint32_t getcoproOp() const {return coproOp;}
		uint32_t getcoproId() const {return coproId;}
		uint32_t getcoproInitOffset() const {return coproInitOffset;}
//		void setcoproReg(uint32_t coproReg) {this->coproReg = coproReg;}
		void setcoproOp(uint32_t coproOp) {this->coproOp = coproOp;}
		
		RegisterInstance getCoproReg(uint32_t which_one, bool is_use);
		
		bool isUsedReg(uint32_t which_one) const;
		bool isDefdReg(uint32_t which_one) const;
		uint32_t regNo() const;
		
		static void set_for_emission();
		static void unset_for_emission(); //I hate static
		
		virtual CopMIRNode *copy();
		
		virtual bool has_side_effects() {return true;}
		
//		virtual std::set<RegType> getUses();
		
//		virtual void setDefReg(RegType new_reg);
//		virtual void rewrite_use(RegType old, RegType new_reg);
//		virtual std::set<RegType> getUses();
		
		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);
		
		virtual
		MIRNode *getKid(uint8_t pos) const
		{
			if(!emission_mode && pos <=1)
				return kids[pos];
			return NULL;
		}

		
		virtual MIRNodeIterator nodeBegin()
		{
			if(emission_mode) {
				ordered_list.clear();
				ordered_list.push_back(this);
				return MIRNodeIterator(ordered_list);
			}
			return MIRNode::nodeBegin();
		}

		virtual //!get an iterarot to the end of the list of nodes
		MIRNodeIterator nodeEnd()
		{
			if(emission_mode) {
				ordered_list.clear();
				ordered_list.push_back(this);
				return MIRNodeIterator(ordered_list, true);
			}
			return MIRNode::nodeEnd();
		}


#if 0
		CopMIRNode(uint8_t opcode, uint32_t BBId, uint32_t coproId, uint32_t coproReg = 0, uint32_t coproOp = 0)
		: MIRNode(opcode,BBId), coproId(coproId), coproReg(coproReg), coproOp(coproOp) {}

#if 0
=======
		CopMIRNode(uint8_t opcode, uint32_t BBId, uint32_t coproId, uint32_t coproReg = 0, uint32_t coproOp = 0, uint32_t coproInitOffset = 0)
		: MIRNode(opcode,BBId), coproId(coproId), coproReg(coproReg), coproOp(coproOp), coproInitOffset(coproInitOffset) {}
>>>>>>> .r1653
#endif

		CopMIRNode(const CopMIRNode& n)
		: MIRNode(n.OpCode, n.BBid), coproId(n.coproId), coproReg(n.coproReg), coproOp(n.coproOp), coproInitOffset(n.coproInitOffset) {}

		MIRNode *copy() {return new CopMIRNode(*this); }
#endif

	};

	//! a class representing a switch operation
	class SwitchMIRNode: public MIRNode
	{
	private:
		typedef BasicBlock<MIRNode> basic_block_t;
		//!a structure to store the switch targets
		struct _switch_info
		{
			uint32_t NumCases; //!number of cases of the switch
			std::vector< std::pair<uint32_t,uint32_t> > caseTargets; //! vector containig the case/target pairs
			uint32_t defTarget; //default target

			_switch_info() {}
			_switch_info(uint32_t NumCases): NumCases(NumCases), caseTargets(NumCases) {}

		} sw_info; //!< contains the switch targets
	public:
		typedef std::vector< std::pair<uint32_t,uint32_t> >::iterator targets_iterator; //!< case-target itarator. Type of iterator is: pair<case, target>

		SwitchMIRNode(uint32_t BBId, uint32_t NumCases, MIRNode *kid)
			:MIRNode(SWITCH, BBId), sw_info(NumCases) {
				setKid(kid, 0);
				return;
			}
		SwitchMIRNode(const SwitchMIRNode& n)
			: MIRNode(SWITCH, n.BBid){}

		virtual MIRNode *copy() { return new SwitchMIRNode(*this); }
		
		virtual bool has_side_effects() {return true;}
		
		/*!
		 * \brief add a target to the switch
		 * \param pos The position in the switch vector
		 * \param sw_case The case of the switch
		 * \param target The target of the new case
		 */
		void addTarget(uint32_t pos, uint32_t sw_case, uint32_t target)
		{
			sw_info.caseTargets[pos] = std::make_pair(sw_case, target);	
		}

		//!add the switch default target
		void addDefaultTarget(uint32_t defTarget) { sw_info.defTarget = defTarget; }

		targets_iterator TargetsBegin() { return sw_info.caseTargets.begin(); }
		targets_iterator TargetsEnd() { return sw_info.caseTargets.end(); }

		uint32_t getDefaultTarget() const { return sw_info.defTarget; }
		uint32_t get_targets_num() const { return sw_info.NumCases;}
		
		virtual void rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id);

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);

	};

	/*! 
	 	\brief This class represent a phi node. 
	*/
	class PhiMIRNode: public MIRNode
	{
	private:
//		std::vector<RegType> params; //!< vector containing the phi params
		std::map<uint16_t, RegType> params; //!< map of (source BBs, source variable)

	public:
		typedef std::map<uint16_t, RegType>::iterator params_iterator;

		PhiMIRNode(uint32_t BBId, RegType defReg)
			: MIRNode(PHI, BBId, defReg) { }
		PhiMIRNode(const PhiMIRNode& n)
			: MIRNode(PHI, n.BBid) { }
			
		virtual ~PhiMIRNode() {};

		virtual MIRNode *copy() { return new PhiMIRNode(*this); }

		void addParam(RegType param, uint16_t source_bb) {
			params[source_bb] = param;
//			params.push_back( param ); 
//			param_bb.push_back(source_bb);
		}

		params_iterator paramsBegin() { return params.begin(); }
		params_iterator paramsEnd() { return params.end(); }
		
		std::vector<uint16_t> param_bb;
		
		virtual bool isPhi() const {
			assert(OpCode == PHI);
			return true;
		}

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);

		virtual std::set<RegType> getUses();

	};


	//!a class representing a costant node
	class ConstNode: public MIRNode
	{
	private:
		int32_t value; //!<the value of this constant
	public:
		typedef int32_t ValueType;

		ConstNode(uint32_t BBId, uint32_t value, MIRNode::RegType reg)
			:MIRNode(CNST, BBId, reg), value(value) {}

		ConstNode(const ConstNode& n)
			:MIRNode(CNST, n.BBid), value(n.value) { }

		virtual MIRNode *copy() { return new ConstNode(*this); }

		virtual ValueType getValue() { return value; }

		void setValue(int32_t val) { value = val; }
		
		virtual void set_preferred_size(operand_size_t size);
		
		virtual RegType *getOwnReg() {
			return 0;
		};

		virtual bool isConst() {return true;}

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false)
		{
			os << nvmOpCodeTable[getOpcode()].CodeName;
			
			operand_size_t size(getProperty<operand_size_t>("size"));
			if(size) {
				os << "<";
				switch(size) {
					case byte8:
						os << " 8";
						break;
				
					case word16:
						os << "16";
						break;
				
					case dword32:
						os << "32";
						break;
				
					default:
						os << "??";
				}
				os << "> ";
			}

			
			os <<"(" << value << ")";
			return os;
		}

	};

	//!a class representing a ldreg
	class LDRegNode: public MIRNode
	{
	public:
		LDRegNode(uint32_t BBId, RegType usedReg)
			:MIRNode(LDREG, BBId) 
			{
				setDefReg(usedReg);
			}

		LDRegNode(const LDRegNode& n)
			:MIRNode(LDREG, n.BBid)
			{
				setDefReg(n.defReg);
			}
		
		virtual void rewrite_use(RegType old, RegType new_reg);

		virtual MIRNode *copy() { return new LDRegNode(*this); }

		virtual bool isStore() { return true; }

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false)
		{
			//if(SSAForm)
			//{
			//	SSARegPrinter rp(defReg);
			//	os << "LDREG(" << rp << ")";
			//}
			//else
			//{
				os << nvmOpCodeTable[getOpcode()].CodeName;
				
				operand_size_t size(getProperty<operand_size_t>("size"));
				if(size) {
					os << "<";
					switch(size) {
						case byte8:
							os << " 8";
							break;
				
						case word16:
							os << "16";
							break;
				
						case dword32:
							os << "32";
							break;
				
						default:
							os << "??";
					}
					os << "> ";
				}
				
				os << "(";
				// FIXME: WTF?!?
				if(SSAForm) {
					std::operator<<(os, defReg);
				} else {
					os << *(defReg.get_model());
				}
//				os << defReg;
				os << ")";
			//}
			return os;
		}
		
		virtual std::set<RegType> getDefs();
//		virtual std::set<RegType> getAllDefs() {
//			std::cout << "getalldefs di ldregnode\n";
//			return std::set<RegType>();
//		}

		virtual std::set<RegType> getUses();

		RegType getUsed()
		{
			return defReg;
		}

		void setUsed(RegType used)
		{
			setDefReg(used);
		}
		
		virtual void set_preferred_size(operand_size_t size);
	};

	//!a class representing a send packet
	class SndPktNode: public MIRNode
	{
	private:
		uint8_t port_number; //!<the port to which the packet is sent
	public:
		SndPktNode(uint32_t BBId, uint8_t port_number)
			:MIRNode(SNDPKT, BBId), port_number(port_number) {}
		SndPktNode(const SndPktNode& n)
			:MIRNode(SNDPKT, n.BBid), port_number(n.port_number) {}

		uint8_t getPort_number() const
		{
			return port_number;
		}

		MIRNode *copy() { return  new SndPktNode(*this); }

		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false)
		{
			os << "sendpkt -> port " << (int)port_number;
			return os;
		}
	};

#if 0
	//!an effector class to print a node in SSA form
	class CodePrinterSSA
	{
		protected:
			MIRNode& _node;
		public:
			CodePrinterSSA(MIRNode& node) : _node(node) {}
//			friend std::ostream& operator<<(std::ostream& os, CodePrinterSSA& node);
	};
#endif

}
#endif


