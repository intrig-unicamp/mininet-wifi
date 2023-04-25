/********************************************************/
/* this is the modified version of Monoburg to create   */
/* code that is indipendent from glib and contains some */
/* definition from the NetVM environment				*/
/********************************************************/



/*
 * netvmburg.c: an iburg like code generator generator
 *
 * Author:
 *   Dietmar Maurer (dietmar@ximian.com)
 *
 * (C) 2001 Ximian, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sstream>
#include <cassert>
#include <vector>
#include <iostream>
#include "netvmburg.h"

#define PREFIX (cpp_class ? cpp_class : "")

using namespace std;

//extern void yyparse (void);

static term_map_t term_hash;
static term_list_t term_list;
static non_term_map_t nonterm_hash;
static non_term_list_t nonterm_list;
static rule_list_t rule_list;
static prefix_list_t prefix_list;

FILE *inputfd;
FILE *outputfd;
string infile;
var_set_t definedvars;
static FILE *deffd;
static FILE *cfd;

static int dag_mode = 0;
static char *cpp_class = NULL;
static int predefined_terms = 0;
static int default_cost = 1;

static void output (const char *fmt, ...) 
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf (outputfd, fmt, ap);
	va_end (ap);
}

Rule*
make_rule (std::string* id, Tree *tree)
{
	Rule *rule = new Rule();
	rule->lhs = nonterm (id);
	rule->tree = tree;

	return rule;
}

void
rule_add (Rule *rule, std::string *code, std::string *cost, std::string *cfunc)
{
	ostringstream cost_stream;

	if (cfunc->empty() && cost->empty())
		cost_stream << default_cost;

	rule_list.push_back(rule);
	rule->cost = new string(cost_stream.str());
	rule->cfunc = cfunc;
	rule->code = code;

	if (!cfunc->empty()) {
		if (!cost->empty())
			yyerror ("duplicated costs (constant costs and cost function)");
		else {
			ostringstream cost_stream2;

			if (dag_mode)
			{
				cost_stream2 << "netvm_burg_cost_" << rule_list.size() << " (p, data)";
				rule->cost = new string(cost_stream2.str());
			}
			else
			{
				cost_stream2 << "netvm_burg_cost_" << rule_list.size() << " (tree, data)";
				rule->cost = new string(cost_stream2.str());
			}
		}
	}

	rule->lhs->rules.push_back(rule);

	if (rule->tree->op)
		rule->tree->op->rules.push_back(rule);
	else 
		rule->tree->nonterm->chain.push_back(rule);
}

void     
create_rule (std::string* id, Tree *tree, std::string* code, std::string* cost, std::string* cfunc)
{
	Rule *rule = make_rule (id, tree);

	rule_add (rule, code, cost, cfunc);
}

Tree *
create_tree (std::string* id, Tree *left, Tree *right)
{
	int arity = (left != NULL) + (right != NULL);
	Term *term = NULL; 
	Tree *tree = new Tree();

	term = term_hash[*id];

	/* try if id has termprefix */
	tree->nonterm = NULL;
	tree->op = NULL ;
	tree->left = tree->right = NULL;

	if (!term) {
		for(prefix_list_t::iterator pl = prefix_list.begin(); pl != prefix_list.end(); pl++)
		{
			if( *pl == id)
			{
				term = create_term (id, -1);
				break;
			}
			
		}
	}

	if (term) {
		if (term->arity == -1)
			term->arity = arity;

		if (term->arity != arity)
			yyerror ("changed arity of terminal %s from %d to %d",
				 id->c_str(), term->arity, arity);

		tree->op = term;
		tree->left = left;
		tree->right = right;
	} else {
		tree->nonterm = nonterm (id);
	}

	return tree;
}

static void
check_term_num (std::string key, Term *value, int num)
{
	if (num != -1 && value->number == num)
		yyerror ("duplicate terminal id \"%s\"", key.c_str());
}
 
void  
create_term_prefix (std::string* id)
{
	if (!predefined_terms)
		yyerror ("%termprefix is only available with -p option");

	prefix_list.push_front(id);
}

Term *
create_term (std::string* id, int num)
{
	Term *term;

	if (!predefined_terms && !nonterm_list.empty())
		yyerror ("terminal definition after nonterminal definition");

	if (num < -1)
		yyerror ("invalid terminal number %d", num);

	for(term_map_t::iterator i = term_hash.begin(); i != term_hash.end(); i++)
	{
		check_term_num(i->first, i->second, num);
	}

	term = new Term();

	term->name = id;
	term->number = num;
	term->arity = -1;

	term_list.push_back(term);

	term_hash[*term->name] = term;

	return term;
}

NonTerm *
nonterm (std::string* id)
{
	NonTerm *nterm;

	if ((nterm = nonterm_hash[*id]) != NULL)
		return nterm;

	nterm = new NonTerm();

	nterm->name = id;
	nonterm_list.push_back(nterm);
	nterm->number = nonterm_list.size();
	
	nonterm_hash[*nterm->name] = nterm;

	return nterm;
}

void
start_nonterm (string* id)
{
	static bool start_def = false;
	
	if (start_def)
		yyerror ("start symbol redeclared");
	
	start_def = true;
	nonterm (id); 
}

/*
static void
emit_struct_offset (void)
{

	output("typedef struct NetVMStateFieldsOffsets\n");
	output("{\n");
	output("	uint32_t	SharedOffs;\n");
	output("} NetVMStateFieldsOffsets;\n");
	output("\n");

	output("typedef struct nvmPortStateFieldsOffsets\n");
	output("{\n");
	output("	uint32_t	PortIdxOffs;\n");
	output("	uint32_t	PEStateOffs;\n");
	output("	uint32_t	ExchangeBufOffs;\n");
	output("} nvmPortStateFieldsOffsets;\n");
	output("\n");

	output("typedef struct nvmPEStateFieldsOffsets\n");
	output("{\n");
	output("	uint32_t	DataMemOffs;\n");
	output("	uint32_t	SharedMemOffs;\n");
	output("	uint32_t	DataMemLenOffs;\n");
	output("	uint32_t	SharedMemLenOffs;\n");
	output("	uint32_t   CoprocTableOffs;\n");
	output("	uint32_t   NetVMPointerOffs;\n");
	output("} nvmPEStateFieldsOffsets;\n");
	output("\n");

	output("typedef struct nvmExchBuffFieldsOffsets\n");
	output("{\n");
	output("	uint32_t	DataPtrOffs;\n");
	output("	uint32_t	BuffLenOffs;\n");
	output("} nvmExchBuffFieldsOffsets;\n");
	output("\n");

	output("typedef struct nvmCoprocFieldOffsets\n");
	output("{\n");
	output("	uint32_t regsOffs;\n");
	output("	uint32_t init;\n");
	output("	uint32_t write;\n");
	output("	uint32_t read;\n");
	output("	uint32_t invoke;\n");
	output("	uint32_t xbuff;\n");
	output("} nvmCoprocFieldOffsets;\n");
	output("\n");

	output("typedef struct nvmStructsOffsets\n");
	output("{\n");
	output("	nvmPortStateFieldsOffsets	PortState;\n");
	output("	nvmPEStateFieldsOffsets	PEState;\n");
	output("	nvmExchBuffFieldsOffsets	ExchBuffer;\n");
	output("	nvmCoprocFieldOffsets		CopState;\n");
	output("	NetVMStateFieldsOffsets		NetVMState;\n");
	output("} nvmStructsOffsets;\n");

	output("typedef struct	nvmPEHandlerStateOffsets\n");
	output("{\n");
	output("	uint32_t PEState;\n");
	output("}nvmPEHandlerStateOffsets;\n");
	output("\n");
	output("typedef struct nvmPEStateOffsets\n");
	output("{\n");
	output("	uint32_t DataMem;\n");
	output("	uint32_t SharedMem;\n");
	output("	uint32_t CoprocTable;\n");
	output("} nvmPEStateOffsets;\n");
	output("\n");
	output("typedef struct nvmMemDescriptorOffsets\n");
	output("{\n");
	output("	uint32_t Base;\n");
	output("}nvmMemDescriptorOffsets; \n");
	output("\n");
	output("	typedef struct nvmExchangeBufferOffsets\n");
	output("	{\n");
	output("		uint32_t PacketBuffer;\n");
	output("	uint32_t InfoData;\n");
	output("	uint32_t PacketLen;\n");
	output("	uint32_t InfoLen;\n");
	output("	} nvmExchangeBufferOffsets;\n");
	output("\n");
	output("	typedef struct nvmNetStructOffset\n");
	output("	{\n");
	output("		nvmExchangeBufferOffsets exchangebufoffset;\n");
	output("	nvmPEHandlerStateOffsets		pestatehandleroffset;\n");
	output("	nvmPEStateOffsets				pestateoffset;\n");
	output("	nvmMemDescriptorOffsets			memdescriptoroffset;\n");
	output("	nvmCoprocFieldOffsets			copstate;\n");
	output("    uint32_t CoproStateSize;\n");
	output("	} nvmNetStructsOffsets;\n");

	void Set_NetVM_Structs_Offsets(nvmStructsOffsets *structsOffs)
	{

	nvmExchangeBufferState	ExchBufState;
	nvmInternalPort			PortState;
	nvmPEState				PEState;


	structsOffs->ExchBuffer.BuffLenOffs = ((uint32_t)&ExchBufState.local_buffer_length) - ((uint32_t)&ExchBufState);
	structsOffs->ExchBuffer.DataPtrOffs = ((uint32_t)&ExchBufState.pkt_pointer) -((uint32_t) &ExchBufState);
	structsOffs->PEState.DataMemLenOffs = ((uint32_t)&PEState.dsl) - ((uint32_t)&PEState);
	structsOffs->PEState.DataMemOffs = ((uint32_t)&PEState.datamem) - ((uint32_t)&PEState);
	structsOffs->PEState.SharedMemLenOffs = ((uint32_t)&PEState.ssl) - ((uint32_t)&PEState);
	structsOffs->PortState.ExchangeBufOffs = ((uint32_t)&PortState.exchange_buffer) - ((uint32_t)&PortState);
	structsOffs->PortState.PEStateOffs = ((uint32_t)&PortState.PE_pointer) - ((uint32_t)&PortState);
	structsOffs->PortState.PortIdxOffs = ((uint32_t)&PortState.idx) - ((uint32_t)&PortState);

	
}*/

static void
emit_tree_string (Tree *tree)
{
	if (tree->op) {
		output ("%s", tree->op->name->c_str());
		if (tree->op->arity) {
			output ("(");
			emit_tree_string (tree->left);
			if (tree->right) {
				output (", ");
				emit_tree_string (tree->right);
			}
			output (")");
		}
	} else 
		output ("%s", tree->nonterm->name->c_str());
}

static void
emit_rule_string (Rule *rule, const char *fill)
{
	output ("%s/* ", fill);
	
	output ("%s: ", rule->lhs->name->c_str());

	emit_tree_string (rule->tree);

	output (" */\n");
}

static int
next_term_num ()
{
	int i = 1;

	term_list_t::iterator l = term_list.begin();

	while (l != term_list.end()) {
		Term *t = *l;
		if (t->number == i) {
			l = term_list.begin();
			i++;
		} else
			l++;
	}
	return i;
}

struct term_compare_func
{
	bool operator()(Term *t1, Term* t2)
	{
		return t1->number < t2->number;
	}
};

/*
static int
term_compare_func (Term *t1, Term *t2)
{
	return t1->number - t2->number;
}
*/

static void
emit_header ()
{
	output ("#include <stdlib.h>\n");
	output ("\n");

	output ("#ifndef MBTREE_TYPE\n#error MBTREE_TYPE undefined\n#endif\n");
	output ("#ifndef MBREG_TYPE\n#error MBREG_TYPE undefined\n#endif\n");
	output ("#ifndef MBTREE_OP\n#define MBTREE_OP(t) ((t)->Code)\n#endif\n");
	output ("#ifndef MBTREE_LEFT\n#define MBTREE_LEFT(t) ((t)->kids[0])\n#endif\n");
	output ("#ifndef MBTREE_RIGHT\n#define MBTREE_RIGHT(t) ((t)->kids[1])\n#endif\n");
	output ("#ifndef MBTREE_STATE\n#define MBTREE_STATE(t) ((t)->State)\n#endif\n");
	output ("#ifndef MBTREE_VALUE\n#define MBTREE_VALUE(t) ((t)->Value)\n#endif\n");
	output ("#ifndef MBCGEN_TYPE\n#define MBCGEN_TYPE int\n#endif\n");
	output ("#ifndef MBALLOC_STATE\n#define MBALLOC_STATE CALLOC (1, sizeof(MBState))\n#endif\n");
	output ("#ifndef MBCOST_DATA\n#define MBCOST_DATA void*\n#endif\n");
	output ("\n");
	output ("#define MBMAXCOST 32768\n");

	output ("\n");
	output ("#define MBCOND(x) if (!(x)) return MBMAXCOST;\n");

	output ("\n");

	for (term_list_t::iterator l = term_list.begin(); l != term_list.end(); l++) {
		Term *t = *l;
		if (t->number == -1)
			t->number = next_term_num ();
	}

	term_list.sort(term_compare_func());

	for (term_list_t::iterator l = term_list.begin(); l != term_list.end(); l++) {
		Term *t = *l;
		if (t->number == -1)
			t->number = next_term_num ();

		if (predefined_terms)
			output ("#define MB_TERM_%s\t %s\n", t->name->c_str(), t->name->c_str());
		else
			output ("#define MB_TERM_%s\t %d\n", t->name->c_str(), t->number);

	}
	output ("\n");

}

static void
emit_nonterm ()
{

	for (non_term_list_t::iterator l = nonterm_list.begin(); l != nonterm_list.end(); l++) {
		NonTerm *n = *l;
		output ("#define MB_NTERM_%s\t%d\n", n->name->c_str(), n->number);
	}
	output ("#define MB_MAX_NTERMS\t%d\n", nonterm_list.size());
	output ("\n");
}

static void
emit_errors()
{
	output ("#define INSSEL_SUCCESS					0\n");
	output ("#define INSSEL_ERROR_ARITY				1\n");
	output ("#define INSSEL_ERROR_NTERM_NOT_FOUND	2\n");
	output ("#define INSSEL_ERROR_RULE_NOT_FOUND		3\n");
	output ("\n");
}

static void
emit_class_end()
{
	if(cpp_class)
	{
		output("};\n");
	}
}

static void
emit_class_declaration()
{
	if(cpp_class)
	{
		output("class %s\n", cpp_class);
		output("{\n");
		output("public:\n");
		output("typedef MBTREE_TYPE MBTree; \n");
		output("typedef MBCOST_DATA MBCostData; \n");
		output("typedef struct %s_MBState MBState; \n", PREFIX);
		//output("typedef IR IR; \n");
	}
}

static void
emit_state ()
{
	int i, j;

	output ("typedef struct %s_MBState MBState;\n", PREFIX);
	output ("struct %s_MBState {\n", PREFIX);
	output ("\tint\t\t op;\n");

	if (dag_mode) {
		output ("\tMBTREE_TYPE\t *tree;\n");
		output ("\tMBREG_TYPE reg1, reg2;\n");
	}
	
	output ("\tMBState\t\t*left, *right;\n");
	output ("\tuint16_t\t\tcost[%d];\n", nonterm_list.size() + 1);

	for (non_term_list_t::iterator l = nonterm_list.begin(); l != nonterm_list.end(); l++) {
		NonTerm *n = *l;
		assert(n->rules.size() < 256);
		i = n->rules.size();
		j = 1;
		while (i >>= 1)
			j++;
		output ("\tunsigned int\t rule_%s:%d;\n",  n->name->c_str(), j); 
	}
	output ("};\n\n");
}

static void
emit_decoders ()
{
	for (non_term_list_t::iterator l = nonterm_list.begin(); l != nonterm_list.end(); l++) {
		NonTerm *n = *l;
		output ("static const int netvm_burg_decode_%s[] = {\n", n->name->c_str());
		output ("\t0,\n");

		for(rule_list_t::iterator rl = n->rules.begin(); rl != n->rules.end(); rl++) {
			//Rule *rule = *rl;
			int index = 0;
			for(rule_list_t::iterator t = rule_list.begin(); t != rule_list.end(); t++)
			{
				if(*t == *rl)
					break;
				index++;
			}
			output ("\t%d,\n", index + 1);
		}
		
		output ("};\n\n");
	}
}

static void
emit_tree_match (string *st, Tree *t)
{
	bool not_first = !(*st ==  "p->");

	/* we can omit this check at the top level */
	if (not_first) {
		if (predefined_terms)
			output ("\t\t\t%sop == %s /* %s */", st->c_str(), t->op->name->c_str(), t->op->name->c_str());
		else
			output ("\t\t\t%sop == %d /* %s */", st->c_str(), t->op->number, t->op->name->c_str());
	}

	if (t->left && t->left->op) {
		string tn = *st + "left->";
		if (not_first)
			output (" &&\n");
		not_first = true;
		emit_tree_match (&tn, t->left);
	}

	if (t->right && t->right->op) {
		string tn = *st + "right->";
		if (not_first)
			output (" &&\n");
		emit_tree_match (&tn, t->right);
	}
}

static void
emit_rule_match (Rule *rule)
{
	Tree *t = rule->tree; 

	if ((t->left && t->left->op) || 
	    (t->right && t->right->op)) {	
		string s("p->");
		output ("\t\tif (\n");
		emit_tree_match (&s, t);
		output ("\n\t\t)\n");
	}
}

static void
emit_costs (string st, Tree *t)
{

	if (t->op) {

		if (t->left) {
			string tn = st + "left->";
			emit_costs (tn, t->left);
		}

		if (t->right) {
			string tn = st + "right->";
			emit_costs (tn, t->right);
		}
	} else
		output ("%scost[MB_NTERM_%s] + ", st.c_str(), t->nonterm->name->c_str());
}

static void
emit_cond_assign (Rule *rule, string cost, string fill)
{
    string rc(!cost.empty() ? "c + " + cost : "c");


	output ("%sif (%s < p->cost[MB_NTERM_%s]) {\n", fill.c_str(), rc.c_str(), rule->lhs->name->c_str());

	output ("%s\tp->cost[MB_NTERM_%s] = %s;\n", fill.c_str(), rule->lhs->name->c_str(), rc.c_str());

	int index = 0;
	for(rule_list_t::iterator f = rule->lhs->rules.begin(); f != rule->lhs->rules.end(); f++)
	{
		if(*f == rule)
			break;
		index++;
	}
	output ("%s\tp->rule_%s = %d;\n", fill.c_str(), rule->lhs->name->c_str(), index + 1);
		//g_list_index (rule->lhs->rules, rule) + 1);

	if (!rule->lhs->chain.empty())
		output ("%s\tclosure_%s (p, %s);\n", fill.c_str(), rule->lhs->name->c_str(), rc.c_str()); 

	output ("%s}\n", fill.c_str());
}

static void
emit_label_func ()
{
	int i;

	if (dag_mode) {
		output ("void\n");
		output ("netvm_burg_label_priv (MBTREE_TYPE *tree, MBCOST_DATA *data, MBState *p) {\n");
	} else {
		output ("MBState *\n");
		output ("%snetvm_burg_label_priv (MBTREE_TYPE *tree, MBCOST_DATA *data) {\n", PREFIX);
	}

	output ("\tint arity;\n");
	output ("\tint c;\n");
	if (!dag_mode) 
		output ("\tMBState *p;\n");
	output ("\tMBState *left = NULL, *right = NULL;\n\n");

	output ("\tswitch (netvm_burg_arity [MBTREE_OP(tree)]) {\n");
	output ("\tcase 0:\n");
	output ("\t\tbreak;\n");
	output ("\tcase 1:\n");
	if (dag_mode) {
		output ("\t\tleft = MBALLOC_STATE;\n");
		output ("\t\tnetvm_burg_label_priv (MBTREE_LEFT(tree), data, left);\n");		
	} else {
		output ("\t\tleft = netvm_burg_label_priv (MBTREE_LEFT(tree), data);\n");
		output ("\t\tright = NULL;\n");
	}
	output ("\t\tbreak;\n");
	output ("\tcase 2:\n");
	if (dag_mode) {
		output ("\t\tleft = MBALLOC_STATE;\n");
		output ("\t\tnetvm_burg_label_priv (MBTREE_LEFT(tree), data, left);\n");		
		output ("\t\tright = MBALLOC_STATE;\n");
		output ("\t\tnetvm_burg_label_priv (MBTREE_RIGHT(tree), data, right);\n");		
	} else {
		output ("\t\tleft = netvm_burg_label_priv (MBTREE_LEFT(tree), data);\n");
		output ("\t\tright = netvm_burg_label_priv (MBTREE_RIGHT(tree), data);\n");
	}
	output ("\t}\n\n");

	output ("\tarity = (left != NULL) + (right != NULL);\n");
	//output ("\tg_assert (arity == netvm_burg_arity [MBTREE_OP(tree)]);\n\n");
	output ("\tif (arity != netvm_burg_arity [MBTREE_OP(tree)])\n\t{\n\t\terror = INSSEL_ERROR_ARITY;\n\t\treturn NULL;\n\t}\n\n");

	if (!dag_mode)
		output ("\tp = MBALLOC_STATE;\n");

	output ("\tmemset (p, 0, sizeof (MBState));\n");
	output ("\tp->op = MBTREE_OP(tree);\n");
	output ("\tp->left = left;\n");
	output ("\tp->right = right;\n");

	if (dag_mode)
		output ("\tp->tree = tree;\n");	
	
	i = 0;
	for (non_term_list_t::iterator l = nonterm_list.begin(); l != nonterm_list.end(); l++) {
		output ("\tp->cost [%d] = 32767;\n", ++i);
	}
	output ("\n");

	output ("\tswitch (MBTREE_OP(tree)) {\n");
	for (term_list_t::iterator l = term_list.begin(); l != term_list.end(); l++) {
		Term *t = *l;
		
		if (predefined_terms)
			output ("\tcase %s: /* %s */\n", t->name->c_str(), t->name->c_str());
		else
			output ("\tcase %d: /* %s */\n", t->number, t->name->c_str());

		for(rule_list_t::iterator rl = t->rules.begin(); rl != t->rules.end(); rl ++){
			Rule *rule = *rl; 
			Tree *t = rule->tree;

			emit_rule_string (rule, "\t\t");

			emit_rule_match (rule);
			
			output ("\t\t{\n");

			output ("\t\t\tc = ");
			
			emit_costs (string(), t);
	
			output ("%s;\n", rule->cost->c_str());

			emit_cond_assign (rule, string(), "\t\t\t");

			output ("\t\t}\n");
		}

		output ("\t\tbreak;\n");
	}
	
	output ("\tdefault:\n");
	output ("#ifdef MBGET_OP_NAME\n");
	output ("\t\tprintf (\"unknown operator: %%s\\n\", MBGET_OP_NAME(MBTREE_OP(tree)));\n");
	output ("#else\n");
	output ("\t\tprintf (\"unknown operator: 0x%%04x\", MBTREE_OP(tree));\n");
	output ("#endif\n");
	output ("\t}\n\n");

	if (!dag_mode) {
		output ("\tMBTREE_STATE(tree) = p;\n");
		output ("\treturn p;\n");
	}

	output ("}\n\n");

	output ("MBState *\n");
	output ("%snetvm_burg_label (MBTREE_TYPE *tree, MBCOST_DATA *data)\n{\n", PREFIX);
	if (dag_mode) {
		output ("\tMBState *p = MBALLOC_STATE;\n");
		output ("\tnetvm_burg_label_priv (tree, data, p);\n");		
	} else {
		output ("\tMBState *p = netvm_burg_label_priv (tree, data);\n");
	}
	output ("\tif ( error == INSSEL_ERROR_ARITY ) return NULL;\n");
	output ("\treturn p->rule_%s ? p : NULL;\n", nonterm_list.front()->name->c_str());
	output ("}\n\n");
}

static string
compute_kids (string ts, Tree *tree, int *n)
{
	ostringstream res;

	if (tree->nonterm) {
		res << "\t\tkids[" << (*n)++ << "] = " << ts << ";\n";
		return res.str();;
	} else if (tree->op && tree->op->arity) {
		//char *res2 = NULL;

		if (dag_mode) {
			res << compute_kids ( ts + "->left" , //g_strdup_printf ("%s->left", ts), 
					    tree->left, n);
			if (tree->op->arity == 2)
				res << compute_kids (ts + "->right", //g_strdup_printf ("%s->right", ts), 
						     tree->right, n);
		} else {
			res << compute_kids ("MBTREE_LEFT(" + ts + ")",// g_strdup_printf ("MBTREE_LEFT(%s)", ts), 
					    tree->left, n);
			if (tree->op->arity == 2)
				res << compute_kids ("MBTREE_RIGHT(" + ts + ")", //g_strdup_printf ("MBTREE_RIGHT(%s)", ts), 
						     tree->right, n);
		}

		return res.str(); // g_strconcat (res, res2, NULL);
	}
	return string();
}

static void
emit_kids ()
{
	int i, j, c, n; //, *si;
	//char **sa;

	output ("int\n");
	output ("%snetvm_burg_rule (MBState *state, int goal)\n{\n", PREFIX);

	//output ("\tg_return_val_if_fail (state != NULL, 0);\n"); 
	output ("\tif (state == NULL) return 0;\n"); 
	//output ("\tg_return_val_if_fail (goal > 0, 0);\n\n");
	output ("\tif  (!(goal > 0)) return 0;\n\n");

	output ("\tswitch (goal) {\n");

	for (non_term_list_t::iterator nl = nonterm_list.begin(); nl != nonterm_list.end(); nl++) {
		NonTerm *n = *nl;
		output ("\tcase MB_NTERM_%s:\n", n->name->c_str());
		output ("\t\treturn netvm_burg_decode_%s [state->rule_%s];\n",
			n->name->c_str(), n->name->c_str());
	}

	//output ("\tdefault: g_assert_not_reached ();\n");
	output ("\tdefault: error = INSSEL_ERROR_NTERM_NOT_FOUND;\n");
	output ("\t}\n");
	output ("\treturn 0;\n");
	output ("}\n\n");


	if (dag_mode) {
		output ("MBState **\n");
		output ("%snetvm_burg_kids (MBState *state, int rulenr, MBState *kids [])\n{\n", PREFIX);
		//output ("\tg_return_val_if_fail (state != NULL, NULL);\n");
		output ("\tif (state == NULL) return NULL;\n");
		//output ("\tg_return_val_if_fail (kids != NULL, NULL);\n\n");
		output ("\tif (state == NULL) return NULL;\n");

	} else {
		output ("MBTREE_TYPE **\n");
		output ("%snetvm_burg_kids (MBTREE_TYPE *tree, int rulenr, MBTREE_TYPE *kids [])\n{\n", PREFIX);
		//output ("\tg_return_val_if_fail (tree != NULL, NULL);\n");
		output ("\tif (tree == NULL) return NULL;\n");
		//output ("\tg_return_val_if_fail (kids != NULL, NULL);\n\n");
		output ("\tif (kids == NULL) return NULL;\n\n");
	}

	output ("\tswitch (rulenr) {\n");

	n = rule_list.size(); //g_list_length (rule_list);
	vector<string> sa(n);
	vector<int> si(n);
	//sa = g_new0 (char *, n);
	//si = g_new0 (int, n);

	/* compress the case statement */
	i = c = 0;
	for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); ++l) {
		Rule *rule = *l;
		int kn = 0;
		string k;

		if (dag_mode)
			k = compute_kids ("state", rule->tree, &kn);
		else
			k = compute_kids ("tree", rule->tree, &kn);

		for (j = 0; j < c; j++)
			if ( sa[j] == k) // !strcmp (sa [j], k))
				break;

		si [i++] = j;
		if (j == c)
			sa [c++] = k;
	}

	for (i = 0; i < c; i++) {
		j  = 0;
		for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); l++)
		{
			if (i == si [j])
				output ("\tcase %d:\n", j + 1);
			j++;
		}
		output ("%s", sa[i].c_str());
		output ("\t\tbreak;\n");
	}

	//output ("\tdefault:\n\t\tg_assert_not_reached ();\n");
	output ("\tdefault:\n\t\terror = INSSEL_ERROR_RULE_NOT_FOUND;\n");
	output ("\t}\n");
	output ("\treturn kids;\n");
	output ("}\n\n");

}

static void
emit_emitter_func ()
{
	int i, rulen;
	map<string*, int> cache;

	//GHashTable *cache = g_hash_table_new (g_str_hash, g_str_equal);

	i = 0;
	for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); l++) {
		Rule *rule = *l;
		
		if (!rule->code->empty()) {
			if((rulen = cache[rule->code])){
//			if ((rulen = GPOINTER_TO_INT (g_hash_table_lookup (cache, rule->code)))) {
				emit_rule_string (rule, "");
				output ("#define netvm_burg_emit_%d netvm_burg_emit_%d\n\n", i, rulen);
				i++;
				continue;
			}
			output ("static void ");

			emit_rule_string (rule, "");

			if(cpp_class){
					output ("netvm_burg_emit_%d (jit::CFG<IR>& cfg, jit::BasicBlock<IR>& BB, MBTREE_TYPE *tree, MBCGEN_TYPE *s)\n", i);
			} else {
				if (dag_mode)
					output ("netvm_burg_emit_%d (nvmJitCFG *cfg, BasicBlock *BB, MBState *state, MBTREE_TYPE *tree, MBCGEN_TYPE *s)\n", i);
				else
					output ("netvm_burg_emit_%d (nvmJitCFG *cfg, BasicBlock *BB, MBTREE_TYPE *tree, MBCGEN_TYPE *s)\n", i);
			}
			output ("{\n");
			output ("%s\n", rule->code->c_str());
			output ("}\n\n");
			cache [rule->code] = i;
			//g_hash_table_insert (cache, rule->code, GINT_TO_POINTER (i));
		}
		i++;
	}

	//g_hash_table_destroy (cache);

	output ("%sMBEmitFunc const %snetvm_burg_func [] = {\n", PREFIX, PREFIX);
	output ("\tNULL,\n");
	i = 0;
	for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); l++){
		Rule *rule = *l;
		if (!rule->code->empty())
			output ("\tnetvm_burg_emit_%d,\n", i);
		else
			output ("\tNULL,\n");
		i++;
	}
	output ("};\n\n");
}

static void
emit_cost_func ()
{
	int i = 0;

	for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); l++){
		Rule *rule = *l;
		
		if (!rule->cfunc->empty()) {
			output ("static uint16_t\n");

			emit_rule_string (rule, "");
			
			if (dag_mode)
				output ("netvm_burg_cost_%d (MBState *state, MBCOST_DATA *data)\n", i + 1);
			else
				output ("netvm_burg_cost_%d (MBTREE_TYPE *tree, MBCOST_DATA *data)\n", i + 1);
			output ("{\n");
			output ("%s\n", rule->cfunc->c_str());
			output ("}\n\n");
		}
		i++;
	}
}

static void
emit_closure ()
{

	for (non_term_list_t::iterator l = nonterm_list.begin(); l != nonterm_list.end(); l++) {
		NonTerm *n = *l;
		
		if (!n->chain.empty())
			output ("static void closure_%s (MBState *p, int c);\n", n->name->c_str());
	}

	output ("\n");

	for (non_term_list_t::iterator l = nonterm_list.begin(); l != nonterm_list.end(); l++) {
		NonTerm *n = *l;
		
		if (!n->chain.empty()) {
			output ("static void\n");
			output ("closure_%s (MBState *p, int c)\n{\n", n->name->c_str());

			for (rule_list_t::iterator rl = n->chain.begin(); rl != n->chain.end(); rl++){
				Rule *rule = *rl;
				
				emit_rule_string (rule, "\t");
				emit_cond_assign (rule, *rule->cost, "\t");
			}
			output ("}\n\n");
		}
	}
}

static string
compute_nonterms (Tree *tree)
{

	if (!tree)
		return string();

	if (tree->nonterm) {
		return "MB_NTERM_" + *tree->nonterm->name + ", ";// g_strdup_printf ("MB_NTERM_%s, ", tree->nonterm->name);
	} else {
		return compute_nonterms(tree->left) + compute_nonterms(tree->right);
		// g_strconcat (compute_nonterms (tree->left), compute_nonterms (tree->right), NULL);
	} 
}

static void
emit_vardefs ()
{
	int i, j, c, n; //, *si;
	//char **sa;

	if (predefined_terms) {
		output ("static uint8_t netvm_burg_arity [MBMAX_OPCODES];\n"); 

		output ("void\n%snetvm_burg_init (void)\n{\n", PREFIX);

		i = 0;
		for (term_list_t::iterator l = term_list.begin(); l != term_list.end(); l++) {
			Term *t = *l;

			output ("\tnetvm_burg_arity [%s] = %d; /* %s */\n", t->name->c_str(), t->arity, t->name->c_str());

		}

		output ("}\n\n");

	} else {
		output ("static const uint8_t netvm_burg_arity [] = {\n"); 
		i = 0;
		for (term_list_t::iterator l = term_list.begin(); l != term_list.end(); l++) {
			Term *t = *l;

			while (i < t->number) {
				output ("\t0,\n");
				i++;
			}
		
			output ("\t%d, /* %s */\n", t->arity, t->name->c_str());

			i++;
		}
		output ("};\n\n");

		output ("static const char *const netvm_burg_term_string [] = {\n");
		output ("\tNULL,\n");
		i = 0;
		for (term_list_t::iterator l = term_list.begin(); l != term_list.end(); l++) {
			Term *t = *l;
			output ("\t\"%s\",\n", t->name->c_str());
		}
		output ("};\n\n");
	}

	output ("const char * const %snetvm_burg_rule_string [] = {\n", PREFIX);
	output ("\tNULL,\n");
	i = 0;
	for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); l++){
		Rule *rule = *l;
		output ("\t\"%s: ", rule->lhs->name->c_str());
		emit_tree_string (rule->tree);
		output ("\",\n");
	}
	output ("};\n\n");

	n = rule_list.size();
	vector<string> sa(n);
	vector<int> si(n);
//sa = g_new0 (char *, n);
//si = g_new0 (int, n);

	/* compress the _nts array */
	i = 0;
	c = 0;
	for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); l++){
		Rule *rule =  *l;
		string s = compute_nonterms (rule->tree);

		for (j = 0; j < c; j++)
			if (sa[j] == s) //!strcmp (sa [j], s))
				break;

		si [i++] = j;
		if (j == c) {
			output ("static const uint16_t netvm_burg_nts_%d [] = { %s0 };\n", c, s.c_str());
			sa [c++] = s;
		}
	}	
	output ("\n");

	output ("const uint16_t *const %snetvm_burg_nts [] = {\n", PREFIX);
	output ("\t0,\n");
	i = 0;
	for (rule_list_t::iterator l = rule_list.begin(); l != rule_list.end(); l++){
	//for (l = rule_list, i = 0; l; l = l->next) {
		Rule *rule = *l;
		output ("\tnetvm_burg_nts_%d, ", si[i++]);
		emit_rule_string (rule, "");
	}
	output ("};\n\n");
}

static void
emit_prototypes ()
{
	if (cpp_class)
	{

		output ("typedef void (*MBEmitFunc) (jit::CFG<IR>& cfg, jit::BasicBlock<IR>& BB, MBTREE_TYPE *tree, MBCGEN_TYPE *s);\n\n");

		output ("static const uint16_t *const netvm_burg_nts [];\n");
		output ("static MBEmitFunc const netvm_burg_func [];\n");
		output ("static const char* const netvm_burg_rule_string [];\n");

		output ("MBState *netvm_burg_label (MBTREE_TYPE *tree, MBCOST_DATA *data);\n");
		output ("int netvm_burg_rule (MBState *state, int goal);\n");

		if (dag_mode)
			output ("MBState **netvm_burg_kids (MBState *state, int rulenr, MBState *kids []);\n");
		else
			output ("MBTREE_TYPE **netvm_burg_kids (MBTREE_TYPE *tree, int rulenr, MBTREE_TYPE *kids []);\n");

		output ("void netvm_burg_init (void);\n");
		output ("uint32_t error;\n");
		output ("private:\n");
		output ("MBState *netvm_burg_label_priv (MBTREE_TYPE *tree, MBCOST_DATA *data);\n");

	} else {

		if (dag_mode)
			output ("typedef void (*MBEmitFunc) (nvmJitCFG *cfg, BasicBlock *BB, MBState *state, MBTREE_TYPE *tree, MBCGEN_TYPE *s);\n\n");
		else
			output ("typedef void (*MBEmitFunc) (nvmJitCFG *cfg, BasicBlock *BB, MBTREE_TYPE *tree, MBCGEN_TYPE *s);\n\n");

		output ("extern const char * const netvm_burg_term_string [];\n");
		output ("extern const char * const netvm_burg_rule_string [];\n");
		//output ("extern const guint16 *const netvm_burg_nts [];\n");
		output ("extern const uint16_t *const netvm_burg_nts [];\n");
		output ("extern MBEmitFunc const netvm_burg_func [];\n");

		output ("MBState *netvm_burg_label (MBTREE_TYPE *tree, MBCOST_DATA *data);\n");
		output ("int netvm_burg_rule (MBState *state, int goal);\n");

		if (dag_mode)
			output ("MBState **netvm_burg_kids (MBState *state, int rulenr, MBState *kids []);\n");
		else
			output ("MBTREE_TYPE **netvm_burg_kids (MBTREE_TYPE *tree, int rulenr, MBTREE_TYPE *kids []);\n");

		output ("extern void netvm_burg_init (void);\n");

		output ("int32_t nvmJitRetargetable_Instruction_Selection(nvmJitCFG *cfg);\n");
		output ("int32_t error;\n");
		output ("nvmStructsOffsets *structsOffs;\n");
		output ("\n");
	}
}

static void check_reach (NonTerm *n);

static void
mark_reached (Tree *tree)
{
	if (tree->nonterm && !tree->nonterm->reached)
		check_reach (tree->nonterm);
	if (tree->left)
		mark_reached (tree->left);
	if (tree->right)
		mark_reached (tree->right);
}

static void
check_reach (NonTerm *n)
{
	//GList *l;

	n->reached = true;

	for (rule_list_t::iterator l = n->rules.begin(); l != n->rules.end(); l++){
		Rule *rule = *l;
		mark_reached (rule->tree);
	}
}

static void
check_result ()
{
	for (term_list_t::iterator l = term_list.begin(); l != term_list.end(); l++) {
		Term *term = *l;
		if (term->arity == -1)
			cerr << "unused terminal \"" << *term->name << "\"\n";
	} 

	check_reach (nonterm_list.front());

	for (non_term_list_t::iterator l = nonterm_list.begin(); l != nonterm_list.end(); l++) {
		NonTerm *n = *l;
		if (!n->reached)
			cerr << "unreachable nonterm \"" << *n->name << "\"\n";
	}
}

static void
usage ()
{
	fprintf (stderr, "Usage is: netvmburg [-C] -d file.h -s file.c [inputfile] \n");
	fprintf(stderr, "-C\temit c++ class deriving from Instruction Selection\n");
	exit (1);
}

//static void
//warning_handler (const gchar *log_domain,
//		 GLogLevelFlags log_level,
//		 const gchar *message,
//		 gpointer user_data)
//{
//	(void) fprintf ((FILE *) user_data, "** WARNING **: %s\n", message);
//}

static char*
add_class_suffix(const char* class_name)
{
	char *res;
	int len = strlen(class_name) + 3;
	res = (char *) malloc(len * sizeof(char));
	strncpy(res, class_name, len);

	strncat(res, "::", 2);
	return res;
}

int
main (int argc, char *argv [])
{
	char *cfile = NULL;
	char *deffile = NULL;
	list<string> infiles;
	list<FILE *> infiles_fd;
	int i;

    //definedvars = g_hash_table_new (g_str_hash, g_str_equal);
	//g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, warning_handler, stderr);

	for (i = 1; i < argc; i++){
		if (argv [i][0] == '-'){
			if (argv [i][1] == 'h') {
				usage ();
			} else if (argv[i][1] == 'C'){
				cpp_class = argv[++i];
			} else if (argv [i][1] == 'e') {
				dag_mode = 1;
			} else if (argv [i][1] == 'p') {
				predefined_terms = 1;
			} else if (argv [i][1] == 'd') {
				deffile = argv [++i];
			} else if (argv [i][1] == 's') {
				cfile = argv [++i];
			} else if (argv [i][1] == 'c') {
				default_cost = atoi (argv [++i]);
			} else if (argv [i][1] == 'D') {
								definedvars[string(&argv[i][2])] = true;
                                //g_hash_table_insert (definedvars, &argv [i][2], GUINT_TO_POINTER (1));
			} else {
				usage ();
			}
		} else {
			infiles.push_back(string(argv[i]));
			//infiles = g_list_append (infiles, argv [i]);
		}
	}

#ifdef WIN32
#define SEPARATOR '\\'
#else 
#define SEPARATOR '/'
#endif

	if (deffile) {

		if (!(deffd = fopen (deffile, "w"))) {
			perror ("cant open header output file");
			exit (-1);
		}
		outputfd = deffd;
		
		output ("#ifndef _%s\n", cpp_class ? cpp_class : "INSTRUCTION_SELECTION_H");
		output ("#define _%s\n\n", cpp_class ? cpp_class : "INSTRUCTION_SELECTION_H");
	} else 
		outputfd = stdout;


	if (!infiles.empty()) {

		for (list<string>::iterator l = infiles.begin(); l != infiles.end(); l++){
			infile = *l;
			if (!(inputfd = fopen (infile.c_str(), "r"))) {
				perror ("cant open input file");
				exit (-1);
			}

			yyparse ();

			reset_parser ();

			infiles_fd.push_back(inputfd);
		}
	} else {
		inputfd = stdin;
		yyparse ();
	}

	check_result ();

	if (nonterm_list.empty()){
		cerr << "no start symbol found\n";
		exit(1);
	}

	emit_header ();
	//emit_struct_offset();
	emit_nonterm ();
	emit_errors();
	emit_state ();
	emit_class_declaration();
	emit_prototypes ();
	emit_class_end();

	cpp_class = add_class_suffix(cpp_class);

	if (deffd) {
		output ("#endif /* %s */\n", cpp_class ? cpp_class : "INSTRUCTION_SELECTION_H");
		fclose (deffd);

		if (cfile == NULL)
			outputfd = stdout;
		else {
			if (!(cfd = fopen (cfile, "w"))) {
				perror ("cant open c output file");
				(void) remove (deffile);
				exit (-1);
			}

			outputfd = cfd;
		}

		//output ("#include \"%s\"\n\n", deffile);
		output ("#include \"%s\"\n\n", deffile);
	}
	
	emit_vardefs ();
	emit_cost_func ();
	emit_emitter_func ();
	emit_decoders ();

	emit_closure ();
	emit_label_func ();

	emit_kids ();

	if(!infiles_fd.empty()){
		for(list<FILE*>::iterator l = infiles_fd.begin(); l != infiles_fd.end(); l++)
		{
			inputfd = *l;
			yyparsetail ();
			fclose (inputfd);
		}
	} else {
		yyparsetail ();
	}

	if (cfile)
		fclose (cfd);

	return 0;
}
