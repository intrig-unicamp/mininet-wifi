#ifndef _OcteoncInsSelector
#define _OcteoncInsSelector

/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include <sstream>
#include <string>

#include "application.h"
#include "cfg.h"
#include "int_structs.h" 
#include "mirnode.h"
#include "octeonc-asm.h"
// include the octeon coprocessor file, not to repeat the same code twice
#include "octeon-coprocessor.h"

#define MBMAX_OPCODES 256
#define MBTREE_TYPE jit::MIRNode
#define MBREG_TYPE  jit::octeonc::octeoncRegType

#define MBTREE_LEFT(t) ((t)->getKid(0))
#define MBTREE_RIGHT(t) ((t)->getKid(1))
#define MBTREE_OP(t) ((t)->getOpcode())
#define MBTREE_STATE(t) ((t)->state)
#define MBTREE_VALUE(t) ((t)->getDefReg())
#define MBALLOC_STATE   new MBState()
#define MBGET_OP_NAME(opcode) nvmOpCodeTable[opcode].CodeName

#define MBTREE_GET_CONST_VALUE(t) (((ConstNode *)t)->getValue())
#define APPLICATION Application::getApp(BB) 
#define ADDINS(i) BB.getCode().push_back((i))

typedef jit::octeonc::octeonc_Insn IR;

using namespace jit;
using namespace octeonc;

#include <stdlib.h>

#ifndef MBTREE_TYPE
#error MBTREE_TYPE undefined
#endif
#ifndef MBREG_TYPE
#error MBREG_TYPE undefined
#endif
#ifndef MBTREE_OP
#define MBTREE_OP(t) ((t)->Code)
#endif
#ifndef MBTREE_LEFT
#define MBTREE_LEFT(t) ((t)->kids[0])
#endif
#ifndef MBTREE_RIGHT
#define MBTREE_RIGHT(t) ((t)->kids[1])
#endif
#ifndef MBTREE_STATE
#define MBTREE_STATE(t) ((t)->State)
#endif
#ifndef MBTREE_VALUE
#define MBTREE_VALUE(t) ((t)->Value)
#endif
#ifndef MBCGEN_TYPE
#define MBCGEN_TYPE int
#endif
#ifndef MBALLOC_STATE
#define MBALLOC_STATE CALLOC (1, sizeof(MBState))
#endif
#ifndef MBCOST_DATA
#define MBCOST_DATA void*
#endif

#define MBMAXCOST 32768

#define MBCOND(x) if (!(x)) return MBMAXCOST;

#define MB_TERM_CNST	 CNST
#define MB_TERM_RET	 RET
#define MB_TERM_SNDPKT	 SNDPKT
#define MB_TERM_PBL	 PBL
#define MB_TERM_LDPORT	 LDPORT
#define MB_TERM_PBLDS	 PBLDS
#define MB_TERM_PBLDU	 PBLDU
#define MB_TERM_PSLDS	 PSLDS
#define MB_TERM_PSLDU	 PSLDU
#define MB_TERM_PILD	 PILD
#define MB_TERM_PBSTR	 PBSTR
#define MB_TERM_PSSTR	 PSSTR
#define MB_TERM_PISTR	 PISTR
#define MB_TERM_ISSBLD	 ISSBLD
#define MB_TERM_ISBLD	 ISBLD
#define MB_TERM_ISSSLD	 ISSSLD
#define MB_TERM_ISSLD	 ISSLD
#define MB_TERM_ISSILD	 ISSILD
#define MB_TERM_IBSTR	 IBSTR
#define MB_TERM_ISSTR	 ISSTR
#define MB_TERM_IISTR	 IISTR
#define MB_TERM_DBLDS	 DBLDS
#define MB_TERM_DBLDU	 DBLDU
#define MB_TERM_DSLDS	 DSLDS
#define MB_TERM_DSLDU	 DSLDU
#define MB_TERM_DILD	 DILD
#define MB_TERM_DBSTR	 DBSTR
#define MB_TERM_DSSTR	 DSSTR
#define MB_TERM_DISTR	 DISTR
#define MB_TERM_IINC_1	 IINC_1
#define MB_TERM_IDEC_1	 IDEC_1
#define MB_TERM_SUBUOV	 SUBUOV
#define MB_TERM_ADDUOV	 ADDUOV
#define MB_TERM_SUB	 SUB
#define MB_TERM_ADD	 ADD
#define MB_TERM_NEG	 NEG
#define MB_TERM_AND	 AND
#define MB_TERM_OR	 OR
#define MB_TERM_NOT	 NOT
#define MB_TERM_IMUL	 IMUL
#define MB_TERM_MOD	 MOD
#define MB_TERM_USHR	 USHR
#define MB_TERM_SHR	 SHR
#define MB_TERM_SHL	 SHL
#define MB_TERM_JCMPEQ	 JCMPEQ
#define MB_TERM_JCMPNEQ	 JCMPNEQ
#define MB_TERM_JCMPLE	 JCMPLE
#define MB_TERM_JCMPL	 JCMPL
#define MB_TERM_JCMPG	 JCMPG
#define MB_TERM_JCMPGE	 JCMPGE
#define MB_TERM_JUMPW	 JUMPW
#define MB_TERM_JNE	 JNE
#define MB_TERM_JEQ	 JEQ
#define MB_TERM_JUMP	 JUMP
#define MB_TERM_SWITCH	 SWITCH
#define MB_TERM_LDREG	 LDREG
#define MB_TERM_STREG	 STREG
#define MB_TERM_COPRUN	 COPRUN
#define MB_TERM_COPINIT	 COPINIT
#define MB_TERM_COPPKTOUT	 COPPKTOUT
#define MB_TERM_CMP	 CMP
#define MB_TERM_INFOCLR	 INFOCLR

#define MB_NTERM_stmt	1
#define MB_NTERM_con	2
#define MB_NTERM_reg	3
#define MB_MAX_NTERMS	3

#define INSSEL_SUCCESS					0
#define INSSEL_ERROR_ARITY				1
#define INSSEL_ERROR_NTERM_NOT_FOUND	2
#define INSSEL_ERROR_RULE_NOT_FOUND		3

typedef struct OcteoncInsSelector_MBState MBState;
struct OcteoncInsSelector_MBState {
	int		 op;
	MBState		*left, *right;
	uint16_t		cost[4];
	unsigned int	 rule_stmt:5;
	unsigned int	 rule_con:1;
	unsigned int	 rule_reg:6;
};

class OcteoncInsSelector
{
public:
typedef MBTREE_TYPE MBTree; 
typedef MBCOST_DATA MBCostData; 
typedef struct OcteoncInsSelector_MBState MBState; 
typedef void (*MBEmitFunc) (jit::CFG<IR>& cfg, jit::BasicBlock<IR>& BB, MBTREE_TYPE *tree, MBCGEN_TYPE *s);

static const uint16_t *const netvm_burg_nts [];
static MBEmitFunc const netvm_burg_func [];
static const char* const netvm_burg_rule_string [];
MBState *netvm_burg_label (MBTREE_TYPE *tree, MBCOST_DATA *data);
int netvm_burg_rule (MBState *state, int goal);
MBTREE_TYPE **netvm_burg_kids (MBTREE_TYPE *tree, int rulenr, MBTREE_TYPE *kids []);
void netvm_burg_init (void);
uint32_t error;
private:
MBState *netvm_burg_label_priv (MBTREE_TYPE *tree, MBCOST_DATA *data);
};
#endif /* OcteoncInsSelector:: */
