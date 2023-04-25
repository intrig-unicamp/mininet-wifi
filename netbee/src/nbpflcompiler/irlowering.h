/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include "compunit.h"
#include "ircodegen.h"
#include "netpflfrontend.h"


#define	LOOKUP_EX_OP_INIT			(uint32)0
#define	LOOKUP_EX_OP_INIT_TABLE	1
#define	LOOKUP_EX_OP_ADD_KEY		2
#define	LOOKUP_EX_OP_ADD_VALUE	3
#define	LOOKUP_EX_OP_SELECT		4
#define	LOOKUP_EX_OP_GET_VALUE	5
#define	LOOKUP_EX_OP_UPD_VALUE	6
#define	LOOKUP_EX_OP_DELETE		7
#define	LOOKUP_EX_OP_RESET		8

#define	LOOKUP_EX_OUT_TABLE_ID		(uint32)0
#define	LOOKUP_EX_OUT_ENTRIES		1
#define	LOOKUP_EX_OUT_KEYS_SIZE		2
#define	LOOKUP_EX_OUT_VALUES_SIZE	3
#define	LOOKUP_EX_OUT_GENERIC		1
#define	LOOKUP_EX_OUT_VALUE_OFFSET	1
#define	LOOKUP_EX_OUT_VALUE			6
#define LOOKUP_EX_OUT_FIELD_SIZE    5
#define LOOKUP_EX_OUT_FIELD_VALUE   6

#define LOOKUP_EX_IN_VALID      7
#define	LOOKUP_EX_IN_VALUE		6

#define	REGEX_MATCH					1
#define	REGEX_MATCH_WITH_OFFSET		2
#define	REGEX_GET_RESULT			3

#define	REGEX_OUT_PATTERN_ID		0
#define	REGEX_OUT_BUF_OFFSET		1
#define	REGEX_OUT_BUF_LENGTH		2

#define	REGEX_IN_MATCHES_FOUND	(uint32)0
#define	REGEX_IN_OFFSET_FOUND		2
#define	REGEX_IN_LENGTH_FOUND		3

enum TranslateTreebehavior{
	NOTHING,
	OFFSET,
	LENGTH
};

enum TranslateIfbehavior{
	TRADITIONAL,
	ADD_COPRO_READ,
	INSIDE_EXTR_STRING
};

class NetPFLFrontEnd;

class IRLowering
{
	struct JCondInfo
	{
		SymbolLabel *TrueLbl;
		SymbolLabel *FalseLbl;

		JCondInfo(SymbolLabel *trueLbl, SymbolLabel *falseLbl)
			:TrueLbl(trueLbl), FalseLbl(falseLbl){}
	};


	struct LoopInfo
	{
		SymbolLabel		*LoopStart;
		SymbolLabel		*LoopExit;

		LoopInfo(SymbolLabel *start, SymbolLabel *exit)
			:LoopStart(start), LoopExit(exit){}
	};


	CompilationUnit			&m_CompUnit;
	MIRCodeGen				&m_CodeGen;
	SymbolProto				*m_Protocol;
	GlobalSymbols			&m_GlobalSymbols;
	list<LoopInfo>			m_LoopStack;
	uint32					m_TreeDepth;
	SymbolLabel				*m_FilterFalse;
	TranslateTreebehavior	translForRegExp;
	TranslateIfbehavior		translForExtrString;
	bool					genExtractionCode; 	//true when the extraction code must be generated
	list<SymbolField*> 		m_Fields; 			//fields for which the extraction code must be generated
	list<uint32> 			m_Positions; 		//position in the info partition for the extracted values
	list<bool>		 		m_MultiFields;		//info about the fact that a certain element of m_Fields is multifield or not
	bool					allFieldsFound; 	//true if the lowered state extracts "allfields"
	NetPFLFrontEnd			*m_FrontEnd;
	SymbolVarInt			*m_VariableToGet;	//variable where to store the size of a match from the regexp copro


	SymbolTemp *AssociateVarTemp(SymbolVarInt *var);

	void TranslateLabel(StmtLabel *stmt);

	void TranslateGen(StmtGen *stmt);

	SymbolLabel *ManageLinkedLabel(SymbolLabel *label, string name);

	void TranslateJump(StmtJump *stmt);

	void TranslateSwitch(HIRStmtSwitch *stmt);

	bool TranslateCase(MIRStmtSwitch *newSwitchSt, StmtCase *caseSt, SymbolLabel *swExit, bool IsDefault);

	void TranslateCases(MIRStmtSwitch *newSwitchSt, CodeList *cases, SymbolLabel *swExit);

	//void TranslateSwitch2(StmtSwitch *stmt); [icerrato] never used

	void TranslateIf(StmtIf *stmt);

	void TranslateLoop(StmtLoop *stmt);

	void TranslateWhileDo(StmtWhile *stmt);

	void TranslateDoWhile(StmtWhile *stmt);

	void TranslateBreak(StmtCtrl *stmt);

	void TranslateContinue(StmtCtrl *stmt);

	MIRNode *TranslateTree(HIRNode *node);

	MIRNode *TranslateIntVarToInt(HIRNode *node);

	MIRNode *TranslateCInt(HIRNode *node);

	MIRNode *TranslateStrVarToInt(HIRNode *node, MIRNode* offset, uint32 size);

	MIRNode *TranslateFieldToInt(HIRNode *node, MIRNode* offset, uint32 size);

	MIRNode *GenMemLoad(MIRNode *offsNode, uint32 size);

	MIRNode *TranslateConstInt(HIRNode *node);

	//Node *TranslateConstStr(Node *node); [icerrato] never used

	MIRNode *TranslateConstBool(HIRNode *node);

	//Node *TranslateTemp(Node *node);[icerrato] not implemented

	void TranslateBoolExpr(HIRNode *expr, JCondInfo &jcInfo);

	void TranslateRelOpInt(MIROpcodes op, HIRNode *relopExpr, JCondInfo &jcInfo);

	void TranslateRelOpStr(MIROpcodes op, HIRNode *relopExpr, JCondInfo &jcInfo);

	void TranslateRelOpStrOperand(HIRNode *operand, MIRNode **loadOperand, MIRNode **loadSize, uint32 *size);

	void TranslateRelOpLookup(HIROpcodes opCode, HIRNode *expr, JCondInfo &jcInfo);

	void TranslateRelOpRegExStrMatch(HIRNode *expr, JCondInfo &jcInfo, string copro);

	bool TranslateLookupSetValue(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *values, bool isKeysList);

	bool TranslateLookupSelect(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *keys);

	//void TranslateLookupInitTable(Node *node); [icerrato] never used

	//void TranslateLookupInit(Node *node); [icerrato]  never used

	void TranslateLookupUpdate(HIRNode *node);

	void TranslateLookupDelete(HIRNode *node);

	void TranslateLookupAdd(HIRNode *node);

	void TranslateFieldDef(HIRNode *node);

	void TranslateVarDeclInt(HIRNode *node);

	void TranslateVarDeclStr(HIRNode *node);

	void TranslateAssignInt(HIRNode *node);

	void TranslateAssignStr(HIRNode *node);

	//void TranslateAssignRef(SymbolVarBufRef *refVar, Node *right);[icerrato] never used

	MIRNode *TranslateArithBinOp(MIROpcodes op, HIRNode *node);

	MIRNode *TranslateArithUnOp(MIROpcodes op, HIRNode *node);

	MIRNode *TranslateDiv(HIRNode *node);
	
	void TranslateStatement(StmtBase *stmt);

	void LowerHIRCode(CodeList *code);

	void EnterLoop(SymbolLabel *start, SymbolLabel *exit);

	void ExitLoop(void);

	void GenTempForVar(HIRNode *node);

	void GenerateInfo(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation);

	void GenerateWarning(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation);

	void GenerateError(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation);

public:

	IRLowering(CompilationUnit &compUnit, MIRCodeGen &codeGen, SymbolLabel *filterFalse, NetPFLFrontEnd *frontEnd)
		:m_CompUnit(compUnit), m_CodeGen(codeGen), m_Protocol(0), m_GlobalSymbols(codeGen.GetGlobalSymbols()),
		m_TreeDepth(0), m_FilterFalse(filterFalse),translForRegExp(NOTHING), translForExtrString(TRADITIONAL), genExtractionCode(true),allFieldsFound(false),m_FrontEnd(frontEnd), m_VariableToGet(0)
	{
		m_CompUnit.MaxStack = 0;
		m_CompUnit.NumLocals = 0;
	}

	void LowerHIRCode(CodeList *code, SymbolProto *proto, bool actionState, list<SymbolField*> fields, list<uint32> positions, list<bool> multifield, string comment = "", bool resetCurrentOffset = false);

	void LowerHIRCode(CodeList *code, string comment);


};

