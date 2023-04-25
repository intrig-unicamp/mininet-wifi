/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "statements.h"
#include "mironode.h"
#include <iostream>

using namespace std;

//forward declaration for friend clauses
class MIRONode;

class CodeWriter
{
//private:
	protected:
	ostream	&m_Stream;
	string	endLine;
	bool	GenNetIL;
	uint32_t * _num_Statements;

	void DumpSymbol(Symbol *sym);
	void DumpOpcode(uint16 opcode);
	void DumpTabLevel(uint32 level);
	void DumpComment(StmtComment *stmt, uint32 level);
	void DumpComment(CommentMIRONode *stmt, uint32 level);	//******** OK
	void DumpBitField(SymbolFieldBitField *bitField, uint32 size);
	void DumpCase(StmtCase *stmt, bool isDefault, uint32 level);
	void DumpCase(CaseMIRONode *stmt, bool isDefault, uint32 level);	//******** OK
	void DumpSwitch(StmtBase *stmt, uint32 level = 0);
	void DumpSwitch(SwitchMIRONode *stmt, uint32 level = 0);	//******** OK
	void DumpJump(StmtJump *stmt, uint32 level = 0);
	void DumpJump(JumpMIRONode *stmt, uint32 level = 0);	//******** OK
	void DumpIf(StmtIf *stmt, uint32 level);
	void DumpLoop(StmtLoop *stmt, uint32 level);
	void DumpDoWhile(StmtWhile *stmt, uint32 level);
	void DumpWhileDo(StmtWhile *stmt, uint32 level);
	void DumpBreak(StmtCtrl *stmt, uint32 level);
	void DumpContinue(StmtCtrl *stmt, uint32 level);
	void DumpBlock(StmtBlock *stmt, uint32 level = 0);
	void DumpBlock(BlockMIRONode *stmt, uint32 level = 0); //********* OK
	void DumpPhi(PhiMIRONode *stmt, uint32 level = 0);
	void DumpVarDetails(Node *node, uint32 level = 0);
	void DumpVarDetails(MIRONode *node, uint32 level = 0); //********* OK
	void DumpInnerFields(SymbolFieldContainer *parent, uint32 level);
	void DumpFieldDetails(Node *node, uint32 level = 0);
	void DumpFieldDetails(MIRONode *node, uint32 level = 0); //********* OK
	void DumpSwitchNetIL(StmtSwitch *stmt, uint32 level);
	void DumpSwitchNetIL(SwitchMIRONode *stmt, uint32 level); //******** OK
	void DumpJumpNetIL(StmtJump *stmt, uint32 level);
	void DumpJumpNetIL(JumpMIRONode *stmt, uint32 level); //*********** OK
	void DumpTreeNetIL(Node *node, uint32 level);
	void DumpTreeNetIL(MIRONode *node, uint32 level); //************* OK

public:
	CodeWriter(ostream &stream, string endline = "\n", uint32_t *num_Statements = NULL)
		:m_Stream(stream), endLine(endline), GenNetIL(0), _num_Statements(num_Statements) {}

	void DumpCode(CodeList *code, uint32 level = 0);
	void DumpCode(std::list<MIRONode*> *code, uint32 level = 0);	//******** OK
	void DumpTree(Node *node, uint32 level = 0);
	void DumpTree(MIRONode *node, uint32 level = 0);	//******** OK
	void DumpStatement(StmtBase *stmt, uint32 level = 0);
	void DumpStatement(StmtMIRONode *stmt, uint32 level = 0);	//******** OK
	void DumpNetIL(CodeList *code, uint32 level = 0);
	void DumpNetIL(std::list<MIRONode*> *code, uint32 level = 0); //******** OK
	static void DumpSymbol(Symbol *sym, ostream&);
	static void DumpOpCode_s(uint16_t opcode, ostream&);

	~CodeWriter() {
		if (m_Stream!=NULL)
		{
			m_Stream.flush();
		}
	}
};

