%{
/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>
#include "expressions.h"
#include <nbprotodb_exports.h>
#include "../nbee/globals/utils.h"
#include "../nbee/globals/debug.h"

#define YYSTYPE void*

int yylex(YYSTYPE *lvalp);
//int yyerror(char **ParsedExpression, char *Message);
int yyerror(void **ParsedExpression, void *Message);

char *ParserErrorBuffer;
int ParserErrorBufferSize;

// This variable (which is the buffer to be parsed, managed by LEX) is needed for debug purposes
extern char *source_buffer_string;

%}



%pure-parser
%parse-param {void **ParsedExpression }

%token OPERAND_NUMBER OPERAND_STRING OPERAND_BOOLEAN
%token OPERAND_VARIABLE_NUMBER OPERAND_VARIABLE_STRING OPERAND_VARIABLE_LOOKUPTABLE
%token FUNCTION_HASSTRING FUNCTION_EXTRACTSTRING FUNCTION_DEBUG FUNCTION_BUFFER2INT FUNCTION_INT2BUFFER FUNCTION_ISPRESENT FUNCTION_ISASN1TYPE
%token FUNCTION_CHANGEBYTEORDER FUNCTION_ASCII2INT FUNCTION_CHECKLOOKUPTABLE FUNCTION_UPDATELOOKUPTABLE
%token OPERAND_PROTOFIELD

%left AND OR
%left GE GT LE LT EQ NEQ
%left BWAND BWOR
%left ADD SUB
%left MUL DIV MOD
%right NOT BWNOT


/* Destructors supported only in bison 2.x*/
/* %destructor {FreeOperatorItem((struct _nbNetPDLExprOperator *) $$); } AND OR GE GT LE LT EQ NEQ BWAND BWOR ADD SUB MUL DIV MOD NOT BWNOT */
/* %destructor {FreeOperandItem((struct _nbNetPDLExprBase *) $$); } OPERAND_NUMBER OPERAND_STRING OPERAND_VARIABLE_NUMBER OPERAND_VARIABLE_STRING OPERAND_RTVAR_PACKETBUFFER OPERAND_PROTOFIELD */
/* %destructor {FreeOperandItem((struct _nbNetPDLExprBase *) $$); } mathexpr stringexpr boolexp*/

%%

program:
        mathexpr
			{
				if (yynerrs > 0)
				{
					ParsedExpression= NULL;
					YYABORT;
				}
				else
				{
					*ParsedExpression= $1;
				}
			}
		| stringexpr
			{
				if (yynerrs > 0)
				{
					ParsedExpression= NULL;
					YYABORT;
				}
				else
				{
					*ParsedExpression= $1;
				}
			}
		| boolexp
			{
				//[icerrato]
				if (yynerrs > 0)
				{
					ParsedExpression= NULL;
					YYABORT;
				}
				else
				{
					*ParsedExpression= $1;
				}
			
			}
			
		| FUNCTION_DEBUG '(' program ')'
		{
			((struct _nbNetPDLExprBase *) *ParsedExpression)->PrintDebug= 1;
		}
		
        ;


parameter_list:
		mathexpr							{$$ = (void *) CreateParameterList(NULL, (struct _nbNetPDLExprBase *) $1); if ($$ == NULL) YYABORT; }
		| stringexpr						{$$ = (void *) CreateParameterList(NULL, (struct _nbNetPDLExprBase *) $1); if ($$ == NULL) YYABORT; }
		| parameter_list ',' mathexpr		{$$ = (void *) CreateParameterList((struct _nbParamsLinkedList *) $1, (struct _nbNetPDLExprBase *) $3); if ($$ == NULL) YYABORT; }
		| parameter_list ',' stringexpr		{$$ = (void *) CreateParameterList((struct _nbParamsLinkedList *) $1, (struct _nbNetPDLExprBase *) $3); if ($$ == NULL) YYABORT; }
		;


mathoperand:
        OPERAND_NUMBER
        | OPERAND_VARIABLE_NUMBER
		;

mathexpr:
		mathoperand
		| FUNCTION_ISASN1TYPE '(' stringexpr ',' OPERAND_NUMBER ',' OPERAND_NUMBER ')'			{ $$ = (void *) CreateFunctionIsASN1Type((struct _nbNetPDLExprString *) $3, (struct _nbNetPDLExprNumber *) $5, (struct _nbNetPDLExprNumber *) $7); if ($$ == NULL) YYABORT; }
		| FUNCTION_BUFFER2INT '(' stringexpr ')'												{ $$ = (void *) CreateFunctionBuffer2IntItem((struct _nbNetPDLExprBase *) $3); if ($$ == NULL) YYABORT; }
        | FUNCTION_ASCII2INT '(' stringexpr ')'													{ $$ = (void *) CreateFunctionAscii2IntItem((struct _nbNetPDLExprBase *) $3); if ($$ == NULL) YYABORT; }
        | FUNCTION_HASSTRING '(' stringexpr ',' OPERAND_STRING ',' OPERAND_NUMBER ')'			{ $$ = (void *) CreateFunctionHasStringItem((struct _nbNetPDLExprBase *) $3, (const char *) $5, (struct _nbNetPDLExprNumber *) $7); if ($$ == NULL) YYABORT; }
        | FUNCTION_ISPRESENT '(' OPERAND_PROTOFIELD ')'											{ $$ = (void *) CreateFunctionIsPresentItem((struct _nbNetPDLExprFieldRef *) $3); if ($$ == NULL) YYABORT; }
        | FUNCTION_CHECKLOOKUPTABLE '(' OPERAND_VARIABLE_LOOKUPTABLE ',' parameter_list ')'		{
																									$$ = (void *) CreateFunctionCheckUpdateLookupTableItem((struct _nbNetPDLExprVariable *) $3, (struct _nbParamsLinkedList *) $5);
																									if ($$ == NULL) YYABORT;
																									else ((struct _nbNetPDLExprFunctionCheckUpdateLookupTable *)$$)->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE;
																								}

        | FUNCTION_UPDATELOOKUPTABLE '(' OPERAND_VARIABLE_LOOKUPTABLE ',' parameter_list ')'	{
																									$$ = (void *) CreateFunctionCheckUpdateLookupTableItem((struct _nbNetPDLExprVariable *) $3, (struct _nbParamsLinkedList *) $5);
																									if ($$ == NULL) YYABORT;
																									else ((struct _nbNetPDLExprFunctionCheckUpdateLookupTable *)$$)->Type= nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE;
																								}
		| '(' mathexpr ')'																		{ $$ = $2; }
		| mathexpr ADD mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr SUB mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr MUL mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr DIV mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr MOD mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr BWOR mathexpr							{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr BWAND mathexpr							{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr EQ mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr NEQ mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr GT mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr GE mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr LT mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr LE mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr AND mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| mathexpr OR mathexpr								{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| BWNOT mathexpr									{ $$ = (void *) CreateExpressionItem(NULL, $1, $2); if ($$ == NULL) YYABORT; }
		| NOT mathexpr										{ $$ = (void *) CreateExpressionItem(NULL, $1, $2); if ($$ == NULL) YYABORT; }
		| stringexpr GT stringexpr							{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| stringexpr LT stringexpr 						{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| stringexpr EQ stringexpr							{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		| stringexpr NEQ stringexpr 						{ $$ = (void *) CreateExpressionItem($1, $2, $3); if ($$ == NULL) YYABORT; }
		;


stringoperand:
        OPERAND_STRING						{ $$ = (void *) CreateStringItem((const char *) $1); }
        | OPERAND_VARIABLE_STRING
        | OPERAND_PROTOFIELD
        ;
        
stringexpr:
		stringoperand
        | FUNCTION_EXTRACTSTRING '(' stringexpr ',' OPERAND_STRING ',' OPERAND_NUMBER ',' OPERAND_NUMBER ')'
							{ 
								$$ = (void *) CreateFunctionExtractStringItem((struct _nbNetPDLExprBase *) $3, (const char *) $5, (struct _nbNetPDLExprNumber *) $7, (struct _nbNetPDLExprNumber *) $9);
								if ($$ == NULL) YYABORT;
							}
		| FUNCTION_INT2BUFFER '(' mathexpr ',' OPERAND_NUMBER ')'		{ $$ = (void *) CreateFunctionInt2BufferItem((struct _nbNetPDLExprBase *) $3, (struct _nbNetPDLExprNumber *) $5); if ($$ == NULL) YYABORT; }
        | FUNCTION_CHANGEBYTEORDER '(' stringexpr ')'					{ $$ = (void *) CreateFunctionChangeByteOrderItem((struct _nbNetPDLExprBase *) $3); if ($$ == NULL) YYABORT; }
		| OPERAND_VARIABLE_STRING '[' mathexpr ':' mathexpr ']'			{ $$ = (void *) UpdateVariableItem($1, $3, $5); if ($$ == NULL) YYABORT; }
		| OPERAND_PROTOFIELD '[' mathexpr ':' mathexpr ']' 				{ $$ = (void *) UpdateProtoFieldItem($1, $3, $5); if ($$ == NULL) YYABORT; }
		| '(' stringexpr ')'
		/* Currently, no operators for string expressions (which are returning strings) are defined */
		;
		
boolexp:
		OPERAND_BOOLEAN { $$ = $1;}
		;

%%



//int yyerror(char **ParsedExpression, char *Message)
int yyerror(void **ParsedExpression, void *Message)
{
	// Do not overwrite an existing error message
	if ((ParserErrorBufferSize > 0) && (ParserErrorBuffer[0] == 0))
	{
		// I don't know why, but it seems that '*ParsedExpression' is always NULL
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, ParserErrorBuffer, ParserErrorBufferSize, 
							"Error compiling NetPDL expression '%s': %s", source_buffer_string, (char*) Message);
	};

	return 1;
}
