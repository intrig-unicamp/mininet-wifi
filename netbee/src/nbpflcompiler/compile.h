/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "netpflfrontend.h"
#include "encapgraph.h"
#include "pflexpression.h"
#include "globalsymbols.h"
#include "errors.h"

struct ParserInfo
{
	GlobalSymbols   *GlobalSyms;
	ErrorRecorder	*ErrRecorder;
	PFLStatement	*Filter;

	EncapGraph	*ProtoGraph;
	HIRCodeGen		CodeGen;
	string			FilterString;

	ParserInfo(GlobalSymbols &globalSyms, ErrorRecorder &errRecorder)
		:GlobalSyms(&globalSyms), ErrRecorder(&errRecorder), Filter(0), CodeGen(globalSyms){}


	void ResetFilter(void)
	{
		Filter = NULL;
	}

};

#define ID_LEN 256

extern int pfl_parse(struct ParserInfo *parserInfo);
void pflcompiler_lex_init(const char *buf);
void pflcompiler_lex_cleanup();


extern void compile(ParserInfo *parserInfo, const char *filter, int debug);

