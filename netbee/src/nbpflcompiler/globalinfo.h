/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "defs.h"
#include "symbols.h"
#include "compilerconsts.h"
#include "encapgraph.h"
#include "../nbee/globals/debug.h"

#include <iostream>
using namespace std;



#ifndef	_DEBUGGING_INFO
#define	_DEBUGGING_INFO
struct DebuggingInfo
{
	int					DebugLevel;
	char				*DumpHIRCodeFilename;
	char				*DumpMIRNoOptGraphFilename;
	char				*DumpProtoGraphFilename;
	char				*DumpFilterAutomatonFilename;
};
#endif


/*!
	\brief This class contains the general structures that are shared by the main components of the compiler
*/
class GlobalInfo
{

private:
	_nbNetPDLDatabase		*m_ProtoDB;			//!< Reference to the NetPDL protocol database
	SymbolProto				**m_ProtoList;		//!< Array of all protocol symbols
	uint32					m_NumProto;			//!< Number of protocols contained inside the database
	ProtoSymbolTable_t		m_ProtoSymbols;		//!< Protocol Symbol Table (associates a protocol name to a \ref SymbolProto symbol)
	CompilerConsts			m_CompilerConsts;
	EncapGraph				m_ProtoGraph;
	EncapGraph				m_ProtoGraphPreferred;
	SymbolProto				*m_StartProtoSym;

public:
	/*!
		\brief	Defined the debugging level (0 debugging disabled, 3 max verbose level)
	*/
	DebuggingInfo			Debugging;			// [ds] Added debugging info levels

	/*!
		\brief	Object constructor

		\return nothing
	*/

	GlobalInfo(_nbNetPDLDatabase &protoDB);

	/*!
		\brief	Object destructor
	*/
	~GlobalInfo();

	//!\todo these should be private!

	void StoreProtoSym(const string protoName, SymbolProto *protoSymbol);
	SymbolProto *LookUpProto(const string protoName);
	void DumpProtoSymbols(ostream &stream);
	void DumpProtoGraph(ostream &stream);
	void DumpProtoGraphPreferred(ostream &stream); //[icerrato]

	CompilerConsts &GetCompilerConsts(void)
	{
		return m_CompilerConsts;
	}

	ProtoSymbolTable_t &GetProtoSymbolTable(void)
	{
		return m_ProtoSymbols;
	}


	SymbolProto **GetProtoList(void)
	{
		return m_ProtoList;
	}

	EncapGraph &GetProtoGraph(void)
	{
		return m_ProtoGraph;
	}
	
	//[icerrato]
	EncapGraph &GetProtoGraphPreferred(void)
	{
		return m_ProtoGraphPreferred;
	}


	SymbolProto *GetStartProtoSym(void)
	{
		return m_StartProtoSym;
	}



	uint32 GetNumProto(void)
	{
		return m_NumProto;
	}

	bool IsInitialized(void)
	{
		return (m_StartProtoSym != NULL);
	}
};
