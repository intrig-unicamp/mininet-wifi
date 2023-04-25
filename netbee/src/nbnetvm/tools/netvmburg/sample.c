/* 
 * This header (everything before the first "%%") is copied 
 * directly to the output.
 */

#include <glib.h>
#include <stdio.h>
#include <string.h>

#define MBTREE_TYPE  MBTree

typedef struct _MBTree MBTree;
struct _MBTree {
	guint16 op;
	MBTree *left, *right;
	gpointer state;
};

#include <glib.h>

#ifndef MBTREE_TYPE
#error MBTREE_TYPE undefined
#endif
#ifndef MBTREE_OP
#define MBTREE_OP(t) ((t)->op)
#endif
#ifndef MBTREE_LEFT
#define MBTREE_LEFT(t) ((t)->left)
#endif
#ifndef MBTREE_RIGHT
#define MBTREE_RIGHT(t) ((t)->right)
#endif
#ifndef MBTREE_STATE
#define MBTREE_STATE(t) ((t)->state)
#endif
#ifndef MBCGEN_TYPE
#define MBCGEN_TYPE int
#endif
#ifndef MBALLOC_STATE
#define MBALLOC_STATE g_new (MBState, 1)
#endif
#ifndef MBCOST_DATA
#define MBCOST_DATA gpointer
#endif

#define MBMAXCOST 32768

#define MBCOND(x) if (!(x)) return MBMAXCOST;

#define MB_TERM_Assign		1
#define MB_TERM_Constant	2
#define MB_TERM_Fetch		3
#define MB_TERM_Mul			5
#define MB_TERM_Plus		6
#define MB_TERM_AltFetch	7
#define MB_TERM_Four		8

#define MB_NTERM_reg	1
#define MB_NTERM_con	2
#define MB_NTERM_addr	3
#define MB_MAX_NTERMS	3

typedef struct _MBState MBState;
struct _MBState {
	int		 op;
	MBState			*left, *right;
	guint16			cost[4];
	unsigned int	rule_reg:2;
	unsigned int	rule_con:2;
	unsigned int	rule_addr:2;
};

typedef void (*MBEmitFunc) (MBTREE_TYPE *tree, MBCGEN_TYPE *s);

extern const char * const mono_burg_term_string [];
extern const char * const mono_burg_rule_string [];
extern const guint16 *const mono_burg_nts [];
extern MBEmitFunc const mono_burg_func [];
MBState *mono_burg_label (MBTREE_TYPE *tree, MBCOST_DATA *data);
int mono_burg_rule (MBState *state, int goal);
MBTREE_TYPE **mono_burg_kids (MBTREE_TYPE *tree, int rulenr, MBTREE_TYPE *kids []);
extern void mono_burg_init (void);

const guint8 mono_burg_arity [] = {
	0,
	2, /* Assign */
	0, /* Constant */
	1, /* Fetch */
	0,
	2, /* Mul */
	2, /* Plus */
	1, /* AltFetch */
	0, /* Four */
};

const char *const mono_burg_term_string [] = {
	NULL,
	"Assign",
	"Constant",
	"Fetch",
	"Mul",
	"Plus",
	"AltFetch",
	"Four",
};

const char * const mono_burg_rule_string [] = {
	NULL,
	"con: Constant",
	"con: Four",
	"addr: con",
	"addr: Plus(con, reg)",
	"addr: Plus(con, Mul(Four, reg))",
	"reg: AltFetch(addr)",
	"reg: Fetch(addr)",
	"reg: Assign(addr, reg)",
};

static const guint16 mono_burg_nts_0 [] = { 0 };
static const guint16 mono_burg_nts_1 [] = { MB_NTERM_con, 0 };
static const guint16 mono_burg_nts_2 [] = { MB_NTERM_con, MB_NTERM_reg, 0 };
static const guint16 mono_burg_nts_3 [] = { MB_NTERM_addr, 0 };
static const guint16 mono_burg_nts_4 [] = { MB_NTERM_addr, MB_NTERM_reg, 0 };

const guint16 *const mono_burg_nts [] = {
	0,
	mono_burg_nts_0, /* con: Constant */
	mono_burg_nts_0, /* con: Four */
	mono_burg_nts_1, /* addr: con */
	mono_burg_nts_2, /* addr: Plus(con, reg) */
	mono_burg_nts_2, /* addr: Plus(con, Mul(Four, reg)) */
	mono_burg_nts_3, /* reg: AltFetch(addr) */
	mono_burg_nts_3, /* reg: Fetch(addr) */
	mono_burg_nts_4, /* reg: Assign(addr, reg) */
};

inline static guint16
/* addr: Plus(con, reg) */
mono_burg_cost_4 (MBTREE_TYPE *tree, MBCOST_DATA *data)
{
  
  return 1;

}

static void /* addr: Plus(con, reg) */
mono_burg_emit_3 (MBTREE_TYPE *tree, MBCGEN_TYPE *s)
{

	int ern = mono_burg_rule (tree->state, MB_NTERM_addr);
	printf ("%s\n", mono_burg_rule_string [ern]);

}

/* addr: Plus(con, Mul(Four, reg)) */
#define mono_burg_emit_4 mono_burg_emit_3

static void /* reg: AltFetch(addr) */
mono_burg_emit_5 (MBTREE_TYPE *tree, MBCGEN_TYPE *s)
{

	int ern = mono_burg_rule (tree->state, MB_NTERM_reg);
	printf ("%s\n", mono_burg_rule_string [ern]);

}

/* reg: Fetch(addr) */
#define mono_burg_emit_6 mono_burg_emit_5

/* reg: Assign(addr, reg) */
#define mono_burg_emit_7 mono_burg_emit_5

MBEmitFunc const mono_burg_func [] = {
	NULL,
	NULL,
	NULL,
	NULL,
	mono_burg_emit_3,
	mono_burg_emit_4,
	mono_burg_emit_5,
	mono_burg_emit_6,
	mono_burg_emit_7,
};

const int mono_burg_decode_reg[] = {
	0,
	6,
	7,
	8,
};

const int mono_burg_decode_con[] = {
	0,
	1,
	2,
};

const int mono_burg_decode_addr[] = {
	0,
	3,
	4,
	5,
};

static void closure_con (MBState *p, int c);

static void
closure_con (MBState *p, int c)
{
	/* addr: con */
	if (c + 0 < p->cost[MB_NTERM_addr]) {
		p->cost[MB_NTERM_addr] = c + 0;
		p->rule_addr = 1;
	}
}

static MBState *
mono_burg_label_priv (MBTREE_TYPE *tree, MBCOST_DATA *data) {
	int arity;
	int c;
	MBState *p;
	MBState *left = NULL, *right = NULL;

	switch (mono_burg_arity [MBTREE_OP(tree)]) {
	case 0:
		break;
	case 1:
		left = mono_burg_label_priv (MBTREE_LEFT(tree), data);
		right = NULL;
		break;
	case 2:
		left = mono_burg_label_priv (MBTREE_LEFT(tree), data);
		right = mono_burg_label_priv (MBTREE_RIGHT(tree), data);
	}

	arity = (left != NULL) + (right != NULL);
	g_assert (arity == mono_burg_arity [MBTREE_OP(tree)]);

	p = MBALLOC_STATE;
	memset (p, 0, sizeof (MBState));
	p->op = MBTREE_OP(tree);
	p->left = left;
	p->right = right;
	p->cost [1] = 32767;
	p->cost [2] = 32767;
	p->cost [3] = 32767;

	switch (MBTREE_OP(tree)) {
	case 1: /* Assign */
		/* reg: Assign(addr, reg) */
		{
			c = left->cost[MB_NTERM_addr] + right->cost[MB_NTERM_reg] + 1;
			if (c < p->cost[MB_NTERM_reg]) {
				p->cost[MB_NTERM_reg] = c;
				p->rule_reg = 3;
			}
		}
		break;
	case 2: /* Constant */
		/* con: Constant */
		{
			c = 0;
			if (c < p->cost[MB_NTERM_con]) {
				p->cost[MB_NTERM_con] = c;
				p->rule_con = 1;
				closure_con (p, c);
			}
		}
		break;
	case 3: /* Fetch */
		/* reg: Fetch(addr) */
		{
			c = left->cost[MB_NTERM_addr] + 1;
			if (c < p->cost[MB_NTERM_reg]) {
				p->cost[MB_NTERM_reg] = c;
				p->rule_reg = 2;
			}
		}
		break;
	case 5: /* Mul */
		break;
	case 6: /* Plus */
		/* addr: Plus(con, reg) */
		{
			c = left->cost[MB_NTERM_con] + right->cost[MB_NTERM_reg] + mono_burg_cost_4 (tree, data);
			if (c < p->cost[MB_NTERM_addr]) {
				p->cost[MB_NTERM_addr] = c;
				p->rule_addr = 2;
			}
		}
		/* addr: Plus(con, Mul(Four, reg)) */
		if (
			p->right->op == 5 /* Mul */ &&
			p->right->left->op == 8 /* Four */
		)
		{
			c = left->cost[MB_NTERM_con] + right->right->cost[MB_NTERM_reg] + 2;
			if (c < p->cost[MB_NTERM_addr]) {
				p->cost[MB_NTERM_addr] = c;
				p->rule_addr = 3;
			}
		}
		break;
	case 7: /* AltFetch */
		/* reg: AltFetch(addr) */
		{
			c = left->cost[MB_NTERM_addr] + 1;
			if (c < p->cost[MB_NTERM_reg]) {
				p->cost[MB_NTERM_reg] = c;
				p->rule_reg = 1;
			}
		}
		break;
	case 8: /* Four */
		/* con: Four */
		{
			c = 0;
			if (c < p->cost[MB_NTERM_con]) {
				p->cost[MB_NTERM_con] = c;
				p->rule_con = 2;
				closure_con (p, c);
			}
		}
		break;
	default:
#ifdef MBGET_OP_NAME
		g_error ("unknown operator: %s", MBGET_OP_NAME(MBTREE_OP(tree)));
#else
		g_error ("unknown operator: 0x%04x", MBTREE_OP(tree));
#endif
	}

	MBTREE_STATE(tree) = p;
	return p;
}

MBState *
mono_burg_label (MBTREE_TYPE *tree, MBCOST_DATA *data)
{
	MBState *p = mono_burg_label_priv (tree, data);
	return p->rule_reg ? p : NULL;
}

int
mono_burg_rule (MBState *state, int goal)
{
	g_return_val_if_fail (state != NULL, 0);
	g_return_val_if_fail (goal > 0, 0);

	switch (goal) {
	case MB_NTERM_reg:
		return mono_burg_decode_reg [state->rule_reg];
	case MB_NTERM_con:
		return mono_burg_decode_con [state->rule_con];
	case MB_NTERM_addr:
		return mono_burg_decode_addr [state->rule_addr];
	default: g_assert_not_reached ();
	}
	return 0;
}

MBTREE_TYPE **
mono_burg_kids (MBTREE_TYPE *tree, int rulenr, MBTREE_TYPE *kids [])
{
	g_return_val_if_fail (tree != NULL, NULL);
	g_return_val_if_fail (kids != NULL, NULL);

	switch (rulenr) {
	case 1:
	case 2:
		break;
	case 3:
		kids[0] = tree;
		break;
	case 4:
	case 8:
		kids[0] = MBTREE_LEFT(tree);
		kids[1] = MBTREE_RIGHT(tree);
		break;
	case 5:
		kids[0] = MBTREE_LEFT(tree);
		kids[1] = MBTREE_RIGHT(MBTREE_RIGHT(tree));
		break;
	case 6:
	case 7:
		kids[0] = MBTREE_LEFT(tree);
		break;
	default:
		g_assert_not_reached ();
	}
	return kids;
}

/* everything below the second "%%" is also copied directly
 * to the output file.
 */

static MBTree *
create_tree (int op, MBTree *left, MBTree *right)
{
	MBTree *t = g_new0 (MBTree, 1);
	
	t->op = op;
	t->left = left;
	t->right = right;

	return t;
}

static void
reduce (MBTree *tree, int goal) 
{
	MBTree *kids[10];
	int ern = mono_burg_rule (tree->state, goal);
	guint16 *nts = mono_burg_nts [ern];
	int i, n;

	mono_burg_kids (tree, ern, kids);

	printf ("TEST\t%d\t%d\t%d\t%s\t%d\n", goal, tree->op, ern, mono_burg_rule_string [ern], nts [0]);
	
	for (i = 0; nts [i]; i++) 
		reduce (kids [i], nts [i]);

	n = (tree->left != NULL) + (tree->right != NULL);

	if (n) { /* not a terminal */
		printf ("XXTE %s %d\n", mono_burg_rule_string [ern], n);
		if (mono_burg_func [ern])
			mono_burg_func [ern] (tree, NULL);
		else
			g_warning ("no code for rule %s\n", 
				   mono_burg_rule_string [ern]);
	} else {
		if (mono_burg_func [ern])
			g_warning ("unused code in rule %s\n", 
				   mono_burg_rule_string [ern]);
	}
}

int
main ()
{
	MBTree *t, *l, *r;
	MBState *s;

	l = create_tree (MB_TERM_Constant, NULL, NULL);

	r = create_tree (MB_TERM_Fetch, l, NULL);
	l = create_tree (MB_TERM_Constant, NULL, NULL);

	r = create_tree (MB_TERM_Assign, l, r);
	l = create_tree (MB_TERM_Four, NULL, NULL);

	r = create_tree (MB_TERM_Mul, l, r);
	l = create_tree (MB_TERM_Constant, NULL, NULL);

	l = create_tree (MB_TERM_Plus, l, r);

	t = create_tree (MB_TERM_Fetch, l, NULL);

	s = mono_burg_label (t, NULL);

	g_assert (s);

	printf("\tgoal\topcode\tern\tstring\tnts\n");
	reduce (t, MB_NTERM_reg);

	getchar();

	return 0;
}
