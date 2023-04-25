/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/






#include "globalinfo.h"
#include "protographwriter.h"


GlobalInfo::GlobalInfo(_nbNetPDLDatabase &protoDB)
		:m_ProtoDB(&protoDB), m_ProtoSymbols(OWN_ITEMS)
{
	Debugging.DebugLevel= 0;
	Debugging.DumpHIRCodeFilename= NULL;
	Debugging.DumpProtoGraphFilename= NULL;
	Debugging.DumpFilterAutomatonFilename= NULL;

	m_NumProto = protoDB.ProtoListNItems;

	m_ProtoList = new SymbolProto*[m_NumProto];
	CHECK_MEM_ALLOC(m_ProtoList);

	uint32 i = 0;
	//init the protocol info list and symbol table
	//add the nodes to the both the PEG: the complete one and the preferred one
	for (i = 0; i < m_NumProto; i++)
	{
		SymbolProto *protoSymbol =  new SymbolProto(*protoDB.ProtoList[i], i);
		CHECK_MEM_ALLOC(protoSymbol);

		m_ProtoList[i] = protoSymbol;
		StoreProtoSym(protoSymbol->Name, protoSymbol);
		m_ProtoGraph.AddNode(protoSymbol);
		m_ProtoGraphPreferred.AddNode(protoSymbol); //[icerrato]
	}

	m_StartProtoSym = LookUpProto("startproto");
	m_ProtoGraph.SetStartProtoNode(m_StartProtoSym);
	m_ProtoGraphPreferred.SetStartProtoNode(m_StartProtoSym); //[icerrato]
}


GlobalInfo::~GlobalInfo()
{
	// we need only to deallocate the ProtoList, since the protocol symbol table
	// contains references to the elements of such array
	for (uint32 i = 0; i < m_NumProto; i++)
		delete m_ProtoList[i];
	delete[] m_ProtoList;
}



void GlobalInfo::DumpProtoGraph(ostream &stream)
{
	ProtoGraphWriter protoWriter(stream);
	protoWriter.DumpProtoGraph(m_ProtoGraph);
}

//[icerrato]
void GlobalInfo::DumpProtoGraphPreferred(ostream &stream)
{
	ProtoGraphWriter protoWriter(stream);
	protoWriter.DumpProtoGraph(m_ProtoGraphPreferred);
}

void GlobalInfo::StoreProtoSym(const string protoName, SymbolProto *protoSymbol)
{
	m_ProtoSymbols.LookUp(protoName, protoSymbol, ENTRY_ADD);
}



SymbolProto *GlobalInfo::LookUpProto(const string protoName)
{
	if (protoName.length() == 0)
		return NULL;

	SymbolProto *protoInfo(0);
	if (!m_ProtoSymbols.LookUp(protoName, protoInfo))
		return NULL;

	return protoInfo;
}




void GlobalInfo::DumpProtoSymbols(ostream &stream)
{
	ProtoSymbolTable_t::Iterator i = m_ProtoSymbols.Begin();
	uint32 j = 0;
	for (; i != m_ProtoSymbols.End(); i++)
	{
		SymbolProto *protoInfo = (SymbolProto*) (*i).Item;
		stream << j++ << ". " << protoInfo->Name << " - " << protoInfo->Info->LongName << endl;

	}

}
