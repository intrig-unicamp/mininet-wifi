/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

#include "defs.h"
#include "encapgraph.h"
#include "pflexpression.h"
#include "errors.h"
#include "sft/sft.hpp"
#include "sft/sft_writer.hpp"
#include "strutils.h"
#include <map>
#include "filtersubgraph.h"
#include "../nbee/globals/globals.h"

using namespace std;

class State;
class Transition;
class InnerExpressionNode;

typedef pair<SymbolProto*, SymbolProto*> EncapLabel_t;

class EncapLabelLess
{
	bool operator()(EncapLabel_t &a, EncapLabel_t &b)
	{
		if (a.first->ID < b.first->ID)
			return true;
		if (a.second->ID < b.second->ID)
			return true;
		return false;
	}
};

template <>
struct STraits<SymbolProto*>
{
	static SymbolProto* null()
	{
		return NULL;
	}


	static SymbolProto* Union(SymbolProto* s1, SymbolProto* s2)
	{
		return s1;
	}


	static std::string ToString(SymbolProto *s_info)
	{
		if (s_info != NULL)
		{
			std::string s;
			StringSplitter strSplit(s_info->ToString(), 10);
			while (strSplit.HasMoreTokens())
			{
				s += strSplit.NextToken();
				if (strSplit.HasMoreTokens())
					s += string("\\n");
			}
			return s;
		}
		return string("");
	}
};


template <>
struct LTraits<EncapLabel_t>
{
	static std::string ToString(EncapLabel_t &l_info)
	{
		std::string from(left(l_info.first->ToString(), 4));
		std::string to(left(l_info.second->ToString(), 4));
		return from + std::string("_") + to;
	}
};

template <>
struct PTraits<PFLTermExpression*>
{
	static PFLTermExpression* One()
	{
		return new PFLTermExpression(true);
	}
	static PFLTermExpression* Zero()
	{
		return new PFLTermExpression(false);
	}

	static PFLExpression* AndOp(PFLExpression* a, PFLExpression* b)
	{
          // optimizations
          if (a->ToString() == "true") return b;
          if (b->ToString() == "true") return a;

          return new PFLBinaryExpression(a, b, BINOP_BOOLAND);
	}

	static PFLExpression* OrOp(PFLExpression* a, PFLExpression* b)
	{
		return new PFLBinaryExpression(a, b, BINOP_BOOLOR);
	}

	static PFLExpression* NotOp(PFLExpression* a)
	{
		return new PFLUnaryExpression(a, UNOP_BOOLNOT);
	}


	static std::string ToString(PFLExpression* p_info)
	{
		assert(p_info != NULL);
		return p_info->ToString();
	}

	static bool DumpNeeded(PFLExpression* p_info)
	{
		if (p_info->IsTerm())
		{
			PFLTermExpression *termExp = (PFLTermExpression*)p_info;
			if (termExp->IsConst())
			{
				//the const expression is needed only if its value is false (useful for debugging purposes)
				if (!termExp->GetValue())
					return true;

				return false;
			}
			if (termExp->GetIRExpr() == NULL)
				return false;
		}
		return true;

	}


};

// These typedef require the previous traits to be already defined
typedef sftFsa<SymbolProto *, EncapLabel_t, PFLTermExpression*> EncapFSA_t;
typedef EncapFSA_t::Alphabet_t Alphabet_t;
typedef EncapFSA_t::trans_pair_t trans_pair_t;


class Code
{
	/*
	*	class developed by Ivano Cerrato 07/05/2012
	*/
	
	string 		m_Code;
	SymbolTemp *m_Variable;
	bool		m_Check;	//if it is true, variable could be checked on an extended transition

public:
	Code(string code, SymbolTemp *variable, bool check)	
		:m_Code(code), m_Variable(variable), m_Check(check) {}
		
	bool Check()
	{
		return m_Check;
	}	
	
	void SetCheck()
	{
		m_Check = true;
	}
		
	string GetCode()
	{
		return m_Code;
	}	

	void SetVariable(SymbolTemp *variable)
	{
		nbASSERT(variable != NULL, "You cannot add NULL to the code");
		nbASSERT(m_Variable == NULL, "A variable has already been assigned. Remove it before add a new one");
		m_Variable = variable;
	}
	
	void ResetVariable()
	{
		m_Variable = NULL;
	}	
	
	SymbolTemp *GetVariable()
	{
		return m_Variable;
	}

};

typedef enum {FIRST,SECOND} instructionNumber;


/*!
 * \brief This class inherits from the sftFsa<> template and represents a concrete Filter FSA
 */

class EncapFSABuilder;

class EncapFSA: public EncapFSA_t
{
	friend class EncapFSABuilder;

private:
  static void migrateTransitionsAndStates(EncapFSA *to, EncapFSA *from, StateIterator init);
  static EncapGraph *m_ProtocolGraph;

  //Methods to manage the code associated to the symbols
  void AddCode(SymbolProto *protocol,string code, SymbolTemp *variable, bool check, instructionNumber number);
  bool AlreadyInserted(list<Code*> lCode, string code, bool check);
  string GetCodeToCheck(SymbolProto *protocol, instructionNumber number);
  void MergeCode(EncapFSA *firstFSA,EncapFSA *secondFSA);
  void MergeCode(EncapFSA *originFSA);
  void AddVariable(SymbolProto *protocol, SymbolTemp *var, instructionNumber number);
  SymbolTemp *GetVariableToCheck(SymbolProto *protocol, instructionNumber number);
  SymbolTemp* GetVariableFromCode(string code, instructionNumber number);

public:

	EncapFSA(Alphabet_t &alphabet)
		:EncapFSA_t(alphabet){}

	~EncapFSA()
	{
		//delete all the PFLExpression object associated to transitions
		EncapFSA_t::TransIterator j = FirstTrans();
		for (; j != LastTrans(); j++)
		{
			trans_pair_t transInfo = (*j).GetInfo();
			delete transInfo.second;
		}
	}
	
	//Methods to manage the code associated to the symbols
	void AddCode1(EncapLabel_t label,string code, bool check = false, SymbolTemp *variable = NULL);
	void AddCode2(EncapLabel_t label,string code, bool check = false, SymbolTemp *variable = NULL);
	string GetCodeToCheck1(EncapLabel_t);
	string GetCodeToCheck2(EncapLabel_t);
	bool HasCode(EncapLabel_t l);	
	list<SymbolTemp*> GetVariables(EncapLabel_t);
	SymbolTemp* GetVariableToCheck1(EncapLabel_t);
	SymbolTemp* GetVariableToCheck2(EncapLabel_t);
	SymbolTemp* GetVariableFromCode1(string code);
	SymbolTemp* GetVariableFromCode2(string code);
	
	void ResetFinals();
	void FixTransitions();
			
	//this static method set the encapsulation graph. if you call it two times, it is an error
	static void SetGraph(EncapGraph *graph)
	{
		nbASSERT(m_ProtocolGraph==NULL,"you can use this method only a time!");
		m_ProtocolGraph = graph;
	}

	StateIterator GetStateIterator(State *s);
	TransIterator GetTransIterator(Transition *t);

	void BooleanNot();
	static EncapFSA* BooleanAND(EncapFSA *fsa1, EncapFSA *fsa2);
	static EncapFSA* BooleanOR(EncapFSA *fsa1, EncapFSA *fsa2, bool setFinals = true);
	static EncapFSA* NFAtoDFA(EncapFSA *orig, bool setInfo = true);
    void fixStateProtocols();
    void setFinalStates();
    void fixActionStates();

    static void prettyDotPrint(EncapFSA *fsa, char *description, char *dotfilename, char *src_file=NULL, int src_line=-1);
    static void printDotLegend(char *src_file, int src_line);
 
        // use the following macro in most cases
#ifdef ENABLE_PFLFRONTEND_DBGFILES
#define PRINT_DOT(fsa,desc,filename) do {                               \
          EncapFSA::prettyDotPrint((fsa),(desc),(filename),__FILE__,__LINE__); \
        } while (0) 
#define PRINT_DOT_LEGEND() do {                               \
          EncapFSA::printDotLegend(__FILE__,__LINE__); \
        } while (0) 

#else
#define PRINT_DOT(fsa,desc,filename)
#define PRINT_DOT_LEGEND()

#endif /* #ifdef ENABLE_PFLFRONTEND_DBGFILES */

	EncapFSA::StateIterator GetStateIterator(EncapFSA::State &state);
	
	static bool ThereIsSelfLoop(list<EncapFSA::Transition*> transition);
	
};

typedef EncapFSA::StateIterator EncapStIterator_t;
typedef EncapFSA::TransIterator EncapTrIterator_t;



/*!
	\brief
*/

typedef enum {ACCEPTING_STATE,ACTION_STATE} status_t;

class EncapFSABuilder
{
	typedef map<EncapGraph::GraphNode*, EncapStIterator_t> NodeMap_t;
	EncapGraph					    &m_Graph;
	EncapFSA::Alphabet_t			&m_Alphabet;
	NodeMap_t					    m_NodeMap;
	int						      	m_Status; 			//it can be nbSUCCESS, nbFAILURE or nbWARNING
	ErrorRecorder 					m_ErrorRecorder; 	//needed to store a message error
#ifdef ENABLE_FIELD_OPT	
	set<pair<string,string> >		predicatesOnFields;	//This set contains all the fields used into the NetPFL filter (except the extraction)
														//Hence, if a field is in this set, it cannot be merged with others in order to reduce the generated MIR code
#endif	
	
	
    /* This method adds a new state to the provided 'fsa' and
     * fills it with 'origNode'. A pointer to the new state is
     * returned.
     *
     * If 'force' is false (default behaviour) and another state
     * with the same 'origNode' is already present, the insertion
     * is skipped and a pointer to the old node is returned.
     *
     * If 'force' is true, the insertion is executed in any case,
     * and a pointer to the inserted node is returned, but not
     * stored internally for the next returns (unless this is the
     * first call for the current 'origNode').
     */
	EncapStIterator_t AddState(EncapFSA &fsa, EncapGraph::GraphNode &origNode, bool force = false); //FIXME: force è necessario?
	
	bool StartAsFirst(list<PFLSetExpression*>* innerList);
	EncapFSA::Alphabet_t CompleteAlphabet(set<SymbolProto*> *allProtos);
  	bool AssignProtocol(EncapFSA* fsa,EncapFSA::State *finalNotAccept);
	void CutOutgoingTransitions(EncapFSA *fsa,EncapFSA::State *state);
	void CutIncomingTransitions(EncapFSA *fsa, EncapFSA::State *state, EncapFSA::State *fail);
	bool RemoveUselessStates(EncapFSA* fsa,EncapFSA::State *fail);
	bool ExpandStates(EncapFSA* fsa,EncapFSA::State *finalNotAccept);
	void ReplaceTransitions(EncapFSA *fsa,map<EncapLabel_t,EncapFSA::State*> labelToState,map<int,map<EncapLabel_t,EncapFSA::State*> > labelFromState,Alphabet_t onSelfLoop,map<SymbolProto*,EncapFSA::State*> newStates);
	void LinkToNotAccept(EncapFSA *fsa,EncapFSA::State *finalNotAccept);
	void CreateNoMatchAutomaton(EncapFSA *fsa);
	bool FindTraps(EncapFSA *fsa,EncapFSA::State *finalNotAccept, status_t status);	
	EncapFSA::State *CreateLink(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp, bool mandatoryTunnels);
	EncapFSA::Alphabet_t CreateAlphabet(set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::Alphabet_t completeAlphabet);
	void CreateNewStateIfNeeded(EncapFSA *fsa, map<SymbolProto*,EncapFSA::State*> *newStates, EncapLabel_t label);
	EncapFSA::State *ManageDefaultROP(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels);
	EncapFSA::State *ManagePLUS(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels);
	EncapFSA::State *ManageQUESTION(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels);
	EncapFSA::State *ManageSTAR(EncapFSA *fsa,set<SymbolProto*>previousProtos,set<SymbolProto*>currentProtos,EncapFSA::State *previousState,PFLRepeatOperator repeatOp,EncapFSA::Alphabet_t completeAlphabet,set<pair<set<SymbolProto*>,PFLExpression*> > currentPredicates,PFLInclusionOperator inclusionOp,int predSize, bool mandatoryTunnels);
	EncapFSA *AnalyzeExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label);
	EncapFSA *AnalyzeTerminalExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label);
	EncapFSA *AnalyzeBinaryExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label);
	EncapFSA *AnalyzeUnaryExpression(SymbolProto *prevProto, SymbolProto *newProto, PFLExpression *expr, EncapLabel_t label);
	void MoveET(EncapFSA::State *previousState,EncapFSA::State *newState,EncapFSA::State *failState, EncapFSA *regExp, EncapFSA *predicates, EncapLabel_t label);
	EncapFSA *ManagePredicatesOnCounters(SymbolProto *prevProto, SymbolProto *newProto,EncapLabel_t label,EncapFSA *predicates,int headerIndex,RangeOperator_t op,string code);
	
public:
	EncapFSABuilder(EncapGraph &protograph, EncapFSA::Alphabet_t &alphabet);
	
	EncapFSA *BuildRegExpFSA(list<PFLSetExpression*>* innerList, status_t status = ACCEPTING_STATE);

	void FixAutomaton(EncapFSA *fsa, status_t status = ACCEPTING_STATE);
			
	EncapFSA *BuildSingleNodeFSA(); 

	/* This method is part of this class and not of EncapFSA in order to reuse code already written */
	void ReduceAutomaton(EncapFSA *fsa);

#ifdef ENABLE_FIELD_OPT
	set<pair<string,string> > GetPredicatesOnFields()
	{
		return predicatesOnFields;
	}
#endif
	
	int GetStatus() 
	{
		return m_Status;
	}
	
	ErrorRecorder &GetErrRecorder(void)
	{
		return m_ErrorRecorder;
	}
};

