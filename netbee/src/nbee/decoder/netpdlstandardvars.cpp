/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include <stdlib.h>		// for malloc/free()
#include <memory.h>		// for memcpy()


#include <nbprotodb_defs.h>

#include "../utils/netpdlutils.h"
#include "netpdlstandardvars.h"
#include "netpdlexpression.h"
#include "../globals/globals.h"
#include "../globals/utils.h"
#include "../globals/debug.h"



/*!
	\brief Default constructor

	\param ErrBuf: pointer to a user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer belongs to the class that creates this one.
	Data stored in this buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.
*/
CNetPDLStandardVars::CNetPDLStandardVars(char* ErrBuf, int ErrBufSize)
{
	m_errbuf= ErrBuf;
	m_errbufSize= ErrBufSize;

	m_currNumVariables= 0;

	memset(&m_defaultVarList, 0, sizeof(m_defaultVarList));
}


//! Default destructor
CNetPDLStandardVars::~CNetPDLStandardVars()
{
	// De-allocate buffers (if needed)
	for (int i= 0; i < m_currNumVariables; i++)
	{
		if (m_variableList[i].Type == nbNETPDL_VARIABLE_TYPE_BUFFER)
			free(m_variableList[i].ValueBuffer);
	}
}


// Documented in base class
int CNetPDLStandardVars::CreateVariable(struct _nbNetPDLElementVariable* Variable)
{
	// First, check that we have still space in our array
	if (m_currNumVariables >= NETPDL_MAX_NVARS)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: reached the maximum number of allowed NetPDL variables.");
		return nbFAILURE;
	}

	// Then, check that no variables with that name exist
	for (int i= 0; i < m_currNumVariables; i++)
	{
		if (strcmp(Variable->Name, m_variableList[i].Name) == 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Cannot create two NetPDL variables with the same name ('%s').", Variable->Name);
			return nbFAILURE;
		}
	}

	// Initialize structure
	memset(&m_variableList[m_currNumVariables], 0, sizeof(struct _variableList));

	// Copy data in the structure
	sstrncpy(m_variableList[m_currNumVariables].Name, Variable->Name, sizeof(m_variableList[m_currNumVariables].Name));

	m_variableList[m_currNumVariables].Type= Variable->VariableDataType;
	m_variableList[m_currNumVariables].Validity= Variable->Validity;

	m_variableList[m_currNumVariables].InitValueNumber= Variable->InitValueNumber;
	m_variableList[m_currNumVariables].InitValueStringSize= Variable->InitValueStringSize;
	m_variableList[m_currNumVariables].InitValueString= Variable->InitValueString;

	if (Variable->VariableDataType == nbNETPDL_VARIABLE_TYPE_BUFFER)
	{
		m_variableList[m_currNumVariables].ValueBuffer= new unsigned char [Variable->Size];

		if (m_variableList[m_currNumVariables].ValueBuffer == NULL)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Not enough memory for building internal structures.");
			return nbFAILURE;
		}

		// Initialize structure
		memset(m_variableList[m_currNumVariables].ValueBuffer, 0, Variable->Size);

		m_variableList[m_currNumVariables].SizeBuffer= Variable->Size;
		m_variableList[m_currNumVariables].Size= 0;
	}

	if (Variable->VariableDataType == nbNETPDL_VARIABLE_TYPE_REFBUFFER)
			m_variableList[m_currNumVariables].Size= 0;

	// Initialize variables (if needed)
	if (m_variableList[m_currNumVariables].InitValueNumber)
		m_variableList[m_currNumVariables].ValueNumber= m_variableList[m_currNumVariables].InitValueNumber;

	if (m_variableList[m_currNumVariables].InitValueString)
		memcpy(m_variableList[m_currNumVariables].ValueBuffer, m_variableList[m_currNumVariables].InitValueString, m_variableList[m_currNumVariables].InitValueStringSize);


	// Increment the current number of variables
	m_currNumVariables++;

	return nbSUCCESS;
}


// Documented in base class
void CNetPDLStandardVars::SetVariableNumber(int VariableID, unsigned int Value)
{
#ifdef _DEBUG
	if (VariableID >= m_currNumVariables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: trying to access to a non-existing variable.");
		return;
	}
#endif

	m_variableList[VariableID].ValueNumber= Value;
}


// Documented in base class
void CNetPDLStandardVars::SetVariableBuffer(int VariableID, unsigned char* Value, int StartingOffset, int Size)
{
#ifdef _DEBUG
	if (VariableID >= m_currNumVariables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: trying to access to a non-existing variable.");
		return;
	}
#endif

	if (Size > m_variableList[VariableID].SizeBuffer - StartingOffset)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "A variable is not big enough to copy the given amount of data.");
		return;
	}

	memcpy(&(m_variableList[VariableID].ValueBuffer[StartingOffset]), Value, Size);
	m_variableList[VariableID].Size= StartingOffset + Size;
}


// Documented in base class
void CNetPDLStandardVars::SetVariableRefBuffer(int VariableID, unsigned char* PtrValue, int StartingOffset, int Size)
{
#ifdef _DEBUG
	if (VariableID >= m_currNumVariables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: trying to get a non-existing variable.");
		return;
	}
#endif

	m_variableList[VariableID].ValueBuffer= &PtrValue[StartingOffset];
	m_variableList[VariableID].Size= Size;
}


// Documented in base class
void CNetPDLStandardVars::GetVariableNumber(int VariableID, unsigned int* ReturnValue)
{
#ifdef _DEBUG
	if (VariableID >= m_currNumVariables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: trying to access to a non-existing variable.");
		return;
	}
#endif

	*ReturnValue= m_variableList[VariableID].ValueNumber;
}


// Documented in base class
void CNetPDLStandardVars::GetVariableBuffer(int VariableID, unsigned char** ReturnBufferPtr, unsigned int* ReturnBufferSize)
{
#ifdef _DEBUG
	if (VariableID >= m_currNumVariables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: trying to access to a non-existing variable.");
		return;
	}
#endif

	*ReturnBufferSize= m_variableList[VariableID].Size;
	*ReturnBufferPtr= m_variableList[VariableID].ValueBuffer;
}


// Documented in base class
int CNetPDLStandardVars::GetVariableBuffer(int VariableID, int StartAt, int Size, unsigned char** ReturnBufferPtr, unsigned int* ReturnBufferSize)
{
#ifdef _DEBUG
	if (VariableID >= m_currNumVariables)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "Internal error: trying to access to a non-existing variable.");
		return nbFAILURE;
	}
#endif

	if ((StartAt + Size) > m_variableList[VariableID].Size)
	{
		// There's still a special case here: we may have the packet buffer, 
		// and we may want to get some data that exceeds the packet buffer
		// (e.g. we have to do a $packetbuffer[$currentoffset:4] to check which field follows)
		// because we have only a portion of the captured packet (this happens also in case of an
		// ICMP packet containing a fragment of the IP packet)
		// In this case, we do not have to return an error; however, we cannot return a smaller
		// buffer which may be less than the size we would like to have, because other functions
		// may complain. Therefore, we do not have too much choice here... and we return a warning.

		if (VariableID == m_defaultVarList.PacketBuffer)
		{
			*ReturnBufferSize= 0;
			*ReturnBufferPtr= NULL;
			return nbWARNING;
		}
		else
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, 
								"Requested an offset that does not exist within a NetPDL variable.");
			return nbFAILURE;
		}
	}

	if (Size == 0)
		*ReturnBufferSize= m_variableList[VariableID].Size - StartAt;
	else
		*ReturnBufferSize= Size;

	*ReturnBufferPtr= &(m_variableList[VariableID].ValueBuffer[StartAt]);

	return nbSUCCESS;
}


// Documented in base class
void CNetPDLStandardVars::DoGarbageCollection(int TimestampSec)
{
	for (int i= 0; i < m_currNumVariables; i++)
	{
		if (m_variableList[i].Validity == nbNETPDL_VARIABLE_VALIDITY_THISPACKET)
		{
			switch (m_variableList[i].Type)
			{
				case nbNETPDL_VARIABLE_TYPE_NUMBER:
				case nbNETPDL_VARIABLE_TYPE_PROTOCOL:
				{
					m_variableList[i].ValueNumber= m_variableList[i].InitValueNumber;
				}; break;

				case nbNETPDL_VARIABLE_TYPE_BUFFER:
				{
					if (m_variableList[i].InitValueStringSize)
						memcpy(m_variableList[i].ValueBuffer, m_variableList[i].InitValueString, m_variableList[i].InitValueStringSize);
					else
						memset(m_variableList[i].ValueBuffer, 0, m_variableList[i].SizeBuffer);
				}; break;

				case nbNETPDL_VARIABLE_TYPE_REFBUFFER:
				{
					m_variableList[i].ValueBuffer= NULL;
					m_variableList[i].SizeBuffer= 0;
				}; break;
			}
		}
	}
}


// Documented in base class
int CNetPDLStandardVars::GetVariableID(const char* Name, int* VariableID)
{
	for (int i= 0; i < m_currNumVariables; i++)
	{
		if (strcmp(Name, m_variableList[i].Name) == 0)
		{
			*VariableID= i;
			return nbSUCCESS;
		}
	}

	errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, m_errbufSize, "'%s' is not a valid NetPDL variable.", Name);
	return nbFAILURE;
}



// Documented in base class
int CNetPDLStandardVars::SetVariableNumber(char* Name, unsigned int Value)
{
int RetVal;
int VariableID;

	RetVal= GetVariableID(Name, &VariableID);

	if (RetVal == nbFAILURE)
		return nbFAILURE;

	SetVariableNumber(VariableID, Value);

	return nbSUCCESS;
}


// Documented in base class
int CNetPDLStandardVars::GetVariableNumber(char* Name, unsigned int* ReturnValue)
{
int RetVal;
int VariableID;

	RetVal= GetVariableID(Name, &VariableID);

	if (RetVal == nbFAILURE)
		return nbFAILURE;

	GetVariableNumber(VariableID, ReturnValue);

	return nbSUCCESS;
}


// Documented in base class
int CNetPDLStandardVars::SetVariableBuffer(char *Name, unsigned char *Value, unsigned int StartingOffset, unsigned int Size)
{
int RetVal;
int VariableID;

	RetVal= GetVariableID(Name, &VariableID);

	if (RetVal == nbFAILURE)
		return nbFAILURE;

	SetVariableBuffer(VariableID, Value, StartingOffset, Size);

	return nbSUCCESS;
}


int CNetPDLStandardVars::SetVariableRefBuffer(char *Name, unsigned char *Value, unsigned int StartingOffset, unsigned int Size)
{
int RetVal;
int VariableID;

	RetVal= GetVariableID(Name, &VariableID);

	if (RetVal == nbFAILURE)
		return nbFAILURE;

	SetVariableRefBuffer(VariableID, Value, StartingOffset, Size);

	return nbSUCCESS;
}


// Documented in base class
int CNetPDLStandardVars::GetVariableBuffer(char *Name, unsigned char** ReturnBufferPtr, unsigned int* ReturnBufferSize)
{
int RetVal;
int VariableID;

	RetVal= GetVariableID(Name, &VariableID);

	if (RetVal == nbFAILURE)
		return nbFAILURE;

	GetVariableBuffer(VariableID, ReturnBufferPtr, ReturnBufferSize);

	return nbSUCCESS;
}




