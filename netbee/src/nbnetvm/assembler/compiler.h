/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#ifndef __COMPILER_H__
#define __COMPILER_H__

/* Include config.h if needed */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Used to keep information about labels */
#include "hashtable.h"

/* These are copied from int_structs.h and coprocessor.h (TODO) */
#define MAX_NETPE_NAME 64
#define MAX_COPRO_NUMBER 10
#define MAX_COPRO_NAME 64

//#define _ASSEMBLER_DEBUG
//#include "gramm.tab.h"

/* debug functions */
#ifdef _ASSEMBLER_DEBUG

#define Output(STR) printf(STR)
#define Output_1(FMT, ARG) printf(FMT, ARG)
#define Output_2(FMT, ARG1, ARG2) printf(FMT, ARG1, ARG2)
#define Output_3(FMT, ARG1, ARG2, ARG3) printf(FMT, ARG1, ARG2, ARG3)
//#define OutputLoud(STR) printf(STR)

#else

#define Output(STR)
#define Output_1(FMT, ARG)
#define Output_2(FMT, ARG1, ARG2)
#define Output_3(FMT, ARG1, ARG2, ARG3)

#endif

#define CALCULATE_REFERENCES 0
#define EMIT_CODE 1

/* global vars and structs */
typedef struct var
{
	char label[256];
	int val;
	struct var *next;
} var, *pvar;


typedef struct case_pair
{
	int key;
	int target;
	struct case_pair *next;
} case_pair;

#define LBL_MAX_SZ 256

#define MAX_SW_CASES (4096)	// Hope these will be enough for everyone ;).

struct SW_INFO;
typedef struct SW_INFO
{
	uint32_t		sw_npairs;	// number of match pairs of the switch
	//char	sw_default_label[LBL_MAX_SZ] ; // target label for the default case of the switch
	int		sw_next_inst_ip;
	uint32_t		sw_pair_cnt;
	int		sw_default_ok;
	case_pair sw_cases[MAX_SW_CASES];
	case_pair defaultcase;
	struct SW_INFO *next;
} swinfo;


typedef struct metadata_s {
	int datamem_size;
	int infopart_size;
	int copros_no;
	char copros[MAX_COPRO_NUMBER][MAX_COPRO_NAME];
	char netpename[MAX_NETPE_NAME];
} metadata;


typedef struct reference
{
	char label[256];
	int target;
	struct reference *next;
} ref, *pref;

enum endianness_e {
	SEGMENT_UNDEFINED_ENDIANNESS,
	SEGMENT_BIG_ENDIAN,
	SEGMENT_LITTLE_ENDIAN
};

typedef struct segment
{
	char label[256];
	uint32_t cur_ip;		// Current instruction pointer.
	uint32_t num_instr;
	char *istream;		// Instruction buffer, contains the X86 generated code.
	pref references;
	int flags;
	int max_stack_size;	//used in case of code segments
	int locals_num;		//used in case of code segments
	enum endianness_e endianness;	// used for data segments
	struct segment *next;
} seg, *pseg;

typedef struct port
{
	char label[256];
	int type;
	int number;
	struct port *next;
} port, *pport;

typedef struct coprocessor
{
	char label[256];
	int regs;
	int number;		// Coprocessor ID
	struct coprocessor *next;
} copro, *pcopro;

extern hash_table *htval;

/* Imported from the scanner */
void netvm_lex_init(char *buf);
void netvm_lex_cleanup();

int nvmparser_lex(void);

/* Defined in the grammar file */

#ifdef __cplusplus
extern "C" {
#endif

pseg yy_assemble(char* program, int debug);
void yy_set_error_buffer(const char *buffer, int buffer_size);
void yy_set_warning_buffer(const char *buffer, int buffer_size);

#ifdef __cplusplus
}
#endif

#endif /*__COMPILER_H_009347487232368925*/
