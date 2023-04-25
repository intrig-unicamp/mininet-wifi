/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifdef ENABLE_FIELD_OPT

#include "tree.h"
#include "symbols.h"
#include "compfields.h"

#include <iomanip>
#include <list>
#include <string.h>
#include <fstream>
#include <iostream>


#define MAX_SYM_STR_SZ 256
#define TREE_NODE	"node%u_%u"
#define STMT_NODE	"stmt%u"
#define TAB_CHAR	"  "

/************************************************************************
 Compact FIELD conditions:
		NEVER USED
		FIXED OR (BIT->not now)
	CONSECUTIVE

 IF INSIDE A BLOCK/CASE/INNERFIELD/LOOP only with ADJACENT fields 

	//[mligios]			REMEMBER INNERFIELDS!!!  m_compatting=0 if previous elements weren't compattable
	//If first element compattable 			   -start!
	//If element compattable after compattable 	   -continue!
	//If element non-compattable after compattable     -stop!
	//If element non-compattable after non-compattable -continue!

Transform LOOP in a Variable Field - conditions:
inside instructions refers to field never used or only used locally! */

/*****************************************************
* 			Main function						  	 								*
*****************************************************/
void CompField::VisitCode(CodeList *code)
{
	m_headcode = code;
	StmtBase *Next = code->Front();
	while(Next)
	{
		if (Next->Flags & STMT_FL_DEAD)
		{
			Next = Next->Next;
			continue;
		}

		m_actualstmt=Next;      //we need to point to the "actual" statement

		VisitStatement(Next);
		Next = Next->Next;
	}
}

/******************************************************
*				for each Statement					  *
******************************************************/

void CompField::VisitStatement(StmtBase *stmt)
{
	nbASSERT(stmt != NULL , "stmt cannot be NULL");

//Start STATEMENT CHECK:
//if this statement it's not GEN -> means that can't be a DEFFLD! -> we must "close" compattable-fixed-field-list!!!!
	if(stmt->Kind!=STMT_GEN)
		StartCheck(NULL);	
//End of STATEMENT CHECK

	switch(stmt->Kind)
	{
	case STMT_GEN:			//here we recognize FIELD DEFINITIONS -> DEFFLD
	{
		#ifdef DEBUG_COMPACT_FIELDS
		   PrintStmt(stmt,"Debug_Statement_Gen.txt");
		#endif
		VisitTree(stmt->Forest);	
	}break;
	case STMT_JUMP:
	case STMT_JUMP_FIELD:
	{
		VisitJump((StmtJump*)stmt);
	}break;
	case STMT_CASE:	  			//Gettin inside a Case! New "Context"
	{
		VisitCase((StmtCase*)stmt, false);
	}break;
	case STMT_SWITCH: 			//Gettin inside a SWITCH! New "Context"
	{
		VisitSwitch((StmtSwitch*)stmt);
	}break;
	case STMT_IF:  				//Gettin inside an IF! New "Context"
	{
		VisitIf((StmtIf*)stmt);
	}break;
	case STMT_LOOP:  			//only <loop type="times2repeat">
	{
		VisitLoop((StmtLoop*)stmt);
	}break;
	case STMT_WHILE:			//LOOP SIZE -> (CONVERTED) -> STATEMENT WHILE -> i'll convert loop in one variable FIELD (if can)
	{
	
		//if this is Loop-Size (converted in pdlparser to stmtWhile!) -> try compatting, otherwise not! only visit
		
		if(((StmtWhile*)stmt)->isLoopSize==true)	
		{
			
			checkingloop++; 	//Integer USED TO IDENTIFY if This LOOP is the first one or it's an Inner-Loop
			m_loopcomp=1;	

			if(checkingloop>1)	//fields used inside LOOP expression-> If InnerLoop ->need to check it - Otherwise not!
				UpdateMapField((StmtWhile*)stmt);	//ADD field-expr to Local-MAP of Used fields


			VisitWhileDo((StmtWhile*)stmt);		//Visit inside this WHILE 
			if(AllCompattable((StmtWhile*)stmt))//check if the validity of these fields end here or if at least one is also referentiated later
			{
			#ifdef DEBUG_COMPACT_FIELDS
				PrintFieldsMap(&m_DefUseLocal,"Debug_List_of_Fields_Used_insideLoop_Compattable.txt");
			#endif
				/****TRANSFORM LOOP IN A VARIABLE-FIELD****/
				TransformLoop((StmtWhile*)stmt);
			}

			CleanTempMap(&m_symboltemp);	//Clean Used Fields of this loop 
			CleanTempMap(&m_DefUseLocal);	//Clean Used Fields Defined inside this loop

			checkingloop--;
		}
		else
		{
				VisitWhileDo((StmtWhile*)stmt);		//Visit inside this WHILE 			
		}

		}break;
	case STMT_DO_WHILE:   	//Gettin inside a WHILE! New "Context"
	{
		VisitDoWhile((StmtWhile*)stmt);
	}break;
	case STMT_BLOCK:  	//Gettin inside a Block! New "Context"
	{
		VisitBlock((StmtBlock*)stmt);
	}break;
	case STMT_BREAK:			
	case STMT_CONTINUE:		//If Break or Continue (Inside Loops) -> Can't Compact this!! Don't know Total Size!! (Offset)
	{
		//TEMP!! THIS loop must be added to NeverLOOP LIST!!?
		//AddNeverCompattableLoop(actualoop,&m_symboltemp);
		m_loopcomp=0;
	}break;
	case STMT_LABEL:
	case STMT_FINFOST:			
	case STMT_COMMENT:			
	{	
		//Do nothing
	}break;
	default:
		nbASSERT(false, "COMPACT_STMT: CANNOT BE HERE!");
		break;
	}

}

/*****************************************************
*				Follow nodes inside the (NODE) tree  *
******************************************************/
void CompField::VisitTree(Node *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");

	if (GET_OP(node->Op) == OP_LABEL)  //More quick!
	{
		return;
	}

	switch(node->Op)
	{
		case IR_DEFFLD:		//Definition Field
		{
			VisitFieldDetails(node->Kids[0]);				
		}break;
		case IR_DEFVARS:
		case IR_DEFVARI:
		case IR_LKADD:
		case IR_LKDEL:
		case IR_LKHIT:
		case IR_LKSEL:
		case IR_REGEXFND:
		case IR_REGEXXTR:
		case IR_LKUPDS: 
		case IR_LKUPDI:
		case IR_ASGNS:
		case IR_ASGNI:
		default:					
		{
			if (node->Kids[0])
			{
				VisitTree(node->Kids[0]);
			}
			if (node->Kids[1])
			{
				VisitTree(node->Kids[1]);
			}
			if (node->Kids[2])			//Really USEFUL??
			{
				VisitTree(node->Kids[2]);
			}
		}break;
	}

}

/******************************************************
*				recognize jumps 					  *
******************************************************/
void CompField::VisitJump(StmtJump *stmt)
{
	nbASSERT(stmt != NULL, "JUMP cannot be NULL");

	if (stmt->FalseBranch != NULL)
		VisitTree(stmt->Forest);

	StartCheck(NULL);  //Jump tree is ended-> close compattable list (start before-Jump, end after-jump)
}

/*****************************************************
*				recognize case	 					 *
*****************************************************/
void CompField::VisitCase(StmtCase *stmt, bool isDefault)
{
	nbASSERT(stmt != NULL, "StmtCase cannot be NULL");

	if (!isDefault)
	{
		VisitTree(stmt->Forest);			//Visit i-case-condition
	}

	StartCheck(NULL);

	if (stmt->Target)
	{
		return;
	}

	VisitCode(stmt->Code->Code);			//Visit code inside this case!!

	StartCheck(NULL);

}

/*****************************************************
*				recognize switch statement			 *
*****************************************************/
void CompField::VisitSwitch(StmtSwitch *swStmt)
{
	nbASSERT(swStmt != NULL, "StmtSwitch cannot be NULL");

#ifdef DEBUG_COMPACT_FIELDS
	PrintNode(swStmt->Forest,"Debug_List_of_Switch_nodes.asm",0);
#endif

	if(checkingloop>0)
		ExtractFieldsFromExpr(swStmt->Forest,&m_symboltemp);	

	VisitTree(swStmt->Forest);				//Visit SWITCH Condition

	VisitCode(swStmt->Cases);				//Visit Cases

	StartCheck(NULL);

	if (swStmt->Default)					
		VisitCase(swStmt->Default, true);		//Visit Default

	StartCheck(NULL);
}

/*****************************************************
*				recognize if statement 				 *
******************************************************/
void CompField::VisitIf(StmtIf *stmt)					//[mligios_check]
{
	nbASSERT(stmt != NULL, "StmtIf cannot be NULL");

	VisitTree(stmt->Forest);				//Visit Condition

#ifdef DEBUG_COMPACT_FIELDS
	PrintNode(stmt->Forest,"Debug_List_of_if_nodes.asm",0);
#endif

	if(checkingloop>0)
		ExtractFieldsFromExpr(stmt->Forest,&m_symboltemp);	

	VisitCode(stmt->TrueBlock->Code);			//Visit TRUE block

	StartCheck(NULL);

	if (!stmt->FalseBlock->Code->Empty())				//if exist
	{
		VisitCode(stmt->FalseBlock->Code);		//Visit FALSE block
		StartCheck(NULL);
	}
}

/*****************************************************
*				recognize loop & similar statement	 *
******************************************************/
void CompField::VisitLoop(StmtLoop *stmt)		
{

	nbASSERT(stmt != NULL, "StmtLoop cannot be NULL");

	VisitTree(stmt->Forest);				//Visit ..

	VisitTree(stmt->InitVal);			//Visit initialization values

#ifdef DEBUG_COMPACT_FIELDS
	PrintNode(stmt->InitVal,"Debug_List_of_Loop_InitVal.asm",0);
#endif

	if(checkingloop>0)
		ExtractFieldsFromExpr(stmt->InitVal,&m_symboltemp);	//Expression Here!!	


	VisitTree(stmt->TermCond);			//Visit ????

	VisitStatement(stmt->IncStmt);			//Visit increment statements

	VisitCode(stmt->Code->Code);			//Visit loop code

	StartCheck(NULL);

}


void CompField::VisitDoWhile(StmtWhile *stmt)				
{
	nbASSERT(stmt != NULL, "StmtDoWhile cannot be NULL");

#ifdef DEBUG_COMPACT_FIELDS
	PrintNode(stmt->Forest,"Debug_List_of_DoWhile.asm",0);
#endif

	if(checkingloop>0)
		ExtractFieldsFromExpr(stmt->Forest,&m_symboltemp);	

	VisitCode(stmt->Code->Code);			//Visit Code inside DoWhile

	VisitTree(stmt->Forest);				//Visit Condition 
	StartCheck(NULL);
}

void CompField::VisitWhileDo(StmtWhile *stmt)
{
	nbASSERT(stmt != NULL, "StmtWhileDo cannot be NULL");

#ifdef DEBUG_COMPACT_FIELDS
	PrintNode(stmt->Forest,"Debug_List_of_WhileDo.asm",0);
#endif
	if(checkingloop>0)
		ExtractFieldsFromExpr(stmt->Forest,&m_symboltemp);	

	VisitTree(stmt->Forest);				//Visit Condition

	VisitCode(stmt->Code->Code);			//Visit Code inside WhileDo

	StartCheck(NULL);	
}

/*****************************************************
*				recognize block statement			 *
*****************************************************/
void CompField::VisitBlock(StmtBlock *stmt)
{
	nbASSERT(stmt != NULL, "StmtBlock cannot be NULL");

	VisitCode(stmt->Code);			//Visit Code inside block

	StartCheck(NULL);
}



/******************************************************
*			Search for Compattable fields			  *
******************************************************/
//[mligios]			REMEMBER INNERFIELDS!!!  m_compatting=0 if previous elements weren't compattable
//If first element compattable 			   -start!
//If element compattable after compattable 	   -continue!
//If element non-compattable after compattable     -stop!
//If element non-compattable after non-compattable -continue!
void CompField::VisitFieldDetails(Node *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(GET_OP(node->Op) == OP_FIELD, "node must be a OP_FIELD");
	nbASSERT(node->Sym->SymKind == SYM_FIELD, "terminal node symbol must be a SYM_FIELD");

	SymbolField *field = (SymbolField*)node->Sym;
	nbASSERT(field != NULL, "field cannot be NULL");

	m_CurrProtoSym=field->Protocol;				//Extract Protocol-Name of This Field

	if(checkingloop>0)	//if -> Inside LoopSize (we don't need to check out used-fields out of loop! -> Useless )
	{
		if(field->Used)		//if -> this field is used?
		{
			//Add To DefUsedMap
			AddFieldToMap((SymbolField *)field,&m_DefUseLocal);
		}	
	}	

	switch(field->FieldType)
	{
		case PDL_FIELD_FIXED:
		{
			SymbolFieldFixed *fixed = (SymbolFieldFixed*)field;
			StartCheck(field);

#ifdef DEBUG_COMPACT_FIELDS	
	PrintSymbol(field,"Debug_List_of_Fixed_Fields.asm");
#endif

			this->VisitInnerFields(fixed);			
		}break;
		case PDL_FIELD_BITFIELD:
		{
#ifdef DEBUG_COMPACT_FIELDS	
	PrintSymbol(field,"Debug_List_of_Bit_Fields.asm");
#endif

//SymbolFieldBitField *bit = (SymbolFieldBitField*)field;
			StartCheck(field);
//TEMP -> bit can't be parent!
//				this->VisitInnerFields(bit);			
		}break;
		case PDL_FIELD_VARLEN:			
		{
#ifdef DEBUG_COMPACT_FIELDS	
	PrintSymbol(field,"Debug_List_of_Variable_Fields.asm");
#endif
			StartCheck(field);
			//extract field from expression!
			Node *expr=((SymbolFieldVarLen *)field)->LenExpr;
			if(checkingloop>0)
			  ExtractFieldsFromExpr(expr,&m_symboltemp);	

		}break;
		case PDL_FIELD_PADDING:
		case PDL_FIELD_TOKEND:
		case PDL_FIELD_TOKWRAP:
		case PDL_FIELD_LINE:
		case PDL_FIELD_PATTERN:
		case PDL_FIELD_EATALL:
		{
			StartCheck(field);
		}break;
		default:
		{
			nbASSERT(false, "What field is this!? (VisitFieldDetails)");			
		}break;
	}

}


/*****************************************************
*				Visit Inner Fields					 *
*****************************************************/

void CompField::VisitInnerFields(SymbolFieldContainer *parent)
{
	if (!parent->InnerFields.empty())
	{
#ifdef DEBUG_COMPACT_FIELDS	
	PrintSymbol(parent,"Debug_List_of_Father_and_child_Fields.asm");
#endif

		for (FieldsList_t::iterator i = parent->InnerFields.begin(); i != parent->InnerFields.end(); i++)// visit inner fields
		{
			SymbolFieldContainer *childField = (SymbolFieldContainer*) *i;
			SymbolFieldContainer *container=parent;

			switch (childField->FieldType)
			{
			case PDL_FIELD_BITFIELD:
				{
				SymbolFieldBitField *bitField = (SymbolFieldBitField *)childField;
				uint32 mask=bitField->Mask;

				//TEMP	-> should check also compattable!?! now fixed-dummy-container will be marked as used!
				if(bitField->Used)
				{
					 parent->Used=true;	
					#ifdef DEBUG_COMPACT_FIELDS	
						PrintSymbol(bitField,"Debug_List_of_Father_and_child_Fields.asm");
					#endif
				}

				// search the fixed field root to get the size
				while (container->FieldType!=PDL_FIELD_FIXED)
				{
					nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");
					// and the mask with the parent mask
					mask=mask & ((SymbolFieldBitField *)container)->Mask;
					container=((SymbolFieldBitField *)container)->DependsOn;

				}
				SymbolFieldFixed *fixed=(SymbolFieldFixed *)container;
				nbASSERT((fixed->Size == 1)||(fixed->Size == 2)||(fixed->Size == 4), "size can be only 1, 2, or 4!");

				}break;
			case PDL_FIELD_FIXED:
				{
				nbASSERT(false, "INNER-FIXED  SHOULDN'T EXISTS, never reached this instruction");
				}break;
			default:
				nbASSERT(false, "An inner field should be either a bit field or a fixed field.");
				break;
			}

		}
	}
}

/*****************************************************
*						Optimization Methods		 *
******************************************************/
void CompField::CleanTempMap(IntMap *mymap)
{
	nbASSERT(mymap != NULL, "mymap cannot be NULL (CLEAN)");	
	mymap = NULL;
}

/******************************************************
* Update Map with fields contained in loop expression *
*******************************************************/
void CompField::UpdateMapField(StmtWhile *stmt){
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(stmt->Prev != NULL, "stmt->Prev cannot be NULL");
	nbASSERT(stmt->Prev->Forest != NULL, "stmt->Prev->Forest cannot be NULL");

	//Extract Expression from Statement LOOP
	Node *expression=Findexpr(stmt->Prev->Forest);
	nbASSERT(expression,"ERROR SEARCHING EXPRESSION-LOOP!");

	//Extract Field from Expression
	ExtractFieldsFromExpr(expression,&m_symboltemp);

}

/*****************************************************
*	Find fields inside this Expression & fill map	 *
******************************************************/
void CompField::ExtractFieldsFromExpr(Node *expr, IntMap *mymap){
	nbASSERT(expr != NULL, "expression cannot be NULL HERE!");
	nbASSERT(mymap != NULL, "mymap cannot be NULL HERE!");


	for(uint32 i=0;i<3;i++)				//check all the kids of this node
	{
		if(expr->Kids[i]!=NULL)			//check this node
		{	
			Symbol *s=expr->Kids[i]->Sym;
			if(s!=NULL)	//check this symbol
			{
				switch(s->SymKind)
				{
				case SYM_FIELD:	
				{
					AddFieldToMap((SymbolField *)s,mymap);
					return;
				}break;
				case SYM_INT_CONST:
				{
					//integer value recognized
				}break;
				default:
				{
					//Do nothing			
				}break;
				}//End of switch
			}						
		ExtractFieldsFromExpr(expr->Kids[i],mymap);	//check Sub-Field!
		}
	}

	return;						//means that we didn't found it!		
	
}

/*****************************************************
*				Add this field to this Map		     *
******************************************************/
void CompField::AddFieldToMap(SymbolField *field,IntMap *mymap){
  nbASSERT(field != NULL, "field cannot be NULL");	
  nbASSERT(mymap != NULL, "field cannot be NULL");
  IntMap::iterator it;

  //Check if this field was already inserted
  if((*mymap).count(field)>0)	//already added to the list-> only UPDATE
  {
	it=(*mymap).find(field);
	(*it).second++;		//Update number of Expression-reference of this field
  }
  else			//first time we met this field -> Insert this field* in the map 
  {
	(*mymap).insert(pair<SymbolField*,int>(field,1));	//new element -> ADD
  }
  
}

/********************************************************
* Add this map to Uncompattable (temporary) Loop List	*
********************************************************/
void CompField::AddUncompattedLoop(StmtWhile *stmt,IntMap *fields){
	nbASSERT(stmt != NULL, "StmtWhile cannot be NULL");
	nbASSERT(fields != NULL, "fields cannot be NULL");
	mylist newelem;
	newelem.stmt=stmt;
	newelem.localmap=(*fields);

	#ifdef DEBUG_COMPACT_FIELDS
		PrintFieldsMap(fields,"Debug_List_of_Fields_Used_inside_temp_Uncompatted_Loop.txt");
	#endif

	UncompattableLoops.push_back(newelem);
}

/*****************************************************
*  Add this map to Never Compattable Loop List		 *
******************************************************/
void CompField::AddNeverCompattableLoop(StmtWhile *stmt,IntMap *fields)
{
	nbASSERT(stmt != NULL, "stmt cannot be NULL");
	nbASSERT(fields != NULL, "fields cannot be NULL");

	mylist element;
	element.stmt=stmt;
	element.localmap=(*fields);

	#ifdef DEBUG_COMPACT_FIELDS
		PrintFieldsMap(fields,"Debug_List_of_Fields_Used_inside_NeverCompattable_Loop.txt");
	#endif

	//Delete this loop from UncompattableLoop List
	UncompattableLoops.remove(element);

	//Add this loop to NeverLoops List
	NeverLoops.push_back(element);

	//TEMP ! CHECK IF Fields is good here (use element?)
	for(IntMap::iterator it=element.localmap.begin() ; it != element.localmap.end(); it++ )
  	{
		//for each symbol of this loop:
		//find all the loops where it's used and add these to NeverCompattable
		if((*it).second!=-1) // if wasn't updated yet
		{
			(*it).second=-1;
			CheckUncompattedLoop((*it).first);	//check if this field is inside other loops (uncompatted)
		}				
        }
}

/***************************************************************
* Find out if this field appears inside Uncompatted Loop List  *
****************************************************************/
void CompField::CheckUncompattedLoop(SymbolField *field)
{
	//for each uncompatted loop:
	for(LoopList::iterator it=UncompattableLoops.begin();it!=UncompattableLoops.end();it++)
	{
	  IntMap *mymap=&((*it).localmap);	  
	  if((*mymap).count(field)>0)	//this field is used (outLoops or inside a NeverCompattable-Loop )
	  {		
		//if i found this field -> this loop must be added to nevercompattable

		//Set all its fields to "-1"
		for(IntMap::iterator i=(*mymap).begin(); i != (*mymap).end(); i++)
		{
			if((*i).second!=-1)
			{
			  (*i).second=-1;
			  CheckUncompattedLoop((*i).first);
			}
		}

		//this loop can't be compatted
  	        AddNeverCompattableLoop((*it).stmt,&((*it).localmap));			  		
	  }
	}
}

/*******************************************************************
*			check if all the elements of the list are compattable  *
*******************************************************************/
bool CompField::AllCompattable(StmtWhile *stmt){

	//****************************************************************************//
	//		First Check: No Used Fields Here!			      //
	//****************************************************************************//
	//for each element of localRef check if references are all inside this loop
	if(m_loopcomp==1)		//this loop it's a leaf (considering loop-TREE) and it contain only - Not Used Fields 
	{
		#ifdef DEBUG_COMPACT_FIELDS	
			PrintStmt(stmt,"Debug_List_of_Loop_without_used_fields.asm");
		#endif

		return true;
	}

	//****************************************************************************//
	//		Second Check: 						      //
	//****************************************************************************//
	//	if Map<SymbF*,int> -> int == -1 means: FIELD USED OUTSIDE LOOPS
	//			   -> int > 0   means: FIELD USED INSIDE LOOPS
	//	IF USED OUTSIDE -> CAN'T COMPACT THIS LOOP 
	//	IF USED INSIDE LOOPS CHECK IF IT'S USED ONLY HERE -> TRANSFORMFLAG = TRUE (#GLOBAL == #LOCAL)
	//	OTHERWISE WAIT PROTOCOL END, THEN CHECK IF CAN BE COMPATTED AND DO IT
	
	bool TransformFlag=true;
	bool never=false;
	IntMap::iterator it;
	IntMap::iterator global_it;
	IntMap::iterator localuse_it;

	for ( it=m_DefUseLocal.begin() ; it != m_DefUseLocal.end(); it++ )	// for each Defined-Used-Field (Inside this Loop)
  	{

	  global_it=m_symbolref.find((*it).first);				// Check if #ofUse is limitated to this loop
	  nbASSERT(global_it != m_symbolref.end(), "global MAP cannot be NULL here"); 

	  if((*global_it).second == -1)  // this field is used outside loops -> Can't Compact this one
	  {
		TransformFlag=false;  // for compatibility

		//Add this LOOP to NeverCompattableLoopList
		never=true;
          }

	  localuse_it=m_symboltemp.find((*it).first);
	  if(localuse_it != m_symboltemp.end()) 		//if this field is Used in expressions inside this loop 
	  {
	  	if((*global_it).second > (*localuse_it).second)	// if #ofTotalUse > #ofUse (Locally) -> Also used later!!
		{
		//Add this LOOP to UnCompattableLoopList
		TransformFlag=false;
		}
	  }else
		TransformFlag=false;		//this field is defined here but it used later... (outside this loop)
	}


	if(never==true)		
	{
		AddNeverCompattableLoop(stmt,&m_DefUseLocal);
	}
	else
	{
		if(TransformFlag==false)
		{
			//Add this LOOP to UnCompattableLoopList + this_IntMap	
			AddUncompattedLoop((StmtWhile*)stmt,&m_DefUseLocal);								
		}
	}

	
	return TransformFlag;	//IF TRUE COMPACT-LOOP NOW! 

}

/***************************************************************
* Transform all the loops remained in Uncompattable Loop List  *
****************************************************************/
//This Function is called by Destructor! Now we're sure about which loop can be compatted and which not
void CompField::TransformRemainedLoop()
{
	//for each uncompatted loop:
	for(LoopList::iterator it=UncompattableLoops.begin();it!=UncompattableLoops.end();it++)
	{
	  IntMap *mymap=&((*it).localmap);	  
	  for(IntMap::iterator i=(*mymap).begin(); i != (*mymap).end(); i++)
	  {
		if((*i).second==-1)
		{
			nbASSERT(false, "If #number ==-1 means Never Compattable! Can't be HERE!");			
		}

		#ifdef DEBUG_COMPACT_FIELDS
			PrintFieldsMap(mymap,"Debug_List_of_Fields_REMAINED_Loop.txt");
		#endif
   	  }

	  //This loop must be compatted
	  StmtWhile *stmt=(*it).stmt;  	  

	  TransformLoop(stmt);
	}
}

/********************************************************************************************/
void CompField::SetAsRemovable(IntMap *mymap)		//Set All the fields of this MAP (inside this loop) as Removable!! (Cause we're going to deleting it)
{
	nbASSERT(mymap != NULL, "MAP can't be NULL!");			

	//Mark this Protocol-Fields-List as toBeUpdated
	toBeUpdated=true;

	for(IntMap::iterator i=(*mymap).begin(); i != (*mymap).end(); i++)
	{
//		cout<<(*i).first->Protocol->Name<<"."<<(*i).first->Name<<" SET as REMOVABLE"<<endl;
		(*i).first->ToKeep=false;
	}

}

/********************************************************************************************/
//				Check if this field is compattable 			    //
/********************************************************************************************/
//if it's the first one m_compatting must be set
//if it's in the middle no operation needed
//if it's the last one m_compatting must be reset & list must be compatted
/********************************************************************************************/
//				Start Check on Field 					    //
/********************************************************************************************/
void CompField::StartCheck(SymbolField *field)			//TO BE FIXED
{
	SymbolFieldFixed *tmpfixed=(SymbolFieldFixed *)field;
	nbASSERT(m_actualstmt!=NULL, "This pointer can't be null! (actual_statement) - (StartCheck)");	

	if(field!=NULL && field->Used==true)		//if this field is used can't compact the loop!  Reset Loop Flag!
		m_loopcomp=0;

	if(tmpfixed != NULL)			
	{
	  if(field->FieldType==PDL_FIELD_FIXED)
	  {
		if(field->Compattable && field->Used==false)	//---------if true means that considerated-field is compattable
		{
			if(m_compatting)		//---------must be added to compact-list
			{
				//continue...compatting..
				m_stopstmt=m_actualstmt;  //this statement can be the last one
				m_longname=m_longname+"_"+field->Name;	
				m_longsize+=tmpfixed->Size;// add to the previous one
			}
			else			//new first of compact-list
			{
				m_startstmt=m_actualstmt;  //this statement is the first one				
				m_stopstmt=m_actualstmt;   //set the last temp to this
				m_compatting=1;
				m_longname="COMPATTED_"+field->Name;			
				m_longsize=tmpfixed->Size;// add to the previous one
			}


		}
		else				//-----------------if false -> should try to compact
		{

			if(m_compatting)		//---------list of compattable isn't empty
			{
				m_compatting=0; 		//reset m_compatting
				CompactFields();		
			}
		}
	 }
	 else
	 {
	 	//it is a bit field!
         }
	}
	else
	{
		if(m_compatting) //list of compattable isn't empty- this isn't a generic field but here we know that we must compact!!!
		{
			m_compatting=0; 		//reset m_compatting
			CompactFields();
		}
	}
}

/******************************************************
*			Link newest field to the correct position *
******************************************************/
SymbolProto *CompField::GetCurrentProto(){			
	SymbolFieldFixed *first,*last;
	first=(SymbolFieldFixed *)m_startstmt->Forest->Kids[0]->Sym;
	last=(SymbolFieldFixed *)m_stopstmt->Forest->Kids[0]->Sym;
	nbASSERT(first!=NULL, "This pointer can't be null! (first)");	
	nbASSERT(last!=NULL, "This pointer can't be null! (last)");	

	if(first->Protocol!=last->Protocol)
		nbASSERT(false, "CAN'T BE! (PROTOCOL of first FIELD!=PROTOCOL of last FIELD)");	

	return first->Protocol;
}

/*****************************************************
*				Compact fields (from start to stop)  *
*****************************************************/
void CompField::CompactFields(){
	SymbolFieldFixed *newSymb;   //new symbolfield

	HIRNode *newNodeExt;	//new node
	HIRNode *newNodeInt;	//new node
	StmtBase *newStmt; 	//new statement

	StmtBase *before; 	
	StmtBase *after; 	
	bool deletelonelybits=false;
	nbASSERT(m_actualstmt!=NULL, "This pointer can't be null! (actual_statement)");	
	nbASSERT(m_startstmt!=NULL, "This pointer can't be null! (start_statement)");
	nbASSERT(m_stopstmt!=NULL, "This pointer can't be null! (stop_statement)");		

	/********************************************/
	//	BUILD & LINK NEW SYMBOLFIELD	    //
	/********************************************/
	nbASSERT(m_longsize, "Size of the new Field Error!!");	

	if(m_startstmt==m_stopstmt)		//List of COMPATTABLE fields contains only one field!! (LONELY-COMPATTABLE-FIELD)
	{
		nbASSERT(m_startstmt->Forest->Kids[0]!=NULL,"THIS Node CAN'T BE NULL! ");
		SymbolField *symb=((SymbolField *)m_startstmt->Forest->Kids[0]->Sym);
		nbASSERT(symb!=NULL,"THIS Symbol CAN'T BE NULL! ");

		if(symb->Used==true || symb->Compattable==false)
		{
		   //this can't be! Who Called CompactFields??!!
		  return;
		}

		FieldsList_t container=((SymbolFieldContainer *)symb)->InnerFields;
		if(container.begin()!=container.end())
		{
			deletelonelybits = true;	//This fixed is a container of fields (e.g. bit_union)
		}
		else
		{
#ifdef DEBUG_COMPACT_FIELDS
			ofstream my_ir("Debug_Lonely_fields.txt",ios::app);
			my_ir <<m_longname<<": "<<m_longsize<<" Don't need to be compatted! (LONELY FIELD) "<<endl;
#endif
			return;				//this is a lonely fixed-field
		}
	}

	m_CurrProtoSym = GetCurrentProto();			//Get Current Protocol by checkin field->Protocol->Name && Check if startfield->proto != stopfield->proto (CAN'T BE!)

	newSymb = new SymbolFieldFixed(m_longname,m_longsize);
	nbASSERT(newSymb!=NULL, "Problems on Allocation of the new SymbolField");	

	newSymb->Used=false;			//GENERATED COMPATTED-FIELD CAN'T BE USED ANYWHERE!
	newSymb->Compattable=true;		//ONCOMINGS TransformLoop() can look this attribute

	// Add the protocol reference in the field Symbol
	newSymb->Protocol=m_CurrProtoSym;
	//Store the field symbol into the field symbol tables                      
	newSymb = (SymbolFieldFixed *)m_GlobalSymbols.StoreProtoField(m_CurrProtoSym,newSymb);

#ifdef DEBUG_COMPACT_FIELDS
	PrintSymbol(newSymb,"Debug_Fields_Fixed_COMPATTED.txt");
#endif

	/********************************************/
	//	BUILD & CONNECT NEW NODES	    //
	/********************************************/

	newNodeInt = new HIRNode(IR_FIELD,newSymb);  //leaf-node!!
	nbASSERT(newNodeInt!=NULL, "Problems on Allocation of the new Internal-node (leaf)");	
	
	newNodeExt = new HIRNode(IR_DEFFLD,newNodeInt);  
	nbASSERT(newNodeExt!=NULL, "Problems on Allocation of the new External-node");	

	/********************************************/
	//	BUILD & CONNECT NEW STATEMENT	    //
	/********************************************/

	newStmt = new StmtGen(newNodeExt);  
	nbASSERT(newStmt!=NULL, "Problems on Allocation of the new Statement");	

	/*********************************************/					//TEMP	-	TO BE FIXED!!
	//	Delete Fields Global-References	     //
	/*********************************************/


	//Mark this Protocol-Fields-List as toBeUpdated
	toBeUpdated=true;

	for(StmtBase *it = m_startstmt;it!=NULL && it!=m_stopstmt->Next;it=it->Next)	//works well??
	{
		SymbolField *field=(SymbolField*)it->Forest->Kids[0]->Sym;
		nbASSERT(field!=NULL, "it->Forest->Kids[0]->Sym Problems!");	
		field->ToKeep=false;
	}


	//void DeleteFieldsRef(SymbolProto *prFindExpressionFieldotoSym, uint32 from, uint32 to);  from=(sym->ID of the first compattable field)
//	SymbolField *from;
//	SymbolField *to;

//	from=(SymbolField *)m_startstmt->Forest->Kids[0]->Sym;
//	to=(SymbolField *)m_stopstmt->Forest->Kids[0]->Sym;


//	cout<<"FROM: ID: "<<from->ID<<endl;
//	cout<<"TO: ID: "<<to->ID<<endl;
//TEMP
/*
	for(StmtBase *it = m_startstmt;it!=NULL && it!=m_stopstmt->Next;it=it->Next)	//works well??
	{
		nbASSERT(it->Forest->Kids[0]->Sym!=NULL, "it->Forest->Kids[0]->Sym Problems!");	
		m_GlobalSymbols.PrintGlobalInfo(m_CurrProtoSym,(SymbolField *)it->Forest->Kids[0]->Sym);
	}
*/
	//DELETE from GlobalSymbols these Field References!			//ERRRRRRRRRRRRRORR
	//m_GlobalSymbols.DeleteFieldsRef(m_CurrProtoSym,from->ID,to->ID);	//works well?? NOOOOOO

	if(deletelonelybits)		//THIS IS A SINGLE FIELD-FIXED BUT IT'S A CONTAINER!
	{
		nbASSERT(m_stopstmt->Next!=NULL,"IF THIS IS A CONTAINER - HERE MUST FOUND INNERFIELD! NOT NULL!");
		StmtBase *it;

#ifdef DEBUG_COMPACT_FIELDS
		ofstream Lonely_container("Debug_Fields_Container_lonely.txt",ios::app);
		Lonely_container <<"FROM: "<<((SymbolField *)m_stopstmt->Forest->Kids[0]->Sym)->Name<<" *******************"<<endl;
#endif
		//we need to find the last bitfield (cause now StopStmt is pointing to the container!)
		for( it = m_stopstmt->Next; it!=NULL && ((SymbolField *)it->Forest->Kids[0]->Sym)->FieldType==PDL_FIELD_BITFIELD ;it=it->Next)	//works well??
		{
#ifdef DEBUG_COMPACT_FIELDS
			Lonely_container <<"INNER: "<<((SymbolField *)it->Forest->Kids[0]->Sym)->Name<<endl;			
#endif

			((SymbolField *)it->Forest->Kids[0]->Sym)->ToKeep=false;		//we need to mark all USELESS FIELDS!!


		}
		//nbASSERT(it!=NULL,"IF THIS IS A CONTAINER - HERE MUST FOUND INNERFIELD! NOT NULL!");
		before=m_startstmt->Prev; 
		after=it;

#ifdef DEBUG_COMPACT_FIELDS
		Lonely_container <<"END INNER &( "<<after<<" )"<<endl;			
#endif

	}
	else	
	{
		after=m_stopstmt->Next;
		before=m_startstmt->Prev;
	}


	/********************************************/
	/**	       CHECK HEAD & TAIL	   **/
	/********************************************/
	if(before==NULL)	//if first -> update HEAD
	{
		m_headcode->SetHead(newStmt);		//m_headcode = pointers to the first Statement
	}

	if(after==NULL)		//if last -> update TAIL
	{

		m_headcode->SetTail(newStmt);
	}

	
	//FIXME: Ivano - this method causes segfoult on windows
	//DESTROY ALL THE STATEMENTS Between Start & Stop
/*
	for(StmtBase *it = m_startstmt;it!=NULL && it!=after;it=it->Next)	
		destroy_stmt(it);
*/

	/*********************************************/
	/**CONNECT WITH PREVIOUS AND NEXT STATEMENTS**/
	/*********************************************/

	newStmt->Next=after;
	newStmt->Prev=before;

	if(before!=NULL)	//it's the first stmt?	
	{
		before->Next=newStmt;
	}

	
	if(after!=NULL)		//it's the last stmt?	
	{
		after->Prev=newStmt;
	}


#ifdef DEBUG_COMPACT_FIELDS
	PrintStmt((StmtBase *)newStmt,"COMPATTED_FIXED_fields.txt");
#endif

}

/********************************************************************************************/
//				   Destroy useless nodes												    //
/********************************************************************************************/
//FIXME: Ivano - this method causes segfoult on windows
/*void CompField::destroy_stmt(StmtBase *stmt)
{	//only statement with field definitions are considered, so others nodes are not checked
	nbASSERT(stmt!=NULL, "Problems on Destroying old Statement! (Wrong ARG)");	
	if(stmt->Forest!=NULL)
		delete(stmt);	
}
*/

/********************************************************
* Find Sub-Graphs needed for building SymbolFieldVarLen	*
*********************************************************/
Node *CompField::Findexpr(Node *sub){
	nbASSERT(sub!=NULL, "This pointer can't be null! (Node-FindExpression)");
	nbASSERT(sub->Kids[1]!=NULL, "This pointer can't be null! (Node-FindExpression)-Kids[1]");
	nbASSERT(sub->Kids[1]->Kids[1]!=NULL, "This pointer can't be null! (Node-FindExpression)-Kids[1]-Kids[1]");
	return sub->Kids[1]->Kids[1];
}

/******************************************************
*				Transform Loop in a Variable Field 	  *
******************************************************/
void CompField::TransformLoop(StmtWhile* stmt){
	nbASSERT(stmt!=NULL, "This pointer can't be null! (actual_loop_statement)");
	nbASSERT(stmt->Prev!=NULL, "This pointer can't be null!!!! (prev_loop_statement->Expression)");	
	nbASSERT(m_actualstmt!=NULL, "This pointer can't be null! (actual_statement)");	

	StmtGen *NewStmtField;
	HIRNode *NewNodeExt;
	HIRNode *NewNodeInt;

	SymbolFieldVarLen *FieldVar;
	Node *expression;
	string Name;

	/********************************************/
	// EXPRESSION IS A SUB-TREE OF STMT->FOREST //
	/********************************************/
	expression=Findexpr(stmt->Prev->Forest);
	nbASSERT(expression,"ERROR SEARCHING EXPRESSION-LOOP!");


	Name = string("loop_union_") + int2str(m_LoopFldCount++, 10);

	/********************************************/
	//	BUILD & CONNECT NEW Virtual-FIELD   //
	/********************************************/
	_nbNetPDLElementFieldVariable *newfield=new _nbNetPDLElementFieldVariable;  

	newfield->ExprTree=(_nbNetPDLExprBase* )expression;  
	
		
	/********************************************/
	//	BUILD & CONNECT NEW SYMBOLFIELD	    //
	/********************************************/

	FieldVar = new SymbolFieldVarLen(Name,(_nbNetPDLElementFieldBase *)newfield,expression);  //-> correct! ->PDL_FIELD_VARLEN
	nbASSERT(FieldVar!=NULL, "Problems on Allocation of the new FieldVar");	


	// Add the protocol reference in the field Symbol
	FieldVar->Protocol=m_CurrProtoSym;			//[mligios] has to be handled better!!!


	FieldVar->Used=false;
	FieldVar->Compattable=false;

	//Store the field symbol into the field symbol tables
	FieldVar = (SymbolFieldVarLen *)m_GlobalSymbols.StoreProtoField(m_CurrProtoSym,FieldVar);

	SetAsRemovable(&m_DefUseLocal);

	/********************************************/
	//	BUILD & CONNECT NEW NODE	    //
	/********************************************/

	NewNodeInt = new HIRNode(IR_FIELD,FieldVar);  //leaf-node!!
	nbASSERT(NewNodeInt!=NULL, "Problems on Allocation of the new Internal-node (leaf)");	
	
	NewNodeExt = new HIRNode(IR_DEFFLD,NewNodeInt);  
	nbASSERT(NewNodeExt!=NULL, "Problems on Allocation of the new External-node");	

	/********************************************/
	//		Build NEW Statement	    //
	/********************************************/

	NewStmtField = new StmtGen(NewNodeExt);  
	nbASSERT(NewStmtField!=NULL, "Problems on Allocation of the new Statement (FIELD VARIABLE)");	


	/********************************************/
	//		Link NEW Statement	    //
	/********************************************/

//TEMP - REMEMBER TO UPDATE FIRST_VERIFY - FIRST_AFTER - FIRST_ENCAP POINTERS IF THIS IS THE FIRST ONE... CAN?	TO BE FIXED!?!?!?!?

	StmtBase *m_beforeloop=stmt->Prev->Prev;
	StmtBase *m_afterloop=stmt->Next;

	if(m_beforeloop==NULL)	//if first -> update HEAD
	{
		m_headcode->SetHead(NewStmtField);
	}else
		m_beforeloop->Next=NewStmtField;

	if(m_afterloop==NULL)	//if last -> update TAIL
	{
		m_headcode->SetTail(NewStmtField);
	}else
		m_afterloop->Prev=NewStmtField;
	

	NewStmtField->Next=m_afterloop;
	NewStmtField->Prev=m_beforeloop;


	/********************************************/
	//		Modify Comments		    //
	/********************************************/

	if(m_afterloop->Kind==STMT_COMMENT)
	   ChangeComments(m_beforeloop,"LOOP TRANSFORMED");
	else
	   nbASSERT(false, "Must be the Comments here! Where are them? (TRANSFORM LOOP)");	
		
	if(m_beforeloop->Kind==STMT_COMMENT)
	   ChangeComments(m_afterloop,"END OF VAR-LOOP");
	else
	   nbASSERT(false, "Must be the Comments here! Where are them? (TRANSFORM LOOP)");	


	/********************************************/
	//		Print 4 Debug		    //
	/********************************************/

	#ifdef DEBUG_COMPACT_FIELDS
		PrintSymbol(FieldVar,"Debug_List_of_Loop_Generated_symbols.asm");
	#endif

	/********************************************/
	//		Destroy OLD Statement	    //
	/********************************************/
	//TEMP
	//DeleteReferenceStmt(stmt);

	//FIXME: Ivano -this method causes segfoult on windows
	//destroy_stmt(stmt);


}

/********************************************************************************************/
//			Change the comments of this statement		  		    //
/********************************************************************************************/
void CompField::ChangeComments(StmtBase *stmt,string comment)
{
	nbASSERT(stmt,"ERROR (ChangeComments) COMMENT-STMT NULL!");
	string newcomm=string(comment);//.append(" size: ").append((StmtWhile*)stmt->ExprString); 
	stmt->Comment=newcomm;
}

/*****************************************************
* Delete Field-Rereference in Global Structures	  	 *
******************************************************/		//FIXME
void CompField::DeleteReferenceStmt(StmtWhile *stmt)
{
	nbASSERT(stmt,"ERROR (DeleteReference) LOOP-STMT NULL!");
	StmtBlock *baseblock=(StmtBlock *)stmt->Code;

	StmtBase *Head=baseblock->Code->Front();
	StmtBase *Tail=baseblock->Code->Back();

	for(StmtBase *it=Head; it!=NULL && it!=Tail->Next; it=it->Next)  //correct? tail->next?
	{
		
		if(it->Kind==STMT_GEN)			
		{
			Node *node=it->Forest;
			nbASSERT(node,"ERROR (DeleteReference) LOOP-Stmt-Forest NULL!");
			DeleteReferenceNodes(node);
		}
	}

}

/********************************************************************************************/		//TO BE FIXED!!

//FIXME
void CompField::DeleteReferenceNodes(Node *node)
{
	nbASSERT(node,"ERROR (DeleteReference) LOOP-node NULL!");

	for(uint32 i=0;i<3;i++)
	{
		if(node->Kids[i]!=NULL)
			DeleteReferenceNodes(node->Kids[i]);
	}

/*
	if(node->Op==IR_DEFFLD)		//IT IS A NODE CONTAINING DEFFLD!
	{
		nbASSERT(node->Kids[0],"ERROR (DeleteReference) LOOP-node->Kids[0] NULL!");				
		SymbolField *symb=(SymbolField *)node->Kids[0]->Sym;

		if(symb!=NULL)
		{
			//DELETE from GlobalSymbols this Field Reference!
			m_GlobalSymbols.DeleteFieldRef(m_CurrProtoSym,symb);			//FIXME! (GLOBALSYMBOLS.CPP)
			//cout<<"SYMBOL-FIELD: "<<symb->Name<<endl;
		}

	}
*/
}

/**********************************************************************************************************************************/
//							DEBUG									  //
/**********************************************************************************************************************************/

/******************************************************
*			Check if it's a valid statement!		  *
******************************************************/
bool CompField::isvalid(StmtBase *stmt)
{
if(stmt==NULL)
   return false;

	switch(stmt->Kind)
	{
	case STMT_LABEL:
	case STMT_GEN:
	case STMT_JUMP:
	case STMT_JUMP_FIELD:
	case STMT_SWITCH:
	case STMT_CASE:
	case STMT_BLOCK:
	case STMT_COMMENT:
	case STMT_IF:
	case STMT_LOOP:
	case STMT_WHILE:
	case STMT_DO_WHILE:
	case STMT_BREAK:
	case STMT_CONTINUE:
	case STMT_PHI:
	case STMT_FINFOST:
	{
		cout<<"OK!! STATEMENT RECOGNIZED!"<<endl;
		return true;
		}break;
	default:
	{
		cout<<"????? STATEMENT unRECOGNIZED!"<<endl;
		return false;  //not valid stmt!!
		}break;
	}
}


#ifdef DEBUG_COMPACT_FIELDS
/*****************************************************
*							PRINT	    			 *
*****************************************************/
/*****************************************************
* 				Print Map of Used fields 			 *
******************************************************/
void CompField::PrintFieldsMap(IntMap *mymap, string fname)
{
	nbASSERT(mymap!=NULL, "This pointer can't be null! (mymap) - (PrintFieldsMap)");	
	IntMap::iterator it;
	ofstream m_StreamData(fname.c_str(), ios::app);	

	for ( it=(*mymap).begin() ; it != (*mymap).end(); it++ )
  	{
		  m_StreamData << "Protocol: "<<((*it).first)->Protocol->Name;
		  m_StreamData << " Name: " << ((*it).first)->Name<< " USED? "<<((*it).first)->Used;
		  m_StreamData << " (TOT_used) => " << (*it).second << endl;
	}
	m_StreamData.close();

}

/*****************************************************
*				Print Statement Base				 *
******************************************************/
void CompField::PrintStmt(StmtBase* stmt, string fname)	
{
	nbASSERT(stmt!=NULL, "This pointer can't be null! (stmt) - (PrintStmt)");	
	ofstream m_StreamData(fname.c_str(), ios::app);				

	m_StreamData<<"***Statement***START"<<endl;
	m_StreamData<<"Opcode: "<<stmt->Opcode<<endl;
	m_StreamData<<"Kind: "<<stmt->Kind<<endl;
	m_StreamData<<"Comments: "<<stmt->Comment<<endl;
	m_StreamData<<"Flags: "<<stmt->Flags<<endl;
	if(stmt->Forest)
		PrintNode(stmt->Forest,fname,0);
	m_StreamData<<"***Statement***END"<<endl;
	m_StreamData.close();
}

/*****************************************************
*				Print Statement While				 *
*****************************************************/
void CompField::PrintWhile(StmtWhile* stmt, string fname){
	nbASSERT(stmt!=NULL, "This pointer can't be null! (statement) - (StmtWhile)");	

	ofstream m_StreamData(fname.c_str(), ios::app);				
	m_StreamData<<"	PRINT-WHILE(START): "<<endl;

	PrintStmt(stmt,fname);			//print base statement

	Printblocks(stmt->Code,fname);	//print specific-block statement

	m_StreamData<<"	PRINT-WHILE(END) "<<endl;
	m_StreamData<<"*******************************************************************************"<<endl;
	m_StreamData.close();
}

/*****************************************************
*				Print Statement Block				 *
*****************************************************/
void CompField::Printblocks(StmtBlock* stmt, string fname)
{
	nbASSERT(stmt!=NULL, "This pointer can't be null! (statement) - (printBlock)");	

	ofstream m_StreamData(fname.c_str(), ios::app);				


	StmtBase *Head=stmt->Code->Front();
	//StmtBase *Tail=stmt->Code->Back();

	m_StreamData<<"*x*x*x*x*  PRINT-BLOCK(START): "<<endl;


	m_StreamData<<"	PRINT-BLOCK(stmt-BASE): "<<endl;
	PrintStmt(stmt,fname);
	m_StreamData<<"	PRINT-BLOCK(stmt-BASE) END"<<endl;

	m_StreamData<<"*Starting NODES inside Block:"<<endl;
	for(StmtBase *it=Head; it!=NULL; it=it->Next)
		PrintStmt(it,fname);		

	m_StreamData<<"*Ended    NODES inside Block"<<endl;
	m_StreamData<<"*x*x*x*x*  PRINT-BLOCK(END) "<<endl;
	m_StreamData.close();
}

/*****************************************************
*				Print Nodes						     *
*****************************************************/
void CompField::PrintNode(Node* node, string fname,uint32 childnumber)
{
	nbASSERT(node!=NULL, "This pointer can't be null! (node) - (PrintNode)");	
	ofstream m_StreamData(fname.c_str(), ios::app);				

	m_StreamData<<"****NODE-START ( "<<childnumber<<" ) - (0=father, 1=kids[0],2=kids[1],3=kids[2])"<<endl;
	m_StreamData<<"    OP: "<<node->Op<<endl;
	m_StreamData<<"    Value: "<<node->Value<<endl;
	m_StreamData<<"    NRefs: "<<node->NRefs<<endl;

	if(node->Sym!=NULL){
		//m_StreamData<<"    -Print_Symbol-"<<endl;
		PrintSymbol(node->Sym,fname);
		//m_StreamData<<"    -Printed_Symbol-"<<endl;
	}
	if(node->SymEx!=NULL){
		//m_StreamData<<"    -Print_SymEx-"<<endl;
		PrintSymbol(node->SymEx,fname);
		//m_StreamData<<"    -Printed_SymEx-"<<endl;
	}

	for(uint32 i=0;i<3;i++)
	{
		if(node->Kids[i]!=NULL)	
		    PrintNode(node->Kids[i],fname,i+1);
	}
	m_StreamData<<"****NODE-END"<<endl;
	m_StreamData.close();

}

/*****************************************************
*				Print Symbols					     *
*****************************************************/
void CompField::PrintSymbol(Symbol *Symb, string fname)
{
	nbASSERT(Symb!=NULL, "This pointer can't be null! (Symbol) - (PrintSymbol)");
	ofstream m_StreamData(fname.c_str(), ios::app);				

	switch(Symb->SymKind)
	{
		case SYM_FIELD:
		{
			SymbolField *Sym=(SymbolField *)Symb;
			PrintSymbolField(Sym,fname);
		}break;
		case SYM_RT_VAR:	//useless.... 	//	PrintSymbolVariable(Sym,fname);
		default:
		{
			m_StreamData<<"	(Symbol-START) "<<endl;
			m_StreamData<<"		(Symbol-Kind):"<<Symb->SymKind<<endl;
			m_StreamData<<"	(Symbol-END) "<<endl;
		}break;
	}	
	m_StreamData.close();

}

/*****************************************************
*				Print Containers & InnerFields	     *
*****************************************************/
void CompField::PrintContainer(SymbolFieldContainer *Container, string fname)
{
	nbASSERT(Container!=NULL, "This pointer can't be null! (Container) - (PrintContainer)");
	ofstream m_StreamData(fname.c_str(), ios::app);	
	if (!Container->InnerFields.empty())
	{
		SymbolField *Sym=(SymbolField*)Container;
	
		//this will end when (SymbolContainer-father)&==(Sym->DependsOn)&

		m_StreamData<<"**SymbolContainer-START & "<<Sym<<endl;
		m_StreamData<<"Name: "<<Sym->Name<<endl;
		m_StreamData<<"FieldType: "<<Sym->FieldType<<endl;
		m_StreamData<<"PDLFieldInfo &: "<<Sym->PDLFieldInfo<<endl;  //pointer
		m_StreamData<<"ID: "<<Sym->ID<<endl;
		m_StreamData<<"Protocol: "<<Sym->Protocol->Name<<endl;		//pointer
		m_StreamData<<"Container &: "<<Sym->DependsOn<<endl;		//pointer


			list<SymbolField*>::iterator start=Container->InnerFields.begin();
			list<SymbolField*>::iterator end=Container->InnerFields.end();
			for(FieldsList_t::iterator it=start;it!=end;it++) //nonsense
			{
				SymbolFieldContainer *tmp=(SymbolFieldContainer*)*it;
				m_StreamData<<"**INSIDE Container START: (&) "<<tmp<<endl;
				//PrintSymbolField(tmp,fname);		
				PrintContainer(tmp,fname);
				m_StreamData<<"**INSIDE Container END"<<endl;		
			}
		m_StreamData<<"**SymbolContainer-END "<<endl;
	}
	//else m_StreamData<<"**Container EMPTY!!!!!"<<endl;

	m_StreamData.close();
}

/*****************************************************
*				Print Symbol Fields  			     *
*****************************************************/
void CompField::PrintSymbolField(SymbolField *Sym, string fname)
{
	nbASSERT(Sym!=NULL, "This pointer can't be null! (SymbolField) - (PrintSymbol)");
	ofstream m_StreamData(fname.c_str(), ios::app);				

	m_StreamData<<"**Symbol-START (kind= "<<Sym->SymKind<<" )"<<endl;
	m_StreamData<<"	Name: "<<Sym->Name<<endl;
	m_StreamData<<"	UsedAsInt: "<<Sym->UsedAsInt<<endl;
	m_StreamData<<"	FieldType: "<<Sym->FieldType<<endl;
	m_StreamData<<"	PDLFieldInfo &: "<<Sym->PDLFieldInfo<<endl;  //pointer
	m_StreamData<<"	ID: "<<Sym->ID<<" : if !REAL number -> Contain innerfields or not Set Yet!"<<endl;
	m_StreamData<<"	Protocol: "<<Sym->Protocol->Name<<endl;	
	m_StreamData<<"	Used?: "<<Sym->Used<<endl;		
	m_StreamData<<"	Container &: "<<Sym->DependsOn<<endl;		//pointer
	m_StreamData<<"	SYM &: "<<Sym<<endl;				//pointer
	if(Sym->DependsOn!=NULL)
		PrintContainer(Sym->DependsOn,fname);
	m_StreamData<<"**Symbol-END"<<endl;
	m_StreamData.close();

}

/*****************************************************
*				Print Symbols Variable			     *
*****************************************************/
void PrintSymbolVariable(SymbolVariable *Sym,string fname)
{
	nbASSERT(Sym!=NULL, "This pointer can't be null! (SymbolField) - (PrintSymbol)");

	ofstream m_StreamData(fname.c_str(), ios::app);				
	m_StreamData<<"**Symbol-VARiable-START"<<endl;
	m_StreamData<<"	Name: "<<Sym->Name<<endl;
	m_StreamData<<"**Symbol-VARiable-END"<<endl;
	m_StreamData.close();


}

/**********************************************************************************************************************************/
//					END											  //
/**********************************************************************************************************************************/

#endif // DEBUG_COMPACT_FIELDS


#endif	//end of field_&_loop_optimizations

