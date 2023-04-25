/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "symbols.h"
#include "encapgraph.h"
#include "ircodegen.h"
#include <nbprotodb.h>
#include "globalsymbols.h"
#include "compilerconsts.h"
#include "filtersubgraph.h"
#include "errors.h"
#include "pflexpression.h"
#include "encapfsa.h"



#include <utility>

using namespace std;


// use this define to visit or not next-proto candidate instructions
#define VISIT_CANDIDATE_PROTO
// use this define to consider not supported protocols as next proto
//#define VISIT_NEXT_PROTO_DEFINED_ONLY
// use this define to execute the 'before' code as soon as you find a protocol
#define EXEC_BEFORE_AS_SOON_AS_FOUND

typedef std::map<SymbolField *,int> IntMap;

enum
{
	PARSER_VISIT_ENCAP = true,
	PARSER_NO_VISIT_ENCAP = false
};

typedef struct _JumpInfo
{
	PFLExpression *expr;
	SymbolLabel *jumpTrueLabel;
	SymbolLabel *jumpFalseLabel;
}JumpInfo;


/* Use this string to label the "last resort" jump, that is those
 * jumps derived by a complemented-set transition, if the only one
 * with a complemented set for a certain protocol. (i.e. if X goto
 * tcp, otherwise FAIL. 'otherwise' is what I call a "last resort"
 * jump).
 */
#define LAST_RESORT_JUMP "others"

/*!
\brief
*/

class PDLParser
{
private:



	struct FilterInfo
	{
		SymbolLabel			*FilterFalse;
		SymbolLabel			*FilterTrue;
		EncapFSA 			&fsa;

		FilterInfo(SymbolLabel *filterFalse, SymbolLabel *filterTrue, EncapFSA &fsa)
			:FilterFalse(filterFalse), FilterTrue(filterTrue), fsa(fsa){}
	};


	typedef list<HIRCodeGen*> CodeGenStack_t;

	struct SwitchCaseLess : public std::binary_function<_nbNetPDLElementCase *, _nbNetPDLElementCase *, bool>
	{
		bool operator()(_nbNetPDLElementCase * left, _nbNetPDLElementCase * right) const
		{
			if(left->ValueNumber < right->ValueNumber)
				return true;
			else
				return false;

		};
	};

	_nbNetPDLDatabase		*m_ProtoDB;			//!<Pointer to the NetPDL protocol database
	GlobalSymbols			&m_GlobalSymbols;
	ErrorRecorder			&m_ErrorRecorder;
	//FldScopeStack_t		*m_FieldScopes;
	HIRCodeGen				*m_CodeGen;
	SymbolProto				*m_CurrProtoSym;
	uint32					m_LoopCount;
	uint32					m_IfCount;
	uint32					m_SwCount;
	//	list<LoopInfo>			m_LoopStack;
	bool					m_IsEncapsulation;
	bool 					m_Preferred; 			//true if we are using the preferred peg while parsing the encapsulation section
	SymbolFieldContainer	*m_ParentUnionField;	// [ds] parsing a field requires to save the current field and try parsing inner fields
	uint32					m_BitFldCount;
	uint32					m_ScopeLevel;
	FilterInfo				*m_FilterInfo;
	CompilerConsts			*m_CompilerConsts;
	EncapGraph				*m_ProtoGraph;
	EncapGraph				*m_ProtoGraphPreferred;
	GlobalInfo				*m_GlobalInfo;

	//ParsingSection			m_CurrentParsingSection;

	SymbolVarInt			*m_nextProtoCandidate;	//!< ID of the next candidate resolved
	SymbolLabel				*m_ResolveCandidateLabel;
	list<int>				m_CandidateProtoToResolve;
	std::map <string, SymbolLabel*> m_toStateProtoMap;

	bool						m_ConvertingToInt;

	CodeGenStack_t			m_CodeGenStack;

	bool						m_InnerElementsNotDefined;

	void ParseFieldDef(_nbNetPDLElementFieldBase *fieldElement);
	void ParseAssign(_nbNetPDLElementAssignVariable *assignElement);
	void ParseLoop(_nbNetPDLElementLoop *loopElement);
	HIRNode *ParseExpressionInt(_nbNetPDLExprBase *expr);
	HIRNode *ParseExpressionStr(_nbNetPDLExprBase *expr);
	HIRNode *ParseOperandStr(_nbNetPDLExprBase *expr);
	HIRNode *ParseOperandInt(_nbNetPDLExprBase *expr);
	void ParseRTVarDecl(_nbNetPDLElementVariable *variable);
	void ParseElement(_nbNetPDLElementBase *element);
	void ParseElements(_nbNetPDLElementBase *firstElement);
	void ParseEncapsulation(_nbNetPDLElementBase *firstElement);
	void ParseExecSection(_nbNetPDLElementExecuteX *execSection);
	//	void EnterLoop(SymbolLabel *start, SymbolLabel *exit);
	//	void ExitLoop(void);
	void EnterSwitch(SymbolLabel *next);
	void ExitSwitch(void);
	//	LoopInfo &GetLoopInfo(void);
	void ParseLoopCtrl(_nbNetPDLElementLoopCtrl *loopCtrlElement);
	void ParseIf(_nbNetPDLElementIf *ifElement);
	void ParseVerify(_nbNetPDLElementExecuteX *verifySection);
	void ParseSwitch(_nbNetPDLElementSwitch *switchElement);
	void ParseSwitchOnStrings(_nbNetPDLElementSwitch *switchElement);		//Ivano
	void ParseCaseOnStrings(_nbNetPDLElementCase *caseElement, _nbNetPDLElementSwitch *switchElement);				//Ivano
	void ParseBoolExpression(_nbNetPDLExprBase *expr);
	void ParseNextProto(_nbNetPDLElementNextProto *element);
	void ParseNextProtoCandidate(_nbNetPDLElementNextProto *element);

	void ParseLookupTable(_nbNetPDLElementLookupTable *table);
	void ParseLookupTableUpdate(_nbNetPDLElementUpdateLookupTable *element);
	HIRNode *ParseLookupTableSelect(HIROpcodes op, string tableName, SymbolLookupTableKeysList *keys);
	
	//[icerrato] this method use the opcode PUSH for an HIR statement, but PUSH is a MIR one
	//void ParseLookupTableAssign(_nbNetPDLElementAssignLookupTable *element);
	
	HIRNode *ParseLookupTableItem(string tableName, string itemName, HIRNode *offsetStart, HIRNode *offsetSize);

	void CheckVerifyResult(uint32 nextProtoID);

	bool CheckAllowedElement(_nbNetPDLElementBase *element);
	//void StoreFieldInScope(const string name, SymbolField *fieldSymbol);
	//FieldsList_t *LookUpFieldInScope(const string name);
	SymbolField *StoreFieldSym(SymbolField *fieldSymbol);
	SymbolField *StoreFieldSym(SymbolField *fieldSymbol, bool force);
	SymbolField *LookUpFieldSym(const string name);
	SymbolField *LookUpFieldSym(const SymbolField *field);
	//void EnterFldScope(void);
	//void ExitFldScope(void);
	char *GetProtoName(struct _nbNetPDLExprBase *expr);
	bool IsPreferred(struct _nbNetPDLExprBase *expr); //[icerrato]

	void ChangeCodeGen(HIRCodeGen &codeGen);
	void RestoreCodeGen(void);

	void GenProtoEntryCode(SymbolProto &protoSymbol);


	void VisitNextProto(_nbNetPDLElementNextProto *element);
	void VisitElement(_nbNetPDLElementBase *element);
	void VisitElements(_nbNetPDLElementBase *firstElement);
	void VisitEncapsulation(_nbNetPDLElementBase *firstElement);
	void VisitSwitch(_nbNetPDLElementSwitch *switchElem);
	void VisitIf(_nbNetPDLElementIf *ifElement);

	//void ParseStartProto(HIRCodeGen &IRCodeGen);

	// [ds] added functions to generate debug information, warnings and errors
	void GenerateInfo(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation);
	void GenerateWarning(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation);
	void GenerateError(string message, const char *file, const char *function, int line, int requiredDebugLevel, int indentation);

	//[mligios] new methods


	void VisitLoop(_nbNetPDLElementLoop *loopElement);
	void VisitExpressionInt(_nbNetPDLExprBase *expr);
	void VisitOperandInt(_nbNetPDLExprBase *expr);	
	void VisitExpressionStr(_nbNetPDLExprBase *expr);	
	void VisitOperandStr(struct _nbNetPDLExprBase *expr);

#ifdef ENABLE_FIELD_OPT
	uint32 insideLoop;
#endif	



public:

	void FastVisit(GlobalSymbols &globalSymbols);
	bool m_notyvisit;
#ifdef ENABLE_FIELD_OPT
	IntMap m_symbolref;
	IntMap &getUsedSymbMap();
	void SetFieldInfo(SymbolField *field);
#endif
	
	//constructor
	PDLParser(_nbNetPDLDatabase &protoDB, GlobalSymbols &globalSymbols, ErrorRecorder &errRecorder);



	~PDLParser(void)
	{
	}
	void ParseBeforeSection(_nbNetPDLElementExecuteX *execSection,HIRCodeGen &irCodeGen, SymbolProto *current);
	void ParseStartProto(HIRCodeGen &IRCodeGen, bool visitEncap);
	void ParseProtocol(SymbolProto &protoSymbol, HIRCodeGen &IRCodeGen, bool visitEncap);
	void ParseEncapsulation(SymbolProto *protoSymbol, std::map<string, SymbolLabel*> toStateProtoMap, EncapFSA *fsa, bool defaultBehav, SymbolLabel *filterFalse, SymbolLabel *filterTrue, HIRCodeGen &irCodeGen, bool preferred);
	void ParseExecBefore(SymbolProto &protoSymbol, EncapFSA &fsa, SymbolLabel *filterFalse,  SymbolLabel *filterTrue, HIRCodeGen &IRCodeGen);
	
	ErrorRecorder &GetErrRecorder(void)
	{
		return m_ErrorRecorder;
	}


};
