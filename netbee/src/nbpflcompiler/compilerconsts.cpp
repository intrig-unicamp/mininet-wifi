/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include "compilerconsts.h"

void CompilerConsts::AddConst(string name, uint32 value)
{
	CompilerConsts::CompilerConstInfo entry(name, value);
	m_ConstsSet.LookUp(name, entry, ENTRY_ADD);
}

bool CompilerConsts::LookupConst(string name, uint32 &value)
{
	CompilerConsts::CompilerConstInfo entry(name, value);
	bool result = m_ConstsSet.LookUp(name, entry);
	value = entry.ActualValue;
	return result;
}

