/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include "defs.h"
#include "pdlparser.h"
#include "utils.h"
#include "tree.h"
#include "typecheck.h"
#include "../nbee/globals/debug.h"

//Uncomment the next row to debug fields
//#define ENABLE_FIELD_DEBUG


using namespace std;

#define VERIFY_ONLY_WITH_PAYLOAD
#define EXEC_BEFORE_ONLY_WITH_PAYLOAD

PDLParser::PDLParser(_nbNetPDLDatabase &protoDB, GlobalSymbols &globalSymbols, ErrorRecorder &errRecorder)
	:m_ProtoDB(&protoDB), m_GlobalSymbols(globalSymbols), m_ErrorRecorder(errRecorder),
	m_CodeGen(0), m_CurrProtoSym(0), m_LoopCount(0), m_IfCount(0), m_SwCount(0),
	m_IsEncapsulation(0), m_Preferred(0), m_ParentUnionField(0), m_BitFldCount(0), m_ScopeLevel(0),
	m_FilterInfo(0), m_CandidateProtoToResolve(0)
{
	m_GlobalInfo = &globalSymbols.GetGlobalInfo();
	m_ProtoGraph = &m_GlobalInfo->GetProtoGraph();
	m_ProtoGraphPreferred = &m_GlobalInfo->GetProtoGraphPreferred();
	m_CompilerConsts = &m_GlobalInfo->GetCompilerConsts();

}


/*****************************************************************************/
/*   Code manipulation                                                       */
/*****************************************************************************/
void PDLParser::ChangeCodeGen(HIRCodeGen &codeGen)
{
	m_CodeGenStack.push_back(m_CodeGen);
	m_CodeGen = &codeGen;
}

void PDLParser::RestoreCodeGen(void)
{
	nbASSERT(!m_CodeGenStack.empty(), "Temporary CodeGen should not be NULL");
	m_CodeGen = m_CodeGenStack.back();
	m_CodeGenStack.pop_back();
}

void PDLParser::GenProtoEntryCode(SymbolProto &protoSymbol)
{	
	if (m_GlobalSymbols.GetProtoStartLbl(&protoSymbol) == NULL)
		m_GlobalSymbols.SetProtoStartLbl(&protoSymbol, m_CodeGen->NewLabel(LBL_LINKED, protoSymbol.Name));

	//	protoSymbol.ExitProtoLbl = m_CodeGen->NewLabel(LBL_LINKED, string(protoSymbol.Name) + string("_EXIT"));
	//	protoSymbol.TrueShtcutLbl = m_CodeGen->NewLabel(LBL_LINKED, string(protoSymbol.Name) + string("_TRUE"));
	//	protoSymbol.FalseShtcutLbl = m_CodeGen->NewLabel(LBL_LINKED, string(protoSymbol.Name) + string("_FALSE"));

	/* generate the label corresponding to the first instruction for the current protocol*/
	// this->m_CodeGen->CommentStatement(string(""));
	// m_CodeGen->LabelStatement(m_GlobalSymbols.GetProtoStartLbl(&protoSymbol));
	this->m_CodeGen->CommentStatement(string("PROTOCOL ").append(protoSymbol.Name).append(": FORMAT"));
	//entryLabel->Comment = "--- PROTOCOL " + protoSymbol.Name + " ---";

	//for the start_proto we do not generate a start_offset variable
	if ((&protoSymbol) == m_GlobalInfo->GetStartProtoSym())
		return;

	/* create the symbol for a new variable holding the offset of the start of the current header */
	SymbolVarInt *protoOffsVar = m_CodeGen->NewIntVar(protoSymbol.Name + string("_offs"));

	SymbolVarInt *currOffsVar= m_GlobalSymbols.CurrentOffsSym;
	/* generate the assignment statement: $proto_offset = $current_offset */
	m_CodeGen->GenStatement(m_CodeGen->BinOp(IR_ASGNI, m_CodeGen->TermNode(IR_IVAR, protoOffsVar), m_CodeGen->TermNode(IR_IVAR, currOffsVar)));

	/* save the newly created symbol into the current protocol symbol for later use */
	m_GlobalSymbols.SetProtoOffsVar(&protoSymbol, protoOffsVar);
}


/*****************************************************************************/
/*    Symbols and fields management                                          */
/*****************************************************************************/
SymbolField *PDLParser::LookUpFieldSym(const string name)
{
	SymbolField *field=m_GlobalSymbols.LookUpProtoFieldByName(m_CurrProtoSym, name);

	return field;
}

SymbolField *PDLParser::LookUpFieldSym(const SymbolField *field)
{
	//lookup in the general fields symbol table
	SymbolField *sym = m_GlobalSymbols.LookUpProtoField(m_CurrProtoSym, field);
	if (sym == NULL)
	{
		this->GenerateWarning(string("Field '") + field->Name + string("' not found in protocol '") + m_CurrProtoSym->Name + string("'"), __FILE__, __FUNCTION__, __LINE__, 1, 6);
	}

	return sym;

	////recursive lookup in the nested scopes symbol tables
	//FieldsList_t *scopeFieldsList = LookUpFieldInScope(name);

	//if (scopeFieldsList == NULL)
	//{
	//	this->GenerateWarning(string("Field '") + name + string("' not found in the current scope and in the scope hierarchy"), __FILE__, __FUNCTION__, __LINE__, 1, 6);
	//	return NULL;
	//}
	//SymbolField *fieldSym = scopeFieldsList->back();

	//return fieldSym;
}

SymbolField *PDLParser::StoreFieldSym(SymbolField *fieldSymbol)
{
	return this->StoreFieldSym(fieldSymbol, false);
}

SymbolField *PDLParser::StoreFieldSym(SymbolField *fieldSymbol, bool force)
{
	SymbolField *storedSym = m_GlobalSymbols.StoreProtoField(m_CurrProtoSym, fieldSymbol, force);

	if (fieldSymbol!=storedSym)
	{
		this->GenerateInfo(string("A field with the same name ('")+storedSym->Name+string("') has been defined more than once. Conflicts could be generated."),
			__FILE__, __FUNCTION__, __LINE__, 2, 5);
	}

	return storedSym;
	//StoreFieldInScope(name, fieldSymbol);
}

//FieldsList_t *PDLParser::LookUpFieldInScope(const string name)
//{
//	FieldsList_t *fieldsList(0);
//	nbASSERT(m_FieldScopes, "Scopes stack not initialized");
//	nbASSERT(!m_FieldScopes->empty(), "Scopes stack empty!!!");
//	nbASSERT(name.length() != 0, "empty name");
//
//	FldScopeStack_t::iterator i = m_FieldScopes->end();
//	uint32 level = 0;
//	while (i != m_FieldScopes->begin())
//	{
//		FieldSymbolTable_t *fldSymTab = *(i-1);
//
//		nbASSERT(fldSymTab != NULL, "fields symbol table is null");
//
//		//DumpFieldSymbols(stdout, *fldSymTab, level);
//
//		if (fldSymTab->LookUp(name, fieldsList))
//			return fieldsList;
//		i--;
//		level++;
//	}
//
//	return NULL;
//}

//void PDLParser::StoreFieldInScope(const string name, SymbolField *fieldSymbol)
//{
//	nbASSERT(m_FieldScopes, "Scopes stack not initialized");
//	nbASSERT(!m_FieldScopes->empty(), "Scopes stack empty!!!");
//
//	FieldsList_t *fieldsList(0);
//	FieldSymbolTable_t *fldSymTab = m_FieldScopes->back();
//
//	if (!fldSymTab->LookUp(name, fieldsList))
//	{
//		fieldsList = new FieldsList_t();
//		if (fieldsList == NULL)
//			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
//		fldSymTab->LookUp(name, fieldsList, ENTRY_ADD);
//	}
//
//	fieldsList->push_back(fieldSymbol);
//}

//void PDLParser::EnterFldScope(void)
//{
//	if (m_IsEncapsulation)
//		return;
//
//	//printf("Entering Scope Level %u\n", ++m_ScopeLevel);
//	nbASSERT(m_FieldScopes, "Scopes stack not initialized");
//	FieldSymbolTable_t *fldSymTab = new FieldSymbolTable_t();
//	if (fldSymTab == NULL)
//		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
//
//	m_FieldScopes->push_back(fldSymTab);
//}
//
//void PDLParser::ExitFldScope(void)
//{
//	if (m_IsEncapsulation)
//		return;
//	//printf("Exiting Scope Level %u\n", m_ScopeLevel--);
//	nbASSERT(m_FieldScopes, "Scopes stack not initialized");
//	FieldSymbolTable_t *fldSymTab = m_FieldScopes->back();
//	delete fldSymTab;
//	m_FieldScopes->pop_back();
//}


/*****************************************************************************/
/*    Parsing                                                                */
/*****************************************************************************/

/*****************************************************************************/
/**** Parsing elements     ***************************************************/
void PDLParser::ParseElements(_nbNetPDLElementBase *firstElement)
{
	_nbNetPDLElementBase *element = firstElement;

	while(element)
	{
		ParseElement(element);
		element = nbNETPDL_GET_ELEMENT(m_ProtoDB, element->NextSibling);
	}
}

void PDLParser::ParseElement(_nbNetPDLElementBase *element)
{
	this->GenerateInfo(string("Parsing element '").append(element->ElementName).append("'"), __FILE__, __FUNCTION__, __LINE__, 3, element->NestingLevel);

	if (!CheckAllowedElement(element))
		return;

	switch(element->Type)
	{
	case nbNETPDL_IDEL_FIELD:	
		{
			ParseFieldDef((_nbNetPDLElementFieldBase*) element);
			SymbolField *field=this->LookUpFieldSym(((_nbNetPDLElementFieldBase*)element)->Name);

			if (field==NULL || !field->IsDefined || !field->IsSupported)
				this->m_InnerElementsNotDefined=true;
		}break;

	case nbNETPDL_IDEL_VARIABLE:
		ParseRTVarDecl((_nbNetPDLElementVariable*)element);
		break;

	case nbNETPDL_IDEL_ASSIGNVARIABLE:
		ParseAssign((_nbNetPDLElementAssignVariable*)element);
		break;

	case nbNETPDL_IDEL_SWITCH:
	{
		ParseSwitch((_nbNetPDLElementSwitch*)element);
		}break;

	case nbNETPDL_IDEL_LOOP:
	{
		ParseLoop((_nbNetPDLElementLoop*)element);
		}break;

	case nbNETPDL_IDEL_LOOPCTRL:
		{
		ParseLoopCtrl((_nbNetPDLElementLoopCtrl*)element);
		}break;

	case nbNETPDL_IDEL_IF:
		{
		ParseIf((_nbNetPDLElementIf*)element);
		}break;

	case nbNETPDL_IDEL_INCLUDEBLK:
		{
			_nbNetPDLElementIncludeBlk *includeBlkElem = (_nbNetPDLElementIncludeBlk*)element;
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, includeBlkElem->IncludedBlock->FirstChild));
			this->m_ParentUnionField=NULL;
		}
		break;

	case nbNETPDL_IDEL_BLOCK:
		{
			_nbNetPDLElementBlock *blockElem = (_nbNetPDLElementBlock*)element;
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, blockElem->FirstChild));
			this->m_ParentUnionField=NULL;
		}
		break;

	case nbNETPDL_IDEL_NEXTPROTO:
		ParseNextProto((_nbNetPDLElementNextProto*)element);
		break;

	case nbNETPDL_IDEL_NEXTPROTOCANDIDATE:
		ParseNextProtoCandidate((_nbNetPDLElementNextProto*)element);
		break;

	case nbNETPDL_IDEL_ALIAS:
		// aliases are translated in the XML parsing, don't parse them here
		//this->GenerateWarning(string("Aliases are not yet supported."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		break;

	case nbNETPDL_IDEL_LOOKUPTABLE:

		if(m_CurrProtoSym->NoSession==false)// || m_CurrProtoSym->Name!="tcp")	
		{
				ParseLookupTable((_nbNetPDLElementLookupTable *)element);
		}
		
		break;

	case nbNETPDL_IDEL_UPDATELOOKUPTABLE:
	{
/*	
		_nbNetPDLElementUpdateLookupTable *NetPDLElement = (_nbNetPDLElementUpdateLookupTable*)element;	
	   if(NetPDLElement->Action == nbNETPDL_UPDATELOOKUPTABLE_ACTION_ADD)
		{
		cout << "Protocol: "<<m_CurrProtoSym->Name<< " session? " <<m_CurrProtoSym->NoSession;
		cout << " Table: "<< NetPDLElement->TableName;
		cout << " Key: "<< NetPDLElement->FirstKey->ExprString <<endl;
		}
*/
		if(m_CurrProtoSym->NoSession==false)
				ParseLookupTableUpdate((_nbNetPDLElementUpdateLookupTable *)element);

	}break;

	case nbNETPDL_IDEL_ASSIGNLOOKUPTABLE:
		//[icerrato]
		//ParseLookupTableAssign((_nbNetPDLElementAssignLookupTable *)element);
		nbASSERT(false,"Element <assign-lookuptable> is not supported yet");
		break;

	default:
		this->GenerateWarning(string("Element '").append(
			nbNETPDL_GET_ELEMENT(m_ProtoDB, element->Parent)->ElementName).append(
			"/").append(string(element->ElementName)).append(
			string("' not supported")),
			__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
		return;
		break;
	}
}


/*****************************************************************************/
/**** Parsing expressions   **************************************************/
HIRNode *PDLParser::ParseExpressionInt(_nbNetPDLExprBase *expr)
{
	HIRNode *subExpr1(0), *subExpr2(0);

	nbASSERT(expr != NULL, "expression cannot be NULL");

	if (expr->ReturnType != nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
	{
		this->GenerateWarning(string("The expression '").append(string(expr->Token)).append(string("' does not return a number, and is not supported")),
			__FILE__, __FUNCTION__, __LINE__, 1, 5);
		return NULL;
	}

	// If this is not a complex expression, let's return the result right now
	if (expr->Type != nbNETPDL_ID_EXPR_OPERAND_EXPR)
		return ParseOperandInt(expr);

	// Let's convert this pointer into an expression
	_nbNetPDLExpression *Expression= (struct _nbNetPDLExpression*) expr;

	nbASSERT(Expression->Operator != NULL, "NULL Operator in expression");
	nbASSERT(Expression->Operand2, "NULL Operand in expression");

	// the second operand could be a number or not
	if (Expression->Operand2->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
	{
		// Parse the second operand
		subExpr2 = ParseOperandInt(Expression->Operand2);
		if (subExpr2==NULL)
		{
			this->GenerateWarning(string("Operand '").append(string(Expression->Operand2->Token)).append("' cannot be evaluated"),
				__FILE__, __FUNCTION__, __LINE__, 1, 5);
			return NULL;
		}

		if (Expression->Operator->OperatorType != nbNETPDL_ID_EXPR_OPER_NOT &&
			Expression->Operator->OperatorType != nbNETPDL_ID_EXPR_OPER_BITWNOT)
		{
			// let's parse binary expressions
			// the second operand is a number, the first operand should be a number
			nbASSERT(Expression->Operand1->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER, "invalid operand type");
			subExpr1 = ParseOperandInt(Expression->Operand1);

			if (subExpr1==NULL)
			{
				this->GenerateWarning(string("Operand '").append(string(Expression->Operand1->Token)).append("' cannot be evaluated"),
					__FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}

			// generate integer/boolean expression with integer/boolean operands
			switch (Expression->Operator->OperatorType)
			{
			case nbNETPDL_ID_EXPR_OPER_PLUS:
				return m_CodeGen->BinOp(IR_ADDI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_MINUS:
				return m_CodeGen->BinOp(IR_SUBI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_MUL:
				return m_CodeGen->BinOp(IR_MULI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_DIV:
				//!\todo convert these assertions to a compilation error
				nbASSERT(subExpr2->IsConst(), "right operand should be a constant");
				nbASSERT(subExpr2->Sym != NULL, "right operand symbol should not be null");
				nbASSERT(subExpr2->Sym->SymKind == SYM_INT_CONST, "right operand symbol should be an integer constant");
				nbASSERT((((SymbolIntConst*)subExpr2->Sym)->Value % 2) == 0, "right operand should be a power of 2")
					if (((SymbolIntConst*)subExpr2->Sym)->Value == 0)
					{
						this->GenerateWarning(string("Division by 0 in expression '").append(expr->Token).append("'"),
							__FILE__, __FUNCTION__, __LINE__, 1, 5);
						return NULL;
					}
					return m_CodeGen->BinOp(IR_DIVI, subExpr1, subExpr2);
					break;

			case nbNETPDL_ID_EXPR_OPER_MOD:
				return m_CodeGen->BinOp(IR_MODI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_BITWAND:
				return m_CodeGen->BinOp(IR_ANDI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_BITWOR:
				return m_CodeGen->BinOp(IR_ORI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_GREATEQUAL:
				return m_CodeGen->BinOp(IR_GEI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_GREAT:
				return m_CodeGen->BinOp(IR_GTI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_LESSEQUAL:
				return m_CodeGen->BinOp(IR_LEI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_LESS:
				return m_CodeGen->BinOp(IR_LTI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_AND:
				return m_CodeGen->BinOp(IR_ANDB, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_OR:
				return m_CodeGen->BinOp(IR_ORB, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_EQUAL:
				return m_CodeGen->BinOp(IR_EQI, subExpr1, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_NOTEQUAL:
				return m_CodeGen->BinOp(IR_NEI, subExpr1, subExpr2);
				break;
			default:
				nbASSERT(false, "CANNOT BE HERE");
				break;
			}

		}
		else
		{
			// let's parse unary expressions
			switch (Expression->Operator->OperatorType)
			{
			case nbNETPDL_ID_EXPR_OPER_NOT:
				return m_CodeGen->UnOp(IR_NOTB, subExpr2);
				break;

			case nbNETPDL_ID_EXPR_OPER_BITWNOT:
				return m_CodeGen->UnOp(IR_NOTI,subExpr2);
				break;

			default:
				nbASSERT(false, "CANNOT BE HERE");

				break;
			}
		}
	}
	else
	{
		// the second operand is a buffer (or you don't have to mind), the first operand should be a buffer
		
		if (Expression->Operand1==NULL)
		{
			this->GenerateWarning(string("Operand '").append(string(Expression->Operand1->Token)).append("' is not defined"),
				__FILE__, __FUNCTION__, __LINE__, 1, 5);
			return NULL;
		}
		if (Expression->Operand2==NULL)
		{
			this->GenerateWarning(string("Operand '").append(string(Expression->Operand1->Token)).append("' is not defined"),
				__FILE__, __FUNCTION__, __LINE__, 1, 5);
			return NULL;
		}
	
		if((Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_EQUAL)||(Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_NOTEQUAL))
		{
		
			//Translate this operation in a string matching one	
	
			nbASSERT(Expression->Operand2->Type==nbNETPDL_ID_EXPR_OPERAND_STRING,"the second operand of an \"==\" must be a constant string");
			_nbNetPDLExprString* operand = (_nbNetPDLExprString*)Expression->Operand2;
			
			SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen->ConstStrSymbol(string((const char*)operand->Value)); 
			HIRNode *buffer = ParseExpressionStr(Expression->Operand1);
			if (buffer==NULL)
			{
				this->GenerateWarning(string("Operand '").append(string(Expression->Operand1->Token)).append("' cannot be evaluated"),
					__FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}
			
			SymbolRegExPDL *regExpSym = new SymbolRegExPDL(buffer,pattern,true,m_GlobalSymbols.GetStringMatchingEntriesCount());  //case sensitive
			m_GlobalSymbols.StoreStringMatchingEntry(regExpSym,m_CurrProtoSym);
			
			HIRNode *copro = m_CodeGen->TermNode(IR_STRINGMATCHINGFND, regExpSym);
			return 	(Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_EQUAL) ? copro : m_CodeGen->UnOp(IR_NOTB,copro);
		}
		else if((Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_GREAT)||(Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_LESS))
		{
			subExpr2 = ParseOperandStr(Expression->Operand2);
			if (subExpr2==NULL)
			{
				this->GenerateWarning(string("Operand '").append(string(Expression->Operand2->Token)).append("' cannot be evaluated"),
					__FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}

			subExpr1 = ParseOperandStr(Expression->Operand1);
			if (subExpr1==NULL)
			{
				this->GenerateWarning(string("Operand '").append(string(Expression->Operand1->Token)).append("' cannot be evaluated"),
					__FILE__, __FUNCTION__, __LINE__, 1, 5);

				return NULL;
			}

			// generate integer/boolean expression with integer/boolean operands
			switch (Expression->Operator->OperatorType)
			{
				case nbNETPDL_ID_EXPR_OPER_GREAT:
					return m_CodeGen->BinOp(IR_GTS, subExpr1, subExpr2);
					break;

				case nbNETPDL_ID_EXPR_OPER_LESS:
					return m_CodeGen->BinOp(IR_LTS, subExpr1, subExpr2);
					break;
					
				default:
					nbASSERT(false,"cannot be here.. there is a bug :-(");
			}
		}
		else
			nbASSERT(false,"cannot be here.. there is a bug :-(");
	}


	return NULL;
}

HIRNode *PDLParser::ParseExpressionStr(_nbNetPDLExprBase *expr)
{

	if (expr == NULL)
		return NULL;

	if (expr->ReturnType != nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
	{
		this->GenerateWarning(string("The expression '") + string(expr->Token) + string("' does not return a buffer, and is not supported"),
			__FILE__, __FUNCTION__, __LINE__, 1, 5);
		return NULL;
	}

	// If this is not a complex expression, let's return the result right now
	if (expr->Type != nbNETPDL_ID_EXPR_OPERAND_EXPR)
		return ParseOperandStr(expr);

	// cast this pointer to an expression ptr
	_nbNetPDLExpression *Expression= (struct _nbNetPDLExpression*) expr;

	nbASSERT(Expression->Operand1 != NULL, "Cannot evaluate expression. No operand found.");

	return ParseOperandStr(Expression->Operand1);
}


/*****************************************************************************/
/**** Parsing variables    ***************************************************/
void PDLParser::ParseRTVarDecl(_nbNetPDLElementVariable *variable)
{
	HIRNode *expression(0);

	if (m_GlobalSymbols.LookUpVariable(variable->Name) != NULL)
	{
		this->GenerateWarning(string("Variable '").append(variable->Name).append("' already declared, it will be updated with the last value"),
			__FILE__, __FUNCTION__, __LINE__, 1, variable->NestingLevel);
	}

	VarValidity validity = VAR_VALID_THISPKT;

	switch(variable->Validity)
	{
		//! Variable has 'static' duration (it lasts forever).
	case nbNETPDL_VARIABLE_VALIDITY_STATIC:
		validity = VAR_VALID_STATIC;
		break;
		//! Variable looses its content after each packet.
	case nbNETPDL_VARIABLE_VALIDITY_THISPACKET:
		validity = VAR_VALID_THISPKT;
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE!!!");
		break;
	}

	SymbolVariable *var(0);
	HIRNode *varNode(0);

	switch(variable->VariableDataType)
	{
		//! Variable is an integer number.
	case nbNETPDL_VARIABLE_TYPE_NUMBER:
		{
			var = m_CodeGen->NewIntVar(variable->Name, validity);
			uint32 initValue(0);
			if (!m_CompilerConsts->LookupConst(variable->Name, initValue))
			{
				initValue = variable->InitValueNumber;
			}
			expression = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(initValue));
			varNode = m_CodeGen->TermNode(IR_IVAR, var);
			m_CodeGen->GenStatement(m_CodeGen->BinOp(IR_DEFVARI, varNode, expression));
		}break;

		//! Variable is a buffer of a given size. Data is *copied* in this buffer (while variables of type nbNETPDL_VARIABLE_REFBUFFER do not copy data).
	case nbNETPDL_VARIABLE_TYPE_BUFFER:
		this->GenerateWarning(string("Variable '").append(string(variable->Name)).append(string("': buffer variable is not supported")),
			__FILE__, __FUNCTION__, __LINE__, 1, variable->NestingLevel);
		var = new SymbolVarBuffer(variable->Name, variable->Size, validity);
		if (var == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		var->IsSupported=false;
#if 0
		string value((const char*)variable->InitValueString, variable->InitValueStringSize);
		expression = m_CodeGen->TermNode(IR_SCONST, m_CodeGen->ConstStrSymbol(value));
#endif
		break;

		//! Variable is a buffer passed by reference (e.g. the packet buffer).
	case nbNETPDL_VARIABLE_TYPE_REFBUFFER:
		{
#ifdef STATISTICS
			this->m_CurrProtoSym->VarDeclared++;
#endif

			ReferenceType refType(REF_IS_UNKNOWN);

			if (string(variable->Name).compare("$packet") == 0)
				//the variable is the $packet one
				refType = REF_IS_PACKET_VAR;

			//!\todo implement reference initialization (if applicable)

			var = new SymbolVarBufRef(variable->Name, validity, refType);
			if (var == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			varNode = m_CodeGen->TermNode(IR_SVAR, var);
			m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_DEFVARS, varNode));
		}break;

		//! Variable is a protocol (or a pointer to a protocol).
	case nbNETPDL_VARIABLE_TYPE_PROTOCOL:
		// we can refer to a protocol by just using its ID (number)
		var = m_CodeGen->NewIntVar(variable->Name, validity);
		expression = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(0));
		varNode = m_CodeGen->TermNode(IR_IVAR, var);
		m_CodeGen->GenStatement(m_CodeGen->BinOp(IR_DEFVARI, varNode, expression));
		break;

	default:
		this->GenerateWarning(string("Variable '").append(string(variable->Name)).append(string("': invalid type")),
			__FILE__, __FUNCTION__, __LINE__, 1, variable->NestingLevel);
		break;
	}

	if (var != NULL)
		m_GlobalSymbols.StoreVarSym(variable->Name, var);

#ifdef STATISTICS
	if (var==NULL || var->IsDefined==false || var->IsSupported==false)
		this->m_CurrProtoSym->VarUnsupported++;
#endif
}


/*****************************************************************************/
/**** Parsing operands     ***************************************************/
HIRNode *PDLParser::ParseOperandStr(struct _nbNetPDLExprBase *expr)
{
	if (expr == NULL)
		return NULL;

	switch (expr->Type)
	{
	case nbNETPDL_ID_EXPR_OPERAND_STRING:		
		{
			_nbNetPDLExprString* operand = (_nbNetPDLExprString*)expr;

			return m_CodeGen->TermNode(IR_SCONST, m_CodeGen->ConstStrSymbol(string((const char*)operand->Value, operand->Size)));
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
		{
			_nbNetPDLExprVariable *operand = (_nbNetPDLExprVariable*)expr;
			SymbolVariable *varSym = (SymbolVariable*)m_GlobalSymbols.LookUpVariable(operand->Name);

			if (varSym == NULL)
			{
				this->GenerateWarning(string("Variable '") + string(operand->Name) + string("' has not been declared"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}

			HIRNode *startAtExp(0), *sizeExp(0);

			switch(varSym->VarType)
			{
			case PDL_RT_VAR_INTEGER:
				this->GenerateWarning(string("Variable '") + string(operand->Name) + string("' is integer, string variable expected"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
				break;
			case PDL_RT_VAR_REFERENCE:
				{
					SymbolVarBufRef *bufRefVar = (SymbolVarBufRef*)varSym;

					// [ds] is this check necessary? in this moment we can't know if a variable has been initialized or not...
					//if (!bufRefVar->IsInited())
					//{
					//	this->GenerateWarning(string("Variable '") + string(operand->Name) + string("' is used without being initialized"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
					//	return NULL;
					//}

					if (string(varSym->Name).compare("$packet") == 0) {
						if (operand->OffsetStartAt && operand->OffsetSize)
						{
							startAtExp = ParseExpressionInt(operand->OffsetStartAt);
							sizeExp = ParseExpressionInt(operand->OffsetSize);
							if (sizeExp->Sym!=NULL && sizeExp->Sym->SymKind==SYM_INT_CONST && ((SymbolIntConst *)sizeExp->Sym)->Value==0)
							{
								if (startAtExp->Sym!=NULL && startAtExp->Sym->SymKind==SYM_RT_VAR)
								{
									return m_CodeGen->TermNode(IR_IVAR, startAtExp->Sym);
								}
								else
								{
									return startAtExp;
								}
							}

							bufRefVar->UsedAsArray=true;
						}
						else if (operand->OffsetStartAt && operand->OffsetSize)
						{
							this->GenerateWarning(string("Variable '") + string(operand->Name) + string("' is used as array, but you should specify both starting offset and size."), __FILE__, __FUNCTION__, __LINE__, 1, 5);
							return NULL;
						}
					} else {
						if (this->m_ConvertingToInt == true) {						
							bufRefVar->UsedAsInt=true;
						} else {
							bufRefVar->UsedAsString=true;
						}
					}

#ifdef STATISTICS
					if (string(varSym->Name).compare("$packet") != 0)
						this->m_CurrProtoSym->VarOccurrences++;

					this->m_CurrProtoSym->VarTotOccurrences++;
#endif
					return m_CodeGen->TermNode(IR_SVAR, bufRefVar, startAtExp, sizeExp);

				}break;
			case PDL_RT_VAR_BUFFER:
				//!\todo support buffer variables
				this->GenerateWarning(string("Variable '") + string(operand->Name) + string("': use of buffer variables is not supported"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
				break;
			case PDL_RT_VAR_PROTO:
				//!\todo support proto variables
				this->GenerateWarning(string("Variable '") + string(operand->Name) + string("': use of protocol variables is not supported"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
				nbASSERT(false, "CANNOT BE HERE");
				break;
			default:
				nbASSERT(false, "CANNOT BE HERE");
				break;
			}

			return m_CodeGen->TermNode(IR_SVAR, varSym, startAtExp, sizeExp);
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD:				
		{
			_nbNetPDLExprFieldRef* operand = (_nbNetPDLExprFieldRef*)expr;

			SymbolField *fieldSym = LookUpFieldSym(operand->FieldName);
					
			if (fieldSym == NULL)
			{
#ifdef STATISTICS
				this->m_CurrProtoSym->UnknownRefFields++;
#endif
				this->GenerateError(string("Operand '").append(operand->FieldName).append("' not defined"),
					__FILE__, __FUNCTION__, __LINE__, 1, 4);
				return NULL;
			}

			if (fieldSym->FieldType == PDL_FIELD_PADDING)
			{
				this->GenerateWarning(string("Operand '") + fieldSym->Name + string("' of padding type is not supported"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}

//THIS FIELD IS BEEN RECOGNIZED AS PART OF EXPRESSION -> SET INFORMATION ABOUT THIS FIELD 
#ifdef ENABLE_FIELD_OPT
			SetFieldInfo(fieldSym);
#endif

			HIRNode *startAtExp(0), *sizeExp(0);

			if (operand->OffsetStartAt && operand->OffsetSize)
			{
				startAtExp = ParseExpressionInt(operand->OffsetStartAt);
//				fieldSym->UsedAsArray=true;

				sizeExp = ParseExpressionInt(operand->OffsetSize);
				fieldSym->UsedAsArray=true;
			}
			else if (operand->OffsetStartAt || operand->OffsetSize)
			{
				this->GenerateWarning(string("Operand used as array should specify both the starting offset and the lenght"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}
			else
			{
				if (this->m_ConvertingToInt == true)
				{
					fieldSym->UsedAsInt=true;
				}else
					fieldSym->UsedAsString=true;
			}
#ifdef OPTIMIZE_SIZED_LOOPS

			if (m_CurrProtoSym->SizedLoopStack.size() > 0)
			{
				StmtBase *loop = m_CurrProtoSym->Field2SizedLoopMap[fieldSym];

				if (loop != NULL)
				{
					bool found=false;
					for (StmtList_t::iterator i=m_CurrProtoSym->SizedLoopStack.begin(); i!=m_CurrProtoSym->SizedLoopStack.end(); i++)
					{
						if ( (*i) == loop )
							found=true;
					}

					if (found==false)
					{
						if (m_CurrProtoSym->FieldReferredInSizedLoopMap[fieldSym] == NULL)
						{
							m_CurrProtoSym->FieldReferredInSizedLoopMap[fieldSym] = (StmtBase*)m_CurrProtoSym->SizedLoopStack.front();
						}
					}
				}
			}
#endif

			return m_CodeGen->TermNode(IR_FIELD, fieldSym, startAtExp, sizeExp);
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:							
		{
			_nbNetPDLExprLookupTable *lookup=(_nbNetPDLExprLookupTable *)expr;

			HIRNode *offsetStart(0);
			HIRNode *offsetSize(0);

			if (lookup->OffsetStartAt!=NULL && lookup->OffsetSize!=NULL)
			{
				ParseExpressionInt(lookup->OffsetStartAt);
				ParseExpressionInt(lookup->OffsetSize);
			}
			return this->ParseLookupTableItem(lookup->TableName, lookup->FieldName, offsetStart, offsetSize);
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_EXPR:
		return ParseExpressionStr(expr);
		break;

	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_EXTRACTSTRING:
		{	
			struct _nbNetPDLExprFunctionRegExp *function = (struct _nbNetPDLExprFunctionRegExp *)expr;
			
			HIRNode *buffer = ParseExpressionStr(function->SearchBuffer);

			int escapes=0;
			int slashes=0;
			int quote=0;
			int length=strlen(function->RegularExpression);
			for (int i=0; i<length; i++)
			{
				if (( (u_char)function->RegularExpression[i]<0x20 || (u_char)function->RegularExpression[i]>0x79 ) &&
					(u_char)function->RegularExpression[i]!='|' &&
					(u_char)function->RegularExpression[i]!='^' &&
					(u_char)function->RegularExpression[i]!='.' &&
					(u_char)function->RegularExpression[i]!='$' &&
					(u_char)function->RegularExpression[i]!='*' &&
					(u_char)function->RegularExpression[i]!='(' && (u_char)function->RegularExpression[i]!=')' &&
					(u_char)function->RegularExpression[i]!='[' && (u_char)function->RegularExpression[i]!=']' &&
					(u_char)function->RegularExpression[i]!='{' && (u_char)function->RegularExpression[i]!='}')
				{
					escapes++;
				} else if (function->RegularExpression[i]=='\\')
				{
					slashes++;
				} else if (function->RegularExpression[i]=='"')
				{
					quote++;
				}
			}

			length += escapes*4 + slashes + quote*4 + 1;
			char *convPattern=(char *)calloc(length, sizeof(char));
			char *p=convPattern;
			for (int i=0; i<(int)strlen(function->RegularExpression); i++)
			{
				if (( (u_char)function->RegularExpression[i]<0x20 || (u_char)function->RegularExpression[i]>0x79 ) &&
					(u_char)function->RegularExpression[i]!='|' &&
					(u_char)function->RegularExpression[i]!='^' &&
					(u_char)function->RegularExpression[i]!='.' &&
					(u_char)function->RegularExpression[i]!='$' &&
					(u_char)function->RegularExpression[i]!='*' &&
					(u_char)function->RegularExpression[i]!='(' && (u_char)function->RegularExpression[i]!=')' &&
					(u_char)function->RegularExpression[i]!='[' && (u_char)function->RegularExpression[i]!=']' &&
					(u_char)function->RegularExpression[i]!='{' && (u_char)function->RegularExpression[i]!='}')
				{
					strcpy(p, "\\\\x");
					p+=3;
					sprintf(p, "%2.2X", (unsigned char)function->RegularExpression[i]);
					p+=2;
				} else if (function->RegularExpression[i]=='\\')
				{
					strcpy(p, "\\\\");
					p+=2;
				} else if (function->RegularExpression[i]=='"')
				{
					strcpy(p, "\\\\x");
					p+=3;
					sprintf(p, "%2.2X", (unsigned char)'"');
					p+=2;
				} else
				{
					*p=function->RegularExpression[i];
					p++;
				}
			}

			*p='\0';


			SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen->ConstStrSymbol(string(convPattern));
			SymbolVarInt *matchedSize = m_CodeGen->NewIntVar("$matched_size", VAR_VALID_THISPKT);

			free(convPattern);
			SymbolRegExPDL *regExp = new SymbolRegExPDL(buffer, pattern, function->CaseSensitiveMatch, m_GlobalSymbols.GetRegExEntriesCount(),function->MatchToBeReturned,matchedSize);
			//store the operation on the regexp and the related protocol
			m_GlobalSymbols.StoreRegExEntry(regExp,m_CurrProtoSym);

			return m_CodeGen->TermNode(IR_REGEXXTR, regExp);		
			
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHANGEBYTEORDER:
		{
			struct _nbNetPDLExprFunctionChangeByteOrder* operand = (struct _nbNetPDLExprFunctionChangeByteOrder*)expr;
			HIRNode *toInvert = ParseExpressionStr(operand->OriginalStringExpression);
			return m_CodeGen->UnOp(IR_CHGBORD, toInvert);
		}break;

		//TODO [OM] Add all the cases (e.g. for checklookuptable() and so on)
	default:
		if (this->m_IsEncapsulation==false)
			m_CurrProtoSym->IsSupported = false;
		this->GenerateWarning(string("Expression '").append(expr->Token).append("' cannot be evaluated"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
		return NULL;
		break;
	}

	return NULL;
}

/*******************************************************************************************************************/
//				SET ATTRIBUTES OF THE FIELD PASSED THROUGH ARGUMENT				   //
/*******************************************************************************************************************/
#ifdef ENABLE_FIELD_OPT
void PDLParser::SetFieldInfo(SymbolField *field)
{

#ifdef ENABLE_FIELD_DEBUG
	ofstream my_ir("Debug_Fields_Modified_by_parser.txt", ios::app );
	my_ir << "********* START FIELD ATTRIBUTES - SET "<<endl;
	my_ir << field->Protocol->Name << "." << field->Name << " Used (outformatandafter) " << field->Used  << " Compattable: "<< field->Compattable << endl; 
#endif

  //Set this Field As Used && Not Compattable!
  field->Used=true;		
  field->Compattable=false;

  IntMap::iterator it;

  //Check if this field was already inserted in GLOBAL-MAP!
  if(m_symbolref.count(field)>0)  		//already added to the list-> UPDATE
  {
	it=m_symbolref.find(field);
	if(insideLoop>0 && (*it).second!=-1)
	{
		(*it).second++;
	}else
		(*it).second=-1;
  }
  else						//first time we meet this field -> INSERT
  {
	if(insideLoop>0)	
		m_symbolref.insert(pair<SymbolField*,int>(field,1));	//new element to add or to update
	else
		m_symbolref.insert(pair<SymbolField*,int>(field,-1));	//new element Used inside NEVER COMPATTABLE code!
  }


//***************************FOR Multidefinitions of this field!  (IT IS REDEFINED LATER INSIDE CODE)
#ifdef ENABLE_FIELD_DEBUG
	my_ir << "EXIST MULTIDEFINITIONS? " <<endl;
#endif

    for(FieldsList_t::iterator Listit = field->SymbolDefs.begin(); Listit!=field->SymbolDefs.end(); Listit++)
    {

#ifdef ENABLE_FIELD_DEBUG
	my_ir << (*Listit)->Protocol->Name << "." << (*Listit)->Name << " Used (outformatandafter) " << (*Listit)->Used << " ( " << " Compattable: "<< (*Listit)->Compattable << endl; 
#endif

	if((*Listit)->Compattable)	//only FIXED are compattable -> Avoid Bit problems
	{	
		(*Listit)->Used=true;
		(*Listit)->Compattable=false;

		  if(!(m_symbolref.count((*Listit))>0))  //if Not found!
		  {			
			if(insideLoop>0)	
				m_symbolref.insert(pair<SymbolField*,int>((*Listit),1));	//new element to add or to update
			else
				m_symbolref.insert(pair<SymbolField*,int>((*Listit),-1));	//new element Used inside uncompattable code 
		   }
	}
    }

#ifdef ENABLE_FIELD_DEBUG
	my_ir << "END OF MULTIDEFINITIONS" <<endl;
#endif

//***************************END OF MULTIDEFINITION CHECK

//***************************THIS field IS A CHILD? FOUND CONTAINER!!

  if(field->DependsOn!=NULL)		
  {

	SymbolField *container=field->DependsOn;


#ifdef ENABLE_FIELD_DEBUG
	my_ir <<"CONTAINER (OF CURRENT FIELD = "<<field->Name<<" ) : "<< container->Protocol->Name << "." << container->Name << endl;
#endif	

	SymbolField *otherfield=m_GlobalSymbols.LookUpProtoFieldByName(container->Protocol, container->Name); //DO NOT USE m_currproto! WHEN CALLED BY FRONTEND (FILTER FIELDS)

	nbASSERT(otherfield!=NULL, "CANNOT BE HERE!!");

	//OTHERFIELD IS THE REAL FIELD CONTAINER! (SYMBOLFIELD POINTED BY DEPENDSON ISN'T REAL!!!!)
	otherfield->Used=true;
	otherfield->Compattable=false;

	//if this container was already found (in Used-Fields MAP), (check if outside loops) otherwise -> do NOTHING!! 
	if(!(m_symbolref.count(otherfield)>0))  //if CONTAINER IS NOT found (INSIDE GLOBAL HASH-MAP OF USED FIELDS)!
	{			
		if(insideLoop>0)
		{
			#ifdef ENABLE_FIELD_DEBUG
				my_ir <<otherfield->Protocol->Name<<"."<<otherfield->Name<<" -> CONTAINER used! (inside Loop)"<<endl;
			#endif

			m_symbolref.insert(pair<SymbolField*,int>(otherfield,1));	//new element to add or to update
		}
		else
		{
			#ifdef ENABLE_FIELD_DEBUG
				my_ir <<otherfield->Protocol->Name<<"."<<otherfield->Name<<" -> CONTAINER used! (OUTSIDE LOOP)"<<endl;
			#endif

			m_symbolref.insert(pair<SymbolField*,int>(otherfield,-1));	//new element Used inside uncompattable code 
		}
	}
	else
	{
		//TEMP?
		if(insideLoop==0)		
		{
			IntMap::iterator tmp;
			tmp=m_symbolref.find(otherfield);
			
			#ifdef ENABLE_FIELD_DEBUG
				my_ir <<otherfield->Protocol->Name<<"."<<otherfield->Name<<" -> CONTAINER REDEFINED! (now OUTSIDE LOOP)"<<endl;
			#endif
			(*tmp).second=-1;
		}
	}
	

//***************************FOR Multidefinitions of this field! (CONTAINER)  (IT IS REDEFINED LATER INSIDE CODE?)
   for(FieldsList_t::iterator Listit = otherfield->SymbolDefs.begin(); Listit!=otherfield->SymbolDefs.end(); Listit++)
    {

	if((*Listit)->Compattable)
	{
		(*Listit)->Used=true;
		(*Listit)->Compattable=false;
		if(!(m_symbolref.count((*Listit))>0))  //if NOT found! (inside fields-MAP)
		{			
			if(insideLoop>0)	
				m_symbolref.insert(pair<SymbolField*,int>((*Listit),1));	//new element to add or to update
			else
				m_symbolref.insert(pair<SymbolField*,int>((*Listit),-1));	//new element Used inside uncompattable code 
		}
		else
		{
			//TEMP?
			if(insideLoop==0)		
			{
				IntMap::iterator tmp;
				tmp=m_symbolref.find(otherfield);
			
				#ifdef ENABLE_FIELD_DEBUG
					my_ir <<otherfield->Protocol->Name<<"."<<otherfield->Name<<" -> CONTAINER REDEFINED! (now OUTSIDE LOOP)"<<endl;
				#endif
				(*tmp).second=-1;
			}
		}


	}  //END OF EX-COMPATTABLE FIELD!

    }  //END OF MULTIDEF OF CONTAINER

  }  //END OF CONTAINER-CHECK
    
#ifdef ENABLE_FIELD_DEBUG
	my_ir << "********* END OF FIELD ATTRIBUTES - SET "<<endl<<endl;
	my_ir.close();
#endif

}


#endif

/*******************************************************************************************************************/
//						PARSE OPERANDS							   //
/*******************************************************************************************************************/
HIRNode *PDLParser::ParseOperandInt(_nbNetPDLExprBase *expr)				
{
	nbASSERT(expr!=NULL, "Expression to parse should not be NULL.");

	if (expr == NULL)
		return NULL;

	switch (expr->Type)
	{
	case nbNETPDL_ID_EXPR_OPERAND_NUMBER:
		{
			_nbNetPDLExprNumber* operand = (_nbNetPDLExprNumber*) expr;
			return m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(operand->Value));
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
		{
			_nbNetPDLExprVariable *operand = (_nbNetPDLExprVariable*)expr;
			Symbol *varSym = m_GlobalSymbols.LookUpVariable(operand->Name);

			if (varSym == NULL)
			{
				this->GenerateWarning(string("Variable '").append(operand->Name).append("' in expression '").append(expr->Token).append("' has not been declared or supported."),
					__FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}

#ifdef STATISTICS
			if ( ((SymbolVariable *)varSym)->VarType==PDL_RT_VAR_REFERENCE)
				this->m_CurrProtoSym->VarOccurrences++;

			this->m_CurrProtoSym->VarTotOccurrences++;
#endif

			return m_CodeGen->TermNode(IR_IVAR, varSym);
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_EXPR:
		return ParseExpressionInt(expr);
		break;

	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_BUF2INT:
		{
			_nbNetPDLExprFunctionBuf2Int *operand = (_nbNetPDLExprFunctionBuf2Int*) expr;

			// preload is required is the variable is a refbuffer
			this->m_ConvertingToInt=true;
			HIRNode *irExpr = ParseOperandStr(operand->StringExpression);
			this->m_ConvertingToInt=false;

			if (irExpr == NULL)
			{
				this->GenerateWarning(string("Expression '").append(operand->StringExpression->Token).append("' cannot be evaluated"),
					__FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}
			else if (irExpr->Op!=IR_CHGBORD && irExpr->Op!=IR_SVAR && !CheckIntOperand(irExpr))
			{
				this->GenerateWarning(string("Cannot convert operand from string to integer in expression '").append(operand->StringExpression->Token),
					__FILE__, __FUNCTION__, __LINE__, 1, 5);
				return NULL;
			}
			else
				return m_CodeGen->UnOp(IR_CINT, irExpr);
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE:				
	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE:				
		{
			_nbParamsLinkedList *paramsList = ((_nbNetPDLExprFunctionCheckUpdateLookupTable *)expr)->ParameterList;
			SymbolLookupTableKeysList *keys=new SymbolLookupTableKeysList();
			HIRNode *parExpr=NULL;
			while (paramsList != NULL)
			{
				if (paramsList->Expression->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
					parExpr=ParseExpressionStr(paramsList->Expression);
				else
					parExpr=ParseExpressionInt(paramsList->Expression);

				if (parExpr==NULL)
				{
					this->GenerateWarning(string("The key expression '").append(
						paramsList->Expression->Token).append(
						"' to select from the lookup table '").append(
						((_nbNetPDLExprFunctionCheckUpdateLookupTable *)expr)->TableName).append(
						"' has not been resolved correctly."),
						__FILE__, __FUNCTION__, __LINE__, 1, 4);
					return NULL;
				}

				keys->AddKey(parExpr);				

				paramsList = paramsList->NextParameter;
			}

			_nbNetPDLExprFunctionCheckUpdateLookupTable *function = (_nbNetPDLExprFunctionCheckUpdateLookupTable *)expr;
//TEMP

			if (expr->Type==nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE)
				return ParseLookupTableSelect(IR_LKSEL, function->TableName, keys);
			else
				return ParseLookupTableSelect(IR_LKHIT, function->TableName, keys);

		}break;

	case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:	
		{
			_nbNetPDLExprLookupTable *lookup=(_nbNetPDLExprLookupTable *)expr;

			HIRNode *offsetStart(0);
			HIRNode *offsetSize(0);

			if (lookup->OffsetStartAt!=NULL && lookup->OffsetSize!=NULL)
			{
				ParseExpressionInt(lookup->OffsetStartAt);
				ParseExpressionInt(lookup->OffsetSize);
			}
//TEMP
			return this->ParseLookupTableItem(lookup->TableName, lookup->FieldName, offsetStart, offsetSize);
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_PROTOREF:
		{
			_nbNetPDLExprProtoRef *protoref = (_nbNetPDLExprProtoRef*)expr;
			SymbolProto *proto = this->m_GlobalSymbols.LookUpProto(protoref->ProtocolName);
			nbASSERT(proto!=NULL, string("The expression ").append(protoref->ProtocolName).append(" refers to an unknown protocol.").c_str());
			return m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(proto->ID));
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_HASSTRING:
		{
//#ifdef USE_REGEX
			struct _nbNetPDLExprFunctionRegExp *function = (struct _nbNetPDLExprFunctionRegExp *)expr;
			
			HIRNode *buffer = ParseExpressionStr(function->SearchBuffer);

			int escapes=0;
			int slashes=0;
			int quote=0;
			int length=strlen(function->RegularExpression);
			for (int i=0; i<length; i++)
			{
				if (( (u_char)function->RegularExpression[i]<0x20 || (u_char)function->RegularExpression[i]>0x79 ) &&
					(u_char)function->RegularExpression[i]!='|' &&
					(u_char)function->RegularExpression[i]!='^' &&
					(u_char)function->RegularExpression[i]!='.' &&
					(u_char)function->RegularExpression[i]!='$' &&
					(u_char)function->RegularExpression[i]!='*' &&
					(u_char)function->RegularExpression[i]!='(' && (u_char)function->RegularExpression[i]!=')' &&
					(u_char)function->RegularExpression[i]!='[' && (u_char)function->RegularExpression[i]!=']' &&
					(u_char)function->RegularExpression[i]!='{' && (u_char)function->RegularExpression[i]!='}')
				{
					escapes++;
				} else if (function->RegularExpression[i]=='\\')
				{
					slashes++;
				} else if (function->RegularExpression[i]=='"')
				{
					quote++;
				}
			}

			length += escapes*4 + slashes + quote*4 + 1;
			char *convPattern=(char *)calloc(length, sizeof(char));
			char *p=convPattern;
			for (int i=0; i<(int)strlen(function->RegularExpression); i++)
			{
				if (( (u_char)function->RegularExpression[i]<0x20 || (u_char)function->RegularExpression[i]>0x79 ) &&
					(u_char)function->RegularExpression[i]!='|' &&
					(u_char)function->RegularExpression[i]!='^' &&
					(u_char)function->RegularExpression[i]!='.' &&
					(u_char)function->RegularExpression[i]!='$' &&
					(u_char)function->RegularExpression[i]!='*' &&
					(u_char)function->RegularExpression[i]!='(' && (u_char)function->RegularExpression[i]!=')' &&
					(u_char)function->RegularExpression[i]!='[' && (u_char)function->RegularExpression[i]!=']' &&
					(u_char)function->RegularExpression[i]!='{' && (u_char)function->RegularExpression[i]!='}')
				{
					strcpy(p, "\\\\x");
					p+=3;
					sprintf(p, "%2.2X", (unsigned char)function->RegularExpression[i]);
					p+=2;
				} else if (function->RegularExpression[i]=='\\')
				{
					strcpy(p, "\\\\");
					p+=2;
				} else if (function->RegularExpression[i]=='"')
				{
					strcpy(p, "\\\\x");
					p+=3;
					sprintf(p, "%2.2X", (unsigned char)'"');
					p+=2;
				} else
				{
					*p=function->RegularExpression[i];
					p++;
				}
			}

			*p='\0';


			SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen->ConstStrSymbol(string(convPattern));

			free(convPattern);

			SymbolRegExPDL *regExp = new SymbolRegExPDL(buffer, pattern, function->CaseSensitiveMatch, m_GlobalSymbols.GetRegExEntriesCount());
			//store the operation on the regexp and the related protocol
			m_GlobalSymbols.StoreRegExEntry(regExp,m_CurrProtoSym);

			return m_CodeGen->TermNode(IR_REGEXFND, regExp);

		}break;

	default:
		{
			this->GenerateWarning(string("Expression of type '").append(expr->Token).append("' is not supported"),
				__FILE__, __FUNCTION__, __LINE__, 1, 5);
			return NULL;
		}break;
	}

	return NULL;
}


/*****************************************************************************/
/**** Parsing fields     *****************************************************/
void PDLParser::ParseFieldDef(_nbNetPDLElementFieldBase *fieldElement)			
{
	SymbolField *fieldSymbol(0);
	HIRNode *expression(0);
	HIRNode *beginoffexpression(0);
	HIRNode *endoffexpression(0);
	HIRNode *enddiscexpression(0);

#ifdef STATISTICS
	this->m_CurrProtoSym->DeclaredFields++;
	if (this->m_ParentUnionField!=NULL)
		this->m_CurrProtoSym->InnerFields++;
#endif

	switch(fieldElement->FieldType)
	{
	case nbNETPDL_ID_FIELD_BIT:
		{
			_nbNetPDLElementFieldBit *fieldBit = (_nbNetPDLElementFieldBit*)fieldElement;
			fieldSymbol = new SymbolFieldBitField(fieldBit->Name, fieldBit->BitMask, (_nbNetPDLElementFieldBase*)fieldBit);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

			fieldSymbol->IntCompatible=true;

//IF this (BIT) field isn't USED -> SET AS COMPATTABLE!!! (we need to know that because Fixed Container can be set as Compattable only if all the childs are Compattable & Not Used)
#ifdef ENABLE_FIELD_OPT	
			if(fieldSymbol->Used==false)
				fieldSymbol->Compattable=true; 
#endif
		}break;

	case nbNETPDL_ID_FIELD_FIXED:
		{
			_nbNetPDLElementFieldFixed *fieldFixed = (_nbNetPDLElementFieldFixed*)fieldElement;
			fieldSymbol = new SymbolFieldFixed(fieldFixed->Name, fieldFixed->Size, fieldElement);					
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

			if (fieldFixed->Size>4)
				fieldSymbol->IntCompatible=false;

//IF this (FIXED) field isn't USED -> SET AS COMPATTABLE!!!
#ifdef ENABLE_FIELD_OPT	
				if(fieldSymbol->Used==false)
					fieldSymbol->Compattable=true; 
#endif

		}break;

	case nbNETPDL_ID_FIELD_VARIABLE:
		{
			_nbNetPDLElementFieldVariable *fieldVariable = (_nbNetPDLElementFieldVariable*)fieldElement;
			expression = ParseExpressionInt(fieldVariable->ExprTree);
			fieldSymbol = new SymbolFieldVarLen(fieldVariable->Name, fieldElement, expression);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;

			if (expression==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
			}
		}break;

	case nbNETPDL_ID_FIELD_TOKENENDED:
		{
			this->GenerateWarning("Type Field TokenEnded Deprecated.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
			_nbNetPDLElementFieldTokenEnded *fieldTokEnd = (_nbNetPDLElementFieldTokenEnded*)fieldElement;
			if(fieldTokEnd->EndOffsetExprTree!=NULL)
				endoffexpression=ParseExpressionInt(fieldTokEnd->EndOffsetExprTree);
			if(fieldTokEnd->EndDiscardExprTree!=NULL)
				enddiscexpression=ParseExpressionInt(fieldTokEnd->EndDiscardExprTree);
				
			fieldSymbol=new SymbolFieldTokEnd(fieldTokEnd->Name, fieldTokEnd->EndTokenString,
												fieldTokEnd->EndTokenStringSize,fieldTokEnd->EndOffsetExprString
												,fieldElement,endoffexpression,enddiscexpression);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;

			if (fieldTokEnd->EndTokenString==NULL && fieldTokEnd->EndOffsetExprString==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;

			}

			if (fieldTokEnd->EndOffsetExprTree!=NULL && endoffexpression==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
			}

			if (fieldTokEnd->EndDiscardExprTree!=NULL && enddiscexpression==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
			}

		}break;

	case nbNETPDL_ID_FIELD_TOKENWRAPPED:
		{
			this->GenerateWarning("Type Field TokenWrapped Deprecated.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
			_nbNetPDLElementFieldTokenWrapped *fieldTokWrap = (_nbNetPDLElementFieldTokenWrapped*)fieldElement;
			if(fieldTokWrap->BeginOffsetExprTree!=NULL)
				beginoffexpression=ParseExpressionInt(fieldTokWrap->BeginOffsetExprTree);
			if(fieldTokWrap->EndOffsetExprTree!=NULL)
				endoffexpression=ParseExpressionInt(fieldTokWrap->EndOffsetExprTree);
			if(fieldTokWrap->EndDiscardExprTree!=NULL)
				enddiscexpression=ParseExpressionInt(fieldTokWrap->EndDiscardExprTree);

			fieldSymbol=new SymbolFieldTokWrap(fieldTokWrap->Name, fieldTokWrap->BeginTokenString,
												fieldTokWrap->BeginTokenStringSize, fieldTokWrap->EndTokenString,
												fieldTokWrap->EndTokenStringSize,fieldTokWrap->BeginOffsetExprString,
												fieldTokWrap->EndOffsetExprString,fieldElement,
												beginoffexpression,endoffexpression,enddiscexpression);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;

			if (fieldTokWrap->BeginTokenString==NULL && fieldTokWrap->BeginOffsetExprString==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;

			}

			if (fieldTokWrap->EndTokenString==NULL && fieldTokWrap->EndOffsetExprString==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;

			}

			if (fieldTokWrap->BeginOffsetExprTree!=NULL && beginoffexpression==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
			}
			if (fieldTokWrap->EndOffsetExprTree!=NULL && endoffexpression==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
			}
			if (fieldTokWrap->EndDiscardExprTree!=NULL && enddiscexpression==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
			}
		}break;

	case nbNETPDL_ID_FIELD_LINE:
		{
			_nbNetPDLElementFieldLine *fieldLine= (_nbNetPDLElementFieldLine*)fieldElement;
			
			fieldSymbol = new SymbolFieldLine(fieldLine->Name, fieldElement);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;
		}break;

	case nbNETPDL_ID_FIELD_PADDING:
		{
			_nbNetPDLElementFieldPadding *fieldPad = (_nbNetPDLElementFieldPadding*)fieldElement;
			fieldSymbol = new SymbolFieldPadding(fieldPad->Name, fieldPad->Align, fieldElement);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;
		}break;

	case nbNETPDL_ID_FIELD_PLUGIN:
		{
			fieldSymbol=new SymbolField(fieldElement->Name, PDL_FIELD_INVALID_TYPE, fieldElement);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;
			fieldSymbol->IsSupported=false;
			fieldSymbol->IsDefined=false;
			this->GenerateWarning(string("Field '") + string(fieldElement->Name) + string("': plugin type is not supported"),
				__FILE__, __FUNCTION__, __LINE__, 1, fieldElement->NestingLevel);
		}break;

	case nbNETPDL_ID_FIELD_PATTERN:
		{
			_nbNetPDLElementFieldPattern *fieldPattern = (_nbNetPDLElementFieldPattern*)fieldElement;

			fieldSymbol=new SymbolFieldPattern(fieldPattern->Name,fieldPattern->PatternRegularExpression
												,fieldElement);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;

			if (fieldPattern->PatternRegularExpression==NULL)
			{
				fieldSymbol->IsSupported=false;
				fieldSymbol->IsDefined=false;
			}

		}break;
	case nbNETPDL_ID_FIELD_EATALL:
		{
			_nbNetPDLElementFieldEatall *fieldEatAll= (_nbNetPDLElementFieldEatall*)fieldElement;
			fieldSymbol = new SymbolFieldEatAll(fieldEatAll->Name, fieldElement);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;
		}break;

	default:
		{
			fieldSymbol=new SymbolField(fieldElement->Name, PDL_FIELD_INVALID_TYPE, fieldElement);
			if (fieldSymbol == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			fieldSymbol->IntCompatible=false;
			fieldSymbol->IsSupported=false;
			fieldSymbol->IsDefined=false;
			this->GenerateWarning(string("Field '").append(fieldElement->Name).append("' not supported"),
				__FILE__, __FUNCTION__, __LINE__, 1, fieldElement->NestingLevel);

#ifdef ENABLE_FIELD_OPT
			nbASSERT(false,"CANNOT BE HERE!");
#endif
			return;
		}break;
	}


	nbASSERT(fieldSymbol != NULL, "the symbol was not defined properly");

	// Add the protocol reference in the field Symbol
	fieldSymbol->Protocol=m_CurrProtoSym;

	if (fieldSymbol->IsSupported==false || fieldSymbol->IsDefined==false)
	{
#ifdef STATISTICS
		m_CurrProtoSym->UnsupportedFields++;
#endif
		if (this->m_IsEncapsulation==false)
			m_CurrProtoSym->IsSupported=false;
	}

	// [ds] store symbol and generate statament *only* if the symbol is actually defined
	if (fieldSymbol->IsDefined)
	{
#ifdef OPTIMIZE_SIZED_LOOPS
		if (m_CurrProtoSym->SizedLoopStack.size() > 0)
		{
			m_CurrProtoSym->Field2SizedLoopMap[ fieldSymbol ] = (StmtBase *)m_CurrProtoSym->SizedLoopStack.front();
		}
#endif

		// prevent definition of field but bit fields when there's an open implicity union field
		if (fieldSymbol->FieldType!=PDL_FIELD_BITFIELD && m_ParentUnionField!=NULL && m_ParentUnionField->Name.find("bit_union_")==0)
		{
			this->GenerateWarning(string("It's not allowed to define not bit fields ('").append(
				fieldSymbol->Name).append("') when there's an open implicity union field ('").append(m_ParentUnionField->Name).append("')"),
				__FILE__, __FUNCTION__, __LINE__, 1, fieldElement->NestingLevel);
			return;
		}

		//Store the field symbol into the field symbol tables
		if (fieldSymbol->FieldType==PDL_FIELD_FIXED && nbNETPDL_GET_ELEMENT(m_ProtoDB, fieldElement->FirstChild)!=NULL)
			// [ds] any bit container has a different definition, otherwise you have any bit defined in containers with the same name as they were defined in only a container
			fieldSymbol = StoreFieldSym(fieldSymbol, true); 			
		else
			fieldSymbol = StoreFieldSym(fieldSymbol);

			// generate code for symbols different from bitfield
		if(fieldSymbol->FieldType!=PDL_FIELD_BITFIELD)
			m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_DEFFLD, m_CodeGen->TermNode(IR_FIELD, fieldSymbol)));
	} else
		return;

	// process inner fields
	// SymbolFieldContainer *container=(SymbolFieldContainer *)fieldSymbol;

	// generate a dummy parent field only for bit fields
	if (fieldSymbol->FieldType==PDL_FIELD_BITFIELD && m_ParentUnionField == NULL)
	{
		m_ParentUnionField = new SymbolFieldFixed(string("bit_union_") + int2str(m_BitFldCount++, 10), ((_nbNetPDLElementFieldBit*)fieldElement)->Size);
		if (m_ParentUnionField == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

		//Store the field symbol into the field symbol tables
		m_ParentUnionField = (SymbolFieldContainer *)StoreFieldSym(m_ParentUnionField);
		
		// Add the protocol reference in the field Symbol
		m_ParentUnionField->Protocol=m_CurrProtoSym;

		//THIS NEW (FIXED) FIELD (bit_union) can be Compatted (if NOT it will be disabled later)
#ifdef ENABLE_FIELD_OPT	
		m_ParentUnionField->Compattable=true;	
#endif
		m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_DEFFLD, m_CodeGen->TermNode(IR_FIELD, m_ParentUnionField)));
		
	}

	// generate code for bitfield symbols for extact action 
	if(fieldSymbol->FieldType==PDL_FIELD_BITFIELD)
		m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_DEFFLD, m_CodeGen->TermNode(IR_FIELD, fieldSymbol)));
	
	if (m_ParentUnionField!=NULL)
	{
		// store dependencies
		SymbolFieldContainer *containerField=(SymbolFieldContainer *)fieldSymbol;
		containerField->DependsOn=m_ParentUnionField;
		m_ParentUnionField->InnerFields.push_back(containerField);

	}

	_nbNetPDLElementBase *innerField = (_nbNetPDLElementBase *)nbNETPDL_GET_ELEMENT(m_ProtoDB, fieldElement->FirstChild);

	if (innerField != NULL)
	{
		// save current element as container
		SymbolFieldContainer *previousContainer=m_ParentUnionField;
		m_ParentUnionField=(SymbolFieldContainer *)fieldSymbol;

		//if (fieldSymbol->Name.compare("response")==0)
		//	fieldSymbol->IsDefined=fieldSymbol->IsDefined;


		// parse inner fields
		while (innerField)
		{

			if (fieldSymbol->FieldType!=PDL_FIELD_BITFIELD)
				this->ParseElement(innerField);
			else if (innerField->Type==nbNETPDL_IDEL_FIELD)
				this->ParseFieldDef((_nbNetPDLElementFieldBase *)innerField);
			else
			{
				this->GenerateWarning(string("Bit field '").append(fieldElement->Name).append("' it's not allowed to contain not field definitions."),
					__FILE__, __FUNCTION__, __LINE__, 1, fieldElement->NestingLevel);
				innerField = (_nbNetPDLElementBase *)nbNETPDL_GET_ELEMENT(m_ProtoDB, innerField->NextSibling);
				continue;
			}

			if (innerField->Type==nbNETPDL_IDEL_FIELD)
			{
				SymbolField *field=this->LookUpFieldSym(((_nbNetPDLElementFieldBase *)innerField)->Name);
				if (field==NULL || !field->IsDefined)
					// if a child is not define, the current symbol is not defined as well
					fieldSymbol->IsDefined=false;
			}

			innerField = (_nbNetPDLElementBase *)nbNETPDL_GET_ELEMENT(m_ProtoDB, innerField->NextSibling);
		}

		// restore container
		m_ParentUnionField=previousContainer;
	}

	// check if you should close the union field
	if (m_ParentUnionField!=NULL)
	{
		if (fieldElement->NextSibling != 0)
		{
			_nbNetPDLElementBase *nextSibling = (_nbNetPDLElementBase *)nbNETPDL_GET_ELEMENT(m_ProtoDB, fieldElement->NextSibling);
			if (nextSibling->Type==nbNETPDL_IDEL_FIELD)
			{
				// if the following symbol is not a bit field, and the union field is implicity, so close it
				if (((_nbNetPDLElementFieldBase *)nextSibling)->FieldType!=nbNETPDL_ID_FIELD_BIT && m_ParentUnionField->Name.find("bit_union_")==0)
					m_ParentUnionField=NULL;
			}
			else
				m_ParentUnionField=NULL;
		}
		//else
		//	m_ParentUnionFielexpr=ParseExpressionInt(key->ExprTree);//d=NULL;
	}
}


/*****************************************************************************/
/**** Parsing assignments     ************************************************/
void PDLParser::ParseAssign(_nbNetPDLElementAssignVariable *assignElement)
{
	SymbolVariable *varSymbol(0);
	HIRNode *expression(0);
	HIRNode *assignNode(0);

	varSymbol = m_GlobalSymbols.LookUpVariable(assignElement->VariableName);

	if (varSymbol == NULL)
	{
		this->GenerateWarning(string("Variable '").append(assignElement->VariableName).append("' has not been declared before assignment"),
			__FILE__, __FUNCTION__, __LINE__, 1, assignElement->NestingLevel);
		return;
	}

	switch (varSymbol->VarType)
	{
	case PDL_RT_VAR_INTEGER:
		{
			expression = ParseExpressionInt(assignElement->ExprTree);
			if (expression!=NULL)
				assignNode = m_CodeGen->BinOp(IR_ASGNI, m_CodeGen->TermNode(IR_IVAR, varSymbol), expression);
			else
			{
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
				this->GenerateWarning(string("Expression '").append(string(assignElement->ExprString)).append(string("' has not been correctly parsed.")),
					__FILE__, __FUNCTION__, __LINE__, 1, assignElement->NestingLevel);
				return;
			}
		}break;
	case PDL_RT_VAR_REFERENCE:
		{
			SymbolVarBufRef *varBufRef = (SymbolVarBufRef *)varSymbol;

			if (varBufRef->RefType==REF_IS_PACKET_VAR) {
				m_ErrorRecorder.PDLError(string("Variable ").append(assignElement->VariableName).append(": cannot perform an assignment to a reserved variable"));
				return;
			}

#ifdef STATISTICS
			this->m_CurrProtoSym->VarOccurrences++;
#endif

			expression = ParseExpressionStr(assignElement->ExprTree);

			if (expression == NULL) {
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
				this->GenerateWarning(string("Variable '").append(string(assignElement->VariableName)).append(string("': the expression has not been correctly parsed.")),
					__FILE__, __FUNCTION__, __LINE__, 1, assignElement->NestingLevel);
				return;
			}

			if (expression->Sym == NULL) {
				this->GenerateWarning(string("Variable '").append(string(assignElement->VariableName)).append(string("': symbol not defined.")),
					__FILE__, __FUNCTION__, __LINE__, 1, assignElement->NestingLevel);
				return;
			}

			switch (expression->Op)
			{
			case IR_FIELD:		
				{
					if (varBufRef->RefType==REF_IS_UNKNOWN || varBufRef->RefType==REF_IS_REF_TO_FIELD)
					{
						SymbolField *field=(SymbolField *)expression->Sym;
						varBufRef->RefType=REF_IS_REF_TO_FIELD;

						varBufRef->ReferredSyms.push_back(field);
						if (field->IntCompatible==false)
							varBufRef->IntCompatible=false;

						assignNode = m_CodeGen->BinOp(IR_ASGNS, m_CodeGen->TermNode(IR_SVAR, varBufRef), expression, field);
#ifdef ENABLE_FIELD_OPT				
	//ANALYZE THIS FIELD! -> SET FLAGS
						SetFieldInfo(field);
#endif
					}
					else
					{
						this->GenerateWarning(string("Variable '").append(string(assignElement->VariableName)).append(string("' has been previously used not as a protocol field, and cannot be assigned to a protocol field.")),
							__FILE__, __FUNCTION__, __LINE__, 1, assignElement->NestingLevel);
						return;
					}
				}break;
			default:
				{
					this->GenerateWarning(string("Variable '").append(string(assignElement->VariableName)).append(string("': the expression should define a field (or a variable -NOT IMPLEMENTED-).")),
						__FILE__, __FUNCTION__, __LINE__, 1, assignElement->NestingLevel);
					return;
				}
			}
		}break;
	default:
		this->GenerateWarning(string("Variable '").append(string(assignElement->VariableName)).append(string("' type not supported")),
			__FILE__, __FUNCTION__, __LINE__, 1, assignElement->NestingLevel);
		return;
		break;
	}

#ifdef STATISTICS
	this->m_CurrProtoSym->VarTotOccurrences++;
#endif

	m_CodeGen->GenStatement(assignNode);
	
}

/*****************************************************************************/
/**** Parsing lookup tables     **********************************************/
//Parses the NetPDL element <lookuptable>
void PDLParser::ParseLookupTable(_nbNetPDLElementLookupTable *table)
{
	if (this->m_GlobalSymbols.LookUpLookupTable(table->Name)!=NULL)
	{
		this->GenerateWarning(string("The lookup table '").append(table->Name).append("' has not been already declared."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return;
	}

	TableValidity validity=(table->AllowDynamicEntries) ? TABLE_VALIDITY_DYNAMIC : TABLE_VALIDITY_STATIC;
	
	//[icerrato]	
	//SymbolLookupTable *symTable=new SymbolLookupTable(table->Name, validity, table->NumExactEntries, table->NumMaskEntries);
	//if (symTable == NULL)
	//	throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	//[icerrato]
	//Crates a new SymbolLookupTable
	SymbolLookupTable *symTable = m_CodeGen->NewLookupTable(table->Name, validity, table->NumExactEntries, table->NumMaskEntries);
	//generates an HIR statement to define the lookup table
	HIRNode *tableNode = m_CodeGen->TermNode(IR_LKTABLE, symTable);	
	m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_LKDEFTABLE, tableNode));

	//store the LookupTable into the GlobaSymbolsTable
	this->m_GlobalSymbols.StoreLookupTable(table->Name, symTable);

	
	//add keys
	SymbolLookupTableItem *keyDescriptor=NULL;
	_nbNetPDLElementKeyData *key = table->FirstKey;
	while (key!=NULL)//iterates on the <key> elements
	{
		PDLVariableType type;
		
		//[icerrato]
		HIRNode *keyNode(0);
		
		switch (key->KeyDataType)
		{
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
			type=PDL_RT_VAR_INTEGER;
			break;
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
			type=PDL_RT_VAR_BUFFER;
			break;
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
			type=PDL_RT_VAR_PROTO;
			break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			break;
		}
		
		//[icerrato]
		keyDescriptor= m_CodeGen->NewLookupTableItem(symTable, key->Name, type, key->Size);			
		//keyDescriptor=new SymbolLookupTableItem(symTable, key->Name, type, key->Size);
	
		
		//[icerrato]
		if(type!=PDL_RT_VAR_BUFFER){
			keyNode = m_CodeGen->TermNode(IR_LKIKEY, keyDescriptor);
			//generates an HIR statement about an integer or a protocol key
			m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_LKDEFKEYI, keyNode/*, expression*/));
		}
		else{
			//type == PDL_RT_VAR_BUFFER
			keyNode = m_CodeGen->TermNode(IR_LKSKEY, keyDescriptor);		
			//generates an HIR statement about a buffer key
			m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_LKDEFKEYS, keyNode));
		}
		
		symTable->AddKey(keyDescriptor);

		key = key->NextKeyData;
	}


	// add values
	SymbolLookupTableItem *valueDescriptor=NULL;
	_nbNetPDLElementKeyData *data = table->FirstData;
	while (data!=NULL)//iterates on the <data> elements
	{
		PDLVariableType type;
		
		//[icerrato]
		HIRNode *keyNode(0);

		switch (data->KeyDataType)
		{
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
			type=PDL_RT_VAR_INTEGER;
			break;
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
			type=PDL_RT_VAR_BUFFER;
			break;
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
			type=PDL_RT_VAR_PROTO;
			break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			break;
		}
		
		//[icerrato]
		valueDescriptor= m_CodeGen->NewLookupTableItem(symTable, data->Name, type, data->Size);
		//valueDescriptor=new SymbolLookupTableItem(symTable, data->Name, type, data->Size);
		
		//[icerrato]
		if(type!=PDL_RT_VAR_BUFFER){
			keyNode = m_CodeGen->TermNode(IR_LKIDATA, valueDescriptor);
			//generates an HIR statement about an integer or a protocol data
			m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_LKDEFDATAI, keyNode/*, expression*/));
		}
		else{
			//type == PDL_RT_VAR_BUFFER
			keyNode = m_CodeGen->TermNode(IR_LKSDATA, valueDescriptor);		
			//generates an HIR statement about a buffer data
			m_CodeGen->GenStatement(m_CodeGen->UnOp(IR_LKDEFDATAS, keyNode));
		}

		symTable->AddValue(valueDescriptor);

		data = data->NextKeyData;
	}

#ifdef STATISTICS
	this->m_CurrProtoSym->LookupDecl++;
#endif
}

HIRNode *PDLParser::ParseLookupTableSelect(HIROpcodes op, string tableName, SymbolLookupTableKeysList *keys)			
{
	SymbolLookupTable *symTable = this->m_GlobalSymbols.LookUpLookupTable(tableName);

	if (symTable == NULL)
	{
		this->GenerateWarning(string("The lookup table '").append(tableName).append("' has not been declared."),
			__FILE__, __FUNCTION__, __LINE__, 1, 5);
		return NULL;
	}
	
	this->m_GlobalSymbols.AssociateProtoTable(m_CurrProtoSym,tableName);
	this->m_GlobalSymbols.AssociateTableProto(tableName,m_CurrProtoSym);

	SymbolLookupTableEntry *entry=new SymbolLookupTableEntry(symTable);

	// create select instruction
	if (op == IR_LKHIT && symTable->Validity!=TABLE_VALIDITY_DYNAMIC)
		return m_CodeGen->UnOp(IR_LKSEL, m_CodeGen->TermNode(IR_LKKEYS, keys), entry);
	else
		return m_CodeGen->UnOp(op, m_CodeGen->TermNode(IR_LKKEYS, keys), entry);

#ifdef STATISTICS
	this->m_CurrProtoSym->LookupOccurrences++;
#endif
}

void PDLParser::ParseLookupTableUpdate(_nbNetPDLElementUpdateLookupTable *element)				
{
	SymbolLookupTable *symTable = this->m_GlobalSymbols.LookUpLookupTable(element->TableName);
	if (symTable==NULL)
	{
		this->GenerateWarning(string("The lookup table '").append(element->TableName).append("' has been declared."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return;
	}
	
	this->m_GlobalSymbols.AssociateProtoTable(m_CurrProtoSym,element->TableName);
	this->m_GlobalSymbols.AssociateTableProto(element->TableName,m_CurrProtoSym);

	switch (element->Action)
	{
	case nbNETPDL_UPDATELOOKUPTABLE_ACTION_ADD:
		{
			if (symTable->Validity==TABLE_VALIDITY_STATIC)
			{
				switch (element->Validity)
				{
				case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER:
					break;
				case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPMAXTIME:
				case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_UPDATEONHIT:
				case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT:
				case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT:
					this->GenerateWarning(string("Update lookup table instruction cannot use dynamic validity values when the table is static."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
					return;
					break;
				default:
					this->GenerateWarning(string("Update lookup table instruction uses an unknown validity value."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
					return;
					break;
				}
			}

			LookupTableEntryValidity validity = LOOKUPTABLE_ENTRY_VALIDITY_NONE;
			switch (element->Validity)
			{
			case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER:
				validity=LOOKUPTABLE_ENTRY_VALIDITY_KEEPFOREVER;
				break;
			case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPMAXTIME:
				validity=LOOKUPTABLE_ENTRY_VALIDITY_KEEPMAXTIME;
				break;
			case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_UPDATEONHIT:
				validity=LOOKUPTABLE_ENTRY_VALIDITY_UPDATEONHIT;
				break;
			case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT:
				validity=LOOKUPTABLE_ENTRY_VALIDITY_REPLACEONHIT;
				break;
			case nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT:
				validity=LOOKUPTABLE_ENTRY_VALIDITY_ADDONHIT;
				break;
			}

			SymbolLookupTableEntry *entry=new SymbolLookupTableEntry(symTable, validity);
			SymbolLookupTableKeysList *keys=new SymbolLookupTableKeysList();
			SymbolLookupTableValuesList *values=new SymbolLookupTableValuesList();

			_nbNetPDLElementLookupKeyData *key = element->FirstKey;
			SymbolLookupTableItemsList_t::iterator keyIterator = symTable->KeysList.begin();
			while (key!=NULL && keyIterator!=symTable->KeysList.end())
			{
				HIRNode *expr = NULL;

				switch ((*keyIterator)->Type)
				{
				case PDL_RT_VAR_BUFFER:
					expr = ParseExpressionStr(key->ExprTree);
					break;
				case PDL_RT_VAR_INTEGER:
					expr = ParseExpressionInt(key->ExprTree);
					break;
				case PDL_RT_VAR_PROTO:
					expr = ParseExpressionInt(key->ExprTree);
					break;
				default:
					break;
				}

				if (expr == NULL)
				{
#ifdef STATISTICS
					this->m_CurrProtoSym->UnsupportedExpr++;
#endif
					this->GenerateWarning(string("The key expression '").append(key->ExprString).append("' to add to the lookup table '").append(symTable->Name).append("' has not been resolved correctly."),
						__FILE__, __FUNCTION__, __LINE__, 1, 4);
					return;
				}

				if (key->Mask!=NULL)
					keys->AddKey(expr, m_CodeGen->TermNode(IR_SCONST, m_CodeGen->ConstStrSymbol(string((const char*)key->Mask))));
				else
					keys->AddKey(expr);


				keyIterator++;
				key = key->NextKeyData;
			}

			_nbNetPDLElementLookupKeyData *data = element->FirstData;
			SymbolLookupTableItemsList_t::iterator valueIterator = symTable->ValuesList.begin();
			while (data!=NULL && valueIterator!=symTable->ValuesList.end())
			{
				HIRNode *expr = NULL;
				switch ((*valueIterator)->Type)
				{
				case PDL_RT_VAR_BUFFER:
					expr = ParseExpressionStr(data->ExprTree);
					break;
				case PDL_RT_VAR_INTEGER:
					expr = ParseExpressionInt(data->ExprTree);
					break;
				case PDL_RT_VAR_PROTO:
					expr = ParseExpressionInt(data->ExprTree);
					break;
				default:
					break;
				}

				if (expr == NULL)
				{
#ifdef STATISTICS
					this->m_CurrProtoSym->UnsupportedExpr++;
#endif
					this->GenerateWarning(string("The value expression '").append(data->ExprString).append("' to add to the lookup table '").append(symTable->Name).append("'has not been resolved correctly."),
						__FILE__, __FUNCTION__, __LINE__, 1, 4);

					return;
				}

				values->AddValue(expr);

				valueIterator++;
				data = data->NextKeyData;
			}

			entry->HiddenValues[HIDDEN_FLAGS_INDEX] = m_CodeGen->TermNode(IR_ICONST, new SymbolIntConst(validity));
			// todo [ds] initialize timestamp
			entry->HiddenValues[HIDDEN_LIFESPAN_INDEX] = m_CodeGen->TermNode(IR_ICONST, new SymbolIntConst(element->KeepTime));
			entry->HiddenValues[HIDDEN_KEEPTIME_INDEX] = m_CodeGen->TermNode(IR_ICONST, new SymbolIntConst(element->KeepTime));
			entry->HiddenValues[HIDDEN_HITTIME_INDEX] = m_CodeGen->TermNode(IR_ICONST, new SymbolIntConst(element->HitTime));
			entry->HiddenValues[HIDDEN_NEWHITTIME_INDEX] = m_CodeGen->TermNode(IR_ICONST, new SymbolIntConst(element->NewHitTime));

			// generate add statement
			m_CodeGen->GenStatement(m_CodeGen->TermNode(IR_LKADD, entry, m_CodeGen->TermNode(IR_LKKEYS, keys), m_CodeGen->TermNode(IR_LKVALS, values)));

			}break;
	case nbNETPDL_UPDATELOOKUPTABLE_ACTION_OBSOLETE:
	case nbNETPDL_UPDATELOOKUPTABLE_ACTION_PURGE:
		{
			// select the entry
			_nbNetPDLElementLookupKeyData *paramsList = element->FirstKey;
			SymbolLookupTableKeysList *keys=new SymbolLookupTableKeysList();
			HIRNode *parExpr=NULL;
			while (paramsList != NULL)
			{
				if (paramsList->ExprTree->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
					parExpr=ParseExpressionStr(paramsList->ExprTree);
				else
					parExpr=ParseExpressionInt(paramsList->ExprTree);

				if (parExpr==NULL)
				{
#ifdef STATISTICS
					this->m_CurrProtoSym->UnsupportedExpr++;
#endif
					this->GenerateWarning(string("The key expression '").append(paramsList->ExprString).append("' to select from the lookup table '").append(symTable->Name).append("' has not been resolved correctly."),
						__FILE__, __FUNCTION__, __LINE__, 1, 4);
					return;
				}

				if (paramsList->Mask!=NULL)
					keys->AddKey(parExpr, m_CodeGen->TermNode(IR_SCONST, m_CodeGen->ConstStrSymbol(string((const char*)paramsList->Mask))));
				else
					keys->AddKey(parExpr);

				paramsList = paramsList->NextKeyData;
			}

			HIRNode *selection=ParseLookupTableSelect(IR_LKSEL, element->TableName, keys);
			if (selection==NULL)
			{
				this->GenerateWarning(string("The keys list to delete an entry from the lookup table '").append(symTable->Name).append("' has not been resolved correctly."),
					__FILE__, __FUNCTION__, __LINE__, 1, 4);
				return;
			}

			m_CodeGen->GenStatement(selection);

			if (element->Action==nbNETPDL_UPDATELOOKUPTABLE_ACTION_PURGE)
				// delete the entry
				m_CodeGen->GenStatement(m_CodeGen->TermNode(IR_LKDEL, symTable));
			else
			{
				if (symTable->Validity==TABLE_VALIDITY_DYNAMIC)
				{
					HIRNode *offsetNode = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(symTable->GetHiddenValueOffset(HIDDEN_LIFESPAN_INDEX)));
					HIRNode *newValueNode = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(180));
					m_CodeGen->GenStatement(m_CodeGen->TermNode(IR_LKUPDI, symTable, offsetNode, newValueNode));
				}
				else
				{
					// delete the entry
					m_CodeGen->GenStatement(m_CodeGen->TermNode(IR_LKDEL, symTable));
				}

			}
		}break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}


#ifdef STATISTICS
	this->m_CurrProtoSym->LookupOccurrences++;
#endif
}

HIRNode *PDLParser::ParseLookupTableItem(string tableName, string itemName, HIRNode *offsetStart, HIRNode *offsetSize)		
{
	SymbolLookupTable *symTable = this->m_GlobalSymbols.LookUpLookupTable(tableName);
	if (symTable==NULL)
	{
		this->GenerateWarning(string("The lookup table '").append(tableName).append("' has not been declared."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return NULL;
	}
	
	this->m_GlobalSymbols.AssociateProtoTable(m_CurrProtoSym,tableName);
	this->m_GlobalSymbols.AssociateTableProto(tableName,m_CurrProtoSym);

	// get the item from the lookup table
	SymbolLookupTableItem *item = symTable->GetValue(itemName);
	if (symTable==NULL)
	{
		this->GenerateWarning(string("The item '").append(tableName).append(".").append(itemName).append("' has not been declared."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return NULL;
	}

	// load the item value
	HIRNode *field = m_CodeGen->TermNode(IR_SVAR, item, offsetStart, offsetSize);

	return field;
}

/*
void PDLParser::ParseLookupTableAssign(_nbNetPDLElementAssignLookupTable *element)
{
	HIRNode *offsetStartNode(0);
	HIRNode *offsetSizeNode(0);
	if (element->OffsetStartAt!=0 && element->OffsetSize!=0)
	{
		offsetStartNode=m_CodeGen->TermNode(IR_ICONST, element->OffsetStartAt);
		offsetSizeNode=m_CodeGen->TermNode(IR_ICONST, element->OffsetSize);
	}

	SymbolLookupTable *symTable = this->m_GlobalSymbols.LookUpLookupTable(element->TableName);
	if (symTable==NULL)
	{
		this->GenerateWarning(string("The lookup table '").append(element->TableName).append("' has not been declared."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return;
	}
	
	this->m_GlobalSymbols.AssociateTableProto(element->TableName,m_CurrProtoSym);

	// get the item from the lookup table
	SymbolLookupTableItem *item = symTable->GetValue(element->FieldName);
	if (symTable==NULL)
	{
		this->GenerateWarning(string("The item '").append(element->TableName).append(".").append(element->FieldName).append("' has not been declared."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return;
	}

	// load the item offset
	HIRNode *field = m_CodeGen->TermNode(PUSH, m_CodeGen->ConstIntSymbol(symTable->GetValueOffset(item->Name))); //[icerrato] PUSH is not a valid HIR opcode!

	// TODO offset

	HIRNode *expr = NULL;
	HIRNode *assign = NULL;
	if (element->ExprTree->ReturnType==nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
	{
		expr=ParseExpressionStr(element->ExprTree);
		if (expr!=NULL)
		{
			// assign the new value
			assign = m_CodeGen->BinOp(IR_LKUPDS, field, expr, symTable);
		}
		else
		{
#ifdef STATISTICS
			this->m_CurrProtoSym->UnsupportedExpr++;
#endif
		}
	}
	else
	{
		expr=ParseExpressionInt(element->ExprTree);
		if (expr!=NULL)
		{
			// assign the new value
			assign = m_CodeGen->BinOp(IR_LKUPDI, field, expr, symTable);
		}
		else
		{
#ifdef STATISTICS
			this->m_CurrProtoSym->UnsupportedExpr++;
#endif
		}
	}

	m_CodeGen->GenStatement(assign);

#ifdef STATISTICS
	this->m_CurrProtoSym->LookupOccurrences++;
#endif
}*/


/*****************************************************************************/
/**** Parsing loops     ******************************************************/
void PDLParser::ParseLoop(_nbNetPDLElementLoop *loopElement)
{
	this->GenerateInfo(string("Parsing 'loop' element"), __FILE__, __FUNCTION__, __LINE__, 3, loopElement->NestingLevel);

	uint32 loopCnt(m_LoopCount++);
	string loopID = m_CurrProtoSym->Name + "lo" + int2str(loopCnt, 10);

	switch(loopElement->LoopType)
	{
		//! The loop is terminated after a given amount of decoded data
	case nbNETPDL_ID_LOOP_SIZE:
		{

	//LOOP SIZE RECOGNIZED! ( INC InsideLoop Counter )
	#ifdef ENABLE_FIELD_OPT
	 insideLoop++;
	#endif

			this->m_CodeGen->CommentStatement(string("WHILE (size) '").append(loopElement->ExprString).append("'"));
			/* generates the following code:

			$loop_N_sent = $currentoffset + size_expr
			while ($currentoffset < $loop_N_sent)
			{
			....
			}
			*/

			//loop expression
			HIRNode *expr = ParseExpressionInt(loopElement->ExprTree);
			//if cannot parse the expression return!
			if (expr == NULL)
			{
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
				if (this->m_IsEncapsulation==false)
					this->m_CurrProtoSym->IsSupported=false;

				this->GenerateWarning(string("'loop' element requires an integer or boolean expression"), __FILE__, __FUNCTION__, __LINE__, 1, loopElement->NestingLevel);
				return;
			}
			
			//$loop_N_sent variable
			SymbolVarInt *sentinel = m_CodeGen->NewIntVar(loopID + string("_sent"));
			//$currentoffset + size_expr
			HIRNode *sum = m_CodeGen->BinOp(IR_ADDI, m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym), expr);
			//$loop_N_sent = $currentoffset + size_expr
			m_CodeGen->GenStatement(m_CodeGen->BinOp(IR_ASGNI, m_CodeGen->TermNode(IR_IVAR, sentinel), sum));
			//while condition: $currentoffset < $loop_N_sent
			HIRNode *condition = m_CodeGen->BinOp(IR_LTI, m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym), \
				m_CodeGen->TermNode(IR_IVAR, sentinel));

			StmtWhile *whileStmt = m_CodeGen->WhileDoStatement(condition);

			//LOOP SIZE RECOGNIZED! ( SET FLAG AS TRUE )
			whileStmt->isLoopSize=true;	

			//COMMENT = EXPRESSION STRING! (ONLY FOR LOOP-SIZE CONVERTED IN WHILE)
			((StmtBase *)whileStmt)->Comment=loopElement->ExprString;

#ifdef OPTIMIZE_SIZED_LOOPS
			if (m_CurrProtoSym->SizedLoopStack.size() > 0)
			{
				m_CurrProtoSym->SizedLoop2SizedLoopMap[ whileStmt ] = (StmtBase *)m_CurrProtoSym->SizedLoopStack.front();
			}

			m_CurrProtoSym->SizedLoopStack.push_front(whileStmt);
#endif

			//EnterFldScope();
			HIRCodeGen whileBodyCodeGen(m_GlobalSymbols, whileStmt->Code->Code);
			ChangeCodeGen(whileBodyCodeGen);
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, loopElement->FirstChild));
			RestoreCodeGen();
			//ExitFldScope();

#ifdef OPTIMIZE_SIZED_LOOPS
			m_CurrProtoSym->SizedLoopStack.pop_front();
#endif
	
	//LOOP SIZE ENDED! ( DEC InsideLoop Counter )
	#ifdef ENABLE_FIELD_OPT
	 insideLoop--;
	#endif

			this->m_CodeGen->CommentStatement(string("END WHILE (size)"));
		}break;

		//! The loop is terminated after a given number of rounds
	case nbNETPDL_ID_LOOP_T2R:
		{
			this->m_CodeGen->CommentStatement(string("WHILE (rounds) '").append(loopElement->ExprString).append("'"));
			/* generates the following code:

			for ($loop_N_cnt = times_expr; $loop_N_cnt > 0; $loop_N_cnt--)
			{
			...
			}

			*/

			//$loop_N_cnt
			SymbolVarInt *index = m_CodeGen->NewIntVar(loopID + "_cnt");

			HIRNode *condition = m_CodeGen->BinOp(IR_GTI, m_CodeGen->TermNode(IR_IVAR, index), \
				m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(0)));
			HIRNode *sub = m_CodeGen->BinOp(IR_SUBI, m_CodeGen->TermNode(IR_IVAR, index), m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(1)));
			StmtGen *indexDecStmt = new StmtGen(m_CodeGen->BinOp(IR_ASGNI, m_CodeGen->TermNode(IR_IVAR, index), sub));
			CHECK_MEM_ALLOC(indexDecStmt);
			//loop expression
			HIRNode *expr = ParseExpressionInt(loopElement->ExprTree);
			//if cannot parse the expression return!
			if (expr == NULL)
			{
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
				if (this->m_IsEncapsulation==false)
					this->m_CurrProtoSym->IsSupported=false;

				this->GenerateWarning(string("'loop' element requires an integer or boolean expression"), __FILE__, __FUNCTION__, __LINE__, 1, loopElement->NestingLevel);
				return;
			}
			StmtLoop *loopStmt = m_CodeGen->LoopStatement(m_CodeGen->TermNode(IR_IVAR, index), expr, condition, indexDecStmt);

			//code for inner elements
			//EnterFldScope();
			HIRCodeGen loopBodyCodeGen(m_GlobalSymbols,loopStmt->Code->Code);
			ChangeCodeGen(loopBodyCodeGen);
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, loopElement->FirstChild));
			RestoreCodeGen();
			this->m_CodeGen->CommentStatement(string("END WHILE (rounds)"));
		}break;

		//! The loop is terminated due to a condition (evaluated at the beginning)
	case nbNETPDL_ID_LOOP_WHILE:
		{
			this->m_CodeGen->CommentStatement(string("WHILE '").append(loopElement->ExprString).append("'"));
			/* generates the following code:

			while(expr)
			{
			....
			....
			}
			*/
			//loop expression
			HIRNode *expr = ParseExpressionInt(loopElement->ExprTree);
			//if cannot parse the expression return!
			if (expr == NULL)
			{
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
				if (this->m_IsEncapsulation==false)
					this->m_CurrProtoSym->IsSupported=false;

				this->GenerateWarning(string("'loop' element requires an integer or boolean expression"), __FILE__, __FUNCTION__, __LINE__, 1, loopElement->NestingLevel);
				return;
			}
			StmtWhile *whileStmt = m_CodeGen->WhileDoStatement(expr);

			//CLEAN WHILE-STATEMENT COMMENT
			((StmtBase *)whileStmt)->Comment="";

			//EnterFldScope();
			HIRCodeGen whileBodyCodeGen(m_GlobalSymbols, whileStmt->Code->Code);
			ChangeCodeGen(whileBodyCodeGen);
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, loopElement->FirstChild));
			RestoreCodeGen();
			//ExitFldScope();
			this->m_CodeGen->CommentStatement(string("END WHILE"));
		}break;

		//! The loop is terminated due to a condition (evaluated at the end)
	case nbNETPDL_ID_LOOP_DOWHILE:
		{
			this->m_CodeGen->CommentStatement(string("DO"));
			/* generates the following code:

			do
			{
			....
			....
			}while (expr);

			*/
			//we have first to parse the code inside the loop and then evaluate the expression
			StmtWhile *whileStmt = m_CodeGen->DoWhileStatement(NULL);


			//CLEAN DOWHILE-STATEMENT COMMENT
			((StmtBase *)whileStmt)->Comment="";

			//EnterFldScope();
			HIRCodeGen whileBodyCodeGen(m_GlobalSymbols, whileStmt->Code->Code);
			ChangeCodeGen(whileBodyCodeGen);
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, loopElement->FirstChild));
			RestoreCodeGen();
			//loop expression
			HIRNode *expr = ParseExpressionInt(loopElement->ExprTree);
			//if cannot parse the expression return!
			if (expr == NULL)
			{
#ifdef STATISTICS
				this->m_CurrProtoSym->UnsupportedExpr++;
#endif
				if (this->m_IsEncapsulation==false)
					this->m_CurrProtoSym->IsSupported=false;

				this->GenerateWarning(string("'loop' element requires an integer or boolean expression"), __FILE__, __FUNCTION__, __LINE__, 1, loopElement->NestingLevel);
				return;
			}
			whileStmt->Forest = expr;
			//ExitFldScope();
			this->m_CodeGen->CommentStatement(string("WHILE '").append(loopElement->ExprString).append("'"));
		}break;

	default:
		this->GenerateWarning(string("'loop' type not supported"), __FILE__, __FUNCTION__, __LINE__, 1, loopElement->NestingLevel);
		break;
	}
}

void PDLParser::ParseLoopCtrl(_nbNetPDLElementLoopCtrl *loopCtrlElement)
{
	switch(loopCtrlElement->LoopCtrlType)
	{
		//! The loopctrl forces the loop to break
	case nbNETPDL_ID_LOOPCTRL_BREAK:
		m_CodeGen->BreakStatement();
		break;
		//! The loopctrl forces the loop to restart
	case nbNETPDL_ID_LOOPCTRL_CONTINUE:
		m_CodeGen->ContinueStatement();
		break;
	default:
		this->GenerateWarning(string("'loopctrl' type not supported"), __FILE__, __FUNCTION__, __LINE__, 1, loopCtrlElement->NestingLevel);
		break;
	}
}


/*****************************************************************************/
/**** Parsing if     *********************************************************/
void PDLParser::ParseIf(_nbNetPDLElementIf *ifElement)
{
	this->m_CodeGen->CommentStatement(string("IF '").append(ifElement->ExprString).append("'"));

	HIRNode *expr = ParseExpressionInt(ifElement->ExprTree);

	if (expr == NULL)
	{
#ifdef STATISTICS
		this->m_CurrProtoSym->UnsupportedExpr++;
#endif
		if (this->m_IsEncapsulation==false)
			this->m_CurrProtoSym->IsSupported=false;

		this->GenerateWarning(string("'if' element needs a valid boolean expression"), __FILE__, __FUNCTION__, __LINE__, 1, ifElement->NestingLevel);

		this->m_CodeGen->CommentStatement(string("END IF '").append(ifElement->ExprString).append("' (ERROR)"));
		return;
	}

	if (ifElement->FirstValidChildIfTrue!=NULL)
	{
		StmtIf *ifStmt = m_CodeGen->IfStatement(expr);

		//EnterFldScope();
		HIRCodeGen branchCodeGen(m_GlobalSymbols, ifStmt->TrueBlock->Code);
		ChangeCodeGen(branchCodeGen);

		// parse and generate code for true branch
		this->m_CodeGen->CommentStatement(string("IF: TRUE"));
		ParseElements(ifElement->FirstValidChildIfTrue);

		RestoreCodeGen();
		//ExitFldScope();

		if (ifElement->FirstValidChildIfFalse)
		{
			// parse and generate code for false branch
			//EnterFldScope();
			HIRCodeGen branchCodeGen(m_GlobalSymbols, ifStmt->FalseBlock->Code);
			ChangeCodeGen(branchCodeGen);

			this->m_CodeGen->CommentStatement(string("IF: FALSE"));

			ParseElements(ifElement->FirstValidChildIfFalse);
			RestoreCodeGen();
			//ExitFldScope();
		}

		this->m_CodeGen->CommentStatement(string("END IF"));
	}
	else
	{
		switch (ifElement->ExprTree->Type)
		{
		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE:
		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE:
			m_CodeGen->GenStatement(expr);
			break;
		default:
			this->GenerateWarning(string("'if' element should specify an 'if-true' branch."), __FILE__, __FUNCTION__, __LINE__, 1, ifElement->NestingLevel);
			this->m_CodeGen->CommentStatement(string("ERROR: if statement without true branch; the boolean expression is not conformed as statement"));
			break;
		}
	}
}


/*****************************************************************************/
/**** Parsing switch     *****************************************************/

void PDLParser::ParseSwitch(_nbNetPDLElementSwitch *switchElement)
{
	_nbNetPDLElementCase *caseElement = switchElement->FirstCase;
	if (caseElement == NULL)
	{
		this->GenerateWarning(string("'switch' element without a case list"), __FILE__, __FUNCTION__, __LINE__, 1, switchElement->NestingLevel);
		if (switchElement->DefaultCase == NULL)
		{
			this->GenerateWarning(string("Empty 'switch' statement"), __FILE__, __FUNCTION__, __LINE__, 1, switchElement->NestingLevel);
		}

		// [ds] in any case, don't create any code
		return;
	}

	string switchID = "tmp_" + m_CurrProtoSym->Name + "_" + int2str(m_SwCount++, 10);

	this->m_CodeGen->CommentStatement(string("SWITCH '").append(switchElement->ExprString).append("'"));

	HIRNode *switchExpr = ParseExpressionInt(switchElement->ExprTree);

	if (switchExpr==NULL)
	{
		if (this->m_IsEncapsulation==false)
			this->m_CurrProtoSym->IsSupported=false;
#ifdef STATISTICS
		this->m_CurrProtoSym->UnsupportedExpr++;
#endif
	}

	SymbolLabel *swExit = m_CodeGen->NewLabel(LBL_LINKED);
	SymbolLabel *swStart(0);
	SymbolVarInt *swTemp(0);

	//First generate the code for the cases with ranged values
	while (caseElement)
	{	
		if (caseElement->FirstChild == 0)
		{
			caseElement = caseElement->NextCase;
			continue;
		}

		if (caseElement->ValueString)
		{
			m_SwCount--;
			return ParseSwitchOnStrings(switchElement);
		}

		if (caseElement->ValueMaxNumber)
		{
			if (swStart == NULL)
				swStart = m_CodeGen->NewLabel(LBL_LINKED);

			/*	if at least one case has a range, generate the code:

			temp = switchExpression

			then for each case with a range:

			if ((temp >= lowerLimit) && (temp <= upperLimit))
			{
			...code for the current case...
			}
			goto SWITCH_EXIT;

			....
			....
			SWITCH_EXIT:
			....

			*/
			//generate: switch_N_tmp = switchExpression
			if (swTemp == NULL)
			{
				swTemp = m_CodeGen->NewIntVar(switchID);
				HIRNode *tempNode = m_CodeGen->TermNode(IR_IVAR, swTemp);
				m_CodeGen->GenStatement(m_CodeGen->BinOp(IR_ASGNI, tempNode, switchExpr));
			}
			Symbol *lowerLimit = m_CodeGen->ConstIntSymbol(caseElement->ValueNumber);
			Symbol *upperLimit = m_CodeGen->ConstIntSymbol(caseElement->ValueMaxNumber);
			HIRNode *geExpr = m_CodeGen->BinOp(IR_GEI, m_CodeGen->TermNode(IR_IVAR, swTemp), m_CodeGen->TermNode(IR_ICONST, lowerLimit));
			HIRNode *leExpr = m_CodeGen->BinOp(IR_LEI, m_CodeGen->TermNode(IR_IVAR, swTemp), m_CodeGen->TermNode(IR_ICONST, upperLimit));
			HIRNode *expr = m_CodeGen->BinOp(IR_ANDB, geExpr, leExpr);

			StmtIf *ifStmt = m_CodeGen->IfStatement(expr);
			//code for inner elements
			//EnterFldScope();
			HIRCodeGen trueBrCodeGen(m_GlobalSymbols, ifStmt->TrueBlock->Code);
			ChangeCodeGen(trueBrCodeGen);
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, caseElement->FirstChild));

			trueBrCodeGen.JumpStatement(swExit);
			RestoreCodeGen();
			//ExitFldScope();
			HIRCodeGen falseBrCodeGen(m_GlobalSymbols, ifStmt->FalseBlock->Code);
			falseBrCodeGen.JumpStatement(swStart);
		}
	
		caseElement = caseElement->NextCase;
	}

	//if we already have a temp holding the value of the switch expression we'll use it
	if (swTemp != NULL)
		switchExpr = m_CodeGen->TermNode(IR_IVAR, swTemp);

	if (swStart != NULL)
		m_CodeGen->LabelStatement(swStart);

	HIRStmtSwitch *swStmt = m_CodeGen->SwitchStatement(switchExpr);
	swStmt->SwExit = swExit;

	//m_CodeGen->LabelStatement(swExit);
	//then generate the ordinary cases
	caseElement = switchElement->FirstCase;

#define OPTIMIZE_SWITCHES
#ifdef OPTIMIZE_SWITCHES
	list<_nbNetPDLElementCase *> sortedCases(0);

	while (caseElement)
	{
		if (caseElement->FirstChild != 0 && !caseElement->ValueMaxNumber)
			sortedCases.push_back(caseElement);

		caseElement = caseElement->NextCase;
	}

	SwitchCaseLess sw_case_less;
	sortedCases.sort(sw_case_less);

	list<_nbNetPDLElementCase *>::iterator i=sortedCases.begin();
	while (i!=sortedCases.end())
#else
	while (caseElement)
#endif
	{
#ifdef OPTIMIZE_SWITCHES
		_nbNetPDLElementCase *caseElement = (_nbNetPDLElementCase *)*i;
#endif

		if (caseElement->FirstChild != 0 && !caseElement->ValueMaxNumber)
		{
			HIRNode *constVal = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(caseElement->ValueNumber));
			StmtCase *caseStmt = m_CodeGen->CaseStatement(swStmt, constVal);
			HIRCodeGen caseCodeGen(m_GlobalSymbols, caseStmt->Code->Code);
			//generate the code for the current case
			//EnterFldScope();
			ChangeCodeGen(caseCodeGen);

#ifdef OPTIMIZE_SWITCHES
			while (i!=sortedCases.end() && ((_nbNetPDLElementCase *)*i)->ValueNumber == caseElement->ValueNumber)
			{
				if (*i != caseElement)
					this->m_CodeGen->CommentStatement(string("Merged cases with the same condition"));
				ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, ((_nbNetPDLElementCase *)*i)->FirstChild));
				i++;
			}
#else
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, caseElement->FirstChild));
#endif

			RestoreCodeGen();
			//ExitFldScope();
			swStmt->NumCases++;

		}

#ifndef OPTIMIZE_SWITCHES
		caseElement = caseElement->NextCase;
#endif
	}

	if (switchElement->DefaultCase)
	{
		StmtCase *defaultCaseStmt = m_CodeGen->DefaultStatement(swStmt);
		HIRCodeGen caseCodeGen(m_GlobalSymbols, defaultCaseStmt->Code->Code);
		//generate the code for the current case
		//EnterFldScope();
		ChangeCodeGen(caseCodeGen);
		ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, switchElement->DefaultCase->FirstChild));
		RestoreCodeGen();
		//ExitFldScope();
	}
	else
	{
		// if we are in the encapsulation section,
		// switches should go on when an explicit default is defined
		swStmt->ForceDefault=!m_IsEncapsulation;
	}

	this->m_CodeGen->CommentStatement(string("END SWITCH"));
}

//Ivano
/*
*	This SWITCH is actually lowered as an IF using the following two methods
*/
void PDLParser::ParseSwitchOnStrings(_nbNetPDLElementSwitch *switchElement)
{
	/*
	*	switch(extractstring(...))
	*	{
	*		case '...':
	*			do something
	*		default:
	*			do something other
	*	}
	*
	*			->
	*
	*	if(extractstring())
	*	{
	*		if(stringmatching(...)
	*			do something
	*		else
	*			do something other
	*	}
	*	
	*/

	HIRNode *expr = ParseExpressionStr(switchElement->ExprTree);		
	StmtIf *ifStmt = m_CodeGen->IfStatement(expr);
	HIRCodeGen branchCodeGen(m_GlobalSymbols, ifStmt->TrueBlock->Code);
		
	ChangeCodeGen(branchCodeGen);
	ParseCaseOnStrings(switchElement->FirstCase,switchElement);
	RestoreCodeGen();
	
	this->m_CodeGen->CommentStatement(string("END SWITCH"));
}

void PDLParser::ParseCaseOnStrings(_nbNetPDLElementCase *caseElement, _nbNetPDLElementSwitch *switchElement)
{
	if (caseElement->FirstChild == 0)
	{
		return ParseCaseOnStrings(caseElement->NextCase,switchElement);
	}
	
	if(!caseElement->ValueString)
	{
		this->GenerateError(string("Case Element that").append(" should evalueate a string, evaluates a number!"),__FILE__, __FUNCTION__, __LINE__, 1, 4);
		return;
	}
	
	stringstream ss;
	ss << caseElement->ValueString;
	this->m_CodeGen->CommentStatement(string("CASE '").append(ss.str()).append("'"));
	

	//create the expression to evaluate
	HIRNode *buffer = m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym);
	SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen->ConstStrSymbol(ss.str()); 
	SymbolRegExPDL *regExpSym = new SymbolRegExPDL(buffer,pattern,true,m_GlobalSymbols.GetStringMatchingEntriesCount());  //case sensitive
	m_GlobalSymbols.StoreStringMatchingEntry(regExpSym,m_CurrProtoSym);
	HIRNode *expr = m_CodeGen->TermNode(IR_STRINGMATCHINGFND, regExpSym);
	
	StmtIf *ifStmt = m_CodeGen->IfStatement(expr);
	HIRCodeGen branchCodeGenTrue(m_GlobalSymbols, ifStmt->TrueBlock->Code);
	
	ChangeCodeGen(branchCodeGenTrue);
	this->m_CodeGen->CommentStatement(string("IF: TRUE - CASE: MATCHED"));
	
	ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, caseElement->FirstChild));
	
	RestoreCodeGen();
	
	if(caseElement->NextCase || switchElement->DefaultCase)
	{
	
		HIRCodeGen branchCodeGenFalse(m_GlobalSymbols, ifStmt->FalseBlock->Code);
		ChangeCodeGen(branchCodeGenFalse);
		this->m_CodeGen->CommentStatement(string("IF: FALSE - CASE: NOT MATCHED"));	
	
		if(caseElement->NextCase)
			ParseCaseOnStrings(caseElement->NextCase,switchElement);
		else if(switchElement->DefaultCase)
			ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, switchElement->DefaultCase->FirstChild));
		
		RestoreCodeGen();
	}
}

/*****************************************************************************/
/**** Parsing protocols     **************************************************/
void PDLParser::ParseStartProto(HIRCodeGen &IRCodeGen, bool visitEncap)				// visitencap ALWAYS true!!
{
	
	ParseProtocol(*m_GlobalInfo->GetStartProtoSym(), IRCodeGen, visitEncap);

	m_GlobalSymbols.CurrentOffsSym = (SymbolVarInt*)m_GlobalSymbols.LookUpVariable(string("$currentoffset"));
	if (m_GlobalSymbols.CurrentOffsSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "variable $currentoffset not found!!!");

	m_GlobalSymbols.PacketBuffSym = (SymbolVarBufRef*)m_GlobalSymbols.LookUpVariable(string("$packet"));
	if (m_GlobalSymbols.PacketBuffSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "variable $packet not found!!!");
}

void PDLParser::ParseProtocol(SymbolProto &protoSymbol, HIRCodeGen &irCodeGen, bool visitEncap)
{
	m_CurrProtoSym = &protoSymbol;
	
	this->GenerateInfo(string("Parsing protocol '").append(protoSymbol.Name).append("'"),
		__FILE__, __FUNCTION__, __LINE__, 1, 2);

	m_CodeGen = &irCodeGen;
	//m_FieldScopes = &(m_GlobalSymbols.GetProtoFieldScopes(&protoSymbol));
	//EnterFldScope();

#ifdef ENABLE_FIELD_OPT
	#ifdef ENABLE_FIELD_DEBUG
		ofstream my_ir("Debug_Parse_field_opt.txt",ios::app);
		my_ir <<" ******************* (PDLPARSER) **********************"<<endl;
		my_ir <<" FORMAT START HERE: "<<endl;
	#endif
#endif
	//Generate the default labels and the first statement of the protocol
	GenProtoEntryCode(protoSymbol);			//[mligios] ADD STATICHE NEL FORMAT DOVRANNO ESSERE CONTROLLATE QUA...?


	_nbNetPDLElementProto *protoElement = protoSymbol.Info;
	
	if (protoElement->FirstExecuteInit && &protoSymbol == m_GlobalInfo->GetStartProtoSym())
	{
		this->GenerateInfo(string("Parsing 'init' section"), __FILE__, __FUNCTION__, __LINE__, 2, 3);
		m_CodeGen->CommentStatement("Init section");
		ParseExecSection(protoElement->FirstExecuteInit);

		SymbolVariable *var = m_CodeGen->NewIntVar("$resolved_candidate_id", VAR_VALID_THISPKT);
		uint32 initValue = 0;
		HIRNode *expression = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(initValue));
		HIRNode *varNode = m_CodeGen->TermNode(IR_IVAR, var);
		m_CodeGen->GenStatement(m_CodeGen->BinOp(IR_DEFVARI, varNode, expression));
	}


	m_notyvisit=true;  

	
	/*
	if (protoElement->FirstExecuteVerify)
	ParseVerify(protoElement->FirstExecuteVerify);

	if (protoElement->FirstExecuteInit)
	ParseExecSection(protoElement->FirstExecuteInit);
	*/


	protoSymbol.FirstExecuteBefore = protoElement->FirstExecuteBefore;

	//ADD & UPDATE OTHERS POINTERS??!?!?!
	protoSymbol.FirstExecuteVerify = protoElement->FirstExecuteVerify;
	protoSymbol.FirstExecuteInit = protoElement->FirstExecuteInit;
	protoSymbol.FirstExecuteAfter = protoElement->FirstExecuteAfter;
	protoSymbol.FirstField = protoElement->FirstField;
	protoSymbol.FirstEncapsulationItem = protoElement->FirstEncapsulationItem;
	//END OF ADD & UPDATE OTHERS POINTERS!!


	this->GenerateInfo(string("Parsing fields"), __FILE__, __FUNCTION__, __LINE__, 2, 3);
	// Reset the not defined flag for inner elements
	this->m_InnerElementsNotDefined=false;

	ParseElements(protoElement->FirstField);		//[mligios]//! Pointer to the first field that has to be decoded.

	// Reset the field union
	this->m_ParentUnionField=NULL;

	// Set the protocol flag if some inner elements has not been defined
	protoSymbol.IsDefined=!this->m_InnerElementsNotDefined;

	if (protoSymbol.IsDefined==false || protoSymbol.IsSupported==false)
	{
		// if the protocol cannot be defined, stops parsing here
		if (m_GlobalInfo->Debugging.DebugLevel > 1)
			GenerateWarning(string("Protocol '").append(protoSymbol.Name).append("' is not supported, and it will not be used."), __FILE__, __FUNCTION__, __LINE__, 0, 3);
		return;
	}

	if (protoElement->FirstExecuteAfter)
	{
		this->GenerateInfo(string("Parsing 'execute' (after) section"), __FILE__, __FUNCTION__, __LINE__, 2, 3);
		m_CodeGen->CommentStatement("Execute after section");
		ParseExecSection(protoElement->FirstExecuteAfter);				
	}
	
	#ifdef ENABLE_FIELD_OPT
		#ifdef ENABLE_FIELD_DEBUG
			my_ir <<" FORMAT && AFTER END HERE (PDLPARSER) -> "<<protoElement->Name<<endl;
		#endif
	#endif

	//m_CurrentParsingSection = VISITING_SECTION_ENCAP;

	if (visitEncap)			//visitEncap... always true... real need? pdlparser.cpp -> parseprotocol()
	{
		if (protoElement->FirstEncapsulationItem){
			VisitEncapsulation(protoElement->FirstEncapsulationItem);		
		}
	}

	#ifdef ENABLE_FIELD_OPT
	{
		#ifdef ENABLE_FIELD_DEBUG
			my_ir << "**************       FAST VISIT     *********************" <<endl;
			my_ir << " Protocol -  "<<protoSymbol.Name<<endl;
		#endif
		//Fast visit of Verify section
		if(protoElement->FirstExecuteVerify)
		{
			this->GenerateInfo(string("Visiting Verify section"),__FILE__, __FUNCTION__, __LINE__, 2, protoElement->FirstExecuteVerify->NestingLevel);	
			#ifdef ENABLE_FIELD_DEBUG
				my_ir << " Verify Section! Protocol -  "<<protoSymbol.Name<<endl;
			#endif
			VisitElements((_nbNetPDLElementBase *)protoElement->FirstExecuteVerify);
		}

		//Fast visit of Before section
		if(protoElement->FirstExecuteBefore)
		{
			this->GenerateInfo(string("Visiting Before section"),__FILE__, __FUNCTION__, __LINE__, 2, protoElement->FirstExecuteBefore->NestingLevel);
			#ifdef ENABLE_FIELD_DEBUG
				my_ir << " Before Section! Protocol -  "<<protoSymbol.Name<<endl;
			#endif
			VisitElements((_nbNetPDLElementBase *)protoElement->FirstExecuteBefore);
		}
		#ifdef ENABLE_FIELD_DEBUG
			my_ir << "*************** END OF FAST VISIT ****************"<<endl<<endl;
			my_ir.close();
		#endif
	}
	#endif

	m_notyvisit=false; 


//end ---------------------------------------
	m_CodeGen = NULL;
	//m_FieldScopes = NULL;

	this->GenerateInfo(string("Parsed protocol '").append(protoSymbol.Name).append("'"),__FILE__, __FUNCTION__, __LINE__, 2, 3);	
}

//**********************************************************************************************************************************//
void PDLParser::ParseNextProto(_nbNetPDLElementNextProto *element)
{
	char *protoName = GetProtoName(element->ExprTree);

	if (protoName!=NULL)
	{
		if(m_Preferred)
		{
			/*
			* We are using the preferred peg, then we avoid to create those 'jump' to protocols that are not into the preferred peg.
			* We perform this check only for the "direct jumps", i.e. those having a protocol id into the attribute 'proto'. The others
			* are jumps to protocols stored into a lookup table and then, if a protocol is into the table, he was reached, and hence it
			* is into the peg.
			*/	
			bool preferred = false;
			if(IsPreferred(element->ExprPreferredTree))
				preferred = IsPreferred(element->ExprPreferredTree);
			if (!preferred)
				return;
		}
	
		SymbolProto *nextPSym = m_GlobalSymbols.LookUpProto(string(protoName));

		if (nextPSym == NULL)
		{
			this->GenerateWarning(string("The specified next protocol is unknown"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}

		if (nextPSym->VerifySectionSupported==false)
		{
			this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid verify section"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}

		if (nextPSym->BeforeSectionSupported==false)
		{
			this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid before section"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}

#ifdef VISIT_NEXT_PROTO_DEFINED_ONLY
		if (nextPSym->IsDefined==false || nextPSym->IsSupported==false)
		{
			this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' is not supported"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}
#endif

//		cout<<"NEXTPROTO PARSED: "<<protoName<<endl;
        // is nextPSym a protocol that is specified by a non-complemented-set transition exiting from the active state?
		std::map <string, SymbolLabel*>::iterator iter = m_toStateProtoMap.find(nextPSym->Name);
		if (iter != m_toStateProtoMap.end())
		{
			//	cout<<"INSIDE IF -> (se noEmpty-data) -> check framelength ( "<<nextPSym->Name<<" ) "<<endl;	
//				if(nextPSym->Name == "tcp")
	//			{
	//					m_CodeGen->CommentStatement("TCP->JUMP recognized");
		//				m_CodeGen->CommentStatement("-length-X");
	//			}
						
				// .. it is!
				m_CodeGen->JumpStatement((*iter).second);
    	}
    	else
		{
		//cout<<"INSIDE ELSE "<<endl;
			std::map <string, SymbolLabel*>::iterator it = m_toStateProtoMap.find(LAST_RESORT_JUMP);

//			m_CodeGen->CommentStatement("TCP->JUMP? recognized");
	//		m_CodeGen->CommentStatement("-length-X?");

			if(it != m_toStateProtoMap.end())
				m_CodeGen->JumpStatement((*it).second);
			//the IF could be not executed in case of fieldextraction, and happens because it is possible that there is not the final not accepting state
		}
//		cout<<"NEXTPROTO ENDED"<<endl;
	}
	else
	{

		// try to solve the indirect reference
		m_CodeGen->CommentStatement("Indirect protocol reference");
	
		HIRNode *indirectProtoRef = ParseExpressionInt(element->ExprTree);
		nbASSERT(indirectProtoRef!=NULL, "Indirect protocol reference unknown");


//MY ADD
if(false)
{
			m_CodeGen->CommentStatement("-NOACK-");				//here before jump to res_candidate -> http
			HIRNode *pktLengthEqZero = m_CodeGen->BinOp(IR_SUBI,
				m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")),
				m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym));

			StmtIf *ifPayload = m_CodeGen->IfStatement(m_CodeGen->BinOp(IR_GTI, pktLengthEqZero, m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(0))));

			HIRCodeGen ifPayloadCode(m_GlobalSymbols, ifPayload->TrueBlock->Code);
			ChangeCodeGen(ifPayloadCode);
}
//STOP
		SymbolVariable *candidate = m_GlobalSymbols.LookUpVariable(string("$resolved_candidate_id"));
		
		nbASSERT(candidate != NULL, "This reference cannot be NULL");
		
		m_CodeGen->GenStatement(
			m_CodeGen->BinOp(
			IR_ASGNI,
			m_CodeGen->TermNode(IR_IVAR, candidate),
			indirectProtoRef));


		m_CodeGen->JumpStatement(m_ResolveCandidateLabel);

//MY_ADD
if(false)
{
	RestoreCodeGen();  
}

	}
}

void PDLParser::ParseNextProtoCandidate(_nbNetPDLElementNextProto *element)
{
	char *protoName = GetProtoName(element->ExprTree);
	std::map <string, SymbolLabel*>::iterator iter = m_toStateProtoMap.find(protoName);
	
	if (iter == m_toStateProtoMap.end())
		return;//we avoid the generation of the verify for a useless protocol, i.e. a protocol which leads to fail
	
	nbASSERT(protoName!=NULL, "Reference to a candidate protocol cannot be indirect");
	SymbolProto *nextPSym = m_GlobalSymbols.LookUpProto(protoName);

	if (nextPSym == NULL)
	{
		this->GenerateWarning(string("The specified next protocol is unknown"),
			__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
		return;
	}

	if (nextPSym->VerifySectionSupported==false)
	{
		this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid verify section"),
			__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
		return;
	}

	if (nextPSym->BeforeSectionSupported==false)
	{
		this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid before section"),
			__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
		return;
	}
	if (nextPSym->Info->FirstExecuteVerify)
	{
		{
#ifdef VERIFY_ONLY_WITH_PAYLOAD
/*
			cout<<"PAYLOAD _ CANDIDATE - "<<protoName<<endl;	
			cout<<"packetlength: "<<((SymbolVarInt *)(m_GlobalSymbols.LookUpVariable("$packetlength")))->Temp->Temp<<endl;
			cout<<"framelength: "<<((SymbolVarInt *)(m_GlobalSymbols.LookUpVariable("$framelength")))->Temp->Temp<<endl;									
*/
			
			HIRNode *pktLengthEqZero = m_CodeGen->BinOp(IR_EQI,
				m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")),
				m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(0)));
			HIRNode *offLowerPBL = m_CodeGen->BinOp(IR_LTI,
				m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym),
				m_CodeGen->TermNode(IR_IVAR,m_CodeGen->NewIntVar("$pbl")));
			HIRNode *offLowerPktLength = m_CodeGen->BinOp(IR_LTI,
				m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym),
				m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")));

//HERE!!!???

			StmtIf *ifPayload = m_CodeGen->IfStatement(
				m_CodeGen->BinOp(IR_ORB, m_CodeGen->BinOp(IR_ANDB, pktLengthEqZero, offLowerPBL), offLowerPktLength));

			HIRCodeGen ifPayloadCode(m_GlobalSymbols, ifPayload->TrueBlock->Code);
			ChangeCodeGen(ifPayloadCode);
#endif
			SymbolProto * aux = m_CurrProtoSym;
			m_CurrProtoSym = nextPSym;
			m_CodeGen->CommentStatement(string("Verify protocol ").append(nextPSym->Name));
			ParseVerify(nextPSym->Info->FirstExecuteVerify);
			m_CurrProtoSym = aux;

			bool found=false;
			for (list<int>::iterator i=m_CandidateProtoToResolve.begin(); i!=m_CandidateProtoToResolve.end(); i++)
			{
				if ( (uint32) (*i) == nextPSym->ID )
				{
					found=true;
				}
			}

			if (!found)
				m_CandidateProtoToResolve.push_back(nextPSym->ID);

			CheckVerifyResult(nextPSym->ID);

#ifdef VERIFY_ONLY_WITH_PAYLOAD
			RestoreCodeGen();
#endif
		}
	}
	else
	{
		ParseNextProto(element);
	}
}

void PDLParser::CheckVerifyResult(uint32 nextProtoID)
{
	SymbolVariable *protoVerifyResult = m_GlobalSymbols.LookUpVariable(string("$protoverify_result"));

	HIRNode *protoFound = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(NETPDL_PROTOVERIFYRESULT_FOUND));
	HIRNode *protoCandidate = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(NETPDL_PROTOVERIFYRESULT_CANDIDATE));
	HIRNode *protoDeferred = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(NETPDL_PROTOVERIFYRESULT_DEFERRED));
	HIRNode *checkFound = m_CodeGen->BinOp(IR_EQI, m_CodeGen->TermNode(IR_IVAR, protoVerifyResult), protoFound);
	HIRNode *checkCandidate = m_CodeGen->BinOp(IR_EQI, m_CodeGen->TermNode(IR_IVAR, protoVerifyResult), protoCandidate);
	HIRNode *checkDeferred = m_CodeGen->BinOp(IR_EQI, m_CodeGen->TermNode(IR_IVAR, protoVerifyResult), protoDeferred);

	HIRNode *exprCandidate = m_CodeGen->BinOp(IR_ORB, m_CodeGen->BinOp(IR_ORB, checkFound, checkCandidate), checkDeferred);

	HIRNode *valueFound = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(NETPDL_PROTOVERIFYRESULT_FOUND));
	HIRNode *exprFound = m_CodeGen->BinOp(IR_EQI, m_CodeGen->TermNode(IR_IVAR, protoVerifyResult), valueFound);

	StmtIf *ifProtoCandidate = m_CodeGen->IfStatement(exprCandidate);
	{
		HIRCodeGen ifProtoCandidateCode(m_GlobalSymbols, ifProtoCandidate->TrueBlock->Code);
		ChangeCodeGen(ifProtoCandidateCode);

		// if the protocol is candidate...
		StmtIf *ifProtoFound = m_CodeGen->IfStatement(exprFound);
		{
			HIRCodeGen ifProtoFoundCode(m_GlobalSymbols, ifProtoFound->TrueBlock->Code);
			ChangeCodeGen(ifProtoFoundCode);

			// if $protoverify_result == FOUND then set candidate id and go to the candidates resolution section
			SymbolVariable *candidate = m_GlobalSymbols.LookUpVariable(string("$resolved_candidate_id"));
			m_CodeGen->GenStatement(
				m_CodeGen->BinOp(
				IR_ASGNI,
				m_CodeGen->TermNode(IR_IVAR, candidate),
				m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(nextProtoID))));
			m_CodeGen->JumpStatement(m_ResolveCandidateLabel);

			RestoreCodeGen();

			HIRCodeGen ifProtoNotFoundCode(m_GlobalSymbols, ifProtoFound->FalseBlock->Code);
			ChangeCodeGen(ifProtoNotFoundCode);

			StmtIf *ifCandidateNotSet = m_CodeGen->IfStatement(m_CodeGen->BinOp(IR_EQI, m_CodeGen->TermNode(IR_IVAR, candidate), m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol((uint32)-1))));
			{
				HIRCodeGen ifCandidateNotSetCode(m_GlobalSymbols, ifCandidateNotSet->TrueBlock->Code);
				ChangeCodeGen(ifCandidateNotSetCode);

				// if $protoverify_result != FOUND then set candidate id only if it has not yet been used
				m_CodeGen->GenStatement(
					m_CodeGen->BinOp(
					IR_ASGNI,
					m_CodeGen->TermNode(IR_IVAR, candidate),
					m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(nextProtoID))));

				RestoreCodeGen();
			}

			RestoreCodeGen();
		}

		//// execute before
		//SymbolProto *nextProtoSym = m_GlobalInfo->GetProtoList()[nextProtoID];
		//ParseExecBefore(*nextProtoSym, m_FilterInfo->SubGraph, m_FilterInfo->FilterFalse, ifProtoCandidateCode);

		RestoreCodeGen();
	}
}

/*****************************************************************************/

void PDLParser::ParseBeforeSection(_nbNetPDLElementExecuteX *execSection,HIRCodeGen &irCodeGen, SymbolProto *current)
{
	m_CurrProtoSym = current;
	m_CodeGen = &irCodeGen;
	this->m_CodeGen->CommentStatement(string("PROTOCOL ").append(current->Name).append(": BEFORE"));	
	ParseExecSection(execSection);
}

/**** Parsing execution sections *****************************************/
void PDLParser::ParseExecSection(_nbNetPDLElementExecuteX *execSection)
{
	// SymbolLabel *label1(0);

	while (execSection)
	{
		if (execSection->WhenExprTree)
		{		
			this->m_CodeGen->CommentStatement(string("WHEN '").append(execSection->WhenExprString).append("'"));
			HIRNode *expr = ParseExpressionInt(execSection->WhenExprTree);
			if (expr == NULL)
			{
				this->GenerateWarning(string("'when' condition needs a valid boolean expression"),
					__FILE__, __FUNCTION__, __LINE__, 1, execSection->NestingLevel);
				this->m_CodeGen->CommentStatement(string("END WHEN (ERROR)"));
				return;
			}
			/*

			it generates:

			cmp (!expr) goto if_N_false
			;code for the current section
			if_N_false:
			...

			*/	
		
			StmtIf *ifStmt = m_CodeGen->IfStatement(expr);
			HIRCodeGen trueBrCodeGen(m_GlobalSymbols, ifStmt->TrueBlock->Code);
			ChangeCodeGen(trueBrCodeGen);
		}


		ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, execSection->FirstChild));

		if (execSection->WhenExprTree)
		{
			RestoreCodeGen();
			this->m_CodeGen->CommentStatement(string("END WHEN"));
		}

			
		//go to next section
		execSection = execSection->NextExecuteElement;
	}
	
}

void PDLParser::ParseVerify(_nbNetPDLElementExecuteX *verifySection)
{
	while (verifySection)
	{
		ParseElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, verifySection->FirstChild)); 
		//go to next section
		verifySection = verifySection->NextExecuteElement;
	}
}

/*****************************************************************************/
/**** Parsing encapsulations     *********************************************/

/* The defaultBehav parameter states what should be done if no encapsulated protocol matches.
 * If true, go to filterTrue, else go to filterFalse.
 */
void PDLParser::ParseEncapsulation(SymbolProto *protoSymbol, std::map <string, SymbolLabel*> toStateProtoMap,
                                   EncapFSA *fsa, bool defaultBehav, SymbolLabel *filterFalse, SymbolLabel *filterTrue, HIRCodeGen &irCodeGen, bool preferred)
{
	m_Preferred = preferred;
	m_CurrProtoSym = protoSymbol;
	m_IsEncapsulation = true;
	m_CodeGen = &irCodeGen;
	m_toStateProtoMap = toStateProtoMap;

	m_CodeGen->CommentStatement(string("PROTOCOL ").append(protoSymbol->Name).append(": ENCAPSULATION"));
	
	// reset protoverify_result
	SymbolVariable *protoVerifyResult = m_GlobalSymbols.LookUpVariable(string("$protoverify_result"));
	m_CodeGen->GenStatement(
		m_CodeGen->BinOp(
		IR_ASGNI,
		m_CodeGen->TermNode(IR_IVAR, protoVerifyResult),
		m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(NETPDL_PROTOVERIFYRESULT_NOTFOUND))));

	m_ResolveCandidateLabel=m_CodeGen->NewLabel(LBL_CODE, string("res_candidate_").append(protoSymbol->Name)); //etichetta "res_candidate_ethIpTcp_lx
	m_CandidateProtoToResolve.clear();

//
//#ifdef ENCAP_ONLY_WITH_PAYLOAD
//	HIRNode *pktLengthEqZero = m_CodeGen->BinOp(IR_EQI,
//		m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")),
//		m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(0)));
//	HIRNode *offLowerPBL = m_CodeGen->BinOp(IR_LTI,
//		m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym),
//		m_CodeGen->TermNode(IR_IVAR,m_CodeGen->NewIntVar("$pbl")));
//	HIRNode *offLowerPktLength = m_CodeGen->BinOp(IR_LTI,
//		m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym),
//		m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")));
//
//	StmtIf *ifPayload = m_CodeGen->IfStatement(
//		m_CodeGen->BinOp(IR_ORB, m_CodeGen->BinOp(IR_ANDB, pktLengthEqZero, offLowerPBL), offLowerPktLength));
//
//	HIRCodeGen ifPayloadCode(m_GlobalSymbols, ifPayload->TrueBlock->Code);
//	ChangeCodeGen(ifPayloadCode);
//#endif
//

	m_CodeGen->GenStatement(m_CodeGen->BinOp(IR_ASGNI,
		m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$resolved_candidate_id")),
		m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol((uint32)-1))));

	//m_FieldScopes = &(m_GlobalSymbols.GetProtoFieldScopes(&protoSymbol));
	//EnterFldScope();

	FilterInfo filterInfo(filterFalse, filterTrue, *fsa);
	m_FilterInfo = &filterInfo;

	//cerr << "Parsing Encapsulation Section for Protocol " << protoSymbol.Name << endl;

	_nbNetPDLElementProto *protoElement = protoSymbol->Info;

	if (protoElement->FirstEncapsulationItem){
		ParseElements(protoElement->FirstEncapsulationItem); 
	}

	m_CodeGen->LabelStatement(m_ResolveCandidateLabel);//qui mette lalabel "res_candidate_ethIpTcp_lx

	if (m_CandidateProtoToResolve.size()>0)
	{
		
		string switchID = "tmp_" + m_CurrProtoSym->Name + "_" + int2str(m_SwCount++, 10);

		this->m_CodeGen->CommentStatement(string("SWITCH '$resolved_candidate'"));

		HIRNode *switchExpr = m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$resolved_candidate_id"));

		SymbolLabel *swExit = m_CodeGen->NewLabel(LBL_LINKED);
		SymbolLabel *swStart(0);
		SymbolVarInt *swTemp(0);

		//if we already have a temp holding the value of the switch expression we'll use it
		if (swTemp != NULL)
			switchExpr = m_CodeGen->TermNode(IR_IVAR, swTemp);

		if (swStart != NULL)
			m_CodeGen->LabelStatement(swStart);

		HIRStmtSwitch *swStmt = m_CodeGen->SwitchStatement(switchExpr);
		swStmt->SwExit = swExit;

		m_CandidateProtoToResolve.sort(less<int>());
		for (list<int>::iterator i=m_CandidateProtoToResolve.begin(); i!=m_CandidateProtoToResolve.end(); i++)
		{
			HIRNode *constVal = m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol( (*i) ));
			StmtCase *caseStmt = m_CodeGen->CaseStatement(swStmt, constVal); //crea il case statement strano
			HIRCodeGen caseCodeGen(m_GlobalSymbols, caseStmt->Code->Code);

			//generate the code for the current case
			ChangeCodeGen(caseCodeGen);

			SymbolProto *nextPSym = m_GlobalSymbols.GetProto(*i);
			
			if (nextPSym == NULL)
			{
				this->GenerateWarning(string("The specified next protocol is unknown"),
					__FILE__, __FUNCTION__, __LINE__, 1, 2);
				return;
			}

			if (nextPSym->VerifySectionSupported==false)
			{
				this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid verify section"),
					__FILE__, __FUNCTION__, __LINE__, 1, 2);
				return;
			}

			if (nextPSym->BeforeSectionSupported==false)
			{
				this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid before section"),
					__FILE__, __FUNCTION__, __LINE__, 1, 2);
				return;
			}

#ifdef VISIT_NEXT_PROTO_DEFINED_ONLY
			if (nextPSym->IsDefined==false || nextPSym->IsSupported==false)
			{
				this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' is not supported"),
					__FILE__, __FUNCTION__, __LINE__, 1, 2);
				return;
			}
#endif

			//check if the next proto is inside the toStateProtoMap

			if (toStateProtoMap.find(nextPSym->Name) != toStateProtoMap.end())
			{	
				//[icerrato] cambiamento fatto per riconoscere il traffico http
				//m_CodeGen->JumpStatement(m_GlobalSymbols.GetProtoStartLbl(nextPSym));
				std::map <string, SymbolLabel*>::iterator iter = m_toStateProtoMap.find(nextPSym->Name);
				m_CodeGen->JumpStatement((*iter).second);
				
		 	}

			RestoreCodeGen();

			swStmt->NumCases++;
		}

		StmtCase *defaultCaseStmt = m_CodeGen->DefaultStatement(swStmt);
		HIRCodeGen caseCodeGen(m_GlobalSymbols, defaultCaseStmt->Code->Code);

		ChangeCodeGen(caseCodeGen);
	
		//Ivano: if defaultBehav is true, the we are parsing the encapsulation section
		//of an accepting state! In our automa, when an acceptin state is reached, it is 
		//impossible to reach a non accepting state. Then, now we have to jump to filterTrue
		//and accept the packet.
		if(defaultBehav)
			m_CodeGen->JumpStatement(m_FilterInfo->FilterTrue);
		else
			m_CodeGen->JumpStatement(m_FilterInfo->FilterFalse);
	
		RestoreCodeGen();

		this->m_CodeGen->CommentStatement(string("END SWITCH")); // /*** END SWITCH ***/
	}

#ifdef ENCAP_ONLY_WITH_PAYLOAD
	RestoreCodeGen();
#endif


	// bool jumpFlag = false;
	m_CodeGen->JumpStatement(defaultBehav? filterTrue : filterFalse);


	m_CodeGen = NULL;
	m_IsEncapsulation = false;
	m_FilterInfo = NULL;
	//m_FieldScopes = NULL;

}

void PDLParser::ParseExecBefore(SymbolProto &protoSymbol, EncapFSA &fsa, SymbolLabel *filterFalse, SymbolLabel *filterTrue, HIRCodeGen &irCodeGen)
{
	SymbolProto *oldProtoSym = m_CurrProtoSym;
	HIRCodeGen *oldCodeGen = m_CodeGen;
	FilterInfo *oldFilterInfo = m_FilterInfo;
	bool oldIsEncapsulation = m_IsEncapsulation;

	m_CurrProtoSym = &protoSymbol;
	m_IsEncapsulation = false;
	m_CodeGen = &irCodeGen;

	FilterInfo filterInfo(filterFalse, filterTrue, fsa);
	m_FilterInfo = &filterInfo;

	_nbNetPDLElementProto *protoElement = protoSymbol.Info;

	if (protoElement->FirstExecuteBefore)
	{
		m_CodeGen->CommentStatement(string(protoSymbol.Name)+string(": execute-before"));

#ifdef EXEC_BEFORE_ONLY_WITH_PAYLOAD
		HIRNode *pktLengthEqZero = m_CodeGen->BinOp(IR_EQI,
			m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")),
			m_CodeGen->TermNode(IR_ICONST, m_CodeGen->ConstIntSymbol(0)));
		HIRNode *offLowerPBL = m_CodeGen->BinOp(IR_LTI,
			m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym),
			m_CodeGen->TermNode(IR_IVAR,m_CodeGen->NewIntVar("$pbl")));
		HIRNode *offLowerPktLength = m_CodeGen->BinOp(IR_LTI,
			m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym),
			m_CodeGen->TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")));

		StmtIf *ifPayload = m_CodeGen->IfStatement(
			m_CodeGen->BinOp(IR_ORB, m_CodeGen->BinOp(IR_ANDB, pktLengthEqZero, offLowerPBL), offLowerPktLength));

		HIRCodeGen ifPayloadCode(m_GlobalSymbols, ifPayload->TrueBlock->Code);
		ChangeCodeGen(ifPayloadCode);
#endif

		ParseExecSection(protoElement->FirstExecuteBefore);

#ifdef EXEC_BEFORE_ONLY_WITH_PAYLOAD
		RestoreCodeGen();
#endif
	}

	m_FilterInfo = oldFilterInfo;
	m_CurrProtoSym = oldProtoSym;
	m_CodeGen = oldCodeGen;
	m_IsEncapsulation = oldIsEncapsulation;
}


/************************************************************************************************************************************************/
//					NECESSARY TO GET HASH-MAP FROM OTHERS CLASS (e.g. FRONTEND)		 				//
/************************************************************************************************************************************************/

#ifdef ENABLE_FIELD_OPT
IntMap &PDLParser::getUsedSymbMap(){
	return m_symbolref;
}
#endif	


/************************************************************************************************************************************************/
//								PROTOCOL VISIT									//
/************************************************************************************************************************************************/
void PDLParser::VisitElements(_nbNetPDLElementBase *firstElement)
{
	_nbNetPDLElementBase *element = firstElement;

	this->GenerateInfo(string("Visiting elements in ").append(firstElement->ElementName).c_str(), __FILE__,__FUNCTION__,__LINE__,2,firstElement->NestingLevel);

	while(element)
	{
		VisitElement(element);
		element = nbNETPDL_GET_ELEMENT(m_ProtoDB, element->NextSibling);

	}
}

/************************************************************************************************************************************************/

void PDLParser::VisitElement(_nbNetPDLElementBase *element)			
{
	if (!CheckAllowedElement(element))
		return;

	switch(element->Type)
	{
	case nbNETPDL_IDEL_UPDATELOOKUPTABLE:				
	case nbNETPDL_IDEL_LOOKUPTABLE:								
	case nbNETPDL_IDEL_ASSIGNLOOKUPTABLE:
	case nbNETPDL_IDEL_LOOPCTRL:	 
	case nbNETPDL_IDEL_ASSIGNVARIABLE:
		break;

	case nbNETPDL_IDEL_INCLUDEBLK:									
		{
			_nbNetPDLElementIncludeBlk *includeBlkElem = (_nbNetPDLElementIncludeBlk*)element;
			VisitElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, includeBlkElem->IncludedBlock->FirstChild));
		}
		break;

	case nbNETPDL_IDEL_BLOCK:									
		{
			_nbNetPDLElementBlock *blockElem = (_nbNetPDLElementBlock*)element;
			VisitElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, blockElem->FirstChild));
		}
		break;
	case nbNETPDL_IDEL_LOOP:
	{
		VisitLoop((_nbNetPDLElementLoop*)element);   
		}
		break;
	case nbNETPDL_IDEL_SWITCH:
	{
		VisitSwitch((_nbNetPDLElementSwitch*)element);
		}
		break;

	case nbNETPDL_IDEL_IF:
	{
		VisitIf((_nbNetPDLElementIf*)element);
		}
		break;
	case nbNETPDL_IDEL_NEXTPROTOCANDIDATE:
	{
#ifdef STATISTICS
		this->m_CurrProtoSym->EncapTent++;
#endif
#ifdef VISIT_CANDIDATE_PROTO
		VisitNextProto((_nbNetPDLElementNextProto*)element);
#else
		this->GenerateWarning(string("Next-proto candidate instruction is not yet supported."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
#endif
		}
		break;

	case nbNETPDL_IDEL_NEXTPROTO:
		VisitNextProto((_nbNetPDLElementNextProto*)element);
		break;

	default:
		{
		this->GenerateWarning(string("Element '").append(
			string(element->ElementName)).append(
			string("' not supported in the '")).append(
			string(nbNETPDL_GET_ELEMENT(m_ProtoDB, element->Parent)->ElementName)).append(
			string("' section")),
			__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
		return;
		}
		break;
	}
}


/************************************************************************************************************************************************/
//							VISIT LOOPS										//
/************************************************************************************************************************************************/

void PDLParser::VisitLoop(_nbNetPDLElementLoop *loopElement)
{
	switch(loopElement->LoopType)
	{
		//! The loop is terminated after a given amount of decoded data
	case nbNETPDL_ID_LOOP_SIZE:
	case nbNETPDL_ID_LOOP_T2R:
	case nbNETPDL_ID_LOOP_WHILE:	
	case nbNETPDL_ID_LOOP_DOWHILE:
		{
			//need to check loop condition to find out if NetPDL-fields are used!
			VisitExpressionInt(loopElement->ExprTree);
			VisitElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, loopElement->FirstChild));		
		}break;

	default:
		this->GenerateWarning(string("'loop' type not supported"), __FILE__, __FUNCTION__, __LINE__, 1, loopElement->NestingLevel);
		break;
	}
}

/************************************************************************************************************************************************/
//						VISIT EXPRESSIONS & OPERANDS (search for Used Fields)						//
/************************************************************************************************************************************************/

void PDLParser::VisitExpressionInt(_nbNetPDLExprBase *expr)
{
	if(expr==NULL)
		return;

	if (expr->ReturnType != nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
	{
		this->GenerateWarning(string("The expression '").append(string(expr->Token)).append(string("' does not return a number, and is not supported")),__FILE__, __FUNCTION__, __LINE__, 1, 5);
		return;
	}


	// If this is not a complex expression, let's return the result right now
	if (expr->Type != nbNETPDL_ID_EXPR_OPERAND_EXPR)
	{
		VisitOperandInt(expr);
		return;
	}

	// Let's convert this pointer into an expression
	_nbNetPDLExpression *Expression= (struct _nbNetPDLExpression*) expr;

	// the second operand could be a number or not
	if (Expression->Operand2->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
	{
		// Visit the second operand
		VisitOperandInt(Expression->Operand2);
		if (Expression->Operator->OperatorType != nbNETPDL_ID_EXPR_OPER_NOT &&	Expression->Operator->OperatorType != nbNETPDL_ID_EXPR_OPER_BITWNOT)
		{
			VisitOperandInt(Expression->Operand1);
		}
	}
	else
	{
		if (Expression->Operand1==NULL)
		{
			this->GenerateWarning(string("Operand '").append(string(Expression->Operand1->Token)).append("' is not defined"),
				__FILE__, __FUNCTION__, __LINE__, 1, 5);
			return;
		}
		if (Expression->Operand2==NULL)
		{
			this->GenerateWarning(string("Operand '").append(string(Expression->Operand1->Token)).append("' is not defined"),
				__FILE__, __FUNCTION__, __LINE__, 1, 5);
			return;
		}
		if((Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_EQUAL)||(Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_NOTEQUAL))
		{
			VisitExpressionStr(Expression->Operand1);
			return;
		}
		else if((Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_GREAT)||(Expression->Operator->OperatorType==nbNETPDL_ID_EXPR_OPER_LESS))
		{
			VisitOperandStr(Expression->Operand2);
			VisitOperandStr(Expression->Operand1);
		}
		else
			nbASSERT(false,"cannot be here.. there is a bug :-(");
	}

	return;
}

/************************************************************************************************************************************************/

void PDLParser::VisitExpressionStr(_nbNetPDLExprBase *expr)
{
	if (expr == NULL)
		return;
	if (expr->ReturnType != nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
	{
		this->GenerateWarning(string("The expression '") + string(expr->Token) + string("' does not return a buffer, and is not supported"),
			__FILE__, __FUNCTION__, __LINE__, 1, 5);
		return;
	}

	// If this is not a complex expression, let's return the result right now
	if (expr->Type != nbNETPDL_ID_EXPR_OPERAND_EXPR)
	{
		VisitOperandStr(expr);
	}
	else
	{
		// cast this pointer to an expression ptr
		_nbNetPDLExpression *Expression= (struct _nbNetPDLExpression*) expr;
		VisitOperandStr(Expression->Operand1);
	}
}

/************************************************************************************************************************************************/

void PDLParser::VisitOperandStr(struct _nbNetPDLExprBase *expr)
{
	if (expr == NULL)
		return;

	switch (expr->Type)
	{
	case nbNETPDL_ID_EXPR_OPERAND_STRING:
		break;
	case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
		{
			_nbNetPDLExprVariable *operand = (_nbNetPDLExprVariable*)expr;
			SymbolVariable *varSym = (SymbolVariable*)m_GlobalSymbols.LookUpVariable(operand->Name);

			if (varSym == NULL)
			{
				this->GenerateWarning(string("Variable '") + string(operand->Name) + string("' has not been declared"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return;
			}

			if(varSym->VarType == PDL_RT_VAR_REFERENCE)
			{
				SymbolVarBufRef *bufRefVar = (SymbolVarBufRef*)varSym;
					if (string(varSym->Name).compare("$packet") == 0)
					{
						if(operand->OffsetStartAt && operand->OffsetSize)
						{
							VisitExpressionInt(operand->OffsetStartAt);
							VisitExpressionInt(operand->OffsetSize);

							bufRefVar->UsedAsArray=true;
						}		
					}
					else
					{
						if (this->m_ConvertingToInt == true) 
						{						
							bufRefVar->UsedAsInt=true;
						} else {
							bufRefVar->UsedAsString=true;
						}
					}
			}
		}break;
	case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD: 
		{
			_nbNetPDLExprFieldRef* operand = (_nbNetPDLExprFieldRef*)expr;
			SymbolField *fieldSym = LookUpFieldSym(operand->FieldName);
			
			if (fieldSym == NULL)
			{
				return;
			}
			if (fieldSym->FieldType == PDL_FIELD_PADDING)
			{
				this->GenerateWarning(string("Operand '") + fieldSym->Name + string("' of padding type is not supported"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return;
			}

		//THIS OPERAND IS A FIELD!!! Set Field Info
		#ifdef ENABLE_FIELD_OPT		
			SetFieldInfo(fieldSym);
		#endif

			if (operand->OffsetStartAt && operand->OffsetSize)
			{
				VisitExpressionInt(operand->OffsetStartAt);
//				fieldSym->UsedAsArray=true;

				VisitExpressionInt(operand->OffsetSize);
				fieldSym->UsedAsArray=true;
			}
			else if (operand->OffsetStartAt || operand->OffsetSize)
			{
				this->GenerateWarning(string("Operand used as array should specify both the starting offset and the lenght"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
				return;
			}else{
				if (this->m_ConvertingToInt == true)
				{
					fieldSym->UsedAsInt=true;
				}else
					fieldSym->UsedAsString=true;

			}
			
		}break;
	case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:							
		{
			_nbNetPDLExprLookupTable *lookup=(_nbNetPDLExprLookupTable *)expr;

			if (lookup->OffsetStartAt!=NULL && lookup->OffsetSize!=NULL)
			{
				VisitExpressionInt(lookup->OffsetStartAt);
				VisitExpressionInt(lookup->OffsetSize);
			}
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_EXPR:
			VisitExpressionStr(expr);
		break;
	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_EXTRACTSTRING:
		{
			struct _nbNetPDLExprFunctionRegExp *function = (struct _nbNetPDLExprFunctionRegExp *)expr;		
			VisitExpressionStr(function->SearchBuffer);			
		}break;

	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHANGEBYTEORDER:
		{
			struct _nbNetPDLExprFunctionChangeByteOrder* operand = (struct _nbNetPDLExprFunctionChangeByteOrder*)expr;
			VisitExpressionStr(operand->OriginalStringExpression);
		}break;
	default:
		this->GenerateWarning(string("Expression '").append(expr->Token).append("' cannot be evaluated"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
		break;
	}
	return;
}

/************************************************************************************************************************************************/

void PDLParser::VisitOperandInt(_nbNetPDLExprBase *expr)				
{
	if (expr == NULL)
		return;

	switch (expr->Type)
	{
	case nbNETPDL_ID_EXPR_OPERAND_NUMBER:
	case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
	case nbNETPDL_ID_EXPR_OPERAND_PROTOREF:
		break;

	case nbNETPDL_ID_EXPR_OPERAND_EXPR:
		 VisitExpressionInt(expr);
		break;
	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_BUF2INT:
		{
			_nbNetPDLExprFunctionBuf2Int *operand = (_nbNetPDLExprFunctionBuf2Int*) expr;
			this->m_ConvertingToInt=true;
			VisitOperandStr(operand->StringExpression);
			this->m_ConvertingToInt=false;
		}break;
	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE:				
	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE:				
		{
			_nbParamsLinkedList *paramsList = ((_nbNetPDLExprFunctionCheckUpdateLookupTable *)expr)->ParameterList;
			while (paramsList != NULL)
			{
				if (paramsList->Expression->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
					VisitExpressionStr(paramsList->Expression);
				else
					VisitExpressionInt(paramsList->Expression);
				paramsList = paramsList->NextParameter;
			}
		}break;
	case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:	
		{
			_nbNetPDLExprLookupTable *lookup=(_nbNetPDLExprLookupTable *)expr;
			if (lookup->OffsetStartAt!=NULL && lookup->OffsetSize!=NULL)
			{
				VisitExpressionInt(lookup->OffsetStartAt);
				VisitExpressionInt(lookup->OffsetSize);
			}
		}break;
	case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_HASSTRING:
		{
			struct _nbNetPDLExprFunctionRegExp *function = (struct _nbNetPDLExprFunctionRegExp *)expr;
			VisitExpressionStr(function->SearchBuffer);
		}break;

	default:
		{
			this->GenerateWarning(string("Expression of type '").append(expr->Token).append("' is not supported"),
				__FILE__, __FUNCTION__, __LINE__, 1, 5);
		}break;
	}
	return;
}

/************************************************************************************************************************************************/
//						END OF VISIT EXPRESSIONS & OPERANDS								//
/************************************************************************************************************************************************/
void PDLParser::VisitIf(_nbNetPDLElementIf *ifElement)
{
	if (ifElement->FirstValidChildIfTrue == NULL)
	{
		this->GenerateWarning(string("'if' element needs at least an 'if-true' branch"),
			__FILE__, __FUNCTION__, __LINE__, 1, ifElement->NestingLevel);
		return;
	}

	VisitExpressionInt(ifElement->ExprTree);

	VisitElements(ifElement->FirstValidChildIfTrue);

	if (ifElement->FirstValidChildIfFalse)
		VisitElements(ifElement->FirstValidChildIfFalse);
}

void PDLParser::VisitSwitch(_nbNetPDLElementSwitch *switchElement)
{
	_nbNetPDLElementCase *caseElement = switchElement->FirstCase;
	if (caseElement == NULL)
	{
		this->GenerateWarning(string("'switch' element without a case list"),
			__FILE__, __FUNCTION__, __LINE__, 1, switchElement->NestingLevel);
		return;
	}
	
	VisitExpressionInt(switchElement->ExprTree);		

	while (caseElement)
	{
		if (caseElement->ValueString)
		{
			this->GenerateWarning(string("String cases not supported"),	__FILE__, __FUNCTION__, __LINE__, 1, switchElement->NestingLevel);
		}

		VisitElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, caseElement->FirstChild));

		caseElement = caseElement->NextCase;
	}

	if (switchElement->DefaultCase)
		VisitElements(nbNETPDL_GET_ELEMENT(m_ProtoDB, switchElement->DefaultCase->FirstChild));
}

void PDLParser::VisitNextProto(_nbNetPDLElementNextProto *element)
{
#ifdef STATISTICS
	this->m_CurrProtoSym->EncapDecl++;
#endif

	char *protoName = GetProtoName(element->ExprTree);

	if (protoName!=NULL)
	{
		SymbolProto *nextPSym = m_GlobalSymbols.LookUpProto(protoName);

		if (nextPSym == NULL)
		{
			this->GenerateWarning(string("The specified next protocol is unknown"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}

		if (nextPSym->VerifySectionSupported==false)
		{
			this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid verify section"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}

		if (nextPSym->BeforeSectionSupported==false)
		{
			this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' does not defined a valid before section"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}

#ifdef VISIT_NEXT_PROTO_DEFINED_ONLY
		if (nextPSym->IsDefined==false || nextPSym->IsSupported==false)
		{
			this->GenerateWarning(string("The specified next protocol '")+nextPSym->Name+string("' is not supported"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return;
		}
#endif

		nbASSERT(m_ProtoGraph->GetNode(m_CurrProtoSym) != NULL, "NULL ProtoGraph Node");
		nbASSERT(m_ProtoGraph->GetNode(nextPSym) != NULL, "NULL ProtoGraph Node");

		//create the complete graph
		m_ProtoGraph->AddEdge(*m_ProtoGraph->GetNode(m_CurrProtoSym), *m_ProtoGraph->GetNode(nextPSym));
		
		if(IsPreferred(element->ExprPreferredTree))
			//create the preferred graph
			m_ProtoGraphPreferred->AddEdge(*m_ProtoGraphPreferred->GetNode(m_CurrProtoSym), *m_ProtoGraphPreferred->GetNode(nextPSym));

#ifdef STATISTICS
		this->m_CurrProtoSym->EncapSucc++;
#endif
	}
}

void PDLParser::VisitEncapsulation(_nbNetPDLElementBase *firstElement)
{
	this->GenerateInfo(string("Visiting encapsulation"),
		__FILE__, __FUNCTION__, __LINE__, 2, firstElement->NestingLevel);

	/*
	Only builds the protocol encapsulation graph
	*/

	m_IsEncapsulation = true;
	VisitElements(firstElement);
	m_IsEncapsulation = false;
}


/*****************************************************************************/
/*     Miscellaneous                                                         */
/*****************************************************************************/



char *PDLParser::GetProtoName(struct _nbNetPDLExprBase *expr)
{

	switch (expr->Type)
	{

	case nbNETPDL_ID_EXPR_OPERAND_PROTOREF:
		{
			_nbNetPDLExprProtoRef* protoref = (_nbNetPDLExprProtoRef*)expr;
			return protoref->ProtocolName;
		}break;
	default:
		// the protocol reference could be indirect
		return NULL;
		break;
	}
}

bool PDLParser::IsPreferred(struct _nbNetPDLExprBase *expr)
{
	if(expr==NULL)
		//the protocol encapsulation is not preferred
		return false;

	switch (expr->Type)
	{

	case nbNETPDL_ID_EXPR_OPERAND_BOOLEAN:
		{
			_nbNetPDLExprBoolean* preferred = (_nbNetPDLExprBoolean*)expr;
			return (preferred->Value==1)? true : false;
		}break;
	default:
		nbASSERT(false,"Cannot be here!");
		return false;
		break;
	}
}


bool PDLParser::CheckAllowedElement(_nbNetPDLElementBase *element)
{
	if (m_IsEncapsulation)
	{
		switch (element->Type)
		{
		case nbNETPDL_IDEL_SWITCH:
		case nbNETPDL_IDEL_IF:
		case nbNETPDL_IDEL_NEXTPROTOCANDIDATE:
		case nbNETPDL_IDEL_NEXTPROTO:
		case nbNETPDL_IDEL_ASSIGNVARIABLE:
			return true;
			break;
		default:
			this->GenerateWarning(string("Element 'encapsulation/").append(element->ElementName).append("' not allowed"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return false;
			break;
		}
	}
	else
	{
		switch(element->Type)
		{
		case nbNETPDL_IDEL_SWITCH:
		case nbNETPDL_IDEL_IF:
			if (m_ParentUnionField!=NULL && m_ParentUnionField->Name.find("bit_union_")==0)
			{
				this->GenerateWarning(string("If an implicity bit union field has not been close, you cannot use IF elements."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
				return false;
			}
			else
				return true;
			break;
		case nbNETPDL_IDEL_NEXTPROTO:
			this->GenerateWarning(string("Element '").append(nbNETPDL_GET_ELEMENT(m_ProtoDB, element->Parent)->ElementName).append("/").append(element->ElementName).append("' not allowed"),
				__FILE__, __FUNCTION__, __LINE__, 1, element->NestingLevel);
			return false;
			break;

		default:
			return true;
			break;
		}
	}
}


void PDLParser::GenerateInfo(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation) {
	if (m_GlobalInfo->Debugging.DebugLevel >= requiredDebugLevel)
	{
		if (m_CurrProtoSym != NULL)
			message=string("(")+this->m_CurrProtoSym->Name+string(") ").append(message);
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_INFO, file, function, line, indentation);
	}
}


void PDLParser::GenerateWarning(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_CurrProtoSym != NULL)
		message=string("(")+this->m_CurrProtoSym->Name+string(") ").append(message);

	if (m_GlobalInfo->Debugging.DebugLevel >= requiredDebugLevel)
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_WARNING, file, function, line, indentation);

	m_ErrorRecorder.PDLWarning(message);
}

void PDLParser::GenerateError(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_CurrProtoSym != NULL)
		message=string("(")+this->m_CurrProtoSym->Name+string(") ").append(message);

	if (m_GlobalInfo->Debugging.DebugLevel >= requiredDebugLevel)
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_ERROR, file, function, line, indentation);
	m_ErrorRecorder.PDLError(message);
}


//void DumpFieldSymbols(FILE *file, FieldSymbolTable_t &symTab, uint32 level)
//{
//	FieldSymbolTable_t::Iterator j = symTab.Begin();
//	for (uint32 i=0; i < level; i++)
//		fprintf(file, "\t");
//	fprintf(file, "level %u:\n", level);
//	for (j; j != symTab.End(); j++)
//	{
//		FieldsList_t *fieldsList = (FieldsList_t *)(*j).Item;
//		for (uint32 i=0; i < level; i++)
//			fprintf(file, "\t");
//		nbASSERT(fieldsList != NULL, "list is null");
//		SymbolField *fieldSym = *(fieldsList->begin());
//		nbASSERT(fieldSym != NULL, "field symbol is null");
//		fprintf(file, "%s\n", fieldSym->Name.c_str());
//	}
//	fflush(file);
//}
//void PDLParser::ParseVerify(_nbNetPDLElementExecuteX *verifyElem)
//{
//	nbASSERT(false, "to be completely revised");
//	return;
//#if 0
//	Node *expr = ParseExpressionInt(verifyElem->WhenExprTree);
//	if (expr == NULL)
//	{

//		m_ErrorRecorder.PDLError(string("ProtoCheck element needs a valid boolean expression"));
//		return;
//	}
//
//	if (!ReverseCondition(&expr))
//	{
//		m_ErrorRecorder.PDLError(string("ProtoCheck element needs a valid boolean expression"));
//		return;
//	}
//
//	string id = m_CurrProtoSym->Name + string("_proto_check") + string(int2str(m_IfCount++, 10));
//	m_CodeGen->LabelStatement(m_CodeGen->NewLabel(LBL_CODE, id));
//	/*
//	it generates:
//
//	cmp (!expr) goto exit_proto
//	fall
//	;code for the rest of the protocol
//
//	*/
//
//	SymbolLabel *fallthrough = m_CodeGen->NewLabel(LBL_CODE, id + string("_falltgh"));
//	m_CodeGen->JCondStatement(m_CurrProtoSym->ExitProtoLbl, fallthrough, expr);
//	m_CodeGen->LabelStatement(fallthrough);
//#endif
//
//}
//
//bool CheckTypes(uint16 expected, uint16 type1, uint16 type2)
//{
//	return (expected == type1 == type2);
//}
//
//bool CheckType(uint16 expected, uint16 type)
//{
//	return (expected == type);
//}
//
//void PDLParser::EnterLoop(SymbolLabel *start, SymbolLabel *exit)
//{
//LoopInfo loopInfo(start, exit);
//m_LoopStack.push_back(loopInfo);
//}
//
//void PDLParser::ExitLoop(void)
//{
//nbASSERT(!m_LoopStack.empty(), "The loop stack is empty");
//m_LoopStack.pop_back();
//}
//
//void PDLParser::EnterSwitch(SymbolLabel *next)
//{
//SwitchInfo swInfo(next);
//m_SwitchStack.push_back(swInfo);
//}
//
//void PDLParser::ExitSwitch(void)
//{
//nbASSERT(!m_SwitchStack.empty(), "The switch stack is empty");
//m_SwitchStack.pop_back();
//}
//
//PDLParser::LoopInfo& PDLParser::GetLoopInfo(void)
//{
//nbASSERT(!m_LoopStack.empty(), "The loop stack is empty");
//return m_LoopStack.back();
//}
