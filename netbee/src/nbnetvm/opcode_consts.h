/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef _OPCODE_CONSTS_H_
#define _OPCODE_CONSTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file opcode_consts.h
* \brief This file contains the constant declarations that are used in the opcode definition files
*  to specify more information about an operation. For each group of consts is given a mask that has
*  to be used to extract the information. All the operation has to be done using arithmetic and and or.
*/

//! type of the Opcode
#define	OPTYPE_MASK		3<<30
#define OP_TYPE_STMT			0<<30
#define OP_TYPE_TERM			1<<30
#define OP_TYPE_NTERM			2<<30
#define OP_TYPE_EXPR			OP_TYPE_TERM|OP_TYPE_NTERM

//! Operation type
#define OPCODE_MASK		31<<25
#define OP_LOAD			0<<25
#define OP_STORE		1<<25
#define OP_ARITH		2<<25
#define OP_JUMP			3<<25
#define OP_STACK		4<<25
#define OP_PTNMTC		5<<25
#define OP_PKT			6<<25
#define OP_INIT			7<<25
#define OP_MISC			8<<25
#define OP_MACRO		9<<25
#define OP_HASH			10<<25
#define OP_CHKSM		11<<25
#define OP_LOC			12<<25
#define OP_TERM			13<<25
#define	OP_IPLEN		14<<25
#define OP_STREG		15<<25
#define OP_CNST			16<<25
#define OP_CNSTREG		17<<25
#define OP_LDREG		18<<25
#define OP_INDIR		19<<25
#define OP_LLOAD		20<<25
#define OP_LSTORE		21<<25
#define OP_LDPKTB		22<<25
#define OP_LDINFOB		23<<25
#define OP_LDSHAREDB	24<<25
#define OP_LDDATAB		25<<25
#define OP_LDPORT		26<<25
#define OP_PHI			27<<25
#define OP_COP			28<<25
#define OP_CHECK		29<<25

//! type of memory for load/store
#define	MEM_MASK		3<<15
#define MEM_DATA		0<<15
#define MEM_SHARED		1<<15
#define MEM_INFO		2<<15
#define MEM_PACKET		3<<15

//! dimension of loaded/stored data
#define DIM_MASK		3<<13
#define	DIM_8			0<<13
#define	DIM_16			1<<13
#define	DIM_32			2<<13

//! signed and unsigned operations (both for load/store and arithmetic)
#define SIGNED			0<<12
#define UNSIGNED		1<<12

//! arithmetic operations
#define	ARITH_MASK		31<<17
#define ARITH_ADD		0<<17
#define ARITH_SUB		1<<17
#define ARITH_MUL		2<<17
#define ARITH_NEG		3<<17
#define ARITH_OR		4<<17
#define ARITH_AND		5<<17
#define ARITH_XOR		6<<17
#define ARITH_NOT		7<<17
#define ARITH_SHL		8<<17
#define ARITH_SHR		9<<17
#define ARITH_IRND		10<<17
#define ARITH_CMP		11<<17
#define ARITH_ROTL		12<<17
#define ARITH_ROTR		13<<17
#define ARITH_IINC_1	14<<17
#define ARITH_IINC_2	15<<17
#define ARITH_IINC_3	16<<17
#define ARITH_IINC_4	17<<17
#define ARITH_IDEC_1	18<<17
#define ARITH_IDEC_2	19<<17
#define ARITH_IDEC_3	20<<17
#define ARITH_IDEC_4	21<<17
#define ARITH_USHR		22<<17
#define ARITH_MCMP		23<<17
#define ARITH_MOD		23<<17

//! Jump type
#define	JMP_MASK		7<<17
#define JMP_JUMP		0<<17
#define JMP_SWITCH		1<<17
#define JMP_CALL		2<<17
#define JMP_CALLW		3<<17
#define JMP_RET			4<<17

// Jump or cmp condition
#define	CND_MASK		7<<22
#define CND_GE			0<<22
#define CND_G			1<<22
#define CND_LE			2<<22
#define CND_L			3<<22
#define CND_NEQ			4<<22
#define CND_NE			5<<22
#define CND_EQ			6<<22
#define WIDE			7<<22

//! Source of jump (of cmp) condition operands (nothing if the comparison is made with 0)
#define	JMPOPS_MASK		1<<8
#define JMPOPS_STACK	0<<8
#define JMPOPS_0		1<<8

//! Numer of targets for jump operations (JMPTGT_NO_TARGETS for impicit target like returns, JMPTGT_SWITCH for switch operations)
#define JMPTGT_MASK			3<<15
#define JMPTGT_ONE_TARGET	0<<15
#define JMPTGT_TWO_TARGETS	1<<15
#define JMPTGT_NO_TARGETS	2<<15
#define JMPTGT_SWITCH		3<<15

//! Stack management instructions
#define	STK_MASK		15<<17
#define STK_PUSH		0<<17
#define STK_PUSH2		1<<17
#define STK_POP			2<<17
#define STK_POP_I		3<<17
#define STK_CONST_1		4<<17
#define STK_CONST_2		5<<17
#define STK_CONST_0		6<<17
#define STK_CONST__1	7<<17
#define STK_TSTAMP		8<<17
#define STK_DUP			9<<17
#define STK_SWAP		10<<17
#define STK_IESWAP		11<<17
#define STK_SLEEP		12<<17
#define STK_PBL			13<<17

//! Pattern matching instructions
#define	PTNMTC_MASK		1<<17
#define PTNMTC_JFLD		0<<17
#define PTNMTC_MJFLD	1<<17
#define PTNMTC_PSCAN	1<<17

//! Packet and data transfer instructions
#define	PTK_MASK		15<<17
#define PKT_SNDPKT		0<<17
#define PKT_DSNDPKT		1<<17
#define PKT_RCVPKT		2<<17
#define PKT_DPMCPY		3<<17
#define PKT_PDMCPY		4<<17
#define PKT_DSMCPY		5<<17
#define PKT_SDMCPY		6<<17
#define PKT_SPMCPY		7<<17
#define PKT_PSMCPY		8<<17
#define PKT_PIMCPY		9<<17
#define PKT_CRTEXBUF	10<<17
#define PKT_DELEXBUF	11<<17


//! Initialization instructions
#define	INIT_MASK		7<<17
#define INIT_SDMSIZE	0<<17
#define INIT_SPBSIZE	1<<17
#define INIT_SSMSIZE	2<<17
#define INIT_LOADE		3<<17
#define INIT_STOREE		4<<17

//! Miscellaneous nvmOPCODEs
#define	MISC_MASK		3<<17
#define MISC_NOP		0<<17
#define MISC_BRKPOINT	1<<17
#define MISC_EXIT		2<<17

// MACROINSTRUCTIONS

//Bit manipulation macroinstruction
#define	MASCO_MASK			15<<17
#define MACRO_FINDBITSET	0<<17
#define MACRO_MFINDBITSET	1<<17
#define MACRO_XFINDBITSET	2<<17
#define MACRO_XMFINDBITSET	3<<17
#define MACRO_SETBIT		4<<17
#define MACRO_CLEARBIT		5<<17
#define MACRO_FLIPBIT		6<<17
#define MACRO_TESTBIT		7<<17
#define MACRO_TESTNBIT		8<<17
#define MACRO_CLZ			9<<17

//! Hashing
#define	HASH_MASK			1<<17
#define HASH_HASH32			0<<17
#define HASH_HASH			1<<17

//! Checksum calculation and update
#define	CHKSM_MASK			7<<17
#define CHKSM_CHKSUMADD		0<<17
#define CHKSM_CHKSUMSUB		1<<17
#define CHKSM_CHKSUMCALCPKT	2<<17
#define CHKSM_JUMPCHECKSUM	3<<17
#define CHKSM_CRCCALC		4<<17
#define CHKSM_CRCCALCPKT	5<<17

//! locals management
#define	LOC_MASK			1<<17
#define LOC_LOCLD			0<<17
#define	LOC_LOCST			1<<17

//! terminal symbols
#define TERM_MASK			3<<17
#define	TERM_STREG			0<<17
#define	TERM_CNST			1<<17
#define	TERM_LDREG			2<<17

//! overflow check
#define	OVERFLOW_MASK		1<<7
#define NO_OVERFLOW			0<<7
#define OVERFLOW			1<<7

//! operators count
#define OP_COUNT_MASK		3<<9
#define	NO_OPS				0<<9
#define ONE_OP				1<<9
#define TWO_OPS				2<<9

//! register defined
#define OP_DEF_MASK			1<<11
#define NO_DEF				0<<11
#define ONE_DEF				1<<11

//! coprocessor operations
#define OP_COP_MASK			3<<17
/// [GM] Questo l'ho messo a caso, spero sia giusto!
#define COP_COPINIT			4<<17
#define COP_COPIN			0<<17
#define COP_COPOUT			1<<17
#define COP_COPRUN			2<<17

#ifdef __cplusplus
}
#endif

#endif
