/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "defs.h"
#include "symbols.h"
#include "globalinfo.h"
#include <ostream>

struct StmtBase; //forward declaration
class CodeList; //forward declaration




/*!
\brief This class contains the collection of all global symbol tables
*/


class GlobalSymbols
{
	friend class netpflfrontend;
private:
	class LabelGen
	{
	private:
		uint32		m_LblCount;

	public:
		LabelGen()
			:m_LblCount(0){}


		uint32 GetLabelCount()
		{
			return m_LblCount;
		}

		uint32 GetNew(uint32 n)
		{
			uint32 lbl = m_LblCount;
			m_LblCount += n;
			return lbl;
		}

	};

	class TempGen
	{
	private:
		uint32		m_TmpCount;

	public:
		TempGen()
			:m_TmpCount(1){}

		uint32 GetTempCount()
		{
			return m_TmpCount;
		}

		uint32 GetNew(uint32 n)
		{
			uint32 tmp = m_TmpCount;
			m_TmpCount += n;
			return tmp;
		}

	};

	template <class T>
	class MemPool
	{

	public:
		typedef T* type_ptr;
		typedef T type;
	private:
		std::list<type_ptr> Objects;
	public:
		MemPool(){}
		~MemPool()
		{
			typename list<type_ptr>::iterator i = Objects.begin();

			for (; i != Objects.end(); i++)
				delete (*i);
		}

		void NewObj(type_ptr obj)
		{
			Objects.push_back(obj);
		}
	};

	struct ProtocolInfo
	{
		StrSymbolTable<SymbolField *>			FieldSymbols;		//!<2nd level symbol table for references to protocol fields
		SymbolField				**m_FieldList;
		uint32					m_NumField;
		SymbolLabel				*StartLbl;
		SymbolVarInt			*ProtoOffsVar;
		CodeList				Code;			//Protocol format code
		CodeList 				StoreFldInfo;		// codelist for the ExtractField Action

		ProtocolInfo()
                :m_FieldList(0),m_NumField(0),StartLbl(0), ProtoOffsVar(0){}

		~ProtocolInfo();

		SymbolField **GetFieldList(void)
		{
			return m_FieldList;
		}
	};

	GlobalInfo				&m_GlobalInfo;				//!< Reference to the \ref GlobalInfo structure holding shared information
	RTVarSymbolTable_t		m_RuntimeVars;				//!< NetPDL runtime variables symbol table
	StrConstSymbolTable_t	m_StrConsts;				//!< NetPDL string constants
	IntConstSymbolTable_t	m_IntConsts;				//!< Integer Constants
	LookupTablesTable_t		m_LookupTables;				//!< Lookup tables
	RegExList_t				m_RegExEntries;				//!< RegEx entries
	RegExList_t 			m_StringMatchingEntries;	//!< StringMatching entries
	RegExList_t				m_RegExEntriesUseful;		//!< RegEx entries to lower in mir code	
	RegExList_t				m_StringMatchEntriesUseful;	//!< StringMatching entries to lower in mir code
	ProtoList_t				m_RegExProtos;				//!< Protocols associated to the RegEx entries
	ProtoList_t				m_StringMatchingProtos;		//!< Protocols associated to the StringMatching entries
	DataItemList_t			m_DataItems;				//!< Data items
	LabelSymbolTable_t		m_Labels;					//!< Labels
	TempSymbolTable_t		m_Temporaries;				//!< Global compiler-generated temporaries
	LabelGen				m_LabelGen;					//!< Label number generator
	TempGen					m_TempGen;					//!< Compiler generated temporaries generator
	uint32					m_ConstOffs;				//!< Offset of the next string constant to be allocated in the constants pool
	ProtocolInfo			*m_ProtocolInfo;
	uint32					m_NumProto;
	MemPool<CodeList>		m_MemPool;
	ProtosTables_t			m_ProtosTables;				//!< Lookup tables used by protocols
	TablesProtos_t			m_TablesProtos;				//!< Protocols which uses lookup tables	
	set<string>				m_UsefulTables;				//!< Tables associated with usuful protocols

public:
	SymbolVarInt			*CurrentOffsSym;
	SymbolVarBufRef		*PacketBuffSym;

	/*!
	\brief	Object constructor
	\param	globalInfo reference to an existing \ref GlobalInfo structure holding shared information
	\return nothing
	*/

	GlobalSymbols(GlobalInfo &globalInfo);

	/*!
	\brief	Object destructor
	*/
	~GlobalSymbols();

	//!\todo these should be private!


	//[mligios]
	void DeleteFieldsRefs(SymbolProto *protoSym);
//	void DeleteFieldsRef(SymbolProto *protoSym, uint32 from, uint32 to);
//	void DeleteFieldRef(SymbolProto *protoSym, SymbolField *fieldSym);
	void PrintGlobalInfo(SymbolProto *protoSym, SymbolField *fieldSym);
	//END


	SymbolField *StoreProtoField(SymbolProto *protoSym, SymbolField *fieldSym);
	SymbolField *StoreProtoField(SymbolProto *protoSym, SymbolField *fieldSym, bool force);
	void StoreLabelSym(const uint32 label, SymbolLabel *labelSymbol);
	void StoreVarSym(const string name, SymbolVariable *varSymbol);
	void StoreTempSym(const uint32 temp, SymbolTemp *tempSymbol);
	void StoreConstSym(const uint32 value, SymbolIntConst *constSymbol);
	void StoreConstSym(const string value, SymbolStrConst *constSymbol);
	
	void StoreLookupTable(const string name, SymbolLookupTable *lookupTable);
	void AssociateProtoTable(SymbolProto *protocol,const string name);
	void AssociateTableProto(const string name,SymbolProto *protocol);
	set<string> GetAssociationProtosTable(SymbolProto *protocol);
	set<SymbolProto*> GetAssociationTableProtos(string name);
	void RemoveProtoTable(SymbolProto *protocol, const string name);
	bool IsLookUpTableUseful(const string name); 
	int GetUsefulTablesNumber();

	void StoreRegExEntry(SymbolRegEx *regEx, SymbolProto *protocol);
	void StoreStringMatchingEntry(SymbolRegEx *regEx, SymbolProto *protocol);
	void StoreRegExEntryUseful(SymbolRegEx *regEx);
	void StoreStringMatchingEntryUseful(SymbolRegEx *regEx);
	void StoreUsefulTable(string table);
	int GetRegExEntriesCount();
	int GetStringMatchingEntriesCount();
	int GetRegExEntriesCountUseful();
	int GetStringMatchingEntriesCountUseful();


	SymbolProto *LookUpProto(const string protoName);
	SymbolProto *GetProto(int index);
	SymbolField *LookUpProtoField(const string protoName, const SymbolField *field);
	SymbolField *LookUpProtoField(SymbolProto *protoSym, const SymbolField *field);
	SymbolField *LookUpProtoFieldById(const string protoName, uint32 index);
	SymbolField *LookUpProtoFieldById(SymbolProto *protoSym, uint32 index);
	SymbolField *LookUpProtoFieldByName(const string protoName, const string fieldName);
	SymbolField *LookUpProtoFieldByName(SymbolProto *protoSym, const string fieldName);
	SymbolVariable *LookUpVariable(const string name);
	SymbolIntConst *LookUpConst(const uint32 value);
	SymbolStrConst *LookUpConst(const string value);
	SymbolLabel *LookUpLabel(const uint32 label);
	SymbolLookupTable *LookUpLookupTable(const string name);
	
	void DumpProtoSymbols(bool fields);

	bool CompareSymbolFields(const SymbolField *sym1, const SymbolField *sym2);

	SymbolLabel *GetProtoStartLbl(SymbolProto *protoSym);
	void SetProtoStartLbl(SymbolProto *protoSym, SymbolLabel *label);
	SymbolVarInt *GetProtoOffsVar(SymbolProto *protoSym);
	void SetProtoOffsVar(SymbolProto *protoSym, SymbolVarInt *var);
	CodeList &GetProtoCode(SymbolProto *protoSym);
	//FldScopeStack_t &GetProtoFieldScopes(SymbolProto *protoSym);
	void AddFieldExtractStatment(SymbolProto *protoSym, StmtFieldInfo *stmt);
	CodeList &GetFieldExtractCode(SymbolProto *protoSym);

	uint32 IncConstOffs(uint32 delta);

	void UnlinkProtoLabels(void);

	CodeList *NewCodeList(bool ownsStmts = true);

	uint32 GetNewLabel(uint32 n)
	{
		return m_LabelGen.GetNew(n);
	}

	uint32 GetNewTemp(uint32 n)
	{
		return m_TempGen.GetNew(n);
	}

	GlobalInfo &GetGlobalInfo(void)
	{
		return m_GlobalInfo;
	}

	LookupTablesList_t GetLookupTablesList()
	{
		LookupTablesList_t list(0);
		LookupTablesTable_t::Iterator i = this->m_LookupTables.Begin();
		while (i != this->m_LookupTables.End())
		{
			list.push_back((*i).Item);
			i++;
		}

		return list;
	}
	
	void SetRegExList(RegExList_t list)
	{
		m_RegExEntries = list;
	}

	RegExList_t GetRegExList()
	{
		return this->m_RegExEntries;
	}
	
	ProtoList_t GetRegExProtoList()
	{
		return this->m_RegExProtos;
	}
	
	RegExList_t GetRegExListUseful()
	{
		return this->m_RegExEntriesUseful;
	}
	
	void SetRegExProtoList(ProtoList_t list)
	{
		m_RegExProtos = list;
	}
	
	void SetStringMatchingList(RegExList_t list)
	{
		m_StringMatchingEntries = list;
	}
	
	RegExList_t GetStringMatchingList()
	{
		return this->m_StringMatchingEntries;
	}
	
	RegExList_t GetStringMatchingListUseful()
	{
		return this->m_StringMatchEntriesUseful;
	}
	
	ProtoList_t GetStringMatchingProtoList()
	{
		return this->m_StringMatchingProtos;
	}
	
	void SetStringMatchingProtoList(ProtoList_t list)
	{
		m_StringMatchingProtos = list;
	}

	void StoreDataItem(SymbolDataItem *item);

	DataItemList_t *GetDataItems();

	void DumpProtoFieldStats(std::ostream &stream);

	uint32 GetNumField(SymbolProto *proto)
	{
		return m_ProtocolInfo[proto->ID].m_NumField;
	}

	SymbolField ** GetFieldList(SymbolProto *proto)
	{
		return m_ProtocolInfo[proto->ID].m_FieldList;
	}
};

