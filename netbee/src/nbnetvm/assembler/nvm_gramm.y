/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



%{

#define YYDEBUG 1


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define YYMALLOC malloc
#define YYFREE free
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <stdio.h>

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <nbnetvm.h>
#include <netvm_bytecode.h>

#include "../opcodes.h"
#include "compiler.h"

#ifdef WIN32
#define snprintf _snprintf
#define strdup _strdup
#endif


/* compiler pass */
int pass;

void warn(const char *s);
int nvmparser_error(const char *s);

int eline = 1;			// line number, to keep track of errors
int stream_pointer = 0;		// current position in the created binary stream
int n_ports = 0;		// number of ports of this PE
int n_copros = 0;		// number of coprocessors this PE uses
int nwarns = 0;
int nerrs = 0;
char *errbuf = NULL;
int errbufpos = 0;
int errbufsize = 0;

char *warnbuf = NULL;
int warnbufpos = 0;
int warnbufsize = 0;

pvar	var_head = NULL;	// head of the variable list
pref	ref_head = NULL;	// head of the reference list
pseg	seg_head = NULL;	// head of the segment list
pport	port_head = NULL;	// head of the port list
pcopro	copro_head = NULL;	// head of the coprocessor list
pseg	cur_seg;
pseg	ip_to_lines_seg;

char tmp_char_buf[256];

int32_t ij = 0;
uint32_t id = 0;


swinfo *sw_head;
swinfo *sw_current;
int isnewswitch = 1;

metadata pe_metadata;

/*********************************/
/*   Winsock functions           */
/*********************************/

#ifdef _MSC_VER

/*start winsock*/
// FULVIO PER GIANLUCA: queste routine vengono create in automatico? In caso negativo, non possiamo mettere quelle
// che sono definite in sockutils?
int
wsockinit()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 1, 1);
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
	{
		return -1;
	}
	return 0;
}

int wsockcleanup()
{
	return WSACleanup();
}

struct addrinfo *
nametoaddrinfo(const char *name)
{
	struct addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;	/*not really*/
	error = getaddrinfo(name, NULL, &hints, &res);
	if (error)
		return NULL;
	else
		return res;
}

#else
int wsockinit()
{
	return 0;
}

int wsockcleanup()
{
	return 0;
}
#endif



/*********************************/
/* Emit structures and functions */
/*********************************/

typedef void (*emit_func)(pseg segment, uint32_t value, uint32_t n);
emit_func emitm;	// current code emitter

// emit routine that returns the instructions length
void emit_lenght (pseg segment, uint32_t value, uint32_t len) {
	segment->cur_ip += len;
}

// emit routine that outputs the actual binary code
void emit_code (pseg segment, uint32_t value, uint32_t len) {
	switch (len) {
		case 1:
			segment->istream[segment->cur_ip] = (uint8_t) value;
			segment->cur_ip++;
			break;

		case 2:
			*((uint16_t *)(segment->istream + segment->cur_ip)) = (uint16_t) value;
			segment->cur_ip += 2;
			break;

		case 4:
			*((uint32_t *)(segment->istream + segment->cur_ip)) = value;
			segment->cur_ip += 4;
			break;

		default:;

	}

	return;
}


/**************************************************************/
/* Functions for the implementation of the switch instruction */
/**************************************************************/
swinfo *new_swinfo()
{
	swinfo *info = malloc(sizeof(struct SW_INFO));
	if (info == NULL)
		return NULL;
	memset(info, 0, sizeof(struct SW_INFO));
	if (sw_head == NULL)
	{
		sw_head = info;
		sw_current = info;
	}
	else
	{
		sw_current->next = info;
		sw_current = info;
	}
	return info;
}


/*************************************************/
/* Functions for the implementation of variables */
/*************************************************/

pvar search_var (char *label) {
	pvar v;

	v = var_head;
	while (v != NULL) {
		if (strcmp (v->label, label) == 0)
			return (v);
		else
			v = v->next;
	}
	return (NULL);
}

int insert_var (char *label, int val){
	pvar v;

	v = search_var (label);

	if (v != NULL) {
		nvmparser_error ("Variable already defined");
		return 0;
	}

	if((v = (pvar) malloc (sizeof (var))) != NULL) {
		strcpy (v->label, label);
		v->val = val;
		v->next = var_head;
		var_head = v;
		return 1;
	} else {
		nvmparser_error ("Out of memory");
		return 0;
	}
}

/***************************************************/
/* Functions for the implementation of jump labels */
/***************************************************/

pref search_ref (char *label) {
	pref v;

	v = ref_head;
	while (v != NULL) {
		if (strcmp (v->label, label) == 0)
			return (v);
		else
			v = v->next;
	}
	return (NULL);
}

pref insert_ref (char *label) {
	pref v;

	v = search_ref (label);

	if (v != NULL){
		nvmparser_error ("label already defined");
		return NULL;
	}

	if ((v = (pref) malloc (sizeof (ref))) != NULL) {
		strcpy (v->label, label);
		v->target = 0;
		v->next = ref_head;
		ref_head = v;
		return v;
	} else {
		nvmparser_error ("Out of memory");
		return NULL;
	}
}

/************************************************/
/* Functions for the implementation of segments */
/************************************************/

pseg search_seg (char *label) {
	pseg v;

	v = seg_head;
	while (v != NULL) {
		if (strcmp (v->label, label) == 0)
			return (v);
		else
			v = v->next;
	}
	return (NULL);
}

pseg insert_seg (char *label, int flags) {
	pseg v;

	v = search_seg (label);

	if (v != NULL) {
			nvmparser_error ("Segment declared twice");
			return NULL;
	}

	if ((v = (pseg) malloc (sizeof (seg))) != NULL) {
		strcpy (v->label, label);
		v->references = NULL;
		v->istream = NULL;
		v->flags = flags;
		v->cur_ip = 0;
		v->num_instr = 0;
		v->next = seg_head;
		seg_head = v;
		return v;
	} else {
		nvmparser_error ("Out of memory");
		return NULL;
	}
	
}

int allocate_istream (pseg v, uint32_t size){
	//printf ("Allocating %d bytes for a segment\n", size);
	v->istream = (char *) malloc (size);
	if (v->istream == NULL){
		nvmparser_error ("Out of memory");
		return 0;
	}
	v->cur_ip = 0;
	return 1;
}


/*********************************************/
/* Functions for the implementation of ports */
/*********************************************/

pport search_port (char *label) {
	pport v;

	v = port_head;
	while (v != NULL) {
		if (strcmp(v->label, label) == 0)
			return (v);
		else
			v = v->next;
	}
	return (NULL);
}

int add_port(char *label, int type) {
	pport v;

	v = search_port (label);

	if (v != NULL) {
		nvmparser_error ("port declared twice");
		return 0;
	}

	if ((v = (pport) malloc (sizeof (port))) != NULL) {
		strcpy (v->label, label);
		v->type = type;
		v->number = n_ports;
		v->next = port_head;
		port_head = v;

		// keep track of the number of ports allocated until now
		n_ports++;

		return 1;
	} else {
		nvmparser_error ("Out of memory");
		return 0;
	}
}


/****************************************************/
/* Functions for the implementation of coprocessors */
/****************************************************/

pcopro search_copro (char *label) {
	pcopro v;

	v = copro_head;
	while (v != NULL) {
		if (strcmp(v->label, label) == 0)
			return (v);
		else
			v = v->next;
	}
	return (NULL);
}

int add_copro (char *label, int regs) {
	pcopro v;

	v = search_copro (label);

	if (v != NULL) {
		nvmparser_error ("coprocessor declared twice");
		return -1;
	}

	if ((v = (pcopro) malloc (sizeof (copro))) != NULL) {
		strcpy (v->label, label);
		v->regs = regs;
		v->number = n_copros;
		v->next = copro_head;
		copro_head = v;

		// keep track of the number of ports allocated until now
		n_copros++;

		return v->number;
	} else {
		nvmparser_error ("Out of memory");
		return -1;
	}
}



/**************************************************************/
/* Functions for the implementation of data section variables */
/**************************************************************/

int search_dataval (char *label, uint32_t *target) {
	uint32_t *target2;
	int ret;

	ret = hash_table_lookup (htval, label, strlen (label), (void *) &target2);
	if (target)
		*target = *target2;

	return (ret);
}

int insert_dataval (char *label, uint32_t target) {
	int ret;
	uint32_t *ptr32;
	char *ptrstr;

	ret = nvmFAILURE;

	if (search_dataval (label, NULL)) {
		fprintf (stderr, "dataval label \"%s\" already defined\n", label);
	} else {
		/* We have to duplicate the data to be inserted in the hash table, as the hash table itself only uses
		 * pointers */
		if (!(ptr32 = (uint32_t *) malloc (sizeof (uint32_t)))) {
			nvmparser_error ("Cannot allocate memory");
		} else if (!(ptrstr = strdup (label))) {
			nvmparser_error ("Cannot allocate memory");
		} else {
			*ptr32 = target;
			if (!(hash_table_add (htval, ptrstr, strlen (ptrstr), ptr32))) {
				nvmparser_error ("Cannot add dataval to hash table");
			} else {
				ret = nvmSUCCESS;
			}
		}
	}

	return (ret);
}

/**
 * Parse a C-style string and convert escape sequencies.
 * @param[in] str The string to be parsed.
 * @param[inout] len The input string length (if -1 strlen() will be used), will be updated to the new string length upon return.
 */
char *c_string_unescape (char *str, int *len) {
	int i, j;
	char c, tmp[4], *eptr;

	if (*len == -1)
		*len = strlen (str);

	/* i is the index in the input string, j in the output */
	i = 0, j = 0;
	while (i < *len) {
		c = str[i];

		if (c == '\\' && i + 1 < *len) {
			switch (str[i + 1]) {
				case 'x':
				case 'X':
					if (i + 1 + 2 < *len) {
						/* Copy relevant part to temp string and parse it */
						tmp[0] = str[i + 2];
						tmp[1] = str[i + 3];
						tmp[2] = '\0';
						str[j] = (char) strtol (tmp, &eptr, 16);
						if (tmp[0] != '\0' && *eptr != '\0')
							fprintf (stderr, "WARNING: Cannot convert hexadecimal number\n");
						i += 4, j++;
					} else {
						fprintf (stderr, "ERROR: Premature end of string\n");
						i++;
					}
					break;
				case '\\':
					/* Turn 2 backslashes into 1 */
					str[j] = c;
					i += 2, j++;
					break;
				default:
					/* Actually this should be an octal number... */
					fprintf (stderr, "WARNING: Unsupported escape sequence \"\\%c\", leaving as is\n", str[i + 1]);
					fprintf (stderr, "--> %s\n", str);
					str[j] = c;
					i++, j++;
					break;
			}
		} else {
			str[j] = c;
			i++, j++;
		}
	}

	/* Terminate (so that we have chances the output string remains C-style, but it depends on strin string contents) */
	str[j] = '\0';
	*len = j;

	return (str);
}


%}

/************************/
/* Instruction Keywords */
/************************/

/* tokens for assembly syntax and intructions */

%token T_DB
%token T_DW
%token T_DD
%token T_EQU
%token SEGMENT
%token ENDS
%token S_PORT
%token S_DATA
%token S_METADATA
%token T_HOST
%token T_PORT
%token T_ADDR
%token NUMBER
%token STRING
%token IDENTIFIER
%token LABEL
%token PORT_TYPE
%token COPRO_DECL
%token DATAMEMSIZE
%token NETPENAME
%token INFOPARTSIZE
%token LOCALS
%token STACKSIZE
%token BYTEORDER
%token DEFAULTCASE

/* tokens for opcodes */

%token T_NO_ARG_INST
%token T_1BYTE_ARG_INST
%token T_1WORD_ARG_INST
%token T_1INT_ARG_INST
%token T_2INT_ARG_INST
%token T_1LABEL_INST
%token T_1_SHORT_LABEL_INST
%token T_1LABEL_1BYTE_ARG_INST
%token T_1LABEL_1WORD_ARG_INST
%token T_1LABEL_1INT_ARG_INST
%token T_1_PUSH_PORT_ARG_INST
%token T_1_PULL_PORT_ARG_INST
%token T_JMP_TBL_LKUP_INST
%token T_COPRO_INIT_INST
%token T_2_COPRO_IO_ARG_INST

%start source

%union {
int	number;
char	id[1024];
}

%type <number> T_NO_ARG_INST
%type <number> T_1BYTE_ARG_INST
%type <number> T_1WORD_ARG_INST
%type <number> T_1INT_ARG_INST
%type <number> T_2INT_ARG_INST
%type <number> T_1LABEL_1BYTE_ARG_INST
%type <number> T_1LABEL_1WORD_ARG_INST
%type <number> T_1LABEL_1INT_ARG_INST
%type <number> T_1LABEL_INST
%type <number> T_1_SHORT_LABEL_INST
%type <number> T_1_PUSH_PORT_ARG_INST
%type <number> T_1_PULL_PORT_ARG_INST
%type <number> T_JMP_TBL_LKUP_INST
%type <number> T_COPRO_INIT_INST
%type <number> T_2_COPRO_IO_ARG_INST


%type <number> PORT_TYPE
%type <number> port_decl
%type <number> port_list
%type <number> port_id

%type <number> COPRO_DECL
%type <number> copro_directive

%type <number> NUMBER
%type <number> expr
%type <number> ip_address
%type <number> ip_port

%type <id> STRING
%type <id> IDENTIFIER
%type <id> LABEL
%type <id> T_ADDR
%type <id> T_PORT
%type <id> literaladdr
%type <id> S_PORT
%type <id> S_DATA
%type <id> S_METADATA

%left '+' '-'
%left '*' '/'

%%

source		: segments
			| equivalences segments
			;

segment			: SEGMENT IDENTIFIER {
				Output_1("segment %s\n", $2);
				if(pass == CALCULATE_REFERENCES){
					emitm = emit_lenght;
					cur_seg = insert_seg($2, 0);
					cur_seg->max_stack_size = -1;
					cur_seg->locals_num	= -1;
					if (strcmp($2, ".pull") == 0)
						cur_seg->flags = BC_CODE_SCN|BC_PULL_SCN;
					if (strcmp($2, ".push") == 0)
						cur_seg->flags = BC_CODE_SCN|BC_PUSH_SCN;
					if (strcmp($2, ".init") == 0)
						cur_seg->flags = BC_CODE_SCN|BC_INIT_SCN;
				} else {
					emitm = emit_code;
					cur_seg = search_seg($2);
					if(allocate_istream(cur_seg, cur_seg->cur_ip) == 0){
						nvmparser_error("failed to allocate segment memory");
					}

					if (cur_seg->max_stack_size == -1)
						cur_seg->max_stack_size = 0;

					if (cur_seg->locals_num == -1)
						cur_seg->locals_num = 0;
					
					sprintf(tmp_char_buf, "%s_ilm", $2);
					tmp_char_buf[8] = '\0';
					ip_to_lines_seg = insert_seg(tmp_char_buf, (cur_seg->flags & ~BC_CODE_SCN) | BC_INSN_LINES_SCN);
					
					if(allocate_istream(ip_to_lines_seg, cur_seg->num_instr*8) == 0)
						nvmparser_error("failed to allocate segment memory");
				}

				cur_seg->cur_ip = 0;
				cur_seg->num_instr = 0;
				emitm(cur_seg, cur_seg->max_stack_size, 4);
				emitm(cur_seg, cur_seg->locals_num, 4);
			} directives code ENDS
			| SEGMENT IDENTIFIER {
				Output_1("segment %s\n", $2);
				if(pass == CALCULATE_REFERENCES){
					emitm = emit_lenght;
					cur_seg = insert_seg($2, 0);
					cur_seg->max_stack_size = -1;
					cur_seg->locals_num	= -1;
					if (strcmp($2, ".pull") == 0)
						cur_seg->flags = BC_CODE_SCN|BC_PULL_SCN;
					if (strcmp($2, ".push") == 0)
						cur_seg->flags = BC_CODE_SCN|BC_PUSH_SCN;
					if (strcmp($2, ".init") == 0)
						cur_seg->flags = BC_CODE_SCN|BC_INIT_SCN;
				} else {
					emitm = emit_code;
					cur_seg = search_seg($2);
					if(allocate_istream(cur_seg, cur_seg->cur_ip) == 0){
						nvmparser_error("failed to allocate segment memory");
					}

					if (cur_seg->max_stack_size == -1)
						cur_seg->max_stack_size = 0;

					if (cur_seg->locals_num == -1)
						cur_seg->locals_num = 0;
					
					sprintf(tmp_char_buf, "%s_ilm", $2);
					tmp_char_buf[8] = '\0';
					ip_to_lines_seg = insert_seg(tmp_char_buf, (cur_seg->flags & ~BC_CODE_SCN) | BC_INSN_LINES_SCN);
					
					if(allocate_istream(ip_to_lines_seg, cur_seg->num_instr*8) == 0)
						nvmparser_error("failed to allocate segment memory");
				}

				cur_seg->cur_ip = 0;
				emitm(cur_seg, cur_seg->max_stack_size, 4);
				emitm(cur_seg, cur_seg->locals_num, 4);
			} code ENDS
			| SEGMENT S_PORT {
				Output_2("%s segment at line %d\n", $2, eline);

				if(pass == CALCULATE_REFERENCES){
					emitm = emit_lenght;
					cur_seg = insert_seg($2, BC_PORT_SCN);
				} else {
					emitm = emit_code;
					cur_seg = search_seg($2);
					if(allocate_istream(cur_seg, cur_seg->cur_ip) == 0) {
						nvmparser_error("failed to allocate segment memory");
					}
				}

				cur_seg->cur_ip = 0;
				emitm(cur_seg, n_ports, 4);
			} port_table ENDS
			| SEGMENT S_DATA BYTEORDER LABEL {
				Output_2("%s segment at line %d\n", $2, eline);

				if (pass == CALCULATE_REFERENCES) {
					emitm = emit_lenght;
					cur_seg = insert_seg($2, BC_INITIALIZED_DATA_SCN);
					cur_seg->max_stack_size = 0;
					cur_seg->locals_num	= 0;

					if (strcmp ($4, "big_endian") == 0 || strcmp ($4, "network") == 0) {
						Output("Requested byte order is BIG endian\n");
						cur_seg -> endianness = SEGMENT_BIG_ENDIAN;
					} else if (strcmp ($4, "little_endian") == 0) {
						Output("Requested byte order is LITTLE endian\n");
						cur_seg -> endianness = SEGMENT_LITTLE_ENDIAN;
					} else {
						Output("Requested byte order is UNDEFINED\n");
						cur_seg -> endianness = SEGMENT_UNDEFINED_ENDIANNESS;
					}

					if (htval)
						nvmparser_error("dataval hash table already allocated, this should not happen");
					else
						htval = hash_table_new (0);
				} else {
					emitm = emit_code;
					cur_seg = search_seg($2);
					if(allocate_istream(cur_seg, cur_seg->cur_ip) == 0){
						nvmparser_error("failed to allocate segment memory");
					}
				}

				cur_seg -> cur_ip = 0;
				emitm(cur_seg, cur_seg -> endianness, 4);
			} data_decls ENDS
			| SEGMENT S_METADATA {
				Output_2("%s segment at line %d\n", $2, eline);

				if (pass == CALCULATE_REFERENCES){
					emitm = emit_lenght;
					cur_seg = insert_seg($2, BC_METADATA_SCN);
					cur_seg -> cur_ip = 0;
					cur_seg->max_stack_size = 0;
					cur_seg->locals_num	= 0;
					pe_metadata.datamem_size = -1;
					pe_metadata.infopart_size = -1;
					pe_metadata.copros_no = 0;
					pe_metadata.netpename[0] = '\0';
				} else {
					emitm = emit_code;
					cur_seg = search_seg($2);

					if (pe_metadata.datamem_size == -1)
						pe_metadata.datamem_size = 0;

					if (pe_metadata.infopart_size == -1)
						pe_metadata.infopart_size = 0;

					/* This segment is fully emitted here */
					if (allocate_istream(cur_seg, 4 + strlen (pe_metadata.netpename) + 4 + 4 + 4 + cur_seg -> cur_ip) == 0){
						nvmparser_error("failed to allocate segment memory");
					}
					cur_seg -> cur_ip = 0;

					/* PE name length and actual name */
					emitm (cur_seg, (uint32_t) strlen (pe_metadata.netpename), 4);
					for (id = 0; id < strlen (pe_metadata.netpename); id++)
						emitm(cur_seg, pe_metadata.netpename[id], 1);

					/* Requested data memory size */
					emitm(cur_seg, (uint32_t) pe_metadata.datamem_size, 4);

					/* Requested info partition size */
					emitm(cur_seg, (uint32_t) pe_metadata.infopart_size, 4);

					/* Number of used coprocessors */
					emitm(cur_seg, (uint32_t) pe_metadata.copros_no, 4);

					/* For every coprocessor, name length and actual name */
					for (ij = 0; ij < pe_metadata.copros_no; ij++) {
						emitm (cur_seg, (uint32_t) strlen (pe_metadata.copros[ij]), 4);
						for (id = 0; id < strlen (pe_metadata.copros[ij]); id++)
							emitm(cur_seg, pe_metadata.copros[ij][id], 1);
					}
				}
			} metadata_directives ENDS

			;

metadata_directive	: copro_directive
			| datamem_directive
			| netpename_directive
			| infosize_directive
			;

metadata_directives	: metadata_directive
			| metadata_directives metadata_directive
			;

netpename_directive	: NETPENAME LABEL {
				if (cur_seg->flags != BC_METADATA_SCN) {
					nvmparser_error("The .infopart_size directive can only be used in the .metadata segment");
				} else {
					if (pass == CALCULATE_REFERENCES) {
						if (pe_metadata.netpename[0] == '\0')
							strncpy (pe_metadata.netpename, $2, MAX_NETPE_NAME);
						else
							nvmparser_error("NetPE name redefined");
					}
				}
			}
			;

infosize_directive	: INFOPARTSIZE NUMBER {
				if (cur_seg->flags != BC_METADATA_SCN) {
					nvmparser_error("The .infopart_size directive can only be used in the .metadata segment");
				} else {
					if (pass == CALCULATE_REFERENCES) {
						if (pe_metadata.infopart_size != -1)
							nvmparser_error("redeclaration of infopart_size");
						else
							if ($2 < 0)
								nvmparser_error("cannot declare a negative infopart_size");
						else
							pe_metadata.infopart_size = $2;
					}
				}
			}
			;

copro_directive		: COPRO_DECL LABEL {
				$$=$<number>0;

				if (pass == CALCULATE_REFERENCES){
					Output_1 ("Coprocessor: name=%s\n", $2);
					id = add_copro ($2, -1);
					Output_1 ("Assigned ID: %d\n", id);

					strncpy (pe_metadata.copros[pe_metadata.copros_no], $2, MAX_COPRO_NAME);
					pe_metadata.copros_no++;

					emitm(cur_seg, (uint32_t) strlen ($2), 4);
					for (id = 0; id < strlen($2); id++)
						emitm(cur_seg, $2[id], 1);
				}
			}
			;

segments		: segment
			| segments segment
			;

directives		: directive
			| directives directive
			;

directive		: datamem_directive
			| local_directive
			| stacksize_directive
			;

datamem_directive:	DATAMEMSIZE NUMBER {
				if (cur_seg->flags != BC_METADATA_SCN) {
					nvmparser_error("The .datamem_size directive can only be used in the .metadata segment");
				} else {
					if (pass == CALCULATE_REFERENCES) {
						if (pe_metadata.datamem_size != -1)
							nvmparser_error("redeclaration of datamem_size");
						else
							if ($2 < 0)
								nvmparser_error("cannot declare a negative datamem_size");
						else
							pe_metadata.datamem_size = $2;
					}
				}
			}
			;

local_directive:	LOCALS NUMBER
						{
							if(pass == CALCULATE_REFERENCES)
							{
								if (cur_seg->locals_num != -1)
									nvmparser_error("redeclaration of locals");
								else
								if ($2 < 0)
									nvmparser_error("cannot declare a negative number of locals");
								else
									cur_seg->locals_num = $2;
							}
						}
					;

stacksize_directive:	STACKSIZE NUMBER
						{
							if(pass == CALCULATE_REFERENCES)
							{
								if (cur_seg->max_stack_size != -1)
									nvmparser_error("redeclaration of max stack size");
								else
								if ($2 < 0)
									nvmparser_error("cannot declare a negative number of max stack size");
								else
									cur_seg->max_stack_size = $2;
							}
						}
						;

code			: lines
			| {warn("Empty code segment\n");}
			;

line		: 
			{
				if(pass != CALCULATE_REFERENCES)
				{
					//first 8 bytes of the segment are reserved for maxstacksize and locals
					emit_code (ip_to_lines_seg, cur_seg->cur_ip - 8, 4);
					emit_code (ip_to_lines_seg, eline, 4);
				}
				cur_seg->num_instr++;
			}
			instr
			
			| label
			| equivalence
			| error		// this is for error recovery
			;

lines			: line
			| lines line
			;


port_table		: {warn("empty port table\n");}
			| port_decls
			;


port_decls		: port_decl
			| port_decl port_decls
			;


port_decl		: PORT_TYPE port_list {$$=$1;}
			| error {}
			;


port_list		: port_id {$$=$<number>0;}
			| port_list ',' port_id
			;


port_id			: LABEL {
				$$=$<number>0;

				if(pass == CALCULATE_REFERENCES){
					Output_2("port: name=%s, type=%d\n", $1, $<number>0);
					add_port($1, $<number>0);
				}

				emitm(cur_seg, $<number>0, 4);
			}
			;

label:			LABEL ':'	{
								if(pass == CALCULATE_REFERENCES){
									pref lr;
									lr = insert_ref($1);
									lr->target = cur_seg->cur_ip;
									Output_2("label %s, target=%d\n", $1, lr->target);
								}
							}
				;

equivalence:	LABEL T_EQU expr {
							Output_2("Equivalence %s=%d\n", $1, $3);
							if(pass == CALCULATE_REFERENCES) insert_var($1,$3);
							}
				;

equivalences		: equivalence
			| equivalences equivalence
			;

instr			: no_arg_inst
			| byte_arg_inst
			| word_arg_inst
			| int_arg_inst
			| double_int_arg_inst
			| jump_inst
			| short_jump_inst
			| byte_jump_inst
			| word_jump_inst
			| int_jump_inst
			| push_port_arg_inst
			| pull_port_arg_inst
			| copro_io_arg_inst
			| jmp_tbl_lkup_inst
			
			;


push_port_arg_inst:	T_1_PUSH_PORT_ARG_INST LABEL {
				if (pass == CALCULATE_REFERENCES) {
					emitm(cur_seg, (char)$1, 1);
					emitm(cur_seg, 0, 4);
				} else {
					pport dport;

					Output_1("push port instruction, port name=%s\n", $2);
					dport = search_port($2);
					if (dport == NULL) {
						nvmparser_error("referenced port not found");
						break;
					}
					if (!(nvmFLAG_ISSET(dport->type, nvmPORT_COLLECTOR) && \
								nvmFLAG_ISSET(dport->type, nvmCONNECTION_PUSH)))
						nvmparser_error("instruction can be issued only on an output push port");
					emitm(cur_seg, (char) $1, 1);
					emitm(cur_seg, dport->number, 4);
					//Output_1 ("emitted port %d\n", dport -> number);
				}
			}
				;

copro_io_arg_inst	: T_2_COPRO_IO_ARG_INST LABEL ',' expr {
				if (pass == CALCULATE_REFERENCES) {
					emitm(cur_seg, (char) $1, 1);	// Opcode
					emitm(cur_seg, 0, 4);		// Coprocessor ID
					emitm(cur_seg, 0, 4);		// Coprocessor register or operation ID
				} else {
					pcopro dcopro;
					Output_2("Coprocessor I/O instruction, copro name=%s, register/op=%d\n", $2, $4);
					dcopro = search_copro ($2);
					if (dcopro == NULL) {
						nvmparser_error("referenced coprocessor not found");
						break;
					}
					emitm(cur_seg, (char) $1, 1);
					emitm(cur_seg, dcopro->number, 4);
					emitm(cur_seg, (uint32_t) $4, 4);
					//Output_1 ("emitted coprocessor %d\n", dcopro -> number);
				}
			}
			| T_COPRO_INIT_INST LABEL ',' LABEL {
				if (pass == CALCULATE_REFERENCES) {
					emitm(cur_seg, (char) $1, 1);   // Opcode
					emitm(cur_seg, 0, 4);           // Coprocessor ID
					emitm(cur_seg, 0, 4);           // Offset in inited_mem
				} else {
					pcopro dcopro;
					uint32_t target;

					if (!search_dataval ($4, &target)) {
						nvmparser_error ("Undefined dval");
						break;
					}

					Output_2("Coprocessor init instruction, copro name=%s, offset=%d\n", $2, target);
					dcopro = search_copro ($2);
					if (dcopro == NULL) {
						nvmparser_error("referenced coprocessor not found");
						break;
					}
					emitm(cur_seg, (char) $1, 1);
					emitm(cur_seg, dcopro->number, 4);
					emitm(cur_seg, (uint32_t) target, 4);
					//Output_1 ("emitted coprocessor %d\n", dcopro -> number);
				}
			}
			;


pull_port_arg_inst:	T_1_PULL_PORT_ARG_INST LABEL {

											if(pass == CALCULATE_REFERENCES){
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, 0, 4);
											}
											else{
												pport dport;

												Output_1("push port intruction, port name=%s\n", $2);
												dport = search_port($2);
												if(dport==NULL){
													nvmparser_error("referenced port not found");
													break;
												}
												if ( !(nvmFLAG_ISSET(dport->type, nvmPORT_EXPORTER) && nvmFLAG_ISSET(dport->type, nvmCONNECTION_PULL)) ) nvmparser_error("instruction can be issued only on an input pull port");
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, dport->number, 4);
											}
										}
				;


no_arg_inst:	T_NO_ARG_INST	{
									emitm(cur_seg, (char)$1, 1);
								}
				;

byte_arg_inst:	T_1BYTE_ARG_INST expr	{
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, (char)$2, 1);
										}
				;

word_arg_inst:	T_1WORD_ARG_INST expr	{
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, (short)$2, 2);
										}
				;

int_arg_inst:	T_1INT_ARG_INST expr	{
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, (int)$2, 4);
										}
				;

double_int_arg_inst:	T_2INT_ARG_INST expr ',' expr	{
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, (int)$2, 4);
											emitm(cur_seg, (int)$4, 4);
										}
				;

jump_inst:	T_1LABEL_INST LABEL	{
									if(pass == CALCULATE_REFERENCES){
										emitm(cur_seg, (char)$1, 1);
										emitm(cur_seg, 0, 4);
									}
									else{
										pref jr;

										jr = search_ref($2);
										if(jr==NULL){
											nvmparser_error("target label not found");
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, 0, 4);
										}
										else{
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, jr->target-cur_seg->cur_ip-4, 4);
											Output_3("simple jmp, label=%s, target=%d, cur_pos=%d\n", $2, jr->target, cur_seg->cur_ip);
										}
									}
								}
				;

short_jump_inst:	T_1_SHORT_LABEL_INST LABEL	{
									if(pass == CALCULATE_REFERENCES){
										emitm(cur_seg, (char)$1, 1);
										emitm(cur_seg, 0, 1);
									}
									else{
										pref jr;

										jr = search_ref($2);
										if(jr==NULL){
											nvmparser_error("target label not found");
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, 0, 1);
										}
										else if(jr->target-cur_seg->cur_ip - 4 > 255){
											nvmparser_error("jump out of range");
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, 0, 1);
										}
										else{
											emitm(cur_seg, (char)$1, 1);
											emitm(cur_seg, jr->target-cur_seg->cur_ip-1, 1);
											Output_3("short jmp, label=%s, target=%d, cur_pos=%d\n", $2, jr->target, cur_seg->cur_ip);
										}
									}
								}
				;


byte_jump_inst:	T_1LABEL_1BYTE_ARG_INST	expr ',' LABEL{
											if(pass == CALCULATE_REFERENCES){
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, 0, 4);
												emitm(cur_seg, 0, 1);
											}
											else{
												pref jr;

												jr = search_ref($4);
												if(jr==NULL) nvmparser_error("target label not found");
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, jr->target-cur_seg->cur_ip-4, 4);
												emitm(cur_seg, *(char*)&($4), 1);
												Output_3("byte jmp, label=%s, target=%d, cur_pos=%d\n", $4, jr->target, cur_seg->cur_ip);
											}
										}
				;


word_jump_inst:	T_1LABEL_1WORD_ARG_INST	expr ',' LABEL{
											if(pass == CALCULATE_REFERENCES){
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, 0, 4);
												emitm(cur_seg, 0, 2);
											}
											else{
												pref jr;

												jr = search_ref($4);
												if(jr==NULL) nvmparser_error("target label not found");
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, jr->target-cur_seg->cur_ip-4, 4);
												emitm(cur_seg, *(char*)&($4), 2);
												Output_3("word jmp, label=%s, target=%d, cur_pos=%d\n", $4, jr->target, cur_seg->cur_ip);
											}
										}
				;


int_jump_inst:	T_1LABEL_1INT_ARG_INST	expr ',' LABEL{
											if(pass == CALCULATE_REFERENCES){
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, 0, 4);
												emitm(cur_seg, 0, 4);
											}
											else{
												pref jr;

												jr = search_ref($4);
												if(jr==NULL) nvmparser_error("target label not found");
												emitm(cur_seg, (char)$1, 1);
												emitm(cur_seg, jr->target-cur_seg->cur_ip-4, 4);
												emitm(cur_seg, *(char*)&($4), 4);
												Output_3("int jmp, label=%s, target=%d, cur_pos=%d\n", $4, jr->target, cur_seg->cur_ip);
											}
										}
				;

jmp_tbl_lkup_inst: T_JMP_TBL_LKUP_INST NUMBER ':' pairs {
										if(pass == CALCULATE_REFERENCES){
												emitm(cur_seg, (char)$1, 1); /*opcode*/
												sw_current->sw_npairs = $2;
												if (sw_current->sw_npairs != sw_current->sw_pair_cnt)
													nvmparser_error("wrong number of cases");
												if (sw_current->sw_npairs > MAX_SW_CASES)
													nvmparser_error("too many cases in switch instruction");
												emitm(cur_seg, 0, 4); /*default target*/
												emitm(cur_seg, 0, 4); /*number of match pairs*/
												emitm(cur_seg, 0, 8 * sw_current->sw_npairs); /*value: label for sw_npairs times*/
												sw_current->sw_next_inst_ip = cur_seg->cur_ip;
												//!\todo OM risistemare questa roba dopo aver verificato memory leaks!!!
												//sw_cases = calloc(sw_npairs, sizeof (case_pair));
												//if (sw_current->sw_cases == NULL)
												//	nvmparser_error("mem allocation failure");
												sw_current->sw_pair_cnt = 0;
												isnewswitch = 1;
											}
											else{
												unsigned int i=0;
												emitm(cur_seg, (char)$1, 1);
												if (sw_current->defaultcase.target == 0)
													emitm(cur_seg, 0, 4);
												else
													emitm(cur_seg, sw_current->defaultcase.target, 4);
												emitm(cur_seg, sw_current->sw_npairs, 4);
												for (i = 0; i < sw_current->sw_npairs; i++)
												{
													emitm(cur_seg, sw_current->sw_cases[i].key, 4);
													emitm(cur_seg, sw_current->sw_cases[i].target, 4);
												}
												//if (sw_cases)
												//	free (sw_cases);
												sw_current = sw_current->next;
											}
							}
				;


pairs			: pair
			| pairs pair
			;

pair			: NUMBER ':' LABEL {
								if(pass == CALCULATE_REFERENCES)
								{
									if (isnewswitch)
									{
										new_swinfo();
										isnewswitch = 0;
									}
									sw_current->sw_pair_cnt++;
								}
								else
								{
									pref jr;
									sw_current->sw_cases[sw_current->sw_pair_cnt].key = (int)$1;
									jr = search_ref($3);
									if(jr==NULL)
										nvmparser_error("target label not found");
									sw_current->sw_cases[sw_current->sw_pair_cnt].target = jr->target - sw_current->sw_next_inst_ip;
									sw_current->sw_pair_cnt++;
								}
							}

			| DEFAULTCASE ':' LABEL {
								if(pass == CALCULATE_REFERENCES)
								{
									if (isnewswitch)
									{
										new_swinfo();
										isnewswitch = 0;
									}
									if (sw_current->sw_default_ok)
										nvmparser_error("only one default case is allowed for a switch instruction");
								/*
									memset(sw_current->sw_default_label, 0, LBL_MAX_SZ);
									strncpy(sw_current->sw_default_label, (char*) $3, LBL_MAX_SZ - 1);
								*/
									sw_current->sw_default_ok = 1;
								}
								else
								{
									pref jr;
									jr = search_ref($3);
									if(jr==NULL)
										nvmparser_error("target label not found");
									sw_current->defaultcase.target = jr->target - sw_current->sw_next_inst_ip;
								}
							}
			;

db			: LABEL T_DB expr {
				if (pass == CALCULATE_REFERENCES) {
					Output_2 ("DB at offset %d, label=%s\n", cur_seg->cur_ip, $1);

					insert_dataval ($1, cur_seg -> cur_ip);

					emitm(cur_seg, (uint8_t) $3, 1);
				} else {
					emitm(cur_seg, (uint8_t) $3, 1);
				}
			}
			| LABEL T_DB STRING {
				int i, len;

				len = -1;
				c_string_unescape ($3, &len);

				if (pass == CALCULATE_REFERENCES) {
					Output_3 ("DB string (len=%d) at offset %d, label=%s\n", len, cur_seg->cur_ip, $1);

					insert_dataval ($1, cur_seg -> cur_ip);

					for (i = 0; i < len; i++)
						emitm(cur_seg, (uint8_t) $3[i], 1);
				} else {
					for (i = 0; i < len; i++)
						emitm(cur_seg, (uint8_t) $3[i], 1);
				}
			}
			;

dw			: LABEL T_DW expr {
				uint16_t val;

				if (pass == CALCULATE_REFERENCES) {
					Output_3 ("DW at offset %d, value=%hd label=%s\n", cur_seg->cur_ip, $3, $1);

					insert_dataval ($1, cur_seg -> cur_ip);

					val = 0;	// Don't care for now
				} else {
					val = (uint16_t) $3;

					/* Adjust endianness */
					if (cur_seg -> endianness == SEGMENT_BIG_ENDIAN)
						val = bo_my2big_16 (val);
					else if (cur_seg -> endianness == SEGMENT_LITTLE_ENDIAN)
						val = bo_my2little_16 (val);
				}

				emitm(cur_seg, val, 2);
			}
			;

dd			: LABEL T_DD expr {
				uint32_t val;

				if (pass == CALCULATE_REFERENCES) {
					Output_3 ("DD at offset %d, value=%d label=%s\n", cur_seg->cur_ip, $3, $1);

					insert_dataval ($1, cur_seg -> cur_ip);

					val = 0;	// Don't care for now
				} else {
					val = (uint32_t) $3;

					/* Adjust endianness */
					if (cur_seg -> endianness == SEGMENT_BIG_ENDIAN)
						val = bo_my2big_32 (val);
					else if (cur_seg -> endianness == SEGMENT_LITTLE_ENDIAN)
						val = bo_my2little_32 (val);
				}

				emitm(cur_seg, val, 4);
			}
			;

data_decls		: data_decls data_decl
			| data_decl
			;

data_decl		: db
			| dw
			| dd
			;

expr			: expr '+' expr	{$$ = $1 + $3;}
			| expr '-' expr	{$$ = $1 - $3;}
			| expr '*' expr	{$$ = $1 * $3;}
			| expr '/' expr	{$$ = $1 / $3;}
			| '(' expr ')'	{$$ = $2;}
			| '-' expr		{$$ = -$2;}
			| NUMBER			{$$ = $1;}
			| LABEL {
					pvar evar;
					evar = search_var($1);

					if(evar == NULL){
						char errstr[64];
						strcpy(errstr, "Variable '");
						strcat(errstr, $1);
						strcat(errstr, "' not found");
						nvmparser_error(errstr);
						$$ = 1;
					}
					else{
						$$ = evar->val;
					}
				}
			| ip_address			{$$=$1;}
			| ip_port			{$$=$1;}
			;

/* Perch\E9 solo MSC? */
ip_address:		T_HOST literaladdr {
#ifdef _MSC_VER
					struct addrinfo *res;
					res = nametoaddrinfo($2);
					if(res != NULL)
					{
						$$ = ntohl((((struct sockaddr_in*)res->ai_addr))->sin_addr.S_un.S_addr);
						Output_2("addr %s - %d\n", $2, $$);
					}
					else
					{
						if(pass == CALCULATE_REFERENCES)
							nvmparser_error("Unable to find host");
						else
							nvmparser_error(NULL);
					}

#else
							nvmparser_error("This version of the NetVM assembler doesn't support ip address resolution");
#endif
					}
				;

literaladdr:	T_ADDR	{strcpy($$,$1);} |
				LABEL	{strcpy($$,$1);}
				;

ip_port:		T_PORT LABEL	{
											/*
								#ifndef ARCH_RUNTIME_OCTEON
											struct servent *s;
											if((s = getservbyname($2, NULL)) != NULL)
											{
												//int i=ntohs(s->s_port);
												$$ =  ntohs(s->s_port);
											}
											else
											{
												nvmparser_error("Unable to resolve port");
											}
								#endif
								*/
										}
				;


//expr_list:		expr | expr "," expr_list;

%%

void warn(const char *s)
{
	fprintf (stderr, "NetIL parser WARNING at line %d\n", eline);

	if (warnbuf != NULL)
	{
		if (s != NULL)
			warnbufpos += snprintf(warnbuf + warnbufpos, warnbufsize-warnbufpos, "Line %d - %s\n", eline, s);
		else
			warnbufpos += snprintf(warnbuf + warnbufpos, warnbufsize-warnbufpos, "Line %d - Unknown\n", eline);
	}

	nwarns++;
}

/* [GM] Dato che mi sfugge la logica dietro alla bufferizzazione degli errori, ho modificato questa funzione in modo che
   stampi direttamente il messaggio. */
int nvmparser_error(const char *s)
{
	fprintf (stderr, "NetIL parser ERROR at line %d: %s\n", eline, s);

	if(errbuf != NULL)
	{
		if (s != NULL)
			errbufpos += snprintf(errbuf + errbufpos, errbufsize - errbufpos, "Line %d - %s\n", eline, s);
		else
			errbufpos += snprintf(errbuf + errbufpos, errbufsize - errbufpos, "Line %d - Unknown\n", eline);
		nerrs++;
	}

	return 1;
}

void yy_set_error_buffer(const char *buffer, int buffer_size)
{
	errbuf = (char*) buffer;
	errbufpos = 0;
	errbufsize = buffer_size;
}

void yy_set_warning_buffer(const char *buffer, int buffer_size)
{
	warnbuf = (char*) buffer;
	warnbufpos = 0;
	warnbufsize = buffer_size;
}

pseg yy_assemble(char* program, int debug)
{
	/*!\todo Cleanup code, is there anything more to clean up? [GM]  */
	var_head = NULL;
	ref_head = NULL;
	seg_head = NULL;
	port_head = NULL;
	copro_head = NULL;
	if (htval) {
		hash_table_destroy (htval, free, free);
		htval = NULL;
	}
	n_ports = 0;
	n_copros = 0;
	eline = 1;
	sw_head = NULL;
	sw_current = NULL;
	isnewswitch = 1;

	pass = CALCULATE_REFERENCES;
	netvm_lex_init(program ? program : "");

	nvmparser_debug = debug;
	nvmparser_parse();

	if(nerrs != 0)
	{
		fprintf (stderr, "%d errors detected while calculating references\n", nerrs);
		wsockcleanup();

		return NULL;
	}
	//fprintf (stderr, "First pass done\n");

	eline = 1; // reset the lines counter

	netvm_lex_cleanup();
	pass = EMIT_CODE;
	sw_current = sw_head;
	netvm_lex_init(program ? program : "");
	nvmparser_parse();
	if(nerrs != 0)
	{
		fprintf (stderr, "%d errors detected while emitting code\n", nerrs);
		wsockcleanup();

		return NULL;
	}

	netvm_lex_cleanup();
	wsockcleanup();

	return seg_head;
}
