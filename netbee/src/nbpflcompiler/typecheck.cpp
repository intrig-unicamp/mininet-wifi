/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "typecheck.h"

bool CheckSymbolBounds(Symbol *sym)
{
	switch(sym->SymKind)
	{
	case SYM_FIELD:
		return CheckFieldBounds((SymbolField*)sym);
		break;
	case SYM_RT_VAR:
		return CheckVarBounds((SymbolVariable*)sym);
		break;
		/*
		case SYM_TEMP:
		case SYM_LABEL:
		case SYM_INT_CONST:
		case SYM_STR_CONST:
		case SYM_PROTOCOL:
		*/
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	return false;
}



bool CheckFieldBounds(SymbolField *fldSym)
{
	switch(fldSym->FieldType)
	{
	case PDL_FIELD_FIXED:
		{
			SymbolFieldFixed *fixedField = (SymbolFieldFixed*)fldSym;
			if (fixedField->Size > 4 || fixedField->Size == 0)
				return false;
			return true;
		}break;
	case PDL_FIELD_BITFIELD:
		{
			SymbolFieldBitField *bitField = (SymbolFieldBitField*)fldSym;
			SymbolFieldContainer *container=bitField->DependsOn;
			//uint32 mask = 0;
			// search the fixed field root to get the size
			while (container->FieldType!=PDL_FIELD_FIXED)
			{
				nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

				container=((SymbolFieldBitField *)container)->DependsOn;
			}

			SymbolFieldFixed *fixed=(SymbolFieldFixed *)container;
			if (fixed->Size == 1 || fixed->Size == 2 || fixed->Size == 4)
				return true;
			else
				return false;
		}break;
	case PDL_FIELD_VARLEN:
	case PDL_FIELD_PADDING:
	case  PDL_FIELD_TOKEND:
	case  PDL_FIELD_TOKWRAP:
	case  PDL_FIELD_LINE:
	case  PDL_FIELD_PATTERN:
	case  PDL_FIELD_EATALL:
		return false;
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	return false;
}


bool CheckVarBounds(SymbolVariable *varSym)
{
	switch(varSym->VarType)
	{
	case PDL_RT_VAR_INTEGER:
		return true;
		break;
	case PDL_RT_VAR_REFERENCE:
		{
			SymbolVarBufRef *reference = (SymbolVarBufRef*)varSym;
			if (reference->RefType == REF_IS_PACKET_VAR)
				return false;
			return true;
		}break;
	case PDL_RT_VAR_BUFFER:
		{
			SymbolVarBuffer *buff = (SymbolVarBuffer*)varSym;
			nbASSERT(buff->Size > 0, "size of buffer variable should be > 0");
			if (buff->Size > 4)
				return false;
			return true;
		}break;

	case PDL_RT_VAR_PROTO:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	return false;
}


//[icerrato] i guess Node is an HIRNode
bool CheckRefBounds(Node *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(node->Op == IR_SVAR || node->Op == IR_FIELD, "node must be an IR_SVAR");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	
	//node is an HIRNode
	
	HIRNode *lenExpr(static_cast<HIRNode*>(node)->GetRightChild());

	if (lenExpr == NULL)
		return CheckSymbolBounds(node->Sym);

	if (lenExpr->Op != IR_ICONST)
		return false;

	nbASSERT(lenExpr->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(lenExpr->Sym->SymKind == SYM_INT_CONST, "symbol must be a SYM_INT_CONST");

	SymbolIntConst *intConst = (SymbolIntConst*)lenExpr->Sym;
	if (intConst->Value > 4 || intConst->Value == 0)
		return false;
	return true;

}



bool CheckIntOperand(Node *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");

	switch (node->Op)
	{
	case IR_ICONST:
		// [ds] return IsSupported value
		return node->Sym->IsSupported;

	case IR_IVAR:
		{
			nbASSERT(node->Sym->SymKind != SYM_RT_VAR, "contained symbol must be a SYM_RT_VAR");
			SymbolVariable *varSym = (SymbolVariable*)node->Sym;
			nbASSERT(varSym->VarType == PDL_RT_VAR_INTEGER, "IR_IVAR node does not contain an integer variable symbol");
			// [ds] return IsSupported value
			return node->Sym->IsSupported;
		}break;

	case IR_SVAR:
		{
			SymbolVariable *varSym = (SymbolVariable*)node->Sym;
			switch(varSym->VarType)
			{
			case PDL_RT_VAR_REFERENCE:
			case PDL_RT_VAR_BUFFER:
				// [ds] return checking result or IsSupported value
				return CheckRefBounds(node) || node->Sym->IsSupported;
			case PDL_RT_VAR_PROTO:
				return false;
			default:
				nbASSERT(false, "CANNOT BE HERE");
				return false;
			}
		}break;

	case IR_FIELD:
		// [ds] return checking result or IsSupported or IsDefined value
		return CheckRefBounds(node) || node->Sym->IsSupported || node->Sym->IsDefined;
		break;

	default:
		return (GET_OP_TYPE(node->Op) == I);
		break;
	}

	return false;
}



