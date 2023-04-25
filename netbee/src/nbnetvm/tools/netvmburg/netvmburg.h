#ifndef __MONO_MONOBURG_H__
#define __MONO_MONOBURG_H__

#include <string>
#include <set>
#include <map>
#include <list>

void yyerror (const char *fmt, ...);
int  yylex   (void);

extern FILE *inputfd;
extern FILE *outputfd;
extern std::string infile;

typedef std::map<std::string, bool> var_set_t;
extern var_set_t definedvars;

typedef struct _Rule Rule;
typedef std::list<Rule*> rule_list_t;

typedef struct _Term Term;
struct _Term{
	std::string* name;
	int number;
	int arity;
	rule_list_t rules;
};

typedef std::map<std::string, Term* > term_map_t;
typedef std::list<Term*> term_list_t;

typedef struct _NonTerm NonTerm;

struct _NonTerm {
	std::string* name;
	int number;
	rule_list_t rules; /* rules with this nonterm on the left side */
	rule_list_t chain;
	bool reached;
};

typedef std::map<std::string, NonTerm* > non_term_map_t;
typedef std::list<NonTerm*> non_term_list_t;

typedef std::list<std::string*> prefix_list_t;

typedef struct _Tree Tree;

struct _Tree {
	Term *op;
	Tree *left;
	Tree *right;
	NonTerm *nonterm; /* used by chain rules */
};

struct _Rule {
	NonTerm *lhs;
	Tree *tree;
	std::string *code;
	std::string *cost;
	std::string *cfunc;
};


Tree    *create_tree    (std::string *id, Tree *left, Tree *right);

Term    *create_term    (std::string *id, int num);

void     create_term_prefix (std::string *id);

NonTerm *nonterm        (std::string *id);

void     start_nonterm  (std::string *id);

Rule    *make_rule      (std::string *id, Tree *tree);

void     rule_add       (Rule *rule, std::string *code, std::string *cost, std::string *cfunc);

void     create_rule    (std::string *id, Tree *tree, std::string *code, std::string *cost, 
			 std::string *cfunc);

void     yyparsetail    (void);
int     yyparse(void);

void     reset_parser (void);

#endif
