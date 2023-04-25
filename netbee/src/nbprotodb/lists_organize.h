/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



// Allow including this file only once
#pragma once


int CountSpecificElement(uint16_t ElementType);
int CountSpecificADTElement(uint16_t ElementType, uint16_t ContainerElementType);
struct _nbNetPDLElementProto **OrganizeListProto(int NumElementsInList, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementShowSumStructure **OrganizeListShowSumStructure(int NumElementsInList, int* Result, char* ErrBuf, int ErrBufSize);
struct _nbNetPDLElementShowSumTemplate **OrganizeListShowSumTemplate(int NumElementsInList, int* Result, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementShowTemplate **OrganizeListShowTemplate(int NumElementsInList, int* Result, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementAdt **OrganizeListGlobalAdt(int NumElementsInList, char *ErrBuf, int ErrBufSize);
struct _nbNetPDLElementAdt **OrganizeListLocalAdt(int NumElementsInList, char *ErrBuf, int ErrBufSize);

