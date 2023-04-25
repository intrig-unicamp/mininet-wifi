/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "pflexpression.h"
#include "dump.h"
#include <sstream>


// static variables initialization
uint32 PFLExpression::m_Count = 0;


//!\ brief literal values corresponding to the elements of the \ref PFLOperator enumeration

const char *PFLOperators[] =
{
	"and",			///< Logical AND
	"or",			///< Logical OR
	"boolnot",		///< Boolean negation
};






PFLStatement::PFLStatement(PFLExpression *exp, PFLAction *action, PFLIndex *index, bool fullencap)
	:m_Exp(exp), m_Action(action), m_HeaderIndex(index), m_FullEncap(fullencap)
{
}
	
PFLStatement::~PFLStatement()
{
	delete m_Exp;
	delete m_Action;
	delete m_HeaderIndex;
}


PFLExpression *PFLStatement::GetExpression(void)
{
	return m_Exp;
}


PFLAction *PFLStatement::GetAction(void)
{
	return m_Action;
}

PFLIndex *PFLStatement::GetHeaderIndex(void) //[icerrato]
{
	return m_HeaderIndex;
}

bool PFLStatement::IsFullEncap()
{
	return m_FullEncap;
}

PFLAction::~PFLAction()
{

}

/***************************************** BINARY *****************************************/

PFLBinaryExpression::PFLBinaryExpression(PFLExpression *LeftExpression, PFLExpression *RightExpression, PFLOperator Operator)
	:PFLExpression(PFL_BINARY_EXPRESSION), m_LeftNode(LeftExpression), m_RightNode(RightExpression), m_Operator(Operator)
{
}

PFLBinaryExpression::~PFLBinaryExpression()
{
	if (m_LeftNode != NULL)
		delete m_LeftNode;

	if (m_RightNode != NULL)
		delete m_RightNode;
}


void PFLBinaryExpression::SwapChilds()
{
	PFLExpression *tmp = m_LeftNode;
	m_LeftNode = m_RightNode;
	m_RightNode = tmp;
}

string PFLBinaryExpression::ToString()
{
	if (m_Text.size() > 0)
		return m_Text;

	string s(PFLOperators[m_Operator]);

	return "( " + m_LeftNode->ToString() + " " + s + " " + m_RightNode->ToString() + " )";
}

void PFLBinaryExpression::Printme(int level)
{
	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"-->PFLBinaryExpression\n");		

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr," LeftChild\n");		

	if (m_LeftNode != NULL)
	{

		m_LeftNode->Printme(level+1);
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL\n");
	}		

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr," RightChild\n");		

	if (m_RightNode != NULL)
	{

		m_RightNode->Printme(level+1);
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL\n");
	}		


	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"<--PFLBinaryExpression\n");		
}

void PFLBinaryExpression::PrintMeDotFormat(ostream &outFile)
{
	outFile << "expr" << GetID() << "[label=\"" << PFLOperators[m_Operator] << "\"];" << endl;
	m_LeftNode->PrintMeDotFormat(outFile);
	outFile << "expr" << GetID() << "->expr" << m_LeftNode->GetID() << ";" << endl;
	m_RightNode->PrintMeDotFormat(outFile);
	outFile << "expr" << GetID() << "->expr" << m_RightNode->GetID() << ";" << endl;
	
}

PFLExpression *PFLBinaryExpression::Clone()
{
	PFLBinaryExpression *expr = new PFLBinaryExpression(m_LeftNode->Clone(), m_RightNode->Clone(), m_Operator);
	expr->SetExprText(this->m_Text);
	return expr;
}

void PFLBinaryExpression::ToCanonicalForm()
{
	m_LeftNode->ToCanonicalForm();
	m_RightNode->ToCanonicalForm();

	if (m_LeftNode->IsConst() && !m_RightNode->IsConst())
		SwapChilds();
}

string PFLBinaryExpression::getAttribute(){
  // try harder by descending into the child nodes.
  // return the attribute if the search was successful and consistent
  
  string left = this->m_LeftNode->getAttribute();
  string right = this->m_RightNode->getAttribute();


  if (left != right)
    nbASSERT(false,"Unhandled situation");

  return left;
}

SymbolProto* PFLBinaryExpression::GetProtocol(){
  // try harder by descending into the child nodes.
  // return the attribute if the search was successful and consistent
  SymbolProto* left = this->m_LeftNode->GetProtocol();
  SymbolProto* right = this->m_RightNode->GetProtocol();

  if (left != right)
  	//it is impossible to assign a protocol to this element
  	return NULL;

  return left;
}

int PFLBinaryExpression::GetHeaderIndex(void)
{
	uint32 left = this->m_LeftNode->GetHeaderIndex();
    uint32 right = this->m_RightNode->GetHeaderIndex();
    
     if (left != right)
	  	//it is impossible to assign an header indexing to this element
  		return -1;

  	return left;
}

bool PFLBinaryExpression::HasPredicate(void)
{
	bool left = this->m_LeftNode->HasPredicate();
    bool right = this->m_RightNode->HasPredicate();
    
	return (left && right);     
}


/***************************************** UNARY *****************************************/

PFLUnaryExpression::PFLUnaryExpression(PFLExpression *Expression, PFLOperator Operator)
	:PFLExpression(PFL_UNARY_EXPRESSION), m_Operator(Operator), m_Node(Expression)
{
}

PFLUnaryExpression::~PFLUnaryExpression()
{
	if (m_Node != NULL)
		delete m_Node;
}

string PFLUnaryExpression::ToString()
{
	if (m_Text.size() > 0)
		return m_Text;

	string s(PFLOperators[m_Operator]);
	return s + m_Node->ToString();
}

PFLExpression *PFLUnaryExpression::Clone()
{
	PFLUnaryExpression *expr = new PFLUnaryExpression(m_Node->Clone(), m_Operator);
	expr->SetExprText(this->m_Text);
	return expr;
}

void PFLUnaryExpression::Printme(int level)
{
	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"-->PFLUnaryExpression");		
	fprintf(stderr, " %s\n", PFLOperators[m_Operator]);
	
	if (m_Node != NULL)
	{
		m_Node->Printme(level+1);
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL\n");
	}		

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"<--PFLUnaryExpression\n");		
}

void PFLUnaryExpression::PrintMeDotFormat(ostream &outFile)
{
	outFile << "expr" << GetID() << "[label=\"" << PFLOperators[m_Operator] << "\"];" << endl;
	m_Node->PrintMeDotFormat(outFile);
	outFile << "expr" << GetID() << "->expr" << m_Node->GetID() << ";" << endl;
}

void PFLUnaryExpression::ToCanonicalForm()
{
	m_Node->ToCanonicalForm();
	return;
}

string PFLUnaryExpression::getAttribute()
{
  return m_Node->getAttribute();
}

SymbolProto* PFLUnaryExpression::GetProtocol(){
	return this->m_Node->GetProtocol();
}


int PFLUnaryExpression::GetHeaderIndex(void)
{	
	return this->m_Node->GetHeaderIndex();
}	

bool PFLUnaryExpression::HasPredicate(void)
{	
	return this->m_Node->HasPredicate();
}	


/***************************************** TERM *****************************************/

//PFLTermExpression
PFLTermExpression::PFLTermExpression(SymbolProto *proto, Node *expr)
  :PFLExpression(PFL_TERM_EXPRESSION), m_Protocol(proto), m_HeaderIndex(0) ,m_IRExpr(expr), m_Value(false)
{
}

PFLTermExpression::PFLTermExpression(SymbolProto *proto, uint32 indexing, Node *expr)
	:PFLExpression(PFL_TERM_EXPRESSION), m_Protocol(proto), m_HeaderIndex(indexing), m_IRExpr(expr), m_Value(false)
{
}

PFLTermExpression::PFLTermExpression(bool constVal)
  :PFLExpression(PFL_TERM_EXPRESSION, true), m_Protocol(NULL), m_HeaderIndex(0), m_IRExpr(NULL), m_Value(constVal)
{
}

PFLTermExpression::~PFLTermExpression()
{
}

PFLTermExpression *PFLTermExpression::Clone()
{
	PFLTermExpression *expr = NULL;
	if (IsConst())
		expr = new PFLTermExpression(m_Value);
	else
		expr = new PFLTermExpression(m_Protocol, m_IRExpr);
	expr->SetExprText(this->m_Text);
	expr->SetHeaderIndex(this->m_HeaderIndex);
	return expr;
}

string PFLTermExpression::ToString()
{
	string s;
	if (IsConst())
	{
		if (m_Value)
			return "true";
		else
			return "false";
	}


	if (PFLExpression::m_Text.size() > 0)
		return m_Text;


	s = m_Protocol->ToString();
	if (m_IRExpr)
	{
		std::stringstream ss;
		CodeWriter codeWriter(ss);
		codeWriter.DumpTree(m_IRExpr, 0);
		s += string("[") + ss.str() + string("]");
	}

	return s;
}

void PFLTermExpression::Printme(int level)
{
	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"-->PFLTermExpression\n");		
	
	nbASSERT(m_Protocol != NULL, "Protocol cannot be NULL");

	if (m_Protocol != NULL)
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"%s\n", m_Protocol->Name.c_str());
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL PROTO\n");
	}		

	if (m_IRExpr != NULL)
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");

		//DumpTree(stderr, m_IRExpr, 0);
	}
	

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"<--PFLTermExpression\n");		
}

void PFLTermExpression::PrintMeDotFormat(ostream &outFile)
{
	nbASSERT(!(m_Protocol == NULL && m_IRExpr == NULL), "Terminal node should refer to at least one protocol or to a valid IR expression tree");
	outFile << "expr" << GetID() << "[label=\"";
	if (m_Protocol != NULL)
		outFile << "proto: " << m_Protocol->Name << "\\n";
	if (m_IRExpr != NULL)
	{
		outFile << "cond: ";
		CodeWriter codeWriter(outFile);
		codeWriter.DumpTree(m_IRExpr, 0);
	}
	outFile << "\"];" << endl;
}

void PFLTermExpression::ToCanonicalForm()
{
	return;
}

string PFLTermExpression::getAttribute()
{

  if (!m_IRExpr) // this happens then this.equals(::One())
    return "";

  /* oh, baby, this is a dirty, dirty hack
   * (to avoid swapped constant and protocol field)
   */
  struct SymbolField* tmp = (struct SymbolField*) (m_IRExpr->Kids[0]->Sym == NULL?
                                                   m_IRExpr->Kids[0]->Kids[0]->Sym :
                                                   m_IRExpr->Kids[1]->Kids[0]->Sym);
  return (m_Protocol->Name + "." + tmp->Name);
}


PFLExpression::~PFLExpression()
{
}

/***************************************** REGEXP *****************************************/

PFLRegExpExpression::PFLRegExpExpression(list<PFLSetExpression*> *InnerList, PFLOperator Operator)
	:PFLExpression(PFL_REGEXP_EXPRESSION), m_InnerList(InnerList), m_Operator(Operator)
{
}



PFLRegExpExpression::~PFLRegExpExpression()
{
	if (m_InnerList != NULL){
		delete m_InnerList;
	}
}

string PFLRegExpExpression::ToString()
{
	if (m_Text.size() > 0)
		return m_Text;

	string s(PFLOperators[m_Operator]);
	
	stringstream ss;
	ss << "( ";
	if (m_InnerList != NULL){
		list<PFLSetExpression*>::iterator it = m_InnerList->begin();
		for (; it != m_InnerList->end(); it++ ){
			ss << s << " " << (*it)->ToString() << " ";
		}		
	}

	return ss.str();
}

void PFLRegExpExpression::Printme(int level)
{ 
	nbASSERT(false,"Method \"void PFLRegExpExpression::Printme(int level)\" not implemented yet");	
}

void PFLRegExpExpression::PrintMeDotFormat(ostream &outFile)
{
	nbASSERT(false,"Method \"void PFLRegExpExpression::PrintMeDotFormat(ostream &outFile)\" not implemented yet");
	
}

PFLExpression *PFLRegExpExpression::Clone()
{
	PFLRegExpExpression *expr = new PFLRegExpExpression(new list<PFLSetExpression*>(m_InnerList->begin(),m_InnerList->end()), m_Operator);
	expr->SetExprText(this->m_Text);
	return expr;
}

void PFLRegExpExpression::ToCanonicalForm()
{
	nbASSERT(false,"Method \"void PFLRegExpExpression::ToCanonicalForm()\" not implemented yet");

}

string PFLRegExpExpression::getAttribute(){
	nbASSERT(false,"Method \"string PFLRegExpExpression::getAttribute(\" not implemented yet");

	return NULL;
}


SymbolProto* PFLRegExpExpression::GetProtocol()
{
	nbASSERT(m_InnerList->size()==1,"This situation is still not implemented");
	PFLSetExpression *set = *(m_InnerList->begin());
	return set->GetProtocol();
}

int PFLRegExpExpression::GetHeaderIndex()
{
	nbASSERT(m_InnerList->size()==1,"This situation is still not implemented");
	PFLSetExpression *set = *(m_InnerList->begin());
	return set->GetHeaderIndex();
}

bool PFLRegExpExpression::HasPredicate()
{
	nbASSERT(false,"you cannot be here!");
	return false;
}

		
/***************************************** SET *****************************************/

PFLSetExpression::PFLSetExpression(list<PFLExpression*> *Elements, PFLOperator Operator, bool mandatoryTunnels, PFLRepeatOperator Repeat, PFLInclusionOperator Inclusion)
	:PFLExpression(PFL_SET_EXPRESSION), m_Elements(Elements),m_Operator(Operator),m_MandatoryTunnels(mandatoryTunnels),m_Inclusion(Inclusion),m_Repeat(Repeat)
{
}

PFLSetExpression::~PFLSetExpression()
{
	if (m_Elements != NULL){
		delete m_Elements;
	}
}

string PFLSetExpression::ToString()
{
	if (m_Text.size() > 0)
		return m_Text;

	string s(PFLOperators[m_Operator]);
	cout << s << " " << endl;
	
	stringstream ss;
	ss << "{";
	if (m_Elements != NULL){
		list<PFLExpression*>::iterator it;
		for (it=m_Elements->begin() ; it != m_Elements->end(); it++ ){
			if(it != m_Elements->begin())
				ss << ",";
			ss << " " << (*it)->ToString() << " ";
		}		
	}
	ss << "}";

	return ss.str();
}

void PFLSetExpression::Printme(int level)
{ 
	nbASSERT(false,"Method \"void PFLRegExpExpression::Printme(int level)\" not implemented yet");	
}

void PFLSetExpression::PrintMeDotFormat(ostream &outFile)
{
	nbASSERT(false,"Method \"void PFLRegExpExpression::PrintMeDotFormat(ostream &outFile)\" not implemented yet");
	
}

PFLExpression *PFLSetExpression::Clone()
{
	PFLSetExpression *expr = new PFLSetExpression(new list<PFLExpression*>(m_Elements->begin(),m_Elements->end()), m_Operator, m_MandatoryTunnels, m_Repeat, m_Inclusion);
	expr->SetExprText(this->m_Text);
	expr->SetAnyPlaceholder(m_AnyPlaceholder);
	return expr;
}

void PFLSetExpression::ToCanonicalForm()
{
	nbASSERT(false,"Method \"void PFLRegExpExpression::ToCanonicalForm()\" not implemented yet");

}

string PFLSetExpression::getAttribute(){
	nbASSERT(false,"Method \"string PFLRegExpExpression::getAttribute()\" not implemented yet");

	return NULL;
}

SymbolProto *PFLSetExpression::GetProtocol()
{
	nbASSERT(m_Elements->size()==1,"This situation is still not implemented");	
	PFLExpression * expr = *(m_Elements->begin());	
	return expr->GetProtocol();
}

list<SymbolProto*> *PFLSetExpression::GetProtocols()
{
//TEMP
	list<SymbolProto*> *allproto = new list<SymbolProto*>();
	for(list<PFLExpression*>::iterator allElements = (m_Elements->begin()) ; allElements != (m_Elements->end()); allElements++)
	{
		PFLExpression *elem = (PFLExpression *)*allElements;
		if(elem->GetType() == PFL_TERM_EXPRESSION)		
		{
			PFLTermExpression *temp = (PFLTermExpression *)(*allElements);
			(*allproto).push_back(temp->GetProtocol());
		}
	}
	return allproto;
}

int PFLSetExpression::GetHeaderIndex()
{
	nbASSERT(m_Elements->size()==1,"This situation is still not implemented");
	PFLExpression * expr = *(m_Elements->begin());
	return expr->GetHeaderIndex();
}

bool PFLSetExpression::HasPredicate()
{
	nbASSERT(false,"you cannot be here!");
	return false;
}
