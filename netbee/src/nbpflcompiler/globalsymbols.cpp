/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/






#include "globalsymbols.h"
#include "statements.h"
#include "errors.h"
#include "defs.h"
#include <map>


GlobalSymbols::ProtocolInfo::~ProtocolInfo()
{
	//delete all the field descriptors contained into the FieldSymbols symbol table
	StrSymbolTable<SymbolField *>::Iterator symbolIterator = FieldSymbols.Begin();
	for (; symbolIterator != FieldSymbols.End(); symbolIterator++)
	{
		SymbolField *sym=(*symbolIterator).Item;
		for (FieldsList_t::iterator defs=sym->SymbolDefs.begin(); defs!=sym->SymbolDefs.end(); defs++)
			delete (*defs);

		delete sym;

		//FieldsList_t *fieldsList = (*symbolIterator).Item;
		//FieldsList_t::iterator fieldIterator = fieldsList->begin();
		//for (; fieldIterator != fieldsList->end(); fieldIterator++)
		//	delete (*fieldIterator);

		//delete fieldsList;
	}
}




GlobalSymbols::GlobalSymbols(GlobalInfo &globalInfo)
:m_GlobalInfo(globalInfo), m_RuntimeVars(OWN_ITEMS), m_StrConsts(OWN_ITEMS), m_IntConsts(OWN_ITEMS), 
 m_LookupTables(0), m_RegExEntries(0),  m_StringMatchingEntries(0), m_RegExEntriesUseful(0), m_StringMatchEntriesUseful(0), m_DataItems(0), m_Labels(OWN_ITEMS), m_Temporaries(OWN_ITEMS),
 m_ConstOffs(0),  CurrentOffsSym(0), PacketBuffSym(0)
{
	m_NumProto = globalInfo.GetNumProto();
	m_ProtocolInfo = new ProtocolInfo[m_NumProto];
	CHECK_MEM_ALLOC(m_ProtocolInfo);
}



GlobalSymbols::~GlobalSymbols()
{
	RTVarSymbolTable_t::Iterator rtvar_i = 	m_RuntimeVars.Begin();
	for (; rtvar_i != m_RuntimeVars.End(); rtvar_i++)
		delete (*rtvar_i).Item;

	StrConstSymbolTable_t::Iterator stcon_i = m_StrConsts.Begin();
	for (; stcon_i != m_StrConsts.End(); stcon_i++)
		delete (*stcon_i).Item;

	IntConstSymbolTable_t::Iterator icon_i = m_IntConsts.Begin();
	for (; icon_i != m_IntConsts.End(); icon_i++)
		delete (*icon_i).Item;

	LabelSymbolTable_t::Iterator lab_i = m_Labels.Begin();
	for (; lab_i != m_Labels.End(); lab_i++)
		delete (*lab_i).Item;

	TempSymbolTable_t::Iterator temp_i= m_Temporaries.Begin();
	for (; temp_i != m_Temporaries.End(); temp_i++)
		delete (*temp_i).Item;

	delete []m_ProtocolInfo;
}



CodeList *GlobalSymbols::NewCodeList(bool ownsStmts)
{
	CodeList *codeList = new CodeList(ownsStmts);
	CHECK_MEM_ALLOC(codeList);
	m_MemPool.NewObj(codeList);
	return codeList;
}

void GlobalSymbols::UnlinkProtoLabels(void)
{
	for (uint32 i = 0; i < m_NumProto; i++)
		m_ProtocolInfo[i].StartLbl->Linked = NULL;
}



SymbolProto *GlobalSymbols::LookUpProto(const string protoName)
{
	return m_GlobalInfo.LookUpProto(protoName);
}

SymbolProto *GlobalSymbols::GetProto(int index)
{
	return m_GlobalInfo.GetProtoList()[index];
}

void GlobalSymbols::StoreVarSym(const string name, SymbolVariable *varSymbol)
{
	m_RuntimeVars.LookUp(name, varSymbol, ENTRY_ADD);
}

void GlobalSymbols::StoreTempSym(const uint32 temp, SymbolTemp *tempSymbol)
{
	m_Temporaries.LookUp(temp, tempSymbol, ENTRY_ADD);
}

void GlobalSymbols::StoreConstSym(const uint32 value, SymbolIntConst *constSymbol)
{
	m_IntConsts.LookUp(value, constSymbol, ENTRY_ADD);
}

void GlobalSymbols::StoreConstSym(const string value, SymbolStrConst *constSymbol)
{
	m_StrConsts.LookUp(value, constSymbol, ENTRY_ADD);
}

void GlobalSymbols::StoreRegExEntry(SymbolRegEx *regEx, SymbolProto *protocol)
{
	m_RegExEntries.push_back(regEx);
	m_RegExProtos.push_back(protocol);
}

void GlobalSymbols::StoreStringMatchingEntry(SymbolRegEx *regEx, SymbolProto *protocol)
{
	m_StringMatchingEntries.push_back(regEx);
	m_StringMatchingProtos.push_back(protocol);
}

void GlobalSymbols::StoreRegExEntryUseful(SymbolRegEx *regEx)
{
	m_RegExEntriesUseful.push_back(regEx);
}

void GlobalSymbols::StoreStringMatchingEntryUseful(SymbolRegEx *regEx)
{
	m_StringMatchEntriesUseful.push_back(regEx);
}

int GlobalSymbols::GetRegExEntriesCount()
{
	nbASSERT(m_RegExEntries.size() == m_RegExProtos.size(),"There is a bug! The two lists must have the same size!");

	int toReturn = m_RegExEntries.size();

	return toReturn;
}

int GlobalSymbols::GetStringMatchingEntriesCount()
{
	nbASSERT(m_StringMatchingEntries.size() == m_StringMatchingProtos.size(),"There is a bug! The two lists must have the same size!");
	
	int toReturn = m_StringMatchingEntries.size();

	return toReturn;
}

int GlobalSymbols::GetRegExEntriesCountUseful()
{

	return m_RegExEntriesUseful.size();
}

int GlobalSymbols::GetStringMatchingEntriesCountUseful()
{
	return m_StringMatchEntriesUseful.size();
}

void GlobalSymbols::StoreDataItem(SymbolDataItem *item)
{
	m_DataItems.push_back(item);
}

DataItemList_t *GlobalSymbols::GetDataItems()
{
	return &m_DataItems;
}

void GlobalSymbols::StoreLabelSym(const uint32 label, SymbolLabel *labelSymbol)
{
	m_Labels.LookUp(label, labelSymbol, ENTRY_ADD);
}


SymbolLabel *GlobalSymbols::LookUpLabel(const uint32 label)
{
	SymbolLabel *labelSymbol = NULL;
	if (!m_Labels.LookUp(label, labelSymbol))
		return NULL;

	return labelSymbol;
}




SymbolVariable *GlobalSymbols::LookUpVariable(const string name)
{
	if (name.length() == 0)
		return NULL;

	SymbolVariable *varSym(0);
	if (!m_RuntimeVars.LookUp(name, varSym))
		return NULL;

	if (varSym->IsDefined==false || varSym->IsSupported==false)
		return NULL;

	return varSym;
}


SymbolIntConst *GlobalSymbols::LookUpConst(const uint32 value)
{
	SymbolIntConst *constSym;
	if (!m_IntConsts.LookUp(value, constSym))
		return NULL;

	return constSym;
}

SymbolStrConst *GlobalSymbols::LookUpConst(const string value)
{
	if (value.length() == 0)
		return NULL;

	SymbolStrConst *constSym(0);
	if (!m_StrConsts.LookUp(value, constSym))
		return NULL;

	return constSym;
}

bool GlobalSymbols::CompareSymbolFields(const SymbolField *sym1, const SymbolField *sym2)
{
	if (sym1->FieldType != sym2->FieldType)
		return false;

	switch (sym1->FieldType)
	{
	case PDL_FIELD_FIXED:
		{
			if (((SymbolFieldFixed *)sym1)->Size!=((SymbolFieldFixed *)sym2)->Size)
				return false;
			else
				return true;
		}break;

	case PDL_FIELD_BITFIELD:
		{
			if (((SymbolFieldBitField *)sym1)->Mask!=((SymbolFieldBitField *)sym2)->Mask)
				return false;
			else if (((SymbolFieldBitField *)sym1)->InnerFields.size()!=((SymbolFieldBitField *)sym2)->InnerFields.size())
				return false;
			else
				return true;
		}break;

	case PDL_FIELD_PADDING:
		{
			if (((SymbolFieldPadding *)sym1)->Align!=((SymbolFieldPadding *)sym2)->Align)
				return false;
			else
				return true;
		}break;

	case PDL_FIELD_VARLEN:
		{
			if (((SymbolFieldVarLen *)sym1)->LenExpr!=((SymbolFieldVarLen *)sym2)->LenExpr)
				return false;
			else if (((SymbolFieldVarLen *)sym1)->DefPoint!=((SymbolFieldVarLen *)sym2)->DefPoint)
				return false;
			else
				return true;
		}break;

	case PDL_FIELD_TOKEND:
		{
			if(((SymbolFieldTokEnd *)sym1)->EndTok!=((SymbolFieldTokEnd *)sym2)->EndTok
				||((SymbolFieldTokEnd *)sym1)->EndTokSize!=((SymbolFieldTokEnd *)sym2)->EndTokSize)
				return false;
			else if(((SymbolFieldTokEnd *)sym1)->EndRegEx!=((SymbolFieldTokEnd *)sym2)->EndRegEx)
				return false;
			else if(((SymbolFieldTokEnd *)sym1)->EndOff!=((SymbolFieldTokEnd *)sym2)->EndOff)
				return false;
			else if(((SymbolFieldTokEnd *)sym1)->EndDiscard!=((SymbolFieldTokEnd *)sym2)->EndDiscard)
				return false;
			else if (((SymbolFieldTokEnd *)sym1)->DefPoint!=((SymbolFieldTokEnd *)sym2)->DefPoint)
				return false;
			else
				return true;
		}break;




	case PDL_FIELD_TOKWRAP:
		{
			if(((SymbolFieldTokWrap *)sym1)->BeginTok!=((SymbolFieldTokWrap *)sym2)->BeginTok
				||((SymbolFieldTokWrap *)sym1)->BeginTokSize!=((SymbolFieldTokWrap *)sym2)->BeginTokSize)
				return false;
			else if(((SymbolFieldTokWrap *)sym1)->EndTok!=((SymbolFieldTokWrap *)sym2)->EndTok
				||((SymbolFieldTokWrap *)sym1)->EndTokSize!=((SymbolFieldTokWrap *)sym2)->EndTokSize)
				return false;
			else if(((SymbolFieldTokWrap *)sym1)->BeginRegEx!=((SymbolFieldTokWrap *)sym2)->BeginRegEx)
				return false;
			else if(((SymbolFieldTokWrap *)sym1)->EndRegEx!=((SymbolFieldTokWrap *)sym2)->EndRegEx)
				return false;
			else if(((SymbolFieldTokWrap *)sym1)->BeginOff!=((SymbolFieldTokWrap *)sym2)->BeginOff)
				return false;
			else if(((SymbolFieldTokWrap *)sym1)->EndOff!=((SymbolFieldTokWrap *)sym2)->EndOff)
				return false;
			else if(((SymbolFieldTokWrap *)sym1)->EndDiscard!=((SymbolFieldTokWrap *)sym2)->EndDiscard)
				return false;
			else if (((SymbolFieldTokWrap *)sym1)->DefPoint!=((SymbolFieldTokWrap *)sym2)->DefPoint)
				return false;
			else
				return true;
		}break;

	case PDL_FIELD_LINE:
		{
			if (((SymbolFieldLine *)sym1)->DefPoint!=((SymbolFieldLine *)sym2)->DefPoint)
				return false;
			else
				return true;
		}break;
	
	case PDL_FIELD_PATTERN:
		{
			if (((SymbolFieldPattern *)sym1)->Pattern!=((SymbolFieldPattern *)sym2)->Pattern)
				return false;
			else if (((SymbolFieldPattern *)sym1)->DefPoint!=((SymbolFieldPattern *)sym2)->DefPoint)
				return false;
			else
				return true;
		}break;

	case PDL_FIELD_EATALL:
		{
			if (((SymbolFieldEatAll *)sym1)->DefPoint!=((SymbolFieldEatAll *)sym2)->DefPoint)
				return false;
			else
				return true;
		}break;
	
	case PDL_FIELD_ALLFIELDS: //[icerrato]
		{
			//it can exist only a SymbolField of this time in the GlobalSymbolTable
			return true;
		}break;
	default:
		nbASSERT(false, "Symbols to compare should be fixed, bitfield, padding, varlen, pattern, eatall or allfields");
		break;
	}

	return false;
}

/********************************************************************************************************************************************************/

//Fields are not consecutive! 
/*
Temp fieldList
for each field (used) -> copy to temp_fieldList
Update NumberOfFields
Clean m_FieldList
Overwrite m_FieldList
*/
//TO BE FIXED ->
//NEED TO KNOW WHICH FIELD MUST BE COUNTED AND WHICH NOT ( IF USED (inside expressions) -> YES , if can't be compatted with: (other fixed =(!Compattable)  OR inside loops= ( ? )) ) 
//REVISIT HIR CODE ???
#ifdef ENABLE_FIELD_OPT
void GlobalSymbols::DeleteFieldsRefs(SymbolProto *protoSym)
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");

	uint32 dim=m_ProtocolInfo[protoSym->ID].m_NumField;  //lenght of FieldList vector
	uint32 j=0;

	SymbolField **TempList;
	SymbolField *TempField;

	TempList=(SymbolField **)malloc(dim*sizeof(SymbolField *));

	for(uint32 k=0;k<dim;k++)
	{
		TempField=m_ProtocolInfo[protoSym->ID].m_FieldList[k];

		//if this field is used or !Compattable -> we need it (NOT TRUE)
		//cout<<TempField->Protocol->Name<<"."<<TempField->Name<<" ToKeep? "<<TempField->ToKeep<<endl;
		if(TempField->ToKeep)
		{
			TempList[j]=m_ProtocolInfo[protoSym->ID].m_FieldList[k];
			j++;
		}
		m_ProtocolInfo[protoSym->ID].m_FieldList[k]=0; //clean previous fieldList
	}

	m_ProtocolInfo[protoSym->ID].m_FieldList=TempList;

//	cout<<"Total NumberOfFields: "<<m_ProtocolInfo[protoSym->ID].m_NumField<<" Actual: "<<j<<endl;
	nbASSERT(m_ProtocolInfo[protoSym->ID].m_NumField>=j, "CAN'T BE!!");

	m_ProtocolInfo[protoSym->ID].m_NumField=j; 	     
	
	TempField=NULL;
	TempList=NULL;

	//free(TempList);	//works?
}
#endif


//	deprecated
/*
void GlobalSymbols::DeleteFieldsRef(SymbolProto *protoSym, uint32 from, uint32 to)		
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");

	uint32 numoffields=to-from;
	nbASSERT(numoffields > 0, "Number of fields to delete is <= 0 ");

	nbASSERT(from < 200, "FROM ! fields to delete is !TRUE ");
	nbASSERT(to < 200, "TO ! fields to delete is !TRUE ");


	uint32 dim=m_ProtocolInfo[protoSym->ID].m_NumField;  //lenght of FieldList vector
	uint32 j=0;

	for(uint32 k=to+1;k<dim;k++)
	{
		//shift & overwrite
		m_ProtocolInfo[protoSym->ID].m_FieldList[from+j]=m_ProtocolInfo[protoSym->ID].m_FieldList[k];
		j++;
	}


		//delete	
	for(uint32 k=(from+j);k<dim;k++)
	{
		m_ProtocolInfo[protoSym->ID].m_FieldList[k]=0;
	}
	
	m_ProtocolInfo[protoSym->ID].m_NumField=dim-(to-from+1); 	     

}


void GlobalSymbols::DeleteFieldRef(SymbolProto *protoSym, SymbolField *fieldSym)
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");
	nbASSERT(fieldSym != NULL, "field symbol should not be NULL");

	uint32 dim=m_ProtocolInfo[protoSym->ID].m_NumField;  //lenght of FieldList vector
	uint32 id=fieldSym->ID;

	for(uint32 k=id;k<(dim-1);k++)
	{
		//shift & overwrite
		m_ProtocolInfo[protoSym->ID].m_FieldList[k]=m_ProtocolInfo[protoSym->ID].m_FieldList[k+1];	
	}

	m_ProtocolInfo[protoSym->ID].m_FieldList[dim-1]=0;   //clean field-list

	m_ProtocolInfo[protoSym->ID].m_NumField--; 	     //update number of deffld

}

void GlobalSymbols::PrintGlobalInfo(SymbolProto *protoSym, SymbolField *fieldSym)
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");
	nbASSERT(fieldSym != NULL, "field symbol should not be NULL");
	cout<<"Protocol :"<<protoSym->Name<<endl;
	cout<<"fieldSym :"<<fieldSym->ID<<endl;
	cout<<"fieldSym-name :"<<fieldSym->Name<<endl;
	cout<<"Global (FieldList_ID): "<<m_ProtocolInfo[protoSym->ID].m_FieldList[fieldSym->ID]<<endl;
	//cout<<"Global (FieldList_ID)NAME: "<<m_ProtocolInfo[protoSym->ID].m_FieldList[fieldSym->ID]->Name<<endl;
}


//TEMP [mligios]
*/
/*******************************************************************************************************************************/

SymbolField *GlobalSymbols::StoreProtoField(SymbolProto *protoSym, SymbolField *fieldSym)
{
	return this->StoreProtoField(protoSym, fieldSym, false);
}

SymbolField *GlobalSymbols::StoreProtoField(SymbolProto *protoSym, SymbolField *fieldSym, bool force)
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");
	nbASSERT(fieldSym != NULL, "field symbol should not be NULL");
	nbASSERT(fieldSym->Name != "", "field name should not be empty");

	SymbolField *sym = LookUpProtoField(protoSym, fieldSym);

	if (sym == NULL)
	{
		sym=fieldSym;
		m_ProtocolInfo[protoSym->ID].FieldSymbols.LookUp(sym->Name, sym, ENTRY_ADD);
		uint32 i=m_ProtocolInfo[protoSym->ID].m_NumField;
		sym->ID=i;
		m_ProtocolInfo[protoSym->ID].m_FieldList = (SymbolField**)realloc(m_ProtocolInfo[protoSym->ID].m_FieldList,(i+1)*sizeof(struct SymbolField*));
		m_ProtocolInfo[protoSym->ID].m_FieldList[i]=sym;
		m_ProtocolInfo[protoSym->ID].m_NumField++;
		return sym;
	}
	else
	{
		// the symbol is already present in the protocol symbols list
		bool toDelete=true;
		if (force)
		{
			sym->SymbolDefs.push_back(fieldSym);
			sym=fieldSym;
			toDelete=false;
		}
		else
		{
			if (this->CompareSymbolFields(sym, fieldSym) == false)
			{
				// the symbol to be stored is not the same as the one in the symbols list
				bool toAdd=true;
				FieldsList_t::iterator i=sym->SymbolDefs.begin();
				for (; i!=sym->SymbolDefs.end(); i++)
				{
					// for each symbol in the inner list compares the two symbols
					if (this->CompareSymbolFields(fieldSym, *i) == true)
					{
						// if this is the same, so do not store it
						sym=*i;
						toAdd=false;
						break;
					}
				}

				if (toAdd)
				{
					// the symbol to add is different from the other definitions
					sym->SymbolDefs.push_back(fieldSym);
					sym=fieldSym;
					uint32 i=m_ProtocolInfo[protoSym->ID].m_NumField;
					sym->ID=i;
					m_ProtocolInfo[protoSym->ID].m_FieldList = (SymbolField**)realloc(m_ProtocolInfo[protoSym->ID].m_FieldList,(i+1)*sizeof(struct SymbolField*));
					m_ProtocolInfo[protoSym->ID].m_FieldList[i]=sym;
					m_ProtocolInfo[protoSym->ID].m_NumField++;
					toDelete=false;
				}
			}
		}

		if (fieldSym->IntCompatible==false)
			sym->IntCompatible=false;
		if (fieldSym->UsedAsInt==true)
			sym->UsedAsInt=true;
		if (fieldSym->UsedAsString==true)
			sym->UsedAsString=true;
		if (fieldSym->UsedAsArray==true)
			sym->UsedAsArray=true;

		if (toDelete)
			delete fieldSym;
		return sym;

	}

}

SymbolField *GlobalSymbols::LookUpProtoFieldById(const string protoName, uint32 index)
{
	nbASSERT(protoName.length() != 0, "protocol name length should not be 0");
	nbASSERT(index >= 0, "id should be a positive integer");

	return LookUpProtoFieldById(LookUpProto(protoName), index);
}

SymbolField *GlobalSymbols::LookUpProtoFieldById(SymbolProto *protoSym, uint32 index)
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");
	nbASSERT(index >= 0, "id should be a positive integer");

	return m_ProtocolInfo[protoSym->ID].GetFieldList()[index];
}

SymbolField *GlobalSymbols::LookUpProtoFieldByName(const string protoName, const string fieldName)
{
	nbASSERT(protoName.length() != 0, "protocol name length should not be 0");
	nbASSERT(fieldName.size() != 0, "field name should not be empty");

	return LookUpProtoFieldByName(LookUpProto(protoName), fieldName);
}

SymbolField *GlobalSymbols::LookUpProtoFieldByName(SymbolProto *protoSym, const string fieldName)
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");
	nbASSERT(fieldName.size() != 0, "field name should not be empty");

	SymbolField *sym = NULL;

	m_ProtocolInfo[protoSym->ID].FieldSymbols.LookUp(fieldName, sym);

	return sym;
}


SymbolField *GlobalSymbols::LookUpProtoField(const string protoName, const SymbolField *field)
{
	nbASSERT(protoName.length() != 0, "protocol name length should not be 0");
	nbASSERT(field != NULL, "field should not be null");

	return LookUpProtoField(LookUpProto(protoName), field);
}


SymbolField *GlobalSymbols::LookUpProtoField(SymbolProto *protoSym, const SymbolField *field)
{
	nbASSERT(protoSym != NULL, "protocol symbol should not be NULL");
	nbASSERT(field != NULL, "field should not be null");

	SymbolField *sym = NULL;

	m_ProtocolInfo[protoSym->ID].FieldSymbols.LookUp(field->Name, sym);

	if (sym!=NULL)
	{
		if (this->CompareSymbolFields(sym, field) == false)
		{
			FieldsList_t::iterator i=sym->SymbolDefs.begin();
			for (; i!=sym->SymbolDefs.end(); i++)
			{
				if (this->CompareSymbolFields(*i, field) == true)
				{
					return *i;
				}
			}
		}
	}

	return sym;
}

SymbolLookupTable *GlobalSymbols::LookUpLookupTable(const string name)
{
	SymbolLookupTable *table=NULL;

	this->m_LookupTables.LookUp(name, table, false);

	return table;
}

void GlobalSymbols::StoreLookupTable(const string name, SymbolLookupTable *lookupTable)
{
	this->m_LookupTables.LookUp(name, lookupTable, true);
}

/*
*	If, in the NetPDL, protocol uses the table "name", then the association is created
*/
void GlobalSymbols::AssociateProtoTable(SymbolProto *protocol,const string name)
{
	ProtosTables_t::iterator protocols = m_ProtosTables.find(protocol);
	set<string> tables;
	
	if(protocols != m_ProtosTables.end())
		tables = (*protocols).second;
	
	tables.insert(name);
	m_ProtosTables[protocol]=tables;	
}

/*
*	If, in the NetPDL, protocol uses the table "name", then the association is created
*/
void GlobalSymbols::AssociateTableProto(const string name,SymbolProto *protocol)
{
	TablesProtos_t::iterator tables = m_TablesProtos.find(name);
	set<SymbolProto*> protocols;
	
	if(tables != m_TablesProtos.end())
		protocols = (*tables).second;
	
	protocols.insert(protocol);
	m_TablesProtos[name]=protocols;	
}

/*
*	Remove the association protocol-table
*/
void GlobalSymbols::RemoveProtoTable(SymbolProto *protocol, const string name)
{
	ProtosTables_t::iterator protocols = m_ProtosTables.find(protocol);
	nbASSERT(protocols != m_ProtosTables.end(),"There is a BUG!");
	set<string> tables = (*protocols).second;
	nbASSERT(tables.size() != 0,"There is a BUG!");
	for(set<string>::iterator t = tables.begin(); t != tables.end(); t++)
	{
		if(*t == name)
		{
			tables.erase(t);
			return;
		}
	}
	nbASSERT(false,"There is a BUG!");
}

/*
*	Returns the set of tables used by protocol
*/
set<string> GlobalSymbols::GetAssociationProtosTable(SymbolProto *protocol)
{
	ProtosTables_t::iterator proto = m_ProtosTables.find(protocol);
	set<string> table;
	
	if(proto != m_ProtosTables.end())
		table = (*proto).second;
	
	return table;	
}

/*
*	Returns the set of protocols using the table "name"
*/
set<SymbolProto*> GlobalSymbols::GetAssociationTableProtos(string name)
{
	TablesProtos_t::iterator table = m_TablesProtos.find(name);
	set<SymbolProto*> protos;
	
	if(table != m_TablesProtos.end())
		protos = (*table).second;
	
	return protos;	
}

/*
*	Store the name of a useful table 
*/
void GlobalSymbols::StoreUsefulTable(string table)
{
	m_UsefulTables.insert(table);
}

/*
*	Returns true if the "name" table is useful
*/
bool GlobalSymbols::IsLookUpTableUseful(const string name)
{
	return (m_UsefulTables.count(name) == 0)? false : true;
}

/*
*	Returns the number of useful tables 
*/
int GlobalSymbols::GetUsefulTablesNumber()
{
	return m_UsefulTables.size();
}

SymbolLabel *GlobalSymbols::GetProtoStartLbl(SymbolProto *protoSym)
{
	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
	return m_ProtocolInfo[protoSym->ID].StartLbl;
}

void GlobalSymbols::SetProtoStartLbl(SymbolProto *protoSym, SymbolLabel *label)
{
	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
	nbASSERT(label != NULL, "label cannot be NULL");
	m_ProtocolInfo[protoSym->ID].StartLbl = label;
}


SymbolVarInt *GlobalSymbols::GetProtoOffsVar(SymbolProto *protoSym)
{
	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
	return m_ProtocolInfo[protoSym->ID].ProtoOffsVar;
}


void GlobalSymbols::SetProtoOffsVar(SymbolProto *protoSym, SymbolVarInt *var)
{
	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
	nbASSERT(var != NULL, "label cannot be NULL");
	m_ProtocolInfo[protoSym->ID].ProtoOffsVar = var;
}


CodeList &GlobalSymbols::GetProtoCode(SymbolProto *protoSym)
{
	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
	return m_ProtocolInfo[protoSym->ID].Code;
}

void GlobalSymbols::AddFieldExtractStatment(SymbolProto *protoSym, StmtFieldInfo *stmt)
{
	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
	m_ProtocolInfo[protoSym->ID].StoreFldInfo.PushBack(stmt);
}

CodeList &GlobalSymbols::GetFieldExtractCode(SymbolProto *protoSym)
{
	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
	return m_ProtocolInfo[protoSym->ID].StoreFldInfo;
}

//
//FldScopeStack_t &GlobalSymbols::GetProtoFieldScopes(SymbolProto *protoSym)
//{
//	nbASSERT(protoSym != NULL, "protoSym cannot be NULL");
//	return m_ProtocolInfo[protoSym->ID].FieldScopes;
//}


uint32 GlobalSymbols::IncConstOffs(uint32 delta)
{
	uint32 val = m_ConstOffs;
	m_ConstOffs += delta;
	return val;
}

void GlobalSymbols::DumpProtoFieldStats(std::ostream &stream)
{
	uint32 numProto = m_GlobalInfo.GetNumProto();
	SymbolProto **protoList = m_GlobalInfo.GetProtoList();
	
	for (uint32 i = 0; i < numProto; i ++)
	{
		std::map<uint32, uint32> fieldTypeCounters;
		
		StrSymbolTable<SymbolField *>::Iterator j = m_ProtocolInfo[i].FieldSymbols.Begin();
		for (; j != m_ProtocolInfo[i].FieldSymbols.End(); j++)
		{
			SymbolField *field = (SymbolField*) (*j).Item;
			std::map<uint32, uint32>::iterator res = fieldTypeCounters.find(field->FieldType);
			if (res != fieldTypeCounters.end())
				(*res).second++;
			fieldTypeCounters.insert(pair<uint32, uint32>(field->FieldType, 1));
		}
		stream << protoList[i]->Name << ":" << std::endl;
		std::map<uint32, uint32>::iterator k = fieldTypeCounters.begin();
		for (; k != fieldTypeCounters.end(); k++)
		{
			switch((*k).first)	
			{
				case PDL_FIELD_FIXED:
				{
					stream << "fixed: ";
				}break;

				case PDL_FIELD_VARLEN:
				{
					stream << "varlen: ";
				}break;

				case PDL_FIELD_PADDING:
				{
					stream << "padding: ";
				}break;

				case PDL_FIELD_TOKEND:
				{
					stream << "tokenended: ";
				}break;

				case PDL_FIELD_TOKWRAP:
				{
					stream << "tokenwrapped: ";
					
				}break;


				case PDL_FIELD_LINE:
				{
					stream << "line: ";

				}break;

				case PDL_FIELD_PATTERN:
				{
					stream << "pattern: ";
				}break;

				case PDL_FIELD_EATALL:
				{
					stream << "eatall: ";
				}break;

				case PDL_FIELD_BITFIELD:
				    stream << "bit: ";
				break;
			}
			stream << (*k).second << std::endl ;
		}
		stream << std::endl;
	} 
}



