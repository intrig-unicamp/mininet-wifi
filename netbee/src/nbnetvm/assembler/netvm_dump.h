/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/** @file netvm_dump.h
 * \brief This file contains prototypes, definitions and data structures for 
 *  NetVM bytecode dump and disassembly
 * 
 */


#ifdef __cplusplus
extern "C" {
#endif


#define MAX_OP_NAME_LEN	40

typedef struct _OpCodeInfo
{
	char		Name[MAX_OP_NAME_LEN];
	uint32_t	Args;
} OpCodeInfo;


extern OpCodeInfo OpCodeTable[256];

char *nvmDumpInsn(char *instrBuf, uint32_t bufLen, uint8_t **bytecode, uint32_t *lineNum);


#ifdef __cplusplus
}
#endif

