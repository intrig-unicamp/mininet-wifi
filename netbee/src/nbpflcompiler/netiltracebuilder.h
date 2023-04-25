/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "cfgvisitor.h"
#include <iostream>
using namespace std;




class NetILTraceBuilder: public ICFGVisitorHandler
{
	typedef PFLCFG::BBType BBType;
private:
	ostream		&m_Stream;
	bool		m_ValidationPass;
	PFLCFG			*m_CFG;


	virtual void VisitBasicBlock(BBType *bb, BBType *comingFrom);
	
	
	virtual void VisitBBEdge(BBType *from, BBType *to)
	{
		//empty handler
		return;
	}

	void InvertBranch(JumpMIRONode *brStmt);
public:

	NetILTraceBuilder(ostream &stream)
		:m_Stream(stream), m_ValidationPass(0){}

	void CreateTrace(PFLCFG &cfg);

	
};

