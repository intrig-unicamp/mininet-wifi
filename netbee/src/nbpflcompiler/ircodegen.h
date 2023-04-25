/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "symbols.h"
#include "globalsymbols.h"
#include <utility>
#include "statements.h"
#include "tree.h"
using namespace std;

/*!
	\brief 
*/


class IRCodeGen
{	

protected:
	//constructor

	IRCodeGen(GlobalSymbols &globalSymbols, CodeList *codeList = 0)
		:m_GlobalSymbols(globalSymbols), m_CodeList(codeList){}

	

	~IRCodeGen(void)
	{
	}

	SymbolTemp *CurrOffsTemp(void)
	{
		nbASSERT(m_GlobalSymbols.CurrentOffsSym != NULL, "CurrentOffsSym cannot be NULL");
		return m_GlobalSymbols.CurrentOffsSym->Temp;
	}

	SymbolVarInt *CurrOffsVar(void)
	{
		return m_GlobalSymbols.CurrentOffsSym;
	}

	StmtBase *Statement(StmtBase *stmt);

	void AppendTree(Node *node);

	StmtCase *CaseStatement(StmtSwitch *switchStmt, Node *constVal);
	StmtCase *DefaultStatement(StmtSwitch *switchStmt);
	StmtBase *GenStatement(Node *node);

public:
	GlobalSymbols		&m_GlobalSymbols;
	CodeList		*m_CodeList;

	StmtComment *CommentStatement(string comment);
	StmtJump *JumpStatement(SymbolLabel *lblSym);
	StmtBlock *BlockStatement(string comment = "");
	SymbolLabel *NewLabel(LabelKind kind, string name = "");
	SymbolTemp *NewTemp(string name, uint32 &tempCount); //maybe only for MIRCodeGen....
	GlobalSymbols &GetGlobalSymbols(void)
	{
		return m_GlobalSymbols;
	}
	SymbolVarInt *NewIntVar(string name, VarValidity validity = VAR_VALID_THISPKT);
	Symbol *ConstIntSymbol(uint32 value);
	Symbol *ConstStrSymbol(const string value);
	
};

extern uint32 stmts;

/********************************************HIR*******************************************************/

class HIRCodeGen: public IRCodeGen
{	

public:
	//constructor

	HIRCodeGen(GlobalSymbols &globalSymbols, CodeList *codeList = 0)
		:IRCodeGen(globalSymbols,codeList) {}

	

	~HIRCodeGen(void) {}

	SymbolTemp *CurrOffsTemp(void)
	{
		return IRCodeGen::CurrOffsTemp();
	}

	SymbolVarInt *CurrOffsVar(void)
	{
		return IRCodeGen::CurrOffsVar();
	}

	void AppendTree(HIRNode *node)
	{
		IRCodeGen::AppendTree(node);
	}
	
	HIRNode *TerOp(HIROpcodes opKind, HIRNode *op1, HIRNode *op2, HIRNode *op3, Symbol *sym = 0);
	HIRNode *BinOp(HIROpcodes opKind, HIRNode *op1, HIRNode *op2, Symbol *sym = 0);
	HIRNode *UnOp(HIROpcodes opKind, HIRNode *op, Symbol *sym = 0, uint32 value = 0);
	HIRNode *UnOp(HIROpcodes opKind, Symbol *sym, uint32 value = 0, Symbol *symEx = 0);
	HIRNode *TermNode(HIROpcodes nodeKind, Symbol *sym, HIRNode *op1 = 0, HIRNode *op2 = 0);
	HIRNode *TermNode(HIROpcodes nodeKind, Symbol *sym, uint32 value);
	HIRNode *TermNode(HIROpcodes nodeKind, uint32 val);	
	HIRNode *TermNode(HIROpcodes nodeKind);	
	HIRNode *ProtoBytesRef(SymbolProto *protoSym, uint32 offs, uint32 len);
	HIRNode *ProtoHIndex(SymbolProto *protoSym, uint32 index);
	
	StmtLabel *LabelStatement(SymbolLabel *lblSym);

	StmtCase *CaseStatement(HIRStmtSwitch *switchStmt, HIRNode *constVal);
	StmtCase *DefaultStatement(HIRStmtSwitch *switchStmt);
	HIRStmtSwitch *SwitchStatement(HIRNode *expr=0);
	StmtIf *IfStatement(HIRNode *condition); 
	StmtWhile *WhileDoStatement(HIRNode *condition);
	StmtWhile *DoWhileStatement(HIRNode *condition);
	StmtLoop *LoopStatement(HIRNode *indexVar, HIRNode *initVal, HIRNode *termCond, StmtGen *incStmt);
	StmtCtrl *BreakStatement(void);
	StmtCtrl *ContinueStatement(void);
	StmtBase *GenStatement(HIRNode *node);

	//[icerrato]
	SymbolLookupTable *NewLookupTable(string name, TableValidity validity, unsigned int maxExactEntriesNr, unsigned int maxMaskedEntriesNr);
	SymbolLookupTableItem *NewLookupTableItem(SymbolLookupTable *table, string name, PDLVariableType type, unsigned int size);

	
};

/********************************************HIR*******************************************************/

class MIRCodeGen: public IRCodeGen
{	

public:
	//constructor

	MIRCodeGen(GlobalSymbols &globalSymbols, CodeList *codeList = 0)
		:IRCodeGen(globalSymbols,codeList) {}

	

	~MIRCodeGen(void) {}

	SymbolTemp *CurrOffsTemp(void)
	{
		return IRCodeGen::CurrOffsTemp();
	}

	SymbolVarInt *CurrOffsVar(void)
	{
		return IRCodeGen::CurrOffsVar();
	}

	void AppendTree(MIRNode *node)
	{
		IRCodeGen::AppendTree(node);
	}
	
	MIRNode *TerOp(MIROpcodes opKind, MIRNode *op1, MIRNode *op2, MIRNode *op3, Symbol *sym = 0);
	MIRNode *BinOp(MIROpcodes opKind, MIRNode *op1, MIRNode *op2, Symbol *sym = 0);
	MIRNode *UnOp(MIROpcodes opKind, MIRNode *op, Symbol *sym = 0, uint32 value = 0);
	MIRNode *UnOp(MIROpcodes opKind, Symbol *sym, uint32 value = 0, Symbol *symEx = 0);
	MIRNode *TermNode(MIROpcodes nodeKind, Symbol *sym, MIRNode *op1 = 0, MIRNode *op2 = 0);
	MIRNode *TermNode(MIROpcodes nodeKind, Symbol *sym, uint32 value);
	MIRNode *TermNode(MIROpcodes nodeKind, uint32 val);	
	MIRNode *TermNode(MIROpcodes nodeKind);	
	
	StmtLabel *LabelStatement(SymbolLabel *lblSym); 
	
	StmtJump *JumpStatement(MIROpcodes opcode, SymbolLabel *lblSym); 
	StmtJump *JCondStatement(MIROpcodes cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, MIRNode *op1, MIRNode *op2); 
	StmtJump *JFieldStatement(MIROpcodes cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, MIRNode *op1, MIRNode *op2, MIRNode *size);
	StmtJump *JCondStatement(SymbolLabel *trueLbl, SymbolLabel *falseLbl, MIRNode *expr);
	
	StmtCase *CaseStatement(MIRStmtSwitch *switchStmt, MIRNode *constVal);
	StmtCase *DefaultStatement(MIRStmtSwitch *switchStmt);
	MIRStmtSwitch *SwitchStatement(MIRNode *expr=0);

	StmtBase *GenStatement(MIRNode *node);
	
};
