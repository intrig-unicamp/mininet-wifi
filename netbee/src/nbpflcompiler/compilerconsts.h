/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include <string>
#include "strsymboltable.h"

using namespace std;





class CompilerConsts
{
private:
	struct CompilerConstInfo
	{
		string	Name;
		uint32	ActualValue;

		CompilerConstInfo(string name, uint32 value)
			:Name(name), ActualValue(value){}
	};

	StrSymbolTable<CompilerConstInfo> m_ConstsSet;

public:
	
	void AddConst(string name, uint32 value);

	bool LookupConst(string name, uint32 &value);

};
