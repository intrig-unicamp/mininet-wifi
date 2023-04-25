#ifndef _PFLMIRNODE_H
#define _PFLMIRNODE_H

#include "symbols.h"
#include "registers.h"
#include "irnode.h"
#include "statements.h"

struct Symbol;
class CodeWriter;
class PFLCFG;


class PhiMIRONode; //fwd declaration for SSA interface
class StmtMIRONode;
class JumpMIRONode;
class SwitchMIRONode;

class MIRONode: public jit::TableIRNode<jit::RegisterInstance, uint16_t>
{
	friend class CodeWriter;
	private:
	protected:
		static PFLCFG *_cfg;
		MIRONode* kids[3]; //!<The kids of this node
		/*!
		 * This register id the virtual register written by this node. It is returned 
		 * by getDefs only if this node is a store register.
		 */
		//RegType defReg; //!<The register defined by this node

		std::list<MIRONode*> ordered_list; //!<the list of this node kids in post order ???

		uint32_t Value;
		std::string Comment;
		uint32_t numRefs;
		//RegType defReg;
		
	public:
		Symbol *Sym;
		Symbol *SymEx;

		static void setCFG(PFLCFG *cfg) { MIRONode::_cfg = cfg; } //FIXME !! This is very bad
		static PFLCFG * getCFG() { return MIRONode::_cfg; }

		virtual MIRONode *copy();

		void setSymbol(Symbol *sym);
		void setValue(uint32_t value) {Value = value; };
		void setComment(std::string str) { Comment = str; };
		void setDefReg(RegType reg); 
		RegType getDefReg();

		RegType *getOwnReg();

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);

		// ------ THIS INTERFACE IS REQUIRED FOR THE SSA ALGORITHMS -------------------------
		typedef PhiMIRONode PhiType; //!< Required interface for the SSA algorithm.
		static PhiType *make_phi(uint16_t, RegType);

		typedef MIRONode CopyType;	//!< Type representing a copy operation
		static CopyType *make_copy(uint16_t, RegType src, RegType dst); //!< ctor for a node that implements a register-to-register copy

		virtual void rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id);
		virtual void rewrite_use(RegType old, RegType new_reg);

		//! Needed for exit from SSA - true if this is a control flow operation, of any type
		virtual bool isJump() const; 

		virtual bool isPhi() const { return false; };
		// ------ END OF INTERFACE REQUIRED BY THE SSA ALGORITHMS ---------------------------
		
		// ------ THIS INTERFACE IS REQUIRED FOR THE COPY FOLDING ALGORITHM -----------------
		typedef std::list<MIRONode *> ContType;
		ContType get_reverse_instruction_list();
		virtual bool isCopy() const;
		// ------ END OF INTERFACE REQUIRED BY THE COPY FOLDING ALGORITHM -------------------
		// ------ INTERFACE REQUIRED BY CONSTANT PROPAGATION ALGORITHM --------------------------
		typedef MIRONode ConstType;
		typedef MIRONode LoadType;
		typedef int32_t ValueType;

		static ConstType * make_const(uint16_t bbID, ValueType const_value, RegType defReg = RegType::invalid);

		virtual bool isConst();
		virtual bool isStore();
		virtual bool isLoad();

		RegType getUsed();
		void setUsed(RegType);

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

/*********************************************************
 * Control Flow simplification algorithm required interface
 * *******************************************************/
		typedef JumpMIRONode 		JumpType;
		typedef SwitchMIRONode 	SwitchType;
		virtual bool isSwitch() const {return false;}
/*********************************************************
 * END Control Flow simplification algorithm required interface
 * *******************************************************/

		virtual ValueType getValue();

		virtual MIRONode * getTree() { return this; };

		virtual void addRef(void)
		{
			numRefs++;
		}

		virtual void removeRef(void)
		{
			numRefs--;
		}

		virtual bool isStateChanger() { return true; }

		typedef StmtMIRONode StoreType;
		
		static StoreType * make_store(uint16_t bbID, MIRONode *kid, RegType defReg);
		static LoadType * make_load(uint16_t bbID, RegType defReg);
		// ------ END OF INTERFACE REQUIRED BY THE CONSTANT PROPAGATION ALGORITHM -------------------

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

		virtual MIRONode *getKid(uint8_t pos) const;
		virtual	void deleteKid(uint8_t pos);

		virtual	MIRONode *unlinkKid(uint8_t pos);

		virtual void swapKids();

		void setKid(MIRONode *node, uint8_t pos);

		bool IsBoolean(void);

		bool IsInteger(void);

		bool IsString(void);


		bool IsTerOp(void);


		bool IsBinOp(void);


		bool IsUnOp(void);


		bool IsTerm(void);


		/*

		   bool BaseNode::IsStmt(void)
		   {
		   return (GET_OP_FLAGS(Op) == IR_STMT);
		   }
		   */

		//bool IsConst(void);

		uint32 GetConstVal(void);


		//Constructors
		MIRONode(uint16_t op)
			: jit::TableIRNode<RegType, OpCodeType>(op),  Value(0),  Sym(0)
		{
			kids[0] = NULL;
			kids[1] = NULL;
			kids[2] = NULL;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}


		//!\Constructor for leaf nodes
		MIRONode(uint16_t op, Symbol *symbol)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(symbol)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = NULL;
			kids[1] = NULL;
			kids[2] = NULL;
			if(!symbol)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		MIRONode(uint16_t op, uint32 value)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(value), Sym(0)/*, ResType(IR_TYPE_INT), OpType(IR_TYPE_INT)*/
		{
			kids[0] = NULL;
			kids[1] = NULL;
			kids[2] = NULL;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}

		//!\Constructor for unary operators
		MIRONode(uint16_t op, MIRONode	*kidNode, Symbol *sym = 0)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0),  Sym(sym)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kidNode;
			kids[1] = NULL;
			kids[2] = NULL;
			if(!sym)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		//!\Constructor for binary operators
		MIRONode(uint16_t op, MIRONode	*kid1, MIRONode *kid2)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(0)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = NULL;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}

		//!\Constructor for references
		MIRONode(uint16_t op, Symbol *label, MIRONode *kid1, MIRONode *kid2)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(label)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = NULL;
			if(!label)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		//!\Constructor for ternary operators
		MIRONode(uint16_t op, MIRONode	*kid1, MIRONode *kid2, MIRONode *kid3)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(0)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = kid3;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}

		//!\Constructor for references
		MIRONode(uint16_t op, Symbol *label,MIRONode *kid1, MIRONode *kid2, MIRONode *kid3)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(label)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = kid3;
			if(!label)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		~MIRONode();

		//*************************************
		//NODE ITERATOR
		//************************************
		struct Orderer
		{
			std::list<MIRONode*> &list;
			Orderer(std::list<MIRONode*> &l): list(l) {}

			void operator()(MIRONode *n)
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
				if(kids[2])
					kids[2]->visitPostOrder(action);

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
				if(kids[2])
					kids[2]->visitPostOrder(action);

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
		//friend std::ostream& operator<<(std::ostream& os, MIRONode& node);

		/*!
		 * \brief class that implements an iterator on the nodes of this tree
		 */
		class MIRONodeIterator
		{
			private:
				std::list<MIRONode*> &nodes; //!<ordered list containing the nodes of this tree
				std::list<MIRONode*>::iterator _it; //!<wrapped iterator on the list
			public:

				typedef MIRONode* value_type;
				typedef ptrdiff_t difference_type;
				typedef MIRONode** pointer;
				typedef MIRONode*& reference;
				typedef std::forward_iterator_tag iterator_category;

				MIRONodeIterator(std::list<MIRONode*> &l): nodes(l) { _it = nodes.begin();}
				MIRONodeIterator(std::list<MIRONode*> &l, bool dummy):nodes(l), _it(nodes.end()) { }

				MIRONode*& operator*()
				{
					return *_it;
				}

				MIRONode*& operator->()
				{
					return *_it;
				}

				MIRONodeIterator& operator++(int)
				{
					_it++;
					return *this;
				}


				bool operator==(const MIRONodeIterator& bb) const
				{
					return bb._it == _it;
				}

				bool operator!=(const MIRONodeIterator& bb) const
				{
					return bb._it != _it;
				}
		};

		typedef MIRONodeIterator IRNodeIterator; //!< Required for the SSA algorithm interface.
		friend class MIRNodeIterator;

		//!get an iterator on the nodes of this tree in post order
		virtual MIRONodeIterator nodeBegin()
		{
			ordered_list.clear();
			sortPostOrder();
			return MIRONodeIterator(ordered_list);
		}

		virtual //!get an iterarot to the end of the list of nodes
			MIRONodeIterator nodeEnd()
			{
				return MIRONodeIterator(ordered_list, true);
			}

};



/*!
 * \brief This class is a genericstatement in PFLNode form
 */
class StmtMIRONode: public MIRONode
{
	friend class CodeWriter;
	friend class CFGWriter;
	friend class NetILTraceBuilder;
	friend class PFLTraceBuilder;
	protected:
		StmtKind Kind;
		std::string Comment;
		StmtMIRONode *GenBy;
		uint16_t Flags;
	public:
		StmtMIRONode(StmtKind kind, MIRONode *kid = 0): MIRONode(0, kid), Kind(kind), Comment(""), GenBy(0), Flags(0) {};

		virtual ~StmtMIRONode() {
		}
		MIRONode *copy();

		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
		virtual MIRONode *getTree() { return getKid(0);}

		virtual std::set<MIRONode::RegType> getDefs();

		virtual RegType* getOwnReg();

		virtual bool isPFLStmt() { return true;};
		 
		std::set<RegType> getAllUses();

};

class BlockMIRONode: public StmtMIRONode
{
	friend class CodeWriter;
	protected:
		std::list<MIRONode*> *Code;

	public:
	BlockMIRONode(std::string str)
		:StmtMIRONode(STMT_BLOCK) { Comment = str; }

	virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
};

/*!
 * \brief This class is a label In MIRONode form
 */
class LabelMIRONode: public StmtMIRONode
{
	public:
		LabelMIRONode(): StmtMIRONode(STMT_LABEL) {};

		MIRONode *copy();
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);

		virtual bool isStateChanger() { return false; }
};


/*!
 * \brief Comment Statement
 */
class CommentMIRONode: public StmtMIRONode
{
	public:
		CommentMIRONode(std::string comment)
			: StmtMIRONode(STMT_COMMENT)
		{
			Comment = comment;
		}

		MIRONode *copy();
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
		virtual bool isStateChanger() { return false; }
};

/*!
 * \brief Gen Statement
 */
class GenMIRONode: public StmtMIRONode
{
	public:
		GenMIRONode(MIRONode *kid = 0)
			: StmtMIRONode(STMT_GEN, kid) {};
		MIRONode *copy();
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
		//virtual bool isConst();
		virtual bool isStore();

		virtual bool isCopy() const;
		//virtual bool isLoad();
		virtual ValueType getValue();
};

/*!
 * \brief Jump node
 */
class JumpMIRONode: public StmtMIRONode
{
	friend class CodeWriter;
	friend class CFGWriter;
	friend class NetILTraceBuilder;
	protected:
	SymbolLabel *TrueBranch;
	SymbolLabel *FalseBranch;

	public:
	JumpMIRONode(uint16_t bb, SymbolLabel *trueBranch, SymbolLabel *falseBranch = 0, MIRONode *kid0 = 0)
		:StmtMIRONode(STMT_JUMP, kid0), TrueBranch(trueBranch), FalseBranch(falseBranch)
	{
		setBBId(bb);
		if (falseBranch == NULL)
		{
			assert(getKid(0) == NULL && "falseBranch = NULL with kid != NULL");
		}
	}
	JumpMIRONode(uint32_t opcode, uint16_t bb, 
			uint32_t jt, 
			uint32_t jf,
			RegType defReg,
			MIRONode * param1,
			MIRONode* param2);
	MIRONode *copy();
	virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
	virtual void rewrite_destination(uint16 oldbb, uint16 newbb);
	
	//! Needed for exit from SSA - true if this is a control flow operation, of any type
	virtual bool isJump() const { return true;} ;
	uint16_t getTrueTarget();
	uint16_t getFalseTarget();
//	uint16_t getTrueTarget() { return getTrueBranch(); }
//	uint16_t getFalseTarget() { return getFalseBranch(); }
	void swapTargets();
	uint16_t getOpcode() const;
};

/*!
 * \brief jump Field node
 */
class JumpFieldMIRONode: public JumpMIRONode
{
	public:
		JumpFieldMIRONode(uint16_t bb, SymbolLabel *trueBranch, SymbolLabel *falseBranch = 0, MIRONode *kid0 = 0)
			:JumpMIRONode(bb, trueBranch, falseBranch, kid0) 
		{ Kind = STMT_JUMP_FIELD; }

		MIRONode *copy();
		
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
};


class CaseMIRONode: public StmtMIRONode
{
	friend class CodeWriter;
	private:
		StmtBlock	Code;
		SymbolLabel *Target;

	public:
		CaseMIRONode(MIRONode *val = 0, SymbolLabel *target = 0)
			:StmtMIRONode(STMT_CASE, val), Target(target) {}

		MIRONode *copy();
		
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);

		void setTarget(SymbolLabel * target) { Target = target; }
		SymbolLabel * getTarget() {return Target;}
};

/*!
 * \brief  Switch node
 */
class SwitchMIRONode: public StmtMIRONode
{
	friend struct StmtSwitch;
	friend class CodeWriter;
	protected:
		SymbolLabel *SwExit; 
		std::list<MIRONode*> Cases;
		uint32_t NumCases;
		CaseMIRONode *Default;
		std::vector<std::pair<uint32_t, uint32_t> > _target_list;

	public:
		typedef std::vector< std::pair<uint32_t, uint32_t> >::iterator targets_iterator;

		SwitchMIRONode(MIRONode *expr = 0)
			: StmtMIRONode(STMT_SWITCH, expr), SwExit(0), NumCases(0), Default(0) {};

		virtual ~SwitchMIRONode();

	MIRONode *copy();
	virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
	virtual void rewrite_destination(uint16 oldbb, uint16 newbb);
	virtual bool has_side_effects() { return true; }

	uint32_t get_targets_num() const { return NumCases; } 

	targets_iterator TargetsBegin();
	targets_iterator TargetsEnd();
	std::vector<std::pair<uint32_t, uint32_t> > &getTargetsList() { return _target_list; }
	uint32_t getDefaultTarget();
	
	//! Needed for exit from SSA - true if this is a control flow operation, of any type
	virtual bool isJump() const { return true;} ;

	virtual bool isSwitch() const {return true; };

	std::set<RegType> getDefs();

	void removeCase(uint32_t case_n);
};

class PhiMIRONode: public StmtMIRONode
{
	private:
//		std::vector<RegType> params; //!< vector containing the phi params
		std::map<uint16_t, RegType> params; //!< map of (source BBs, source variable)

	public:
		typedef std::map<uint16_t, RegType>::iterator params_iterator;

		PhiMIRONode(uint32_t BBId, RegType defReg);

		PhiMIRONode(const PhiMIRONode& n);
			
		virtual ~PhiMIRONode() {
		};

		virtual MIRONode *copy() { return new PhiMIRONode(*this); }

		void addParam(RegType param, uint16_t source_bb) {
			params[source_bb] = param;
//			params.push_back( param ); 
//			param_bb.push_back(source_bb);
		}

		params_iterator paramsBegin() { return params.begin(); }
		params_iterator paramsEnd() { return params.end(); }
		
		std::vector<uint16_t> param_bb;
		
		virtual bool isPhi() const {return true; };

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);

		virtual std::set<RegType> getUses();

		virtual std::set<RegType> getDefs();

		virtual std::set<RegType> getAllUses();

};

std::ostream& operator<<(std::ostream& os, MIRONode& node);




#endif
