/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once

/*!
	\file netpdlexpression.h

	This file defines the class used to manage NetPDL expressions
*/

#include <nbee_pxmlreader.h>
#include "netpdlvariables.h"


#define MATCHING_OFFSET_COUNT 30 /* For returning results from regex; this number should be a multiple of 3 */


/*!
	\brief This class manages NetPDL expressions

	This class accepts an 'expr' node (or such), and it returns the value of the expression.
*/
class CNetPDLExpression
{
public:
	CNetPDLExpression(CNetPDLVariables *NetPDLVars, char *ErrBuf, int ErrBufSize);
	virtual ~CNetPDLExpression();

	// Evaluating methods
	int EvaluateExprNumber(struct _nbNetPDLExprBase *ExprNode, _nbPDMLField *PDMLStartField, unsigned int *Result);
	int EvaluateExprString(struct _nbNetPDLExprBase *ExprNode, _nbPDMLField *PDMLStartField, unsigned char **ResultString, unsigned int *ResultStringLen);
	int EvaluateSwitch(struct _nbNetPDLElementSwitch *SwitchNodeInfo, _nbPDMLField *PDMLStartField, struct _nbNetPDLElementCase **Result);

	int EvaluateAssignVariable(struct _nbNetPDLElementAssignVariable *AssignVariableElement, struct _nbPDMLField *PDMLStartField);
	int EvaluateAssignLookupTable(struct _nbNetPDLElementAssignLookupTable *LookupTable, struct _nbPDMLField *PDMLStartField);
	int EvaluateLookupTable(struct _nbNetPDLElementUpdateLookupTable *LookupTableEntry, struct _nbPDMLField *PDMLStartField);

private:
	int GetOperandBuffer(struct _nbNetPDLExprBase *OperandBase, struct _nbPDMLField *PDMLStartField, 
								  unsigned char **BufferValue, char **BufferMask, unsigned int *BufferMaxSize);
	int GetOperandNumber(struct _nbNetPDLExprBase *OperandBase, struct _nbPDMLField *PDMLStartField, unsigned int *ResultValue);

	//! Pointer to the run-time variables managed by the NetPDL engine
	CNetPDLVariables *m_netPDLVariables;

	//! Pointer to the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	char *m_errbuf;
	//! Size of the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	int m_errbufSize;
};
