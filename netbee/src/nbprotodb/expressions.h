/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once		/* Do not include this file more than once */

#include <nbprotodb_exports.h>


#ifdef __cplusplus
extern "C" {
#endif


#define CREATE_EXPR_ALLOW_NUMBER_ONLY 1
#define CREATE_EXPR_ALLOW_BUFFER_ONLY 2
#define CREATE_EXPR_ALLOW_ALL         3
#define CREATE_EXPR_ALLOW_BOOL_ONLY	  4  //[icerrato] 

int CreateExpression(const char *ExprString, int AllowExprType, char **AllocatedExprString, struct _nbNetPDLExprBase** AllocatedExprTree, char *ErrBuf, int ErrBufSize);

struct _nbNetPDLExprNumber* CreateNumberItem(const char *Token, int Radix);
struct _nbNetPDLExprString* CreateStringItem(const char *AsciiString);
struct _nbNetPDLExprBoolean *CreateBooleanItem(const char *Token); //[icerrato]
struct _nbNetPDLExprProtoRef *CreateProtocolReferenceItem(const char *Token);
char* AllocateAsciiString(const char *Token, unsigned int *NewStringSize, int TerminateStringWithZero, int TokenDoesNotHaveApex);

struct _nbNetPDLExprLookupTable* CreateLookupTableItem(const char* Token, int* TokenType);

void* CreateAliasItem(const char* Token, int* TokenType);

void* CreateVariableItem(const char* Token, int* TokenType);
struct _nbNetPDLExprLookupTable* CreateLookupTableItem(const char* Token, int* TokenType);
struct _nbNetPDLExprFieldRef* CreateProtoFieldItem(const char *Token);

struct _nbNetPDLExprVariable* UpdateVariableItem(void *Variable, void *StartOffset, void *Size);
struct _nbNetPDLExprFieldRef* UpdateProtoFieldItem(void *ProtoField, void *StartOffset, void *Size);

struct _nbNetPDLExprOperator* CreateOperatorItem(int Token);
struct _nbNetPDLExprBase* CreateExpressionItem(void *Operand1, void* Operator, void* Operand2);


struct _nbNetPDLExprFunctionIsASN1Type* CreateFunctionIsASN1Type(struct _nbNetPDLExprString * StringOperand, struct _nbNetPDLExprNumber *ClassNumber, struct _nbNetPDLExprNumber *TagNumber);
struct _nbNetPDLExprFunctionRegExp* CreateFunctionHasStringItem(struct _nbNetPDLExprBase* SearchingBuffer, const char* RegularExpression, struct _nbNetPDLExprNumber* CaseSensitive);
struct _nbNetPDLExprFunctionRegExp* CreateFunctionExtractStringItem(struct _nbNetPDLExprBase* SearchingBuffer, const char* RegularExpression, struct _nbNetPDLExprNumber* CaseSensitive, struct _nbNetPDLExprNumber* MatchToBeReturned);
struct _nbNetPDLExprFunctionBuf2Int* CreateFunctionBuffer2IntItem(struct _nbNetPDLExprBase* StringOperand);
struct _nbNetPDLExprFunctionInt2Buf* CreateFunctionInt2BufferItem(struct _nbNetPDLExprBase* NumericOperand, struct _nbNetPDLExprNumber* SizeOfResultBuffer);
struct _nbNetPDLExprFunctionIsPresent* CreateFunctionIsPresentItem(struct _nbNetPDLExprFieldRef* FieldOperand);
struct _nbNetPDLExprFunctionAscii2Int* CreateFunctionAscii2IntItem(struct _nbNetPDLExprBase* StringOperand);
struct _nbNetPDLExprFunctionChangeByteOrder* CreateFunctionChangeByteOrderItem(struct _nbNetPDLExprBase* StringOperand);
struct _nbNetPDLExprFunctionCheckUpdateLookupTable* CreateFunctionCheckUpdateLookupTableItem(struct _nbNetPDLExprVariable* LookupTable, struct _nbParamsLinkedList* ParameterList);

struct _nbParamsLinkedList* CreateParameterList(struct _nbParamsLinkedList* PreviousParameterList, struct _nbNetPDLExprBase* AdditionalExpression);


void FreeOperatorItem(struct _nbNetPDLExprOperator* OperatorElement);
void FreeOperandItem(struct _nbNetPDLExprBase* GenericOperandElement);
void FreeExpression(struct _nbNetPDLExprBase* ExpressionElement);

#ifdef __cplusplus
}
#endif
