/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

/*
*	The management of the variable length fields and of the filed extraction 
*	have been implemented by Ivano Cerrato in 2011/12
*/

#include "irlowering.h"
#include <math.h> //for log10()
#include "statements.h"
#include "../nbee/globals/debug.h"

/*
*	resetCurrentOffset is true if we are generating the code for an extended transition and we are doing field extraction.
* 	In this case, the current offset must be set at the beginning of the header after the code generation. 
*	When ActionState is true, the code for the extraction must be generated
*/
void IRLowering::LowerHIRCode(CodeList *code, SymbolProto *proto, bool ActionState, list<SymbolField*> fields, list<uint32> positions, list<bool> multifield, string comment, bool resetCurrentOffset)
{
	m_Protocol = proto;
	genExtractionCode = ActionState;	

	if(ActionState)
	{	
		allFieldsFound = false;
	
		m_Fields.clear();
		for(list<SymbolField*>::iterator i = fields.begin(); i != fields.end(); i++)
			m_Fields.push_back(*i);

		m_Positions.clear();
		for(list<uint32>::iterator i = positions.begin(); i != positions.end(); i++)
			m_Positions.push_back(*i);
			
		m_MultiFields.clear();
		for(list<bool>::iterator i = multifield.begin(); i != multifield.end(); i++)
			m_MultiFields.push_back(*i);
		
		nbASSERT((m_Fields.size() == m_Positions.size()), "There is a BUG!");
		nbASSERT((m_Fields.size() == m_MultiFields.size()), "There is a BUG!");
	}	
			
	LowerHIRCode(code, comment);
	
	if(ActionState)
	{
		//The current state is an action state. 
		if(proto->ExtractG != NULL)
		{
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,proto->ExtractG), m_CodeGen.TermNode(PUSH, 1)),proto->ExtractG));
		}
		
		if(!allFieldsFound)
		{
			//we have to reset the variables which counts the instance of a field into the header
			for(FieldsList_t::iterator fld = proto->fields_ToExtract.begin(); fld != proto->fields_ToExtract.end(); fld++)
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(PUSH, (uint32)0),(*fld)->FieldCount));
		}	
		else		
		{
			SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(proto, "allfields");
			nbASSERT(symAllFields!=NULL,"There is an error");
			MIRNode *position=m_CodeGen.TermNode(PUSH, proto->beginPosition);
			m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,proto->NExFields), position));
		}
	}
	
	if(resetCurrentOffset)
	{
		m_CodeGen.CommentStatement(string("Field Extraction - Set the current offset at the beginning of this header"));		
		SymbolVarInt *protoOffset =  m_GlobalSymbols.GetProtoOffsVar(proto);
		SymbolTemp *protoOffsTemp = protoOffset->Temp;
		m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, m_CodeGen.CurrOffsTemp(), m_CodeGen.TermNode(LOCLD,protoOffsTemp)));
	}
	
}

void IRLowering::LowerHIRCode(CodeList *code, string comment)
{
	if (comment.compare("")!=0)
		m_CodeGen.CommentStatement(comment);	

	LowerHIRCode(code);
}

void IRLowering::LowerHIRCode(CodeList *code)
{
	StmtBase *next = code->Front();
	while(next)
	{
		TranslateStatement(next);
			next = next->Next;
	}
}

void IRLowering::TranslateLabel(StmtLabel *stmt)
{

	nbASSERT(stmt != NULL ,"stmt  cannot be NULL");
	SymbolLabel *label = ManageLinkedLabel((SymbolLabel*)stmt->Forest->Sym, "");
	m_CodeGen.LabelStatement(label);
	//StmtLabel *lblStmt = m_CodeGen.LabelStatement(label);
	//lblStmt->Comment = stmt->Comment;
}

void IRLowering::TranslateGen(StmtGen *stmt)
{
	HIRNode *tree = static_cast<HIRNode*>(stmt->Forest);


	nbASSERT(tree != NULL, "Forest cannot be NULL in a Gen Statement");
	switch (tree->Op)
	{
	case IR_DEFFLD:
		TranslateFieldDef(tree);
		break;

	case IR_DEFVARI:
		TranslateVarDeclInt(tree);
		break;

	case IR_DEFVARS:
		TranslateVarDeclStr(tree);
		break;

	case IR_ASGNI:
		TranslateAssignInt(tree);
		break;

	case IR_ASGNS:
		TranslateAssignStr(tree);
		break;

	case IR_LKADD:
		TranslateLookupAdd(tree);
		break;

	case IR_LKDEL:
		TranslateLookupDelete(tree);
		break;

	case IR_LKHIT:
	case IR_LKSEL:
		{
			SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)tree->Sym;
			nbASSERT(tree->GetLeftChild() != NULL, "Lookup select instruction should specify a keys list");
			SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)tree->GetLeftChild()->Sym;
			TranslateLookupSelect(entry, keys);
		}
		break;

	case IR_LKUPDS:
	case IR_LKUPDI:
		TranslateLookupUpdate(tree);
		//return true;
		break;
/*[icerrato] never used
	case IR_DATA:
		{
			SymbolDataItem *data = (SymbolDataItem *)tree->Sym;
			m_CodeGen.GenStatement(m_CodeGen.TermNode(IR_DATA, data));
		}
		//return true;
		break;
*/		
	//[icerrato]
	//IR code about this symbols should not be generated
	case IR_LKDEFKEYI:		
	case IR_LKDEFKEYS:
	case IR_LKDEFDATAI:
	case IR_LKDEFDATAS:
	case IR_LKDEFTABLE:
		break;

	default:
		nbASSERT(false, "IRLOWERING::translateGen: CANNOT BE HERE");
		break;
	}
}


SymbolLabel *IRLowering::ManageLinkedLabel(SymbolLabel *label, string name)
{
	nbASSERT(label != NULL, "label cannot be NULL");

	if (label->LblKind != LBL_LINKED){
		return label;
		}

	if (label->Linked != NULL){
		return label->Linked;
		
	}
	
	label->Linked = m_CodeGen.NewLabel(LBL_CODE, name);
	return label->Linked;
}

void IRLowering::TranslateJump(StmtJump *stmt)
{	
	nbASSERT(stmt->TrueBranch != NULL, "true branch should not be NULL");

	SymbolLabel *trueBranch = ManageLinkedLabel(stmt->TrueBranch, "jump_true"); 

	if (stmt->Forest == NULL)
	{
		m_CodeGen.JumpStatement(JUMPW, trueBranch);
		return;
	}

	nbASSERT(stmt->FalseBranch != NULL, "false branch should not be NULL");
	SymbolLabel *falseBranch = ManageLinkedLabel(stmt->FalseBranch, "jump_false");
	JCondInfo jcInfo(trueBranch, falseBranch);
	TranslateBoolExpr(static_cast<HIRNode*>(stmt->Forest), jcInfo);
}


void IRLowering::TranslateRelOpInt(MIROpcodes op, HIRNode *relopExpr, JCondInfo &jcInfo)
{
	MIRNode *leftExpr = TranslateTree(relopExpr->GetLeftChild());
	MIRNode *rightExpr = TranslateTree(relopExpr->GetRightChild());

	if (leftExpr && rightExpr)
	{
		m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, m_CodeGen.BinOp(op, leftExpr, rightExpr));
	}
	else
	{
		m_CodeGen.CommentStatement(string("ERROR: One of two operands was not translated, jump to false by default."));
		m_CodeGen.JumpStatement(JUMPW, jcInfo.FalseLbl);
	}
}

void IRLowering::TranslateRelOpStr(MIROpcodes op, HIRNode *relopExpr, JCondInfo &jcInfo)
{
#ifdef USE_STRING_COMPARISON
	// operand nodes
	MIRNode *leftExpr = 0, *rightExpr = 0;
	// constant sizes (if a size is known at run-time, the constant size will be 0)
	uint32 leftSize = 0, rightSize = 0;
	// node to load sizes
	MIRNode *leftSizeNode = 0, *rightSizeNode = 0;

	nbASSERT(relopExpr!=NULL,"There is a bug!");
	nbASSERT(relopExpr->GetLeftChild()!=NULL,"There is a bug!");
	nbASSERT(relopExpr->GetRightChild()!=NULL,"There is a bug!");

	this->TranslateRelOpStrOperand(relopExpr->GetLeftChild(), &leftExpr, &leftSizeNode, &leftSize);
	this->TranslateRelOpStrOperand(relopExpr->GetRightChild(), &rightExpr, &rightSizeNode, &rightSize);
	
	if (leftExpr!=NULL && rightExpr!=NULL)
	{
		switch (op)
		{
		case JFLDEQ:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					else
						m_CodeGen.IRCodeGen::JumpStatement(jcInfo.FalseLbl);
				}
				else
				{
					//	if ( size(string1)==size(string2) && EQ(string1, string2, size(string1)) )
					//		true;
					//	else
					//		false;
					SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
					JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.JCondStatement(JCMPEQ, andJCInfo.TrueLbl, andJCInfo.FalseLbl, m_CodeGen.TermNode(LOCLD, size_a), rightSizeNode);
					m_CodeGen.LabelStatement(leftTrue);
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(LOCLD, size_a));
				}
			}break;
		case JFLDNEQ:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					else
						m_CodeGen.IRCodeGen::JumpStatement(jcInfo.TrueLbl);
				}
				else
				{
					// if ( size(string1)!=size(string2) || NEQ(string1, string2, size(string1)) )
					//		true;
					//	else
					//		false;
					SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
					JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.JCondStatement(JCMPNEQ, orJCInfo.TrueLbl, orJCInfo.FalseLbl, m_CodeGen.TermNode(LOCLD, size_a), rightSizeNode);
					m_CodeGen.LabelStatement(leftFalse);
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(LOCLD, size_a));
				}
			}break;
		case JFLDGT:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
					{
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					}
					else
					{
						uint32 minSize=min(leftSize, rightSize);

						//	if ( GT(string1, string2, mimSize) || ( EQ(string1, string2, mimSize) && size(string1)>size(string2) ) )
						//		true;
						//	else
						//		false;
						SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
						m_CodeGen.JFieldStatement(op, orJCInfo.TrueLbl, orJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftFalse);

						SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
						m_CodeGen.JFieldStatement(JFLDEQ, andJCInfo.TrueLbl, andJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftTrue);
						m_CodeGen.JCondStatement(JCMPG, jcInfo.TrueLbl, jcInfo.FalseLbl, leftSizeNode, rightSizeNode);
					}
				}
				else
				{
					SymbolTemp *strMinSize=m_CodeGen.NewTemp("str_min_size", m_CompUnit.NumLocals);
					MIRNode *loadStrMinSize=m_CodeGen.TermNode(LOCLD, strMinSize);
					SymbolLabel *minTrue=m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *minFalse=m_CodeGen.NewLabel(LBL_CODE, "if_false");
					SymbolLabel *minDone=m_CodeGen.NewLabel(LBL_CODE, "end_if");
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					SymbolTemp *size_b=m_CodeGen.NewTemp("str_size_b", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, rightSizeNode, size_b));
					m_CodeGen.JCondStatement(JCMPLE, minTrue, minFalse, m_CodeGen.TermNode(LOCLD, size_a), m_CodeGen.TermNode(LOCLD, size_b));
					m_CodeGen.LabelStatement(minTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_a), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_b), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minDone);
					m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, loadStrMinSize);
				}
			}break;
		case JFLDLT:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
					{
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					}
					else
					{
						uint32 minSize=min(leftSize, rightSize);

						//	if ( LT(string1, string2, mimSize) || ( EQ(string1, string2, mimSize) && size(string1)<size(string2) ) )
						//		true;
						//	else
						//		false;
						SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
						m_CodeGen.JFieldStatement(op, orJCInfo.TrueLbl, orJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftFalse);

						SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
						m_CodeGen.JFieldStatement(JFLDEQ, andJCInfo.TrueLbl, andJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftTrue);
						m_CodeGen.JCondStatement(JCMPL, jcInfo.TrueLbl, jcInfo.FalseLbl, leftSizeNode, rightSizeNode);
					}
				}
				else
				{
					SymbolTemp *strMinSize=m_CodeGen.NewTemp("str_min_size", m_CompUnit.NumLocals);
					MIRNode *loadStrMinSize=m_CodeGen.TermNode(LOCLD, strMinSize);
					SymbolLabel *minTrue=m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *minFalse=m_CodeGen.NewLabel(LBL_CODE, "if_false");
					SymbolLabel *minDone=m_CodeGen.NewLabel(LBL_CODE, "end_if");
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					SymbolTemp *size_b=m_CodeGen.NewTemp("str_size_b", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, rightSizeNode, size_b));
					m_CodeGen.JCondStatement(JCMPLE, minTrue, minFalse, m_CodeGen.TermNode(LOCLD, size_a), m_CodeGen.TermNode(LOCLD, size_b));
					m_CodeGen.LabelStatement(minTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_a), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_b), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minDone);
					m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, loadStrMinSize);
				}
			}break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			break;
		}
	}
	else
	{
		this->GenerateWarning("One of the operand of the string comparison has not been defined.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
		m_CodeGen.CommentStatement("ERROR: One of the operand of the string comparison has not been defined.");
	}
#else
	this->GenerateWarning("String comparisons disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
	m_CodeGen.CommentStatement("ERROR: String comparisons disabled.");
#endif
}

void IRLowering::TranslateRelOpStrOperand(HIRNode *operand, MIRNode **loadOperand, MIRNode **loadSize, uint32 *size)
{
	switch(operand->Op)
	{
	case IR_SCONST:
		{
			SymbolTemp *memTemp=m_CodeGen.NewTemp(string("const_str_tmp_offs"), this->m_CompUnit.NumLocals);
			m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, memTemp, m_CodeGen.TermNode(PUSH, ((SymbolStrConst*)operand->Sym)->MemOffset)));
			*loadOperand = m_CodeGen.TermNode(LOCLD, memTemp);
			*size = ((SymbolStrConst*)operand->Sym)->Size;
			*loadSize = m_CodeGen.TermNode(PUSH, *size);
		}break;
	case IR_FIELD:
		{
			switch (((SymbolField *)operand->Sym)->FieldType)
			{
			case PDL_FIELD_FIXED:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldFixed *)operand->Sym)->IndexTemp);
				*size = ((SymbolFieldFixed *)operand->Sym)->Size;
				*loadSize = m_CodeGen.TermNode(PUSH, *size);
				break;
			case PDL_FIELD_VARLEN:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldVarLen *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldVarLen *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_TOKEND:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokEnd *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokEnd *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_TOKWRAP:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokWrap *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokWrap *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_LINE:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldLine *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldLine *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_PATTERN:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldPattern *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldPattern *)operand->Sym)->LenTemp);
				break;
			case PDL_FIELD_EATALL:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldEatAll *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldEatAll *)operand->Sym)->LenTemp);
				break;
			default:
				this->GenerateWarning("Support for string comparisons with fields is supported only for fixed, variable length, tokenended, tokenwrapped, line, pattern or eatall fields.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
				break;
			}
		}break;
	case IR_SVAR:
		{
			SymbolVariable *var=(SymbolVariable *)operand->Sym;
			if (var->VarType==PDL_RT_VAR_REFERENCE)
			{
				SymbolVarBufRef *refVar=(SymbolVarBufRef *)var;
				if (refVar->Name.compare("$packet")==0)
				{
					HIRNode *leftKid=operand->GetLeftChild();
					HIRNode *rightKid=operand->GetRightChild();
					if (leftKid!=NULL && rightKid!=NULL)
					{
						*loadOperand=TranslateTree(leftKid);
						*loadSize=TranslateTree(rightKid);
						//// in this case, we have a offset...
						//switch (leftKid->Sym->SymKind)
						//{
						//case SYM_RT_VAR:
						//	if (((SymbolVariable *)leftKid->Sym)->VarType==PDL_RT_VAR_INTEGER && ((SymbolVariable *)leftKid->Sym)->Name.compare("$currentoffset")==0)
						//		*loadOperand=m_CodeGen.TermNode(LOCLD, m_CodeGen.CurrOffsTemp());
						//	break;
						//}

						//// ... and a size already specified as node
						//switch (rightKid->Sym->SymKind)
						//{
						//case SYM_INT_CONST:
						//	// if we have a constant size, let's store it, we can optimize the code
						//	*size=((SymbolIntConst *)rightKid->Sym)->Value;
						//	*loadSize=m_CodeGen.TermNode(PUSH, *size);
						//	break;
						//default:
						//	*loadSize=m_CodeGen.TermNode(LOCLD, rightKid->Sym);
						//	break;
						//}
					}
					else
					{
						this->GenerateWarning("Support for string comparisons using $packet is supported only with both starting offset and size specified.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
						break;
					}
				}
				else
				{
					if (refVar->IndexTemp==NULL || refVar->LenTemp==NULL)
					{
						this->GenerateWarning("A variable to be used as string should define offset and size.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
						break;
					}
					*loadOperand=m_CodeGen.TermNode(LOCLD, refVar->IndexTemp);

					if (refVar->GetFixedSize()==0)
					{
						// size is not known at compile-time, we should use the symbol to refer to the length
						*loadSize = m_CodeGen.TermNode(LOCLD, refVar->LenTemp);
					}
					else
					{
						// size is fixed, we can use this value
						*size=refVar->GetFixedSize();
						*loadSize = m_CodeGen.TermNode(PUSH, *size);
					}
				}
			}
			else
			{
				this->GenerateWarning("Support for string comparisons with variables is supported only for buffer references.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
				break;
			}
			break;
		}break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}

void IRLowering::TranslateBoolExpr(HIRNode *expr, JCondInfo &jcInfo)
{
	switch(expr->Op)
	{
	case IR_NOTB:
		{
			JCondInfo notJCInfo(jcInfo.FalseLbl, jcInfo.TrueLbl);
			TranslateBoolExpr(expr->GetLeftChild(), notJCInfo);
		};break;
	case IR_ANDB:
		{
			SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
			JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
			TranslateBoolExpr(expr->GetLeftChild(), andJCInfo);
			m_CodeGen.LabelStatement(leftTrue);
			TranslateBoolExpr(expr->GetRightChild(), jcInfo);
		};break;
	case IR_ORB:
		{
			SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
			TranslateBoolExpr(expr->GetLeftChild(), orJCInfo);
			m_CodeGen.LabelStatement(leftFalse);
			TranslateBoolExpr(expr->GetRightChild(), jcInfo);
		}break;

	case IR_EQI:
		TranslateRelOpInt(JCMPEQ, expr, jcInfo);
		break;
	case IR_GEI:
		TranslateRelOpInt(JCMPGE, expr, jcInfo);
		break;
	case IR_GTI:
		TranslateRelOpInt(JCMPG, expr, jcInfo);
		break;
	case IR_LEI:
		TranslateRelOpInt(JCMPLE, expr, jcInfo);
		break;
	case IR_LTI:
		TranslateRelOpInt(JCMPL, expr, jcInfo);
		break;
	case IR_NEI:
		TranslateRelOpInt(JCMPNEQ, expr, jcInfo);
		break;

	case IR_EQS:
		TranslateRelOpStr(JFLDEQ, expr, jcInfo);
		break;
	case IR_NES:
		TranslateRelOpStr(JFLDNEQ, expr, jcInfo);
		break;
	case IR_GTS:
		TranslateRelOpStr(JFLDGT, expr, jcInfo);
		break;
	case IR_LTS:
		TranslateRelOpStr(JFLDLT, expr, jcInfo);
		break;

	case IR_ICONST:
		{
			SymbolIntConst *sym = (SymbolIntConst*)expr->Sym;
			if (sym->Value)
				m_CodeGen.JumpStatement(JUMPW, jcInfo.TrueLbl);
			else
				m_CodeGen.JumpStatement(JUMPW, jcInfo.FalseLbl);
		};break;

	case IR_IVAR:
		{
			SymbolVariable *sym = (SymbolVariable *)expr->Sym;
			switch (sym->VarType)
			{
			case PDL_RT_VAR_INTEGER:
				{
					SymbolVarInt *symInt=(SymbolVarInt *)sym;

					if (symInt->Temp!=NULL) {
						MIRNode *jump=m_CodeGen.BinOp(JCMPGE, m_CodeGen.TermNode(LOCLD, symInt->Temp), m_CodeGen.TermNode(PUSH, 1));
						m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, jump);
					} else {
						this->GenerateWarning(string("The int variable ")+sym->Name+string(" is unassigned"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
						m_CodeGen.CommentStatement(string("ERROR: The int variable ")+sym->Name+string(" is unassigned"));
					}
				}break;
			default:
				{
					this->GenerateWarning(string("The type of the variable ")+sym->Name+string(" is not supported in a boolean expression"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
					m_CodeGen.CommentStatement(string("ERROR: The type of the variable ")+sym->Name+string(" is not supported in a boolean expression"));
				}
			}

		};break;

	case IR_CINT:
		{
			MIRNode *cint=TranslateCInt(expr);

			if (cint!=NULL)
			{
				MIRNode *jump=m_CodeGen.BinOp(JCMPGE, cint, m_CodeGen.TermNode(PUSH, 1));
				m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, jump);
			}
		}break;

	case IR_LKSEL:
		TranslateRelOpLookup(IR_LKSEL, expr, jcInfo);
		break;

	case IR_LKHIT:
		TranslateRelOpLookup(IR_LKHIT, expr, jcInfo);
		break;

	case IR_REGEXFND:
		TranslateRelOpRegExStrMatch(expr, jcInfo,string("regexp"));
		break;
		
	case IR_REGEXXTR:
		nbASSERT(translForExtrString == TRADITIONAL, "There is a BUG!");
		translForExtrString = ADD_COPRO_READ;
		TranslateRelOpRegExStrMatch(expr, jcInfo,string("regexp"));
		nbASSERT(m_VariableToGet != NULL, "There is a BUG!");
		break;
		
	case IR_STRINGMATCHINGFND:
		TranslateRelOpRegExStrMatch(expr, jcInfo,string("stringmatching"));
		break;
	default:
		this->GenerateWarning(string("The node type is not supported"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
		m_CodeGen.CommentStatement(string("ERROR: The node type is not supported"));
		break;
	}
}


void IRLowering::TranslateRelOpRegExStrMatch(HIRNode *expr, JCondInfo &jcInfo, string copro)
{
	nbASSERT(expr->Sym != NULL, "The RegExp operand should define a valid RegExp symbol");

	SymbolRegEx *regExpGeneric = static_cast<SymbolRegEx*>(expr->Sym);

	// set pattern id	
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, regExpGeneric->Id),
		new SymbolLabel(LBL_ID, 0, copro),
		REGEX_OUT_PATTERN_ID));

	
	if(regExpGeneric->MyType()==NETPDL)
	{
		SymbolRegExPDL *regExp = static_cast<SymbolRegExPDL*>(regExpGeneric);

		translForRegExp=OFFSET;
		m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
			TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
			new SymbolLabel(LBL_ID, 0,copro),
			REGEX_OUT_BUF_OFFSET));	
				
		translForRegExp= LENGTH;
		MIRNode *size = TranslateTree(static_cast<HIRNode*>(regExp->Offset));
		
		MIRNode *statement = NULL;
		
		if(translForExtrString != INSIDE_EXTR_STRING)
			statement = (translForRegExp==LENGTH) ? m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL),size) : size;
		else
		{
			nbASSERT(m_VariableToGet != NULL, "There is a BUG!");
			SymbolTemp *tmp = AssociateVarTemp(m_VariableToGet);
			nbASSERT(tmp != NULL, "There is a BUG!");
			statement = m_CodeGen.UnOp(LOCLD,tmp);
		}
		
		nbASSERT(statement != NULL, "There is a BUG!");
		
		m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
			statement,
			new SymbolLabel(LBL_ID, 0,copro),
			REGEX_OUT_BUF_LENGTH));

		translForRegExp=NOTHING;
		
		//needed if we are lowering the HIR opcode IR_REGEXXTR
		if(translForExtrString == ADD_COPRO_READ)
		{	
			nbASSERT(m_VariableToGet == NULL && regExp->MatchedSize != NULL, "There is a BUG! (or two nested switches on string values)");
			m_VariableToGet = regExp->MatchedSize;
		}
	}
	else if(regExpGeneric->MyType()==NETPFL)
	{
		SymbolRegExPFL *regExp = static_cast<SymbolRegExPFL*>(regExpGeneric);
		m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,m_CodeGen.UnOp(LOCLD,regExp->Offset),new SymbolLabel(LBL_ID, 0, copro),REGEX_OUT_BUF_OFFSET));	

		m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.UnOp(LOCLD,regExp->Size),
				new SymbolLabel(LBL_ID, 0, copro),
				REGEX_OUT_BUF_LENGTH));			
	}
	else  
		nbASSERT(false,"Cannot be here!");	

	// find the pattern
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0,copro), REGEX_MATCH_WITH_OFFSET));

	// check the result
	MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, copro), REGEX_IN_MATCHES_FOUND);

	MIRNode *leftExpr = validNode;
	MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
	m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));
}

void IRLowering::TranslateRelOpLookup(HIROpcodes opCode, HIRNode *expr, JCondInfo &jcInfo)
{
	nbASSERT(expr->Sym != NULL, "The lookup selection node should specify a valid lookup table entry symbol");
	nbASSERT(expr->Sym->SymKind == SYM_RT_LOOKUP_ENTRY, "The lookup selection node should specify a valid lookup table entry symbol");
	nbASSERT(expr->GetLeftChild() != NULL && expr->GetLeftChild()->Sym != NULL, "The lookup selection node should specify a valid keys list symbol.");
	nbASSERT(expr->GetLeftChild()->Sym->SymKind == SYM_RT_LOOKUP_KEYS, "The lookup selection node should specify a valid keys list symbol.");

	SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)expr->Sym;
	SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)expr->GetLeftChild()->Sym;

	if (this->TranslateLookupSelect(entry, keys)==false)
	{
		m_CodeGen.JumpStatement(JUMPW, jcInfo.FalseLbl);
	}

	// jump to true if coprocessor returns valid
	MIRNode *validNode = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALID);

	MIRNode *jump=m_CodeGen.BinOp(JCMPEQ, m_CodeGen.TermNode(PUSH, 1), validNode);
	
	if (opCode == IR_LKSEL)
		m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, jump);
	else
	{
		SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
		m_CodeGen.JCondStatement(leftTrue, jcInfo.FalseLbl, jump);
		m_CodeGen.LabelStatement(leftTrue);

		{
			/*
			Here the entry to update has been found:
			- if the entry has been added to be kept max time:
			get the timestamp value
			get the lifespan value
			if timestamp+lifespan > current timestamp -> delete the entry
			- if the entry has been added to be updated on hit:
			get the timestamp value
			set the timestamp to the current value
			- if the entry has been added to be replaced on hit:
			create a copy of the masked entry to update
			remove the mask and set the correct value
			remove the entry to update
			add the new entry
			- if the entry has been added to be added on hit:
			create a copy of the masked entry to update
			remove the mask and set the correct value
			add the new entry
			*/

			// get the entry flag
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, entry->Table->Id),
				entry->Table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			uint32 flagOffset = entry->Table->GetHiddenValueOffset(HIDDEN_FLAGS_INDEX);
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, flagOffset),
				entry->Table->Label,
				LOOKUP_EX_OUT_VALUE_OFFSET));

			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_GET_VALUE));

			MIRNode *flagNode = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALUE);

			StmtSwitch *swStmt = m_CodeGen.SwitchStatement(flagNode);
			swStmt->SwExit = m_CodeGen.NewLabel(LBL_LINKED);
			SymbolLabel *swExit = ManageLinkedLabel(swStmt->SwExit, "switch_exit");


			// create the keepmaxtime case
			SymbolLabel *caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			StmtCase *newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_KEEPMAXTIME), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);
			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Check if the entry should be deleted");
				uint32 offset = entry->Table->GetHiddenValueOffset(HIDDEN_TIMESTAMP_INDEX);
				SymbolTemp *timestampS = m_CodeGen.NewTemp("timestamp_s", m_CompUnit.NumLocals);
				SymbolTemp *timestampUs = m_CodeGen.NewTemp("timestamp_us", m_CompUnit.NumLocals);
				// put the timestamp on the stack
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampS, m_CodeGen.TermNode(TSTAMP_S)));
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampUs, m_CodeGen.TermNode(TSTAMP_US)));

				SymbolTemp *entryTimestampS = m_CodeGen.NewTemp("entry_timestamp_s", m_CompUnit.NumLocals);
				SymbolTemp *lifespan = m_CodeGen.NewTemp("lifespan", m_CompUnit.NumLocals);

				// get lifespan
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->GetHiddenValueOffset(HIDDEN_LIFESPAN_INDEX)),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_GET_VALUE));

				MIRNode *lifespanValueNode = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALUE);

				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, lifespan, lifespanValueNode));


				// get entry timestamp (seconds)
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, offset),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_GET_VALUE));

				MIRNode *entryTimestampSValue = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALUE);

				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, entryTimestampS, entryTimestampSValue));


				SymbolLabel *toDelete = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *dontDelete = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				SymbolLabel *done = m_CodeGen.NewLabel(LBL_CODE, "end_if");
				MIRNode *currentTime = m_CodeGen.TermNode(LOCLD, timestampS);
				MIRNode *packetLimitTime = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, entryTimestampS), m_CodeGen.TermNode(LOCLD, lifespan));
				m_CodeGen.JCondStatement(JCMPGE, toDelete, dontDelete, packetLimitTime, currentTime);
				m_CodeGen.LabelStatement(toDelete);

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_DELETE));

				m_CodeGen.JumpStatement(JUMPW, done);
				m_CodeGen.LabelStatement(dontDelete);
				m_CodeGen.JumpStatement(JUMPW, done);
				m_CodeGen.LabelStatement(done);

			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// create the updateonhit case
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_UPDATEONHIT), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);

			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Update the timestamp of the selected entry");
				uint32 offset = entry->Table->GetHiddenValueOffset(HIDDEN_TIMESTAMP_INDEX);
				SymbolTemp *timestampS = m_CodeGen.NewTemp("timestamp_s", m_CompUnit.NumLocals);
				SymbolTemp *timestampUs = m_CodeGen.NewTemp("timestamp_ms", m_CompUnit.NumLocals);
				// put the timestamp on the stack
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampS, m_CodeGen.TermNode(TSTAMP_S)));
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampUs, m_CodeGen.TermNode(TSTAMP_US)));

				// save new timestamp
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, offset),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(LOCLD, timestampS),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_UPD_VALUE));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, offset+1),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(LOCLD, timestampUs),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_UPD_VALUE));
			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// create the replaceonhit case
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_REPLACEONHIT), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);
			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Replace the masked entry");
				// do the actual work for this case
			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// create the addonhit case
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_ADDONHIT), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);
			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Add an entry related to the masked selected one");
				// do the actual work for this case
			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// by default, go away
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			StmtCase *defCase = new StmtCase(NULL, caseLabel);
			CHECK_MEM_ALLOC(defCase);
			swStmt->Default = defCase;
			m_CodeGen.LabelStatement(caseLabel);
			m_CodeGen.JumpStatement(JUMPW, swExit);


			//generate the exit label
			m_CodeGen.LabelStatement(swExit);
			//reset the switch exit label;
			swStmt->SwExit->Linked = NULL;

		}


		m_CodeGen.JumpStatement(JUMPW, jcInfo.TrueLbl);
	}
}

bool IRLowering::TranslateLookupSelect(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *keys)
{
	m_CodeGen.CommentStatement(string("Lookup an entry in the table ").append(entry->Table->Name));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, entry->Table->Id),
		entry->Table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// reset coprocessor
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_RESET));

	

	if (this->TranslateLookupSetValue(entry, keys, true)==false)
		return false;

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, entry->Table->Id),
		entry->Table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// select the entry with the specified key
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_SELECT));

	return true;
}

void IRLowering::TranslateLookupAdd(HIRNode *node)
{
	SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)node->Sym;
	SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)node->GetLeftChild()->Sym;
	SymbolLookupTableValuesList *values = (SymbolLookupTableValuesList *)node->GetRightChild()->Sym;

	m_CodeGen.CommentStatement(string("Add entry in the table ").append(entry->Table->Name));


	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, entry->Table->Id),
		entry->Table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// reset coprocessor
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_RESET));
	
	// set keys list
	if (this->TranslateLookupSetValue(entry, keys, true)==false)
		return;

	// set values list
	if (this->TranslateLookupSetValue(entry, values, false)==false)
		return;
	
}

void IRLowering::TranslateLookupDelete(HIRNode *node)
{
	nbASSERT(node->Sym != NULL, "The lookup delete node should specify a valid lookup table entry symbol");
	nbASSERT(node->Sym->SymKind == SYM_RT_LOOKUP, "The lookup delete node should specify a valid lookup table entry symbol");

	SymbolLookupTable *table = (SymbolLookupTable *)node->Sym;

	m_CodeGen.CommentStatement(string("Delete selected entry in the table '").append(table->Name).append("'"));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->Id),
		table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// select the entry with the specified key
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_DELETE));

}

void IRLowering::TranslateLookupUpdate(HIRNode *node)
{
	SymbolLookupTable *table = (SymbolLookupTable *)node->Sym;
	HIRNode *offsetNode = node->GetLeftChild();
	HIRNode *newValueNode = node->GetRightChild();

	nbASSERT(table != NULL, "The update instruction should specify a lookup table");
	nbASSERT(newValueNode != NULL, "The update instruction should specify a valid value");
	nbASSERT(offsetNode != NULL, "The update instruction should specify a valid offset");


	m_CodeGen.CommentStatement(string("Update a value of the selected entry in the table ").append(table->Name));

	//SymbolTemp *temp;
	MIRNode *toUpdate = TranslateTree(newValueNode);
	SymbolTemp *valueToUpdate = m_CodeGen.NewTemp("value_to_update", m_CompUnit.NumLocals);
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, toUpdate, valueToUpdate));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->Id),
		table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// set value offset
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, ((SymbolIntConst *)offsetNode->Sym)->Value),
		table->Label,
		LOOKUP_EX_OUT_VALUE_OFFSET));

	// set value
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(LOCLD, valueToUpdate),
		table->Label,
		LOOKUP_EX_OUT_VALUE));

	// update entry
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_UPD_VALUE));
}

//[icerrato] never used
/*void IRLowering::TranslateLookupInit(Node *node)
{
#ifdef USE_LOOKUPTABLE
	// set tables number
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, node->Value),
		new SymbolLabel(LBL_ID, 0, "lookup_ex"),
		LOOKUP_EX_OUT_TABLE_ID));

	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, "lookup_ex"), LOOKUP_EX_OP_INIT));

#else
	this->GenerateWarning(string("Lookup table instructions (init) has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table instructions (init) has not yet been implemented.");
#endif
}
*/

//[icerrato] never used
/*void IRLowering::TranslateLookupInitTable(Node *node)
{
#ifdef USE_LOOKUPTABLE
	SymbolLookupTable *table = (SymbolLookupTable *)node->Sym;

	m_CodeGen.CommentStatement(string("Initialize the table ").append(table->Name));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->Id),
		table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// set entries number
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->MaxExactEntriesNr+table->MaxMaskedEntriesNr),
		table->Label,
		LOOKUP_EX_OUT_ENTRIES));

	// set data size
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->KeysListSize),
		table->Label,
		LOOKUP_EX_OUT_KEYS_SIZE));

	// set value size
	uint32 valueSize = table->ValuesListSize;
	if (table->Validity==TABLE_VALIDITY_DYNAMIC)
	{
		// hidden values
		for (int i=0; i<HIDDEN_LAST_INDEX; i++)
			valueSize += (table->HiddenValuesList[i]->Size/sizeof(uint32));
	}
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, valueSize),
		table->Label,
		LOOKUP_EX_OUT_VALUES_SIZE));

	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_INIT_TABLE));

#else
	this->GenerateWarning(string("Lookup table instructions (init) has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table instructions (init) has not yet been implemented.");
#endif

}*/

bool IRLowering::TranslateLookupSetValue(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *values, bool isKeysList)
{

	// load the value in the coprocessor
	MIRNode *value=NULL;

	SymbolLookupTableItemsList_t::iterator itemIter;
	if (isKeysList)
		itemIter=entry->Table->KeysList.begin();
	else
		itemIter=entry->Table->ValuesList.begin();

	NodesList_t::iterator maskIter;
	if (isKeysList)
		maskIter=((SymbolLookupTableKeysList *)values)->Masks.begin();
	NodesList_t::iterator valueIter = values->Values.begin();
	while (valueIter != values->Values.end())
	{
		uint32 size = (*itemIter)->Size;

		if (isKeysList && (*maskIter)!=NULL)
		{
			// if you have a not null mask, add a dummy value (0)
			uint32 refLoaded = 0;
			MIRNode *pieceOffset = m_CodeGen.TermNode(PUSH, (uint32)(0));
			uint32 bytes2Load = 0;
			// put value in steps of max 4 bytes
			while (refLoaded<size)
			{
				bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));

				// set table id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

					//add value
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					GenMemLoad(pieceOffset, bytes2Load),
					entry->Table->Label,
					LOOKUP_EX_OUT_GENERIC));
							
				if (isKeysList)
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
				else
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

				refLoaded += bytes2Load;
			}
		}
		else
		{
			// in this case you have a value or a key without mask to set
			switch ((*valueIter)->Op)
			{
			case IR_SCONST:
				this->GenerateWarning(string("Conditions with lookup table instructions are implemented only with string variables."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
				m_CodeGen.CommentStatement("ERROR: Conditions with lookup table instructions are implemented only with string variables.");
				return false;
				break;
			case IR_FIELD:
				{
					nbASSERT((*valueIter)->Sym != NULL, "Field IR should contain a valid symbol.");
					//SymbolFieldFixed *field = (SymbolFieldFixed *)(*valueIter)->Sym;

					this->GenerateWarning(string("Conditions with lookup table instructions are implemented only with string variables."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
					m_CodeGen.CommentStatement("ERROR: Conditions with lookup table instructions are implemented only with string variables.");
					return false;
				}break;
			case IR_SVAR:
				{
					nbASSERT((*valueIter)->Sym != NULL, "node symbol cannot be NULL");
					HIRNode *fieldOffset = static_cast<HIRNode*>((*valueIter))->GetLeftChild();
								
					MIRNode *MIRfieldOffset = NULL; //[icerrato]

					if (fieldOffset != NULL)
					{
						MIRfieldOffset = TranslateTree(fieldOffset);
						HIRNode *lenNode = static_cast<HIRNode*>((*valueIter))->GetRightChild();
						nbASSERT(lenNode->Op == IR_ICONST, "lenNode should be an IR_ICONST");
						size = ((SymbolIntConst*)lenNode->Sym)->Value;
					}
					
					SymbolVarBufRef *ref = (SymbolVarBufRef*)(*valueIter)->Sym;

					if (ref->RefType == REF_IS_PACKET_VAR && ref->Name.compare("$packet")==0)
					{
#if 0
						this->GenerateWarning(string("Conditions with lookup table instructions are implemented only with string variables (references to fields)."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
						m_CodeGen.CommentStatement("ERROR: Conditions with lookup table instructions are implemented only with string variables (references to fields).");
						return false;
#else
						uint32 refLoaded = 0;
						MIRNode *pieceOffset = NULL;
						uint32 bytes2Load = 0;
						// SymbolTemp *indexTemp = ref->IndexTemp;
						// put value in steps of max 4 bytes
						while (refLoaded<size)
						{
							if (size>refLoaded)
							{
								//pieceOffset = m_CodeGen.TermNode(LOCLD, indexTemp);
								//if (fieldOffset!=0)
								//	pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, fieldOffset);
								//if (refLoaded>0)
								//	pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, m_CodeGen.TermNode(PUSH, refLoaded));
								if (refLoaded>0)
									pieceOffset = m_CodeGen.BinOp(ADD, MIRfieldOffset, m_CodeGen.TermNode(PUSH, refLoaded));
								else
									pieceOffset = MIRfieldOffset;

								bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));
							}

							// set table id
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
								m_CodeGen.TermNode(PUSH, entry->Table->Id),
								entry->Table->Label,
								LOOKUP_EX_OUT_TABLE_ID));

							if (size>refLoaded)
							{
								// add value
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
								GenMemLoad(pieceOffset, bytes2Load),
								entry->Table->Label,
								LOOKUP_EX_OUT_GENERIC));
							
							}
							else
							{
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, (uint32)0),
									entry->Table->Label,
									LOOKUP_EX_OUT_GENERIC));
							}

							if (isKeysList)
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
							else
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

							refLoaded += bytes2Load;
						}
#endif
					} else if (ref->RefType == REF_IS_REF_TO_FIELD) {
						
					
						nbASSERT(ref->GetFixedMaxSize() <= size, "The reference variable should define a valid max size.");

						SymbolTemp *indexTemp = ref->IndexTemp;
						if (indexTemp == 0)
							indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

						uint32 refLoaded = 0;
						MIRNode *pieceOffset = NULL;
						uint32 bytes2Load = 0;
						// put value in steps of max 4 bytes
						
						while (refLoaded<size)
						{
							if (ref->GetFixedSize()==0)
							{
								// check if ( refLoaded < ref_len)
								SymbolLabel *jumpTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
								SymbolLabel *jumpFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
								SymbolLabel *jumpExit = m_CodeGen.NewLabel(LBL_CODE, "end_if");
								m_CodeGen.JCondStatement(JCMPL, jumpTrue, jumpFalse, m_CodeGen.TermNode(PUSH, refLoaded), m_CodeGen.TermNode(LOCLD, ref->LenTemp));

								m_CodeGen.LabelStatement(jumpTrue);
								//
								// if true, load the current step
								//
								
								pieceOffset = m_CodeGen.TermNode(LOCLD, indexTemp);
								if (fieldOffset!=0)
									pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, MIRfieldOffset);
								if (refLoaded>0)
									pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, m_CodeGen.TermNode(PUSH, refLoaded));
								bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));



								// set table id
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, entry->Table->Id),
									entry->Table->Label,
									LOOKUP_EX_OUT_TABLE_ID));

								if (ref->GetFixedMaxSize()>refLoaded)
								{
									// add value
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										GenMemLoad(pieceOffset, bytes2Load),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));

									
								}
								else
								{
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										m_CodeGen.TermNode(PUSH, (uint32)0),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));
								}

								if (isKeysList)
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
								else
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

								m_CodeGen.JumpStatement(JUMPW, jumpExit);


								m_CodeGen.LabelStatement(jumpFalse);
								//
								// if false, load 0
								//


								// set table id
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, entry->Table->Id),
									entry->Table->Label,
									LOOKUP_EX_OUT_TABLE_ID));
						
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, (uint32)0),
									entry->Table->Label,
									LOOKUP_EX_OUT_GENERIC));

								if (isKeysList)
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
								else

									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));
								m_CodeGen.JumpStatement(JUMPW, jumpExit);

								m_CodeGen.LabelStatement(jumpExit);
							}
							else
							{
								if (ref->GetFixedMaxSize()>refLoaded)
								{
														
									pieceOffset = m_CodeGen.TermNode(LOCLD, indexTemp);
									if (MIRfieldOffset!=0)
										pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, MIRfieldOffset);
									if (refLoaded>0)
										pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, m_CodeGen.TermNode(PUSH, refLoaded));

									bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));
								}

								// set table id
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, entry->Table->Id),
									entry->Table->Label,
									LOOKUP_EX_OUT_TABLE_ID));

								if (ref->GetFixedMaxSize()>refLoaded)
								{
									// add value
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										GenMemLoad(pieceOffset, bytes2Load),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));

									// Field_size
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, (*itemIter)->Size),
									entry->Table->Label,
									LOOKUP_EX_OUT_FIELD_SIZE));
									
								}
								else
								{
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										m_CodeGen.TermNode(PUSH, (uint32)0),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));
								}

								if (isKeysList)
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
								else
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));
							}
							refLoaded += bytes2Load;
						}
					}
				}break;
			default:
				// set table id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				// add value
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					TranslateTree(static_cast<HIRNode*>(*valueIter)),
					entry->Table->Label,
					LOOKUP_EX_OUT_GENERIC));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

				break;
			}
		}

		itemIter++;
		valueIter++;
		if (isKeysList)
			maskIter++;
	}

	if (isKeysList==false)
	{
		// add hidden values
		for (int valueIter=0; valueIter<HIDDEN_LAST_INDEX; valueIter++)
		{
			// set table id
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, entry->Table->Id),
				entry->Table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			// [ds] note that this loop has to be used only with the timestamp (2 int, not initialized)
			//      in other cases, you should change this implementation
			for (uint32 size=0; size < (entry->Table->HiddenValuesList[valueIter]->Size/sizeof(uint32)); size++)
			{
				if (entry->HiddenValues[valueIter]!=NULL)
					value = TranslateTree(static_cast<HIRNode*>(entry->HiddenValues[valueIter]));
				else
					value = m_CodeGen.TermNode(PUSH, (uint32)-1);

				//add value
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					value,
					entry->Table->Label,
					LOOKUP_EX_OUT_GENERIC));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));
			}
		}
	}
	return true;
/*#else
	return false;
#endif*/
}

bool IRLowering::TranslateCase(MIRStmtSwitch *newSwitchSt, StmtCase *caseSt, SymbolLabel *swExit, bool IsDefault)
{
	//if the code for the current case is empty we drop it
	//! \todo manage the number of cases!
	if (caseSt->Code->Code->Empty())
		return false;

	SymbolLabel *caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
	if (IsDefault)
	{
		StmtCase *defCase = new StmtCase(NULL, caseLabel);
		CHECK_MEM_ALLOC(defCase);
		newSwitchSt->Default = defCase;
	}
	else
	{
		StmtCase *newCase = new StmtCase(TranslateTree(static_cast<HIRNode*>(caseSt->Forest)), caseLabel);
		CHECK_MEM_ALLOC(newCase);
		newSwitchSt->Cases->PushBack(newCase);

		// [ds] no. of cases will be computed by actually adding case statements
		newSwitchSt->NumCases++;
	}
	m_CodeGen.LabelStatement(caseLabel);	
	LowerHIRCode(caseSt->Code->Code);
	m_CodeGen.JumpStatement(JUMPW, swExit);
	
	return true;
}


void IRLowering::TranslateCases(MIRStmtSwitch *newSwitchSt, CodeList *cases, SymbolLabel *swExit)
{
	StmtBase *caseSt = cases->Front();
	while(caseSt)
	{
		nbASSERT(caseSt->Kind == STMT_CASE, "statement must be a STMT_CASE");
		TranslateCase(newSwitchSt, (StmtCase*)caseSt, swExit, false);
		caseSt = caseSt->Next;
	}
}

void IRLowering::TranslateSwitch(HIRStmtSwitch *stmt)
{
	MIRStmtSwitch *swStmt = m_CodeGen.SwitchStatement(TranslateTree(static_cast<HIRNode*>(stmt->Forest)));
	// [ds] no. of cases will be computed by actually adding case statements
	//swStmt->NumCases = stmt->NumCases;
	//if the exit label was not already created we must create it
	SymbolLabel *swExit = ManageLinkedLabel(stmt->SwExit, "switch_exit");

	TranslateCases(swStmt, stmt->Cases, swExit);
	if (stmt->Default != NULL)
	{		
		if(!TranslateCase(swStmt, stmt->Default, swExit, true))
			goto label; 
		/*
		*	This goto is a very very bad trick! However it allows us to manage the following situation:
		*
		*	<switch expr="buf2int(label)">
		*		<case value="0"> <nextproto proto="#ip" preferred="true"/> </case>
		*		<case value="2"> <nextproto proto="#ipv6"/> </case>
		*		<default> 
		*			<nextproto proto="#ip"/>
		*		</default>		
		*	</switch>
		*
		*	If the filter doesn't have the keyword fullencap, the default will be empty, and this cause a segfault
		*	during the managing of the control flow graph. Then, we treat this situation as the swith would not have
		*	the default statement.
		*/
	}
	else if (stmt->ForceDefault)
	{
		StmtCase *defCase = new StmtCase(NULL, m_FilterFalse);
		CHECK_MEM_ALLOC(defCase);
		swStmt->Default = defCase;
	}
	else
	{
label:
		StmtCase *defCase = new StmtCase(NULL, swExit);
		CHECK_MEM_ALLOC(defCase);
		swStmt->Default = defCase;
	}

	//generate the exit label
	m_CodeGen.LabelStatement(swExit);
	//reset the switch exit label;
	stmt->SwExit->Linked = NULL;
}

//[icerrato] never used
/*void IRLowering::TranslateSwitch2(StmtSwitch *stmt)
{
	SymbolTemp *tmp = m_CodeGen.NewTemp("", m_CompUnit.NumLocals);
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, TranslateTree(stmt->Forest), tmp));

	//if the exit label was not already created we must create it
	SymbolLabel *swExit = ManageLinkedLabel(stmt->SwExit, "switch_exit");

	StmtCase *caseSt = (StmtCase*)stmt->Cases->Front();
	while(caseSt)
	{
		nbASSERT(caseSt->Kind == STMT_CASE, "statement must be a STMT_CASE");


		if (caseSt->Code->Code->Empty())
			return;
		SymbolLabel *nextCase = m_CodeGen.NewLabel(LBL_CODE, "next");
		SymbolLabel *code = m_CodeGen.NewLabel(LBL_CODE, "code");
		m_CodeGen.JCondStatement(nextCase, code, m_CodeGen.BinOp(JCMPNEQ, m_CodeGen.TermNode(LOCLD, tmp), TranslateTree(caseSt->Forest)));
		m_CodeGen.LabelStatement(code);
		LowerHIRCode(caseSt->Code->Code);
		m_CodeGen.JumpStatement(JUMPW, swExit);
		m_CodeGen.LabelStatement(nextCase);
		caseSt = (StmtCase*)caseSt->Next;
	}
	if (stmt->Default != NULL)
	{
		LowerHIRCode(stmt->Default->Code->Code);
	}
	else if (stmt->ForceDefault)
	{
		m_CodeGen.JumpStatement(JUMPW, m_FilterFalse);
	}
	else
	{
		m_CodeGen.JumpStatement(JUMPW, swExit);
	}

	//generate the exit label
	m_CodeGen.LabelStatement(swExit);
	//reset the switch exit label;
	stmt->SwExit->Linked = NULL;
}*/


void IRLowering::EnterLoop(SymbolLabel *start, SymbolLabel *exit)
{
	LoopInfo loopInfo(start, exit);
	m_LoopStack.push_back(loopInfo);
}

void IRLowering::ExitLoop(void)
{
	nbASSERT(!m_LoopStack.empty(), "The loop stack is empty");
	m_LoopStack.pop_back();
}

void IRLowering::TranslateIf(StmtIf *stmt)
{
	/*
	If both true and false branches are present, it generates:

	cmp (!expr) goto if_N_false
	if_N_true:
	;code for the true branch
	goto if_N_end
	if_N_false:
	;code for the false branch
	if_N_end:

	...

	otherwise it generates:

	cmp (!expr) goto if_N_false
	if_N_true:
	;code for the true branch
	if_N_false:
	...

	*/

	HIRNode *expr = static_cast<HIRNode*>(stmt->Forest);
	ReverseCondition(&expr);

	SymbolLabel *trueLabel = m_CodeGen.NewLabel(LBL_CODE, "if_true");
	SymbolLabel *falseLabel = m_CodeGen.NewLabel(LBL_CODE, "if_false");
	SymbolLabel *exitLabel = m_CodeGen.NewLabel(LBL_CODE, "end_if");

	JCondInfo jcInfo(falseLabel, trueLabel);
	
	nbASSERT(translForExtrString == TRADITIONAL || translForExtrString == INSIDE_EXTR_STRING, "There is a bug!");

	TranslateBoolExpr(expr, jcInfo); 

	//if_N_true:
	m_CodeGen.LabelStatement(trueLabel);
	
	if(translForExtrString == ADD_COPRO_READ)
	{
		//get the length of the matched string from the regexp copro
		nbASSERT(m_VariableToGet != NULL, "There is a BUG!");
		m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0,"regexp"), REGEX_GET_RESULT));

		// get the length
		MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, "regexp"), REGEX_IN_LENGTH_FOUND);

		nbASSERT(AssociateVarTemp(m_VariableToGet) != NULL, "There is a BUG!");
		m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, validNode, AssociateVarTemp(m_VariableToGet)));

		translForExtrString = INSIDE_EXTR_STRING;
	}

	if (stmt->TrueBlock->Code->Empty()==false)
	{
		LowerHIRCode(stmt->TrueBlock->Code/*, "true branch"*/);
		//goto if_N_end
		m_CodeGen.JumpStatement(JUMPW, exitLabel);

		//if_N_false:
		m_CodeGen.LabelStatement(falseLabel);
		if(stmt->FalseBlock->Code->Empty()==false)
		{
			//code for the false branch
			LowerHIRCode(stmt->FalseBlock->Code/*, "false branch"*/);
		}
		//goto if_N_end
		m_CodeGen.JumpStatement(JUMPW, exitLabel);
	}
	else
		m_CodeGen.JumpStatement(JUMPW, exitLabel);
		
	if(translForExtrString == INSIDE_EXTR_STRING)
		translForExtrString = TRADITIONAL;
		
	//if_N_end:
	m_CodeGen.LabelStatement(exitLabel);
}


void IRLowering::GenTempForVar(HIRNode *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(node->Sym != NULL, "node symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind == SYM_RT_VAR, "node symbol must be a SYM_RT_VAR");

	SymbolVariable *varSym = (SymbolVariable*)node->Sym;
	nbASSERT(varSym->VarType == PDL_RT_VAR_INTEGER, "node symbol must be an integer variable");

	SymbolVarInt *intVar = (SymbolVarInt*)varSym;
	SymbolTemp *temp = m_CodeGen.NewTemp(varSym->Name, m_CompUnit.NumLocals);
	intVar->Temp = temp;
}

void IRLowering::TranslateLoop(StmtLoop *stmt)
{
	/* generates the following code:

	$index = $initVal
	goto test
	start:

	loop body

	<inc_statement>
	test:
	if (condition) goto start
	exit:
	...
	*/

	GenTempForVar(static_cast<HIRNode*>(stmt->Forest));
	MIRNode *index = TranslateTree(static_cast<HIRNode*>(stmt->Forest));
	MIRNode *initVal = TranslateTree(static_cast<HIRNode*>(stmt->InitVal));
	//index = initval
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, initVal, index->Sym));
	SymbolLabel *test = m_CodeGen.NewLabel(LBL_CODE);
	SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
	SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
	EnterLoop(start, exit);
	//jump test
	m_CodeGen.JumpStatement(JUMPW, test);
	//start:
	m_CodeGen.LabelStatement(start);
	//<inc_statement> 
	TranslateStatement(stmt->IncStmt);	
	//loop body
	LowerHIRCode(stmt->Code->Code/*, "loop body"*/);
	//test:
	m_CodeGen.LabelStatement(test);
	//<inc_statement>
	JCondInfo jcInfo(start, exit);
	TranslateBoolExpr(static_cast<HIRNode*>(stmt->TermCond), jcInfo);
	//exit:
	m_CodeGen.LabelStatement(exit);
	ExitLoop();
}

void IRLowering::TranslateWhileDo(StmtWhile *stmt)
{
	/* generates the following code:

	goto test
	start:

	;loop body

	test:
	if (condition) goto start else goto exit
	exit:
	...
	*/

#ifdef OPTIMIZE_SIZED_LOOPS
	bool skip=true;

	for (StmtList_t::iterator i=m_Protocol->SizedLoopToPreserve.begin(); i!=m_Protocol->SizedLoopToPreserve.end(); i++)
	{
		if ((*i)==stmt)
			skip=false;
	}

	HIRNode *sentinel = static_cast<HIRNode*>(stmt->Forest)->GetRightChild();

	if (skip == false || sentinel == NULL)
	{
#endif

		SymbolLabel *test = m_CodeGen.NewLabel(LBL_CODE);
		SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
		SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
		EnterLoop(start, exit);
		//jump test
		m_CodeGen.JumpStatement(JUMPW, test);
		//start:
		m_CodeGen.LabelStatement(start);
		//loop body
		LowerHIRCode(stmt->Code->Code, "loop body");
		//test:
		m_CodeGen.LabelStatement(test);
		//if (condition) goto start else goto exit
		JCondInfo jcInfo(start, exit);
		TranslateBoolExpr(static_cast<HIRNode*>(stmt->Forest), jcInfo);
		//exit:
		m_CodeGen.LabelStatement(exit);
		ExitLoop();

#ifdef OPTIMIZE_SIZED_LOOPS
	}
	else
	{
		SymbolVarInt *sentinelVar = (SymbolVarInt *)sentinel->Sym;
		SymbolTemp *currOffsTemp = m_CodeGen.CurrOffsTemp();

		m_CodeGen.TermNode(LOCLD, currOffsTemp);
		// Node *offset = m_CodeGen.TermNode(LOCLD, currOffsTemp);
		MIRNode *newOffset = m_CodeGen.TermNode(LOCLD, sentinelVar->Temp);

		m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, newOffset, currOffsTemp));

		//SymbolLabel *test = m_CodeGen.NewLabel(LBL_CODE);
		//SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
		//SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
		//EnterLoop(start, exit);
		////jump test
		//m_CodeGen.JumpStatement(JUMPW, test);
		////start:
		//m_CodeGen.LabelStatement(start);
		////loop body
		//LowerHIRCode(&stmt->Code.Code, "loop body");
		////test:
		//m_CodeGen.LabelStatement(test);
		////if (condition) goto start else goto exit
		//JCondInfo jcInfo(start, exit);
		//TranslateBoolExpr(stmt->Forest, jcInfo);
		////exit:
		//m_CodeGen.LabelStatement(exit);
		//ExitLoop();
	}
#endif
}

void IRLowering::TranslateDoWhile(StmtWhile *stmt)
{
	/* generates the following code:

	start:

	;loop body

	if (condition) goto start else goto exit
	exit:
	...
	*/
	SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
	SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
	EnterLoop(start, exit);
	//start:
	m_CodeGen.LabelStatement(start);
	//loop body
	LowerHIRCode(stmt->Code->Code, "loop body");
	//if (condition) goto start else goto exit
	JCondInfo jcInfo(start, exit);
	TranslateBoolExpr(static_cast<HIRNode*>(stmt->Forest), jcInfo); 

	//exit:
	m_CodeGen.LabelStatement(exit);
	ExitLoop();
}

void IRLowering::TranslateBreak(StmtCtrl *stmt)
{
	nbASSERT(!m_LoopStack.empty(), "empty loop stack");
	SymbolLabel *exit = m_LoopStack.back().LoopExit;
	m_CodeGen.JumpStatement(JUMPW, exit);
}

void IRLowering::TranslateContinue(StmtCtrl *stmt)
{
	nbASSERT(!m_LoopStack.empty(), "empty loop stack");
	SymbolLabel *start = m_LoopStack.back().LoopStart;
	m_CodeGen.JumpStatement(JUMPW, start);
}

void IRLowering::TranslateFieldDef(HIRNode *node)
{
	nbASSERT(node->Op == IR_DEFFLD, "node must be an IR_DEFFLD");
	HIRNode *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Op == IR_FIELD, "left child must be an IR_FIELD");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_FIELD, "left child symbol must be a SYM_FIELD");
	SymbolField *fieldSym = (SymbolField*)leftChild->Sym;
	SymbolTemp *currOffsTemp = m_CodeGen.CurrOffsTemp();
	SymbolVarInt *protoOffsVar = m_GlobalSymbols.GetProtoOffsVar(m_Protocol);
	nbASSERT(currOffsTemp != NULL, "currOffsTemp cannot be NULL");
	nbASSERT(protoOffsVar != NULL, "m_Protocol->ProtoOffsVar cannot be NULL");
	nbASSERT(protoOffsVar->Temp != NULL, "m_Protocol->ProtoOffsVar->Temp cannot be NULL");

	SymbolTemp *protoOffsTemp = protoOffsVar->Temp;


	// [ds] we will use the symbol that defines different version of the same field
	SymbolField *sym=this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, fieldSym);
	
	m_CodeGen.CommentStatement(string("Parsing field: '")+sym->Name+("' of protocol: '")+sym->Protocol->Name+string("'"));

	bool defined=false;
	bool usedAsInt=false;
	bool usedAsArray=false;
	bool usedAsString=false;
	bool intCompatible=true;

	if (sym->IntCompatible==false)
		intCompatible=false;
	if (sym->UsedAsInt)
		usedAsInt=true;
	if (sym->UsedAsString)
		usedAsString=true;
	if (sym->UsedAsArray)
		usedAsArray=true;

	MIRNode *offset=NULL;

	// offset = $currentoffset
	offset=m_CodeGen.TermNode(LOCLD, currOffsTemp);
	
	if ((sym->FieldType==PDL_FIELD_FIXED || sym->FieldType==PDL_FIELD_VARLEN || sym->FieldType==PDL_FIELD_TOKEND|| sym->FieldType==PDL_FIELD_TOKWRAP
				|| sym->FieldType==PDL_FIELD_LINE || sym->FieldType==PDL_FIELD_PATTERN || sym->FieldType==PDL_FIELD_EATALL ) && sym->DependsOn!=NULL)
	{
		// shift the offset according to previous defined fields in this container
		SymbolFieldContainer *container=sym->DependsOn;
		switch (sym->FieldType)
		{
		case PDL_FIELD_FIXED:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldFixed *)sym->DependsOn)->IndexTemp);
			break;
		case PDL_FIELD_VARLEN:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldVarLen *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_TOKEND:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokEnd *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_TOKWRAP:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokWrap *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_LINE:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldLine *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_PATTERN:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldPattern *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_EATALL:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldEatAll *)sym->DependsOn)->IndexTemp);
			break;

		default:
			break;

		}

		bool offsetIncremented = false; 
		SymbolTemp *offsetAux = m_CodeGen.NewTemp(string("OffsetAux_ind"), m_CompUnit.NumLocals); 
		MIRNode *offNode = static_cast<MIRNode*>(offset->Clone());
		m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offNode, offsetAux));

		FieldsList_t::iterator innerField =  container->InnerFields.begin();
		while (innerField != container->InnerFields.end() && *innerField!=sym)
		{
			offsetIncremented = true; 

			switch ((*innerField)->FieldType)
			{
			case PDL_FIELD_FIXED:
				{
					SymbolFieldFixed *fixedFieldSym = static_cast<SymbolFieldFixed*>(*innerField);
					SymbolTemp *lenTemp = fixedFieldSym->LenTemp;
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD,m_CodeGen.UnOp(LOCLD,lenTemp),m_CodeGen.UnOp(LOCLD,offsetAux)), offsetAux));
				}
				break;
			case PDL_FIELD_VARLEN:
				{
					SymbolFieldVarLen *varLenFieldSym = static_cast<SymbolFieldVarLen*>(*innerField);
					SymbolTemp *lenTemp = varLenFieldSym->LenTemp;
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD,m_CodeGen.UnOp(LOCLD,lenTemp),m_CodeGen.UnOp(LOCLD,offsetAux)), offsetAux));

				}
				break;
			case PDL_FIELD_BITFIELD:
				break;
			case PDL_FIELD_PADDING:
				break;
			case PDL_FIELD_TOKEND:
				{				
					SymbolFieldTokEnd *tokendFieldSym = static_cast<SymbolFieldTokEnd*>(*innerField);
					SymbolTemp *lenTemp = tokendFieldSym->LenTemp;					
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD,m_CodeGen.UnOp(LOCLD,lenTemp),m_CodeGen.UnOp(LOCLD,offsetAux)), offsetAux));
				break;
				}

			case PDL_FIELD_TOKWRAP:
				{
					nbASSERT(false,"Inner tokenwrap fields are not supported");
					//the following code is probably wrong

					MIRNode *begin=offset;
					MIRNode *len = m_CodeGen.TermNode(PUSH, (uint32) 0);
					MIRNode *new_begin = m_CodeGen.TermNode(PUSH, (uint32) 0);
					MIRNode *offsetNode=offset;
					MIRNode *lenRegEx;

					SymbolTemp *lenTemp = m_CodeGen.NewTemp(sym->Name + string("_len"), m_CompUnit.NumLocals);
					((SymbolFieldTokWrap *)sym)->LenTemp = lenTemp;

					if(((SymbolFieldTokWrap *)sym)->BeginTok!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->BeginTok));

						SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						nbASSERT(sym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
						m_GlobalSymbols.StoreRegExEntry(regExp,sym->Protocol);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(static_cast<HIRNode*>(regExp->Offset) )),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						MIRNode *leftExpr = validNode;
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32) 0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
	
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
							//if the field doesn't start from $current_offset, the field doesn't exist so don't update the offset
							SymbolLabel *labelBeginTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
							SymbolLabel *labelBeginFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
							m_CodeGen.JCondStatement(labelBeginTrue, labelBeginFalse, m_CodeGen.BinOp(JCMPEQ, offset,m_CodeGen.BinOp(SUB,offsetNode,lenRegEx)));
							m_CodeGen.LabelStatement(labelBeginFalse);
								break;
							m_CodeGen.LabelStatement(labelBeginTrue);
								begin=m_CodeGen.BinOp(SUB,offsetNode, lenRegEx);

					}
					else if(((SymbolFieldTokWrap *)sym)->BeginRegEx!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->BeginRegEx));

						SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						nbASSERT(sym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
						m_GlobalSymbols.StoreRegExEntry(regExp,sym->Protocol);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(static_cast<HIRNode*>(regExp->Offset) )),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						MIRNode *leftExpr = validNode;
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32) 0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
							begin=m_CodeGen.BinOp(SUB,offsetNode, lenRegEx);
					}
					else
						break;

					if(((SymbolFieldTokWrap *)sym)->EndTok!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->EndTok));
						//Search the endtoken from the first byte after th end of begintoken
						SymbolRegExPDL *regExp = new SymbolRegExPDL(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						nbASSERT(sym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
						m_GlobalSymbols.StoreRegExEntry(regExp,sym->Protocol);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(static_cast<HIRNode*>(regExp->Offset) )),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						MIRNode *leftExpr = validNode;
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH,(uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							len=m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(SUB,offsetNode,begin), lenTemp);
					}
					else if(((SymbolFieldTokWrap *)sym)->EndRegEx!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->EndRegEx));

						SymbolRegExPDL *regExp = new SymbolRegExPDL(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						nbASSERT(sym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
						m_GlobalSymbols.StoreRegExEntry(regExp,sym->Protocol);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(static_cast<HIRNode*>(regExp->Offset) )),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						MIRNode *leftExpr = validNode;
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH,(uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							len=m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(SUB,offsetNode,begin), lenTemp);
					}
					else
						break;


					if(((SymbolFieldTokWrap *)sym)->BeginOff!=NULL)
					{
					new_begin = TranslateTree( static_cast<HIRNode*>(((SymbolFieldTokWrap *)sym)->BeginOff ));
					begin=m_CodeGen.BinOp(ADD,offset,new_begin);
					}

					if(((SymbolFieldTokWrap *)sym)->EndOff!=NULL)
					{
						MIRNode *offExpr = TranslateTree(static_cast<HIRNode*>( ((SymbolFieldTokWrap *)sym)->EndOff ));
						len = m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(SUB,m_CodeGen.BinOp(ADD,offExpr,offset),begin), lenTemp);

					}

					offset=m_CodeGen.BinOp(ADD,begin,len);

					if(((SymbolFieldTokWrap *)sym)->EndDiscard!=NULL)
					{
						SymbolTemp *discTemp = m_CodeGen.NewTemp(sym->Name + string("_disc"), m_CompUnit.NumLocals);
						((SymbolFieldTokWrap *)sym)->DiscTemp = discTemp;
						MIRNode *discExpr = TranslateTree( static_cast<HIRNode*>(((SymbolFieldTokWrap *)sym)->EndDiscard ));
						MIRNode *disc = m_CodeGen.UnOp(LOCST, discExpr, discTemp);
						offset=m_CodeGen.BinOp(ADD,offset,disc);
					}
					break;
				}


			case PDL_FIELD_LINE:
				{
					SymbolFieldLine *lineFieldSym = static_cast<SymbolFieldLine*>(*innerField);
					SymbolTemp *lenTemp = lineFieldSym->LenTemp;
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD,m_CodeGen.UnOp(LOCLD,lenTemp),m_CodeGen.UnOp(LOCLD,offsetAux)), offsetAux));
				break;	
				}
		
				case PDL_FIELD_PATTERN:
				{
					SymbolFieldPattern *patternFieldSym = static_cast<SymbolFieldPattern*>(*innerField);
					SymbolTemp *lenTemp = patternFieldSym->LenTemp;
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD,m_CodeGen.UnOp(LOCLD,lenTemp),m_CodeGen.UnOp(LOCLD,offsetAux)), offsetAux));
					break;
				}
				
			case PDL_FIELD_EATALL:
				nbASSERT(false, "The \"eatall\" field must be the last innerfield");
				break;
			default:
				break;
			}

			innerField++;
		}

		if(offsetIncremented)
			offset = m_CodeGen.UnOp(LOCLD, offsetAux );			
	}
	
	switch(fieldSym->FieldType)
	{
		case PDL_FIELD_FIXED: 
		{
			SymbolFieldFixed *fixedFieldSym = static_cast<SymbolFieldFixed*>(sym);
			
			/*
			*	In case of fixed field, depending on some condition that (at least for me - Ivano) are unknown, we generate
			*	two variables: field_value and field_ind.
			*	When the field must be extracted, and this field is "usedAsInt", both the variables are required. Then, I check
			*	hire if the extraction must be performed, in order to generate the proper variables.
			*/
			uint32 position = 0;
			bool multiField = false;
			bool allFields = false;
			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mfIt =  m_MultiFields.begin();
			
			for(; it != m_Fields.end(); it++, pIt++, mfIt++)
			{
				if(*it == fixedFieldSym || (*it)->Name == "allfields")
				{
					extract = true;
					position = *pIt;
					multiField = *mfIt;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}

									
			//The fixed fields do not need a variable called fieldname_len
			
#ifdef ENABLE_FIELD_OPT			
			if(fixedFieldSym->Used)
			{
#endif
				if (usedAsInt)
				{
					if (intCompatible==false)
					{
						this->GenerateWarning(string("Field ")+fieldSym->Name+(" is used as integer, but is not compatible to integer type; definition is aborted"), __FILE__, __FUNCTION__, __LINE__, 1, 4);
						m_CodeGen.CommentStatement(string("ERROR: Field ")+fieldSym->Name+(" is used as integer, but is not compatible to integer type; definition is aborted"));

						if (sym->DependsOn==NULL)
						{
							//$add = $currentoffset + size
							MIRNode *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(PUSH, fixedFieldSym->Size));
							//$currentoffset = $add
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
						}
						return;
					}

					if (fixedFieldSym->ValueTemp==NULL)
					{
						SymbolTemp *valueTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_value"), m_CompUnit.NumLocals);
						fixedFieldSym->ValueTemp = valueTemp;
					}

					//$fld_value = load($currentoffset)
					MIRNode *field_ind=offset;
					MIRNode *field_value=this->GenMemLoad(field_ind, fixedFieldSym->Size);
					if (fixedFieldSym->Size==3)
					{
						// the field size is 3 bytes, you should align the value to 4 bytes (xx.xx.xx.00 -> 00.xx.xx.xx)
						field_value=m_CodeGen.BinOp(SHR, field_value, m_CodeGen.TermNode(PUSH, 8));
					}
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, field_value, fixedFieldSym->ValueTemp));

					if(genExtractionCode && extract) 
					{
						/*
						* We are here because this field is used both inside the format (or the after) (information given by "usedAsInt") and outside (information given by 
						* fixedFieldSym->UsedOutFormatAndAfter). Because within the format (and the after) the field is referred with the variable fieldname_value, while outside it
						* is referred with the variable fieldname_ind, we must create also this second variable.
						*/
						if (fixedFieldSym->IndexTemp==NULL)
						{
							SymbolTemp *indexTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
							fixedFieldSym->IndexTemp = indexTemp;
						}
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, currOffsTemp), fixedFieldSym->IndexTemp));
					}
					defined=true;
				}//end if(usedAsInt)
				
				if (usedAsString || usedAsArray || fixedFieldSym->InnerFields.size())
				{			

					if (fixedFieldSym->IndexTemp==NULL)
					{
						SymbolTemp *indexTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
						fixedFieldSym->IndexTemp = indexTemp;
					}
			
					//$fld_index = $currentoffset
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, currOffsTemp), fixedFieldSym->IndexTemp));

					defined=true;
				}

				if (defined==false) 
				{
					if (fixedFieldSym->IndexTemp==NULL)
					{
						SymbolTemp *indexTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
						fixedFieldSym->IndexTemp = indexTemp;
					}

					//$fld_index = $currentoffset
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, fixedFieldSym->IndexTemp));
				}
	
#ifdef ENABLE_FIELD_OPT
			}
#endif

			if (fixedFieldSym->DependsOn==NULL)
			{
				//$add = $currentoffset + size
				MIRNode *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(PUSH, fixedFieldSym->Size));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}
			
			nbASSERT(! (multiField && fixedFieldSym->MultiProto),"MultiField and MultiProto together? This is not allowed in the grammar!");

						
			if(genExtractionCode && extract) 
			{
#ifdef ENABLE_FIELD_OPT
				nbASSERT(fixedFieldSym->Used,"How is it possibile? You want to extact the value of an unused fixed field!");
#endif
			
				//if we want to extract each field, we have to generate little different code
				if (!allFields)//!fixedFieldSym->Protocol->ExAllfields)
				{			
					if(fixedFieldSym->MultiProto)
					{
						/*
						*	proto*.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiProto - Test"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiPtoto_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiProto_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + fixedFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, fixedFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH, fixedFieldSym->Size), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + fixedFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)),fixedFieldSym->FieldCount));
						
						m_CodeGen.LabelStatement(labelFalse);
					}
					
					else if(multiField)
					{
						/*
						*	proto%n.field*
						*	proto.field*
						*	Extract each instance of a field
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + fixedFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, fixedFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH, fixedFieldSym->Size), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), fixedFieldSym->FieldCount));
						m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + fixedFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, /*fixedFieldSym->Position*/position)));
					}

					else
					{
						/*
						*	proto.field
						*	proto%n.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*/
						m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("Field extraction - extract ") + fixedFieldSym->Name);

						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,fixedFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, /*fixedFieldSym->Position*/position)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,fixedFieldSym->Size), m_CodeGen.TermNode(PUSH, /*(fixedFieldSym->Position)*/position+2)));
						m_CodeGen.CommentStatement(string("Field extraction - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), fixedFieldSym->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}	
				}

				else
				{  
					//if the field hasn't valid PDLFieldInfo is a bit_union field and it contains bitfields
					if(fixedFieldSym->PDLFieldInfo!=NULL)
					{  
						allFieldsFound = true;
					
						//we have to obtain the symbol for allfields
						SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
						nbASSERT(symAllFields!=NULL,"There is an error");
										
						MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,fixedFieldSym->ID), positionId));
						MIRNode *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,fixedFieldSym->IndexTemp), positionOff));
						MIRNode *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,fixedFieldSym->Size), positionLen));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),fixedFieldSym->Protocol->position));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),fixedFieldSym->Protocol->NExFields));
					}
				}
			}

		}break;

	case PDL_FIELD_VARLEN:
		{
			SymbolFieldVarLen *varlenFieldSym = (SymbolFieldVarLen*)sym;
			
#ifdef ENABLE_FIELD_OPT
			if(varlenFieldSym->Used)
			{
#endif
				SymbolTemp *indexTemp = m_CodeGen.NewTemp(varlenFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
				varlenFieldSym->IndexTemp = indexTemp;
				//$fld_index = $currentoffset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
#ifdef ENABLE_FIELD_OPT
			}
#endif

			SymbolTemp *lenTemp = m_CodeGen.NewTemp(varlenFieldSym->Name + string("_len"), m_CompUnit.NumLocals); 
			varlenFieldSym->LenTemp = lenTemp;
			
			MIRNode *lenExpr = TranslateTree(static_cast<HIRNode*>(varlenFieldSym->LenExpr));
			
			//$fld_len = lenExpr
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, lenExpr, varlenFieldSym->LenTemp));

			if (sym->DependsOn==NULL)
			{
				//$add = $currentoffset + $len
				MIRNode *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, varlenFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}
			
			uint32 position = 0;			
			bool multiField = false;
			bool allFields = false;
			
			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mfIt =  m_MultiFields.begin();
			for(; it != m_Fields.end(); it++, pIt++, mfIt++)
			{
				if(*it == varlenFieldSym || (*it)->Name == "allfields")
				{
					extract = true;
					position = *pIt;
					multiField = *mfIt;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}

			if(genExtractionCode && extract)
			{			
#ifdef ENABLE_FIELD_OPT
				nbASSERT(varlenFieldSym->Used,"How is it possibile? You want to extact the value of an unused variable length field!");
#endif
				if(!allFields)
				{
					if(varlenFieldSym->MultiProto)
					{
						/*
						*	proto*.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiProto - Test"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + varlenFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.UnOp(LOCLD,varlenFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + varlenFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)),varlenFieldSym->FieldCount));
						
						m_CodeGen.LabelStatement(labelFalse);
					}

					else if(multiField)
					{
						/*
						*	proto%n.field*
						*	proto.field*
						*	Extract each instance of a field
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + varlenFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.UnOp(LOCLD,varlenFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), varlenFieldSym->FieldCount));
						m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + varlenFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, position)));			
					}

					else
					{
						/*
						*	proto.field
						*	proto%n.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*/
						m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("Field extraction - extract ") + varlenFieldSym->Name);
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.UnOp(LOCLD,varlenFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, position+2)));
						m_CodeGen.CommentStatement(string("Field extraction - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), varlenFieldSym->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}
				}

				else
				{			
					//if the field hasn't valid PDLFieldInfo is a bit_union field and it contains bitfields
					if(varlenFieldSym->PDLFieldInfo!=NULL)
					{  
						allFieldsFound = true;
	
						//we have to obtain the symbol for allfields
						SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
						nbASSERT(symAllFields!=NULL,"There is an error");
							
						MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,varlenFieldSym->ID), positionId));
						MIRNode *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->IndexTemp), positionOff));
						MIRNode *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.UnOp(LOCLD,varlenFieldSym->LenTemp), positionLen));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),varlenFieldSym->Protocol->position));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),varlenFieldSym->Protocol->NExFields));		  
					}
				}
			}
		}break;

	case PDL_FIELD_BITFIELD:
		{
			SymbolFieldBitField* bitField=(SymbolFieldBitField*)sym;
			
			uint32 position = 0;
			bool multiField = false;
			bool allFields = false;
			
			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mfIt =  m_MultiFields.begin();
			for(; it != m_Fields.end(); it++, pIt++, mfIt++)
			{
				if(*it == bitField || (*it)->Name == "allfields")
				{
					extract = true;
					position = *pIt;
					multiField = *mfIt;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}

			if(genExtractionCode && extract)
			{
				if(!allFields)
				{
					nbASSERT(bitField->DependsOn != NULL, "depends-on pointer should be != NULL");
					MIRNode *memOffs(0); 
					uint32 mask=bitField->Mask;
					SymbolFieldContainer *container=(SymbolFieldContainer *)bitField->DependsOn;

					// search the fixed field root to get the size
					while (container->FieldType!=PDL_FIELD_FIXED)
					{
						nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

						// and the mask with the parent mask
						mask=mask & ((SymbolFieldBitField *)container)->Mask;

						container=((SymbolFieldBitField *)container)->DependsOn;
					}

					SymbolFieldContainer *fieldContainer=(SymbolFieldContainer *)this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, container);
					SymbolFieldFixed *fixed=(SymbolFieldFixed *)fieldContainer;

					if(fixed->IndexTemp != NULL)
					{
						// after applying the bit mask, you should also shift the field to align the actual value
						uint8 shift=0;
						uint32 tMask=mask;

						while ((tMask & 1)==0)
						{
							shift++;
							tMask=tMask>>1;
						}

						memOffs = m_CodeGen.TermNode(LOCLD, fixed->IndexTemp);
						MIRNode *memLoad = GenMemLoad(memOffs, fixed->Size);
						MIRNode *_field = m_CodeGen.BinOp(AND, memLoad, m_CodeGen.TermNode(PUSH, mask));

						if (bitField->MultiProto) 
						{
							/*
							*	proto*.field
							*	Extract the first instance of field
							*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
							*	The offset into the info partition must be calculated
							*/
							m_CodeGen.CommentStatement(string("MultiProto - Test"));
							MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, bitField->FieldCount);
							MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
							SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
							SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
							m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

							m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + bitField->Name);
							MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
							m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
							SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
							if (shift>0)
							{   
								MIRNode *validation=m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)),m_CodeGen.TermNode(PUSH,0x80000000));
								m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
							}
							else
							{
								MIRNode *validation=m_CodeGen.BinOp(ADD,_field,m_CodeGen.TermNode(PUSH,0x80000000));
								m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
							}
							m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + bitField->Name + " into the info partition");
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
							m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->FieldCount), m_CodeGen.TermNode(PUSH, 1)),bitField->FieldCount));
						
							m_CodeGen.LabelStatement(labelFalse);
						}

						else if (multiField)
						{
							/*
							*	proto%n.field*
							*	proto.field*
							*	Extract each instance of a field
							*	The offset into the info partition must be calculated
							*/
							m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + bitField->Name);
							MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, bitField->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
							m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
							SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
							if (shift>0)
							{   
								MIRNode *validation=m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)),m_CodeGen.TermNode(PUSH,0x80000000));
								m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
							}
							else
							{
								MIRNode *validation=m_CodeGen.BinOp(ADD,_field,m_CodeGen.TermNode(PUSH,0x80000000));
								m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
							}
							m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->FieldCount), m_CodeGen.TermNode(PUSH, 1)), bitField->FieldCount));
							m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + bitField->Name + " into the info partition");
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, bitField->FieldCount), m_CodeGen.TermNode(PUSH,position)));				
						}

						else
						{
							/*
							*	proto.field
							*	proto%n.field
							*	Extract the first instance of field
							*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
							*/
							m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
							MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, bitField->FieldCount);
							MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
							SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_true");
							SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
							m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

							m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.CommentStatement(string("Field extraction - extract ") + bitField->Name);
							if (shift>0)
							{   
								MIRNode *validation=m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)),m_CodeGen.TermNode(PUSH,0x80000000));
								m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(PUSH, position)));
							}
							else
							{
								MIRNode *validation=m_CodeGen.BinOp(ADD,_field,m_CodeGen.TermNode(PUSH,0x80000000));
								m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(PUSH, position)));
							}
							m_CodeGen.CommentStatement(string("Field extraction - Increase field counter"));
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->FieldCount), m_CodeGen.TermNode(PUSH, 1)), bitField->FieldCount));

							m_CodeGen.LabelStatement(labelFalse);
						}
					}
				}

				else
				{			
					allFieldsFound = true;
					
					//we have to obtain the symbol for allfields
					SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
					nbASSERT(symAllFields!=NULL,"There is an error");
									
					MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,bitField->ID), positionId));
					MIRNode *positionValue= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
					
					MIRNode *memOffs(0);
					uint32 mask=bitField->Mask;
					SymbolFieldContainer *container=(SymbolFieldContainer *)bitField->DependsOn;
					
					// search the fixed field root to get the size
					while (container->FieldType!=PDL_FIELD_FIXED)

					{
						nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

						// and the mask with the parent mask
						mask=mask & ((SymbolFieldBitField *)container)->Mask;


						container=((SymbolFieldBitField *)container)->DependsOn;
					}

									SymbolFieldContainer *fieldContainer=(SymbolFieldContainer *)this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, container);
									SymbolFieldFixed *fixed=(SymbolFieldFixed *)fieldContainer;


					if(fixed->IndexTemp != NULL)
					{
						// after applying the bit mask, you should also shift the field to align the actual value
						uint8 shift=0;
						uint32 tMask=mask;


						while ((tMask & 1)==0)
						{
							shift++;
							tMask=tMask>>1;
						}


						memOffs = m_CodeGen.TermNode(LOCLD, fixed->IndexTemp);
						MIRNode *memLoad = GenMemLoad(memOffs, fixed->Size);
						MIRNode *_field = m_CodeGen.BinOp(AND, memLoad, m_CodeGen.TermNode(PUSH, mask));

						if (shift>0)
						{   m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)), positionValue));
						}
						else
						{	m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, _field, positionValue));
						}

					}
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),bitField->Protocol->position));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),bitField->Protocol->NExFields));		
				}
			}
		
		}
		break;
	case PDL_FIELD_PADDING:
		{
			/*
			From NetPDLProtoDecoder.cpp in the NetBee libray sources:
			FieldLen= (StartingOffset - m_protoStartingOffset) % ( ((struct _nbNetPDLElementFieldPadding *)FieldElement)->Align);
			In other words:
			$fld_len = ($currentoffset-$protooffset) MOD $fld_align
			*/

			SymbolFieldPadding *paddingFieldSym = (SymbolFieldPadding*)fieldSym;
			MIRNode *currOffsNode = m_CodeGen.TermNode(LOCLD, currOffsTemp);
			MIRNode *protoOffsNode = m_CodeGen.TermNode(LOCLD, protoOffsTemp);
			//$fld_len = ($currentoffset-$protooffset) MOD $fld_align
			MIRNode *lenExpr = m_CodeGen.BinOp(MOD, m_CodeGen.BinOp(SUB, currOffsNode, protoOffsNode), m_CodeGen.TermNode(PUSH, paddingFieldSym->Align));
			//$add = $currentoffset + $len
			MIRNode *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), lenExpr);
			//$currentoffset = $add
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
		}break;

	case PDL_FIELD_TOKEND:
		{
			SymbolFieldTokEnd *tokendFieldSym = (SymbolFieldTokEnd*)sym;
			
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(tokendFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			tokendFieldSym->IndexTemp = indexTemp;
						
			//$fld_index = $currentoffsets
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
			
			SymbolTemp *lenTemp;
			MIRNode *len = m_CodeGen.TermNode(PUSH, (uint32) 0); 
			lenTemp = m_CodeGen.NewTemp(tokendFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
			tokendFieldSym->LenTemp = lenTemp;

			SymbolLabel *len_false = m_CodeGen.NewLabel(LBL_CODE, "if_false");
		
			
			if(tokendFieldSym->EndTok!=NULL)
			{
				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokendFieldSym->EndTok));
				SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
				nbASSERT(tokendFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
				m_GlobalSymbols.StoreRegExEntryUseful(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));
				
				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.UnOp(LOCLD,tokendFieldSym->IndexTemp), 
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));
				
				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL),m_CodeGen.UnOp(LOCLD,tokendFieldSym->IndexTemp)),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));

				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				MIRNode *validNode = m_CodeGen.UnOp(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				MIRNode *leftExpr = validNode;
				MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));
				
				
				
				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					m_CodeGen.JumpStatement(JUMPW, len_false); 

				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					len = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
			}
			else if(tokendFieldSym->EndRegEx!=NULL)
			{ 
				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokendFieldSym->EndRegEx));
				SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
				nbASSERT(tokendFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
				m_GlobalSymbols.StoreRegExEntryUseful(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.UnOp(LOCLD,tokendFieldSym->IndexTemp), 
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), m_CodeGen.UnOp(LOCLD,tokendFieldSym->IndexTemp)),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				MIRNode *leftExpr = validNode;
				MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					m_CodeGen.JumpStatement(JUMPW, len_false);
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					len = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
			}
			else
				break;

			if(tokendFieldSym->EndOff!=NULL)
			{
				len = TranslateTree( static_cast<HIRNode*>(tokendFieldSym->EndOff ));
			}

			if(tokendFieldSym->EndDiscard!=NULL)
			{
				if (tokendFieldSym->DiscTemp==NULL)
				{
					SymbolTemp *discTemp = m_CodeGen.NewTemp(tokendFieldSym->Name + string("_disc"), m_CompUnit.NumLocals);
					tokendFieldSym->DiscTemp = discTemp;
				}
			
				MIRNode *discExpr = TranslateTree(static_cast<HIRNode*>(tokendFieldSym->EndDiscard));
				//$fld_disc = discExpr
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, discExpr, tokendFieldSym->DiscTemp));
			}
			

			if (sym->DependsOn==NULL)
			{	
				//$fld_index = $currentoffset
				MIRNode *addNode;
				//$add = $currentoffset + $len

				addNode = m_CodeGen.BinOp(ADD, m_CodeGen.UnOp(LOCLD, currOffsTemp), m_CodeGen.UnOp(LOCLD,tokendFieldSym->LenTemp));

				//$add = $add +$discard
				if(tokendFieldSym->DiscTemp!=NULL)
				{
					addNode=m_CodeGen.BinOp(ADD,addNode,m_CodeGen.TermNode(LOCLD, tokendFieldSym->DiscTemp));
				}
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}
			
			uint32 position = 0;
			bool multiField = false;
			bool allFields = false;

			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mf =  m_MultiFields.begin();
			for(; it != m_Fields.end(); it++, pIt++, mf++)
			{
				if(*it == tokendFieldSym || (*it)->Name == "allfields")
				{
					extract = true;
					position = *pIt;
					multiField = *mf;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}
		
			if(genExtractionCode && extract)
			{
				if(!allFields)
				{
					if(tokendFieldSym->MultiProto)
					{
						/*
						*	proto*.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiProto - Test"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, tokendFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiProto_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiProto_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + tokendFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, tokendFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.UnOp(LOCLD,tokendFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + tokendFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)),tokendFieldSym->FieldCount));
						
						m_CodeGen.LabelStatement(labelFalse);
					}

					else if(multiField)
					{
						/*
						*	proto%n.field*
						*	proto.field*
						*	Extract each instance of a field
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + tokendFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, tokendFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, tokendFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.UnOp(LOCLD,tokendFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), tokendFieldSym->FieldCount));
						m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + tokendFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, tokendFieldSym->FieldCount), m_CodeGen.TermNode(PUSH,position)));		
					}

					else
					{
						/*
						*	proto.field
						*	proto%n.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*/
						m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, tokendFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "field_extraction_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("HeaderIndex - extract ") + tokendFieldSym->Name);
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokendFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH,position)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.UnOp(LOCLD,tokendFieldSym->LenTemp), m_CodeGen.TermNode(PUSH,position+2)));
						m_CodeGen.CommentStatement(string("Header Indexing - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), tokendFieldSym->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}
				}

				else
				{
					allFieldsFound = true;
				
					//we have to obtain the symbol for allfields
					SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
					nbASSERT(symAllFields!=NULL,"There is an error");
				
					MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,tokendFieldSym->ID), positionId));
					MIRNode *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokendFieldSym->IndexTemp), positionOff));
					MIRNode *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokendFieldSym->LenTemp), positionLen));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),tokendFieldSym->Protocol->position));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),tokendFieldSym->Protocol->NExFields));
				}
			}
			m_CodeGen.LabelStatement(len_false);
			// do nothing
	break;
	}


	case PDL_FIELD_TOKWRAP:
		{	
			nbASSERT(false,"tokenwrapped fields are not supported");

			SymbolFieldTokWrap *tokwrapFieldSym = (SymbolFieldTokWrap*)sym;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(tokwrapFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			tokwrapFieldSym->IndexTemp = indexTemp;
			SymbolTemp *lenTemp;
			MIRNode* offsetNode=offset;
			MIRNode* lenRegEx;
			MIRNode* begin=offset;
			MIRNode *len=m_CodeGen.TermNode(PUSH,(uint32)0);
			MIRNode* new_begin;
			
			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));


			if (tokwrapFieldSym->LenTemp==NULL)
			{
				lenTemp = m_CodeGen.NewTemp(tokwrapFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
				tokwrapFieldSym->LenTemp = lenTemp;
			}

			if(tokwrapFieldSym->BeginTok!=NULL)
			{
				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->BeginTok));

				SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
				nbASSERT(tokwrapFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
				m_GlobalSymbols.StoreRegExEntryUseful(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.UnOp(LOCLD,tokwrapFieldSym->IndexTemp), 
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL),m_CodeGen.UnOp(LOCLD,tokwrapFieldSym->IndexTemp)), 
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result

				MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				MIRNode *leftExpr = validNode;
				MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
					SymbolLabel *labelBeginTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *labelBeginFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JCondStatement(labelBeginTrue, labelBeginFalse, m_CodeGen.BinOp(JCMPEQ, m_CodeGen.UnOp(LOCLD,tokwrapFieldSym->IndexTemp),m_CodeGen.BinOp(SUB,offsetNode,lenRegEx)));
					m_CodeGen.LabelStatement(labelBeginFalse);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.UnOp(LOCLD,tokwrapFieldSym->IndexTemp), indexTemp));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
						break;
					m_CodeGen.LabelStatement(labelBeginTrue);
						//$begin = $offsetNode - $lenRegEx
						begin=m_CodeGen.BinOp(SUB,offsetNode,lenRegEx);

			}
			else if(tokwrapFieldSym->BeginRegEx!=NULL)
			{

				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->BeginRegEx));

				SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
				nbASSERT(tokwrapFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
				m_GlobalSymbols.StoreRegExEntryUseful(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(static_cast<HIRNode*>(regExp->Offset)) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				MIRNode *leftExpr = validNode;
				MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
					//$begin = $offsetNode - $lenRegEx
					begin=m_CodeGen.BinOp(SUB,offsetNode,lenRegEx);
			}
			else
				break;


			if(tokwrapFieldSym->EndTok!=NULL)
			{

				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->EndTok));

				SymbolRegExPDL *regExp = new SymbolRegExPDL(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
				nbASSERT(tokwrapFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
				m_GlobalSymbols.StoreRegExEntryUseful(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(static_cast<HIRNode*>(regExp->Offset)) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				MIRNode *leftExpr = validNode;
				MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					len=m_CodeGen.BinOp(SUB,offsetNode,begin);
			}
			else if(tokwrapFieldSym->EndRegEx!=NULL)
			{

				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->EndRegEx));

				SymbolRegExPDL *regExp = new SymbolRegExPDL(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
				nbASSERT(tokwrapFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
				m_GlobalSymbols.StoreRegExEntryUseful(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(static_cast<HIRNode*>(regExp->Offset)),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(static_cast<HIRNode*>(regExp->Offset)) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				MIRNode *leftExpr = validNode;
				MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					len=m_CodeGen.BinOp(SUB,offsetNode,begin);
			}
			else
				break;


			if(tokwrapFieldSym->BeginOff!=NULL)
			{
				new_begin = TranslateTree(static_cast<HIRNode*>( tokwrapFieldSym->BeginOff ));
				begin=m_CodeGen.BinOp(ADD,offset,new_begin);
			}
			//$fld_index = $begin
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, begin, indexTemp));

			if(tokwrapFieldSym->EndOff!=NULL)
			{
				MIRNode *offExpr = TranslateTree( static_cast<HIRNode*>(tokwrapFieldSym->EndOff ));
				len =m_CodeGen.BinOp(SUB,m_CodeGen.BinOp(ADD,offExpr,offset),begin);
			}
			//$fld_len = $len
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));

			if(tokwrapFieldSym->EndDiscard!=NULL)
			{
				if (tokwrapFieldSym->DiscTemp==NULL)
				{
				SymbolTemp *discTemp = m_CodeGen.NewTemp(tokwrapFieldSym->Name + string("_disc"), m_CompUnit.NumLocals);
				tokwrapFieldSym->DiscTemp = discTemp;
				}
			MIRNode *discExpr = TranslateTree(static_cast<HIRNode*>(tokwrapFieldSym->EndDiscard));
			//$fld_disc = discExpr
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, discExpr, tokwrapFieldSym->DiscTemp));
			}

			if (sym->DependsOn==NULL)
			{	MIRNode *addNode;

				//$add = $begin + $len
				if(tokwrapFieldSym->LenTemp!=NULL)
				{
				addNode = m_CodeGen.BinOp(ADD, begin, len);
				}
				//$add = $add +$discard
     			if(tokwrapFieldSym->DiscTemp!=NULL)
				{

				addNode=m_CodeGen.BinOp(ADD,addNode,m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->DiscTemp));
				}

				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}
			
			uint32 position = 0;
			bool multiField = false;
			bool allFields = false;

			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mf =  m_MultiFields.begin();
			for(; it != m_Fields.end(); it++, pIt++, mf++)
			{
				if(*it == tokwrapFieldSym || (*it)->Name == "allfields")
				{
					extract = true;
					position = *pIt;
					multiField = *mf;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}
		
			if(genExtractionCode && extract)
			{
				if(!allFields) 
				{
					if(tokwrapFieldSym->MultiProto)
					{
						/*
						*	proto*.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiProto - Test"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + tokwrapFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + tokwrapFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)),tokwrapFieldSym->FieldCount));
						
						m_CodeGen.LabelStatement(labelFalse);
					}

					else if(multiField)
					{
						/*
						*	proto%n.field*
						*	proto.field*
						*	Extract each instance of a field
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + tokwrapFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), tokwrapFieldSym->FieldCount));
						m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + tokwrapFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, position)));			
					}

					else
					{
						/*
						*	proto.field
						*	proto%n.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*/
						m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("HeaderIndex - extract ") + tokwrapFieldSym->Name);
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH,position)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, position+2)));
						m_CodeGen.CommentStatement(string("Header Indexing - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), tokwrapFieldSym->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}
				}

				else
				{
					allFieldsFound = true;
				
					//we have to obtain the symbol for allfields
					SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
					nbASSERT(symAllFields!=NULL,"There is an error");
								
					MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,tokwrapFieldSym->ID), positionId));
					MIRNode *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->IndexTemp), positionOff));
					MIRNode *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->LenTemp), positionLen));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),tokwrapFieldSym->Protocol->position));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),tokwrapFieldSym->Protocol->NExFields));
				}
			}
	break;

		};


		case PDL_FIELD_LINE:
			{
			SymbolFieldLine *lineFieldSym = static_cast<SymbolFieldLine*>(sym);
			
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(lineFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			lineFieldSym->IndexTemp = indexTemp;
			SymbolTemp *lenTemp;

			lenTemp = m_CodeGen.NewTemp(lineFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
			lineFieldSym->LenTemp = lenTemp;

			
			MIRNode *len = m_CodeGen.TermNode(PUSH, (uint32) 0);

			//$fld_index = $currentoffset 
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));

			SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)lineFieldSym->EndTok));
			SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
			
			nbASSERT(lineFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");

			m_GlobalSymbols.StoreRegExEntryUseful(regExp);
			

			//set id
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_PATTERN_ID));

			// set buffer offset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
			m_CodeGen.UnOp(LOCLD,lineFieldSym->IndexTemp), 
			new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_OFFSET));

			// calculate and set buffer length 
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, 
			m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL),
			m_CodeGen.UnOp(LOCLD,lineFieldSym->IndexTemp)),
  			new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_LENGTH));

			// find the pattern
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

			// check the result
			MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

			MIRNode *leftExpr = validNode;
			MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
			SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
			SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));
			SymbolLabel *len_false = m_CodeGen.NewLabel(LBL_CODE, "if_false");

			// if there aren't any matches the field doesn't exist, don't update the offset
			m_CodeGen.LabelStatement(labelFalse);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
				m_CodeGen.JumpStatement(JUMPW, len_false);
			m_CodeGen.LabelStatement(labelTrue);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
				len = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));

			//len_false cuold be put here?

			if (sym->DependsOn==NULL)
			{	
				//$add = $currentoffset + $len
				MIRNode *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, lineFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}
			
			uint32 position = 0;
			bool multiField = false;
			bool allFields = false;

			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mf =  m_MultiFields.begin();
			for(; it != m_Fields.end(); it++, pIt++)
			{
				if(*it == lineFieldSym || (*it)->Name == "allfields")
				{
					extract = true;
					position = *pIt;
					multiField = *mf;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}

			if(genExtractionCode && extract)
			{
				if(!allFields)
				{
					if(lineFieldSym->MultiProto)
					{
						/*
						*	proto*.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiProto - Test"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, lineFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiProto_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiProto_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + lineFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, lineFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + lineFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)),lineFieldSym->FieldCount));
						
						m_CodeGen.LabelStatement(labelFalse);
					}

					else if(multiField)
					{
						/*
						*	proto%n.field*
						*	proto.field*
						*	Extract each instance of a field
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + lineFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, lineFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, lineFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), lineFieldSym->FieldCount));
						m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + lineFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, lineFieldSym->FieldCount), m_CodeGen.TermNode(PUSH,position)));			
					}

					else
					{
						/*
						*	proto.field
						*	proto%n.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*/
						m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, lineFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("Field extraciton - extract ") + lineFieldSym->Name);
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH,position)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR,m_CodeGen.TermNode(LOCLD,lineFieldSym->LenTemp), m_CodeGen.TermNode(PUSH,position+2)));
						m_CodeGen.CommentStatement(string("Field extraction - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), lineFieldSym->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}
				}
				else
				{
					allFieldsFound = true;
				
					// we have to obtain the symbol for allfields
					SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
					nbASSERT(symAllFields!=NULL,"There is an error");
				
				
					MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,lineFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,lineFieldSym->ID), positionId));
					MIRNode *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->IndexTemp), positionOff));
					MIRNode *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->LenTemp), positionLen));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),lineFieldSym->Protocol->position));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),lineFieldSym->Protocol->NExFields));
					
				}
			}
				
			m_CodeGen.LabelStatement(len_false);
			break;

			}
			
		case PDL_FIELD_PATTERN:
		{
			SymbolFieldPattern *patternFieldSym = static_cast<SymbolFieldPattern*>(sym);
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(patternFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			patternFieldSym->IndexTemp = indexTemp;
			SymbolTemp *lenTemp;

			lenTemp = m_CodeGen.NewTemp(patternFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
			patternFieldSym->LenTemp = lenTemp;

			MIRNode *ind = m_CodeGen.TermNode(PUSH, (uint32) 0);
			MIRNode *len = m_CodeGen.TermNode(PUSH, (uint32) 0);

			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));

			SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)patternFieldSym->Pattern));

			SymbolRegExPDL *regExp = new SymbolRegExPDL(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCountUseful());
			nbASSERT(patternFieldSym->Protocol!=NULL,"There is a bug! A field must be inside a protocol O_o");
			m_GlobalSymbols.StoreRegExEntryUseful(regExp);

			//set id
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.TermNode(PUSH, regExp->Id),
					new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_PATTERN_ID));

			// set buffer offset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
			m_CodeGen.UnOp(LOCLD,patternFieldSym->IndexTemp),
			new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_OFFSET));

			// calculate and set buffer length
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
			m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), m_CodeGen.UnOp(LOCLD,patternFieldSym->IndexTemp)),
			new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_LENGTH));

			// find the pattern
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));


			// check the result
			MIRNode *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

			MIRNode *leftExpr = validNode;
			MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
			SymbolLabel *len_false = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
			SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));
		
			// if there aren't any matches the field doesn't exist, don't update the offset
			m_CodeGen.LabelStatement(labelFalse);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
				m_CodeGen.JumpStatement(JUMPW, len_false);

			m_CodeGen.LabelStatement(labelTrue);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
				
				/*
				*	Get the offset of the field
				*/
				ind = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")),REGEX_IN_OFFSET_FOUND);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SUB,ind,m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND /*REGEX_IN_OFFSET_FOUND*/)),m_CodeGen.TermNode(LOCLD, indexTemp)), indexTemp));
			
				/*
				*	Get the length of the field
				*/
				len = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND /*REGEX_IN_OFFSET_FOUND*/);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));

			if (sym->DependsOn==NULL)
			{
				//$add = $currentoffset + $len
				MIRNode *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, patternFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			uint32 position = 0;
			bool multiField = false;
			bool allFields = false;

			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mf =  m_MultiFields.begin();
			for(; it != m_Fields.end(); it++, pIt++)
			{
				if(*it == patternFieldSym || (*it)->Name == "allfields")
				{
					extract = true;
					position = *pIt;
					multiField = *mf;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}
			
			if(genExtractionCode && extract)
			{
				if(!allFields)
				{
					if(patternFieldSym->MultiProto)
					{
						/*
						*	proto*.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiProto - Test"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, patternFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + patternFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, patternFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + patternFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)),patternFieldSym->FieldCount));
						
						m_CodeGen.LabelStatement(labelFalse);
					}

					else if(multiField)
					{
						/*
						*	proto%n.field*
						*	proto.field*
						*	Extract each instance of a field
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + patternFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, patternFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, patternFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), patternFieldSym->FieldCount));
						m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + patternFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, patternFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, position)));	
					}

					else
					{
						/*
						*	proto.field
						*	proto%n.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*/
						m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, patternFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("Field extraction - extract ") + patternFieldSym->Name);
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH,position)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->LenTemp), m_CodeGen.TermNode(PUSH,position+2)));
						m_CodeGen.CommentStatement(string("Field extraction - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), patternFieldSym->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}
				}

				else
				{
					allFieldsFound = true;
				
					//we have to obtain the symbol for allfields
					SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
					nbASSERT(symAllFields!=NULL,"There is an error");
				
			
					MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,patternFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,patternFieldSym->ID), positionId));
					MIRNode *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->IndexTemp), positionOff));
					MIRNode *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->LenTemp), positionLen));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),patternFieldSym->Protocol->position));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),patternFieldSym->Protocol->NExFields));
					
				}
			
			}
		
			// if there aren't any matches the field doesn't exist, don't update the offset
			m_CodeGen.LabelStatement(len_false);
			break;

			}
		case PDL_FIELD_EATALL:
		{
			SymbolFieldEatAll *eatallFieldSym = static_cast<SymbolFieldEatAll*>(sym);
			MIRNode *currOffsNode = m_CodeGen.TermNode(LOCLD, currOffsTemp);
			SymbolTemp *lenTemp = NULL;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(eatallFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			eatallFieldSym->IndexTemp = indexTemp;

			lenTemp = m_CodeGen.NewTemp(eatallFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
			eatallFieldSym->LenTemp = lenTemp;

			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));	
	
			if(eatallFieldSym->DependsOn==NULL)
			{
				//$fld_len = ($packet_end-$currentoffset)
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(SUB, m_CodeGen.TermNode(PBL), currOffsNode), lenTemp));
			}
			else
			{
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(SUB,  currOffsNode,m_CodeGen.TermNode(LOCLD,indexTemp)), lenTemp));
			}



			if (eatallFieldSym->DependsOn==NULL)
			{
				//$add = $currentoffset + $len
				MIRNode *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, eatallFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			uint32 position = 0;
			bool multiField = false;
			bool allFields = false;
	
			bool extract = false;
			list<SymbolField*>::iterator it =  m_Fields.begin();
			list<uint32>::iterator pIt =  m_Positions.begin();
			list<bool>::iterator mf =  m_MultiFields.begin();
			for(; it != m_Fields.end(); it++, pIt++)
			{
				if(*it == eatallFieldSym || (*it)->Name == "allfields") 
				{
					extract = true;
					position = *pIt;
					multiField = *mf;
					allFields = (*it)->Name == "allfields";
					break;
				}
			}
			
			if(genExtractionCode && extract)
			{
				if(!allFields)
				{
					if(eatallFieldSym->MultiProto)
					{
						/*
						*	proto*.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiProto - Test"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, eatallFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("MultiProto - Calculate the offset into the Info-Partition for the field and extract ") + eatallFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, eatallFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiProto - Store the number of extracted fields ") + eatallFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, m_Protocol->ExtractG), m_CodeGen.TermNode(PUSH, 1)), m_CodeGen.TermNode(PUSH, position)));
						m_CodeGen.CommentStatement(string("MultiProto - Increase field counter"));						
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)),eatallFieldSym->FieldCount));
						
						m_CodeGen.LabelStatement(labelFalse);
					}

					else if(multiField)
					{
						/*
						*	proto%n.field*
						*	proto.field*
						*	Extract each instance of a field
						*	The offset into the info partition must be calculated
						*/
						m_CodeGen.CommentStatement(string("MultiField - Calculate the offset into the Info-Partition for the field and extract ") + eatallFieldSym->Name);
						MIRNode *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, eatallFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, eatallFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
						m_CodeGen.CommentStatement(string("MultiField - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), eatallFieldSym->FieldCount));
						m_CodeGen.CommentStatement(string("MultiField - Store the number of extracted fields ") + eatallFieldSym->Name + " into the info partition");
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, eatallFieldSym->FieldCount), m_CodeGen.TermNode(PUSH,position)));			
					}

					else
					{
						/*
						*	proto.field
						*	proto%n.field
						*	Extract the first instance of field
						*	The variable field_counter must be tested and then, if it is 0, the extraction is performed
						*/
						m_CodeGen.CommentStatement(string("Field extraction - Test on field instance"));
						MIRNode *leftExpr = m_CodeGen.TermNode(LOCLD, eatallFieldSym->FieldCount);
						MIRNode *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "Field_extraction_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("Field extraction - extract ") + eatallFieldSym->Name);
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH,position)));
						m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->LenTemp), m_CodeGen.TermNode(PUSH,position+2)));
						m_CodeGen.CommentStatement(string("Field extraction - Increase field counter"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), eatallFieldSym->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}
				}

				else
				{
					allFieldsFound = true;
				
					//we have to obtain the symbol for allfields
					SymbolField *symAllFields=this->m_GlobalSymbols.LookUpProtoFieldByName(this->m_Protocol, "allfields");
					nbASSERT(symAllFields!=NULL,"There is an error");
								
					MIRNode *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,eatallFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,eatallFieldSym->ID), positionId));
					MIRNode *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->IndexTemp), positionOff));
					MIRNode *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->LenTemp), positionLen));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),eatallFieldSym->Protocol->position));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),eatallFieldSym->Protocol->NExFields));
				}
			}

		}break;

	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}

void IRLowering::TranslateVarDeclInt(HIRNode *node)
{
	nbASSERT(node->Op == IR_DEFVARI, "node must be an IR_DEFVARI");
	HIRNode *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");

	HIRNode *rightChild = node->GetRightChild();
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	nbASSERT(varSym->VarType == PDL_RT_VAR_INTEGER, "left child symbol must be an integer variable");

	SymbolVarInt *intVar = (SymbolVarInt*)varSym;
	
	SymbolTemp *temp = m_CodeGen.NewTemp(varSym->Name, m_CompUnit.NumLocals);
	intVar->Temp = temp;
	MIRNode *initializer(0);
	if (intVar->Name.compare("$pbl") == 0)		
		initializer = m_CodeGen.TermNode(PBL);
/*	if (intVar->Name.compare("$packetlength") == 0) //FIXME: Ivano - is it right?		
		initializer = m_CodeGen.TermNode(PBL);*/
	else
		initializer = TranslateTree(rightChild);
	nbASSERT(initializer != NULL, "initializer cannot be NULL");
	//$var = init_expr
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, initializer, temp));

}

void IRLowering::TranslateVarDeclStr(HIRNode *node)
{
	nbASSERT(node->Op == IR_DEFVARS, "node must be an IR_DEFVARS");
	HIRNode *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");


	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	switch (varSym->VarType)
	{
	case PDL_RT_VAR_REFERENCE:
		//do nothing
		break;
	case PDL_RT_VAR_BUFFER:
		//!\todo support buffer variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	case PDL_RT_VAR_PROTO:
		//!\todo support proto variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}

SymbolTemp *IRLowering::AssociateVarTemp(SymbolVarInt *var)
{
	//if the var has not already an associated temporary we create it
	//such case is when a variable has been generated by the compiler
	if(var->Temp == NULL)
	{
		SymbolTemp *temp = m_CodeGen.NewTemp(var->Name, m_CompUnit.NumLocals);
		var->Temp = temp;
	}
	return var->Temp;
}

void IRLowering::TranslateAssignInt(HIRNode *node)
{
	nbASSERT(node->Op == IR_ASGNI, "node must be an IR_ASGNI");
	HIRNode *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");

	HIRNode *rightChild = node->GetRightChild();
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	nbASSERT(varSym->VarType == PDL_RT_VAR_INTEGER, "left child symbol must be an integer variable");
	SymbolVarInt *intVar = (SymbolVarInt*)varSym;

	if (rightChild->Op==IR_SVAR && rightChild->Sym!=NULL && rightChild->Sym->SymKind==SYM_RT_LOOKUP_ITEM)
	{
		SymbolLookupTableItem *item = (SymbolLookupTableItem *)rightChild->Sym;
		SymbolLookupTable *table = item->Table;

		if (item->Size <= 4)
		{
			// get the entry flag
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, table->Id),
				table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			uint32 itemOffset = table->GetValueOffset(item->Name);
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, itemOffset),
				table->Label,
				LOOKUP_EX_OUT_VALUE_OFFSET));

			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_GET_VALUE));

			MIRNode *itemNode = m_CodeGen.TermNode(COPIN, table->Label, LOOKUP_EX_IN_VALUE);
		
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, itemNode, AssociateVarTemp(intVar)));
		}
		else
		{
			this->GenerateWarning(string("A lookup table item should be assigned to an integer variable, but the size is > 4."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
			m_CodeGen.CommentStatement(string("ERROR: A lookup table item should be assigned to an integer variable, but the size is > 4."));
		}
	}
	else
	{
		//$var = init_expr
		m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, TranslateTree(rightChild), AssociateVarTemp(intVar)));
	}
}

/*
//[icerrato] never used
void IRLowering::TranslateAssignRef(SymbolVarBufRef *refVar, Node *right)
{
	nbASSERT(false, "CANNOT BE HERE");
#if 0
	if (refVar->IndexTemp == NULL)
	{
		nbASSERT(refVar->LenTemp == NULL, "index temp is NULL, lenTemp should be NULL");
		refVar->IndexTemp = m_CodeGen.NewTemp(refVar->Name + string("_ind"), m_CompUnit.NumLocals);
		refVar->LenTemp = m_CodeGen.NewTemp(refVar->Name + string("_len"), m_CompUnit.NumLocals);
	}

	//!\todo manage packet reference
#endif
}*/

void IRLowering::TranslateAssignStr(HIRNode *node)
{
	nbASSERT(node->Op == IR_ASGNS, "node must be an IR_ASGNS");
	HIRNode *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");

	HIRNode *rightChild = node->GetRightChild();
	nbASSERT(rightChild != NULL, "right child cannot be NULL");
	nbASSERT(rightChild->IsString(), "right child should be of type string");
	nbASSERT(rightChild->Op == IR_FIELD || rightChild->Op == IR_SVAR, "right child should be a string");
	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	
	
	switch (varSym->VarType)
	{

		case PDL_RT_VAR_REFERENCE:
		{
			SymbolVarBufRef *ref=(SymbolVarBufRef *)varSym;
			SymbolField *referee=(SymbolField *)node->Sym;
			
			switch (referee->FieldType)
			{
			case PDL_FIELD_FIXED:
				{
					SymbolFieldFixed *fixedField=(SymbolFieldFixed *)referee;

					if (fixedField->UsedAsInt)
					{
						if (ref->IntCompatible)
						{
							// the reference is used as integer, preload the value

							if (ref->ValueTemp == NULL)
								ref->ValueTemp=m_CodeGen.NewTemp(ref->Name + string("_ref_temp"), m_CompUnit.NumLocals);

							m_CodeGen.CommentStatement(ref->Name + string(" = ") + fixedField->Name + string(" (preload of ") + ref->ValueTemp->Name + string(")"));


							//[icerrato]
							CodeList *hcode = m_GlobalSymbols.NewCodeList(true);
							HIRCodeGen hircodegen(m_GlobalSymbols, hcode);

							MIRNode *cint=TranslateFieldToInt(hircodegen.TermNode(IR_FIELD, fixedField), NULL, fixedField->Size);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, cint, ref->ValueTemp));
						}
						else
						{
							// the reference is used as integer, but is not compatible
							// (e.g. there's an assignment of a non-int field)
							this->GenerateWarning(string("The variable ")+ref->Name+string(" is used as integer, and it must be compatible to integer;")+
								string(" assignment of ")+fixedField->Name+string(" is not legal"), __FILE__, __FUNCTION__, __LINE__, 1, 3);
							m_CodeGen.CommentStatement(string("ERROR: The variable ")+ref->Name+string(" is used as integer, and it must be compatible to integer;")+
								string(" assignment of ")+fixedField->Name+string(" is not legal"));
						}
					}

					if (fixedField->UsedAsArray || fixedField->UsedAsString)
					{
						SymbolTemp *indexTemp = ref->IndexTemp;
						if (indexTemp == 0)
							indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

						SymbolTemp *lenTemp = ref->LenTemp;
						if (lenTemp == 0)
							lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

						m_CodeGen.CommentStatement(ref->Name + string(" = ") + fixedField->Name + string(" (") + indexTemp->Name + string(")"));

						//$fld_index = $referee_index
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, fixedField->IndexTemp), indexTemp));

						//$fld_len = var_size
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(PUSH, fixedField->Size), lenTemp));
					}

					if (!fixedField->UsedAsInt && !fixedField->UsedAsArray && !fixedField->UsedAsString)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" is not used, assignment will be omitted"), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						m_CodeGen.CommentStatement(string("INFO: The variable ")+ref->Name+string(" is not used, assignment will be omitted"));
					}

				}break;


			case PDL_FIELD_VARLEN:
				{
					SymbolFieldVarLen *varLenField=(SymbolFieldVarLen *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);


					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + varLenField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, varLenField->IndexTemp), indexTemp));
					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, static_cast<MIRNode*>(varLenField->LenExpr), lenTemp));
				}break;

			case PDL_FIELD_TOKEND:
				{
					SymbolFieldTokEnd *tokEndField=(SymbolFieldTokEnd *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + tokEndField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, tokEndField->IndexTemp), indexTemp));

					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.TermNode(LOCLD, tokEndField->LenTemp), lenTemp));

				}break;

			case PDL_FIELD_TOKWRAP:
				{
					SymbolFieldTokWrap *tokWrapField=(SymbolFieldTokWrap *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);


					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);


					m_CodeGen.CommentStatement(ref->Name + string(" = ") + tokWrapField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, tokWrapField->IndexTemp), indexTemp));

					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, tokWrapField->LenTemp), lenTemp));

				}break;


			case PDL_FIELD_LINE:
				{
					SymbolFieldLine *lineField=(SymbolFieldLine *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + lineField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, lineField->IndexTemp), indexTemp));

					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.TermNode(LOCLD, lineField->LenTemp), lenTemp));

				}break;

			case PDL_FIELD_PATTERN:
				{
					SymbolFieldPattern *patternField=(SymbolFieldPattern *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + patternField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, patternField->IndexTemp), indexTemp));
					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD,patternField->LenTemp), lenTemp));
				}break;

			case PDL_FIELD_EATALL:
				{
					SymbolFieldEatAll *eatallField=(SymbolFieldEatAll *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + eatallField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, eatallField->IndexTemp), indexTemp));
					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD,eatallField->LenTemp), lenTemp));
				}break;

			default:
				nbASSERT(false, "A reference variable should refer to fixed , variable length, tokenended, tokenwrapped, line, pattern or eatall field");
			}
		}

		//!\todo support reference variables
		//TranslateAssignRef(((SymbolVarBufRef*)varSym)->Referee, rightChild);
		break;
	case PDL_RT_VAR_BUFFER:
		//!\todo support buffer variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	case PDL_RT_VAR_PROTO:
		//!\todo support proto variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}


MIRNode *IRLowering::TranslateIntVarToInt(HIRNode *node)
{

	nbASSERT(node->Op == IR_IVAR, "node must be an IR_IVAR");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind = SYM_RT_VAR, "contained symbol must be an RT_VAR");
	
	if(((SymbolVariable*)node->Sym)->VarType == PDL_RT_VAR_INTEGER)
	{
		SymbolVarInt *intVar = static_cast<SymbolVarInt*>(node->Sym);
		
		if ((intVar->Name.compare("$pbl") == 0) ||(intVar->Name.compare("$framelength") == 0))
			return m_CodeGen.TermNode(PBL);
		else
			return m_CodeGen.TermNode(LOCLD, intVar->Temp);
	}
	else
	{
		//we are loading an integer variable not defined into the netpdl
		node->Sym->SymKind=SYM_TEMP;
		//FIXME: sta cosa è una porcata.. dovrebbe già essere un SYM_TEMP, ma non so dove si perde
		return m_CodeGen.TermNode(LOCLD, (SymbolTemp*)node->Sym);
	}
}

MIRNode *IRLowering::TranslateStrVarToInt(HIRNode *node, MIRNode* offset, uint32 size)
{
	//!\todo Implement lowering of CINT(SVAR)
	nbASSERT(node->Op == IR_SVAR, "node must be an IR_VAR");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");

	if (node->Sym->SymKind==SYM_RT_VAR)
	{
		SymbolVariable *varSym = (SymbolVariable*)node->Sym;

		switch(varSym->VarType)
		{
		case PDL_RT_VAR_REFERENCE:
			{
				SymbolVarBufRef *ref = (SymbolVarBufRef*)varSym;

				if (ref->RefType == REF_IS_PACKET_VAR) {
					nbASSERT(offset != NULL, "offsNode should be != NULL");
					return GenMemLoad(offset, size); 
				} else if (ref->RefType == REF_IS_REF_TO_FIELD) {
					if (ref->IntCompatible==false)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" is not compatible to integer, it cannot be converted;")+
							string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						return m_CodeGen.TermNode(PUSH, (uint32) -1);
					}
					if (ref->UsedAsString)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" is used as string in the NetPDL database, it cannot be converted;")+
							string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						return m_CodeGen.TermNode(PUSH, (uint32)-1);
					}
					if (ref->ValueTemp == NULL)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" has not been preloaded, and it cannot be used;")+
							string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						return m_CodeGen.TermNode(PUSH, (uint32)-1);
					}

					return m_CodeGen.TermNode(LOCLD, ref->ValueTemp);
				}


			}
			break;

		case PDL_RT_VAR_BUFFER:
			nbASSERT(false, "TO BE IMPLEMENTED");
			return NULL;
			break;

		default:
			nbASSERT(false, "CANNOT BE HERE");
			return NULL;
		}
	} else if (node->Sym->SymKind==SYM_RT_LOOKUP_ITEM)
	{
		SymbolLookupTableItem *item = (SymbolLookupTableItem *)node->Sym;
		SymbolLookupTable *table = item->Table;

		if (item->Size <= 4)
		{
			// get the entry flag
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, table->Id),
				table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			uint32 itemOffset = table->GetValueOffset(item->Name);
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, itemOffset),
				table->Label,
				LOOKUP_EX_OUT_VALUE_OFFSET));

			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_GET_VALUE));

			MIRNode *itemNode = m_CodeGen.TermNode(COPIN, table->Label, LOOKUP_EX_IN_VALUE);

			return itemNode;
		}
		else
		{
			this->GenerateWarning(string("A lookup table item should be assigned to an integer variable, but the size is > 4."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
			m_CodeGen.CommentStatement(string("ERROR: A lookup table item should be assigned to an integer variable, but the size is > 4."));
		}
	} else
	{
		nbASSERT(false, "CANNOT BE HERE");
	}
	//!\todo maybe we should manage this somewhere else!!!

	return NULL;
}


MIRNode *IRLowering::TranslateFieldToInt(HIRNode *node, MIRNode* offset, uint32 size)
{
	nbASSERT(node->Op == IR_FIELD, "node must be an IR_FIELD");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind == SYM_FIELD, "contained symbol should be a field");

	SymbolField *fieldSym = (SymbolField*)node->Sym;

	// [ds] we will use the symbol that defines different version of the same field
	SymbolField *sym = this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, fieldSym);


	if (sym->IntCompatible==false && offset==NULL && size==0)
	{
		this->GenerateWarning(string("The field ")+sym->Name+string(" is not compatible to integer, it cannot be converted;")+
			string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return m_CodeGen.TermNode(PUSH, (uint32)-1);
	}

	MIRNode *memOffs(0);
	switch(fieldSym->FieldType)
	{
	case PDL_FIELD_FIXED:
		{
			SymbolFieldFixed *fixedField = (SymbolFieldFixed*)sym;

			if (fixedField->ValueTemp != NULL)
			{
				return m_CodeGen.TermNode(LOCLD, fixedField->ValueTemp);
			}
			else if (offset != NULL)
			{
				memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedField->IndexTemp), offset);
			}
			else
			{
				memOffs = m_CodeGen.TermNode(LOCLD, fixedField->IndexTemp);
				size = fixedField->Size;
			}
			return GenMemLoad(memOffs, size);
		}break;
	case PDL_FIELD_VARLEN:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because var-len fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldVarLen *varlenField = (SymbolFieldVarLen*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;
	case PDL_FIELD_BITFIELD:
		{
			SymbolFieldBitField *bitField = (SymbolFieldBitField*)sym;
			nbASSERT(bitField->DependsOn != NULL, "depends-on pointer should be != NULL");

			uint32 mask=bitField->Mask;
			SymbolFieldContainer *container=(SymbolFieldContainer *)bitField->DependsOn;


			// search the fixed field root to get the size
			while (container->FieldType!=PDL_FIELD_FIXED)
			{
				nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

				// and the mask with the parent mask
				mask=mask & ((SymbolFieldBitField *)container)->Mask;

				container=((SymbolFieldBitField *)container)->DependsOn;
			}

			SymbolFieldContainer *protoContainer=(SymbolFieldContainer *)this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, container);
			SymbolFieldFixed *fixed=(SymbolFieldFixed *)protoContainer;

			nbASSERT((fixed->Size == 1)||(fixed->Size == 2)||(fixed->Size == 4), "size can be only 1, 2, or 4!");
			nbASSERT(fixed->IndexTemp != NULL, string("the container ").append(fixed->Name).append(" of the field '").append(fieldSym->Protocol->Name).append(".").append(fieldSym->Name).append("' was not defined").c_str());

			// after applying the bit mask, you should also shift the field to align the actual value
			uint8 shift=0;
			uint32 tMask=mask;
			while ((tMask & 1)==0)
			{
				shift++;
				tMask=tMask>>1;
			}

			memOffs = m_CodeGen.TermNode(LOCLD, fixed->IndexTemp);
			MIRNode *memLoad = GenMemLoad(memOffs, fixed->Size); 
			MIRNode *field = m_CodeGen.BinOp(AND, memLoad, m_CodeGen.TermNode(PUSH, mask));
			if (shift>0)
				return m_CodeGen.BinOp(SHR, field, m_CodeGen.TermNode(PUSH, shift));
			else
				return field;
		}break;

	case PDL_FIELD_TOKEND:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because tok-end fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldTokEnd *tokendField = (SymbolFieldTokEnd*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_TOKWRAP:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because tok-wrap fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldTokWrap *tokwrapField = (SymbolFieldTokWrap*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;


	case PDL_FIELD_LINE:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because tok-end fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldLine *lineField = (SymbolFieldLine*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_PATTERN:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because pattern fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldPattern *patternField = (SymbolFieldPattern*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_EATALL:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because eatall fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldEatAll *eatallField = (SymbolFieldEatAll*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_PADDING:
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	nbASSERT(false, "CANNOT BE HERE");
	return NULL;
}

MIRNode *IRLowering::GenMemLoad(MIRNode *offsNode, uint32 size)
{

	MIROpcodes memOp;
	switch(size)
	{
	case 1:
		memOp = PBLDU; //pload.8
		break;
	case 2:
		memOp = PSLDU;
		break;
	case 3:
	case 4:
		memOp = PILD;
		break;
	default:
		this->GenerateWarning(string("Size should be 1, 2, 3 or 4 bytes"), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		break;
	}

	MIRNode *memLoadNode = m_CodeGen.UnOp(memOp,offsNode);
	
	if (size == 3)
		return m_CodeGen.BinOp(AND, memLoadNode, m_CodeGen.TermNode(PUSH, 0xFFF0));

	return memLoadNode;
}

MIRNode *IRLowering::TranslateCInt(HIRNode *node)
{
	HIRNode *child = node->GetLeftChild();
	nbASSERT(child != NULL, "left child cannot be NULL");

	if (child->Op == IR_CHGBORD)
	{
		// change byte order
		HIRNode *node = child->GetLeftChild();

		switch (node->Op)
		{
		case IR_FIELD:
			{   SymbolField* field = (SymbolField*)node->Sym;
				switch(field->FieldType)
				{
				case PDL_FIELD_FIXED:
					{
						SymbolFieldFixed *fixed =(SymbolFieldFixed*)field;
						switch(fixed->Size)
						{
						case 1:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, m_CodeGen.TermNode(LOCLD,fixed->ValueTemp), m_CodeGen.TermNode(PUSH, 24)));
							}break;
						case 2:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, m_CodeGen.TermNode(LOCLD,fixed->ValueTemp), m_CodeGen.TermNode(PUSH, 16)));
							}break;
						case 3:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, m_CodeGen.TermNode(LOCLD,fixed->ValueTemp), m_CodeGen.TermNode(PUSH, 8)));
							}break;
						case 4:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.TermNode(LOCLD, fixed->ValueTemp));
							}break;
						default:
							nbASSERT(false, "changebyteorder can accept only members with size le than 4");
							break;
						}
					}break;
				default:
					nbASSERT(false, "changebyteorder can accept only members with constant size");
					break;
				}
			}break;
		case IR_SVAR:
			{
				MIRNode *toInvert = TranslateTree(node);
				switch (node->Sym->SymKind)
				{
				case SYM_RT_VAR:
					{
						SymbolVarBufRef *ref = (SymbolVarBufRef*)node->Sym;
						HIRNode *sizeNode = node->GetRightChild();
						nbASSERT(sizeNode->Op==IR_ICONST, "changebyteorder can accept only members with constant size");
						uint32 size = ((SymbolIntConst *)sizeNode->Sym)->Value;


							if (ref->RefType == REF_IS_PACKET_VAR) {
								switch(size)
								{
								case 1:
									{
									return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, toInvert, m_CodeGen.TermNode(PUSH, 24)));
									}break;
								case 2:
									{
									return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, toInvert, m_CodeGen.TermNode(PUSH, 24)));
									}break;
								case 3:
									{
									return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, toInvert, m_CodeGen.TermNode(PUSH, 8)));
									}break;
								case 4:
									{
									return m_CodeGen.UnOp(IESWAP,toInvert);
									}break;
								default:
									nbASSERT(false, "changebyteorder can accept only members with size le than 4");
									break;

								}
							}

					}break;
				case SYM_RT_LOOKUP_ITEM:
					{
					}break;
				default:
					nbASSERT(false, "String variable unknown");
					break;
				}
			}break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			break;
		}
	}
	else
	{
		// other nodes...
		nbASSERT(child->Sym != NULL, "child symbol cannot be NULL");

		uint32 size(0);
		HIRNode *offsNode = child->GetLeftChild();
		MIRNode *MIRoffsNode = NULL;

		if (offsNode != NULL)
		{
			MIRoffsNode = TranslateTree(offsNode);
			HIRNode *lenNode = child->GetRightChild();
			if (lenNode->Op != IR_ICONST)
			{
				this->GenerateWarning(string("A buf2int expression must be used with an ICONST value."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
				return NULL;
			}
			size = ((SymbolIntConst*)lenNode->Sym)->Value;
		}

		switch (child->Op)
		{
		case IR_IVAR:
			return TranslateIntVarToInt(child);
			break;
		case IR_SVAR:
			return TranslateStrVarToInt(child, MIRoffsNode, size);
			break;
		case IR_FIELD:
			return TranslateFieldToInt(child, MIRoffsNode, size);
			break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			return NULL;
			break;
		}
	}
	return NULL;
}

MIRNode *IRLowering::TranslateConstInt(HIRNode *node)
{
	nbASSERT(node->Op == IR_ICONST || node->Op == IR_BCONST, "node must be an IR_ICONST or and IR_BCONST");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind = SYM_INT_CONST, "contained symbol must be an INT_CONST");
	SymbolIntConst *intConst = (SymbolIntConst*)node->Sym;

	return m_CodeGen.TermNode(PUSH, intConst->Value);

}

//[icerrato] never used
//Node *IRLowering::TranslateConstStr(Node *node)
//{
	/*nbASSERT(node->Op == IR_SCONST, "node must be an IR_SCONST");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind = SYM_STR_CONST, "contained symbol must be an STR_CONST");
	SymbolStrConst *strConst = (SymbolStrConst*)node->Sym;

	return m_CodeGen.UnOp(strConst->MemOffset, strConst->Size);*/

//	return NULL;
//}

MIRNode *IRLowering::TranslateConstBool(HIRNode *node)
{
	//at the moment bool constants are handled like integer variables
	//indeed they can only be generated by constant folding...
	return TranslateConstInt(node);

}

MIRNode *IRLowering::TranslateArithBinOp(MIROpcodes op, HIRNode *node)
{
	nbASSERT(node->IsBinOp(), "node must be a binary operator");
	nbASSERT(node->IsInteger(), "node must be an integer operator");

	HIRNode *leftChild = node->GetLeftChild();
	HIRNode *rightChild = node->GetRightChild();

	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	//nbASSERT(leftChild->IsInteger(), "left child must be integer");
	//nbASSERT(rightChild->IsInteger(), "right child must be integer");
	MIRNode *left = TranslateTree(leftChild);
	MIRNode *right = TranslateTree(rightChild);
	return m_CodeGen.BinOp(op, left, right);
}

MIRNode *IRLowering::TranslateArithUnOp(MIROpcodes op, HIRNode *node)
{
	nbASSERT(node->IsUnOp(), "node must be a unary operator");
	nbASSERT(node->IsInteger(), "node must be an integer operator");

	HIRNode *leftChild = node->GetLeftChild();
	HIRNode *rightChild = node->GetRightChild();

	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(rightChild == NULL, "right child should be NULL");

	nbASSERT(leftChild->IsInteger(), "left child must be integer");

	return m_CodeGen.UnOp(op, TranslateTree(leftChild));
}

uint32 log2(uint32 X)
{
	return (uint32)(log((double)X)/log((double)2));
}

MIRNode *IRLowering::TranslateDiv(HIRNode *node)
{
	nbASSERT(node->IsBinOp(), "node must be a binary operator");
	nbASSERT(node->IsInteger(), "node must be an integer operator");

	HIRNode *leftChild = node->GetLeftChild();
	HIRNode *rightChild = node->GetRightChild();

	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	nbASSERT(leftChild->IsInteger(), "left child must be integer");
	nbASSERT(rightChild->IsInteger(), "right child must be integer");
	nbASSERT(rightChild->Sym->SymKind == SYM_INT_CONST, "right operand symbol should be an integer constant");
	nbASSERT((((SymbolIntConst*)rightChild->Sym)->Value % 2) == 0, "right operand should be a power of 2")
		uint32 shiftVal = log2(((SymbolIntConst*)rightChild->Sym)->Value);
	return m_CodeGen.BinOp(SHR, TranslateTree(leftChild), m_CodeGen.TermNode(PUSH, shiftVal));
}

MIRNode *IRLowering::TranslateTree(HIRNode *node) 
{
	nbASSERT(node!=NULL,"node cannot be NULL");

	switch(node->Op)
	{

	case IR_IVAR:
		return TranslateIntVarToInt(node);
		break;
		/*these should be handled by TranslateRelOpStr
		case IR_SVAR:
		break;
		case IR_FIELD:
		break;
		case IR_SCONST:
		return TranslateConstStr(node);
		break;
		*/
	case IR_ITEMP:
		//we should not find a Temp inside the High Level IR
		nbASSERT(false, "CANNOT BE HERE");
		return NULL;
		break;
	case IR_ICONST:
		return TranslateConstInt(node);
		break;
	case IR_BCONST:
		return TranslateConstBool(node);
		break;
	case IR_CINT:
		return TranslateCInt(node);
		break;
	case IR_ADDI:
		return TranslateArithBinOp(ADD, node);
		break;
	case IR_SUBI:
		return TranslateArithBinOp(SUB, node);
		break;
	case IR_DIVI:
		return TranslateDiv(node);
		break;
	case IR_MULI:
		return TranslateArithBinOp(IMUL, node);
		break;
	case IR_NEGI:
		return TranslateArithUnOp(NEG, node);
		break;
	case IR_SHLI:
		return TranslateArithBinOp(SHL, node);
		break;
	case IR_MODI:
		return TranslateArithBinOp(MOD, node);
		break;
	case IR_SHRI:
		return TranslateArithBinOp(SHR, node);
		break;
	case IR_XORI:
		return TranslateArithBinOp(XOR, node);
		break;
	case IR_ANDI:
		return TranslateArithBinOp(AND, node);
		break;
	case IR_ORI:
		return TranslateArithBinOp(OR, node);
		break;
	case IR_NOTI:
		return TranslateArithUnOp(NOT, node);
		break;
	case IR_SCONST:
		{
			SymbolTemp *offset=m_CodeGen.NewTemp(((SymbolStrConst*)node->Sym)->Name + string("_offs"), m_CompUnit.NumLocals);
			MIRNode *offsetNode=m_CodeGen.TermNode(PUSH, ((SymbolStrConst*)node->Sym)->MemOffset);
			m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, offset, offsetNode));
			return m_CodeGen.TermNode(LOCLD,
				offset,
				m_CodeGen.TermNode(PUSH, ((SymbolStrConst*)node->Sym)->Size));
		}break;
	case IR_FIELD:
		this->GenerateWarning("Support for string comparisons is not yet implemented.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
		m_CodeGen.CommentStatement("ERROR: Support for string comparisons is not yet implemented.");
		return NULL;
		break;
	case IR_SVAR:
		{
			nbASSERT(node->Sym != NULL, "node symbol cannot be NULL");

			uint32 size(0);
			HIRNode *offsNode = node->GetLeftChild();
			MIRNode *MIRoffsNode = NULL;

			if (offsNode != NULL)
			{
				MIRoffsNode = TranslateTree(offsNode);
				HIRNode *lenNode = node->GetRightChild();
				nbASSERT(lenNode->Op == IR_ICONST, "lenNode should be an IR_ICONST");
				size = ((SymbolIntConst*)lenNode->Sym)->Value;
			}
			
			if(translForRegExp==OFFSET)
				return MIRoffsNode;

			if(translForRegExp==LENGTH){ 
				translForRegExp=NOTHING;
				return m_CodeGen.TermNode(PUSH,size);
			}

			switch (node->Sym->SymKind)
			{
			case SYM_RT_VAR:
				{	
					SymbolVarBufRef *ref = (SymbolVarBufRef*)node->Sym;

					if (ref->RefType == REF_IS_PACKET_VAR) {
						nbASSERT(MIRoffsNode != NULL, "offsNode should be != NULL");
						return GenMemLoad(MIRoffsNode, size);
					} else if (ref->RefType == REF_IS_REF_TO_FIELD) {
						SymbolTemp *indexTemp = ref->IndexTemp;
						if (indexTemp == 0)
							indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

						SymbolTemp *lenTemp = ref->LenTemp;
						if (lenTemp == 0)
							lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

						return m_CodeGen.TermNode(LOCLD, ref->IndexTemp);
					}
				}break;
			case SYM_RT_LOOKUP_ITEM:
				{
					SymbolLookupTableItem *item = (SymbolLookupTableItem *)node->Sym;
					SymbolLookupTable *table = item->Table;

					if (item->Size <= 4)
					{
						// get the entry flag
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
							m_CodeGen.TermNode(PUSH, table->Id),
							table->Label,
							LOOKUP_EX_OUT_TABLE_ID));

						uint32 itemOffset = table->GetValueOffset(item->Name);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
							m_CodeGen.TermNode(PUSH, itemOffset),
							table->Label,
							LOOKUP_EX_OUT_VALUE_OFFSET));

						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_GET_VALUE));
						return m_CodeGen.TermNode(COPIN, table->Label, LOOKUP_EX_IN_VALUE);
					}
					else
					{
						this->GenerateWarning(string("A lookup table item should be assigned to an integer variable, but the size is > 4."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
						m_CodeGen.CommentStatement(string("ERROR: A lookup table item should be assigned to an integer variable, but the size is > 4."));
					}
				}break;
			default:
				nbASSERT(false, "String variable unknown");
				break;
			}
		}break;

	case IR_LKSEL:
		{
			//SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)node->Sym;
			//nbASSERT(node->GetLeftChild() != NULL, "Lookup select instruction should specify a keys list");
			//SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)node->GetLeftChild()->Sym;
			//TranslateLookupSelect(entry, keys);
		}break;

	case IR_LKHIT:
		//TranslateRelOpLookup(IR_LKHIT, expr, jcInfo);
		break;

	case IR_REGEXFND:
		break;
	
	case IR_STRINGMATCHINGFND:
		break;
		
	case IR_REGEXXTR:
		break;

		/*
		The following cases are managed by TranslateBoolExpr()

		case IR_ANDB:

		case IR_ORB:

		case IR_NOTB:

		case IR_EQI:

		case IR_GEI:

		case IR_GTI:

		case IR_LEI:

		case IR_LTI:

		case IR_NEI:

		case IR_EQS:

		case IR_NES:
		*/
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	return NULL;
}

void IRLowering::TranslateStatement(StmtBase *stmt)
{
	m_TreeDepth = 0;
	switch(stmt->Kind)
	{
	case STMT_LABEL:
		TranslateLabel((StmtLabel*)stmt);
		break;

	case STMT_GEN:
		TranslateGen((StmtGen*)stmt);
		break;

	case STMT_JUMP:
		TranslateJump((StmtJump*)stmt);
		break;

	case STMT_SWITCH:
		TranslateSwitch((HIRStmtSwitch*)stmt);
		break;
	case STMT_COMMENT:
		m_CodeGen.CommentStatement(stmt->Comment);
		break;
	case STMT_IF:
		TranslateIf((StmtIf*)stmt);
		break;
	case STMT_LOOP:
		TranslateLoop((StmtLoop*)stmt);
		break;
	case STMT_WHILE:
		TranslateWhileDo((StmtWhile*)stmt);
		break;
	case STMT_DO_WHILE:
		TranslateDoWhile((StmtWhile*)stmt);
		break;
	case STMT_BREAK:
		TranslateBreak((StmtCtrl*)stmt);
		break;
	case STMT_CONTINUE:
		TranslateContinue((StmtCtrl*)stmt);
		break;
	case STMT_FINFOST:
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}


void IRLowering::GenerateInfo(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_GlobalSymbols.GetGlobalInfo().Debugging.DebugLevel >= requiredDebugLevel)
	{
		if (m_Protocol!=NULL)
			message=string("(")+this->m_Protocol->Name+string(") ").append(message);
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_INFO, file, function, line, indentation);
	}
}


void IRLowering::GenerateWarning(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_GlobalSymbols.GetGlobalInfo().Debugging.DebugLevel >= requiredDebugLevel)
	{
		if (m_Protocol!=NULL)

			message=string("(")+this->m_Protocol->Name+string(") ").append(message);
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_WARNING, file, function, line, indentation);
	}
}


void IRLowering::GenerateError(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_GlobalSymbols.GetGlobalInfo().Debugging.DebugLevel >= requiredDebugLevel)
	{
		if (m_Protocol!=NULL)
			message=string("(") + this->m_Protocol->Name+string(") ").append(message);
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_ERROR, file, function, line, indentation);
	}
}
