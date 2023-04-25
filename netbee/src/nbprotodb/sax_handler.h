/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <nbprotodb_exports.h>

XERCES_CPP_NAMESPACE_USE

#define NETPDL_MAX_NESTING_LEVELS 30




class CNetPDLSAXHandler : public DefaultHandler
{
public:
    CNetPDLSAXHandler(char *ErrBuf, int ErrBufSize);
    ~CNetPDLSAXHandler();


    // -----------------------------------------------------------------------
    //  Handlers for the SAX2 DocumentHandler interface
    // -----------------------------------------------------------------------
    void startElement(const XMLCh* const URI, const XMLCh* const LocalName, const XMLCh* const Qname, const Attributes& Attributes);
    void endElement(const XMLCh* const URI, const XMLCh* const LocalName, const XMLCh* const Qname);


    // -----------------------------------------------------------------------
    //  Handlers for the SAX ErrorHandler interface
    // -----------------------------------------------------------------------
	void warning(const SAXParseException& Exception);
    void error(const SAXParseException& Exception);
    void fatalError(const SAXParseException& Exception);

	bool ErrorOccurred() {return m_errorOccurred;}


private:
	uint32_t NetPDLNestingLevels[NETPDL_MAX_NESTING_LEVELS];

	unsigned int m_currentLevel;
	unsigned int m_currNetPDLElementNestingLevel;
	bool m_errorOccurred;

	//! Pointer to the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	char *m_errbuf;

	//! Size of the buffer that will keep the error message (if any); this buffer belongs to the class that creates this one.
	int m_errbufSize;
};
