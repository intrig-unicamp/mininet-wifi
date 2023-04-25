%{
/*
 * monoburg.y: yacc input grammer
 *
 * Author:
 *   Dietmar Maurer (dietmar@ximian.com)
 *
 * (C) 2001 Ximian, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#include <sstream>

#include "netvmburg.h"

#ifdef WIN32
#define snprintf _snprintf
#define strdup _strdup
#define YYMALLOC malloc
#define YYFREE free
#endif

using namespace std;
  
static int yylineno = 0;
static int yylinepos = 0;


%}

%union {
  string* text;
  int   ivalue;
  Tree  *tree;
  Rule  *rule;
  rule_list_t* rule_list;
}

%token <text> IDENT
%token <text> CODE
%token <text> STRING
%token  START
%token  COST
%token  TERM
%token  TERMPREFIX
%token <ivalue> INTEGER

%type   <tree>          tree
%type   <text>          optcost
%type   <text>          optcfunc
%type   <text>          optcode
%type   <rule>          rule
%type   <rule_list>     rule_list

%%

decls   : /* empty */ 
	| START IDENT { start_nonterm ($2); } decls
	| TERM  tlist decls
	| TERMPREFIX plist decls
	| rule_list optcost optcode optcfunc {
			//GList *tmp;
			for (rule_list_t::iterator tmp = $1->begin(); tmp != $1->end(); tmp++) {
				rule_add (*tmp, $3, $2, $4);
			}
			delete $1;
			//g_list_free ($1);
		} decls
	;

rule	: IDENT ':' tree { $$ = make_rule ($1, $3); }
	;

rule_list : rule { $$ = new rule_list_t(); $$->push_back($1);}
	| rule ',' rule_list { $$ = $3; $$->push_front($1);}
	;

optcode : /* empty */ { $$ = new string(); }
	| CODE 
	;

plist	: /* empty */
	| plist IDENT { create_term_prefix ($2);}
	;

tlist	: /* empty */
	| tlist IDENT { create_term ($2, -1);}
	| tlist IDENT '=' INTEGER { create_term ($2, $4); }
	;

tree	: IDENT { $$ = create_tree ($1, NULL, NULL); }
	| IDENT '(' tree ')' { $$ = create_tree ($1, $3, NULL); }
	| IDENT '(' tree ',' tree ')' { $$ = create_tree ($1, $3, $5); }
	;

optcost : /* empty */ {$$ = new string(); }
	| STRING
	| INTEGER { 
		ostringstream res;
		res << $1;
	$$ = new string(res.str());}
	//g_strdup_printf ("%d", $1); }//
	;

optcfunc : /*empty */ { $$ = new string(); }
	 | COST CODE { $$ = $2; }
	 ;
%%

static char input[2048];
static char *Next = input;

void 
yyerror (const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  fprintf (stderr, "line %d(%d): ", yylineno, yylinepos);
  vfprintf (stderr, fmt, ap);
  fprintf(stderr, "\n");

  va_end (ap);

  exit (-1);
}

static int state = 0;

void
reset_parser ()
{
  state = 0;
}

struct pplist {
  struct pplist *next;
  bool ignore;
};

static struct pplist *pp = NULL;

static char*
getvar (const char *input)
{
    char *var = strdup(input); //g_strchug (g_strdup (input));
	while (isspace(*var))
		var++;
    char *ptr;

    for (ptr = var; *ptr && *ptr != '\n'; ++ptr) {
        if (isspace(*ptr)) {
            break;
        }
    }
    *ptr = '\0';

    return var;
}

static void
push_if (char *input, bool flip)
{
  struct pplist *new_pp = new pplist();//g_new (struct pplist, 1);
  char *var = getvar (input);

  new_pp->ignore = (definedvars[var] == false) ^ flip;// (g_hash_table_lookup (definedvars, var) == NULL) ^ flip;
  new_pp->next = pp;

  new_pp->ignore |= (pp ? pp->ignore : 0);
  pp = new_pp;
  //g_free (var);
}

static void
flip_if ()
{
  if (!pp)
      yyerror ("%%else without %%if");

  pp->ignore = !pp->ignore | (pp->next ? pp->next->ignore : 0);
}

static void
pop_if ()
{
  struct pplist *prev_pp = pp;

  if (!pp)
      yyerror ("%%endif without %%if");

  pp = pp->next;
  delete prev_pp;
  //g_free (prev_pp);
}

static char
nextchar ()
{
	int next_state ;
	bool ll;

	if (!*Next) {
		Next = input;
		*Next = 0;
		do {
			if (!fgets (input, sizeof (input), inputfd))
				return 0;

			ll = (input [0] == '%' && input [1] == '%');
			next_state = state;

			if (state == 1) {
				if (!ll && input [0] == '%') {
					if (!strncmp (&input [1], "ifdef", 5)) {
						push_if (&input [6], false);
						ll = true;
						continue;
					}
					else if (!strncmp (&input [1], "ifndef", 6)) {
						push_if (&input [7], true);
						ll = true;
						continue;
					}
					else if (!strncmp (&input [1], "else", 4)) {
						flip_if ();
						ll = true;
						continue;
					}
					else if (!strncmp (&input [1], "endif", 5)) {
						pop_if ();
						ll = true;
						continue;
					}
				}
				if (pp && pp->ignore) {
					ll = true;
					continue;
				}
			}

			switch (state) {
				case 0:
					if (ll) {
						next_state = 1;
					} else
						fputs (input, outputfd);
					break;
				case 1:
					if (ll) {
						next_state = 2;
						*Next = 0;
					}
					break;
				default:
					return 0;
			}
			ll = state != 1 || input[0] == ';';
			state = next_state;
			yylineno++;
		} while (next_state == 2 || ll);
	} 

	return *Next++;
}

void
yyparsetail (void)
{
  fputs (input, outputfd);
  while (fgets (input, sizeof (input), inputfd))
    fputs (input, outputfd);
  input [0] = '\0';
}

int 
yylex (void) 
{
  char c;

  do {

    if (!(c = nextchar ()))
      return 0;

    yylinepos = Next - input + 1;

    if (isspace (c))
      continue;

    if (c == '%') {
      if (!strncmp (Next, "start", 5) && isspace (Next[5])) {
	Next += 5;
	return START;
      }

      if (!strncmp (Next, "termprefix", 10) && isspace (Next[10])) {
	Next += 10;
	return TERMPREFIX;
      }

      if (!strncmp (Next, "term", 4) && isspace (Next[4])) {
	Next += 4;
	return TERM;
      }
      return c;
    }

    if (isdigit (c)) {
	    int num = 0;

	    do {
		    num = 10*num + (c - '0');
	    } while (isdigit (c = (*Next++)));

	    yylval.ivalue = num;
	    return INTEGER;
    }

    if (isalpha (c)) {
      char *n = Next;
      int l;

      if (!strncmp (Next - 1, "cost", 4) && isspace (Next[3])) {
	Next += 4;
	return COST;
      }

      while (isalpha (*n) || isdigit (*n) || *n == '_') 
	      n++;

      l = n - Next + 1;
      yylval.text = new string (Next - 1, l);
      Next = n;
      return IDENT;
    }
    
    if (c == '"') {
      int i = 0;
      static char buf [100000];
 
      while ((c = *Next++) != '"' && c)
	buf [i++] = c;
      
      buf [i] = '\0';
      yylval.text = new string(buf);

      return STRING;
    }

    if (c == '{') {
      unsigned int i = 0, d = 1;
      static char buf [100000];
	  buf[0] = '\0';

	  //snprintf(buf, 100000, "#line %d \"%s\"\n", yylineno, infile.c_str());
	  i = strlen(buf);
 
      while (d && (c = nextchar ())) {
	buf [i++] = c;
	assert (i < sizeof (buf));

	switch (c) {
	case '{': d++; break;
	case '}': d--; break;
	default:
		break;
	}
      }
      buf [--i] = '\0';
      yylval.text = new string(buf);

      return CODE;
    }
    
    return c;
  
  } while (1);
}


