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
	\brief Prototype for functions that serializes a NetPDL element.

	This function serializes a NetPDL element for getting ready to save it on disk.
	Pointers to char strings are replaced by the string itself (terminated by a '0' char), 
	in 8-bit ascii representation.
	Integers and numbers are saved in network byte order.

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
typedef int (*nbNetPDLElementSerializeHandler) (struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize);


int SerializeElementGeneric(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize);
int SerializeElementProto(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize);
int SerializeElementShowTemplate(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize);
int SerializeElementShowSumTemplate(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize);


#ifdef __cplusplus
}
#endif
