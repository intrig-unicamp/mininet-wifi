/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include <nbprotodb.h>
#include "defs.h"
#include "intsymboltable.h"
#include "strsymboltable.h"
#include "symtabletree.h"
#include "statements.h"
#include "utils.h"
#include "registers.h"
#include <string>
#include <list>
#include <vector>
#ifdef ENABLE_PFLFRONTEND_PROFILING
#include <nbee_profiler.h>
#endif

#ifdef OPTIMIZE_SIZED_LOOPS
#if (defined(_WIN32) || defined(_WIN64))
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

typedef std::tr1::unordered_map<const SymbolField*, StmtBase*> Field2StmtMap_t;
typedef std::tr1::unordered_map<const SymbolField*, int> Field2IntMap_t;
typedef std::tr1::unordered_map<const StmtBase*, StmtBase*> Stmt2StmtMap_t;
typedef list<const StmtBase*> StmtList_t;
#endif


//forward declarations;
struct SymbolField;
struct SymbolFieldContainer;
struct SymbolProto;
struct SymbolVariable;
struct SymbolStrConst;
struct SymbolIntConst;
struct SymbolLabel;
struct SymbolTemp;
struct SymbolVarBufRef;
struct SymbolLookupTable;
struct SymbolRegEx;
struct SymbolDataItem;

typedef list<SymbolField*> FieldsList_t;	//different fields can share the same name in a protocol description
typedef list<SymbolVarBufRef *> RefBufVarsList_t;
typedef list<Symbol *> SymbolsList_t;
typedef list<SymbolLookupTable *> LookupTablesList_t;
typedef list<SymbolRegEx *> RegExList_t;
typedef list<SymbolProto *> ProtoList_t;
typedef list<SymbolDataItem *> DataItemList_t;
typedef map<SymbolProto*,set<string> > ProtosTables_t;
typedef map<string,set<SymbolProto*> > TablesProtos_t;

//StrSymbolTable specializations
typedef StrSymbolTable<FieldsList_t*> FieldSymbolTable_t;
typedef StrSymbolTable<SymbolProto*> ProtoSymbolTable_t;
typedef StrSymbolTable<SymbolVariable*> RTVarSymbolTable_t;
typedef StrSymbolTable<SymbolStrConst*> StrConstSymbolTable_t;
typedef IntSymbolTable<SymbolLabel*> LabelSymbolTable_t;
typedef IntSymbolTable<SymbolIntConst*> IntConstSymbolTable_t;
typedef IntSymbolTable<SymbolTemp*> TempSymbolTable_t;
typedef StrSymbolTable<SymbolLookupTable *> LookupTablesTable_t;

//typedef DiGraph<SymbolProto*> ProtoGraph_t;


typedef vector<FieldSymbolTable_t*> FldScopeStack_t;

/*!
\brief Enumeration of symbol kinds
*/

enum SymbolKind
{
	SYM_DATA,		//!< dtaa segment item

	SYM_TEMP,		//!< compiler-generated integer temporary
	SYM_LABEL,		//!< compiler-generated label
	SYM_INT_CONST,	//!< integer constant
	SYM_STR_CONST,	//!< string or bytearray constant
	SYM_PROTOCOL,	//!< NetPDL protocol description
	SYM_FIELD,		//!< NetPDL field description
	SYM_RT_VAR,		//!< NetPDL runtime variable

	SYM_RT_REGEX,			//!< NetPDL runtime regular expression operation

	SYM_RT_LOOKUP,			//!< NetPDL runtime lookup table
	SYM_RT_LOOKUP_ITEM,		//!< NetPDL runtime lookup table item (key or value)
	SYM_RT_LOOKUP_ENTRY,	//!< NetPDL runtime lookup table entry
	SYM_RT_LOOKUP_KEYS,		//!< NetPDL runtime lookup table entry keys list
	SYM_RT_LOOKUP_VALUES,	//!< NetPDL runtime lookup table enttry values list
};


/*!
\brief Enumeration of PDL Field Types
*/
// FULVIO qui ci sarebbe da sostituire questo con i codici definiti in NetPDL (PDLFieldType --> nbNetPDLFieldNodeTypes_t)
enum PDLFieldType
{
	PDL_FIELD_FIXED,		//!< Fixed length field: the size of the field is known at compile-time
	PDL_FIELD_VARLEN,		//!< Variable length field: the size of the field is known at runtime
	PDL_FIELD_BITFIELD,		//!< Bitfield: the field is represented by a bit-mask applied to a fixed len field
	PDL_FIELD_PADDING,
	PDL_FIELD_TOKEND,
	PDL_FIELD_TOKWRAP,
	PDL_FIELD_LINE,
	PDL_FIELD_PATTERN,
	PDL_FIELD_EATALL,
	PDL_FIELD_ALLFIELDS,    //!< This value is used only for extraction fields action>
	PDL_FIELD_INVALID_TYPE	//!< This value is used only as a sentinel, in order to avoid invalid field types
};


/*!
\brief Enumeration of PDL Runtime Variable Types
*/
enum PDLVariableType
{
	PDL_RT_VAR_INTEGER,		//!< Integer variable
	PDL_RT_VAR_REFERENCE,	//!< Another name for a (portion of a) buffer variable or field
	PDL_RT_VAR_BUFFER,		//!< Buffer (string) variable. Implies memory allocation
	PDL_RT_VAR_PROTO,		//!< Represents a protocol
};


/*!
\brief This structure is a base class for holding information about  symbols (variables, constants, labels)
*/

struct Symbol
{

	SymbolKind			SymKind;		//!< Kind of symbol (temporary, label, constant)
	bool				IsSupported;	//!< Specifies whether the symbol is supported (i.e. can be parsed or used in expressions) or not
	bool				IsDefined;		//!< Specifies whether the symbol is defined (i.e. you can know the type and the length) or not

	/*!
	\brief Object constructor

	\param kind		//!< Kind of symbol (temporary or label)
	*/

	Symbol(SymbolKind kind)
		:SymKind(kind), IsSupported(true), IsDefined(true){}

	virtual Symbol *copy();
  virtual ~Symbol() {}
};

/*!
\brief Enumeration of PDL variable and  temporary validities
*/

enum VarValidity
{
	VAR_VALID_STATIC,		//!< the temp (or variable) has static (for all packets) validity
	VAR_VALID_THISPKT,		//!< the temp (or variable) is valid only for the current packet
};

enum TempScope
{
	TMP_SCOPE_GLOBAL = false,
	TMP_SCOPE_LOCAL = true
};


/*!
\brief This symbol holds information about an  (Compiler generated) temporary. It inherits from \ref Symbol
*/

struct SymbolTemp: public Symbol
{
	//TODO [PV] this should become a register instance
	uint32		Temp;		//!< The temporary represented by this symbol
	string		Name;
	jit::RegisterInstance LocalReg;

	//!\warning OM compiler generated temporaries should correspond to local variables
	//	VarValidity	Validity;	//!< Temporary validity (one of VarValidity enum)
	//	TempScope	Scope;		//!< Temporary scope (one of TempScope enum)

	/*!
	\brief Object constructor

	\param	temp		integer value representing the temporary
	\param	flags		scope (global or local to the current protocol) and validity (static or relative the current packet) of the variable
	*/
#if 0
	SymbolTemp(uint32 temp, VarValidity validity, TempScope scope, string name = 0)
		:Symbol(SYM_TEMP), Validity(validity), Scope(scope), Name(name){}
#endif

	SymbolTemp(uint32 temp, string name = 0)
		:Symbol(SYM_TEMP), Temp (temp), Name(name), LocalReg(jit::RegisterInstance(0/*space*/, temp/*name*/, 0/*version*/)) {}


	SymbolTemp(jit::RegisterInstance reg)
		: Symbol(SYM_TEMP), Temp(reg.get_model()->get_name()), Name(std::string("")), LocalReg(reg) {}

	string ToString(void)
	{
		return Name;
	}

	string Details(void)
	{
		return string("");
	}

	virtual Symbol *copy();
};



struct SymbolIntConst: public Symbol
{

	uint32	Value;

	/*!
	\brief Object constructor

	\param	Value 32 bit integer value
	*/

	SymbolIntConst(uint32 value)
		:Symbol(SYM_INT_CONST), Value(value){}

	virtual Symbol *copy();
};


struct SymbolStrConst: public Symbol
{
	string  Name;
	uint32	Size;			//!< Size of the string
	uint32	MemOffset;		//!< Memory offset where it will be stored (this value is used during  code generation)


	/*!
	\brief Object constructor

	\param	name		name (== value) of the constant
	\param	size		size in bytes of the string
	\param	memOffset	memory offset where the constant will be stored
	*/

	SymbolStrConst(const string name, uint32 size, uint32 memOffset = 0)
		:Symbol(SYM_STR_CONST), Name(name), Size(size), MemOffset(memOffset){}
	virtual Symbol *copy();
};


/* Da uno scambio di mail con Olivier...
 *
 * LBL_CODE è una label normale, che si trova ovunque nel codice
 * generato. In generale se devi generare una nuova label devi usare
 * quel tipo.
 * 
 * LBL_LINKED è un tipo di label che si trova solo nell'IR di alto
 * livello. Se nel codice HIR si trova una label di quel tipo
 * significa che è un placeholder che verrà riempito da una label vera
 * e propria (di tipo LBL_CODE) in un secondo momento: le label di
 * questo tipo hanno una proprietà Linked che verrà assegnata in fase
 * di lowering del codice. Tutto questo serve per come è fatto il
 * meccanismo di generazione del codice in netpflfrontend. In pratica
 * le uniche LBL_LINKED che vengono generate sono quelle di inizio del
 * codice HIR per ogni protocollo e servono come target delle jump o
 * delle switch generate dai costrutti nextproto nell'encapsulation
 * NetPDL.
 *
 * In fase di parsing delle sezioni format del netpdl, per ogni
 * protocollo viene generata una label di tipo LBL_LINKED, dalla quale
 * segue tutto il codice HIR per il format del protocollo. In fase di
 * parsing delle sezioni encapsulation vengono generate delle jump,
 * dei branch o delle switch che puntano a queste label. Il problema è
 * che quando ci sono filtri composti, noi ripercorriamo più volte
 * l'encapsulation graph, ma le sezioni format vengono parsificate una
 * volta sola.
 *
 * Se le label di inizio protocollo fossero già quelle definitive
 * succederebbe un casino, perché per ogni termine del filtro composto
 * (e.g. ip or tcp) devo generare il codice che mi porta da startproto
 * al protocollo relativo a quel termine (e.g. ip), dopodiché ripeto
 * la cosa per gli altri termini (e.g. tcp). Ogni volta che faccio
 * questo passaggio, le label di inizio protocollo devono essere
 * "nuove", altrimenti tutti i branch di encapsulation nel codice
 * finale punteranno sempre alle stesse porzioni di codice.
 *
 * Da qui il trucco di far diventare quelle label dei placeholder che
 * ogni volta che viene generato il codice che porta a un protocollo,
 * vengono resettati (in GlobalSymbols::UnlinkProtoLabels(void)). In
 * fase di lowering, ogni volta che si trova una label di quel tipo ne
 * viene generata una di tipo LBL_CODE che le viene associata. Quando
 * si trova un riferimento (in un branch, jump, ecc) a una LBL_LINKED
 * si va a recuperare la relativa LBL_CODE.
 */
enum LabelKind
{
	LBL_CODE,
	LBL_LINKED,
	LBL_ID		// [ds] used to identify specific runtime elements (e.g. lookup table or specific operations id)
};

/*!
\brief This symbol holds information about an  label. It inherits from \ref Symbol
*/

struct SymbolLabel: public Symbol
{
	LabelKind	LblKind;
	uint32		Label;	//!< The label represented by this symbol
	string		Name;	//!< label name
	union
	{
		StmtBase	*Code;	//!< Pointer to the labelled statement
		SymbolLabel *Linked;//!< Pointer to another (linked) label
		SymbolProto	*Proto;
	};
	/*!
	\brief Object constructor

	\param	label		integer value representing the label
	\param	code		pointer to the labelled statement
	\param	name		string name of the label
	*/

	SymbolLabel(LabelKind kind, uint32 label, string name = 0)
		:Symbol(SYM_LABEL), LblKind(kind), Label(label), Name(name), Code(0){}
	virtual Symbol *copy();
};




/*!
\brief This symbol holds information about a NetPDL field description. It inherits from \ref Symbol
*/

struct SymbolField: public Symbol
{

	string						Name;
	PDLFieldType				FieldType;
	_nbNetPDLElementFieldBase	*PDLFieldInfo;
	uint32						ID;
	SymbolProto             	*Protocol;
	list<uint32>				HeaderIndex;		// list of instances from which perform the extraction
	SymbolTemp*					FieldCount;	
	FieldsList_t				SymbolDefs;			// [ds] contains different definitions for the same symbol

	SymbolFieldContainer		*DependsOn;			// [ds] the container can be contained in a SymbolFieldContainer

	bool						UsedAsInt;			//	[ds] set to True if the field is used at least once in a buf2int()
	bool						UsedAsArray;		//	[ds] set to True if the field is used at least once in a field[start:len] expression
	bool						UsedAsString;		//	[ds] set to True if the field is used at least once as string
	bool						IntCompatible;		//	[ds] set to True if the field can be converted to int (size<=4)
	bool 						MultiProto;			// true when extracting fields from multiple instances of the same protocol
	list<uint32>				Positions;			// contains the position in the info partition
	/*
	*	The next two variable are needed to manage the multifield
	*/
	list<bool>					IsMultiField;		// true if the n-th instance of src in the extraction list is multifield
	set<uint32>					MultiFields;		// contains the instances of src for which the multifield has been specified
	
	//[mligios]
#ifdef ENABLE_FIELD_OPT
	bool 						Compattable;			//true if this field can be compattated
	bool 						Used;					//true if this field is used somewhere into the code or into the filter
	bool						ToKeep;			//true if, when optimization ends, this field is still useful
#endif	
	
	/*!
	\brief Object constructor

	\param	name		name of the field
	\param	type		type of the field (fixed, varlen, bitmask)
	*/

	SymbolField(const string name, PDLFieldType type, _nbNetPDLElementFieldBase *fieldInfo)
		:Symbol(SYM_FIELD), Name(name), FieldType(type), PDLFieldInfo(fieldInfo),ID(-1), Protocol(0), 
		HeaderIndex(0), FieldCount(0), DependsOn(0), UsedAsInt(false), UsedAsArray(false), UsedAsString(false),
		IntCompatible(true), MultiProto(false), IsMultiField(0)
#ifdef ENABLE_FIELD_OPT
	,Compattable(false),Used(false),ToKeep(true)
#endif
		 {}
	virtual Symbol *copy();
};


struct SymbolFieldContainer: public SymbolField
{
	FieldsList_t				InnerFields;	// [ds] the container has a list of direct child fields

	SymbolFieldContainer(const string name, /*nbNetPDLFieldNodeTypes_t*/ PDLFieldType type, _nbNetPDLElementFieldBase *fieldInfo)
		:SymbolField(name, type, fieldInfo), InnerFields(0) {}
	virtual Symbol *copy();
};


struct SymbolFieldFixed: public SymbolFieldContainer
{
	SymbolTemp		*IndexTemp;
	SymbolTemp		*ValueTemp;
	SymbolTemp		*LenTemp;
	uint32			Size;
	StmtBase		*DefPoint;
	bool			Sensitive;

	SymbolFieldFixed(const string name, uint32 size, _nbNetPDLElementFieldBase *fieldInfo = 0, StmtBase * defPoint = 0, SymbolTemp* indexTemp = 0, SymbolTemp* valueTemp = 0, bool sensitive = false)
		:SymbolFieldContainer(name, PDL_FIELD_FIXED, fieldInfo), IndexTemp(indexTemp), ValueTemp(valueTemp), Size(size), DefPoint(defPoint), Sensitive(sensitive) {}
	virtual Symbol *copy();
};


struct SymbolFieldBitField: public SymbolFieldContainer
{
	uint32				Mask;
	SymbolTemp			*IndexTemp;

	SymbolFieldBitField(const string name, uint32 mask, _nbNetPDLElementFieldBase *fieldInfo)
		:SymbolFieldContainer(name, PDL_FIELD_BITFIELD, fieldInfo), Mask(mask){}
	virtual Symbol *copy();
};


struct SymbolFieldPadding: public SymbolField
{
	uint32			    	Align;
	StmtBase			*DefPoint;

	SymbolFieldPadding(const string name, uint32 align, _nbNetPDLElementFieldBase *fieldInfo, StmtBase * defPoint = 0)
		:SymbolField(name, PDL_FIELD_PADDING, fieldInfo), Align(align), DefPoint(defPoint){}
	virtual Symbol *copy();
};


struct SymbolFieldVarLen: public SymbolFieldContainer
{
	SymbolTemp			*IndexTemp;
	SymbolTemp			*LenTemp;
	StmtBase			*DefPoint;
	Node				*LenExpr;
	bool 				Sensitive;
//[mligios] TEMP ADD HERE


	SymbolFieldVarLen(const string name, _nbNetPDLElementFieldBase *fieldInfo, Node *lenExpr = 0, StmtBase *defPoint = 0, SymbolTemp* indexTemp = 0, SymbolTemp* lenTemp = 0, bool sensitive = false)
		:SymbolFieldContainer(name, PDL_FIELD_VARLEN, fieldInfo), IndexTemp(indexTemp), LenTemp(lenTemp), DefPoint(defPoint),LenExpr(lenExpr), Sensitive(sensitive) {}
	virtual Symbol *copy();
};




struct SymbolFieldTokEnd: public SymbolFieldContainer
{
	SymbolTemp		*IndexTemp;
	SymbolTemp		*LenTemp;
	SymbolTemp		*DiscTemp;
	unsigned char		*EndTok;
	uint32			EndTokSize;
	char			*EndRegEx;
	Node			*EndOff;
	Node			*EndDiscard;
	StmtBase		*DefPoint;
	bool			Sensitive;


	SymbolFieldTokEnd(const string name,unsigned char *endtok, uint32 endtoksize,char *endregex, _nbNetPDLElementFieldBase *fieldInfo, Node *endoff = 0, Node *enddiscard=0, StmtBase *defPoint = 0, SymbolTemp* indexTemp = 0, SymbolTemp* lenTemp = 0,SymbolTemp* discTemp = 0, bool sensitive = false)
		:SymbolFieldContainer(name, PDL_FIELD_TOKEND, fieldInfo), IndexTemp(indexTemp), LenTemp(lenTemp), DiscTemp(discTemp), EndTok(endtok), EndTokSize(endtoksize),
		 EndRegEx(endregex), EndOff(endoff), EndDiscard(enddiscard), DefPoint(defPoint), Sensitive(sensitive) {}
	virtual Symbol *copy();
};



struct SymbolFieldTokWrap: public SymbolFieldContainer
{
	SymbolTemp		*IndexTemp;
	SymbolTemp		*LenTemp;
	SymbolTemp		*DiscTemp;
	unsigned char	*BeginTok;
	uint32			BeginTokSize;
	unsigned char	*EndTok;
	uint32			EndTokSize;
	char			*BeginRegEx;
	char			*EndRegEx;
	Node			*BeginOff;
	Node			*EndOff;
	Node			*EndDiscard;
	StmtBase		*DefPoint;


	SymbolFieldTokWrap(const string name,unsigned char *begintok, uint32 begintoksize,unsigned char *endtok, uint32 endtoksize,char *beginregex,char *endregex, _nbNetPDLElementFieldBase *fieldInfo, Node *beginoff = 0, Node *endoff = 0,Node *enddisc = 0, StmtBase *defPoint = 0, SymbolTemp* indexTemp = 0, SymbolTemp* lenTemp = 0,SymbolTemp* discTemp = 0)
		:SymbolFieldContainer(name, PDL_FIELD_TOKWRAP, fieldInfo),  IndexTemp(indexTemp), LenTemp(lenTemp), DiscTemp(discTemp),
		 BeginTok(begintok), BeginTokSize(begintoksize), EndTok(endtok), EndTokSize(endtoksize),
		 BeginRegEx(beginregex), EndRegEx(endregex),BeginOff(beginoff),
		 EndOff(endoff), EndDiscard(enddisc), DefPoint(defPoint) {}
	virtual Symbol *copy();
};



struct SymbolFieldLine: public SymbolFieldContainer
{
	SymbolTemp		*IndexTemp;
	SymbolTemp		*LenTemp;
	char			*EndTok;
	uint32			EndTokSize;
	StmtBase		*DefPoint;
	bool			Sensitive;

	SymbolFieldLine(const string name, _nbNetPDLElementFieldBase *fieldInfo,StmtBase*defPoint=0,SymbolTemp *indexTemp=0,SymbolTemp *lenTemp=0, bool sensitive = false)
		:SymbolFieldContainer(name, PDL_FIELD_LINE, fieldInfo), IndexTemp(indexTemp), LenTemp(lenTemp), DefPoint(defPoint), Sensitive(sensitive)
	{
    EndTok=(char *)malloc(10*sizeof(char));
    sprintf(EndTok,"\x0D\x0A|\x0A");
	}
	~SymbolFieldLine() { free((void*)EndTok);}
	virtual Symbol *copy();
};

struct SymbolFieldPattern: public SymbolFieldContainer
{
	SymbolTemp		*IndexTemp;
	SymbolTemp		*LenTemp;
	char			*Pattern;
	StmtBase		*DefPoint;
	bool			Sensitive;

	SymbolFieldPattern(const string name,char *pattern, _nbNetPDLElementFieldBase *fieldInfo, StmtBase *defPoint = 0, SymbolTemp* indexTemp = 0, SymbolTemp* lenTemp = 0, bool sensitive = false)
		:SymbolFieldContainer(name, PDL_FIELD_PATTERN, fieldInfo), IndexTemp(indexTemp), LenTemp(lenTemp), Pattern(pattern), DefPoint(defPoint), Sensitive(sensitive){}
	virtual Symbol *copy();
};

struct SymbolFieldEatAll: public SymbolFieldContainer
{
	SymbolTemp		*IndexTemp;
	SymbolTemp		*LenTemp;
	StmtBase		*DefPoint;
	bool			 Sensitive;

	SymbolFieldEatAll(const string name, _nbNetPDLElementFieldBase *fieldInfo,StmtBase*defPoint=0,SymbolTemp *indexTemp=0,SymbolTemp *lenTemp=0, bool sensitive = false)
		:SymbolFieldContainer(name, PDL_FIELD_EATALL, fieldInfo), IndexTemp(indexTemp), LenTemp(lenTemp), DefPoint(defPoint), Sensitive(sensitive)
	{
	}

	virtual Symbol *copy();
};



/*!
\brief This symbol holds information about a NetPDL runtime variable. It inherits from \ref Symbol
*/


struct SymbolVariable: public Symbol
{
	string			Name;			//!< Name of the variable
	PDLVariableType	VarType;			//!< Type of the variable
	VarValidity		Validity;		//!< Validity of the variable (static, thispkt)

	SymbolVariable(const string name, PDLVariableType type, VarValidity validity = VAR_VALID_THISPKT)
		:Symbol(SYM_RT_VAR), Name(name), VarType(type), Validity(validity){}
	virtual Symbol *copy();
};



struct SymbolVarInt: public SymbolVariable
{
	SymbolTemp	*Temp;

	SymbolVarInt(const string name, VarValidity validity = VAR_VALID_THISPKT, SymbolTemp *irTemp = 0)
		:SymbolVariable(name, PDL_RT_VAR_INTEGER, validity), Temp(irTemp){}
	virtual Symbol *copy();
};


struct SymbolVarProto: public SymbolVariable
{

	SymbolVarProto(const string name, VarValidity validity = VAR_VALID_THISPKT)
		:SymbolVariable(name, PDL_RT_VAR_PROTO, validity){}
	virtual Symbol *copy();
};

enum ReferenceType
{
	REF_IS_UNKNOWN,
	REF_IS_PACKET_VAR,
	REF_IS_REF_TO_PACKET_VAR,
	REF_IS_REF_TO_FIELD,
	REF_IS_REF_TO_VAR
};


struct SymbolVarBufRef: public SymbolVariable
{
	ReferenceType	RefType;

	SymbolTemp	*IndexTemp;
	SymbolTemp	*LenTemp;
	SymbolTemp	*ValueTemp;

	SymbolsList_t	ReferredSyms;		//	[ds] list used to save any referred symbols in the NetPDL database
	bool			UsedAsInt;			//	[ds] set to True if the variable is used at least once in a buf2int()
	bool			UsedAsArray;		//	[ds] set to True if the variable is used at least once in a variable[start:len] expression
	bool			UsedAsString;		//	[ds] set to True if the variable is used at least once as string
	bool			IntCompatible;		//	[ds] set to True if the variable can be converted to int (size<=4)

	bool IsInited(void)
	{
		if (RefType == REF_IS_PACKET_VAR)
			return true;
		else if (RefType != REF_IS_UNKNOWN)
			return true;
		else
			return false;
	}

	uint32 GetFixedMaxSize(void)
	{
		uint32 size=0;
		for (SymbolsList_t::iterator i=ReferredSyms.begin(); i!=ReferredSyms.end(); i++)
		{
			Symbol *sym=(*i);
			uint32 tempSize=0;

			switch (sym->SymKind)
			{
			case SYM_FIELD:
				{
					SymbolField *field=(SymbolField *)sym;
					switch (field->FieldType)
					{
					case PDL_FIELD_FIXED:
						tempSize=((SymbolFieldFixed *)field)->Size;
						break;
					case PDL_FIELD_BITFIELD:
						tempSize=1;
						break;
					default:
						continue;
					}
				}break;

			default:
				continue;
			}

			if (size<tempSize)
				size=tempSize;
		}

		return size;
	}

	uint32 GetFixedSize(void)
	{
		uint32 size=0;
		for (SymbolsList_t::iterator i=ReferredSyms.begin(); i!=ReferredSyms.end(); i++)
		{
			Symbol *sym=(*i);
			uint32 tempSize=0;

			switch (sym->SymKind)
			{
			case SYM_FIELD:
				{
					SymbolField *field=(SymbolField *)sym;
					switch (field->FieldType)
					{
					case PDL_FIELD_FIXED:
						tempSize=((SymbolFieldFixed *)field)->Size;
						break;
					case PDL_FIELD_BITFIELD:
						tempSize=1;
						break;
					default:
						continue;
					}
				}break;

			default:
				continue;
			}

			if (size==0)
				size=tempSize;
			else if (size!=tempSize)
				return 0;
		}

		return size;
	}

	SymbolVarBufRef(const string name, VarValidity validity = VAR_VALID_THISPKT, ReferenceType refType = REF_IS_UNKNOWN)
		:SymbolVariable(name, PDL_RT_VAR_REFERENCE, validity), RefType(refType), IndexTemp(0), LenTemp(0), ValueTemp(0),
		UsedAsInt(false), UsedAsArray(false), UsedAsString(false), IntCompatible(true) {}
	virtual Symbol *copy();
};


struct SymbolVarBuffer: public SymbolVariable
{
	uint32		Size;

	SymbolVarBuffer(const string name, uint32 size, VarValidity validity = VAR_VALID_THISPKT)
		:SymbolVariable(name, PDL_RT_VAR_BUFFER, validity), Size(size){}
	virtual Symbol *copy();
};


/*!
\brief This symbol holds information about a NetPDL protocol description. It inherits from \ref Symbol
*/

struct SymbolProto: public Symbol
{
	string						Name;					//!<protocol name
	_nbNetPDLElementProto		*Info;					//!<Pointer to the corresponding NetPDL protocol information
	_nbNetPDLElementExecuteX 	*FirstExecuteBefore;	//!<Pointer to the first element of the execute-before

//[mligios] TEMP HERE
	_nbNetPDLElementExecuteX 	*FirstExecuteVerify;	//! Pointer to the first 'verify' section (within the execute-code);
	_nbNetPDLElementExecuteX 	*FirstExecuteInit; 	//! Pointer to the first 'init' section (within the execute-code); 
	_nbNetPDLElementExecuteX 	*FirstExecuteAfter;     //! Pointer to the first 'after' section (within the execute-code); 
	_nbNetPDLElementBase 		*FirstField;		 //! Pointer to the first field that has to be decoded.
	_nbNetPDLElementBase 		*FirstEncapsulationItem; //! Pointer to the first 'element in the encapsulation' item;
//[mligios] END HERE


	uint32						ID;	//!<Index inside the Protocol List of the \ref GlobalInfo structure
	bool						VerifySectionSupported;
	bool						BeforeSectionSupported;

	double						Level;

	SymbolTemp*					NExFields;
	SymbolTemp*					position;
	uint32						beginPosition;
	SymbolTemp*					ExtractG;	
	FieldsList_t				fields_ToExtract; //[icerrato] fields for which the extraction is required

#ifdef OPTIMIZE_SIZED_LOOPS
	Field2StmtMap_t				Field2SizedLoopMap;
	Field2StmtMap_t				FieldReferredInSizedLoopMap;
	Stmt2StmtMap_t				SizedLoop2SizedLoopMap;
	StmtList_t					SizedLoopStack;
	StmtList_t					SizedLoopToPreserve;
	//Field2IntMap_t				ReferredFields;
#endif
#ifdef STATISTICS
	uint32						DeclaredFields;
	uint32						InnerFields;
	uint32						UnsupportedFields;
	uint32						UnknownRefFields;
	uint32						UnsupportedExpr;
	uint32						VarDeclared;
	uint32						VarUnsupported;
	uint32						VarOccurrences;
	uint32						VarTotOccurrences;
	uint32						EncapDecl;
	uint32						EncapTent;
	uint32						EncapSucc;
	uint32						LookupDecl;
	uint32						LookupOccurrences;
#endif
#ifdef ENABLE_PFLFRONTEND_PROFILING
	int64_t					ParsingTime;
#endif

	bool					NoSession;
	bool 					NoAck;

	/*!
	\brief Object constructor
	_
	\param	info	reference to a nbNetPDLElememntProto structure holding information for a NetPDL <protocol> node
	*/

	SymbolProto(_nbNetPDLElementProto &info, uint32 id)
		:Symbol(SYM_PROTOCOL), Name(info.Name), Info(&info), ID(id), VerifySectionSupported(true), BeforeSectionSupported(true),
          Level(0),NExFields(0),position(0),beginPosition(0), ExtractG(0)
#ifdef OPTIMIZE_SIZED_LOOPS
		, SizedLoopStack(0), SizedLoopToPreserve(0)
#endif
#ifdef STATISTICS
		, DeclaredFields(0), InnerFields(0), UnsupportedFields(0), UnknownRefFields(0), UnsupportedExpr(0),
		VarDeclared(0), VarUnsupported(0), VarOccurrences(0), VarTotOccurrences(0),
		EncapDecl(0), EncapTent(0), EncapSucc(0), LookupDecl(0), LookupOccurrences(0)
#endif
#ifdef ENABLE_PFLFRONTEND_PROFILING
		, ParsingTime(0)
#endif
		, NoSession (false), NoAck(false)
		/*StartLbl(), ExitProtoLbl(0), TrueShtcutLbl(0), FalseShtcutLbl(0), ProtoOffsVar(0),
		FieldSymbols(NO_OWN_ITEMS)*/ {}
	virtual Symbol *copy();

	string &ToString(void)
	{
		return Name;
	}
};



enum DataType
{
	DATA_TYPE_BYTE,
	DATA_TYPE_WORD,
	DATA_TYPE_DOUBLE,
};

struct SymbolDataItem: public Symbol
{
	DataType	Type;
	uint32		Id;
	string 		Name;
	string		Value;

	SymbolDataItem(string name, DataType type, string value, uint32 id)
		:Symbol(SYM_DATA), Type(type), Id(id), Name(name), Value(value){}

};


enum RegExType{
	NETPDL,
	NETPFL
};

struct SymbolRegEx: public Symbol
{
	SymbolStrConst *Pattern;
	int CaseSensitive;
	int MatchToReturn;
	int Id;
	
	virtual RegExType MyType(void)=0;
	
protected:
	SymbolRegEx(SymbolStrConst *pattern, int caseSensitive, int id, int matchToReturn = 0)
		: Symbol(SYM_RT_REGEX), Pattern(pattern), CaseSensitive(caseSensitive), MatchToReturn(matchToReturn), Id(id)
	{}
};

struct SymbolRegExPDL: public SymbolRegEx 
{
	//this structure represents a regular expression defined in the NetPDL database

	Node *Offset;
	SymbolVarInt *MatchedSize;
	
	SymbolRegExPDL(Node *offset, SymbolStrConst *pattern, int caseSensitive, int id, int matchToReturn = 0, SymbolVarInt *matchedSize = NULL)
		: SymbolRegEx(pattern, caseSensitive, id, matchToReturn), Offset(offset), MatchedSize(matchedSize)
	{}
	
	RegExType MyType(void){
		return NETPDL;
	}

};

struct SymbolRegExPFL: public SymbolRegEx 
{
	//this structure represents a regular expression required by the NetPFL expression (i.e. keywords matches and contains are used)

	SymbolTemp *Offset;
	SymbolTemp *Size;	

	SymbolRegExPFL(SymbolStrConst *pattern, int caseSensitive, int id,SymbolTemp *offset, SymbolTemp *size = 0)
		: SymbolRegEx(pattern, caseSensitive, id), Offset(offset), Size(size)
	{}
	
	RegExType MyType(void){
		return NETPFL;
	}

};


enum TableValidity
{
	TABLE_VALIDITY_STATIC,
	TABLE_VALIDITY_DYNAMIC
};

struct SymbolLookupTableItem: public Symbol
{
	SymbolLookupTable	*Table;
	string			 Name;	  //!< Name of the item
	PDLVariableType	 Type;	  //!< Type of the item (allowed values: number, buffer, protocol)
	unsigned int	 Size;	  //!< Size of the item

	SymbolLookupTableItem(SymbolLookupTable *table, string name, PDLVariableType type)
		: Symbol(SYM_RT_LOOKUP_ITEM), Table(table), Name(name), Type(type)
	{
		switch (type)
		{
		case PDL_RT_VAR_INTEGER:
			Size = 4;
			break;
		case PDL_RT_VAR_PROTO:
			nbASSERT(false, "Protocol type in a lookup table has not yet been implemented");
			break;
		case PDL_RT_VAR_BUFFER:
			nbASSERT(false, "Data item of buffer type should also specify the size");
			break;
		default:
			nbASSERT(false, "Only integer, buffer and proto type are allowed");
			break;
		}
	}

	SymbolLookupTableItem(SymbolLookupTable *table, string name, PDLVariableType type, unsigned int size)
		: Symbol(SYM_RT_LOOKUP_ITEM), Table(table), Name(name), Type(type), Size(size)
	{
		switch (type)
		{
		case PDL_RT_VAR_PROTO:
		case PDL_RT_VAR_INTEGER:
			if (Size==0)
				Size = 4;
			break;
		case PDL_RT_VAR_BUFFER:
			break;
		default:
			nbASSERT(false, "Only integer, buffer and proto type are allowed");
			break;
		}
	}
};

enum LookupTableEntryValidity {
	/* 0000 0000	*/
	LOOKUPTABLE_ENTRY_VALIDITY_NONE			= 0x00,
	/* 0000 0001	*/
	LOOKUPTABLE_ENTRY_VALIDITY_KEEPMAXTIME	= 0x01,
	/* 0000 0010	*/
	LOOKUPTABLE_ENTRY_VALIDITY_UPDATEONHIT	= 0x02,
	/* 0000 0100	*/
	LOOKUPTABLE_ENTRY_VALIDITY_REPLACEONHIT	= 0x04,
	/* 0000 1000	*/
	LOOKUPTABLE_ENTRY_VALIDITY_ADDONHIT		= 0x08,
	/* 0001 0000	*/
	LOOKUPTABLE_ENTRY_VALIDITY_KEEPFOREVER	= 0x10,
	/* 1111 1111	*/
	LOOKUPTABLE_ENTRY_VALIDITY_DUMMY		= 0xff
};

#define	HIDDEN_FLAGS_INDEX		0
#define	HIDDEN_TIMESTAMP_INDEX	1
#define	HIDDEN_LIFESPAN_INDEX	2
#define	HIDDEN_KEEPTIME_INDEX	3
#define	HIDDEN_HITTIME_INDEX	4
#define	HIDDEN_NEWHITTIME_INDEX	5
#define	HIDDEN_LAST_INDEX		6

typedef list<Node *> NodesList_t;

struct SymbolLookupTableValuesList: public Symbol {
	NodesList_t	Values;

	SymbolLookupTableValuesList(): Symbol(SYM_RT_LOOKUP_VALUES), Values(0) {}

	void AddValue(Node *value)
	{
		this->Values.push_back(value);
	}
};

struct SymbolLookupTableKeysList: public SymbolLookupTableValuesList {
	NodesList_t Masks;

	SymbolLookupTableKeysList()
		: SymbolLookupTableValuesList(), Masks(0)
	{
		SymKind = SYM_RT_LOOKUP_KEYS;
	}

	void AddKey(Node *value, Node* mask)
	{
		this->AddValue(value);
		this->Masks.push_back(mask);
	}

	void AddKey(Node *value)
	{
		this->AddValue(value);
		this->Masks.push_back(NULL);
	}
};

struct SymbolLookupTableEntry: public Symbol {
	LookupTableEntryValidity	Validity;
	SymbolLookupTable			*Table;
	Node*						HiddenValues[HIDDEN_LAST_INDEX];

	SymbolLookupTableEntry(SymbolLookupTable *table, LookupTableEntryValidity validity)
		: Symbol(SYM_RT_LOOKUP_ENTRY), Validity(validity), Table(table)
	{
		for (int i=0; i<HIDDEN_LAST_INDEX; i++)
			HiddenValues[i]=NULL;
	}

	SymbolLookupTableEntry(SymbolLookupTable *table)
		: Symbol(SYM_RT_LOOKUP_ENTRY), Validity(LOOKUPTABLE_ENTRY_VALIDITY_NONE),  Table(table)
	{
		for (int i=0; i<HIDDEN_LAST_INDEX; i++)
			HiddenValues[i]=NULL;
	}
};

typedef list<SymbolLookupTableItem *> SymbolLookupTableItemsList_t;
typedef SymbolLookupTableItem *TableItemDescriptorPtr_t;


static uint32 SymbolLookupTableCounter=0;
/*!
\brief This symbol holds information about a NetPDL lookup table. It inherits from \ref Symbol
*/
struct SymbolLookupTable: public Symbol
{
	string							Name;				//!< Name of the table
	uint32							Id;					//!< Id of the table (to be used with the coprocessor)
	TableValidity					Validity;			//!< Validity of table entries
	uint32							MaxExactEntriesNr;	//!< Max numbers of exact entries
	uint32							MaxMaskedEntriesNr;	//!< Max numbers of masked entries
	SymbolLookupTableItemsList_t	KeysList;			//!< List of keys
	uint32							KeysListSize;		//!< Size in bytes of the keys list
	SymbolLookupTableItemsList_t	ValuesList;			//!< List of values
	uint32							ValuesListSize;		//!< Size in bytes of the values list
	TableItemDescriptorPtr_t		HiddenValuesList[HIDDEN_LAST_INDEX];	//!< List of hidden values (if the table is dynamic, you have timestamp, lifespan, ...)
	SymbolLabel						*Label;

	SymbolLookupTable(string name, TableValidity validity, unsigned int maxExactEntriesNr, unsigned int maxMaskedEntriesNr)
		: Symbol(SYM_RT_LOOKUP), Name(name), Validity(validity), MaxExactEntriesNr(maxExactEntriesNr), MaxMaskedEntriesNr(maxMaskedEntriesNr),
		KeysList(0), KeysListSize(0), ValuesList(0), ValuesListSize(0), Label(0)
	{
		int i$ = Name.find("$");
		if (i$>=0)
			Label = new SymbolLabel(LBL_ID, 0, string("lookup_").append(Name.substr(i$+1)));
		else
			Label = new SymbolLabel(LBL_ID, 0, string("lookup_").append(Name));

		// [ds] use temporary the generic name 'lookup_ex'
		Label = new SymbolLabel(LBL_ID, 0, string("lookup_ex"));

		Id = SymbolLookupTableCounter++;

		if (validity==TABLE_VALIDITY_DYNAMIC)
		{
			// add hidden values, you need flags, timestamp, lifespan, keeptime, hittime, newhittime
			HiddenValuesList[HIDDEN_FLAGS_INDEX] = new SymbolLookupTableItem(this, "Flags", PDL_RT_VAR_INTEGER);
			HiddenValuesList[HIDDEN_TIMESTAMP_INDEX] = new SymbolLookupTableItem(this, "Timestamp", PDL_RT_VAR_INTEGER, 8);
			HiddenValuesList[HIDDEN_LIFESPAN_INDEX] = new SymbolLookupTableItem(this, "Lifespan", PDL_RT_VAR_INTEGER);
			HiddenValuesList[HIDDEN_KEEPTIME_INDEX] = new SymbolLookupTableItem(this, "KeepTime", PDL_RT_VAR_INTEGER);
			HiddenValuesList[HIDDEN_HITTIME_INDEX] = new SymbolLookupTableItem(this, "HitTime", PDL_RT_VAR_INTEGER);
			HiddenValuesList[HIDDEN_NEWHITTIME_INDEX] = new SymbolLookupTableItem(this, "NewHitTime", PDL_RT_VAR_INTEGER);
		}
	}

	void AddKey(SymbolLookupTableItem* key)
	{
		KeysList.push_back(key);
		KeysListSize+=key->Size;
	}

	void AddValue(SymbolLookupTableItem *value)
	{
		ValuesList.push_back(value);
		ValuesListSize+=value->Size;
	}

	SymbolLookupTableItem *GetValue(string name)
	{
		for (SymbolLookupTableItemsList_t::iterator i=ValuesList.begin(); i!=ValuesList.end(); i++)
		{
			if ((*i)->Name.compare(name)==0)
				return *i;
		}

		return NULL;
	}

	uint32 GetValueOffset(string name)
	{
		uint32 offset=0;
		for (SymbolLookupTableItemsList_t::iterator i=ValuesList.begin(); i!=ValuesList.end(); i++)
		{
			if ((*i)->Name.compare(name)==0)
				break;
			offset+=(*i)->Size;
		}


		return offset/sizeof(uint32);
	}

	uint32 GetHiddenValueOffset(uint32 index)
	{
		uint32 offset=ValuesListSize;
		for (uint32 i=0; i<index; i++)
			offset+=HiddenValuesList[i]->Size;

		return offset/sizeof(uint32);
	}

	~SymbolLookupTable()
	{
		for (SymbolLookupTableItemsList_t::iterator i=KeysList.begin(); i!=KeysList.end(); i++)
			delete (*i);
		for (SymbolLookupTableItemsList_t::iterator i=ValuesList.begin(); i!=ValuesList.end(); i++)
			delete (*i);
		for (int i=0; i<HIDDEN_LAST_INDEX; i++)
			delete HiddenValuesList[i];
	}
};
