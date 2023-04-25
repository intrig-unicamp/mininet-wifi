/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "tree.h"
#include "statements.h"
#include <iostream>
#include "pflcfg.h"
#include "mironode.h"
#include <list>
using namespace std;


struct CompilationUnit
{
	uint32		NumLocals;
	uint32		MaxStack;
	uint32		OutPort;

	CodeList		IRCode;
	CodeList		InitIRCode;
	CodeList		DataSegmentCode;

	string		PFLSource;
	
	PFLCFG			InitCfg;
	PFLCFG			DataSegmentCfg;
	PFLCFG			Cfg;

	bool			UsingCoproLookupEx;
	bool			UsingCoproStringMatching;
	bool			UsingCoproRegEx;
	DataItemList_t	*DataItems;

	CompilationUnit(string source)
		:NumLocals(0), MaxStack(0), PFLSource(source),
		UsingCoproLookupEx(false), UsingCoproStringMatching(false), UsingCoproRegEx(false),
		DataItems(0)
	{}
	
	void GenerateNetIL(ostream &stream);

	~CompilationUnit();
};
