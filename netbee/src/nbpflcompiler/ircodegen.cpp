/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "defs.h"
#include "ircodegen.h"
#include "errors.h"
#include "tree.h"
#include "statements.h"
#include <string>

using namespace std;

uint32 stmts = 0;

SymbolLabel *IRCodeGen::NewLabel(LabelKind kind, string name)
{
	uint32 label = m_GlobalSymbols.GetNewLabel(1);
	string lblName;
	if (name.size() == 0)
			lblName = string("l") + int2str(label, 10);
		else
			lblName = name + string("_l") + int2str(label, 10);
	SymbolLabel *lblSym = new SymbolLabel(kind, label, lblName);
	if (lblSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_GlobalSymbols.StoreLabelSym(label, lblSym);
	return lblSym;
}


SymbolTemp *IRCodeGen::NewTemp(string name, uint32 &tempCount)
{
	uint32 temp = m_GlobalSymbols.GetNewTemp(1);
	tempCount++;
	string tmpName;
	if (name.size() == 0)
			tmpName = string("t") + int2str(temp, 10);
		else
			tmpName = name + string("_t") + int2str(temp, 10);
	SymbolTemp *tempSym = new SymbolTemp(temp, tmpName);
	if (tempSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_GlobalSymbols.StoreTempSym(temp, tempSym);
	return tempSym;
}


SymbolVarInt *IRCodeGen::NewIntVar(string name, VarValidity validity)
{
	if (name[0] != '$')
		name = string("$") + name;

	SymbolVarInt *varSym = new SymbolVarInt(name, validity);
	if (varSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_GlobalSymbols.StoreVarSym(name, varSym);
	return varSym;
}

Symbol *IRCodeGen::ConstIntSymbol(const uint32 value)
{
	SymbolIntConst *constant = m_GlobalSymbols.LookUpConst(value);
	if (constant == NULL)
	{
		constant = new SymbolIntConst(value);
		if (constant == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		m_GlobalSymbols.StoreConstSym(value, constant);
	}

	return constant;
}


Symbol *IRCodeGen::ConstStrSymbol(const string value)
{
	SymbolStrConst *constant = m_GlobalSymbols.LookUpConst(value);
	if (constant == NULL)
	{
		uint32 len = value.length();
		constant = new SymbolStrConst(value, len, m_GlobalSymbols.IncConstOffs(len));
		if (constant == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		m_GlobalSymbols.StoreConstSym(value, constant);
	}

	return constant;
}
void IRCodeGen::AppendTree(Node *node)
{
	nbASSERT(m_CodeList != NULL, "Cannot perform Append operation when Code List is NULL!!!");
	nbASSERT(!m_CodeList->Empty(), "Code list should contain at least one statement.");
	
	StmtBase *lastStmt = m_CodeList->Back();
	nbASSERT(lastStmt->Forest == NULL, "the current statement already has a linked forest");

	lastStmt->Forest = node;
	return;
}


StmtBase *IRCodeGen::Statement(StmtBase *stmt)
{
	nbASSERT(m_CodeList != NULL, "Cannot generate a statement when Code List is NULL!!!");
	m_CodeList->PushBack(stmt);
	stmts++;
	return stmt;
}


StmtComment *IRCodeGen::CommentStatement(string comment)
{
	StmtComment *stmtComment = new StmtComment(comment);
	CHECK_MEM_ALLOC(stmtComment);
	Statement(stmtComment);
	return stmtComment;
}

StmtBlock *IRCodeGen::BlockStatement(string comment)
{
	StmtBlock *stmtBlck = new StmtBlock(comment);
	CHECK_MEM_ALLOC(stmtBlck);
	Statement(stmtBlck);
	return stmtBlck;

}

StmtCase *IRCodeGen::CaseStatement(StmtSwitch *switchStmt, Node *constVal)
{
	nbASSERT(switchStmt != NULL, "switchStmt cannot be NULL");
	nbASSERT(constVal != NULL, "constVal cannot be NULL");
	StmtCase *stmtCase = new StmtCase(constVal);
	CHECK_MEM_ALLOC(stmtCase);
	switchStmt->Cases->PushBack(stmtCase);
	return stmtCase;
}

StmtCase *IRCodeGen::DefaultStatement(StmtSwitch *switchStmt)
{
	nbASSERT(switchStmt != NULL, "switchStmt cannot be NULL");
	StmtCase *stmtCase = new StmtCase();
	CHECK_MEM_ALLOC(stmtCase);
	switchStmt->Default = stmtCase;
	return stmtCase;
}

StmtBase *IRCodeGen::GenStatement(Node *node)
{
	StmtBase *stmt = new StmtGen(node);
	CHECK_MEM_ALLOC(stmt);
	Statement(stmt);
	return stmt;
}

StmtJump *IRCodeGen::JumpStatement(SymbolLabel *lblSym)
{
	nbASSERT(lblSym != NULL, "label symbol cannot be null for an unconditional jump");
	
	StmtJump *stmt = new StmtJump(lblSym);
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

/********************************************HIR*******************************************************/

HIRNode *HIRCodeGen::TerOp(HIROpcodes opKind, HIRNode *op1, HIRNode *op2, HIRNode *op3, Symbol *lbl)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	nbASSERT(op3 != NULL, "child node cannot be NULL");

	HIRNode *node = new HIRNode(opKind, lbl, op1, op2, op3);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

HIRNode *HIRCodeGen::BinOp(HIROpcodes opKind, HIRNode *op1, HIRNode *op2, Symbol *lbl)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	HIRNode *node = new HIRNode(opKind, lbl, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


HIRNode *HIRCodeGen::UnOp(HIROpcodes opKind, HIRNode *operand, Symbol *sym, uint32 value)
{
	nbASSERT(operand != NULL, "child node cannot be NULL");
	HIRNode *node = new HIRNode(opKind, operand, sym, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	
	return node;
}


HIRNode *HIRCodeGen::UnOp(HIROpcodes opKind, Symbol *sym, uint32 value , Symbol *symEx)
{
	nbASSERT(sym != NULL, "symbol cannot be NULL");
	HIRNode *node = new HIRNode(opKind, sym, value, symEx);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	
	return node;
}

HIRNode *HIRCodeGen::TermNode(HIROpcodes nodeKind, Symbol *sym, HIRNode *op1, HIRNode *op2)
{
	nbASSERT(sym != NULL, "Symbol cannot be NULL");
	HIRNode *node = new HIRNode(nodeKind, sym, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

HIRNode *HIRCodeGen::TermNode(HIROpcodes nodeKind, Symbol *sym, uint32 value)
{
	nbASSERT(sym != NULL, "Symbol cannot be NULL");
	HIRNode *node = new HIRNode(nodeKind, sym, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

HIRNode *HIRCodeGen::TermNode(HIROpcodes nodeKind, uint32 value)
{
	HIRNode *node = new HIRNode(nodeKind, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


HIRNode *HIRCodeGen::TermNode(HIROpcodes nodeKind)
{
	HIRNode *node = new HIRNode(nodeKind);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

HIRNode *HIRCodeGen::ProtoBytesRef(SymbolProto *protoSym, uint32 offs, uint32 len)
{
	nbASSERT(m_GlobalSymbols.GetProtoOffsVar(protoSym) != NULL, "The protocol symbol has not an associated $proto_offs variable");
	nbASSERT(m_GlobalSymbols.PacketBuffSym != NULL, "The $packet variable has not been set");

	HIRNode *protoOffs = TermNode(IR_IVAR, m_GlobalSymbols.GetProtoOffsVar(protoSym));
	SymbolVarBufRef *packetVar = m_GlobalSymbols.PacketBuffSym;
	// startAt = $proto_offs + offs
	HIRNode *startAt = BinOp(IR_ADDI, protoOffs, TermNode(IR_ICONST, ConstIntSymbol(offs)));
	HIRNode *size = TermNode(IR_ICONST, ConstIntSymbol(len));
	// return the resulting ProtoBytesRef node: $packet[$proto_offs + offs:len]
	return TermNode(IR_IVAR, packetVar, startAt, size);
	
}

HIRNode *HIRCodeGen::ProtoHIndex(SymbolProto *protoSym, uint32 index)
{
	nbASSERT(m_GlobalSymbols.GetProtoOffsVar(protoSym) != NULL, "The protocol symbol has not an associated $proto_offs variable");
	nbASSERT(m_GlobalSymbols.PacketBuffSym != NULL, "The $packet variable has not been set");

	TermNode(IR_IVAR, m_GlobalSymbols.GetProtoOffsVar(protoSym));
	//SymbolVarBufRef *packetVar = m_GlobalSymbols.PacketBuffSym;


	return TermNode(IR_IVAR);//, packetVar);//, startAt, size);
}

StmtLabel *HIRCodeGen::LabelStatement(SymbolLabel *lblSym)
{	
	StmtLabel *lblStmt =  new StmtLabel();
	CHECK_MEM_ALLOC(lblStmt);
	Statement(lblStmt);
	AppendTree(TermNode(IR_LABEL, lblSym));
	if (lblSym->LblKind == LBL_CODE)
		lblSym->Code = lblStmt;
	return lblStmt;
}

HIRStmtSwitch *HIRCodeGen::SwitchStatement(HIRNode *expr)
{
	HIRStmtSwitch *stmt = new HIRStmtSwitch(expr);
	CHECK_MEM_ALLOC(stmt);
	Statement(stmt);
	return stmt;
}

StmtCase *HIRCodeGen::CaseStatement(HIRStmtSwitch *switchStmt, HIRNode *constVal)
{	
	return IRCodeGen::CaseStatement(switchStmt, constVal);
}

StmtCase *HIRCodeGen::DefaultStatement(HIRStmtSwitch *switchStmt)
{
	return IRCodeGen::DefaultStatement(switchStmt);
}

StmtIf *HIRCodeGen::IfStatement(HIRNode *condition)
{
	nbASSERT(condition != NULL, "condition cannot be NULL");
	StmtIf *stmtIf = new StmtIf(condition);
	CHECK_MEM_ALLOC(stmtIf);
	Statement(stmtIf);
	return stmtIf;
}

StmtWhile *HIRCodeGen::WhileDoStatement(HIRNode *condition)
{
	nbASSERT(condition != NULL, "condition cannot be NULL");
	StmtWhile *stmtWhile = new StmtWhile(STMT_WHILE, condition);
	CHECK_MEM_ALLOC(stmtWhile);
	Statement(stmtWhile);
	return stmtWhile;
}

StmtWhile *HIRCodeGen::DoWhileStatement(HIRNode *condition)
{
	//[OM]: The condition is undefined when generating a Do-While statement, so this assert is wrong!
	//nbASSERT(condition != NULL, "condition cannot be NULL");
	StmtWhile *stmtWhile = new StmtWhile(STMT_DO_WHILE, condition);
	CHECK_MEM_ALLOC(stmtWhile);
	Statement(stmtWhile);
	return stmtWhile;
}


StmtLoop *HIRCodeGen::LoopStatement(HIRNode *indexVar, HIRNode *initVal, HIRNode *termCond, StmtGen *incStmt)
{
	nbASSERT(indexVar != NULL, "indexVar cannot be NULL");
	nbASSERT(initVal != NULL, "initVal cannot be NULL");
	nbASSERT(termCond != NULL, "termCond cannot be NULL");
	nbASSERT(incStmt != NULL, "incStmt");
	StmtLoop *stmtLoop = new StmtLoop(indexVar, initVal, termCond, incStmt);
	CHECK_MEM_ALLOC(stmtLoop);
	Statement(stmtLoop);
	return stmtLoop;
}

StmtCtrl *HIRCodeGen::BreakStatement(void)
{
	StmtCtrl *stmtCtrl = new StmtCtrl(STMT_BREAK);
	CHECK_MEM_ALLOC(stmtCtrl);
	Statement(stmtCtrl);
	return stmtCtrl;
}

StmtCtrl *HIRCodeGen::ContinueStatement(void)
{
	StmtCtrl *stmtCtrl = new StmtCtrl(STMT_CONTINUE);
	CHECK_MEM_ALLOC(stmtCtrl);
	Statement(stmtCtrl);
	return stmtCtrl;
}


StmtBase *HIRCodeGen::GenStatement(HIRNode *node)
{
	return IRCodeGen::GenStatement(node);
}

//[icerrato]
//creates a SymbollookupTableItem
SymbolLookupTableItem *HIRCodeGen::NewLookupTableItem(SymbolLookupTable *table, string name, PDLVariableType type, unsigned int size)
{
	SymbolLookupTableItem *lookupTableItemSym = new SymbolLookupTableItem(table, name, type, size);
	if (lookupTableItemSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return lookupTableItemSym;
}

//[icerrato]
//creates a SymbolLookupTable
SymbolLookupTable *HIRCodeGen::NewLookupTable(string name, TableValidity validity, unsigned int maxExactEntriesNr, unsigned int maxMaskedEntriesNr){
	if (name[0] != '$')
		name = string("$") + name;	

	SymbolLookupTable *lookupTableSym = new SymbolLookupTable(name,validity,maxExactEntriesNr,maxMaskedEntriesNr);
	if (lookupTableSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	//m_GlobalSymbols.StoreLookupTable(name,lookupTableSym);
	return lookupTableSym;
}


/********************************************MIR*******************************************************/

MIRNode *MIRCodeGen::TerOp(MIROpcodes opKind, MIRNode *op1, MIRNode *op2, MIRNode *op3, Symbol *lbl)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	nbASSERT(op3 != NULL, "child node cannot be NULL");

	MIRNode *node = new MIRNode(opKind, lbl, op1, op2, op3);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

MIRNode *MIRCodeGen::BinOp(MIROpcodes opKind, MIRNode *op1, MIRNode *op2, Symbol *lbl)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	MIRNode *node = new MIRNode(opKind, lbl, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


MIRNode *MIRCodeGen::UnOp(MIROpcodes opKind, MIRNode *operand, Symbol *sym, uint32 value )
{
	nbASSERT(operand != NULL, "child node cannot be NULL");
	MIRNode *node = new MIRNode(opKind, operand, sym, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		
	return node;
}


MIRNode *MIRCodeGen::UnOp(MIROpcodes opKind, Symbol *sym, uint32 value , Symbol *symEx)
{
	nbASSERT(sym != NULL, "symbol cannot be NULL");
	MIRNode *node = new MIRNode(opKind, sym, value, symEx);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	
	return node;
}

MIRNode *MIRCodeGen::TermNode(MIROpcodes nodeKind, Symbol *sym, MIRNode *op1, MIRNode *op2)
{
	nbASSERT(sym != NULL, "Symbol cannot be NULL");
	MIRNode *node = new MIRNode(nodeKind, sym, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

MIRNode *MIRCodeGen::TermNode(MIROpcodes nodeKind, Symbol *sym, uint32 value)
{
	nbASSERT(sym != NULL, "Symbol cannot be NULL");
	MIRNode *node = new MIRNode(nodeKind, sym, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

MIRNode *MIRCodeGen::TermNode(MIROpcodes nodeKind, uint32 value)
{
	MIRNode *node = new MIRNode(nodeKind, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


MIRNode *MIRCodeGen::TermNode(MIROpcodes nodeKind)
{
	MIRNode *node = new MIRNode(nodeKind);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

StmtJump *MIRCodeGen::JumpStatement(MIROpcodes opcode, SymbolLabel *lblSym)
{
	StmtJump *stmt = IRCodeGen::JumpStatement(lblSym);
	stmt->Opcode = opcode;
	return stmt;
}

StmtJump *MIRCodeGen::JCondStatement(MIROpcodes cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, MIRNode *op1, MIRNode *op2)
{
	nbASSERT(trueLbl != NULL, "trueLbl cannot be null for a conditional jump");
	nbASSERT(falseLbl != NULL, "falseLbl cannot be null for a conditional jump");

	StmtJump *stmt = new StmtJump(trueLbl, falseLbl, BinOp(cond, op1, op2));
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

StmtJump *MIRCodeGen::JFieldStatement(MIROpcodes cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, MIRNode *op1, MIRNode *op2, MIRNode *size)
{
	nbASSERT(trueLbl != NULL, "trueLbl cannot be null for a conditional jump");
	nbASSERT(falseLbl != NULL, "falseLbl cannot be null for a conditional jump");

	StmtJumpField *stmt = new StmtJumpField(trueLbl, falseLbl, TerOp(cond, op1, op2, size));
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

StmtJump *MIRCodeGen::JCondStatement(SymbolLabel *trueLbl, SymbolLabel *falseLbl, MIRNode *expr)
{
	StmtJump *stmt = new StmtJump(trueLbl, falseLbl, expr);
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

MIRStmtSwitch *MIRCodeGen::SwitchStatement(MIRNode *expr)
{
	MIRStmtSwitch *stmt = new MIRStmtSwitch(expr);
	CHECK_MEM_ALLOC(stmt);
	Statement(stmt);
	return stmt;
}

StmtCase *MIRCodeGen::CaseStatement(MIRStmtSwitch *switchStmt, MIRNode *constVal)
{	
	return IRCodeGen::CaseStatement(switchStmt, constVal);
}

StmtCase *MIRCodeGen::DefaultStatement(MIRStmtSwitch *switchStmt)
{
	return IRCodeGen::DefaultStatement(switchStmt);
}

StmtBase *MIRCodeGen::GenStatement(MIRNode *node)
{
	return IRCodeGen::GenStatement(node);
}

StmtLabel *MIRCodeGen::LabelStatement(SymbolLabel *lblSym)
{	
	StmtLabel *lblStmt =  new StmtLabel();
	CHECK_MEM_ALLOC(lblStmt);
	Statement(lblStmt);
	AppendTree(TermNode(NOP, lblSym)); //[icerrato] I use a NOP but it could be wrong... The IR_VAR cannot be used..
	if (lblSym->LblKind == LBL_CODE)
		lblSym->Code = lblStmt;
	return lblStmt;
}
