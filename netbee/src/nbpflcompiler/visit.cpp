/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "tree.h"
#include "symbols.h"
#include "visit.h"

/********************************************************************************************/
//				Main function						    //
/********************************************************************************************/
void Visit::VisitCode(CodeList *Code)
{
	StmtBase *Next = Code->Front();

	while(Next)
	{
		if (Next->Flags & STMT_FL_DEAD)
		{
			Next = Next->Next;
			continue;
		}
		m_actualstmt = Next;
		VisitStatement(Next);
		Next = Next->Next;
	}
}

/********************************************************************************************/
//				for each Statement					    //
/********************************************************************************************/

void Visit::VisitStatement(StmtBase *stmt)
{
	nbASSERT(stmt != NULL , "stmt cannot be NULL");
	
	switch(stmt->Kind)
	{
	case STMT_GEN:			
	{
		VisitTree(stmt->Forest);	
	}break;
	case STMT_JUMP:
	case STMT_JUMP_FIELD:
	{
		StmtJump *jump = static_cast<StmtJump*>(stmt);
		VisitJump(jump);
	}break;
	case STMT_CASE:	  			//Gettin inside a Case! New "Context"
	{
		StmtCase *stmtcase = static_cast<StmtCase*>(stmt);	
		VisitCase(stmtcase, false);
	}break;
	case STMT_SWITCH: 			//Gettin inside a SWITCH! New "Context"
	{
		StmtSwitch *stmtswitch = static_cast<StmtSwitch*>(stmt);		
		VisitSwitch(stmtswitch);
	}break;
	case STMT_IF:  				//Gettin inside an IF! New "Context"
	{
		StmtIf *stmtif = static_cast<StmtIf*>(stmt);		
		VisitIf(stmtif);
	}break;
	case STMT_LOOP:  			//only <loop type="times2repeat">
	{
		StmtLoop *stmtloop = static_cast<StmtLoop*>(stmt);			
		VisitLoop(stmtloop);
	}break;
	case STMT_WHILE:
	{
		StmtWhile *stmtwhile = static_cast<StmtWhile*>(stmt);			
		VisitWhileDo(stmtwhile);		//Visit inside this WHILE 
	}break;
	case STMT_DO_WHILE:   	//Gettin inside a WHILE! New "Context"
	{
		StmtWhile *stmtwhile = static_cast<StmtWhile*>(stmt);				
		VisitDoWhile(stmtwhile);
	}break;
	case STMT_BLOCK:  	//Gettin inside a Block! New "Context"
	{
		StmtBlock *stmtblock = static_cast<StmtBlock*>(stmt);				
		VisitBlock(stmtblock);
	}break;
	case STMT_BREAK:			
	case STMT_CONTINUE:		//If Break or Continue (Inside Loops) -> Can't Compact this!! Don't know Total Size!! (Offset)
	{
		//cout<<"break"<<endl;
	}break;
	case STMT_LABEL:
	{
		//cout<<"LABEL:"<<endl;
	}break;
	case STMT_COMMENT:			
	{	
		//cout<<"COMMENT: "<<stmt->Comment<<endl;		
	}break;
	default:
		nbASSERT(false, "CANNOT BE HERE!");
		break;
	}

}

/********************************************************************************************/
//				Follow nodes inside the (NODE) tree 			    //
/********************************************************************************************/
void Visit::VisitTree(Node *node)
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
			//VisitFieldDetails(node->Kids[0]);				
			//cout<<"deffld"<<endl;
		}break;
		case IR_ASGNI:
		{
		bool flag=false;
			for(uint32 i =0;i<2;i++){
				if(node->Kids[i])
				{
//					cout<<"Sym: "<<node->Kids[i]->Sym<<" Value: "<< node->Kids[i]->Value<<endl;
					if(node->Kids[i]->Sym)
					{
						Symbol *S=node->Kids[i]->Sym;
//						cout<<"Symbol: "<<S->SymKind<<endl;	
						if(S->SymKind==SYM_RT_VAR)
						{
//USEFUL
			//				cout<<"Name: "<<((SymbolVariable *)S)->Name<<endl;
						}					
						if(S->SymKind==SYM_RT_LOOKUP_ITEM)
						{
//						cout<<"Symbol - FOUND: "<<S->SymKind<<endl;
//						SymbolLookupTableItem *T = (SymbolLookupTableItem *)S;
//						cout<<"ITEM: "<<T->Name<<"Type: "<<T->Type<<endl; //PDLVariableType
//						if(T->Type==PDL_RT_VAR_PROTO)
//							cout<<"proto!!"<<endl;
//						cout<<T->Table->Label->Name<<endl;					
//						cout<<"LABEL: "<<T->Table->Label->Proto->Name<<endl;
						flag=true;
						}					
					}
				}
			}
			
			if(flag){
				flag=false;
				AddPacketLengthCheck(m_actualstmt);
			}		
		}
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
		default:					
		{

			if (node->Kids[0])
			{
				//cout<<"kid 0"<<endl;
				VisitTree(node->Kids[0]);
			}
			if (node->Kids[1])
			{
				//cout<<"kid 1"<<endl;
				VisitTree(node->Kids[1]);
			}
			if (node->Kids[2])			//Really USEFUL??
			{
				//cout<<"kid 2"<<endl;			
				VisitTree(node->Kids[2]);
			}
		}break;
	}

}

/********************************************************************************************/
//				recognize jumps 					    //
/********************************************************************************************/
void Visit::VisitJump(StmtJump *stmt)
{
	nbASSERT(stmt != NULL, "JUMP cannot be NULL");

//USEFUL
/*
	if(stmt->TrueBranch)
	{
//		SymbolLabel *label = (SymbolLabel *)stmt->TrueBranch;
//		cout<<"JUMP TO -> "<<label->Name<<endl;
	}
*/	
	if (stmt->FalseBranch != NULL)
		VisitTree(stmt->Forest);

}
/********************************************************************************************/
//				recognize case	 					    //
/********************************************************************************************/
void Visit::VisitCase(StmtCase *stmt, bool isDefault)
{
	nbASSERT(stmt != NULL, "StmtCase cannot be NULL");

	if (!isDefault)
	{
		VisitTree(stmt->Forest);			//Visit i-case-condition
	}

	if (stmt->Target)
	{
		return;
	}

	nbASSERT(stmt->Code != NULL, "StmtCase->Code cannot be NULL");	

	VisitCode(stmt->Code->Code);			//Visit code inside this case!!
}

/********************************************************************************************/
//				recognize switch statement				    //
/********************************************************************************************/
void Visit::VisitSwitch(StmtSwitch *swStmt)
{
	nbASSERT(swStmt != NULL, "StmtSwitch cannot be NULL");

	VisitTree(swStmt->Forest);				//Visit SWITCH Condition

	VisitCode(swStmt->Cases);				//Visit Cases

	if (swStmt->Default)					
		VisitCase(swStmt->Default, true);		//Visit Default

}
/********************************************************************************************/
//				recognize if statement 					    //
/********************************************************************************************/
void Visit::VisitIf(StmtIf *stmt)					//[mligios_check]
{
	nbASSERT(stmt != NULL, "StmtIf cannot be NULL");

	VisitTree(stmt->Forest);				//Visit Condition

	nbASSERT(stmt->TrueBlock != NULL, "Stmt->TrueBlock cannot be NULL");	

	VisitCode(stmt->TrueBlock->Code);			//Visit TRUE block

	if (stmt->FalseBlock)
	{
		nbASSERT(stmt->FalseBlock != NULL, "Stmt->FalseBlock cannot be NULL");		
		nbASSERT(stmt->FalseBlock->Code != NULL, "Stmt->FalseBlock->Code cannot be NULL");				
		if (!stmt->FalseBlock->Code->Empty())				//if exist
		{
			VisitCode(stmt->FalseBlock->Code);		//Visit FALSE block
		}
	}
}

/********************************************************************************************/
//				recognize loop & similar statement			    //
/********************************************************************************************/
void Visit::VisitLoop(StmtLoop *stmt)		
{

	nbASSERT(stmt != NULL, "StmtLoop cannot be NULL");

	VisitTree(stmt->Forest);				//Visit ..

	VisitTree(stmt->InitVal);			//Visit initialization values

	VisitTree(stmt->TermCond);			//Visit ????

	VisitStatement(stmt->IncStmt);			//Visit increment statements

	nbASSERT(stmt->Code != NULL, "StmtLoop->Code cannot be NULL");	
	
	VisitCode(stmt->Code->Code);			//Visit loop code

}
/********************************************************************************************/
void Visit::VisitDoWhile(StmtWhile *stmt)				
{
	nbASSERT(stmt != NULL, "StmtDoWhile cannot be NULL");
	
	nbASSERT(stmt->Code != NULL, "StmtWhile->Code cannot be NULL");	
	
	VisitCode(stmt->Code->Code);			//Visit Code inside DoWhile

	VisitTree(stmt->Forest);				//Visit Condition 
}
/********************************************************************************************/
void Visit::VisitWhileDo(StmtWhile *stmt)
{
	nbASSERT(stmt != NULL, "StmtWhileDo cannot be NULL");

	VisitTree(stmt->Forest);				//Visit Condition

	nbASSERT(stmt->Code != NULL, "StmtWhile->Code cannot be NULL");	

	VisitCode(stmt->Code->Code);			//Visit Code inside WhileDo
}

/********************************************************************************************/
//				recognize block statement				    //
/********************************************************************************************/
void Visit::VisitBlock(StmtBlock *stmt)
{
	nbASSERT(stmt != NULL, "StmtBlock cannot be NULL");

	VisitCode(stmt->Code);			//Visit Code inside block

}




void Visit::AddPacketLengthCheck(StmtBase *stmt)
{
	nbASSERT(stmt!=NULL,"Stmt can't be NULL! (AddPacketLengthCheck)");

	StmtBase *prev=stmt->Prev;
	StmtBase *next=stmt->Next;


//	StmtBase *secnext=next->Next;
//	StmtIf *checklength;
/*
	StmtComment *newcomment;
	newcomment = new StmtComment("Start HIR CODE about ack recognition");

	prev->Next = newcomment;
	newcomment->Prev = prev;
	newcomment->Next = stmt;	

	newcomment = new StmtComment("Stop HIR CODE about ack recognition");	
	newcomment->Prev = stmt;
	newcomment->Next = next;	

	stmt->Next = newcomment;
	next->Prev = newcomment;
	
*/

	string comment = "Check for Empty Packets";

	CodeList *ifcode = m_GlobalSymbols.NewCodeList(true);
	HIRCodeGen m_CodeGen(m_GlobalSymbols,ifcode);	
	HIRNode *pktLength = m_CodeGen.BinOp(IR_SUBI,
		m_CodeGen.TermNode(IR_IVAR, m_GlobalSymbols.LookUpVariable("$packetlength")),
		m_CodeGen.TermNode(IR_IVAR, m_GlobalSymbols.CurrentOffsSym));						


	StmtIf *ifPayload = m_CodeGen.IfStatement(m_CodeGen.BinOp(IR_GTI, pktLength, m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(0))));

	
	ifPayload->Prev = prev;
	prev->Next = ifPayload;
	
	ifPayload->Next = next;
	next->Prev = ifPayload;

	ifcode->SetHead(stmt);
	ifcode->SetTail(stmt);
		
	StmtBlock *block = new StmtBlock(ifcode,comment);
	ifPayload->TrueBlock = block;
	
//	m_CodeGen.CommentStatement("Stop HIR CODE about ack recognition");				

}




