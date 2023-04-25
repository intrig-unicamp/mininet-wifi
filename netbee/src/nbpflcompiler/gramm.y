/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


%{


//#define YYDEBUG 1

/*
hack to overcome the  "warning C4273: 'malloc' : inconsistent dll linkage" in visual studio when linking the dll C runtime
*/
#ifdef WIN32
  #define YYMALLOC malloc
  #define YYFREE free
#endif

#include <stdlib.h>
#include "defs.h"
#include "compile.h"
#include "utils.h"
#include "pflexpression.h"
#include "tree.h"
#include "symbols.h"
#include "typecheck.h"
#include "../nbee/globals/utils.h"
#include <limits.h>



%}

/* create a reentrant parser */
%pure-parser
%locations

%parse-param {struct ParserInfo *parserInfo}
%lex-param {struct ParserInfo *parserInfo}


%union{
    FieldsList_t    			*FieldsList;
    SymbolField					*Field;
    PFLStatement    			*stmt;
    PFLAction       			*act;
    PFLExpression				*exp;
    uint32						index;
    Node						*IRExp;
    char						id[ID_LEN];
    uint32						number;	
    bool						flag;
    PFLRepeatOperator   		repOp;
    PFLSetExpression*			setExp;     
    list<PFLSetExpression*>* 	setExpList;  
    list<PFLExpression*>*   	expList; 
}



%{

//NOTE: these declarations are duplicated in scanner.l, so if you modify them here you need to do the same in the other file
int pfl_lex (YYSTYPE *lvalp, YYLTYPE *llocp, struct ParserInfo *parserInfo);
int pfl_error(YYLTYPE *llocp, struct ParserInfo *parserInfo, const char *s);

PFLExpression *GenUnOpNode(PFLOperator op, PFLExpression *kid);
PFLExpression *GenBinOpNode(PFLOperator op, PFLExpression *kid1, PFLExpression *kid2);
PFLExpression *GenTermNode(SymbolProto *protocol, Node *irExpr);
PFLExpression *MergeNotInTermNode(ParserInfo *parserInfo, PFLExpression *node1);
PFLExpression *MergeTermNodes(ParserInfo *parserInfo, HIROpcodes op, PFLExpression *node1, PFLExpression *node2);
PFLExpression *ManagePFLProtoField(char *protoName, ParserInfo *parserInfo, YYLTYPE *llocp);

PFLExpression *GenProtoBytesRef(YYLTYPE *llocp, ParserInfo *parserInfo, char *protocol, uint32 offs, uint32 len);
PFLExpression *GenNumber(struct ParserInfo *parserInfo, uint32 value);
PFLExpression *GenString(struct ParserInfo *parserInfo, string value);	
bool CheckOperandTypes(ParserInfo *parserInfo, PFLExpression *expr);
SymbolField *CheckField(ParserInfo *parserInfo, char * protoName, YYLTYPE *llocp, uint32 headerIndex = 0);

set<pair<SymbolProto*,uint32> > protocolsForExtraction;

#define STORE_EXP_TEXT(e, fc, fl, lc, ll)	do{\
							string tmp_s;\
							tmp_s.assign(parserInfo->FilterString, fc, lc - fc);\
							e->SetExprText(tmp_s);\
						}while(0)

#define PFL_ERROR(l, pi, s) do{ pfl_error(l, pi, s); YYERROR; }while(0)
%}
%token <id>		PFL_RETURN_PACKET
%token <id>		PFL_EXTRACT_FIELDS
%token <id>		PFL_ON
%token <id>		PFL_PORT
%token <id>		PFL_NUMBER
%token <id>		PFL_HEXNUMBER
%token <id>		PFL_PROTOCOL
%token <id>		PFL_PROTOFIELD
%token <id>		PFL_PROTOCOL_INDEX
%token <id>		PFL_PROTOFIELD_INDEX
%token <id>		PFL_PROTOMULTIFIELD_INDEX
%token <id>		PFL_MULTIPROTOFIELD
%token <id>		PFL_PROTOMULTIFIELD
%token <id>		PFL_IPV4ADDR
%token <id>		PFL_IPV6ADDR
%token <id>		PFL_MACADDR
%token			PFL_ADD
%token			PFL_ADDADD
%token			PFL_SUB
%token			PFL_MULMUL
%token			PFL_BWAND
%token			PFL_BWOR
%token			PFL_BWXOR
%token			PFL_BWNOT
%token			PFL_SHL
%token			PFL_SHR
%token			PFL_AND
%token			PFL_OR
%token			PFL_NOT
%token			PFL_EQ
%token			PFL_NE
%token			PFL_LT
%token			PFL_LE
%token			PFL_GT
%token	 		PFL_GE
%token			PFL_MATCHES
%token 			PFL_CONTAINS
%token			PFL_SENSITIVE
%token <id>		PFL_STRING
%token 			PFL_IN
%token			PFL_NOTIN
%token			PFL_QUESTION
%token			PFL_ANY
%token			PFL_TUNNELED
%token			PFL_FULLENCAP
%token			PFL_NOSESSION
%token			PFL_NOACK


%left	PFL_OR 
%left   PFL_AND
%left	PFL_IN PFL_NOTIN
%left   PFL_BWAND PFL_BWOR
%left	PFL_SHL PFL_SHR
%left	PFL_ADD PFL_SUB
%left	PFL_ADDADD
%left	PFL_MUL
%left 	PFL_MUOLMUL
%nonassoc	PFL_NOT PFL_BWNOT

%type <stmt> 	   	Statement
%type <act> 	  	Action
%type <act> 	 	ExtractFields
%type <act> 	  	ReturnPkt
%type <exp> 	  	BoolExpression
%type <exp> 	  	UnaryExpression
%type <setExpList>	InExpression	   
%type <setExpList>	NotInExpression    
%type <repOp>	   	Repeater 
%type <setExp>		ProtoOrAny		   
%type <exp> 	   	OrExpression
%type <exp> 	   	AndExpression
%type <exp> 	   	BoolOperand
%type <exp> 	   	RelExpression
%type <exp> 	   	Protocol
%type <exp> 	   	BitwiseExpression
%type <exp> 	   	ShiftExpression
%type <exp> 	   	AddExpression
%type <exp> 	   	MulExpression
%type <exp> 	   	Term
%type <number> 	   	GenericNumber
%type <FieldsList> 	FieldsList
%type <Field> 	   	FieldsListItem
%type <flag>   	   	sensitive
%type <exp>			ProtoFieldOrProtofieldIndex   
%type <setExp>		Proto	
%type <flag>		Tunnel
%type <flag>		FullEncap
%type <expList>		SetOfProtocol 	  
%type <expList>		ElementOfSet 	
%type <exp>			SetOrExpression
%type <exp> 	   	SetAndExpression
%type <exp> 	  	SetUnaryExpression
%type <expList>		ProtocolOrSet	  

%type <flag>		FlagNosession

%type <flag>		FlagNoack
%destructor {delete $$;} BoolExpression BoolOperand RelExpression Protocol Term
%destructor {delete $$;} BitwiseExpression ShiftExpression AddExpression MulExpression

%%


Statement: BoolExpression FullEncap
								{
									if (yynerrs > 0)
									{									
										parserInfo->Filter = NULL;
									}
									else
									{ 
										parserInfo->Filter = new PFLStatement($1, new PFLReturnPktAction(1), NULL,$2);
									}
									$$ = parserInfo->Filter;			
									if(parserInfo->FilterString.length() != (size_t)(@$.last_column-@$.first_column)){
										printf("Error messages:\n  1. [PFL error] syntax error\n");
										exit(-1);	
									}	
									
								}
								
			| FullEncap Action								
								{
									if (yynerrs > 0)
									{									
										parserInfo->Filter = NULL;
									}
									else
									{ 
										parserInfo->Filter = new PFLStatement(NULL, (PFLAction*)$2, NULL,$1);
									}
									
									$$ = parserInfo->Filter;
								}
		   |BoolExpression FullEncap Action	
								{
									if (yynerrs > 0)
									{									
										parserInfo->Filter = NULL;
									}
									else
									{ 
										parserInfo->Filter = new PFLStatement($1,(PFLAction*)$3, NULL,$2);
									}
									
									$$ = parserInfo->Filter;
								}
                        ;		
                        
FullEncap: /*nothing*/ 
								{ 
									$$ = false;
								}
				| PFL_FULLENCAP
								{
									$$ = true;
								}
				;							


Action:		ReturnPkt
		   |ExtractFields	
								{
									$$ = $1;
								}
                        ;			

ExtractFields: PFL_EXTRACT_FIELDS '('FieldsList')'
								{
									PFLExtractFldsAction *action= new PFLExtractFldsAction();
									for(FieldsList_t::iterator i = ($3)->begin(); i != ($3)->end(); i++)
										action->AddField((*i));
									$$=action;
								}
						;

FieldsList: FieldsListItem
								{
									//$1->Protocol->NeedExtraction = true;
									FieldsList_t *list = new FieldsList_t();
									list->push_back($1);
									$$ = list;
								}
           |FieldsList ',' FieldsListItem
		   						{
		   							//$3->Protocol->NeedExtraction = true;
									FieldsList_t *list = $1;
									list->push_back($3);
									$$ = list;
								}
						;
						
		
FieldsListItem: PFL_PROTOFIELD
								{
									//i.e. proto.field
									SymbolField *field = CheckField(parserInfo,$1, &yylloc);
									if (field == NULL)
									{
									char ErrMessage[1024];

										ssnprintf(ErrMessage, sizeof(ErrMessage), "Unknown field in extraction statement for protocol '%s'", $1);
										PFL_ERROR(&yylloc, parserInfo, ErrMessage);
//										PFL_ERROR(&yylloc, parserInfo, "Unknown field in extraction statement");
									}
									field->HeaderIndex.push_back(1);//remember that: proto.field == proto%1.field
									
									if(field->MultiProto && field->HeaderIndex.size()>0)
										PFL_ERROR(&yylloc, parserInfo, "Unallowed extraction list. You wrote something like: \"proto*.field,proto%n.field\" or \"proto*.field,proto.field\"");
										
									$$ = field;
								}
				|PFL_MULTIPROTOFIELD
								{
									//i.e. proto*.field
									string field_name($1);
									//we remove the "*" symbol from the field name, so we can check it properly
									string field_no_star = remove_chars(field_name, "*", "");
									SymbolField *field = CheckField(parserInfo, (char*)field_no_star.c_str(), &yylloc);
									if (field == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Unknown field in extraction statement");
									}
									field->MultiProto = true;
									
									if(field->MultiProto && field->HeaderIndex.size()>0)
										PFL_ERROR(&yylloc, parserInfo, "Unallowed extraction list. You wrote something like: \"proto*.field,proto%n.field\" or \"proto*.field,proto.field\"");

										
									$$ = field;
																
								}
				|PFL_PROTOMULTIFIELD
								{
									//i.e. proto.field*
									string field_name($1);
									//we remove the "*" symbol from the field name, so we can check it properly
									string field_no_star = remove_chars(field_name, "*", "");
									SymbolField *field = CheckField(parserInfo, (char*)field_no_star.c_str(), &yylloc);
									if (field == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Unknown field in extraction statement");
									}
									if(field->Name == "allfields")
									{
										PFL_ERROR(&yylloc, parserInfo, "\"proto.allfields*\" is not supported"); 
									}
																		
									field->HeaderIndex.push_back(1);

									//field->MultiField = true;
									uint32 multifield = (field->MultiProto) ? field->HeaderIndex.size()+1 : field->HeaderIndex.size();
									field->MultiFields.insert(multifield);


									if(field->MultiProto && field->HeaderIndex.size()>0)
										PFL_ERROR(&yylloc, parserInfo, "Unallowed extraction list. You wrote something like: \"proto*.field,proto%n.field\" or \"proto*.field,proto.field\"");

										
									$$ = field;
																
								}
				|PFL_PROTOFIELD_INDEX {
									//i.e. proto%n.field
									char *fieldName, *index;
									uint32 num = 0;
									fieldName = strchr($1, '.');
									if(fieldName != NULL)
									{
										*fieldName = '\0';
										fieldName++;
									}

									index = strchr($1, '%');
									if(index != NULL)
									{
										*index = '\0';
										index++;
										str2int(index, &num, 10);
									}
									
									char *field_name($1);
									
									strcat(field_name, (char*)("."));
									strcat(field_name, fieldName);
									
									string field_ok = field_name;
									SymbolField *field = CheckField(parserInfo, (char*)field_ok.c_str(), &yylloc,num);

									if (field == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Unknown field in extraction statement");
									}
									
									field->HeaderIndex.push_back(num);
									
									if(field->MultiProto && field->HeaderIndex.size()>0)
										PFL_ERROR(&yylloc, parserInfo, "Unallowed extraction list. You wrote something like: \"proto*.field,proto%n.field\" or \"proto*.field,proto.field\"");

										
									$$ = field;
								}
				|PFL_PROTOMULTIFIELD_INDEX{
									//i.e. proto%n.field*									
									char *element = $1;
									element[sizeof(element)] = '\0';
																	
									char *fieldName, *index;
									uint32 num = 0;
									fieldName = strchr(element, '.');
									if(fieldName != NULL)
									{
										*fieldName = '\0';
										fieldName++;
									}

									index = strchr(element, '%');
									if(index != NULL)
									{
										*index = '\0';
										index++;
										str2int(index, &num, 10);
									}
									
									char *field_name(element);
									
									strcat(field_name, (char*)("."));
									strcat(field_name, fieldName);
									
									string field_ok = field_name;
									SymbolField *field = CheckField(parserInfo, (char*)field_ok.c_str(), &yylloc,num);

									if (field == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Unknown field in extraction statement");
									}
									
									field->HeaderIndex.push_back(num);
									//field->MultiField = true;
									uint32 multifield = (field->MultiProto) ? field->HeaderIndex.size()+1 : field->HeaderIndex.size();
									field->MultiFields.insert(multifield);
									
									
									if(field->MultiProto && field->HeaderIndex.size()>0)
										PFL_ERROR(&yylloc, parserInfo, "Unallowed extraction list. You wrote something like: \"proto*.field,proto%n.field\" or \"proto*.field,proto.field\"");

									$$ = field;			
								}
						;
										

ReturnPkt: PFL_RETURN_PACKET PFL_ON PFL_PORT GenericNumber
									{
										
										PFLReturnPktAction *action= new PFLReturnPktAction($4);
										$$=action;
																			
									}
						;

UnaryExpression:	'(' BoolExpression ')' // DARK cito una modifica di Fulvio, per compatibilità bison-2.4: rimuovo: {$$ = $2}
								{
									$$ = $2;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BoolOperand Tunnel FlagNosession FlagNoack
						{
							//$1 is a term expression
							//we have to create a list containing the single protocol
							list<PFLExpression*> *elementList = new list<PFLExpression*>();
							elementList->push_back($1);
							PFLSetExpression *setExpr = new PFLSetExpression(elementList,SET_OP,$2); //DEFAULT_ROP - DEFAULT_INCLUSION
							if (setExpr == NULL)
								throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
							setExpr->SetAnyPlaceholder(false);
							list<PFLSetExpression*> *setList = new list<PFLSetExpression*>();
							setList->push_back(setExpr);
							PFLRegExpExpression *expr = new PFLRegExpExpression(setList, REGEXP_OP); 
							if (expr == NULL)
								throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
								
							//Nosession		|| NoACK					
							if ($3 == true || $4 == true)
							{
									list<SymbolProto*> *protos = setExpr->GetProtocols();
									for(list<SymbolProto*>::iterator p = protos->begin(); p!=protos->end(); p++)
									{
										if($3)
											(*p)->NoSession=true;
										if($4)
											(*p)->NoAck=true;										
									}
							}	
							
							$$ = expr;
							STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
						}
					|PFL_NOT UnaryExpression
								{
									PFLExpression *child = $2;
									if (child->GetType() == PFL_TERM_EXPRESSION)
									{
										$$ = MergeNotInTermNode(parserInfo, child);
									}
									else
									{
										PFLUnaryExpression *expr = new PFLUnaryExpression(child, UNOP_BOOLNOT);
										if (expr == NULL)
											throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
										
										$$ = expr;
									}
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}

					|BoolOperand Tunnel FlagNosession FlagNoack PFL_IN InExpression {

						//$1 is a PFLTermExpression
						//$6 is a list<PFLSetExpression*>*
						(*$6->begin())->SetInclusionOperator(IN_OP);
						list<PFLExpression*> *setList = new list<PFLExpression*>();
						setList->push_front($1);
						PFLSetExpression *setExpr = new PFLSetExpression(setList,SET_OP,$2);
						if (setExpr == NULL)
							throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
						setExpr->SetAnyPlaceholder(false);
						$6->push_front(setExpr);
						PFLRegExpExpression *expr = new PFLRegExpExpression($6, REGEXP_OP); 
						if (expr == NULL)
							throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
							
						//Nosession		|| NoACK					
						if ($3 == true || $4 == true)
						{
								list<SymbolProto*> *protos = setExpr->GetProtocols();
								for(list<SymbolProto*>::iterator p = protos->begin(); p!=protos->end(); p++)
								{
									if($3)
										(*p)->NoSession=true;
									if($4)
										(*p)->NoAck=true;										
								}
						}														
						
						$$ = expr;
						STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
												
					}
					|BoolOperand Tunnel FlagNosession FlagNoack PFL_NOTIN NotInExpression {
						//$1 is a PFLTermExpression
						//$4 is a list<PFLSetExpression*>*
						(*$6->begin())->SetInclusionOperator(NOTIN);
						list<PFLExpression*> *setList = new list<PFLExpression*>();
						setList->push_front(static_cast<PFLExpression*>($1));						
						PFLSetExpression *setExpr = new PFLSetExpression(setList,SET_OP,$2);
						if (setExpr == NULL)
							throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
						setExpr->SetAnyPlaceholder(false);
						$6->push_front(setExpr);
						PFLRegExpExpression *expr = new PFLRegExpExpression($6, REGEXP_OP); 
						if (expr == NULL)
							throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
							
						//Nosession		|| NoACK					
						if ($3 == true || $4 == true)
						{
								list<SymbolProto*> *protos = setExpr->GetProtocols();
								for(list<SymbolProto*>::iterator p = protos->begin(); p!=protos->end(); p++)
								{
									if($3)
										(*p)->NoSession=true;
									if($4)
										(*p)->NoAck=true;										
								}
						}														
							
						$$ = expr;
						STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
						
					};
					
Tunnel: /*nothing*/
		{
			$$ = false;
		}				
		| PFL_TUNNELED
		{
			$$ = true;
		}
		;
					
InExpression:		ProtoOrAny{
					//$1 contains a PFLSetExpression*
					//we have to create the list of sets
					list<PFLSetExpression*>* setExpList = new list<PFLSetExpression*>();
					//we add the set to the list
					setExpList->push_back($1);
					$$ = setExpList;
				}
			
			| InExpression PFL_IN ProtoOrAny {
					//$3 contains a PFLSetExpression*
					//$1 contantains list<PFLSetExpression*>*
					$3->SetInclusionOperator(IN_OP);
					$1->push_back($3);
					$$ = $1;
				}
			|InExpression PFL_NOTIN NotInExpression{
					//$1 contantains list<PFLSetExpression*>*
					//$3 contantains list<PFLSetExpression*>*
			
					//the first element of $3 is involved in a "notin" operation
					(*$3->begin())->SetInclusionOperator(NOTIN);
					for(list<PFLSetExpression*>::iterator it = $3->begin(); it != $3->end(); it++)
						$1->push_back(*it);
					$3->clear();
					delete($3);
					$$ = $1;
				}			
			;
					
NotInExpression: Proto {
				list<PFLSetExpression*>* setExpList = new list<PFLSetExpression*>();
				setExpList->push_back($1);
				$$ = setExpList;
			}
			| NotInExpression PFL_NOTIN Proto {
				$3->SetInclusionOperator(NOTIN);
				$1->push_back($3);
				$$ = $1;
			}
			| NotInExpression PFL_IN InExpression {
				//$1 contantains list<PFLSetExpression*>*
				//$3 contantains list<PFLSetExpression*>*
				(*$3->begin())->SetInclusionOperator(IN_OP);
				for(list<PFLSetExpression*>::iterator it = $3->begin(); it != $3->end(); it++)
					$1->push_back(*it);
				$3->clear();
				delete($3);
				$$ = $1;
			};

ProtoOrAny:	Proto 	
			| PFL_ANY {
				//we have found the any placeholder.
				//we create the empty list
				list<PFLExpression*> *setList = new list<PFLExpression*>();
				//we create the set expression
				PFLSetExpression *setExpr = new PFLSetExpression(setList,SET_OP);
				if (setExpr == NULL)
					throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
				setExpr->SetAnyPlaceholder(true);
				$$ = setExpr;	
			}
			
			| PFL_ANY Repeater {
				list<PFLExpression*> *setList = new list<PFLExpression*>();
				PFLSetExpression *setExpr = new PFLSetExpression(setList,SET_OP,false,$2);//we specify the repeat operator in the constructor
				if (setExpr == NULL)
					throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
				setExpr->SetAnyPlaceholder(true);
				$$ = setExpr;
					
			}
		;	

Proto: ProtocolOrSet Repeater Tunnel FlagNosession FlagNoack
			{
				//$1 is a list<PFLExpression>*
				//We have to create the set expression				
				PFLSetExpression *setExpr = new PFLSetExpression($1,SET_OP,$3,$2);//we specify the repeat operator in the constructor
				if (setExpr == NULL)
					throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
				setExpr->SetAnyPlaceholder(false);

						//Nosession		|| NoACK					
						if ($4 == true || $5 == true)
						{
								list<SymbolProto*> *protos = setExpr->GetProtocols();
								for(list<SymbolProto*>::iterator p = protos->begin(); p!=protos->end(); p++)
								{
									if($4)
										(*p)->NoSession=true;
									if($5)
										(*p)->NoAck=true;										
								}
						}														
			
				$$ = setExpr;
			}
			|ProtocolOrSet Tunnel FlagNosession FlagNoack
			{
				//$1 is a list<PFLExpression>*
				//We have to create the set expression
				PFLSetExpression *setExpr = new PFLSetExpression($1,SET_OP,$2);
				if (setExpr == NULL)
					throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
				setExpr->SetAnyPlaceholder(false);
				
				//Nosession		|| NoACK					
				if ($3 == true || $4 == true)
				{
					list<SymbolProto*> *protos = setExpr->GetProtocols();
					for(list<SymbolProto*>::iterator p = protos->begin(); p!=protos->end(); p++)
					{
						if($3)
							(*p)->NoSession=true;
						if($4)
							(*p)->NoAck=true;										
					}
				}														
		
				$$ = setExpr;			
			}
			;			

//[icerrato]
ProtocolOrSet: BoolOperand{
				//the current element of the header chain is a single protocol, eventually with header indexing and/or predicate, and which is not within the { }
				list<PFLExpression*> *setList = new list<PFLExpression*>();
				setList->push_back($1);
				$$ = setList;
			}
			|SetOfProtocol
			;

//we do not support repeat operator on a set
SetOfProtocol: '{' ElementOfSet '}'{ 
			$$ = $2;
			};

//a set cannot contains the any placeholder
ElementOfSet: 
		SetOrExpression
			{
				//we create a list containing the elements of the set
				list<PFLExpression*> *setList = new list<PFLExpression*>();
				setList->push_back($1);
				$$ = setList;
			}
		|  ElementOfSet ',' SetOrExpression{
			list<PFLExpression*> *setList = new list<PFLExpression*>();
			setList->push_back($3);
			for(list<PFLExpression*>::iterator it = $1->begin(); it != $1->end(); it++)
				setList->push_back(*it);
			$$ = setList;			
			//[icerrato]		
		};
			
Repeater:	PFL_ADD	
			    {
				   $$=PLUS;   
			    }
			|PFL_MUL
			    {
		 		   $$=STAR;
		  	    }
			|PFL_QUESTION
			    {
				   $$=QUESTION;
			    }
			;
			
SetOrExpression:	SetAndExpression
					|SetOrExpression PFL_OR SetAndExpression
					{
                    	PFLBinaryExpression *expr = new PFLBinaryExpression($1, $3, BINOP_BOOLOR);
						if (expr == NULL)
							throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
						$$ = expr;
				
						STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
				
						//[icerrato]
					}

					;			
					
SetAndExpression:		SetUnaryExpression
						|SetAndExpression PFL_AND SetAndExpression
						{
				           
							PFLBinaryExpression *expr = new PFLBinaryExpression($1, $3, BINOP_BOOLAND);
							if (expr == NULL)
								throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
							$$ = expr;
							
							STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
					
							//[icerrato]
						}
						;			

SetUnaryExpression:	'(' SetOrExpression ')' 
			{
				$$ = $2;
				STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
				//[icerrato]
			}

			|PFL_NOT SetUnaryExpression
			{
				PFLExpression *child = $2;
				if (child->GetType() == PFL_TERM_EXPRESSION)
				{
					$$ = MergeNotInTermNode(parserInfo, child);
				}
				else
				{
					PFLUnaryExpression *expr = new PFLUnaryExpression(child, UNOP_BOOLNOT);
					if (expr == NULL)
						throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
					
					$$ = expr;
				}
				STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
				
				//[icerrato]
			}
			|BoolOperand 
				{
					$$ = $1;
					STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
					//[icerrato]			
				}
;		


 
 			
BoolExpression:		OrExpression
				;


OrExpression:		AndExpression
					|OrExpression PFL_OR AndExpression
								{
		                             /*
		                               * Let the automaton algorithms apply the desired optimizations; do not try
		                               * to do anything at this level about predicates.
		                               *	PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ORB, $1, $3);
		                               */
		                              PFLExpression *termNode = NULL;
									if (termNode != NULL)
									{
										$$ = termNode;
									}
									else
									{
										PFLBinaryExpression *expr = new PFLBinaryExpression($1, $3, BINOP_BOOLOR);
										if (expr == NULL)
											throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
										$$ = expr;
									}
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}

					;
					
					
AndExpression:		UnaryExpression
					|AndExpression PFL_AND AndExpression
								{
                                                                  /*
                                                                   * Let the automaton algorithms apply the desired optimizations; do not try
                                                                   * to do anything at this level about predicates.
                                                                   *	PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ANDB, $1, $3);
                                                                   */
                                                                  PFLExpression *termNode = NULL;
									if (termNode != NULL)
									{
										$$ = termNode;
									}
									else
									{
										PFLBinaryExpression *expr = new PFLBinaryExpression($1, $3, BINOP_BOOLAND);
										if (expr == NULL)
											throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
										$$ = expr;
									}
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					;
					
BoolOperand:	 	Protocol
					|RelExpression 
					;		
				
FlagNosession:		/*nothing*/
				{
					$$ = false;	// protoSymbol->NoSession = false
				}
			|PFL_NOSESSION 
				{
					$$ = true;	// protoSymbol->NoSession = true
				}
				;

FlagNoack:		/*nothing*/
				{
					$$ = false;	// protoSymbol->NoAck = false
				}
			|PFL_NOACK 
				{
					$$ = true;	// protoSymbol->NoAck = true
				}
				;


sensitive:		/*nothing*/
				{
				$$ = false;
				}
			|PFL_SENSITIVE {
				$$ = true;	
				}
					;

RelExpression:		ProtoFieldOrProtofieldIndex PFL_MATCHES PFL_STRING sensitive { 
									/*
									*  PFL_PROTOFIELD|PFL_PROTOFIELD_INDEX 
									*/
									
									PFLTermExpression *protoExp = static_cast<PFLTermExpression*>($1);
									
									Node *node = protoExp->GetIRExpr();
									
									SymbolField *fieldSym = static_cast<SymbolField*>(node->Sym);
																					
									switch(fieldSym->FieldType)
										{
											case PDL_FIELD_FIXED: 
												static_cast<SymbolFieldFixed*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_VARLEN:
												static_cast<SymbolFieldVarLen*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_TOKEND:
												cout << "PFL warning: tokenended fields are deprecated" << endl;
												static_cast<SymbolFieldTokEnd*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_LINE:
												static_cast<SymbolFieldLine*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_PATTERN:
												static_cast<SymbolFieldPattern*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_EATALL:
												static_cast<SymbolFieldEatAll*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											default:
												PFL_ERROR(&yylloc, parserInfo, "Only fixed length, variable, tokenended, line, pattern and eatall fields are supported with kweword \"matches\" and \"contains\"");
										}
										
										
										
														
 									/*
									*  PFL_STRING
									*/
									
									PFLExpression *expr=GenString(parserInfo,$3);
									
									/*
									*  PFL_MATCHES
									*/
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_MATCH, /*protocol*/$1, expr); 
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}

									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
									
					}
			

			|ProtoFieldOrProtofieldIndex PFL_CONTAINS PFL_STRING sensitive {
									/*
									*  PFL_PROTOFIELD|PFL_PROTOFIELD_INDEX 
									*/
								
									PFLTermExpression *protoExp = static_cast<PFLTermExpression*>($1);
									
									Node *node = protoExp->GetIRExpr();
									
									SymbolField *fieldSym = static_cast<SymbolField*>(node->Sym);
																					
									switch(fieldSym->FieldType)
										{
											case PDL_FIELD_FIXED: 
												static_cast<SymbolFieldFixed*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_VARLEN:
												static_cast<SymbolFieldVarLen*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_TOKEND:
												cout << "PFL warning: tokenended fields are deprecated" << endl;
												static_cast<SymbolFieldTokEnd*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_LINE:
												static_cast<SymbolFieldLine*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_PATTERN:
												static_cast<SymbolFieldPattern*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											case PDL_FIELD_EATALL:
												static_cast<SymbolFieldEatAll*>(fieldSym)->Sensitive = $4;//sensitive;
												break;
											default:
												PFL_ERROR(&yylloc, parserInfo, "Only fixed length, variable, tokenended, line, pattern and eatall fields are supported with kweword \"matches\" and \"contains\"");
										}				
 									/*
									*  PFL_STRING
									*/
									
									PFLExpression *expr=GenString(parserInfo,$3);
									
									/*
									*  PFL_CONTAINS
									*/
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_CONTAINS, /*protocol*/$1, expr); 
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}

									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
									
					}


					|BitwiseExpression PFL_EQ BitwiseExpression
								{
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_EQI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BitwiseExpression PFL_NE BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_NEI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BitwiseExpression PFL_GT BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_GTI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BitwiseExpression PFL_GE BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_GEI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BitwiseExpression PFL_LT BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_LTI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BitwiseExpression PFL_LE BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_LEI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
								
					;
					
ProtoFieldOrProtofieldIndex:	PFL_PROTOFIELD
					{
					PFLExpression *protocol = ManagePFLProtoField($1,parserInfo, &yylloc);
						if(protocol == NULL)
							YYERROR;
					$$ = protocol; 
					
					}
				|PFL_PROTOFIELD_INDEX 
					{
					char *index;
					uint32 num = 0;
					char *protoName = $1, *fieldName;
					GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;									
					SymbolField *fieldSym(0);
					fieldName = strchr($1, '.');
					
					index = strchr(protoName, '%');
					
					index[strlen(index)-strlen(strchr(protoName, '.'))]='\0';
					if (index != NULL)
					{
						*index = '\0';
						index++;
						str2int(index, &num, 10);
					}


					if (fieldName != NULL)
					{
						*fieldName = '\0';
						fieldName++;
					}

					SymbolProto *protoSymbol = globalSymbols.LookUpProto(protoName);
					
					if (protoSymbol == NULL)
					{
						PFL_ERROR(&yylloc, parserInfo, "Invalid PROTOCOL identifier");

					}
					fieldSym = globalSymbols.LookUpProtoFieldByName(protoName, fieldName);
					
					if (fieldSym == NULL)
					{
						PFL_ERROR(&yylloc, parserInfo, "Field not valid for the specified protocol");
					}
					
					switch(fieldSym->FieldType)
					{
						case PDL_FIELD_TOKEND:
							cout << "PFL warning: tokenended fields are deprecated" << endl;
						case PDL_FIELD_FIXED: 
						case PDL_FIELD_VARLEN:
						case PDL_FIELD_LINE:
						case PDL_FIELD_PATTERN:
						case PDL_FIELD_EATALL:
						case PDL_FIELD_BITFIELD:
							break;
						default:
							PFL_ERROR(&yylloc, parserInfo, "Only fixed length, variable, tokenended, line, pattern, eatall and bitfield fields are supported");
					}
					HIRCodeGen &codeGen = parserInfo->CodeGen;
					Node *irExpr = codeGen.TermNode(IR_FIELD, fieldSym);
					if (irExpr == NULL)
						throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");									
					$$ = GenTermNode(protoSymbol, irExpr);
					static_cast<PFLTermExpression*>($$)->SetHeaderIndex(num);
					STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
		
					}
					;

/*ProtocolNoIndex:	PFL_PROTOCOL
				{
					GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;
					SymbolProto *protoSymbol = globalSymbols.LookUpProto($1);
				
					if (protoSymbol == NULL)
					{
						PFL_ERROR(&yylloc, parserInfo, "Invalid PROTOCOL identifier");
					}
					PFLTermExpression *expr = new PFLTermExpression(protoSymbol);
					if (expr == NULL)
						throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
				
					$$ = expr;
					STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
				};
*/
Protocol:			PFL_PROTOCOL
								{
									GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;
									SymbolProto *protoSymbol = globalSymbols.LookUpProto($1);
									
									if (protoSymbol == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Invalid PROTOCOL identifier");
									}
									PFLTermExpression *expr = new PFLTermExpression(protoSymbol);
									if (expr == NULL)
										throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
									
									$$ = expr;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
								
					|PFL_PROTOCOL_INDEX
								{
									char *index;
									uint32 num = 0;
									GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;
									
									index = strchr($1, '%');
									if (index != NULL)
									{
										*index = '\0';
										index++;
										str2int(index, &num, 10);
									}
									
									SymbolProto *protoSymbol = globalSymbols.LookUpProto($1);
									
									if (protoSymbol == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Invalid PROTOCOL identifier");
									}
									
								
									PFLTermExpression *expr = new PFLTermExpression(protoSymbol, num);
									if (expr == NULL)
										throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
									
									$$ = expr;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					;
					
BitwiseExpression:	ShiftExpression
					|BitwiseExpression PFL_BWAND ShiftExpression	
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ANDI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");

									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");

									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BitwiseExpression PFL_BWOR ShiftExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ORI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|BitwiseExpression PFL_BWXOR ShiftExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_XORI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|PFL_BWNOT ShiftExpression
								{
									PFLTermExpression *termNode = (PFLTermExpression*)$2;
									HIRCodeGen &codeGen = parserInfo->CodeGen;
									termNode->SetIRExpr(codeGen.UnOp(IR_NOTI, static_cast<HIRNode*>(termNode->GetIRExpr())));
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|'('BitwiseExpression')'
								{
									$$ = $2;
								}
					
					;

ShiftExpression:	AddExpression
					|ShiftExpression PFL_SHL AddExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_SHLI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|ShiftExpression PFL_SHR AddExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_SHRI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}

					;
					
AddExpression:		MulExpression
					|AddExpression PFL_ADDADD MulExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ADDI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
					|AddExpression PFL_SUB MulExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_SUBI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}

					;

MulExpression:		Term
					|MulExpression PFL_MULMUL MulExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_MULI, $1, $3);
									if (termNode == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Operations between fields of different protocols are not supported");
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										PFL_ERROR(&yylloc, parserInfo, "type mismatch in expression");
									}
									$$ = termNode;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
						;
					
Term:				GenericNumber	
								{									
									$$ = GenNumber(parserInfo, $1);
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
								
								
					|PFL_IPV4ADDR	{
									uint32 add = 0;
									if (IPaddr2int($1, &add) != 0)
									{
										PFL_ERROR(&yylloc, parserInfo, "IP address not valid!");
									}
									$$ = GenNumber(parserInfo, add);
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
								
					|PFL_MACADDR   {
			
									uint32 add = 0;
									if (MACaddr2int($1, &add) != 0)
									{
										PFL_ERROR(&yylloc, parserInfo, "MAC address not valid!");
									}
									$$ = GenNumber(parserInfo, add);
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
									PFL_ERROR(&yylloc, parserInfo, "MAC address are not supported!");
								}
					|PFL_PROTOFIELD {
									char *protoName = $1, *fieldName;
									GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;									
									SymbolField *fieldSym(0);
									fieldName = strchr($1, '.');

									if (fieldName != NULL)
									{
										*fieldName = '\0';
										fieldName++;
									}
									
									SymbolProto *protoSymbol = globalSymbols.LookUpProto(protoName);
									
									if (protoSymbol == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Invalid PROTOCOL identifier");

									}
									fieldSym = globalSymbols.LookUpProtoFieldByName(protoName, fieldName);
									
									if (fieldSym == NULL)
									{
										PFL_ERROR(&yylloc, parserInfo, "Field not valid for the specified protocol");
									}
									
									switch(fieldSym->FieldType)
									{
										case PDL_FIELD_TOKEND:
											cout << "PFL warning: tokenended fields are deprecated" << endl;
										case PDL_FIELD_FIXED: 
										case PDL_FIELD_VARLEN:
										case PDL_FIELD_LINE:
										case PDL_FIELD_PATTERN:
										case PDL_FIELD_EATALL:
										case PDL_FIELD_BITFIELD:
											break;
										default:
											PFL_ERROR(&yylloc, parserInfo, "Only fixed length, variable, tokenended, line, pattern, eatall and bitfield fields are supported");
									}
									HIRCodeGen &codeGen = parserInfo->CodeGen;
									Node *irExpr = codeGen.TermNode(IR_FIELD, fieldSym);
									if (irExpr == NULL)
										throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");									
									$$ = GenTermNode(protoSymbol, irExpr);
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
								
/*								
					|PFL_IPV6ADDR
*/
					|PFL_PROTOFIELD_INDEX 
								{
								char *index;
								uint32 num = 0;
								char *protoName = $1, *fieldName;
								GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;									
								SymbolField *fieldSym(0);
								fieldName = strchr($1, '.');
								
								index = strchr(protoName, '%');
								
								index[strlen(index)-strlen(strchr(protoName, '.'))]='\0';
								if (index != NULL)
								{
									*index = '\0';
									index++;
									str2int(index, &num, 10);
								}

								if (fieldName != NULL)
								{
									*fieldName = '\0';
									fieldName++;
								}

								SymbolProto *protoSymbol = globalSymbols.LookUpProto(protoName);
								
								if (protoSymbol == NULL)
								{
									PFL_ERROR(&yylloc, parserInfo, "Invalid PROTOCOL identifier");

								}
								fieldSym = globalSymbols.LookUpProtoFieldByName(protoName, fieldName);
								
								if (fieldSym == NULL)
								{
									PFL_ERROR(&yylloc, parserInfo, "Field not valid for the specified protocol");
								}
								
								switch(fieldSym->FieldType)
								{
									case PDL_FIELD_TOKEND:
										cout << "PFL warning: tokenended fields are deprecated" << endl;
									case PDL_FIELD_FIXED: 
									case PDL_FIELD_VARLEN:
									case PDL_FIELD_LINE:
									case PDL_FIELD_PATTERN:
									case PDL_FIELD_EATALL:
									case PDL_FIELD_BITFIELD:
										break;
									default:
										PFL_ERROR(&yylloc, parserInfo, "Only fixed length, variable, tokenended, line, pattern, eatall and bitfield fields are supported");
								}
								HIRCodeGen &codeGen = parserInfo->CodeGen;
								Node *irExpr = codeGen.TermNode(IR_FIELD, fieldSym);
								if (irExpr == NULL)
									throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");									
								$$ = GenTermNode(protoSymbol, irExpr);
								static_cast<PFLTermExpression*>($$)->SetHeaderIndex(num);
								STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
							
							
								}
					|PFL_PROTOCOL'['GenericNumber']'
								{
									PFLExpression *expr = GenProtoBytesRef(&yylloc, parserInfo, $1, $3, 1);
									if (expr == NULL)
										YYERROR;
									$$ = expr;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								
								}
					|PFL_PROTOCOL'['GenericNumber':'GenericNumber']' 
								{
									PFLExpression *expr = GenProtoBytesRef(&yylloc, parserInfo, $1, $3, $5);
									if (expr == NULL)
										YYERROR;
									$$ = expr;
									STORE_EXP_TEXT($$, @$.first_column, @$.first_line, @$.last_column, @$.last_line);
								}
						
					;
					
					
GenericNumber:		PFL_NUMBER
								{
									uint32 num = 0;
									if (str2int($1, &num, 10) != 0)
									{
										PFL_ERROR(&yylloc, parserInfo, "Decimal number out of range");
									}
									
									$$ = num;
								}
									
					|PFL_HEXNUMBER		
								{
									uint32 num = 0;
									if (str2int($1, &num, 16) != 0)
									{
										PFL_ERROR(&yylloc, parserInfo, "Hexadecimal number out of range");
									}
									
									$$ = num;
								}	
					
%%

SymbolField* CheckField(ParserInfo *parserInfo, char * protoName, YYLTYPE *llocp, uint32 headerIndex)
{
	char *fieldName;
	GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;									
	//SymbolField *fieldSym(0);
	fieldName = strchr(protoName, '.');

	if (fieldName != NULL)
	{
		*fieldName = '\0';
		fieldName++;
	}

	SymbolProto *protoSymbol = globalSymbols.LookUpProto(protoName);
	
	if (protoSymbol == NULL)
	{
		return NULL;
	}

	if(strcmp(fieldName,"allfields")==0)
   	{	
   		if(protocolsForExtraction.count(make_pair<SymbolProto*,uint32>(protoSymbol,headerIndex))!=0)
   		{
   			pfl_error(llocp, parserInfo, "You cannot specify a protocol two times for the field extraction, if the second of them is on \"allfields\"");
   			return NULL;
   		}
   		SymbolField *sym = new SymbolField(fieldName,PDL_FIELD_ALLFIELDS,NULL);
       	sym->Protocol=protoSymbol;
       	globalSymbols.StoreProtoField(protoSymbol, sym);
		return sym;
   	}
    else
    {
    	protocolsForExtraction.insert(make_pair<SymbolProto*,uint32>(protoSymbol,headerIndex)); //this set is needed in order to verify that the protocol with all field is not present in the extractfields yet
		return globalSymbols.LookUpProtoFieldByName(protoName, fieldName);
	}
	
}

PFLExpression *GenUnOpNode(PFLOperator op, PFLExpression *kid)
{
	PFLUnaryExpression *expr = new PFLUnaryExpression(kid, op);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}


PFLExpression *GenBinOpNode(PFLOperator op, PFLExpression *kid1, PFLExpression *kid2)
{
	PFLBinaryExpression *expr = new PFLBinaryExpression(kid1, kid2, op);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}

PFLExpression *GenTermNode(SymbolProto *protocol, Node *irExpr)
{
	PFLTermExpression *expr = new PFLTermExpression(protocol, irExpr);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}

PFLExpression *MergeNotInTermNode(ParserInfo *parserInfo, PFLExpression *node1)
{
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(node1 != NULL, "node1 cannot be NULL");
	nbASSERT(node1->GetType() == PFL_TERM_EXPRESSION, "node1 should be a terminal node");
	PFLTermExpression *kid1 = (PFLTermExpression*)node1;

	PFLUnaryExpression *expr = new PFLUnaryExpression(kid1, UNOP_BOOLNOT);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}


HIRNode *ConvToInt(ParserInfo *parserInfo, HIRNode *node)
{
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(node != NULL, "Node cannot be NULL");
	if (CheckIntOperand(node))
	{
		switch(node->Op)
		{
			case IR_ICONST:
			case IR_IVAR:
				return node;
				break;
			case IR_SVAR:
			case IR_FIELD:
			{
				HIRCodeGen &codeGen = parserInfo->CodeGen;
				node->NRefs--; //otherwise it will appear as a dag and this is not the case
				return codeGen.UnOp(IR_CINT, node);
			}break;
			default:
				return node;
				break;
		}
	}
	return NULL;
}

HIRNode *StringToString(ParserInfo *parserInfo, HIRNode *node)
{
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(node != NULL, "Node cannot be NULL");

	switch(node->Op)
	{
			return node;
			break;
		case IR_SVAR:
		case IR_FIELD:
		{	//SymbolField *sf=(SymbolField*)node->Sym;		
			HIRCodeGen &codeGen = parserInfo->CodeGen;
			node->NRefs--; //otherwise it will appear as a dag and this is not the case
			return codeGen.UnOp(IR_CINT, node);
		}break;
		default:
			return node;
			break;
	}
	return node;
}

bool CheckOperandTypes(ParserInfo *parserInfo, PFLExpression *expr)
{
	nbASSERT(expr->GetType() == PFL_TERM_EXPRESSION, "expression should be a terminal node");
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(expr != NULL, "expr cannot be NULL");
	nbASSERT(expr->GetType() == PFL_TERM_EXPRESSION, "expr should be a terminal node");
	HIRNode *node = static_cast<HIRNode*>(((PFLTermExpression*)expr)->GetIRExpr());
	nbASSERT(node != NULL, "Contained Node cannot be NULL");
	
	if (GET_OP_TYPE(node->Op) == IR_TYPE_INT)
	{      
		HIRNode *leftChild = node->GetLeftChild();
		HIRNode *rightChild = node->GetRightChild();
		if (leftChild)
		{
			leftChild = ConvToInt(parserInfo, leftChild);
			if(leftChild == NULL)
			{
				return false;
			} 
			node->SetLeftChild(leftChild);
		}
		if (rightChild)
		{		
			rightChild = ConvToInt(parserInfo, rightChild);
			if(rightChild == NULL)
			{
				return false;
			} 
			node->SetRightChild(rightChild);
		}
			
		return true;
	}
	
	else if (GET_OP_TYPE(node->Op) == IR_TYPE_STR)
	{
	
		HIRNode *leftChild = node->GetLeftChild();
		HIRNode *rightChild = node->GetRightChild();
		if (leftChild)
		{
			leftChild = StringToString(parserInfo, leftChild);
			if(leftChild == NULL)
			{
				return false;
			} 
			node->SetLeftChild(leftChild);
		}
		if (rightChild)
		{		
			rightChild = StringToString(parserInfo, rightChild);
			if(rightChild == NULL)
			{
				return false;
			} 
			node->SetRightChild(rightChild);
		}
		
		return true; 		
	}	
	
	nbASSERT(false, "SHOULD NOT BE HERE");
	return true;
} 

PFLExpression *MergeTermNodes(ParserInfo *parserInfo, HIROpcodes op, PFLExpression *node1, PFLExpression *node2)
{
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(node1 != NULL, "node1 cannot be NULL");
	nbASSERT(node2 != NULL, "node2 cannot be NULL");
	//node1 should be a terminal node
	if(node1->GetType() != PFL_TERM_EXPRESSION)
		return NULL;
	//node2 should be a terminal node
	if(node2->GetType() != PFL_TERM_EXPRESSION)	
		return NULL;
	nbASSERT(node2->GetType() == PFL_TERM_EXPRESSION, "node2 should be a terminal node");
	PFLTermExpression *kid1 = (PFLTermExpression*)node1;
	PFLTermExpression *kid2 = (PFLTermExpression*)node2;

	// save protocol information about the two terminal nodes
	SymbolProto *proto1 = kid1->GetProtocol();
	SymbolProto *proto2 = kid2->GetProtocol();
	
	// save the IR trees of the two terminal nodes
	HIRNode *irExpr1 = static_cast<HIRNode*>(kid1->GetIRExpr()); //i.e. IR_FILED
	HIRNode *irExpr2 = static_cast<HIRNode*>(kid2->GetIRExpr()); //i.e. IR_SCONST
	
	if (proto1 != proto2)
	{
		// the two terminal nodes refer to different protocols
		if (proto2 == NULL)
		{
			nbASSERT(irExpr2 != NULL, "irExpr2 cannot be NULL");
			// if the second has a NULL protocol (i.e. it contains a constant expression) we delete it
			kid2->SetIRExpr(NULL);
			delete kid2;
		}
		else if (proto1 == NULL)
		{
			nbASSERT(irExpr1 != NULL, "irExpr1 cannot be NULL");
			// if the first has a NULL protocol (i.e. it contains a constant expression) we copy the protocol 
			// information from the second, then we delete the second
			kid1->SetProtocol(proto2);
			kid2->SetIRExpr(NULL);
			delete kid2;
		}
		else
		{
			//The two terminal nodes cannot be merged in a single PFLTermExpression node
			return NULL;
		}
	}
	else
	{
			// the two terminal nodes refer to the same protocol
			// so we simply delete the second
			kid2->SetIRExpr(NULL);
			delete kid2;
	}
	
	//handle the cases such as (ip and (ip.src == 10.0.0.1)) where one operand is redundant
	
	if (irExpr1 == NULL)
	{
		kid1->SetIRExpr(irExpr2);
		return kid1;
	}
	if (irExpr2 == NULL)
	{
		kid1->SetIRExpr(irExpr1);
		return kid1;
	}
	
	// we create a new Node as an op between the two subexpressions
	// and we append it to the first terminal node

	HIRCodeGen &codeGen = parserInfo->CodeGen;
	kid1->SetIRExpr(codeGen.BinOp(op, irExpr1, irExpr2));
	return kid1;
}

PFLExpression *GenNumber(struct ParserInfo *parserInfo, uint32 value)
{
	HIRCodeGen &codeGen = parserInfo->CodeGen;
	HIRNode *irExpr = codeGen.TermNode(IR_ICONST, codeGen.ConstIntSymbol(value));
	return GenTermNode(NULL, irExpr);
}

PFLExpression *GenString(struct ParserInfo *parserInfo, string value)
{
	HIRCodeGen &codeGen = parserInfo->CodeGen;
	HIRNode *irExpr = codeGen.TermNode(IR_SCONST,codeGen.ConstStrSymbol(value));
	return GenTermNode(NULL, irExpr);

}


PFLExpression *GenProtoBytesRef(YYLTYPE *llocp, ParserInfo *parserInfo, char *protocol, uint32 offs, uint32 len)
{
	GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;
	HIRCodeGen &codeGen = parserInfo->CodeGen;
	SymbolProto *protoSymbol = globalSymbols.LookUpProto(protocol);

	if (protoSymbol == NULL)
	{
		pfl_error(llocp, parserInfo, "Invalid PROTOCOL identifier");
		return NULL;
	}

	HIRNode *irExpr = codeGen.ProtoBytesRef(protoSymbol, offs, len);

	if (irExpr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return GenTermNode(protoSymbol, irExpr);
}

PFLExpression *ManagePFLProtoField(char *protoName, ParserInfo *parserInfo, YYLTYPE *llocp){ 
	char *fieldName;
	GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;									
	SymbolField *fieldSym(0);
	fieldName = strchr(protoName, '.');

	if (fieldName != NULL)
	{
		*fieldName = '\0';
		fieldName++;
	}
	
	SymbolProto *protoSymbol = globalSymbols.LookUpProto(protoName);
		
	if (protoSymbol == NULL)
	{
		pfl_error(llocp, parserInfo, "Invalid PROTOCOL identifier");
		return NULL;

	}
	fieldSym = globalSymbols.LookUpProtoFieldByName(protoName, fieldName);
										
	if (fieldSym == NULL)
	{
		pfl_error(llocp, parserInfo, "The specified field is not valid for the specified protocol");
		return NULL;
	}
			
	HIRCodeGen &codeGen = parserInfo->CodeGen;
	Node *irExpr = codeGen.TermNode(IR_FIELD, fieldSym);
							
	if (irExpr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");									
	
	return GenTermNode(protoSymbol, irExpr); 
}

int pfl_error(YYLTYPE *llocp, struct ParserInfo *parserInfo, const char *s)
{
	parserInfo->ErrRecorder->PFLError(s);
	//fprintf(stderr, "%s\n", s);
	return 1;
}


void compile(ParserInfo *parserInfo, const char *filter, int debug)
{

#ifdef YYDEBUG
#if YYDEBUG != 0
	pfl_debug = debug;
#endif
#endif
	parserInfo->FilterString = string(filter);
	parserInfo->ResetFilter();
	pflcompiler_lex_init(filter);
	pfl_parse(parserInfo);
	pflcompiler_lex_cleanup();

} 
