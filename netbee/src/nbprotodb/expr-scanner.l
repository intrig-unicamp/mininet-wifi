%option nounput

%{
/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <stdlib.h>
#include <string.h>
#include <nbprotodb_exports.h>
#include "expressions.h"
#include "expr-grammar.tab.h"

#define YY_NO_INPUT

void pdl_error(char **, char*);


YY_BUFFER_STATE source_buffer= 0;
/* String needed for debug purposes */
char *source_buffer_string= NULL;

/* This variable is used to determine the token type from within custom code */
int TokenType;

#define YY_DECL int pdl_lex(void** lvalp)
/* #define YY_DECL int yylex(YYSTYPE *lvalp) */

%}

%%

"+"						{*lvalp= (void *) CreateOperatorItem(ADD);   return ADD; }
"-"						{*lvalp= (void *) CreateOperatorItem(SUB);   return SUB; }
"*"						{*lvalp= (void *) CreateOperatorItem(MUL);   return MUL; }
"div"					{*lvalp= (void *) CreateOperatorItem(DIV);   return DIV; }
"mod"					{*lvalp= (void *) CreateOperatorItem(MOD);   return MOD; }
"bitwand"				{*lvalp= (void *) CreateOperatorItem(BWAND); return BWAND; }
"bitwor"				{*lvalp= (void *) CreateOperatorItem(BWOR);  return BWOR; }
"bitwnot"				{*lvalp= (void *) CreateOperatorItem(BWNOT); return BWNOT; }

"=="					{*lvalp= (void *) CreateOperatorItem(EQ);    return EQ; }
"!="					{*lvalp= (void *) CreateOperatorItem(NEQ);   return NEQ; }
"ge"					{*lvalp= (void *) CreateOperatorItem(GE);    return GE; }
"gt"					{*lvalp= (void *) CreateOperatorItem(GT);    return GT; }
"le"					{*lvalp= (void *) CreateOperatorItem(LE);    return LE; }
"lt"					{*lvalp= (void *) CreateOperatorItem(LT);    return LT; }
"and"					{*lvalp= (void *) CreateOperatorItem(AND);   return AND; }
"or"					{*lvalp= (void *) CreateOperatorItem(OR);    return OR; }
"not"					{*lvalp= (void *) CreateOperatorItem(NOT);   return NOT; }


"checkasn1type"			{ return FUNCTION_ISASN1TYPE; }
"buf2int"				{ return FUNCTION_BUFFER2INT; }
"int2buf"				{ return FUNCTION_INT2BUFFER; }
"ascii2int"				{ return FUNCTION_ASCII2INT; }
"hasstring"				{ return FUNCTION_HASSTRING; }
"extractstring"			{ return FUNCTION_EXTRACTSTRING; }
"ispresent"				{ return FUNCTION_ISPRESENT; }
"changebyteorder"		{ return FUNCTION_CHANGEBYTEORDER; }
"checklookuptable"		{ return FUNCTION_CHECKLOOKUPTABLE; }
"updatelookuptable"		{ return FUNCTION_UPDATELOOKUPTABLE; }
"debug"					{ return FUNCTION_DEBUG; }

%[icerrato]
"true"					{ *lvalp= (void*) CreateBooleanItem(yytext);		return OPERAND_BOOLEAN; }
"false"					{ *lvalp= (void*) CreateBooleanItem(yytext);		return OPERAND_BOOLEAN; }

[0-9]+					{ *lvalp= (void*) CreateNumberItem(yytext, 10);    return OPERAND_NUMBER; }
0x[0-9a-fA-F]+			{ *lvalp= (void *) CreateNumberItem(yytext+2, 16); return OPERAND_NUMBER; }
0b[0-1]+				{ *lvalp= (void *) CreateNumberItem(yytext+2, 2);  return OPERAND_NUMBER; }


$[0-9a-zA-Z_]+						{ *lvalp= (void *) CreateVariableItem(yytext, &TokenType); return (TokenType); }
$[0-9a-zA-Z_]+"."[0-9a-zA-Z_]+		{ *lvalp= (void *) CreateLookupTableItem(yytext, &TokenType); return (TokenType); }

#[0-9a-zA-Z_]+						{ *lvalp= (void *) CreateProtocolReferenceItem(yytext); return OPERAND_NUMBER; }

[0-9a-zA-Z_]+						{ *lvalp= (void *) CreateProtoFieldItem(yytext); return OPERAND_PROTOFIELD; }
[0-9a-zA-Z_]+("."[0-9a-zA-Z_]+)+	{ *lvalp= (void *) CreateProtoFieldItem(yytext); return OPERAND_PROTOFIELD; }

%[0-9a-zA-Z_]+						{ *lvalp= (void *) CreateAliasItem(yytext, &TokenType); return (TokenType); }


'[^']*'					{
							/* Do not allocate string item here, since strings can be used also for other purposes */
							/* (e.g. as argument in hasstring() ) and in that case we do not want to create an expression */
							/* element for them. So, we duplicate only the string value in order to avoid that */
							/* 'yytext' gets lost. */
							*lvalp= (void *) strdup(yytext); return OPERAND_STRING;
						}


"}"						{ return *yytext; }
,						{ return *yytext; }
[][:()]					{ return *yytext; }
[ \t\n\r]				;       /* skip whitespace and such*/


.           pdl_error(NULL, "Unknown character");

%%


int pdl_wrap(void)
{
    return 1;
}

void protodb_lex_init(const char * buffer)
{
	source_buffer_string= (char*) buffer;
	source_buffer= yy_scan_string(buffer);
}


void protodb_lex_cleanup()
{
	if (source_buffer)
		yy_delete_buffer(source_buffer);
	source_buffer= 0;
}

