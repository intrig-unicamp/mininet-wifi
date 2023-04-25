/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifdef ENABLE_FIELD_OPT

//	#define ENABLE_FIELD_DEBUG  //Debug Info about Field & Loop Optimization!  ->Stats about optimizations


	#ifdef ENABLE_FIELD_DEBUG
		#define ACTIVEDEBUG		//Debug files about fields informations
	#endif

#pragma once

#include "statements.h"
#include "globalsymbols.h"
#include "pdlparser.h"
#include <iostream>


//Uncomment the next row to generate debug files for the algorithm to compact fields
//#define DEBUG_COMPACT_FIELDS

using namespace std;

struct mylist{
	StmtWhile *stmt;			//Pointer to Uncompatted (yet) Loop-Statement
	IntMap localmap;			//Local Used-Fields-Map referred to this Loop-Statement
   // Assignment operator.
   bool operator ==(const mylist& st)
   {
      return stmt == st.stmt;  			// Check only stmt pointer (Only one struct for each Uncompatted-Loop)
   }

};

typedef list<mylist> LoopList;

class CompField
{
private:
#ifdef DEBUG_COMPACT_FIELDS
/*********************	  Print (DEBUG)		****************************/

	void PrintWhile(StmtWhile* stmt, string fname);
	void Printblocks(StmtBlock* stmt,string fname);
	void PrintStmt(StmtBase* stmt, string fname);
	void PrintNode(Node* node, string fname,uint32 childnumber);
	void PrintSymbol(Symbol *Symb, string fname);
	void PrintContainer(SymbolFieldContainer *Container, string fname);
	void PrintSymbolField(SymbolField *Sym, string fname);
	void PrintSymbolVariable(SymbolVariable *Sym,string fname);
	void print_values(StmtBase *stmt,string fname);
	void PrintFieldsMap(IntMap *mymap, string fname);
	
	
#endif

protected:
	GlobalSymbols		&m_GlobalSymbols;	
	IntMap 			&m_symbolref;		//MAP contain all Used-FIELDS (filled in PdlParser)!!
	IntMap 			m_symboltemp;		//MAP contain all the fields used in expressions inside the local-LOOP
	IntMap			m_DefUseLocal;		//MAP contain all the Defined used fields inside the local-LOOP


	LoopList 		UncompattableLoops;	//This List contain LoopStmt* && the Map of all Used-Field inside this Loop (These Loops can be transformed (don't know yet) )
	LoopList 		NeverLoops;		//These Loops Can't BE TRANSFORMED! (This List contain LoopStmt* && the Map of all Used-Field inside this Loop)

	bool 			m_compatting;		//flag used for recognize which fields can be compatted	
	uint32 			m_longsize;		//Size of the new field (FIXED-COMPATTED)
	string	 		m_longname;		//Name of the new field (FIXED-COMPATTED)

	StmtBase		*m_actualstmt;          //pointer to the current statement
	StmtBase 		*m_startstmt;		//pointer to the first compattable field
	StmtBase		*m_stopstmt;            //pointer to the last compattable field

	CodeList		*m_headcode;		//pointer to the first statement!

	bool			m_loopcomp;		//Flag useful for identify which loop can compact
	uint32 			m_LoopFldCount;		//counter for new Variable-Loop-Fields

	SymbolProto 		*m_CurrProtoSym;	//pointer to the current protocol

	uint32 			checkingloop;		//This number identify  which inner-level-loop is..


	bool			toBeUpdated;
	ofstream 		*fp;

/*********************	  Visit CODE	(protected)	****************/

	void VisitSymbol(Symbol *sym);
	void VisitOpcode(uint16 opcode);
	void VisitComment(StmtComment *stmt);
	void VisitBitField(SymbolFieldBitField *bitField, uint32 size);
	void VisitCase(StmtCase *stmt, bool isDefault = false);
	void VisitSwitch(StmtSwitch *stmt);
	void VisitJump(StmtJump *stmt);
	void VisitIf(StmtIf *stmt);
	void VisitLoop(StmtLoop *stmt);
	void VisitDoWhile(StmtWhile *stmt);
	void VisitWhileDo(StmtWhile *stmt);
	void VisitBreak(StmtCtrl *stmt);
	void VisitContinue(StmtCtrl *stmt);
	void VisitBlock(StmtBlock *stmt);
	void VisitVarDetails(Node *node);
	void VisitInnerFields(SymbolFieldContainer *parent);
	void VisitFieldDetails(Node *node);

/*********************	  Compact-Fields	**************************/

	void CompactFields();						//Compact (Compattable)-Fields sequence
	void StartCheck(SymbolField *field);				//Check if can start -> CompactFields

/*********************	  Compact-Loops		**************************/

	void TransformLoop(StmtWhile* stmt);				//Transform Loop to Variable-Field with same expression (-> same length) 
	bool AllCompattable(StmtWhile *stmt);				//Verify if all the elements inside this Loop can be erased (Calling TransformLoop)

	void TransformRemainedLoop();					//Transform All Loop-Size remain in UncompattableLoops LIST (Called every "Protocol-End" (destructor))

/*********************	Fill Loop-List ***********************************/

	void CheckUncompattedLoop(SymbolField *field);			//Used to set "NeverCompattable" (adding to the Never-List) all the Uncompatted Loop that contain Fields used Outside LOOPS
	void AddUncompattedLoop(StmtWhile *stmt,IntMap *fields);	//Add this Map-of-Fields && Stmt Pointer To Uncompatted-List
	void AddNeverCompattableLoop(StmtWhile *stmt,IntMap *fields);	//Add this Map-of-Fields && Stmt Pointer To Never-Compattable-List
	
/*********************** Handle Hash-Map **********************************/

	void UpdateMapField(StmtWhile *stmt);			//Used to Extract Used-Fields from Loop Expression && save them to the Local-Map
	void AddFieldToMap(SymbolField *field,IntMap *mymap);	//Used To ADD this Used-field to the Local-Map

	void UnsetUsedField(StmtWhile *stmt);			//Unset Used-Fields contained in this Transformed-Loop 
	void CleanTempMap(IntMap *mymap);			//Erase Local-Fields-Map (actually set = NULL)

/*****************  Extract Fields from Expression	 *******************/

	void ExtractFieldsFromExpr(Node *expr, IntMap *mymap);	//Used To ADD all the Used-fields contained in this Expression to the Local-Map
	Node *Findexpr(Node *sub);				//extract sub-tree represent expression-Tree from GenStatement (Extract Expression from Statement LOOP)

/*********************	  Miscellaneuos		****************************/

	SymbolProto *GetCurrentProto();					//Get Current Protocol
	void ChangeComments(StmtBase *stmt,string comment);		//Modify Comment attribute of this Statement

/*********************	  free memory		****************************/

	void destroy_loop(StmtWhile *);			//Destroy Loop-Statement && All Sub-Nodes
	void destroy_stmt(StmtBase *stmt);		//Destroy This-Statement && All Sub-Nodes
	void destroy_nodes(Node *node);			//Destroy This-Node && All Sub-Nodes

/******************     Modify global information  *************************/

	void SetAsRemovable(IntMap *mymap);		//Set All the fields as Removable!!

			//FIXME
	void DeleteReferenceStmt(StmtWhile *stmt);
	void DeleteReferenceNodes(Node *node);

/*********************	  Debug Or Deprecated	****************************/

	bool isvalid(StmtBase *stmt);				    //Check if this Statement can be recognized (Valid Statement)	

/***************************************************************************/

public:
	CompField(GlobalSymbols &m_GlobalSym, IntMap &m_SymbolRef, ofstream *my_ir = NULL)
		:m_GlobalSymbols(m_GlobalSym),m_symbolref(m_SymbolRef), m_compatting(0),m_longsize(0),m_longname(""),m_actualstmt(NULL),m_startstmt(NULL),m_stopstmt(NULL),m_headcode(NULL),m_loopcomp(0),m_LoopFldCount(0),checkingloop(0),toBeUpdated(false)
#ifdef ENABLE_FIELD_DEBUG
,fp(my_ir)
#endif
{ 
#ifdef DEBUG_COMPACT_FIELDS
	PrintFieldsMap(&m_symbolref,"Debug_ALL_Fields_USED.txt");
#endif
}

/*********************	  Visit	(Public)	*****************************/

	void VisitCode(CodeList *code);
	void VisitTree(Node *node);
	void VisitStatement(StmtBase *stmt);

/****************************************************************************/

	~CompField() {
		TransformRemainedLoop();

		if(toBeUpdated){	
#ifdef ENABLE_FIELD_DEBUG
			(*fp) <<" Protocol(Before Loop & Field Optimization):"<<(m_CurrProtoSym)->Name;
			(*fp) <<" Number of fields: "<<m_GlobalSymbols.GetNumField(m_CurrProtoSym)<<endl;
#endif
			m_GlobalSymbols.DeleteFieldsRefs(m_CurrProtoSym);	
#ifdef ENABLE_FIELD_DEBUG
			(*fp) <<" Protocol(After Loop & Field Optimization): "<<(m_CurrProtoSym)->Name;
			(*fp) <<" Number of fields: "<<m_GlobalSymbols.GetNumField(m_CurrProtoSym)<<endl;
			(*fp) <<"****************************************************************************"<<endl;		
#endif
		}
		else 
		{				
#ifdef ENABLE_FIELD_DEBUG
			if(m_CurrProtoSym->ID!=0)		//ID==0 -> CRASH
			{
				(*fp) <<" Protocol(Not Optimized):"<<(m_CurrProtoSym)->Name<<" (Already Done OR Not needed)"<<endl;
				(*fp) <<"****************************************************************************"<<endl;	
			}
#endif
		}

	}
};

#endif

