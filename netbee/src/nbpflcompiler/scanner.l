/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


%{

#include "defs.h"
#include "pflexpression.h"
#include "compile.h"
#include "parser.hpp"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define YY_NO_INPUT
//#define YY_NO_UNPUT
static YY_BUFFER_STATE src_buffer = 0;
static uint32 col = 0; //holds the current column
static uint32 row = 1; //holds the current row


#define UPDATE_LOC \
		do{\
			llocp->first_column = col;\
			llocp->first_line = row;\
			col += strlen(yytext);\
			llocp->last_column = col;\
			llocp->last_line = row;\
		}while(0)


//NOTE: these declarations are duplicated in gramm.y, so if you modify them here you need to do the same in the other file
#define YY_DECL int pfl_lex (YYSTYPE *lvalp, YYLTYPE *llocp, struct ParserInfo *parserInfo)
int pfl_error(YYLTYPE *llocp, struct ParserInfo *parserInfo, const char *s);

%}

%option noyywrap
%option nounput
%option never-interactive

ID			[a-z][_a-z0-9]*
CHAR		[^\n]
DECNUMBER	[0-9]+
HEXNUMBER	0x[0-9a-fA-F]+
ADDRv4		[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+
MACADDR		[0-9a-fA-F][0-9a-fA-F]":"[0-9a-fA-F][0-9a-fA-F]":"[0-9a-fA-F][0-9a-fA-F]":"[0-9a-fA-F][0-9a-fA-F]":"[0-9a-fA-F][0-9a-fA-F]":"[0-9a-fA-F][0-9a-fA-F]
HINDEX		[1-9][0-9]*
and			"&&"
or			"||"
not			"!"
plus		"+"
plusplus	"++"
minus		"-"
mult		"*"
multmult	"**"
bwand		"&"
bwor		"|"
bwxor		"^"
bwnot		"~"
eq		"=="
ne		"!="
gt		">"
lt		"<"
ge		">="
le		"<="
shl		"<<"
shr		">>"
string		"\"".[^\"]*"\""
question	"\?"


%%

{DECNUMBER}		{UPDATE_LOC; strncpy(lvalp->id, yytext, ID_LEN - 1);return PFL_NUMBER;};

{HEXNUMBER}		{UPDATE_LOC; strncpy(lvalp->id, yytext, ID_LEN - 1);return PFL_HEXNUMBER;};

{MACADDR}		{UPDATE_LOC; strncpy(lvalp->id, yytext, ID_LEN - 1);return PFL_MACADDR;};

"matches"		{UPDATE_LOC; return PFL_MATCHES;};
"contains"		{UPDATE_LOC; return PFL_CONTAINS;};
"casesensitive"		{UPDATE_LOC; return PFL_SENSITIVE;};
"extractfields" 	{return PFL_EXTRACT_FIELDS;};
"returnpacket"		{UPDATE_LOC; return PFL_RETURN_PACKET;};
"retpkt"		{UPDATE_LOC; return PFL_RETURN_PACKET;};
"port"			{UPDATE_LOC; return PFL_PORT;};
"p"			{UPDATE_LOC; return PFL_PORT;}
"on"			{UPDATE_LOC; return PFL_ON;};
"and"			{UPDATE_LOC; return PFL_AND;};
"or"			{UPDATE_LOC; return PFL_OR;};
"not"			{UPDATE_LOC; return PFL_NOT;};
"in"			{UPDATE_LOC; return PFL_IN;};
"notin"			{UPDATE_LOC; return PFL_NOTIN;};
"any"			{UPDATE_LOC; return PFL_ANY;}; 
"tunneled"		{UPDATE_LOC; return PFL_TUNNELED;};
"fullencap"		{UPDATE_LOC; return PFL_FULLENCAP;};
"nosession"		{UPDATE_LOC; return PFL_NOSESSION;};
"noack"		{UPDATE_LOC; return PFL_NOACK;};
{string}		{UPDATE_LOC; memcpy( lvalp->id, &yytext[1], strlen(yytext)-2);   lvalp->id[strlen(yytext)-2]='\0';return PFL_STRING;};
{ID}			{UPDATE_LOC; strncpy(lvalp->id, yytext, ID_LEN - 1); return PFL_PROTOCOL;};
{ID}\.{ID}		{UPDATE_LOC; strncpy(lvalp->id, yytext, ID_LEN - 1); return PFL_PROTOFIELD;};
{ID}\%{HINDEX}			{UPDATE_LOC; strncpy(lvalp->id, yytext, 255); return PFL_PROTOCOL_INDEX;};
{ID}\%{HINDEX}\.{ID}	{UPDATE_LOC; strncpy(lvalp->id, yytext, 255); return PFL_PROTOFIELD_INDEX;};
{ID}\*\.{ID}    		{UPDATE_LOC; strncpy(lvalp->id, yytext, 255); return PFL_MULTIPROTOFIELD;};
{ID}\.{ID}\*			{UPDATE_LOC; strncpy(lvalp->id, yytext, 255); return PFL_PROTOMULTIFIELD;};
{ID}\%{HINDEX}\.{ID}\*	{UPDATE_LOC; strncpy(lvalp->id, yytext, 255); return PFL_PROTOMULTIFIELD_INDEX;};
{ADDRv4}		{UPDATE_LOC; strncpy(lvalp->id, yytext, ID_LEN - 1); return PFL_IPV4ADDR;};
{plus}			{UPDATE_LOC; return PFL_ADD;};
{plusplus}		{UPDATE_LOC; return PFL_ADDADD;};
{minus}			{UPDATE_LOC; return PFL_SUB;};
{question}		{UPDATE_LOC; return PFL_QUESTION;};
{mult}			{UPDATE_LOC; return PFL_MUL;};
{multmult}		{UPDATE_LOC; return PFL_MULMUL;};
{bwand}			{UPDATE_LOC; return PFL_BWAND;};
{bwor}			{UPDATE_LOC; return PFL_BWOR;};
{bwxor}			{UPDATE_LOC; return PFL_BWXOR;};
{bwnot}			{UPDATE_LOC; return PFL_BWNOT;};
{shl}			{UPDATE_LOC; return PFL_SHL;}
{shr}			{UPDATE_LOC; return PFL_SHR;}
{and}			{UPDATE_LOC; return PFL_AND;};
{or}			{UPDATE_LOC; return PFL_OR;};
{not}			{UPDATE_LOC; return PFL_NOT;};
{eq}			{UPDATE_LOC; return PFL_EQ;};
{ne}			{UPDATE_LOC; return PFL_NE;};
{gt}			{UPDATE_LOC; return PFL_GT;};
{lt}			{UPDATE_LOC; return PFL_LT;};
{ge}			{UPDATE_LOC; return PFL_GE;};
{le}			{UPDATE_LOC; return PFL_LE;};
\:			{UPDATE_LOC; return ':';};
\[			{UPDATE_LOC; return '[';};
\]			{UPDATE_LOC; return ']';};
\(			{UPDATE_LOC; return '(';};
\)			{UPDATE_LOC; return ')';};
\n			{UPDATE_LOC; row++; return '\n';};
\,			{UPDATE_LOC; return ',';};
\{			{UPDATE_LOC; return '{';};
\}			{UPDATE_LOC; return '}';};
" "|\t			{UPDATE_LOC;};
.			pfl_error(llocp, parserInfo, "unrecognized character");


%%


void pflcompiler_lex_init(const char *buf)
{
	src_buffer = yy_scan_string(buf);
}


void pflcompiler_lex_cleanup()
{
	if (src_buffer != NULL)
		yy_delete_buffer(src_buffer);
	src_buffer = NULL;
}
