/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "tree.h"


OpDescr OpDescriptions[] = 
{
#define IR_OPERATOR(code, sym, name) {sym, name},
	#include "iroperators.h"
#undef IR_OPERATOR
};


OpDescr NvmOps[] =
{
#define nvmOPCODE(id, name, pars, code, consts, desc) {name,name},
	#include "../nbnetvm/opcodes.txt"
#undef nvmOPCODE
};


const char *IRTypeNames[] =
{
	"",
	"I",
	"S",
	"B"
};



bool Node::IsBoolean(void)
{
	if (Op > mir_first_op)
		return false;
	return (GET_OP_RTYPE(Op) == IR_TYPE_BOOL);
}

bool Node::IsInteger(void)
{
	if (Op > mir_first_op)
		return true;
	return (GET_OP_RTYPE(Op) == IR_TYPE_INT);
}

bool Node::IsString(void)
{
	if (Op > mir_first_op)
		return false;
	return (GET_OP_RTYPE(Op) == IR_TYPE_STR);
}


bool Node::IsTerOp(void)
{
	if (Op > mir_first_op)
		return (Kids[0] != NULL && Kids[1] != NULL && Kids[2] != NULL);
	return (GET_OP_FLAGS(Op) == IR_TEROP);
}


bool Node::IsBinOp(void)
{
	if (Op > mir_first_op)
		return (Kids[0] != NULL && Kids[1] != NULL && Kids[2] == NULL);
	return (GET_OP_FLAGS(Op) == IR_BINOP);
}


bool Node::IsUnOp(void)
{
	if (Op > mir_first_op)
		return (Kids[0] != NULL && Kids[1] == NULL && Kids[2] == NULL);
	return (GET_OP_FLAGS(Op) == IR_UNOP);
}


bool Node::IsTerm(void)
{
	if (Op > mir_first_op)
		return (Kids[0] == NULL && Kids[1] == NULL && Kids[2] == NULL);
	return (GET_OP_FLAGS(Op) == IR_TERM);
}


bool Node::IsConst(void)
{
	if (Op > mir_first_op)
		return (Op == PUSH);
	return (GET_OP(Op) == OP_CONST);
}


uint32 Node::GetConstVal(void)
{
	nbASSERT(IsConst(), "node must be constant");
	if (Op < HIR_LAST_OP)
		return ((SymbolIntConst *)Sym)->Value;
	
	return Value;
}



//!\todo put this function in a class (e.g. TreeTransform or TreeOptimize)
bool ReverseCondition(HIRNode **expr)
{
	HIRNode *node(*expr);

	//if (!node->IsBoolean())
	//	return false;

	if (node->Op == IR_NOTB)
	{
		//remove the NOT node
		*expr = static_cast<HIRNode*>(node->Kids[0]);
		(*expr)->NRefs--;
		node->Kids[0]=NULL;
		delete node;
		return true;
	}

	if ((*expr)->NRefs > 0)
		(*expr)->NRefs--;
	node = new HIRNode(IR_NOTB, *expr);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	node->Kids[0] = *expr;
	*expr = node;

	return true;
}

MIRONode * Node::translateToMIRONode()
{
	Symbol *newsym = NULL;
	if(Sym)
		newsym = Sym->copy();
	Symbol *newsymEx = NULL;
	if(SymEx)
		newsymEx = SymEx->copy();

	MIRONode *new_node = new MIRONode(Op, newsym);
	new_node->SymEx = newsymEx;

	new_node->setValue(Value);

	for(int i = 0; i < 3; i++)
	{
		if( Kids[i] != NULL)
		{
			new_node->setKid(Kids[i]->translateToMIRONode(), i);
		}
	}
	return new_node;
};


/*************************************HIRNode***************************************************************/

Node *HIRNode::Clone()
{
	HIRNode *newNode = new HIRNode(this->hirOp);
	newNode->Sym = this->Sym;
	newNode->SymEx = this->SymEx;
	
	if (this->Kids[0])
		newNode->Kids[0] = this->Kids[0]->Clone();
	if (this->Kids[1])
		newNode->Kids[1] = this->Kids[1]->Clone();
	if (this->Kids[2])
		newNode->Kids[2] = this->Kids[2]->Clone();
	
	newNode->Value = this->Value;
	newNode->NRefs = this->NRefs;
	
	return newNode;
}

/*************************************MIRNode***************************************************************/

Node *MIRNode::Clone()
{
	MIRNode *newNode = new MIRNode(this->mirOp);
	newNode->Sym = this->Sym;
	newNode->SymEx = this->SymEx;
	
	if (this->Kids[0])
		newNode->Kids[0] = this->Kids[0]->Clone();
	if (this->Kids[1])
		newNode->Kids[1] = this->Kids[1]->Clone();
	if (this->Kids[2])
		newNode->Kids[2] = this->Kids[2]->Clone();
	
	newNode->Value = this->Value;
	newNode->NRefs = this->NRefs;
	
	return newNode;
}


