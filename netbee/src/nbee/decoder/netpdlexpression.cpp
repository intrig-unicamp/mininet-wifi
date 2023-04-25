/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <string.h>
#include <nbprotodb.h>
#include <nbprotodb.h>
#include <nbprotodb_defs.h>
#include <nbee_netpdlutils.h>
#include <xercesc/util/XMLString.hpp>

#include <pcre.h>

#include "netpdlprotodecoder.h"
#include "netpdlexpression.h"
#include "pdmlmaker.h"
#include "../utils/netpdlutils.h"
#include "../globals/globals.h"
#include "../globals/debug.h"


// Global variable
extern struct _nbNetPDLDatabase *NetPDLDatabase;

#define MIN(a, b) ( (a < b)? a : b)

#ifndef WIN32
#define strnicmp strncasecmp
#endif

/*!
	\brief Standard constructor.

	\param NetPDLVars: pointer to the NetPDLRunTimeVars class in order to access to the
	currently defined global variables.

	\param Errbuf: pointer to a user-allocated buffer (of length 'm_errbufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrbufSize: size of the previous buffer.
*/
CNetPDLExpression::CNetPDLExpression(CNetPDLVariables *NetPDLVars, char *Errbuf, int ErrbufSize)
{
	m_netPDLVariables= NetPDLVars;

	// Store internally the pointer to the error buffer. This buffer belongs to the class that creates this one.
	m_errbuf= Errbuf;
	m_errbufSize= ErrbufSize;
}


//! Default destructor.
CNetPDLExpression::~CNetPDLExpression()
{
}


/*!
	\brief It evaluates a 'switch' and a 'showmap' node.

	The 'switch' node is used to speed up the expression evaluation (and to make the NetPDL mode compact).
	It is applicable when several choices rely on the same field for their result.

	\param SwitchNodeInfo: internal structure related to the node that contains the 'switch' construct.

	\param PDMLStartField: pointer to a field within the current PDML fragment, which is
	the starting point to evaluate the given expression (i.e. it determines the starting point 
	to retrieve the value of fieldref or such).

	\param Result: keeps the result of the expression. It will be NULL if the switch has no 'case' nodes 
	satisfying the expression, the proper 'case' node otherwise. The caller will use this pointer to 
	decide what to do (for example jump to the next proto if the switch is related to a nextproto clause).

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	The result of the expression is returned into the Result parameter, 
	which is meaningful only in case of success.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLExpression::EvaluateSwitch(struct _nbNetPDLElementSwitch *SwitchNodeInfo, 
									  struct _nbPDMLField *PDMLStartField, struct _nbNetPDLElementCase **Result)
{
long RetVal;
unsigned int KeyValue;
unsigned char *KeyString = NULL;
unsigned char *TmpString;
unsigned int KeyStringSize;
bool CompareAsString;
struct _nbNetPDLElementCase *CaseListInfo;


	if (SwitchNodeInfo->ExprTree->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER)
		CompareAsString= true;
	else
		CompareAsString= false;

	if (CompareAsString)
	{
		KeyStringSize= sizeof(TmpString);
		RetVal= EvaluateExprString(SwitchNodeInfo->ExprTree, PDMLStartField, &TmpString, &KeyStringSize);
		KeyString= TmpString;
	}
	else
		RetVal= EvaluateExprNumber(SwitchNodeInfo->ExprTree, PDMLStartField, &KeyValue);

	if (RetVal != nbSUCCESS)	// there has been an error somewhere
	{
		*Result= NULL;
		return RetVal;
	}

	CaseListInfo= SwitchNodeInfo->FirstCase;

	if (CompareAsString)
	{
		while (CaseListInfo)
		{
			if (SwitchNodeInfo->CaseSensitive)
			{
				// Let's do a check of the string size first; otherwise, key string "bla" 
				// appears to match case string "blabla", since the memcmp compares only the first
				// three characters (as specified by the KeyStringSize number)
				if ((KeyStringSize == CaseListInfo->ValueStringSize) &&
					(memcmp(KeyString, CaseListInfo->ValueString, KeyStringSize) == 0))
				{
					*Result= CaseListInfo;
					return nbSUCCESS;
				}
			}
			else
			{
				// Let's compare using string-based functions (since we're using a case-insensitive match,
				// we suppose we're having strings and not simply memory buffers
				if ((KeyStringSize == CaseListInfo->ValueStringSize) &&
					(strnicmp((char *) KeyString, (char *) CaseListInfo->ValueString, KeyStringSize) == 0))
				{
					*Result= CaseListInfo;
					return nbSUCCESS;
				}
			}

			CaseListInfo= CaseListInfo->NextCase;
		}
	}
	else
	{
		while (CaseListInfo)
		{
			if (CaseListInfo->ValueMaxNumber)
			{
				if ((CaseListInfo->ValueNumber <= KeyValue) && (KeyValue <= CaseListInfo->ValueMaxNumber))
				{
					*Result= CaseListInfo;
					return nbSUCCESS;
				}
			}
			else
			{
				if (KeyValue == CaseListInfo->ValueNumber)
				{
					*Result= CaseListInfo;
					return nbSUCCESS;
				}
			}

			CaseListInfo= CaseListInfo->NextCase;
		}
	}

	// No suitable 'case' have been found
	if (SwitchNodeInfo->DefaultCase)
		*Result= SwitchNodeInfo->DefaultCase;
	else
		*Result= NULL;

	return nbSUCCESS;
}




/*!
	\brief It returns the result of a string expression.

	This method is used to retrieve the result of a string expression (e.g. an expression that contains
	'substr') and it returns a string.

	\param ExprNode: the node that contains the expression that has to be evaluated.

	\param PDMLStartField: pointer to a field within the current PDML fragment, which is
	the starting point to evaluate the given expression (i.e. it determines the starting point 
	to retrieve the value of fieldref or such).

	\param ResultString: this parameter, passed as reference, will contain the result.
	This buffer belongs to some internal structures, hence it should not be modified by the caller.
	This buffer contains a binary string, hence it is not zero-terminated.

	\param ResultStringLen: the length of the previous buffer.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	The result of the expression is returned into the Result parameter, 
	which is meaningful only in case of success.
	In case of error, the error message can be retrieved by the GetLastError() method.<br>
	In case we are looking for a PDML field which cannot be found, or we're going to get access to
	the packet buffer that appears to be truncated, it returns nbWARNING. This return code
	should not be managed as critical error, since there are cases in which this is not avoidable.
*/
int CNetPDLExpression::EvaluateExprString(struct _nbNetPDLExprBase *ExprNode, struct _nbPDMLField *PDMLStartField,
									  unsigned char **ResultString, unsigned int *ResultStringLen)
{
struct _nbNetPDLExpression* Expression;
char *Mask;
int RetVal;

	// If this is not a complex expression, let's return the result right now
	if (ExprNode->Type != nbNETPDL_ID_EXPR_OPERAND_EXPR)
	{
		RetVal= GetOperandBuffer(ExprNode, PDMLStartField, ResultString, &Mask, ResultStringLen);

		if (RetVal != nbSUCCESS)
			return RetVal;

		goto success;
	}

	// Let's convert this pointer into an expression
	Expression= (struct _nbNetPDLExpression*) ExprNode;

	// Get the first operand
	if (Expression->Operand1 == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Cannot evaluate expression. No operand found.");

		*ResultString= 0;
		return nbFAILURE;
	}

	RetVal= GetOperandBuffer(Expression->Operand1, PDMLStartField, ResultString, &Mask, ResultStringLen);
	if (RetVal != nbSUCCESS)
		return RetVal;

	// In case the operator is NULL, let's return now
	if (Expression->Operator == NULL)		
		goto success;


	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Cannot evaluate expression. Operand still unsupported, maybe?");

	*ResultString= NULL;
	*ResultStringLen= 0;
	return nbFAILURE;

success:

	if (ExprNode->PrintDebug)
	{
	char ResultMessage[1024];

		ssnprintf(ResultMessage, sizeof(ResultMessage), "Result of selected expression: %s\n", ResultString);

// Very rude way to print the message on Windows
#ifdef _MSC_VER
		MessageBoxA(NULL, ResultMessage, "NetPDL Debug", 0);
#else
		fprintf(stderr, "[NetPDL Debug] %s", ResultMessage);
#endif
	}

	return nbSUCCESS;
}



/*!
	\brief It starts the evaluation of a numeric expression.

	\param ExprNode: the node that contains the expression that has to be evaluated.

	\param PDMLStartField: pointer to a field within the current PDML fragment, which is
	the starting point to evaluate the given expression (i.e. it determines the starting point 
	to retrieve the value of fieldref or such).

	\param Result: keeps the result of the expression; in case it is a boolean expression, '0' means
	false, otherwise means true.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	The result of the expression is returned into the Result parameter, 
	which is meaningful only in case of success.
	In case of error, the error message can be retrieved by the GetLastError() method.<br>
	In case we are looking for a PDML field which cannot be found, or we're going to get access to
	the packet buffer that appears to be truncated, it returns nbWARNING. This return code
	should not be managed as critical error, since there are cases in which this is not avoidable.
*/

int CNetPDLExpression::EvaluateExprNumber(struct _nbNetPDLExprBase *ExprNode, struct _nbPDMLField *PDMLStartField, unsigned int *Result)
{
struct _nbNetPDLExpression* Expression;
unsigned char *Value1Buffer, *Value2Buffer;
char *Mask1Buffer, *Mask2Buffer;
unsigned int Value1BufferSize, Value2BufferSize;
unsigned int Value1Number, Value2Number;
int RetVal;

	// If this is not a complex expression, let's return the result right now
	if (ExprNode->Type != nbNETPDL_ID_EXPR_OPERAND_EXPR)
	{
		RetVal= GetOperandNumber(ExprNode, PDMLStartField, Result);

		if (RetVal != nbSUCCESS)
			return RetVal;

		goto success;
	}

	// Let's convert this pointer into an expression
	Expression= (struct _nbNetPDLExpression*) ExprNode;

	// The first operand may be missing in case of a 'not' or 'bitwnot' expression
	if (Expression->Operand1)
	{
		if (Expression->Operand1->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
			RetVal= GetOperandNumber(Expression->Operand1, PDMLStartField, &Value1Number);
		else
			RetVal= GetOperandBuffer(Expression->Operand1, PDMLStartField, &Value1Buffer, &Mask1Buffer, &Value1BufferSize);

		if (RetVal != nbSUCCESS)
			return RetVal;
	}

	// In case the operator is NULL, let's return now
	if (Expression->Operator == NULL)
	{
		*Result= Value1Number;
		goto success;
	}

	// If we're in an 'and' and the first result is '0', let's return now
	if ((Expression->Operator->OperatorType == nbNETPDL_ID_EXPR_OPER_AND) && (Value1Number == 0))
	{
		*Result= Value1Number;
		goto success;
	}

	// If we're in an 'or' and the first result is non-zero, let's return now
	if ((Expression->Operator->OperatorType == nbNETPDL_ID_EXPR_OPER_OR) && (Value1Number != 0))
	{
		*Result= Value1Number;
		goto success;
	}

	// Get the second operand
	if (Expression->Operand2->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
	{
		RetVal= GetOperandNumber(Expression->Operand2, PDMLStartField, &Value2Number);
	}
	else
	{
		RetVal= GetOperandBuffer(Expression->Operand2, PDMLStartField, &Value2Buffer, &Mask2Buffer, &Value2BufferSize);
	}

	if (RetVal != nbSUCCESS)
		return RetVal;

	// Evaluate expression
	switch (Expression->Operator->OperatorType)
	{
		case nbNETPDL_ID_EXPR_OPER_PLUS: { *Result= (Value1Number + Value2Number); goto success; };
		case nbNETPDL_ID_EXPR_OPER_MINUS: { *Result= (Value1Number - Value2Number); goto success; };
		case nbNETPDL_ID_EXPR_OPER_MUL: { *Result= (Value1Number * Value2Number); goto success;};
		case nbNETPDL_ID_EXPR_OPER_DIV: { *Result= (Value1Number / Value2Number); goto success; };
		case nbNETPDL_ID_EXPR_OPER_MOD: { *Result= (Value1Number % Value2Number); goto success; };
		case nbNETPDL_ID_EXPR_OPER_BITWAND: { *Result= (Value1Number & Value2Number); goto success; };
		case nbNETPDL_ID_EXPR_OPER_BITWOR: { *Result= (Value1Number | Value2Number); goto success; };

		case nbNETPDL_ID_EXPR_OPER_GREATEQUAL: { *Result= (Value1Number >= Value2Number); goto success; };
		
		case nbNETPDL_ID_EXPR_OPER_LESSEQUAL: { *Result= (Value1Number <= Value2Number); goto success; };
		case nbNETPDL_ID_EXPR_OPER_AND: { *Result= (Value1Number && Value2Number); goto success; };
		case nbNETPDL_ID_EXPR_OPER_OR: { *Result= (Value1Number || Value2Number); goto success; };

		case nbNETPDL_ID_EXPR_OPER_NOT: { *Result= !Value2Number; goto success; };
		case nbNETPDL_ID_EXPR_OPER_BITWNOT: { *Result= ~Value2Number; goto success; };
		
		case nbNETPDL_ID_EXPR_OPER_GREAT: 
		{
			if (Expression->Operand1->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
			{
				*Result= (Value1Number > Value2Number);
				goto success;
			}
			else
			{
			int Size= MIN(Value1BufferSize, Value2BufferSize);

				if (memcmp(Value1Buffer, Value2Buffer, Size) > 0)
					*Result= true;
				else
					*Result= false;

				goto success;
			}
		}
		
		case nbNETPDL_ID_EXPR_OPER_LESS:
		{
			if (Expression->Operand1->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
			{
				*Result= (Value1Number < Value2Number);
				goto success;
			}
			else
			{
			int Size= MIN(Value1BufferSize, Value2BufferSize);

				if (memcmp(Value1Buffer, Value2Buffer, Size) < 0)
					*Result= true;
				else
					*Result= false;

				goto success;
			}
		}

		case nbNETPDL_ID_EXPR_OPER_EQUAL:
		{
			if (Expression->Operand1->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
			{
				*Result= (Value1Number == Value2Number);
				goto success;
			}
			else
			{
			int Size= MIN(Value1BufferSize, Value2BufferSize);

				if (memcmp(Value1Buffer, Value2Buffer, Size) == 0)
					*Result= true;
				else
					*Result= false;

				goto success;
			}
		}

		case nbNETPDL_ID_EXPR_OPER_NOTEQUAL:
		{
			if (Expression->Operand1->ReturnType == nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER)
			{
				*Result= (Value1Number != Value2Number);
				goto success;
			}
			else
			{
			int Size= MIN(Value1BufferSize, Value2BufferSize);

				if (memcmp(Value1Buffer, Value2Buffer, Size) == 0)
					*Result= false;
				else
					*Result= true;

				goto success;
			}
		}

		default: 
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Cannot evaluate expression. Operator still unsupported, maybe?");

			*Result= 0;
			return nbFAILURE;
		}
	}

success:

	if (ExprNode->PrintDebug)
	{
	char ResultMessage[1024];

		ssnprintf(ResultMessage, sizeof(ResultMessage), "Result of selected expression: %u\n", *Result);

// Very rude way to print the message on Windows
#ifdef _MSC_VER
		MessageBoxA(NULL, ResultMessage, "NetPDL Debug", 0);
#else
		fprintf(stderr, "[NetPDL Debug] %s", ResultMessage);
#endif
	}

	return nbSUCCESS;
}


/*!
	\brief Returns a numeric operand

	This function accepts an operand as a parameter, and it returns back its value.

	\param OperandBase: Element containing the operand to be examined.

	\param PDMLStartField: starting field of the current PDML fragment.

	\param ResultValue: this parameter, passed as reference, will contain 
	the result.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLExpression::GetOperandNumber(struct _nbNetPDLExprBase *OperandBase, struct _nbPDMLField *PDMLStartField, unsigned int *ResultValue)
{
int RetVal;

	switch (OperandBase->Type)
	{
		case nbNETPDL_ID_EXPR_OPERAND_NUMBER:
		{
			struct _nbNetPDLExprNumber* Operand;

			Operand= (struct _nbNetPDLExprNumber*) OperandBase;
			*ResultValue= Operand->Value;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_PROTOREF:
		{
		struct _nbNetPDLExprProtoRef* Operand;

			Operand= (struct _nbNetPDLExprProtoRef*) OperandBase;
			*ResultValue= Operand->Value;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
		{
		struct _nbNetPDLExprVariable* Operand;

			Operand= (struct _nbNetPDLExprVariable*) OperandBase;

			m_netPDLVariables->GetVariableNumber( (long) Operand->CustomData, ResultValue);
			
			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:
		{
		struct _nbNetPDLExprLookupTable* Operand;

			Operand= (struct _nbNetPDLExprLookupTable*) OperandBase;

			m_netPDLVariables->GetTableDataNumber( (long) Operand->TableCustomData, (long) Operand->FieldCustomData, ResultValue);
			
			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_EXPR:
		{		
			RetVal= EvaluateExprNumber(OperandBase, PDMLStartField, ResultValue);

			if (RetVal != nbSUCCESS)
				return RetVal;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISASN1TYPE:
		{
		struct _nbNetPDLExprFunctionIsASN1Type* Operand;
		unsigned char *BufferValue;
		char *BufferMask;
		unsigned int BufferMaxSize;
		unsigned int Tag, Class;

			Operand= (struct _nbNetPDLExprFunctionIsASN1Type *) OperandBase;

			// Let's retrieve the string variable passed to check ASN.1 type
			RetVal= GetOperandBuffer(Operand->StringExpression, PDMLStartField, &BufferValue, &BufferMask, &BufferMaxSize);

			switch (RetVal)
			{
			case nbSUCCESS:
				break;
			case nbWARNING:
				{
					*ResultValue= 0;
					return nbSUCCESS;
				}; break;
			default:
				return nbFAILURE;
			}

			// Let's retrieve the Class Number passed as parameter
			RetVal= GetOperandNumber(Operand->ClassNumber, PDMLStartField, &Class);

			if (RetVal != nbSUCCESS)
				return RetVal;

			// Check the Class Number
			if ( (unsigned int)((BufferValue[0] & 0xC0) >> 6) != Class)
			{
				*ResultValue= 0;
				return nbSUCCESS;
			}

			// Let's retrieve the Tag Number passed as parameter
			RetVal= GetOperandNumber(Operand->TagNumber, PDMLStartField, &Tag);

			if (RetVal != nbSUCCESS)
				return RetVal;

			// Check the Tag Number
			switch(BufferValue[0] & 0x1F) 
			{
				case 0x1F: // This is the case of ASN.1 Id Octet in Long Form
					{
					unsigned int TagToCompare= 0;
					unsigned int i= 1;
						
						while (1)
						{
							TagToCompare= TagToCompare << 7;
							TagToCompare+= (BufferValue[i] & 0x7F);

							if ( (BufferValue[i] & 0x80) == 0x00 )
								break;

							i++;
						}

						if ( TagToCompare != Tag)
						{
							*ResultValue= 0;
							return nbSUCCESS;
						}
					}; break;
				default: // This is the case of ASN.1 Id Octet in Short Form
					{
						if ( (BufferValue[0] & 0x1F) != Tag)
						{
							*ResultValue= 0;
							return nbSUCCESS;
						}
					}
			}

			// All checks passed, let's return a positive match
			*ResultValue= 1;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_HASSTRING:
		{
		struct _nbNetPDLExprFunctionRegExp* Operand;
		int RexExpReturnCode;
		unsigned char *TmpString;
		unsigned int TmpStringSize;
		int MatchingOffset[MATCHING_OFFSET_COUNT];

			Operand= (struct _nbNetPDLExprFunctionRegExp*) OperandBase;

			// Let's get the buffer in which search has to be done
			TmpStringSize= sizeof(TmpString);
			if (EvaluateExprString(Operand->SearchBuffer, PDMLStartField, &TmpString, &TmpStringSize) == nbFAILURE)
				return nbFAILURE;

#ifdef _DEBUG
			if (TmpString == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Unexpected buffer equal to zero");
				return nbFAILURE;
			}
#endif

			RexExpReturnCode= pcre_exec(
				(pcre *) Operand->PCRECompiledRegExp,		// the compiled pattern
				NULL,								// no extra data - we didn't study the pattern
				(char*) TmpString,							// the subject string
				TmpStringSize,						// the length of the subject
				0,									// start at offset 0 in the subject
				0,									// default options
				MatchingOffset,						// output vector for substring information
				MATCHING_OFFSET_COUNT);				// number of elements in the output vector

			if (RexExpReturnCode >= 0)	// Match found
			{
				*ResultValue= 1;
				return nbSUCCESS;
			}

			if (RexExpReturnCode == -1)	// Match not found
			{
				*ResultValue= 0;
				return nbSUCCESS;
			}

			// Otherwise, there's an error
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
									"Error in Regular Expression match: PCRE reported error %d.", RexExpReturnCode);
			return nbFAILURE;
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_BUF2INT:
		{
		struct _nbNetPDLExprFunctionBuf2Int* Operand;
		unsigned char *BufferValue;
		char *BufferMask;
		unsigned int BufferMaxSize;
		int Number, Mask;

			Operand= (struct _nbNetPDLExprFunctionBuf2Int*) OperandBase;
			
			RetVal= GetOperandBuffer(Operand->StringExpression, PDMLStartField, &BufferValue, &BufferMask, &BufferMaxSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

			if (BufferMask == NULL)
			{
				*ResultValue= NetPDLHexDumpToLong(BufferValue, BufferMaxSize, 1);
				return nbSUCCESS;
			}
			else
			{
				Number= NetPDLHexDumpToLong(BufferValue, BufferMaxSize, 1);
				// Being an ascii buffer, its size is two times the buffer that contains the number
				Mask= NetPDLAsciiStringToLong((char*) BufferMask, BufferMaxSize * 2, 16);

				// Rotate until the last bit is '0'
				while ((Mask & 0x01) == 0)
				{
					Mask= Mask >> 1;
					Number= Number >> 1;
				}

				*ResultValue= Number & Mask;

				return nbSUCCESS;
			}
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ASCII2INT:
		{
		struct _nbNetPDLExprFunctionAscii2Int* Operand;
		unsigned char *ResultString;
		unsigned int ResultStringLen;

			Operand= (struct _nbNetPDLExprFunctionAscii2Int*) OperandBase;

			RetVal= EvaluateExprString(Operand->AsciiStringExpression, PDMLStartField, &ResultString, &ResultStringLen);

			if (RetVal != nbSUCCESS)
				return RetVal;

			*ResultValue= NetPDLAsciiStringToLong((char*) ResultString, ResultStringLen, 10);

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISPRESENT:
		{
		struct _nbNetPDLExprFunctionIsPresent* Operand;
		unsigned int FieldOffset;		// This variable is useless, however needed for calling ScanForFieldRefValue()
		char *BufferMask;		// This variable is useless, however needed for calling ScanForFieldRefValue()
		unsigned int BufferMaxSize;		// This variable is useless, however needed for calling ScanForFieldRefValue()

			Operand= (struct _nbNetPDLExprFunctionIsPresent*) OperandBase;

			if (PDMLStartField == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "We're trying to locate the value of a fieldref within a NULL PDML fragment");
				return nbFAILURE;
			}

			RetVal= CPDMLMaker::ScanForFieldRefValue(PDMLStartField, Operand->NetPDLField->ProtoName, Operand->NetPDLField->FieldName,
														&FieldOffset, &BufferMaxSize, &BufferMask, m_errbuf, m_errbufSize);

			if (RetVal == nbSUCCESS)
			{
				*ResultValue= 1;
				return nbSUCCESS;
			}

			if (RetVal == nbWARNING)
			{
				*ResultValue= 0;
				return nbSUCCESS;
			}

			return nbFAILURE;
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE:
		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE:
		{
		struct _nbNetPDLExprFunctionCheckUpdateLookupTable* Operand;
		struct _nbLookupTableKey* KeyList;
		unsigned int value;
		struct _nbParamsLinkedList* KeyParameter;
		unsigned int Timestamp;
		int TableID;
		int i;
		
			Operand= (struct _nbNetPDLExprFunctionCheckUpdateLookupTable*) OperandBase;

			TableID= (long) Operand->LookupTableCustomData;

			KeyList= m_netPDLVariables->GetStructureForTableKey(TableID);
			if (KeyList == NULL)
				return nbFAILURE;

			i= 0;
			KeyParameter= Operand->ParameterList;

			while(KeyParameter)
			{
				switch(KeyParameter->Expression->ReturnType)
				{
					case nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER:
					{	
						RetVal= EvaluateExprNumber(KeyParameter->Expression, PDMLStartField, &value);
						*(KeyList[i].Value) = value;
						if (RetVal != nbSUCCESS)
							return RetVal;
					}; break;

					case nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER:
					{
						RetVal= EvaluateExprString(KeyParameter->Expression, PDMLStartField, &(KeyList[i].Value), &(KeyList[i].Size));
						if (RetVal != nbSUCCESS)
							return RetVal;
					}; break;

					default:
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: unexpected value.");
						return nbFAILURE;
					}; break;
				}

				i++;
				KeyParameter= KeyParameter->NextParameter;
			}

			m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.TimestampSec, &Timestamp);

			if (OperandBase->Type == nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE)
				RetVal= m_netPDLVariables->LookupForTableEntry(TableID, KeyList, Timestamp, 1, 1, 1);
			else
				RetVal= m_netPDLVariables->LookupAndUpdateTableEntry(TableID, KeyList, Timestamp);

			if (RetVal == nbSUCCESS)
			{
				*ResultValue= 1;
				return nbSUCCESS;
			}

			if (RetVal == nbWARNING)
			{
				*ResultValue= 0;
				return nbSUCCESS;
			}

			return nbFAILURE;
		}

//TEMP FULVIO URGENTISSIMO Aggiungere un nuovo flag nel PDML, altrimenti quando si ricrea il dump, questo va a pallino nel caso di
// campi bigendian
// FULVIO: la soluzione precedente fa schifo; molto meglio mantenere, in "value", il valore effettivo del campo, ed utilizzare il flag
// del byte order per andare ad accedere al campo stesso attraverso il fieldref

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Found a tag which is not a valid element within an expression.");
			return nbFAILURE;
		}
	}
}



/*!
	\brief Returns a buffer-based operand.

	This function accepts an operand as a parameter, and it returns back its value.

	\param OperandBase: Element containing the operand to be examined.

	\param PDMLStartField: starting field of the current PDML fragment.

	\param BufferValue: this parameter, passed as reference, will contain 
	the result. This buffer belongs to some internal structures, hence it should
	not be modified by the caller.
	This buffer contains a binary string, hence it is not zero-terminated.

	\param BufferMask: this parameter, passed as reference, will contain 
	the mask associated to a given buffer. This is needed in case of protofield, which
	may have a 'mask'. The mask is needed in case we want to get the numeric value of
	the field, so we have to return this parameter as well.
	This buffer belongs to some internal structures, hence it should not be 
	modified by the caller.
	This buffer contain the ascii-based mask and it is zero-terminated.

	\param BufferMaxSize: size of the 'BufferValue'. When the function returns,
	this variable will contain the size of the binary string that has to be handled. This
	is needed because we may have hex strings which are not NULL terminated, therefore
	we have to tell the caller the size of the actual data that has to be processed.

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
int CNetPDLExpression::GetOperandBuffer(struct _nbNetPDLExprBase *OperandBase, struct _nbPDMLField *PDMLStartField, 
								  unsigned char **BufferValue, char **BufferMask, unsigned int *BufferMaxSize)
{
int RetVal;

	switch (OperandBase->Type)
	{
		case nbNETPDL_ID_EXPR_OPERAND_STRING:
		{
		struct _nbNetPDLExprString* Operand;

			Operand= (struct _nbNetPDLExprString*) OperandBase;

			*BufferMaxSize= Operand->Size;
			*BufferValue= (unsigned char*) Operand->Value;
			*BufferMask= NULL;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_VARIABLE:
		{
		struct _nbNetPDLExprVariable* Operand;
		unsigned int StartAt, Size;

			Operand= (struct _nbNetPDLExprVariable*) OperandBase;

			if (Operand->OffsetStartAt)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetStartAt, PDMLStartField, &StartAt);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}
			else
				StartAt= 0;

			if (Operand->OffsetSize)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetSize, PDMLStartField, &Size);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}
			else
				Size= 0;

			RetVal= m_netPDLVariables->GetVariableBuffer((long) (Operand->CustomData), StartAt, Size, BufferValue, BufferMaxSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

			// In case we're getting a string from a variable, the mask is NULL
			*BufferMask= NULL;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE:
		{
		struct _nbNetPDLExprLookupTable* Operand;
		unsigned int StartAt, Size;

			Operand= (struct _nbNetPDLExprLookupTable*) OperandBase;

			if (Operand->OffsetStartAt)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetStartAt, PDMLStartField, &StartAt);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}
			else
				StartAt= 0;

			if (Operand->OffsetSize)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetSize, PDMLStartField, &Size);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}
			else
				Size= 0;

			RetVal= m_netPDLVariables->GetTableDataBuffer((long) (Operand->TableCustomData), (long) (Operand->FieldCustomData),
															StartAt, Size, BufferValue, BufferMaxSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

			// In case we're getting a string from a variable, the mask is NULL
			*BufferMask= NULL;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD:
		{
		struct _nbNetPDLExprFieldRef* Operand;
		char *FieldName;
		char *FirstField;
		char *CurrentField;
		unsigned int FieldOffset;
		unsigned int StartAt;
		unsigned int BufferSize;

			Operand= (struct _nbNetPDLExprFieldRef*) OperandBase;

			if (PDMLStartField == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "We're trying to locate the value of a fieldref within a NULL PDML fragment");
				return nbFAILURE;
			}

			// Duplicate current field name
			FieldName= strdup(Operand->FieldName);
			if (FieldName == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: insufficient memory");
				return nbFAILURE;
			}

			// Get the name of the first field:
			FirstField= strtok(FieldName, NETPDL_COMMON_SYNTAX_SEP_FIELDS);

			// Get the name of the first subfield:
			CurrentField= strtok(NULL, NETPDL_COMMON_SYNTAX_SEP_FIELDS);

			if (CurrentField == NULL)
			{
				// Subfields are missing, let's return the single field value
				RetVal= CPDMLMaker::ScanForFieldRefValue(PDMLStartField, Operand->ProtoName, Operand->FieldName,
															&FieldOffset, BufferMaxSize, BufferMask, m_errbuf, m_errbufSize);
			}
			else
			{
			struct _nbPDMLField *TempPDMLElement= PDMLStartField;
			uint8_t SubfieldFound;

				// Recognize PDML Element relevant to first field
				RetVal= CPDMLMaker::ScanForFieldRef(PDMLStartField, Operand->ProtoName, FirstField, &TempPDMLElement, m_errbuf, m_errbufSize); 

				if (RetVal != nbSUCCESS)
				{
					if (RetVal == nbWARNING)
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
							"The NetPDL protocol database contains a reference to a field that cannot be located in the PDML fragment: Protocol '%s', Field '%s'",
							PDMLStartField->ParentProto->Name, Operand->FieldName);
					}
					return RetVal;
				}

				// Scan all subfields specified within recognized PDML fragment
				while( CurrentField != NULL )
				{
					SubfieldFound= false;
					TempPDMLElement= TempPDMLElement->FirstChild;

					while (TempPDMLElement)
					{
						if ( strncmp(CurrentField, TempPDMLElement->Name, strlen(CurrentField)) == 0 )
						{
							SubfieldFound= true;
							break;
						}

						TempPDMLElement= TempPDMLElement->NextField;
					}

					// Get next subfield
					CurrentField = strtok(NULL, NETPDL_COMMON_SYNTAX_SEP_FIELDS);
				}

				if ( SubfieldFound )
				{
					RetVal= CPDMLMaker::ScanForFieldRefValue(TempPDMLElement, Operand->ProtoName, TempPDMLElement->Name,
																&FieldOffset, BufferMaxSize, BufferMask, m_errbuf, m_errbufSize);
				}
				else
				{
					RetVal= nbWARNING;
				}
			}

			// Release memory resources previously allocated
			free(FieldName);

			if (RetVal != nbSUCCESS)
			{
				if (RetVal == nbWARNING)
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
						"The NetPDL protocol database contains a reference to a field that cannot be located in the PDML fragment: Protocol '%s', Field '%s'",
						PDMLStartField->ParentProto->Name, Operand->FieldName);
				}
				return RetVal;
			}

			// Check if we need to extract the whole string or only a subset of it
			if (Operand->OffsetStartAt)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetStartAt, PDMLStartField, &StartAt);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}
			else
				StartAt= 0;

			if (Operand->OffsetSize)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetSize, PDMLStartField, BufferMaxSize);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			RetVal= m_netPDLVariables->GetVariableBuffer(m_netPDLVariables->m_defaultVarList.PacketBuffer,
														FieldOffset + StartAt, *BufferMaxSize, BufferValue, &BufferSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

			if (BufferSize != *BufferMaxSize)
			{
				// The packet buffer appears to be truncated; the field size is larger than data contained in the packet buffer
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "The packet buffer appears to be truncated.");
				return nbWARNING;
			}

			// If we have a masked field accessed by [], let's move the mask of this field onward
			if ((StartAt) && (*BufferMask))
				(*BufferMask) = *BufferMask + StartAt * 2; 

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD_THIS:
		{
		struct _nbNetPDLExprFieldRef* Operand;
		char *FieldName;
		char *CurrentField;
		unsigned int FieldOffset;
		unsigned int StartAt;
		unsigned int BufferSize;

			Operand= (struct _nbNetPDLExprFieldRef*) OperandBase;

			if (PDMLStartField == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "We're trying to locate the value of a fieldref within a NULL PDML fragment");
				return nbFAILURE;
			}

			// Duplicate current field name
			FieldName= strdup(Operand->FieldName);
			if (FieldName == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: insufficient memory");
				return nbFAILURE;
			}

			// Get the name of the first field:
			CurrentField= strtok(FieldName, NETPDL_COMMON_SYNTAX_SEP_FIELDS);

			// Get the name of the first subfield:
			CurrentField= strtok(NULL, NETPDL_COMMON_SYNTAX_SEP_FIELDS);

			if (CurrentField == NULL)
			{
				// Subfields are missing, let's return the single field value
				RetVal= CPDMLMaker::ScanForFieldRefValue(PDMLStartField, NULL, NULL,
															&FieldOffset, BufferMaxSize, BufferMask, m_errbuf, m_errbufSize);
			}
			else
			{
			struct _nbPDMLField *TempPDMLElement= PDMLStartField;
			uint8_t SubfieldFound;

				// Recognize PDML Element relevant to first field
				RetVal= CPDMLMaker::ScanForFieldRef(PDMLStartField, NULL, NULL, &TempPDMLElement, m_errbuf, m_errbufSize); 

				if (RetVal != nbSUCCESS)
				{
					if (RetVal == nbWARNING)
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
							"The NetPDL protocol database contains a reference to a field that cannot be located in the PDML fragment: Protocol '%s', Field '%s'",
							PDMLStartField->ParentProto->Name, Operand->FieldName);
					}
					return RetVal;
				}

				// Scan all subfields specified within recognized PDML fragment
				while( CurrentField != NULL )
				{
					SubfieldFound= false;
					TempPDMLElement= TempPDMLElement->FirstChild;

					while (TempPDMLElement)
					{
						if ( strncmp(CurrentField, TempPDMLElement->Name, strlen(CurrentField)) == 0 )
						{
							SubfieldFound= true;
							break;
						}

						TempPDMLElement= TempPDMLElement->NextField;
					}

					// Get next subfield
					CurrentField = strtok(NULL, NETPDL_COMMON_SYNTAX_SEP_FIELDS);
				}

				if ( SubfieldFound )
				{
					RetVal= CPDMLMaker::ScanForFieldRefValue(TempPDMLElement, Operand->ProtoName, TempPDMLElement->Name,
																&FieldOffset, BufferMaxSize, BufferMask, m_errbuf, m_errbufSize);
				}
				else
				{
					RetVal= nbWARNING;
				}
			}

			// Release memory resources previously allocated
			free(FieldName);

			// Check if we need to extract the whole string or only a subset of it
			if (Operand->OffsetStartAt)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetStartAt, PDMLStartField, &StartAt);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}
			else
				StartAt= 0;

			if (Operand->OffsetSize)
			{
				RetVal= EvaluateExprNumber(Operand->OffsetSize, PDMLStartField, BufferMaxSize);
				if (RetVal != nbSUCCESS)
					return RetVal;
			}

			RetVal= m_netPDLVariables->GetVariableBuffer(m_netPDLVariables->m_defaultVarList.PacketBuffer,
															FieldOffset + StartAt, *BufferMaxSize, BufferValue, &BufferSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

			if (BufferSize != *BufferMaxSize)
			{
				// The packet buffer appears to be truncated; the field size is larger than data contained in the packet buffer
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "The packet buffer appears to be truncated.");
				return nbWARNING;
			}

			// If we have a masked field accessed by [], let's move the mask of this field onward
			if ((StartAt) && (*BufferMask))
				(*BufferMask) = *BufferMask + StartAt * 2; 

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_EXPR:
		{
			RetVal= EvaluateExprString(OperandBase, PDMLStartField, BufferValue, BufferMaxSize);
			
			if (RetVal != nbSUCCESS)
				return RetVal;

			return nbSUCCESS;
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_EXTRACTSTRING:
		{
		struct _nbNetPDLExprFunctionRegExp* Operand;
		int RexExpReturnCode;
		unsigned char *TmpString;
		unsigned int TmpStringSize;
		int MatchingOffset[MATCHING_OFFSET_COUNT];

			Operand= (struct _nbNetPDLExprFunctionRegExp*) OperandBase;

			// Let's get the buffer in which search has to be done
			if (EvaluateExprString(Operand->SearchBuffer, PDMLStartField, &TmpString, &TmpStringSize) == nbFAILURE)
				return nbFAILURE;

#ifdef _DEBUG
			if (TmpString == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Unexpected buffer equal to zero");
				return nbFAILURE;
			}
#endif

			RexExpReturnCode= pcre_exec(
				(pcre *) Operand->PCRECompiledRegExp,		// the compiled pattern
				NULL,								// no extra data - we didn't study the pattern
				(char*) TmpString,					// the subject string
				TmpStringSize,						// the length of the subject
				0,									// start at offset 0 in the subject
				0,									// default options
				MatchingOffset,						// output vector for substring information
				MATCHING_OFFSET_COUNT);				// number of elements in the output vector

			if (RexExpReturnCode >= 0)	// Match found
			{
			int ReturnedMatch;

				ReturnedMatch= Operand->MatchToBeReturned * 2;

				if (ReturnedMatch >= (MATCHING_OFFSET_COUNT / 3 * 2))
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: a structure is not big enough for keeping the result.");
					return nbFAILURE;
				}

				*BufferMaxSize= MatchingOffset[ReturnedMatch + 1] - MatchingOffset[ReturnedMatch];
				*BufferValue= TmpString + MatchingOffset[ReturnedMatch];
				*BufferMask= NULL;

				return nbSUCCESS;
			}

			if (RexExpReturnCode == -1)	// Match not found
			{
				*BufferMaxSize= 0;
				return nbSUCCESS;
			}

			// Otherwise, there's an error
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
									"Error in Regular Expression match: PCRE reported error %d.", RexExpReturnCode);
			return nbFAILURE;
		}

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_INT2BUF:
		{
		struct _nbNetPDLExprFunctionInt2Buf* Operand;
		unsigned int Result;

			Operand= (struct _nbNetPDLExprFunctionInt2Buf*) OperandBase;
			
			RetVal= GetOperandNumber(Operand->NumericExpression, PDMLStartField, &Result);

			if (RetVal != nbSUCCESS)
				return RetVal;

			NetPDLLongToHexDump(Result, Operand->ResultSize, Operand->Result);

			*BufferValue= Operand->Result;
			*BufferMaxSize= Operand->ResultSize;
			*BufferMask= NULL;

			return nbSUCCESS;
		};

		case nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHANGEBYTEORDER:
		{
		struct _nbNetPDLExprFunctionChangeByteOrder* Operand;
		unsigned int ResultStringLen;
		unsigned char *ResultString;

			Operand= (struct _nbNetPDLExprFunctionChangeByteOrder*) OperandBase;

			RetVal= EvaluateExprString(Operand->OriginalStringExpression, PDMLStartField, &ResultString, &ResultStringLen);

			if (RetVal != nbSUCCESS)
				return RetVal;

			*BufferMaxSize= ResultStringLen;
			*BufferValue= Operand->ResultBuffer;
			*BufferMask= NULL;

			switch (ResultStringLen)
			{
				case 1:
				{
					Operand->ResultBuffer[0]= ResultString[0];
				}; break;

				case 2:
				{
					Operand->ResultBuffer[0]= ResultString[1];
					Operand->ResultBuffer[1]= ResultString[0];
				}; break;

				case 4:
				{
					Operand->ResultBuffer[0]= ResultString[3];
					Operand->ResultBuffer[1]= ResultString[2];
					Operand->ResultBuffer[2]= ResultString[1];
					Operand->ResultBuffer[3]= ResultString[0];
				}; break;

				case 8:
				{
					Operand->ResultBuffer[0]= ResultString[7];
					Operand->ResultBuffer[1]= ResultString[6];
					Operand->ResultBuffer[2]= ResultString[5];
					Operand->ResultBuffer[3]= ResultString[4];
					Operand->ResultBuffer[4]= ResultString[3];
					Operand->ResultBuffer[5]= ResultString[2];
					Operand->ResultBuffer[6]= ResultString[1];
					Operand->ResultBuffer[7]= ResultString[0];
				}; break;

				default:
				{
					errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize,
							"The changebyteorder() function cannot operate when string size is different from 1, 2, 4 or 8 (only if 64 bits are supported) bytes.");
					return nbFAILURE;
				}
			}

			return nbSUCCESS;
		}

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Found a tag which is not a valid element within an expression.");
			return nbFAILURE;
		}
	}
}



/*!
	\brief Assign a value to a run-time variable.

	This method is used to set the value of a variable; lookuptables use another function.

	\param AssignVariableElement: the node that contains the variable.

	\param PDMLStartField: pointer to a field within the current PDML fragment, which is
	the starting point to evaluate the given expression (i.e. it determines the starting point 
	to retrieve the value of fieldref or such).

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message is stored in the m_errbuf variable.
*/
int CNetPDLExpression::EvaluateAssignVariable(struct _nbNetPDLElementAssignVariable *AssignVariableElement, struct _nbPDMLField *PDMLStartField)
{
unsigned int Value;
unsigned char* BufferValue;
unsigned int BufferValueSize;
int RetVal;

	switch (AssignVariableElement->VariableDataType)
	{
		case nbNETPDL_VARIABLE_TYPE_NUMBER:
		case nbNETPDL_VARIABLE_TYPE_PROTOCOL:
		{
			RetVal= EvaluateExprNumber(AssignVariableElement->ExprTree, PDMLStartField, &Value);

			if (RetVal != nbSUCCESS)
				return RetVal;

			m_netPDLVariables->SetVariableNumber((long) (AssignVariableElement->CustomData), Value);

			return nbSUCCESS;

		}; break;

		case nbNETPDL_VARIABLE_TYPE_BUFFER:
		{
			RetVal= EvaluateExprString(AssignVariableElement->ExprTree, PDMLStartField, &BufferValue, &BufferValueSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

#ifdef _DEBUG
			if ((AssignVariableElement->OffsetSize) && (AssignVariableElement->OffsetSize != BufferValueSize))
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: offset size different from the buffer value size.");
				return nbFAILURE;
			}
#endif

			m_netPDLVariables->SetVariableBuffer((long) (AssignVariableElement->CustomData), BufferValue, AssignVariableElement->OffsetStartAt, BufferValueSize);
			return nbSUCCESS;

		}; break;

		case nbNETPDL_VARIABLE_TYPE_REFBUFFER:
		{
			RetVal= EvaluateExprString(AssignVariableElement->ExprTree, PDMLStartField, &BufferValue, &BufferValueSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

#ifdef _DEBUG
			if ((AssignVariableElement->OffsetSize) && (AssignVariableElement->OffsetSize != BufferValueSize))
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: offset size different from the buffer value size.");
				return nbFAILURE;
			}
#endif

			m_netPDLVariables->SetVariableRefBuffer((long) (AssignVariableElement->CustomData), BufferValue, AssignVariableElement->OffsetStartAt, BufferValueSize);

			return nbSUCCESS;

		}; break;

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: unexpected value.");
			return nbFAILURE;

		}; break;
	}
}


/*!
	\brief Assign a value to a member of a lookup table.

	This method is used to change the value of a member of a lookup table.

	\param LookupTable: the element that defines the lookup table.

	\param PDMLStartField: pointer to a field within the current PDML fragment, which is
	the starting point to evaluate the given expression (i.e. it determines the starting point 
	to retrieve the value of fieldref or such).

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error.
	In case of error, the error message is stored in the m_errbuf variable.
*/
int CNetPDLExpression::EvaluateAssignLookupTable(struct _nbNetPDLElementAssignLookupTable *LookupTable, struct _nbPDMLField *PDMLStartField)
{
int RetVal;
unsigned int Value;
unsigned char* BufferValue;
unsigned int BufferValueSize;

	switch (LookupTable->FieldDataType)
	{
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
		{
			RetVal= EvaluateExprNumber(LookupTable->ExprTree, PDMLStartField, &Value);

			if (RetVal != nbSUCCESS)
				return RetVal;

			RetVal= m_netPDLVariables->SetTableDataNumber((long) (LookupTable->TableCustomData), (long) (LookupTable->FieldCustomData), Value);

			return RetVal;

		}; break;

		case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
		{
			RetVal= EvaluateExprString(LookupTable->ExprTree, PDMLStartField, &BufferValue, &BufferValueSize);

			if (RetVal != nbSUCCESS)
				return RetVal;

#ifdef _DEBUG
			if ((LookupTable->OffsetSize) && (LookupTable->OffsetSize != BufferValueSize))
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: offset size different from the buffer value size.");
				return nbFAILURE;
			}
#endif
			RetVal= m_netPDLVariables->SetTableDataBuffer((long) (LookupTable->TableCustomData), (long) (LookupTable->FieldCustomData), BufferValue, LookupTable->OffsetStartAt, BufferValueSize);

			return RetVal;

		}; break;

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: unexpected value.");
			return nbFAILURE;

		}; break;
	}

	return nbSUCCESS;
}


/*!
	\brief Evaluate a lookup-table.

	This method is used to perform some computation on a lookup table.

	\param LookupTableEntry: the element that has to be updated in the lookup table.

	\param PDMLStartField: pointer to a field within the current PDML fragment, which is
	the starting point to evaluate the given expression (i.e. it determines the starting point 
	to retrieve the value of fieldref or such).

	\return nbSUCCESS if everything is fine, nbFAILURE in case or error. nbWARNING can be returned
	in case a lookup has to be performed, and the key cannot be found in the lookup table.
	The nbWARNING case does not corresponds to an error.
	In case of error, the error message is stored in the m_errbuf variable.
*/
int CNetPDLExpression::EvaluateLookupTable(struct _nbNetPDLElementUpdateLookupTable *LookupTableEntry, struct _nbPDMLField *PDMLStartField)
{
struct _nbLookupTableKey* KeyList;
struct _nbLookupTableData* DataList;
struct _nbNetPDLElementLookupKeyData* TempKeyData;
int RetVal;
int TableID;
unsigned int value;
int i;

	TableID= (long) LookupTableEntry->TableCustomData;

	KeyList= m_netPDLVariables->GetStructureForTableKey(TableID);
	if (KeyList == NULL)
		return nbFAILURE;

	i= 0;
	TempKeyData= LookupTableEntry->FirstKey;

	while(TempKeyData)
	{
		switch(KeyList[i].KeyType)
		{
			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
			{
				RetVal= EvaluateExprNumber(TempKeyData->ExprTree, PDMLStartField, &value);
				*(KeyList[i].Value) = value;
				if (RetVal != nbSUCCESS)
					return RetVal;
			}; break;

			case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
			{
				RetVal= EvaluateExprString(TempKeyData->ExprTree, PDMLStartField, &(KeyList[i].Value), &(KeyList[i].Size));
				if (RetVal != nbSUCCESS)
					return RetVal;
			}; break;

			default:
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: unexpected value.");
				return nbFAILURE;
			}; break;
		}

		if (LookupTableEntry->KeysHaveMasks)
			KeyList[i].Mask= TempKeyData->Mask;

		i++;
		TempKeyData= TempKeyData->NextKeyData;
	}


	switch (LookupTableEntry->Action)
	{
		case nbNETPDL_UPDATELOOKUPTABLE_ACTION_ADD:
		{
		unsigned int Timestamp;

			DataList= m_netPDLVariables->GetStructureForTableData(TableID);
			if (DataList == NULL)
				return nbFAILURE;

			i= 0;
			TempKeyData= LookupTableEntry->FirstData;

			while(TempKeyData)
			{
				switch(DataList[i].DataType)
				{
					case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER:
					case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL:
					{
					unsigned int Value;

						RetVal= EvaluateExprNumber(TempKeyData->ExprTree, PDMLStartField, &Value);
						DataList[i].Value= (unsigned char*) Value;
	
						if (RetVal != nbSUCCESS)
							return RetVal;
					}; break;

					case nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER:
					{
						RetVal= EvaluateExprString(TempKeyData->ExprTree, PDMLStartField, &(DataList[i].Value), &(DataList[i].Size));
						if (RetVal != nbSUCCESS)
							return RetVal;
					}; break;

					default:
					{
						errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: unexpected value.");
						return nbFAILURE;
					}; break;
				}

				i++;
				TempKeyData= TempKeyData->NextKeyData;
			}

			m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.TimestampSec, &Timestamp);

			return (m_netPDLVariables->AddTableEntry(TableID, KeyList, DataList, Timestamp, LookupTableEntry->KeysHaveMasks, LookupTableEntry->Validity, LookupTableEntry->KeepTime, LookupTableEntry->HitTime, LookupTableEntry->NewHitTime));

		}; break;

		case nbNETPDL_UPDATELOOKUPTABLE_ACTION_PURGE:
		{
		unsigned int Timestamp;

			m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.TimestampSec, &Timestamp);

			return (m_netPDLVariables->PurgeTableEntry(TableID, KeyList, LookupTableEntry->KeysHaveMasks, Timestamp));
		}; break;

		case nbNETPDL_UPDATELOOKUPTABLE_ACTION_OBSOLETE:
		{
		unsigned int Timestamp;

			m_netPDLVariables->GetVariableNumber(m_netPDLVariables->m_defaultVarList.TimestampSec, &Timestamp);

			return (m_netPDLVariables->ObsoleteTableEntry(TableID, KeyList, LookupTableEntry->KeysHaveMasks, Timestamp));
		}; break;

		default:
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: unexpected value.");
			return nbFAILURE;
		}; break;
	}

	return nbSUCCESS;
}
