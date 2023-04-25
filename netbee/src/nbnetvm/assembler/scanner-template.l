/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


%option batch
%option nounput

char      [^\r\n]
decnumber [0-9]+
hexnumber 0x[0-9a-fA-F]+
var       [a-zA-Z_][a-zA-Z_0-9]*
address   [a-z_0-9]+(\.[a-z_0-9]*)*
dotquad	  ([0-9]{1,3}\.){3}([0-9]{1,3})
colquad	  ([0-9a-fA-F]{2}:){3}([0-9a-fA-F]{2})
newline   (\r|\n|\r\n)
string    "\""[^\"]*"\""

%{

#include <nbnetvm.h>
#include "nvm_gramm.tab.h"
#include "../opcodes.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef WIN32
#define fileno _fileno
#endif

#define YY_NO_INPUT

#ifdef FLEX_SCANNER
#define YY_NO_UNPUT
static YY_BUFFER_STATE in_buffer;
#else
static char *in_buffer;

#undef getc
#define getc(fp)  (*in_buffer == 0 ? EOF : *in_buffer++)
#endif

extern int eline;
extern int nvmparser_error(const char *s);

#define YY_NO_UNPUT
static YY_BUFFER_STATE in_buffer;

int32_t str2int(const char *s, uint32_t *num, uint8_t base);

%}

%%

segment			return SEGMENT;
ends			return ENDS;
ip_host			return T_HOST;
tcp_port		return T_PORT;
\.locals		return LOCALS;
\.maxstacksize		return STACKSIZE;
\.byte_order		return BYTEORDER;

\.datamem_size		return DATAMEMSIZE;
\.netpe_name		return NETPENAME;
\.info_size		return INFOPARTSIZE;
\.use_coprocessor	return COPRO_DECL;

!!opcodes		//Insert here opcodes dynamically generated from opcodes.def

push_input		{nvmparser_lval.number = (nvmPORT_EXPORTER | nvmCONNECTION_PUSH); return PORT_TYPE;}
push_output		{nvmparser_lval.number = (nvmPORT_COLLECTOR | nvmCONNECTION_PUSH); return PORT_TYPE;}
pull_input		{nvmparser_lval.number = (nvmPORT_EXPORTER | nvmCONNECTION_PULL); return PORT_TYPE;}
pull_output		{nvmparser_lval.number = (nvmPORT_COLLECTOR | nvmCONNECTION_PULL); return PORT_TYPE;}
use_coprocessor		{return COPRO_DECL;}
db			return T_DB;
dw			return T_DW;
dd			return T_DD;
equ			return T_EQU;
default			return DEFAULTCASE;
\.ports			{strcpy(nvmparser_lval.id,yytext); return S_PORT;}
\.data			{strcpy(nvmparser_lval.id,yytext); return S_DATA;}
\.metadata		{strcpy(nvmparser_lval.id,yytext); return S_METADATA;}
{decnumber}		{uint32_t n;
					if (str2int(yytext, &n, 10) != 0)
						nvmparser_error("decimal number out of range");
					nvmparser_lval.number = n;
					return NUMBER;
				}

{hexnumber}		{uint32_t n;
					if (str2int(yytext, &n, 16) != 0)
						nvmparser_error("decimal number out of range");
					nvmparser_lval.number = n;
					return NUMBER;
				}
{dotquad}		{uint32_t n, t, i; char dotquad[16], *c; // size_t len;
					/* 12345678901234567890
					   255.255.255.255
					   max length: 15
					 */

					/* printf("matched text: '%s'\n", yytext); */
					// len = strlen(yytext);
					n = 0;
					strncpy(dotquad, yytext, sizeof(dotquad));
					dotquad[sizeof(dotquad)-1] = '\0';

					/* XXX: FIXME: Usage of strtok!!! */
					c = strtok(dotquad, ".");
					if(str2int(c, &n, 10) != 0 || n > 255) {
						/* printf("offending string: '%s'\n", c); */
						nvmparser_error("dotted quad notation (first pass) wrong or out of range");
					}

					/* printf("dotted quad: %u\n", n); */

					for(i = 0; i < 3 && (c = strtok(NULL, ".")) ; ++i) {
						if(str2int(c, &t, 10) != 0 || t > 255) {
							/* printf("offending string: '%s', pass: %u\n", c, i); */
							nvmparser_error("dotted quad notation wrong or out of range");
						}
						n <<= 8;
						n |= t;
						/* printf("dotted quad: %u\n", n); */
					}
					nvmparser_lval.number = n;
					return NUMBER;
				}

{colquad}		{uint32_t n, t, i; char colquad[12], *c; // size_t len;
					/* 12345678901234567890
					   00:00:00:00
					   max length: 11
					 */

					/* printf("matched text: '%s'\n", yytext); */
					// len = strlen(yytext);
					n = 0;
					strncpy(colquad, yytext, sizeof(colquad));
					colquad[sizeof(colquad)-1] = '\0';

					/* XXX: FIXME: Usage of strtok!!! */
					c = strtok(colquad, ":");
					if(str2int(c, &n, 16) != 0 || n > 255) {
						/* printf("offending string: '%s'\n", c); */
						nvmparser_error("colon quad notation (first pass) wrong or out of range");
					}

					/* printf("colon quad: %u\n", n); */

					for(i = 0; i < 3 && (c = strtok(NULL, ":")) ; ++i) {
						if(str2int(c, &t, 16) != 0 || t > 255) {
							/* printf("offending string: '%s', pass: %u\n", c, i); */
							nvmparser_error("colon quad notation wrong or out of range");
						}
						n <<= 8;
						n |= t;
						/* printf("colon quad: %u\n", n); */
					}
					nvmparser_lval.number = n;
					return NUMBER;
				}
{string}   		{
				if (yyleng - 2 > 1024 - 1)
						printf ("String too long!!!\n");
				strncpy (nvmparser_lval.id, yytext + 1, yyleng - 2);  /* Copy trimming away quotes */
				nvmparser_lval.id[yyleng - 2] = '\0';
				return (STRING);
			}


{var}			{/*printf ("LABEL: %s\n", yytext);*/ strcpy(nvmparser_lval.id,yytext); return LABEL;};
\.{var}			{strcpy(nvmparser_lval.id,yytext); return IDENTIFIER;};
{address}		{/*printf ("ADDR!\n");*/ strcpy(nvmparser_lval.id,yytext); return T_ADDR;}
\;{char}*{newline}					eline++; //remove comments
\;{char}*{newline}(\'{newline})		eline+=2; //remove comments - Ivano: this happens with the official NetPDL. In fact there is a swith in HTTP with matches the character 0xD, i.e. new line.
\(				{return('(');}
\)				{return(')');}
\*				return('*');
\/				return('/');
\+				return('+');
\-				return('-');
\:				return ':';
\,				return ',';
{newline}		eline++;
" "|\t			;
.				nvmparser_error("Invalid character:"); {printf("\t%s\n",yytext);}

%%

void netvm_lex_init(char *buf)
{
	in_buffer = nvmparser__scan_string(buf);
}

/*
 * Do any cleanup necessary after parsing.
 */
void netvm_lex_cleanup()
{
	if (in_buffer != NULL)
		nvmparser__delete_buffer(in_buffer);
	in_buffer = NULL;
}


int nvmparser_wrap() { return 1; }


int32_t str2int(const char *s, uint32_t *num, uint8_t base)
{
uint32_t n;
char *endp;

	n = strtoul(s, &endp, base);
	if ((*endp != '\0') || (errno == ERANGE))
		return -1;

	*num = n;

	return 0;

}
