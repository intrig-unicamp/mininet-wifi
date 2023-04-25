/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> //for isxdigit
#include <pcre.h>
#include <nbprotodb.h>
#include <nbprotodb_exports.h>
#include <nbprotodb_defs.h>
#include "../nbee/globals/globals.h"
#include "../nbee/globals/utils.h"
#include "../nbee/globals/debug.h"
#include "../nbee/utils/netpdlutils.h"
#include "expr-grammar.tab.h"
#include "expressions.h"



// The error buffer is allocated by the caller
extern "C" char *ParserErrorBuffer;
extern "C" int ParserErrorBufferSize;

struct _nbNetPDLExprBase* FirstExpression;
struct _nbNetPDLExprBase* LastExpression;

extern struct _nbNetPDLDatabase *NetPDLDatabase;

extern int errorsnprintf(char *File, char *Function, int Line, char* Buffer, int BufSize, char *Format, ...);
 


// External Function prototypes
extern "C" void protodb_lex_init(const char * buffer);
extern "C" int pdl_parse (void *YYPARSE_PARAM);
extern "C" void protodb_lex_cleanup();



int CreateExpression(const char *ExprString, int AllowExprType, char **AllocatedExprString, struct _nbNetPDLExprBase** AllocatedExprTree, char *ErrBuf, int ErrBufSize)
{
	int StringLen;

	ParserErrorBuffer= ErrBuf;
	ParserErrorBufferSize= ErrBufSize;

	if (ParserErrorBufferSize > 0)
		ParserErrorBuffer[0]= 0;

	StringLen= (int) strlen(ExprString) + 1;

	if (StringLen == 1)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Expression string empty.");
		return nbFAILURE;
	}


	*AllocatedExprString= (char *) malloc(sizeof(char) * StringLen);

	if (*AllocatedExprString == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return nbFAILURE;
	}

	strcpy(*AllocatedExprString, ExprString); //copy the value of the attribute


	protodb_lex_init(ExprString);

	/*
	*	Ivano: the commented prototype had problems in free bsd and in some versions of windows..
	*	On my ubuntu, bot the commented and the new code work well..
	*/
    //if (pdl_parse((void **) AllocatedExprTree) != 0)
    if (pdl_parse((void *) AllocatedExprTree) != 0)
    {
		protodb_lex_cleanup();
		return nbFAILURE;
	}

    if (*AllocatedExprTree == NULL)
    {
		protodb_lex_cleanup();
		return nbFAILURE;
	}

	// Check if the created expression is compatible with the hosting element
	if ((*AllocatedExprTree)->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
	{
		if ((AllowExprType == CREATE_EXPR_ALLOW_BUFFER_ONLY) || (AllowExprType == CREATE_EXPR_ALLOW_BOOL_ONLY))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
				"Found an expression ('%s') whose return type is not allowed in that element.", *AllocatedExprString );

			protodb_lex_cleanup();
			return nbFAILURE;
		}
	}
	if ((*AllocatedExprTree)->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
	{
		if ((AllowExprType == CREATE_EXPR_ALLOW_NUMBER_ONLY) || (AllowExprType == CREATE_EXPR_ALLOW_BOOL_ONLY))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
				"Found an expression ('%s') whose return type is not allowed in that element.", *AllocatedExprString );

			protodb_lex_cleanup();
			return nbFAILURE;
		}
	}
	
	if ((*AllocatedExprTree)->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BOOLEAN)//[icerrato]
	{
		if ((AllowExprType == CREATE_EXPR_ALLOW_NUMBER_ONLY) || (AllowExprType == CREATE_EXPR_ALLOW_BUFFER_ONLY))
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
				"Found an expression ('%s') whose return type is not allowed in that element.", *AllocatedExprString );

			protodb_lex_cleanup();
			return nbFAILURE;
		}
	}
	
	
	if (FirstExpression == NULL)
	{
		FirstExpression= *AllocatedExprTree;
		LastExpression= *AllocatedExprTree;
	}
	else
	{
		LastExpression->NextExpression= *AllocatedExprTree;
		LastExpression= *AllocatedExprTree;
	}

	protodb_lex_cleanup();
	return nbSUCCESS;
}



void* CreateAliasItem(const char* Token, int* TokenType)
{
struct _nbNetPDLElementAlias *NetPDLElementAlias;
unsigned int i;
char* TmpPtr;

	for (i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLElement;

		NetPDLElement= NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLElement->Type == nbNETPDL_IDEL_ALIAS)
		{
			NetPDLElementAlias= (struct _nbNetPDLElementAlias *) NetPDLElement;

			if (strcmp(NetPDLElementAlias->Name, Token) == 0)
				break;
			else 
				NetPDLElementAlias= NULL;
		}
	}

	if (NetPDLElementAlias == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
			"Found an alias ('%s') which is not among the aliases declared in the protocol database.",
			NetPDLElementAlias->Name);
		return NULL;
	}

	// Check if this is a number or a string
	switch (NetPDLElementAlias->ReplaceWith[0])
	{
		case '\'':	// Operand is a string
		{
			*TokenType= OPERAND_STRING;
			return (strdup(NetPDLElementAlias->ReplaceWith));
		}; break;

		default:	// Operand is a number
		{
			*TokenType= OPERAND_NUMBER;

			if (NetPDLElementAlias->ReplaceWith[0] == '0')
			{
				if (NetPDLElementAlias->ReplaceWith[1] == 'x')
				{
					TmpPtr= NetPDLElementAlias->ReplaceWith + 2;

					while ((*TmpPtr) && (((*TmpPtr >= 'a') && (*TmpPtr <= 'f')) || ((*TmpPtr >= 'A') && (*TmpPtr <= 'F')) || ((*TmpPtr >= '0') && (*TmpPtr <= '9'))))
						TmpPtr++;

					if (*TmpPtr == 0)	// This means that the format of the hex value is correct
					{
						return CreateNumberItem(NetPDLElementAlias->ReplaceWith + 2, 16);
					}
					else
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
							"The replace string in alias '%s' is neither a number, nor a string.",
							NetPDLElementAlias->Name);
						return NULL;
					}
				}

				if (NetPDLElementAlias->ReplaceWith[1] == 'b')
				{
					TmpPtr= NetPDLElementAlias->ReplaceWith + 2;

					while ((*TmpPtr) && ((*TmpPtr >= '0') && (*TmpPtr <= '1')))
						TmpPtr++;

					if (*TmpPtr == 0)	// This means that the format of the bin value is correct
					{
						return CreateNumberItem(NetPDLElementAlias->ReplaceWith + 2, 2);
					}
					else
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
							"The replace string in alias '%s' is neither a number, nor a string.",
							NetPDLElementAlias->Name);
						return NULL;
					}
				}

			}

			// Here, it should be a dec number
			TmpPtr= NetPDLElementAlias->ReplaceWith;

			while ((*TmpPtr) && ((*TmpPtr >= '0') && (*TmpPtr <= '9')))
				TmpPtr++;

			if (*TmpPtr == 0)	// This means that the format of the dec value is correct
			{
				return CreateNumberItem(NetPDLElementAlias->ReplaceWith, 10);
			}
			else
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
					"The replace string in alias '%s' is neither a number, nor a string.",
					NetPDLElementAlias->Name);
				return NULL;
			}
		}; break;
	}

	return NULL;
}


struct _nbNetPDLExprNumber *CreateNumberItem(const char *Token, int Radix)
{
struct _nbNetPDLExprNumber *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprNumber *) malloc(sizeof(struct _nbNetPDLExprNumber));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprNumber));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_NUMBER;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;
	NetPDLElement->Value= NetPDLAsciiStringToLong( (char*) Token, (int) strlen(Token), Radix);

#ifdef DEBUG_EXPRPARSER
	printf("Detected Number: %s\n", Token);
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, (char *) Token, NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprString* CreateStringItem(const char *AsciiString)
{
struct _nbNetPDLExprString *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprString *) malloc(sizeof(struct _nbNetPDLExprString));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprString));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_STRING;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;

	// Allocate string
	NetPDLElement->Value= AllocateAsciiString(AsciiString, &NetPDLElement->Size, 0, 0);

	// Check the error; the error message has already been filled in by the AllocateAsciiString()
	if (NetPDLElement->Value == NULL)
		return NULL;

#ifdef DEBUG_EXPRPARSER
	printf("Detected String: %s\n", AsciiString);
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, (char *) AsciiString, NETPDL_MAX_SHORTSTRING);
#endif

	// Buffer containing the string has been allocated in the lexical parser
	// So, let's free it now
	free((char *) AsciiString);

	if (NetPDLElement->Value == NULL)
		return NULL;
	else
		return NetPDLElement;
}

//[icerrato]
struct _nbNetPDLExprBoolean *CreateBooleanItem(const char *Token)
{
	struct _nbNetPDLExprBoolean *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprBoolean *) malloc(sizeof(struct _nbNetPDLExprBoolean));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprBoolean));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_BOOLEAN;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BOOLEAN;
	NetPDLElement->Value= (strcmp(Token,"true")==0)? 1 : 0;

#ifdef DEBUG_EXPRPARSER
	printf("Detected Boolean: %s\n", Token);
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, (char *) Token, NETPDL_MAX_SHORTSTRING);
#endif
	return NetPDLElement;
}


struct _nbNetPDLExprProtoRef *CreateProtocolReferenceItem(const char *Token)
{
struct _nbNetPDLExprProtoRef *NetPDLElement;
int StringSize;

	NetPDLElement= (struct _nbNetPDLExprProtoRef *) malloc(sizeof(struct _nbNetPDLExprProtoRef));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprProtoRef));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_PROTOREF;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

	// Allocate string
	// Copy variable name into the new structure
	StringSize= (int) strlen(Token) - 1;

	NetPDLElement->ProtocolName= (char *) malloc(sizeof(char) * StringSize + 1);
	if (NetPDLElement->ProtocolName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	memcpy(NetPDLElement->ProtocolName, Token + 1, StringSize);
	NetPDLElement->ProtocolName[StringSize]= 0;


#ifdef DEBUG_EXPRPARSER
	printf("Detected Protocol Reference: %s\n", NetPDLElement->ProtocolName);
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, (char *) NetPDLElement->ProtocolName, NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprOperator *CreateOperatorItem(int Token)
{
struct _nbNetPDLExprOperator *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprOperator *) malloc(sizeof(struct _nbNetPDLExprOperator));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprOperator));

	switch(Token)
	{
		case ADD: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_PLUS; break;
		case SUB: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_MINUS; break;
		case MUL: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_MUL; break;
		case DIV: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_DIV; break;
		case MOD: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_MOD; break;
		case BWAND: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_BITWAND; break;
		case BWOR: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_BITWOR; break;
		case BWNOT: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_BITWNOT; break;

		case EQ: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_EQUAL; break;
		case NEQ: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_NOTEQUAL; break;
		case GT: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_GREAT; break;
		case GE: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_GREATEQUAL; break;
		case LT: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_LESS; break;
		case LE: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_LESSEQUAL; break;
		case AND: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_AND; break;
		case OR: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_OR; break;
		case NOT: NetPDLElement->OperatorType= nbNETPDL_ID_EXPR_OPER_NOT; break;
	}

#ifdef DEBUG_EXPRPARSER
	printf("Detected Operator: %d\n", Token);
#endif
#ifdef _DEBUG
	switch(Token)
	{
		case ADD: sstrncpy(NetPDLElement->Token, "ADD", NETPDL_MAX_SHORTSTRING); break;
		case SUB: sstrncpy(NetPDLElement->Token, "SUB", NETPDL_MAX_SHORTSTRING); break;
		case MUL: sstrncpy(NetPDLElement->Token, "MUL", NETPDL_MAX_SHORTSTRING); break;
		case DIV: sstrncpy(NetPDLElement->Token, "DIV", NETPDL_MAX_SHORTSTRING); break;
		case MOD: sstrncpy(NetPDLElement->Token, "MOD", NETPDL_MAX_SHORTSTRING); break;
		case BWAND: sstrncpy(NetPDLElement->Token, "BWAND", NETPDL_MAX_SHORTSTRING); break;
		case BWOR: sstrncpy(NetPDLElement->Token, "BWOR", NETPDL_MAX_SHORTSTRING); break;
		case BWNOT: sstrncpy(NetPDLElement->Token, "BWNOT", NETPDL_MAX_SHORTSTRING); break;

		case EQ: sstrncpy(NetPDLElement->Token, "EQ", NETPDL_MAX_SHORTSTRING); break;
		case NEQ: sstrncpy(NetPDLElement->Token, "NEQ", NETPDL_MAX_SHORTSTRING); break;
		case GT: sstrncpy(NetPDLElement->Token, "GT", NETPDL_MAX_SHORTSTRING); break;
		case GE: sstrncpy(NetPDLElement->Token, "GE", NETPDL_MAX_SHORTSTRING); break;
		case LT: sstrncpy(NetPDLElement->Token, "LT", NETPDL_MAX_SHORTSTRING); break;
		case LE: sstrncpy(NetPDLElement->Token, "LE", NETPDL_MAX_SHORTSTRING); break;
		case AND: sstrncpy(NetPDLElement->Token, "AND", NETPDL_MAX_SHORTSTRING); break;
		case OR: sstrncpy(NetPDLElement->Token, "OR", NETPDL_MAX_SHORTSTRING); break;
		case NOT: sstrncpy(NetPDLElement->Token, "NOT", NETPDL_MAX_SHORTSTRING); break;
	}
#endif

	return NetPDLElement;
}


// Due to some problem due to the checklookuptable(), this function can create either a variable
// or a lookup table, hence the return type is 'void' in order to accomodate both cases.
void* CreateVariableItem(const char *Token, int* TokenType)
{
struct _nbNetPDLElementVariable* NetPDLVariableDef;
struct _nbNetPDLElementLookupTable* NetPDLLookupTableDef;
unsigned int i;

	// Detecting if the token is a variable or a lookup table
	for (i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLTempElement;

		NetPDLTempElement= NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_VARIABLE)
		{
			NetPDLVariableDef= (struct _nbNetPDLElementVariable *) NetPDLTempElement;

			if (strcmp(NetPDLVariableDef->Name, Token) == 0)
			{
			struct _nbNetPDLExprVariable *NetPDLElement;
			int StringSize;

				NetPDLElement= (struct _nbNetPDLExprVariable *) malloc(sizeof(struct _nbNetPDLExprVariable));

				if (NetPDLElement == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
					return NULL;
				}
				memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprVariable));


				NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_VARIABLE;

				// Copy variable name in the new structure
				StringSize= (int) strlen(Token);

				NetPDLElement->Name= (char *) malloc(sizeof(char) * StringSize + 1);
				if (NetPDLElement->Name == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
					return NULL;
				}

				memcpy(NetPDLElement->Name, Token, StringSize);
				NetPDLElement->Name[StringSize]= 0;

				// Detecting if the variable is a number or a buffer
				switch (NetPDLVariableDef->VariableDataType)
				{
					case nbNETPDL_VARIABLE_TYPE_NUMBER:
					case nbNETPDL_VARIABLE_TYPE_PROTOCOL:
					{
						NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;
						*TokenType= OPERAND_VARIABLE_NUMBER;
					}; break;

					case nbNETPDL_VARIABLE_TYPE_BUFFER:
					case nbNETPDL_VARIABLE_TYPE_REFBUFFER:
					{
						NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;
						*TokenType= OPERAND_VARIABLE_STRING;
					}; break;

					default:
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Internal error: value not allowed.");
						return NULL;
					}
				}

	#ifdef _DEBUG
				// Initializing to a fake number, just for debug.
				NetPDLElement->CustomData = (void*) 99999999;
	#endif

	#ifdef DEBUG_EXPRPARSER
				printf("Detected Variable: %s\n", Token);
	#endif
	#ifdef _DEBUG
				sstrncpy(NetPDLElement->Token, (char *) Token, NETPDL_MAX_SHORTSTRING);
	#endif

				return (void*) NetPDLElement;
			}
		}
	}


	// We may be in the case in which we have only a table name, which is required in the checklookuptable() function
	// In this case, previous code failed, and we're arriving here
	for (i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLTempElement;

		NetPDLTempElement= NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_LOOKUPTABLE)
		{
			NetPDLLookupTableDef= (struct _nbNetPDLElementLookupTable *) NetPDLTempElement;

			if (strcmp(NetPDLLookupTableDef->Name, Token) == 0)
			{
			struct _nbNetPDLExprLookupTable *NetPDLElement;
			int StringSize;

				NetPDLElement= (struct _nbNetPDLExprLookupTable *) malloc(sizeof(struct _nbNetPDLExprLookupTable));

				if (NetPDLElement == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
					return NULL;
				}
				memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprLookupTable));

				NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE;

				// Copy lookup table name in the new structure
				StringSize= (int) strlen(Token);

				NetPDLElement->TableName= (char *) malloc(sizeof(char) * StringSize + 1);
				if (NetPDLElement->TableName == NULL)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
					return NULL;
				}

				memcpy(NetPDLElement->TableName, Token, StringSize);
				NetPDLElement->TableName[StringSize]= 0;

				// In this case, the ReturnType does not matter
				NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_DONTMIND;

				*TokenType= OPERAND_VARIABLE_LOOKUPTABLE;

	#ifdef _DEBUG
				// Initializing to a fake number, just for debug.
				NetPDLElement->TableCustomData = (void*) 99999999;
				NetPDLElement->FieldCustomData = (void*) 99999999;
	#endif

	#ifdef DEBUG_EXPRPARSER
				printf("Detected LookupTable: %s\n", Token);
	#endif
	#ifdef _DEBUG
				sstrncpy(NetPDLElement->Token, (char *) Token, NETPDL_MAX_SHORTSTRING);
	#endif

				return (void*) NetPDLElement;
			}
		}
	}

	// If we're here, it means that we didn't find neither a variable, nor a lookup table
	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
			"Found variable '%s' in expression. This variable has not been defined in the NetPDL variable list.", Token);
	return NULL;

}


struct _nbNetPDLExprLookupTable* CreateLookupTableItem(const char *Token, int* TokenType)
{
struct _nbNetPDLExprLookupTable* NetPDLElement;
struct _nbNetPDLElementLookupTable* NetPDLLookupTableDef;
unsigned int i;
char *FieldNamePtr;
int StringSize;


	NetPDLElement= (struct _nbNetPDLExprLookupTable *) malloc(sizeof(struct _nbNetPDLExprLookupTable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprLookupTable));


	// Copy variable name in the new structure
	// Let's split between table name and field name
	FieldNamePtr=(char *)strchr(Token, '.');
	*FieldNamePtr= 0;
	FieldNamePtr++;

	StringSize= (int) strlen(Token);

	NetPDLElement->TableName= (char *) malloc(sizeof(char) * StringSize + 1);
	if (NetPDLElement->TableName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	memcpy(NetPDLElement->TableName, Token, StringSize);
	NetPDLElement->TableName[StringSize]= 0;

	// Let's copy the field name
	StringSize= (int) strlen(FieldNamePtr);

	NetPDLElement->FieldName= (char *) malloc(sizeof(char) * StringSize + 1);
	if (NetPDLElement->FieldName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	memcpy(NetPDLElement->FieldName, FieldNamePtr, StringSize);
	NetPDLElement->FieldName[StringSize]= 0;

	// Detecting if the variable is a number or a buffer
	for (i= 1; i < NetPDLDatabase->GlobalElementsListNItems; i++)
	{
	struct _nbNetPDLElementBase *NetPDLTempElement;

		NetPDLTempElement= NetPDLDatabase->GlobalElementsList[i];

		if (NetPDLTempElement->Type == nbNETPDL_IDEL_LOOKUPTABLE)
		{
			NetPDLLookupTableDef= (struct _nbNetPDLElementLookupTable *) NetPDLTempElement;

			if (strcmp(NetPDLLookupTableDef->Name, NetPDLElement->TableName) == 0)
			{
			struct _nbNetPDLElementKeyData* TempKeyData;

				TempKeyData= NetPDLLookupTableDef->FirstKey;

				while (TempKeyData)
				{
					if (strcmp(TempKeyData->Name, NetPDLElement->FieldName) == 0)
					{
						NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE;

						switch (TempKeyData->KeyDataType)
						{
							case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
							case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
							{
								NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;
								*TokenType= OPERAND_VARIABLE_NUMBER;
							}; break;

							case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
							{
								NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;
								*TokenType= OPERAND_VARIABLE_STRING;
							}; break;

							default:
							{
								errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Internal error: value not allowed.");
								return NULL;
							}
						}
					}

					TempKeyData= TempKeyData->NextKeyData;
				}


				TempKeyData= NetPDLLookupTableDef->FirstData;

				while (TempKeyData)
				{
					if (strcmp(TempKeyData->Name, NetPDLElement->FieldName) == 0)
					{
						NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE;

						switch (TempKeyData->KeyDataType)
						{
							case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
							case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
							{
								NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;
								*TokenType= OPERAND_VARIABLE_NUMBER;
							}; break;

							case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
							{
								NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;
								*TokenType= OPERAND_VARIABLE_STRING;
							}; break;

							default:
							{
								errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Internal error: value not allowed.");
								return NULL;
							}
						}
					}

					TempKeyData= TempKeyData->NextKeyData;
				}

				break;
			}
		}
	}

	if (NetPDLElement->Type == 0)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize,
			"Found variable '%s' in expression. This variable has not been defined in the NetPDL variable list.", NetPDLElement->TableName);
		return NULL;
	}

#ifdef _DEBUG
	// Initializing to a fake number, just for debug.
	NetPDLElement->TableCustomData = (void*) 99999999;
	NetPDLElement->FieldCustomData = (void*) 99999999;
#endif

#ifdef DEBUG_EXPRPARSER
	printf("Detected LookupTable: %s\n", Token);
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, (char *) Token, NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFieldRef* CreateProtoFieldItem(const char *Token)
{
struct _nbNetPDLExprFieldRef *NetPDLElement;
int StringSize;

	NetPDLElement= (struct _nbNetPDLExprFieldRef *) malloc(sizeof(struct _nbNetPDLExprFieldRef));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFieldRef));

	// Let's check it this is a special form of fieldref: the 'this' field
	if (strncmp(Token, NETPDL_EXPR_THISFIELD, strlen(NETPDL_EXPR_THISFIELD)) == 0)
		NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD_THIS;
	else
		NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD;

	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;

	// Copy fieldref name into the new structure
	StringSize= (int) strlen(Token);

	NetPDLElement->FieldName= (char *) malloc(sizeof(char) * StringSize + 1);
	if (NetPDLElement->FieldName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	memcpy(NetPDLElement->FieldName, Token, StringSize);
	NetPDLElement->FieldName[StringSize]= 0;

#ifdef DEBUG_EXPRPARSER
	if (NetPDLElement->Type == nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD)
		printf("Detected NetPDL Field: %s\n", Token);
	else
		printf("Detected 'this' NetPDL Field: %s\n", Token);
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, (char *) Token, NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprVariable* UpdateVariableItem(void *Variable, void *StartOffset, void *Size)
{
struct _nbNetPDLExprVariable *NetPDLElement;
// unused
// struct _nbNetPDLExprBase *OperandElement;

	NetPDLElement= (struct _nbNetPDLExprVariable *) Variable;

//	OperandElement= (struct _nbNetPDLExprBase  *) StartOffset;

	NetPDLElement->OffsetStartAt= (struct _nbNetPDLExprBase *) StartOffset;

//	OperandElement= (struct _nbNetPDLExprBase *) Size;

	NetPDLElement->OffsetSize= (struct _nbNetPDLExprBase *) Size;

	return NetPDLElement;
}


struct _nbNetPDLExprFieldRef* UpdateProtoFieldItem(void *ProtoField, void *StartOffset, void *Size)
{
struct _nbNetPDLExprFieldRef *NetPDLElement;
// unused
// struct _nbNetPDLExprBase *OperandElement;

	NetPDLElement= (struct _nbNetPDLExprFieldRef *) ProtoField;

//	OperandElement= (struct _nbNetPDLExprBase *) StartOffset;
	NetPDLElement->OffsetStartAt= (struct _nbNetPDLExprBase *) StartOffset;

//	OperandElement= (struct _nbNetPDLExprBase *) Size;
	NetPDLElement->OffsetSize= (struct _nbNetPDLExprBase *) Size;

	return NetPDLElement;
}


struct _nbNetPDLExprBase *CreateExpressionItem(void *Operand1, void* Operator, void* Operand2)
{
struct _nbNetPDLExpression *NetPDLElement;
struct _nbNetPDLExprOperator *OperatorElement;
struct _nbNetPDLExprBase *OperandElement1, *OperandElement2;

	NetPDLElement= (struct _nbNetPDLExpression *) malloc(sizeof(struct _nbNetPDLExpression));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExpression));

	OperandElement1= (struct _nbNetPDLExprBase *) Operand1;
	OperandElement2= (struct _nbNetPDLExprBase *) Operand2;
	OperatorElement= (struct _nbNetPDLExprOperator *) Operator;

	// Update links
	NetPDLElement->Operand1= OperandElement1;
	NetPDLElement->Operand2= OperandElement2;
	NetPDLElement->Operator= OperatorElement;

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_EXPR;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

#ifdef DEBUG_EXPRPARSER
	printf("Detected new sub-expression number");
	if (OperandElement1)
		printf(" Op1 %s", OperandElement1->Token);
	if (OperandElement2)
		printf(" Op2 %s", OperandElement2->Token);
	printf("\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "NewExpressionNumber", NETPDL_MAX_SHORTSTRING);
#endif

	return (struct _nbNetPDLExprBase*) NetPDLElement;
}


/*!
	\brief Allocate an ASCII string.

	\param Token: original string to be allocated. This string is in ascii, is NULL-terminated, and it can
	contain hex chars in the form \\xNN. For instance, these chars gets translated in the proper ASCII char, 
	spanning over ONE byte.

	\param NewStringSize: this parameter is used to return the size of the allocated size. This is useful
	when the string is not NULL terminated in order to know its size.

	\param TerminateStringWithZero: if 'true', the allocated string will have a \\x0 terminating character.

	\param TokenDoesNotHaveApex: if 'true', the original token is not included within \' markers.

	\return A pointer to the allocated string. The string MUST be deallocated by the caller.
*/
char* AllocateAsciiString(const char *Token, unsigned int *NewStringSize, int TerminateStringWithZero, int TokenDoesNotHaveApex)
{
int SizeToBeAllocated;
int i, TokenIndex;
char *BufferPtr;

	SizeToBeAllocated= (int) strlen(Token) - 2;

	// The allocated string does not need the \x0 character at the beginning and at the end
	if (TerminateStringWithZero)
		SizeToBeAllocated+= 1;

	if (TokenDoesNotHaveApex)
		SizeToBeAllocated+= 2;

	// Let's check if there are '\xnn' strings
	BufferPtr= (char *) Token;
	while (BufferPtr)
	{
		BufferPtr= strstr(BufferPtr, "\\x");

		if (BufferPtr == NULL)
			break;

		if (!isxdigit( *(BufferPtr+2)) )
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Invalid string: a '\\x' must be followed by two integers.");
			return NULL;
		}

		if (!isxdigit( *(BufferPtr+3)) )
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Invalid string: a '\\x' must be followed by two integers.");
			return NULL;
		}
		else
		{
			SizeToBeAllocated-= 3;
			BufferPtr+= 4;
		}
	}

	// The string does not have the \0 at the end (since it may be a binary string),
	// so let's allocate the 'net' size
	BufferPtr= (char *) malloc(sizeof(char) * SizeToBeAllocated);
	if (BufferPtr == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

 	if (TokenDoesNotHaveApex)
		TokenIndex= 0;
	else
		TokenIndex= 1; // We have to skip the ' character at the beginning

	// Now, let's copy the content of Token in the new string buffer
	for (i= 0; i < SizeToBeAllocated; i++)
	{
		if ((Token[TokenIndex] == '\\') && (Token[TokenIndex + 1] == 'x'))
		{
			BufferPtr[i]= ConvertHexCharToDec(Token[TokenIndex + 2]) * 16 + ConvertHexCharToDec(Token[TokenIndex + 3]);
			TokenIndex+= 4;
			continue;
		}
		BufferPtr[i]= Token[TokenIndex];
		TokenIndex++;
	}

	if (TerminateStringWithZero)
		BufferPtr[SizeToBeAllocated - 1]= 0;

	// If the user is interested in the size of the new string buffer
	if (NewStringSize)
		*NewStringSize= (int) SizeToBeAllocated;

	return BufferPtr;
}


struct _nbNetPDLExprFunctionIsASN1Type* CreateFunctionIsASN1Type(struct _nbNetPDLExprString * StringOperand, struct _nbNetPDLExprNumber *ClassNumber, struct _nbNetPDLExprNumber *TagNumber)
{
struct _nbNetPDLExprFunctionIsASN1Type *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprFunctionIsASN1Type *) malloc(sizeof(struct _nbNetPDLExprFunctionIsASN1Type));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionIsASN1Type));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISASN1TYPE;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

	NetPDLElement->StringExpression= (struct _nbNetPDLExprBase *) StringOperand;
	NetPDLElement->ClassNumber= (struct _nbNetPDLExprBase *) ClassNumber;
	NetPDLElement->TagNumber= (struct _nbNetPDLExprBase *) TagNumber;

#ifdef DEBUG_EXPRPARSER
	printf("Detected IsASN1Type() function\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "IsASN1Type()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionRegExp *CreateFunctionHasStringItem(struct _nbNetPDLExprBase* SearchingBuffer, const char *RegularExpression, struct _nbNetPDLExprNumber* CaseSensitive)
{
struct _nbNetPDLExprFunctionRegExp *NetPDLElement;
int PCREFlags;
const char *PCREErrorPtr;
int PCREErrorOffset;

	NetPDLElement= (struct _nbNetPDLExprFunctionRegExp *) malloc(sizeof(struct _nbNetPDLExprFunctionRegExp));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionRegExp));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_HASSTRING;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

	// The SearchingBuffer may be either a string operator (e.g. a standard packet buffer) or a more complex expression
	NetPDLElement->SearchBuffer= SearchingBuffer;

        // Check for the \x00: This check avoid the presence of \x00 sequence that is invalid (see below)
        /* 
          Problem:  the pcre_compile accept the NULL char within a pattern, but it must be escaped 
                    like this "foo\\x00foo" or this "foo\\0foo".
                    Since AllocateAsciiString converts \x00 into the real bytes value 0x00 this make the pcre_compile to fail.
          Solution: instead of "\x00" a regular expression in NetPDL must use the sequence "\0" that is passed as it is
                    from AllocateAsciiString to the pcre_compile.
        */
        const char *null_check = strstr(RegularExpression, "\\x00");
        if(null_check!=NULL){
                errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, 
			"Regular expression string '%s' contains the invalid sequence '\\x00'. Use sequence '\\0' if you want to represent the NULL byte\n", RegularExpression);
		return NULL;
        }

	// Allocate string
	NetPDLElement->RegularExpression= AllocateAsciiString(RegularExpression, NULL, 1, 0);

	// Check the error; the error message has already been filled in by the AllocateAsciiString()
	if (NetPDLElement->RegularExpression == NULL)
		return NULL;

	// Buffer containing the regular expression has been allocated in the lexical parser
	// So, let's free it now
	free((char *) RegularExpression);


	// Let's compile the regular expression for our convenience
	NetPDLElement->CaseSensitiveMatch= CaseSensitive->Value;
	if (NetPDLElement->CaseSensitiveMatch)
		PCREFlags= 0;
	else
		PCREFlags= PCRE_CASELESS;

	// This number is no longer required
	FreeOperandItem( (struct _nbNetPDLExprBase* ) CaseSensitive);


	NetPDLElement->PCRECompiledRegExp= (void *) pcre_compile(NetPDLElement->RegularExpression, PCREFlags, &PCREErrorPtr, &PCREErrorOffset, NULL);

	if (NetPDLElement->PCRECompiledRegExp == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, 
			"Regular expression compilation for string '%s' has failed at offset %d with error '%s'",
			NetPDLElement->RegularExpression, PCREErrorOffset, PCREErrorPtr);

		return NULL;
	}

#ifdef DEBUG_EXPRPARSER
	printf("Detected HasString function: RegExp %s\n", NetPDLElement->RegularExpression);
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "HasString()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionRegExp *CreateFunctionExtractStringItem(struct _nbNetPDLExprBase* SearchingBuffer,
							const char *RegularExpression, struct _nbNetPDLExprNumber* CaseSensitive, struct _nbNetPDLExprNumber* MatchToBeReturned)
{
struct _nbNetPDLExprFunctionRegExp *NetPDLElement;

	// This function is exactly the same as CreateFunctionHasStringItem(); only the function code changes
	NetPDLElement= CreateFunctionHasStringItem(SearchingBuffer, RegularExpression, CaseSensitive);

	if (NetPDLElement)
	{
		NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_EXTRACTSTRING;
		NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;
		NetPDLElement->MatchToBeReturned= MatchToBeReturned->Value;
		// This number is no longer required
		FreeOperandItem((struct _nbNetPDLExprBase* ) MatchToBeReturned);

#ifdef DEBUG_EXPRPARSER
		printf("Detected ExtractString function: RegExp %s\n", NetPDLElement->RegularExpression);
#endif
#ifdef _DEBUG
		strncpy(NetPDLElement->Token, "ExtractString()", NETPDL_MAX_SHORTSTRING);
#endif
	}

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionIsPresent* CreateFunctionIsPresentItem(struct _nbNetPDLExprFieldRef* FieldOperand)
{
struct _nbNetPDLExprFunctionIsPresent *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprFunctionIsPresent *) malloc(sizeof(struct _nbNetPDLExprFunctionIsPresent));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionIsPresent));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISPRESENT;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

	NetPDLElement->NetPDLField= FieldOperand;


#ifdef DEBUG_EXPRPARSER
	printf("Detected ispresent() function\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "ispresent()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionBuf2Int* CreateFunctionBuffer2IntItem(struct _nbNetPDLExprBase* StringOperand)
{
struct _nbNetPDLExprFunctionBuf2Int *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprFunctionBuf2Int *) malloc(sizeof(struct _nbNetPDLExprFunctionBuf2Int));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionBuf2Int));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_BUF2INT;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

	NetPDLElement->StringExpression= StringOperand;


#ifdef DEBUG_EXPRPARSER
	printf("Detected buf2int() function\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "buf2int()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionInt2Buf* CreateFunctionInt2BufferItem(struct _nbNetPDLExprBase* NumericOperand, struct _nbNetPDLExprNumber* SizeOfResultBuffer)
{
struct _nbNetPDLExprFunctionInt2Buf *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprFunctionInt2Buf *) malloc(sizeof(struct _nbNetPDLExprFunctionInt2Buf));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionInt2Buf));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_INT2BUF;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;

	NetPDLElement->NumericExpression= NumericOperand;

	NetPDLElement->ResultSize= SizeOfResultBuffer->Value;

	// This number is no longer required
	FreeOperandItem((struct _nbNetPDLExprBase* ) SizeOfResultBuffer);


#ifdef DEBUG_EXPRPARSER
	printf("Detected int2buf() function\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "int2buf()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionAscii2Int* CreateFunctionAscii2IntItem(struct _nbNetPDLExprBase* StringOperand)
{
struct _nbNetPDLExprFunctionAscii2Int *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprFunctionAscii2Int *) malloc(sizeof(struct _nbNetPDLExprFunctionAscii2Int));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionAscii2Int));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ASCII2INT;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

	NetPDLElement->AsciiStringExpression= StringOperand;

#ifdef DEBUG_EXPRPARSER
	printf("Detected Ascii2Int() function\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "Ascii2Int()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionChangeByteOrder* CreateFunctionChangeByteOrderItem(struct _nbNetPDLExprBase* StringOperand)
{
struct _nbNetPDLExprFunctionChangeByteOrder *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprFunctionChangeByteOrder *) malloc(sizeof(struct _nbNetPDLExprFunctionChangeByteOrder));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionChangeByteOrder));

	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHANGEBYTEORDER;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER;

	NetPDLElement->OriginalStringExpression= StringOperand;

#ifdef DEBUG_EXPRPARSER
	printf("Detected ChangeByteOrder() function\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "ChangeByteOrder()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}


struct _nbNetPDLExprFunctionCheckUpdateLookupTable* CreateFunctionCheckUpdateLookupTableItem(struct _nbNetPDLExprVariable* LookupTable, struct _nbParamsLinkedList* ParameterList)
{
struct _nbNetPDLExprFunctionCheckUpdateLookupTable *NetPDLElement;

	NetPDLElement= (struct _nbNetPDLExprFunctionCheckUpdateLookupTable *) malloc(sizeof(struct _nbNetPDLExprFunctionCheckUpdateLookupTable));

	if (NetPDLElement == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(NetPDLElement, 0, sizeof(struct _nbNetPDLExprFunctionCheckUpdateLookupTable));

	// The 'Type' member will be set after this function
//	NetPDLElement->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE;
	NetPDLElement->ReturnType= nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER;

	NetPDLElement->TableName= (char*) malloc(strlen(LookupTable->Name) + 1);

	if (NetPDLElement->TableName == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}

	strcpy(NetPDLElement->TableName, LookupTable->Name);

	// The variable structure is no longer required
	FreeOperandItem((struct _nbNetPDLExprBase*) LookupTable);

	// Save parameter list for this function
	NetPDLElement->ParameterList= ParameterList;


#ifdef DEBUG_EXPRPARSER
	printf("Detected checklookuptable() function\n");
#endif
#ifdef _DEBUG
	sstrncpy(NetPDLElement->Token, "checklookuptable()", NETPDL_MAX_SHORTSTRING);
#endif

	return NetPDLElement;
}



struct _nbParamsLinkedList* CreateParameterList(struct _nbParamsLinkedList* PreviousParameterList, struct _nbNetPDLExprBase* AdditionalExpression)
{
struct _nbParamsLinkedList* AdditionalLinkedExpression;

	AdditionalLinkedExpression= (struct _nbParamsLinkedList *) malloc(sizeof(struct _nbParamsLinkedList));

	if (AdditionalLinkedExpression == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Not enough memory for building the protocol database.");
		return NULL;
	}
	memset(AdditionalLinkedExpression, 0, sizeof(struct _nbParamsLinkedList));

	AdditionalLinkedExpression->Expression= AdditionalExpression;

	if (PreviousParameterList)
	{
	struct _nbParamsLinkedList* TempLinkedExpression;

		TempLinkedExpression= PreviousParameterList;

		while (TempLinkedExpression->NextParameter)
		{
			TempLinkedExpression= TempLinkedExpression->NextParameter;
		}
		TempLinkedExpression->NextParameter= AdditionalLinkedExpression;

		return PreviousParameterList;
	}
	else
	{
		return AdditionalLinkedExpression;
	}
}



/*****************************************************/
/*               Cleanup functions                   */
/*****************************************************/


void FreeOperatorItem(struct _nbNetPDLExprOperator* OperatorElement)
{
	free(OperatorElement);
}

void FreeOperandItem(struct _nbNetPDLExprBase* GenericOperandElement)
{
	if (GenericOperandElement)
	{
		switch (GenericOperandElement->Type)
		{
			case nbNETPDL_ID_EXPR_OPERAND_NUMBER: 
				break;

			case nbNETPDL_ID_EXPR_OPERAND_STRING:
			{
			struct _nbNetPDLExprString* Element= (struct _nbNetPDLExprString*) GenericOperandElement;
				
				if (Element->Value)
					free(Element->Value);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_PROTOREF:
			{
			struct _nbNetPDLExprProtoRef* Element= (struct _nbNetPDLExprProtoRef*) GenericOperandElement;
				
				if (Element->ProtocolName)
					free(Element->ProtocolName);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
			{
			struct _nbNetPDLExprVariable* Element= (struct _nbNetPDLExprVariable*) GenericOperandElement;

				if (Element->Name)
					free(Element->Name);

				if (Element->OffsetSize)
					FreeExpression(Element->OffsetSize);
				if (Element->OffsetStartAt)
					FreeExpression(Element->OffsetStartAt);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:
			{
			struct _nbNetPDLExprLookupTable* Element= (struct _nbNetPDLExprLookupTable*) GenericOperandElement;

				if (Element->TableName)
					free(Element->TableName);
				if (Element->FieldName)
					free(Element->FieldName);

				if (Element->OffsetSize)
					FreeExpression(Element->OffsetSize);
				if (Element->OffsetStartAt)
					FreeExpression(Element->OffsetStartAt);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD:
			case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD_THIS:
			{
			struct _nbNetPDLExprFieldRef* Element= (struct _nbNetPDLExprFieldRef*) GenericOperandElement;

				if (Element->FieldName)
					free(Element->FieldName);
				if (Element->ProtoName)
					free(Element->ProtoName);

				if (Element->OffsetSize)
					FreeExpression(Element->OffsetSize);
				if (Element->OffsetStartAt)
					FreeExpression(Element->OffsetStartAt);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISASN1TYPE:
			{
			struct _nbNetPDLExprFunctionIsASN1Type *Element= (struct _nbNetPDLExprFunctionIsASN1Type*) GenericOperandElement;

				if (Element->StringExpression)
					free(Element->StringExpression);
	
				if (Element->ClassNumber)
					free(Element->ClassNumber);

				if (Element->TagNumber)
					free(Element->TagNumber);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_HASSTRING:
			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_EXTRACTSTRING:
			{
			struct _nbNetPDLExprFunctionRegExp* Element= (struct _nbNetPDLExprFunctionRegExp*) GenericOperandElement;

				if (Element->RegularExpression)
					free(Element->RegularExpression);

				if (Element->SearchBuffer)
					FreeExpression(Element->SearchBuffer);

				if (Element->PCRECompiledRegExp)
					pcre_free(Element->PCRECompiledRegExp);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_BUF2INT:
			{
			struct _nbNetPDLExprFunctionBuf2Int* Element= (struct _nbNetPDLExprFunctionBuf2Int*) GenericOperandElement;

				if (Element->StringExpression)
					FreeExpression( (struct _nbNetPDLExprBase *) Element->StringExpression);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_INT2BUF:
			{
			struct _nbNetPDLExprFunctionInt2Buf* Element= (struct _nbNetPDLExprFunctionInt2Buf*) GenericOperandElement;

				if (Element->NumericExpression)
					FreeExpression( (struct _nbNetPDLExprBase *) Element->NumericExpression);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISPRESENT:
			{
			struct _nbNetPDLExprFunctionIsPresent* Element= (struct _nbNetPDLExprFunctionIsPresent*) GenericOperandElement;

				if (Element->NetPDLField)
					FreeExpression( (struct _nbNetPDLExprBase *) Element->NetPDLField);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHANGEBYTEORDER:
			{
			struct _nbNetPDLExprFunctionChangeByteOrder* Element= (struct _nbNetPDLExprFunctionChangeByteOrder*) GenericOperandElement;

				if (Element->OriginalStringExpression)
					FreeExpression( (struct _nbNetPDLExprBase *) Element->OriginalStringExpression);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ASCII2INT:
			{
			struct _nbNetPDLExprFunctionAscii2Int* Element= (struct _nbNetPDLExprFunctionAscii2Int*) GenericOperandElement;

				if (Element->AsciiStringExpression)
					FreeExpression( (struct _nbNetPDLExprBase *) Element->AsciiStringExpression);

			}; break;

			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE:
			case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE:
			{
			struct _nbNetPDLExprFunctionCheckUpdateLookupTable* Element= (struct _nbNetPDLExprFunctionCheckUpdateLookupTable*) GenericOperandElement;
			struct _nbParamsLinkedList* TempLinkedExpression1, * TempLinkedExpression2;

				if (Element->TableName)
					free(Element->TableName);

				TempLinkedExpression1= Element->ParameterList;

				while (TempLinkedExpression1)
				{
					FreeExpression( (struct _nbNetPDLExprBase *) TempLinkedExpression1->Expression);
					TempLinkedExpression2= TempLinkedExpression1->NextParameter;

					free(TempLinkedExpression1);
					TempLinkedExpression1= TempLinkedExpression2;
				}

			}; break;

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, "Internal error: detected an element that cannot be deallocated by the parser.");
			}

		}

		free(GenericOperandElement);
	}
}


void FreeExpression(struct _nbNetPDLExprBase* ExpressionElement)
{
	if (ExpressionElement == NULL)
		return;

	if (ExpressionElement->Type == nbNETPDL_ID_EXPR_OPERAND_EXPR)
	{
	struct _nbNetPDLExpression* Expression;

		Expression= (struct _nbNetPDLExpression*) ExpressionElement;

		if (Expression->Operand1)
		{
			if (Expression->Operand1->Type == nbNETPDL_ID_EXPR_OPERAND_EXPR)
				FreeExpression(Expression->Operand1);
			else
				FreeOperandItem(Expression->Operand1);
		}

		if (Expression->Operator)
			FreeOperatorItem(Expression->Operator);

		if (Expression->Operand2)
		{
			if (Expression->Operand2->Type == nbNETPDL_ID_EXPR_OPERAND_EXPR)
				FreeExpression(Expression->Operand2);
			else
				FreeOperandItem(Expression->Operand2);
		}

		free(ExpressionElement);
	}
	else
		FreeOperandItem(ExpressionElement);

}
