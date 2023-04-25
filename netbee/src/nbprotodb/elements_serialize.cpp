/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "elements_serialize.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"


int SerializeElementGeneric(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize)
{
	return nbFAILURE;
}


int SerializeElementProto(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize)
{
	return nbFAILURE;
}


int SerializeElementShowTemplate(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize)
{
	return nbFAILURE;
}

int SerializeElementShowSumTemplate(struct _nbNetPDLElementBase *NetPDLElement, char *SerializeBuffer, int SerializeBufferSize, char *ErrBuf, int ErrBufSize)
{
	return nbFAILURE;
}


