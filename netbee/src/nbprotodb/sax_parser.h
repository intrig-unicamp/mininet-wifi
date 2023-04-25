/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



// Allow including this file only once
#pragma once

#include <nbprotodb_exports.h>


//! This class organizes the content of the NetPDL Protocol Database.
class CNetPDLSAXParser
{
public:
	CNetPDLSAXParser();
	virtual ~CNetPDLSAXParser();

	/*! 
		\brief Returns a string keeping the last error message that occurred within the current instance of the class

		\return A buffer that keeps the last error message.
		This buffer will always be NULL terminated.
	*/
	char *GetLastError() { return m_errbuf;	}

	struct _nbNetPDLDatabase *Initialize(const char* FileName, int Flags);
	void Cleanup();

private:
	int UpdateProtoRefInExpression(struct _nbNetPDLExprBase* Expression);
	void PrintElements(int CurrentLevel= 0, uint32_t CurrentElementIdx= 1);

	void VisitElement(struct _nbNetPDLElementBase *FirstElement, int Level, char *ADTName);
	int DuplicateInnerADT(struct _nbNetPDLElementBase *FirstChildElement/*, int Level*//*, char *ADTName*/);
	int DuplicateADT(char *ADTToDuplicate, struct _nbNetPDLElementBase *NetPDLParentElement, struct _nbNetPDLElementBase *NetPDLFirstElementToLink, struct _nbNetPDLElementReplace *FirstReplaceElement);

	//! Buffer that keeps the error message (if any)
	char m_errbuf[2048];
};

