/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/
#pragma once

#include "statements.h"
#include "globalsymbols.h"
#include "pdlparser.h"
#include <iostream>


using namespace std;

class Visit
{

	protected:
	CodeList 	*Code;
	GlobalSymbols &m_GlobalSymbols;

	StmtBase		*m_actualstmt;          //pointer to the current statement

/*********************	  Visit CODE	(protected)	****************/

	void VisitSymbol(Symbol *sym);
	void VisitCase(StmtCase *stmt, bool isDefault = false);
	void VisitSwitch(StmtSwitch *stmt);
	void VisitJump(StmtJump *stmt);
	void VisitIf(StmtIf *stmt);
	void VisitLoop(StmtLoop *stmt);
	void VisitDoWhile(StmtWhile *stmt);
	void VisitWhileDo(StmtWhile *stmt);
	void VisitBlock(StmtBlock *stmt);



/***************************************************************************/
	void AddPacketLengthCheck(StmtBase *stmt);
	
	public:
	
	Visit(CodeList *Code,GlobalSymbols &m_GlobalSymbols):Code(Code),m_GlobalSymbols(m_GlobalSymbols){
	}

	~Visit(){
	}

/*********************	  Visit	(Public)	*****************************/

	void VisitCode(CodeList *code);
	void VisitTree(Node *node);
	void VisitStatement(StmtBase *stmt);

/****************************************************************************/

};

