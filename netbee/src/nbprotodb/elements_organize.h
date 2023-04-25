/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



// Allow including this file only once
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

/*!
	\brief Prototype for functions that organizes links within a NetPDL element.

	This function scans the list of elements looking for the information required to
	complete the data structure needed to parse protocols better.
	Pointers to other elements are indexes related to the NetPDL element list.

	This function accepts the following parameters:
	- pointer to the element that has to be serialized
	- pointer to the buffer that will contain the serialized element
	- size of the previous buffer
	- a buffer in which it can print an error string (if something bad occurs)
	- the size of the previous buffer

	This function serializes the element, and it and returns the number of bytes
	used, or nbFAILURE is case of error. In this case, it sets the proper error
	string.
*/
typedef int (*nbNetPDLElementOrganizeHandler) (struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);


// That's just a function for getting the protocol name (when we get an error)
const char* GetProtoName(struct _nbNetPDLElementBase *NetPDLElement);


int OrganizeElementGeneric(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementProto(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);

int OrganizeElementUpdateLookupTable(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);

int OrganizeElementShowTemplate(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementIf(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementCase(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementSwitch(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementExpr(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementSection(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementLoop(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);

int OrganizeElementField(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementSubfield(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementMap(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementFieldmatch(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);

int OrganizeElementSet(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementDefaultItem(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementChoice(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);

int OrganizeElementAdt(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementAdtfield(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementReplace(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);

int OrganizeElementBlock(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);
int OrganizeElementIncludeBlk(struct _nbNetPDLElementBase *NetPDLElementInfo, char *ErrBuf, int ErrBufSize);

int DuplicateADT(char *ADTToDuplicate, struct _nbNetPDLElementBase *NetPDLParentElement, struct _nbNetPDLElementBase *NetPDLFirstElementToLink, struct _nbNetPDLElementReplace *FirstReplaceElement, char *ErrBuf, int ErrBufSize);

#ifdef __cplusplus
}
#endif

