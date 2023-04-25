/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




// IR_OPCODE(		code,			sym,			name,			nKids,		nSyms,		op,			details,	Types,		RTypes,		description)

IR_OPCODE(		IR_DEFFLD,		"fdef",			"DEFFLD",		1,		0,		OP_DEFFLD,		IR_STMT,	0,		0,		"define a pdl Field")

IR_OPCODE(		IR_DEFVARI,		"vdecl.i",		"DEFVAR.I",		1,		1,		OP_DEFVAR,		IR_STMT,	I,		0,		"declare an integer variable")

IR_OPCODE(		IR_DEFVARS,		"vdecl.s",		"DEFVAR.S",		1,		1,		OP_DEFVAR,		IR_STMT,	S,		0,		"declare a string variable")

IR_OPCODE(		IR_ASGNI,		"=",			"ASGN.I",		2,		0,		OP_ASGN,		IR_STMT,	I,		0,		"assign integer")

IR_OPCODE(		IR_ASGNS,		"=",			"ASGN.S",		2,		0,		OP_ASGN,		IR_STMT,	S,		0,		"assign string")

IR_OPCODE(		IR_LABEL,		"lbl",			"LABEL",		0,		1,		OP_LABEL,		IR_STMT,	0,		0,		"label")

IR_OPCODE(		IR_IVAR,		"ivar",			"IVAR",			2,		1,		OP_VAR,			IR_TERM,	0,		I,		"NetPDL runtime integer variable")

IR_OPCODE(		IR_SVAR,		"svar",			"SVAR",			2,		1,		OP_VAR,			IR_TERM,	0,		S,		"NetPDL runtime variable (buffer/reference)")

IR_OPCODE(		IR_FIELD,		"fld",			"FIELD",		2,		1,		OP_FIELD,		IR_TERM,	0,		S,		"NetPDL field")

IR_OPCODE(		IR_ITEMP,		"itmp",			"TEMP",			0,		1,		OP_TEMP,		IR_TERM,	0,		I,		"Compiler generated temporary")

IR_OPCODE(		IR_ICONST,		"icon",			"ICON",			0,		1,		OP_CONST,		IR_TERM,	0,		I,		"integer Constant value")

IR_OPCODE(		IR_SCONST,		"scon",			"SCON",			0,		1,		OP_CONST,		IR_TERM,	0,		S,		"string Constant value")

IR_OPCODE(		IR_BCONST,		"bcon",			"BCON",			0,		1,		OP_CONST,		IR_TERM,	0,		B,		"boolean Constant value")

IR_OPCODE(		IR_CINT,		"ci",			"CI",			1,		0,		OP_CINT,		IR_UNOP,	S,		I,		"convert to integer")

/*****************************************************************************************************************************************************************************************************************************************/
IR_OPCODE(		IR_RSTR,		"rs",			"RS",			1,		0,		OP_RSTR,		IR_UNOP,	S,		S,		"a string remains a string")
/*****************************************************************************************************************************************************************************************************************************************/

IR_OPCODE(		IR_CHGBORD,		"chgbord",		"CHGBORD",		1,		0,		OP_CHGBORD,		IR_UNOP,	S,		S,		"change byte order")

IR_OPCODE(		IR_ADDI,		"+",			"ADD.I",		2,		0,		OP_ADD,			IR_BINOP,	I,		I,		"addition")

IR_OPCODE(		IR_SUBI,		"-",			"SUB.I",		2,		0,		OP_SUB,			IR_BINOP,	I,		I,		"subtraction")

IR_OPCODE(		IR_DIVI,		"/",			"DIV.I",		2,		0,		OP_DIV,			IR_BINOP,	I,		I,		"division")

IR_OPCODE(		IR_MULI,		"*",			"MUL.I",		2,		0,		OP_MUL,			IR_BINOP,	I,		I,		"multiply")

IR_OPCODE(		IR_NEGI,		"-",			"NEG.I",		1,		0,		OP_NEG,			IR_UNOP,	I,		I,		"2's complement")

// IR_OPCODE(	code,				sym,			name,			nKids,		nSyms,		op,			details,	Types,		RTypes,		description)

IR_OPCODE(		IR_SHLI,		"<<",			"SHL.I",		2,		0,		OP_SHL,			IR_BINOP,	I,		I,		"shift left")

IR_OPCODE(		IR_MODI,		"%",			"MOD.I",		2,		0,		OP_MOD,			IR_BINOP,	I,		I,		"modulus")

IR_OPCODE(		IR_SHRI,		">>",			"SHR.I",		2,		0,		OP_SHR,			IR_BINOP,	I,		I,		"shift right")
	
IR_OPCODE(		IR_XORI,		"^",			"XOR.I",		2,		0,		OP_XOR,			IR_BINOP,	I,		I,		"bitwise xor")

IR_OPCODE(		IR_ANDI,		"&",			"AND.I",		2,		0,		OP_AND,			IR_BINOP,	I,		I,		"bitwise and")

IR_OPCODE(		IR_ORI,			"|",			"OR.I",			2,		0,		OP_OR,			IR_BINOP,	I,		I,		"bitwise or")

IR_OPCODE(		IR_NOTI,		"~",			"NOT.I",		1,		0,		OP_NOT,			IR_UNOP,	I,		I,		"1's complement")

IR_OPCODE(		IR_ANDB,		"&&",			"AND.B",		2,		0,		OP_BOOLAND,		IR_BINOP,	B,		B,		"bool and")

IR_OPCODE(		IR_ORB,			"||",			"OR.B",			2,		0,		OP_BOOLOR,		IR_BINOP,	B,		B,		"bool or")

IR_OPCODE(		IR_NOTB,		"!",			"NOT.B",		1,		0,		OP_BOOLNOT,		IR_UNOP,	B,		B,		"bool not")

IR_OPCODE(		IR_EQI,			"==",			"EQ.I",			2,		0,		OP_EQ,			IR_BINOP,	I,		B,		"equal to")

IR_OPCODE(		IR_MATCH,		"match",		"MATCH",		2,		0,		OP_MATCH,		IR_BINOP,	S,		B,		"matches the regular expression")

IR_OPCODE(		IR_CONTAINS,		"contains",		"CONTAINS",		2,		0,		OP_CONTAINS,		IR_BINOP,	S,		B,		"contains the string expression")
		
IR_OPCODE(		IR_GEI,			">=",			"GE.I",			2,		0,		OP_GE,			IR_BINOP,	I,		B,		"greater than or equal to")
	
IR_OPCODE(		IR_GTI,			">",			"GT.I",			2,		0,		OP_GT,			IR_BINOP,	I,		B,		"greater than")
	
IR_OPCODE(		IR_LEI,			"<=",			"LE.I",			2,		0,		OP_LE,			IR_BINOP,	I,		B,		"less than or equal to")
	
IR_OPCODE(		IR_LTI,			"<",			"LT.I",			2,		0,		OP_LT,			IR_BINOP,	I,		B,		"less than")
	
IR_OPCODE(		IR_NEI,			"!=",			"NE.I",			2,		0,		OP_NE,			IR_BINOP,	I,		B,		"not equal to")

IR_OPCODE(		IR_EQS,			"==",			"EQ.S",			2,		0,		OP_EQ,			IR_BINOP,	S,		B,		"equal to")
		
IR_OPCODE(		IR_NES,			"!=",			"NE.S",			2,		0,		OP_NE,			IR_BINOP,	S,		B,		"not equal to")

IR_OPCODE(		IR_GTS,			">",			"GT.S",			2,		0,		OP_GT,			IR_BINOP,	S,		B,		"greater than")

// IR_OPCODE(	code,			sym,				name,			nKids,		nSyms,		op,			details,	Types,		RTypes,		description)

IR_OPCODE(		IR_LTS,			"<",			"LT.S",			2,		0,		OP_LT,			IR_BINOP,	S,		B,		"less than")

IR_OPCODE(		IR_LKINIT,		"lkup.init",		"LKUP.INIT",		0,		1,		OP_LKINIT,		IR_STMT,	0,		0,		"initialize the lookup coprocessor")

IR_OPCODE(		IR_LKINITTAB,		"lkup.inittab",		"LKUP.INITTAB",		0,		1,		OP_LKINITTAB,	IR_STMT,		0,		0,		"initialize a lookup table")

IR_OPCODE(		IR_LKKEYS,		"lkup.keys",		"LKUP.KEYS",		0,		1,		OP_LKKEYS,		IR_TERM,	0,		0,		"keys list of an entry of a lookup table")

IR_OPCODE(		IR_LKVALS,		"lkup.vals",		"LKUP.VALS",		0,		1,		OP_LKVALS,		IR_TERM,	0,		0,		"values list of an entry of a lookup table")

IR_OPCODE(		IR_LKADD,		"lkup.add",		"LKUP.ADD",		2,		1,		OP_LKADD,		IR_STMT,	0,		0,		"add an entry in a lookup table")

IR_OPCODE(		IR_LKSEL,		"lkup.sel",		"LKUP.SEL",		1,		1,		OP_LKSEL,		IR_UNOP,	0,		B,		"select an entry from a lookup table")

IR_OPCODE(		IR_LKHIT,		"lkup.hit",		"LKUP.HIT",		1,		1,		OP_LKHIT,		IR_UNOP,	0,		B,		"select an entry from a lookup table and update the timestamp")

IR_OPCODE(		IR_LKGET,		"lkup.get",		"LKUP.GET",		0,		1,		OP_LKGET,		IR_TERM,	I,		0,		"get the value at the specified offset of the selected entry in a lookup table")

IR_OPCODE(		IR_LKUPDS,		"lkup.upd",		"LKUP.UPD",		2,		1,		OP_LKUPD,		IR_STMT,	S,		0,		"update a string value at the specified offset of the selected entry in a lookup table")

IR_OPCODE(		IR_LKUPDI,		"lkup.upd",		"LKUP.UPD",		2,		1,		OP_LKUPD,		IR_STMT,	I,		0,		"update an integer value at the specified offset of the selected entry in a lookup table")

IR_OPCODE(		IR_LKDEL,		"lkup.del",		"LKUP.DEL",		0,		1,		OP_LKDEL,		IR_STMT,	0,		0,		"delete the selected entry from a lookup table")

IR_OPCODE(		IR_REGEXINIT,		"regex.init",		"REGEX.INIT",		0,		1,		OP_REGEXINIT,		IR_STMT,	0,		0,		"initialize the Regulare Expression coprocessor")

IR_OPCODE(		IR_REGEXFND,		"regex.fnd",		"REGEX.FND",		0,		1,		OP_REGEXFND,		IR_STMT,	0,		B,		"find a Regular Expression pattern in the specified buffer")

IR_OPCODE(		IR_REGEXXTR,		"regex.xtr",		"REGEX.XTR",		0,		1,		OP_REGEXXTR,		IR_STMT,	0,		B,		"extract a buffer matching a specified pattern in the specified buffer")

IR_OPCODE(		IR_STRINGMATCHINGINIT,	"stringmatching.init",	"REGEX.INIT",		0,		1,		OP_STRINGMATCHINGINIT,	IR_STMT,	0,		0,		"initialize the String Matching coprocessor")

IR_OPCODE(		IR_STRINGMATCHINGFND,	"stringmatching.fnd",	"STRINGMATCHING.FND",	0,		1,		OP_STRINGMATCHINGFND,	IR_STMT,	0,		B,		"find a String pattern in the specified buffer")


IR_OPCODE(		IR_DATA,		"data",			"DATA",			0,		1,		OP_DATA,		IR_STMT,	0,		0,		"define a data item")

//the next operators are usefull to define a key of a lookup table
//IR_OPCODE(	code,		  sym,			name,		   nKids,	        nSyms,	        op,			details,	Types,	        RTypes,		description)
IR_OPCODE(	IR_LKIKEY,	"lkikey",		"LKIKEY",		2,		1,		OP_LKKEY,		IR_TERM,	0,		I,		"integer key of a lookup table")
IR_OPCODE(	IR_LKDEFKEYI,	"lkkdecl.i",		"LKDEFKEY.I",		1,		1,		OP_LKDEFKEY,		IR_STMT,	I,		0,		"declare an integer key of a lookup table")
IR_OPCODE(	IR_LKSKEY,	"lkskey",		"LKSKEY",		2,		1,		OP_LKKEY,		IR_TERM,	0,		S,		"buffer key of a lookup table")
IR_OPCODE(	IR_LKDEFKEYS,	"lkkdecl.s",		"LKDEFKEY.S",		1,		1,		OP_LKDEFKEY,		IR_STMT,	S,		0,		"declare a string key of a lookup table")

//the next operators are usefull to define a data of a lookup table
//IR_OPCODE(	code,		  sym,			name,		   nKids,	        nSyms,	        op,			details,	Types,	        RTypes,		description)
IR_OPCODE(	IR_LKIDATA,	"lkidata",		"LKIDATA",		2,		1,		OP_LKDATA,		IR_TERM,	0,		I,		"integer data of a lookup table")
IR_OPCODE(	IR_LKDEFDATAI,	"lkddecl.i",		"LKDEFDATA.I",		1,		1,		OP_LKDEFDATA,		IR_STMT,	I,		0,		"declare an integer data of a lookup table")
IR_OPCODE(	IR_LKSDATA,	"lksdata",		"LKSDATA",		2,		1,		OP_LKDATA,		IR_TERM,	0,		I,		"buffer data of a lookup table")
IR_OPCODE(	IR_LKDEFDATAS,	"lkddecl.s",		"LKDEFDATA.S",		1,		1,		OP_LKDEFDATA,		IR_STMT,	S,		0,		"declare a string data of a lookup table")

//the next operator is usefull to define a lookup table
//IR_OPCODE(	code,		  sym,			name,		   nKids,	        nSyms,	        op,			details,	Types,	        RTypes,		description)
IR_OPCODE(	IR_LKTABLE,	"lktable",		"LKTABLE",		0,		1,		OP_LKTABLE,		IR_TERM,	0,		0,		"lookup table")
IR_OPCODE(	IR_LKDEFTABLE,	"lktdecl",		"LKDEFTABLE",		1,		1,		OP_LKDEFTABLE,		IR_STMT,	0,		0,		"declare a lookup table")

IR_OPCODE(	HIR_LAST_OP,	"---",			"---",			9,		9,		OP_LAST,		IR_LAST,	3,		3,		"INVALID_OP")

