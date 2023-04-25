/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "symbols.h"
#include "tree.h"
#include <list>
#include <iostream>
#include "sft/librange/common.h" // for RangeOperator_t

using namespace std;

/*!
\brief Possible types for a boolean expression.

Each element of the enumeration should match to a subclass of
PFLExpression
*/
enum PFLExpressionType
{
	PFL_BINARY_EXPRESSION,	///< Binary Expression (for AND, OR operators)
	PFL_UNARY_EXPRESSION,	///< Unary Expression (for NOT operator)
	PFL_TERM_EXPRESSION,	///< A term of a boolean expression
	PFL_REGEXP_EXPRESSION,
	PFL_SET_EXPRESSION
};


/*!
\brief Possible operators for a boolean expression
*/
enum PFLOperator
{
	BINOP_BOOLAND,		///< Logical AND
	BINOP_BOOLOR,		///< Logical OR
	UNOP_BOOLNOT,
	REGEXP_OP,
	SET_OP
};

enum PFLInclusionOperator //[icerrato]
{
	NOTIN,
	IN_OP,				//Unfortunately, the 'IN' keyword is reserved in Windows
	DEFAULT_INCLUSION
};

enum PFLRepeatOperator //[icerrato]
{
	PLUS = 0,
	STAR,
	QUESTION,
	DEFAULT_ROP
};


/*!
\brief Types for a PFL action
*/
enum PFLActionType

{
	PFL_RETURN_PACKET_ACTION,
	PFL_EXTRACT_FIELDS_ACTION,
	PFL_CLASSIFY_ACTION
};


class PFLExpression; //fw decl
class PFLAction; //fw decl
class PFLIndex;

/*!
\brief This class represents a single PFL statement, which is composed of a boolean filtering expression
and a corresponding action, like return packet or extract fields
*/
class PFLStatement
{
	PFLExpression	*m_Exp;			//The filtering expression
	PFLAction		*m_Action;		//The corresponding action
	PFLIndex		*m_HeaderIndex;	//The index header selected //[icerrato] this variable is useless... it must be associated to the PFLTermExpression
	bool			m_FullEncap; 	// true in case of the netPFL keyword has been specified in the filter

public:
	/*!
	\brief constructor
	\param exp pointer to a filtering expression object
	\param action pointer to an action object
	\param fullencap is true if the NetPFL keyword "fullencap" has been specified in the filter
	*/
	PFLStatement(PFLExpression *exp, PFLAction *action, PFLIndex *headerindex,bool fullencap = false);

	/*!
	\brief destructor
	*/
	~PFLStatement();

	/*!
	\brief Returns the filtering expression of this statement
	*/
	PFLExpression *GetExpression(void);

	/*!
	\brief Returns the action of this statement
	*/
	PFLAction *GetAction(void);

	PFLIndex *GetHeaderIndex(void); 

	bool IsFullEncap(); 
};


/*!
\brief This class represents a generic PFL action
*/
class PFLAction
{
	PFLActionType m_Type;	//!< the kind of action

public:

	/*!
	\brief constructor
	\param type the kind of this action
	*/
	PFLAction(PFLActionType type)
		:m_Type(type){}

	/*!
	\brief destructor
	*/
	virtual ~PFLAction();

	/*!
	\brief It returns the runtime type of the PFLAction
	*/
	PFLActionType GetType(void)
	{
		return m_Type;
	}

	string ToString()
	{
		switch (m_Type)
		{
		case PFL_RETURN_PACKET_ACTION: 
			return string("PFL_RETURN_PACKET_ACTION");	
		case PFL_EXTRACT_FIELDS_ACTION:
			return string("PFL_EXTRACT_FIELDS_ACTION");
		case PFL_CLASSIFY_ACTION: 
			return string("PFL_CLASSIFY_ACTION");
		}
		return string("");
	}	
};



/*!
\brief This class represents a Return Packet Action
*/
class PFLReturnPktAction: public PFLAction
{
	uint32 	m_Port;		//!< the port where accepted packets will be sent (NOTE: probably in future versions of the language this will be discarded)
public:

	/*!
	\brief constructor
	\param port the port where accepted packets will be sent
	*/
	PFLReturnPktAction(uint32 port)
		:PFLAction(PFL_RETURN_PACKET_ACTION), m_Port(port){}
};


/*!
\brief This class represents an Extract Fields Action
*/
class PFLExtractFldsAction: public PFLAction
{
	FieldsList_t 	m_FieldsList;	//!< the list of fields to be extracted

	//NOTE: these are here for future enhancements of the language
	//************************************************************
	bool 			m_InnerProto;
	bool			m_ProtoPath;
	//************************************************************

public:

	/*!
	\brief constructor
	*/
	PFLExtractFldsAction()
		:PFLAction(PFL_EXTRACT_FIELDS_ACTION){}


	/*!
	\brief adds a field to the list
	\param field a pointer to a field symbol
	*/
	void AddField(SymbolField *field)
	{
		m_FieldsList.push_back(field);
	}


	/*!
	\brief returns a reference to the internal list of fields
	*/
	FieldsList_t &GetFields(void)
	{
		return m_FieldsList;
	}

};


/*!
\brief This class represents a generic expression tree node.

This class is not instantiable directly. You should use the derived
classes.
*/
class PFLExpression
{
private:
	static uint32		m_Count;
	uint32			m_ID;
protected:

	string			m_Text;			//!<The original expression text

	PFLExpressionType m_Type;		//!<Represents the kind of the expression tree node (one of the \ref PFLExpressionType enumeration values)
	bool			m_Const;		//!<Flag: if true the expression is constant

public:
	/*!
	\brief constructor
	\param type the kind of the expression tree node (one of the \ref PFLExpressionType enumeration values)
	*/
	PFLExpression(PFLExpressionType type, bool isConst = false)
		:m_Type(type), m_Const(isConst)
	{
		m_ID = m_Count++;
	}

	/*!
	\brief destructor
	*/
	virtual ~PFLExpression();


	virtual string ToString() = 0;


	virtual PFLExpression *Clone() = 0;

	/*!
	\brief It prints out the details of the expression to stderr.
	\param level Nesting level of the expression. Use to properly indent the text on screen.
	*/
	virtual void Printme(int level) = 0;

	/*!
	\brief Appends to outfile the current expression node in Dot format (useful to be 
	visualized by the dot program of the graphviz package).
	\param outFile Pointer to the output text file
	*/
	virtual void PrintMeDotFormat(ostream &outFile) = 0;

	/*!
	\brief Transforms binary expressions in the form "const <op> expr" into expressions
	in the form  "expr <op> const"
	*/
	virtual void ToCanonicalForm() = 0;


	/*!
	\brief It returns the runtime type of the PFLExpression
	\return The type of the expression, i.e. one of the \ref PFLExpressionType enumeration values
	*/

	PFLExpressionType GetType()
	{
		return m_Type;
	}

	/*!
	\brief It returns the ID of the expression
	\return the numeric ID of the expression
	*/

	uint32 GetID()
	{
		return m_ID;
	}

	/*!
	\brief It tells whether the expression is constant or not
	\return true if the expression is constant, false otherwise
	*/

	bool IsConst()
	{
		return m_Const;
	}

	bool IsTerm()
	{
		return m_Type == PFL_TERM_EXPRESSION;
	}

	bool IsBinary()
	{
		return m_Type == PFL_BINARY_EXPRESSION;
	}

	bool IsUnary()
	{
		return m_Type == PFL_UNARY_EXPRESSION;
	}


	void SetExprText(string s)
	{
		m_Text = s;
	}

	virtual string getAttribute() = 0;
	virtual SymbolProto* GetProtocol() = 0;
	virtual int GetHeaderIndex() = 0;
	virtual bool HasPredicate() = 0;
};


class PFLIndex
{
	uint32 m_Index;

public:

	PFLIndex(uint32 num)
		:m_Index(num){}

	virtual ~PFLIndex();

	uint32 GetNum(void)
	{
		return m_Index;
	}
};



/*!
\brief This class represents a generic expression tree node.

This class is not instantiable directly. You should use the derived
classes.
*/


/*!
\brief	It represents a binary expression.
*/
class PFLBinaryExpression : public PFLExpression
{


private:

	/*!
	\brief The left operand (=left node).
	*/
	PFLExpression* m_LeftNode;

	/*!
	\brief The right operand (=right node).
	*/
	PFLExpression* m_RightNode;

	/*!
	\brief Operator of the expression.
	*/
	PFLOperator m_Operator;

	void SwapChilds();


public:
	virtual ~PFLBinaryExpression();
	virtual string ToString();
	virtual PFLExpression *Clone();
	virtual void ToCanonicalForm();
	virtual string getAttribute();
	virtual SymbolProto* GetProtocol();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);
	virtual int GetHeaderIndex(void);
	virtual bool HasPredicate(void);


	/*!
	\brief It creates a new binary expression.

	It creates a new binary expression, given the left and right operands, and the operator between them.

	\param LeftExpression Pointer to the left operand.
	\param RightExpression Pointer to the right operand.
	\param Operator Operator between the operands.
	*/
	PFLBinaryExpression(PFLExpression *LeftExpression, PFLExpression *RightExpression, PFLOperator Operator);

	/*!
	\brief	Returns the operator between the operands.

	\return	The operator between the operands.

	*/
	PFLOperator GetOperator()
	{
		return m_Operator;
	}

	/*!
	\brief	Returns the left operand (=left node).

	\return The left operand.
	*/
	PFLExpression* GetLeftNode()
	{
		return m_LeftNode;
	}

	/*!
	\brief Returns the right operand (=right node).

	\return The right operand.
	*/
	PFLExpression* GetRightNode()
	{
		return m_RightNode;
	}


	/*!
	\brief	Sets the left operand (=left node).
	\param left Pointer to the left operand.
	\return nothing
	*/
	void SetLeftNode(PFLExpression *left)
	{
		m_LeftNode = left;
	}

	/*!
	\brief Sets the right operand (=right node).
	\param right Pointer to the right operand.
	\return nothing
	*/
	void SetRightNode(PFLExpression *right)
	{
		m_RightNode = right;
	}



};

/*!
\brief	It represents a unary expression.

For example "~ip" is a unary expression. "ip" represents the operand,
"~" represents the operator.
*/
class PFLUnaryExpression : public PFLExpression
{
public:

	virtual ~PFLUnaryExpression();
	virtual string ToString();
	virtual PFLExpression *Clone();
	virtual void ToCanonicalForm();
	virtual string getAttribute();
	virtual SymbolProto* GetProtocol();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);
	virtual int GetHeaderIndex(void);
	virtual bool HasPredicate(void);

	/*!
	\brief It creates a new unary expression.

	It creates a new unary expression, given the operand, and the operator prepended/postpended to it.

	\param Expression Pointer to the operand.
	\param RightExpression Pointer to the right operand.
	\param Operator Operator prepended/postpended to the operand.
	*/
	PFLUnaryExpression(PFLExpression *Expression, PFLOperator Operator);

	/*!
	\brief	Returns the operator prepended/postpended to the operand.

	\return	The operator of the expression.

	*/
	PFLOperator GetOperator()
	{
		return this->m_Operator;
	}


	/*!
	\brief Returns the operand of the expression

	\return Pointer to the operand of the expression.
	*/
	PFLExpression* GetChild()
	{
		return this->m_Node;
	}

	/*!
	\brief Sets the operand of the expression
	\param child Pointer to the operand.
	\return nothing
	*/
	void SetChild(PFLExpression *child)
	{
		m_Node = child;
	}

private:

	/*!
	\brief The operator of the expression.
	*/
	PFLOperator m_Operator;

	/*!
	\brief The operand of the expression.
	*/
	PFLExpression* m_Node;
};

/*!
\brief	It represents a leaf node.

Any leaf node represents a boolean term in the filter expression, i.e. a term that evaluates the presence of a specific protocol
in the packet, or an IR expression that evaluates a condition on some protocol fields
*/
class PFLTermExpression : public PFLExpression
{

protected:
	/*!
	\brief The protocol (symbol) associated to the boolean terminal node
	*/
	SymbolProto		*m_Protocol;
	uint32			m_HeaderIndex; 
	Node			*m_IRExpr;
	bool			m_Value;

public:
	~PFLTermExpression();
	virtual string ToString();
	virtual PFLTermExpression *Clone();
	virtual void ToCanonicalForm();
	virtual string getAttribute();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);

	int GetHeaderIndex(void){
		return m_HeaderIndex;
	}

	bool HasPredicate(void)
	{
		return (m_IRExpr!=NULL);
	}

	void SetHeaderIndex(uint32 index){
		m_HeaderIndex = index;
	}

	SymbolProto *GetProtocol()
	{
		return m_Protocol;
	}


	Node *GetIRExpr()
	{
		return m_IRExpr;
	}

	RangeOperator_t getIRRangeOperator() 
	{
		/* A normalization is needed here.
		* Both "foo > X" and "X < foo" have the same meaning,
		* but on the upper levels I expect always the first form.
		* So, if we find in the second case,
		* try and invert the operator on the fly.
		*/
		if (m_IRExpr->Kids[0]->Sym == NULL)
		{
			// that is, if we are in "canonical" form: "foo > X"
			switch(GET_OP(m_IRExpr->Op))
			{
				case OP_EQ: return EQUAL;
				case OP_NE: return NOT_EQUAL;
				case OP_LT: return LESS_THAN;
				case OP_LE: return LESS_EQUAL_THAN;
				case OP_GT: return GREAT_THAN;
				case OP_GE: return GREAT_EQUAL_THAN;
				case OP_MATCH: return MATCH;
				case OP_CONTAINS: return CONTAINS;
				default: nbASSERT(false, "Congratulations, you have found a bug"); return INVALID;
			}
		}
		else {
			// "X < foo"
			switch(GET_OP(m_IRExpr->Op))
			{
				case OP_EQ: return EQUAL;
				case OP_NE: return NOT_EQUAL;
				case OP_LT: return GREAT_THAN;
				case OP_LE: return GREAT_EQUAL_THAN;
				case OP_GT: return LESS_THAN;
				case OP_GE: return LESS_EQUAL_THAN;
				default: nbASSERT(false, "Congratulations, you have found a bug"); return INVALID;
			}
		}
	}

	uint32 getIRValue() {
		// reverse engineering FTW
		Node *tmp = (m_IRExpr->Kids[0]->Sym == NULL?
			m_IRExpr->Kids[1] :
		m_IRExpr->Kids[0]);
		struct SymbolIntConst* tmp2 = (struct SymbolIntConst*) tmp->Sym;
		return tmp2->Value;
	}

	string getIRStringValue(){

		Node *tmp = (m_IRExpr->Kids[0]->Sym == NULL?
			m_IRExpr->Kids[1] :
		m_IRExpr->Kids[0]);
		struct SymbolStrConst* tmp2 = (struct SymbolStrConst*) tmp->Sym;
		return tmp2->Name;

	}

	void SetProtocol(SymbolProto *proto)
	{
		m_Protocol = proto;
	}

	void SetIRExpr(Node *irExpr)
	{
		m_IRExpr = irExpr;
	}

	bool GetValue()
	{
		nbASSERT(m_Const, "cannot retrieve the value of a non-constant terminal node");
		return m_Value;
	}


	/*!
	\brief It creates a leaf node

	\param proto a protocol symbol
	\param an IR expression node that evaluates a condition
	*/
	PFLTermExpression(SymbolProto *proto, uint32 indexing, Node *expr = 0);

	PFLTermExpression(SymbolProto *proto, Node *expr = 0);

	PFLTermExpression(bool constVal);
};



/*
*	This class represent a set of protocol, i.e. a list of protocols that will be translated in an OR operation of the regular expression
*/
class PFLSetExpression : public PFLExpression
{

private:

	list<PFLExpression*>* m_Elements; 	//this list contains the element of the set (however it is represented with a list :-D)
	//Each element can be a binary, unary or term expression																
	PFLOperator m_Operator;
	bool	m_MandatoryTunnels; 		//for the keyword "tunneled"
	PFLInclusionOperator m_Inclusion; 	//indicate if the set is involved in a "notin" or "in" operation
	PFLRepeatOperator m_Repeat; 	 	//indicate +,*,? or nothing
	bool	m_AnyPlaceholder;			//is this set the any placeholer?

public:
	virtual ~PFLSetExpression();
	virtual string ToString();
	virtual PFLExpression *Clone();
	virtual void ToCanonicalForm();
	virtual string getAttribute();
	virtual SymbolProto* GetProtocol();
	list<SymbolProto*> *GetProtocols();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);
	virtual int GetHeaderIndex(void);
	virtual bool HasPredicate(void);

	PFLSetExpression(list<PFLExpression*> *Elements, PFLOperator Operator, bool mandatoryTunnels = false, PFLRepeatOperator Repeat = DEFAULT_ROP, PFLInclusionOperator Inclusion = DEFAULT_INCLUSION);

	PFLRepeatOperator GetRepeatOperator(){
		return m_Repeat;
	}

	void SetRepeatOperator(PFLRepeatOperator op){
		m_Repeat = op;
	}

	void SetInclusionOperator(PFLInclusionOperator op){
		m_Inclusion = op;
	}

	void SetMandatoryTunnels()
	{
		m_MandatoryTunnels = true;
	}

	void ResetMandatoryTunnels()
	{
		m_MandatoryTunnels = false;
	}

	bool MandatoryTunnels(){
		return m_MandatoryTunnels;
	}


	PFLInclusionOperator GetInclusionOperator(){
		return m_Inclusion;
	}

	PFLOperator GetOperator()
	{
		return m_Operator;
	}

	list<PFLExpression*>* GetElements()
	{
		return m_Elements;
	}

	void SetElements(list<PFLExpression*>* Elements)
	{
		m_Elements = Elements;
	}

	bool IsAnyPlaceholder()
	{
		return m_AnyPlaceholder;
	}

	void SetAnyPlaceholder(bool AnyPlaceholder)
	{
		m_AnyPlaceholder = AnyPlaceholder;
	}

};

/*
*	This class represent an header chaining, that will be translated in a regular expression
*/
class PFLRegExpExpression : public PFLExpression
{

private:
	list<PFLSetExpression*>* m_InnerList; //an element of the list is a set. The set can contain 0, 1 or more elements. If the set has 0 elements, it represents the any placeholder
	PFLOperator m_Operator;

public:
	virtual ~PFLRegExpExpression();
	virtual string ToString();
	virtual PFLExpression *Clone();
	virtual void ToCanonicalForm();
	virtual string getAttribute();
	virtual SymbolProto* GetProtocol();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);
	virtual int GetHeaderIndex(void);
	virtual bool HasPredicate(void);

	PFLRegExpExpression(list<PFLSetExpression*> *InnerList, PFLOperator Operator);


	PFLOperator GetOperator()
	{
		return m_Operator;
	}


	list<PFLSetExpression*>* GetInnerList()
	{
		return m_InnerList;
	}


	void SetInnerList(list<PFLSetExpression*>* InnerList)
	{
		m_InnerList = InnerList;
	}
};
