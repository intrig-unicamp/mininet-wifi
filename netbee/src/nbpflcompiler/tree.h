/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include "defs.h"
#include "symbols.h"
#include "mironode.h"
#include <stdio.h>
			   //the last HIR opcode is HIR_LAST_OP and it is defined in irops.h

struct OpDescr
{
	const char*		Sym;
	const char*		Name;
};


enum IROperators
{
#define IR_OPERATOR(code, sym, name) code,
#include "iroperators.h"
#undef IR_OPERATOR
};


enum OpDetails
{
	IR_TERM	= 0,
	IR_UNOP	= 1, 
	IR_BINOP	= 2,
	IR_TEROP = 3,
	IR_STMT	= 4,
	IR_LAST = 0xF
};

enum IRType
{
	IR_TYPE_INT		= 1,
	IR_TYPE_STR		= 2,
	IR_TYPE_BOOL	= 3
};

enum IRTypeAliases
{
	I = IR_TYPE_INT,
	S = IR_TYPE_STR,
	B = IR_TYPE_BOOL
};



enum HIROpcodes
{
	hir_first_op = 0, //this is the uint16 value for the first HIR opcode
			  //you cannot use it in the hir code...

#define IR_OPCODE(code, sym, name, nKids, nSyms, op, flags, optypes, rtypes,  description) code = (op << 8) + (flags<<4) + (rtypes<<2) + optypes,
	#include "irops.h" 
#undef IR_OPCODE
};

enum MIROpcodes
{
	mir_first_op = 14000, //this is the uint16 value for the first MIR opcode
			  //you cannot use it in the mir code...	
#define nvmOPCODE(id, name, pars, code, consts, desc) id,
	#include "../nbnetvm/opcodes.txt"
#undef nvmOPCODE
};


/*
Opcode Details:
	Operands type:
		[IR_TYPE_INT	= 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
		IR_TYPE_STR		= 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
		IR_TYPE_BOOL	= 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1]
	Return Type:		 |       |       |       |       |
		[IR_TYPE_INT	= 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0
		IR_TYPE_STR		= 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0
		IR_TYPE_BOOL	= 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0]
	Flags:				 |       |       |       |       |
		[HIR_TERM      = 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		HIR_UNOP       = 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0
		HIR_BINOP		= 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0
		HIR_TEROP		= 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0
		HIR_STMT		= 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0]

		[OP				= X X X X X X X X 0 0 0 0 0 0 0 0]
*/

#define IR_OP_MASK		0xFF00
#define IR_FLAGS_MASK	0x00F0
#define IR_RTYPE_MASK	0x000C
#define IR_TYPE_MASK	0x0003

#define GET_OP(op)         ((op & IR_OP_MASK)    >> 8)
#define GET_OP_FLAGS(op)   ((op & IR_FLAGS_MASK) >> 4)
#define GET_OP_TYPE(op)    ( op & IR_TYPE_MASK)
#define GET_OP_RTYPE(op)   ((op & IR_RTYPE_MASK) >> 2)


extern OpDescr OpDescriptions[];
extern OpDescr NvmOps[];
extern const char *IRTypeNames[];

struct Node
{
friend struct StmtBase;

protected:
	Node(uint16 op)
		:Op(op), Sym(0), SymEx(0), Value(0), NRefs(0)
	{
		Kids[0] = NULL;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}


	//!\Constructor for leaf nodes
	Node(uint16 op, Symbol *symbol, uint32 value = 0, Symbol *symEx = 0)
		:Op(op), Sym(symbol), SymEx(symEx), Value(value), NRefs(0)
	{
		Kids[0] = NULL;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}

	Node(uint16 op, uint32 value)
		:Op(op), Sym(0), SymEx(0), Value(value), NRefs(0)
	{
		Kids[0] = NULL;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}

	//!\Constructor for unary operators
	Node(uint16 op, Node	*kidNode, Symbol *sym = 0, uint32 value = 0)
		:Op(op), Sym(sym), SymEx(0), Value(value), NRefs(0)
	{
		if (kidNode)
		{
			nbASSERT(kidNode->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kidNode->NRefs++;
		}
		Kids[0] = kidNode;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}

	//!\Constructor for binary operators
	Node(uint16 op, Node	*kid1, Node *kid2)
		:Op(op), Sym(0), SymEx(0), Value(0), NRefs(0)
	{
		if (kid1)	
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = NULL;
	}

	//!\Constructor for references
	Node(uint16 op, Symbol *label, Node *kid1, Node *kid2)
		:Op(op), Sym(label), SymEx(0), Value(0), NRefs(0)
	{
		if (kid1)
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{	
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = NULL;
	}

	//!\Constructor for ternary operators
	Node(uint16 op, Node	*kid1, Node *kid2, Node *kid3)
		:Op(op), Sym(0), SymEx(0), Value(0), NRefs(0)
	{
		if (kid1)
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		if (kid3)
		{
			nbASSERT(kid3->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid3->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = kid3;
	}

	//!\Constructor for references
	Node(uint16 op, Symbol *label, Node *kid1, Node *kid2, Node *kid3)
		:Op(op), Sym(label), SymEx(0), Value(0), NRefs(0)
	{
		if (kid1)
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		if (kid3)
		{
			nbASSERT(kid3->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid3->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = kid3;
	}


	void SwapChilds(void)
	{
		Node *tmp = Kids[0];
		Kids[0] = Kids[1];
		Kids[1] = tmp;
	}


	Node *GetLeftChild(void)
	{
		return Kids[0];
	}

	Node *GetRightChild(void)
	{
		return Kids[1];
	}

	Node *GetThirdChild(void)
	{
		return Kids[2];
	}

	void SetLeftChild(Node *child)
	{
		Kids[0] = child;
	}

	void SetRightChild(Node *child)
	{
		Kids[1] = child;
	}

	void SetThirdChild(Node *child)
	{
		Kids[2] = child;
	}
	

public:	
	uint16		Op;
	Symbol		*Sym;
	Symbol		*SymEx;
	Node		*Kids[3];
	uint32		Value;
	uint32		NRefs;

	bool IsBoolean(void);
	bool IsString(void);
	bool IsInteger(void);
	bool IsTerOp(void);
	bool IsBinOp(void);
	bool IsUnOp(void);
	bool IsTerm(void);
	//bool IsStmt(void);
	bool IsConst(void);
	
	uint32 GetConstVal(void);
	
	MIRONode * translateToMIRONode(); 
	
	virtual Node *Clone()=0;
	
	virtual ~Node() {
		if (Kids[0])
		{
			delete(Kids[0]);
			Kids[0] = NULL;
		}
		if (Kids[1])
		{
			delete(Kids[1]);
			Kids[1] = NULL;
		}
		if (Kids[2])
		{
			delete(Kids[2]);
			Kids[2] = NULL;
		}
	
	
	}
	
	
};


/*************************************************HIRNode****************************************************/

//this struct rappresent a HIR Node
struct HIRNode: public Node
{
friend class IRCodeGen;
friend class HIRCodeGen;
friend class CompField;

friend bool ReverseCondition(HIRNode **expr);
	
protected:

	HIRNode(HIROpcodes op)
		:Node(op){
			hirOp = op;
		}
		
	//!\Constructor for leaf nodes
	HIRNode(HIROpcodes op, Symbol *symbol, uint32 value = 0, Symbol *symEx = 0)
		:Node(op,symbol,value,symEx) {
			hirOp = op;
		}

	HIRNode(HIROpcodes op, uint32 value)
		:Node(op,value) {
			hirOp = op;
		}

	//!\Constructor for unary operators
	HIRNode(HIROpcodes op, HIRNode	*kidNode, Symbol *sym = 0, uint32 value = 0)
		:Node(op, kidNode,sym,value){
			hirOp = op;
		}

	//!\Constructor for binary operators
	HIRNode(HIROpcodes op, HIRNode	*kid1, HIRNode *kid2)
		:Node(op, kid1, kid2) {
			hirOp = op;
		}

	//!\Constructor for references
	HIRNode(HIROpcodes op, Symbol *label, HIRNode *kid1, HIRNode *kid2)
		:Node(op, label, kid1, kid2) {
			hirOp = op;
		}

	//!\Constructor for ternary operators
	HIRNode(HIROpcodes op, HIRNode	*kid1, HIRNode *kid2, HIRNode *kid3)
		:Node(op, kid1, kid2, kid3) {
			hirOp = op;
		}

	//!\Constructor for references
	HIRNode(HIROpcodes op, Symbol *label, HIRNode *kid1, HIRNode *kid2, HIRNode *kid3)
		:Node(op, label, kid1,kid2, kid3) {
			hirOp = op;
		}


	/*~HIRNode() 
	{
		if (Kids[0])
		{
			delete(Kids[0]);
			Kids[0] = NULL;
		}
		if (Kids[1])
		{
			delete(Kids[1]);
			Kids[1] = NULL;
		}
		if (Kids[2])
		{
			delete(Kids[2]);
			Kids[2] = NULL;
		}
	}
*/
public:

	HIROpcodes hirOp;

	void SwapChilds(void)
	{
		Node::SwapChilds();
	}


	HIRNode *GetLeftChild(void)
	{
		return static_cast<HIRNode*>(Node::GetLeftChild());

	}

	HIRNode *GetRightChild(void)
	{
		return static_cast<HIRNode*>(Node::GetRightChild());
	}

	HIRNode *GetThirdChild(void)
	{
		return static_cast<HIRNode*>(Node::GetThirdChild());
	}

	void SetLeftChild(HIRNode *child)
	{
		Node::SetLeftChild(child);
	}

	void SetRightChild(HIRNode *child)
	{
		Node::SetRightChild(child);
	}

	void SetThirdChild(HIRNode *child)
	{
		Node::SetThirdChild(child);
	}

	
	Node *Clone();

};

bool ReverseCondition(HIRNode **expr); 

/*************************************************MIRNode****************************************************/

//this struct rappresent a MIR Node
struct MIRNode: public Node
{
friend class IRCodeGen;
friend class MIRCodeGen;

protected:
	MIRNode(MIROpcodes op)
		:Node(op){
			mirOp = op;
		}
		
	//!\Constructor for leaf nodes
	MIRNode(MIROpcodes op, Symbol *symbol, uint32 value = 0, Symbol *symEx = 0)
		:Node(op,symbol,value,symEx) {
			mirOp = op;
		}

	MIRNode(MIROpcodes op, uint32 value)
		:Node(op,value) {
			mirOp = op;
		}

	//!\Constructor for unary operators
	MIRNode(MIROpcodes op, MIRNode	*kidNode, Symbol *sym = 0, uint32 value = 0)
		:Node(op, kidNode,sym,value){
			mirOp = op;
		}

	//!\Constructor for binary operators
	MIRNode(MIROpcodes op, MIRNode	*kid1, MIRNode *kid2)
		:Node(op, kid1, kid2) {
			mirOp = op;
		}

	//!\Constructor for references
	MIRNode(MIROpcodes op, Symbol *label, MIRNode *kid1, MIRNode *kid2)
		:Node(op, label, kid1, kid2) {
			mirOp = op;
		}

	//!\Constructor for ternary operators
	MIRNode(MIROpcodes op, MIRNode	*kid1, MIRNode *kid2, MIRNode *kid3)
		:Node(op, kid1, kid2, kid3) {
			mirOp = op;
		}

	//!\Constructor for references
	MIRNode(MIROpcodes op, Symbol *label, MIRNode *kid1, MIRNode *kid2, MIRNode *kid3)
		:Node(op, label, kid1,kid2, kid3) {
			mirOp = op;
		}


/*	~MIRNode() 
	{
		if (Kids[0])
		{
			delete(Kids[0]);
			Kids[0] = NULL;
		}
		if (Kids[1])
		{
			delete(Kids[1]);
			Kids[1] = NULL;
		}
		if (Kids[2])
		{
			delete(Kids[2]);
			Kids[2] = NULL;
		}
	}
*/
public:
	MIROpcodes mirOp;
	
	void SwapChilds(void)
	{
		Node::SwapChilds();
	}


	MIRNode *GetLeftChild(void)
	{
		return static_cast<MIRNode*>(Node::GetLeftChild());
	}

	MIRNode *GetRightChild(void)
	{
		return static_cast<MIRNode*>(Node::GetRightChild());
	}

	MIRNode *GetThirdChild(void)
	{
		return static_cast<MIRNode*>(Node::GetThirdChild());
	}

	void SetLeftChild(MIRNode *child)
	{
		Node::SetLeftChild(child);
	}

	void SetRightChild(MIRNode *child)
	{
		Node::SetRightChild(child);
	}

	void SetThirdChild(MIRNode *child)
	{
		Node::SetThirdChild(child);
	}


	
	Node *Clone();

};
