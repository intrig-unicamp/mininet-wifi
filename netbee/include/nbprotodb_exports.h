/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/*!
	\file nbprotodb_exports.h

	This file defines the class used to keep the data created internally by the library when parsing a NetPDL description file.
*/


// Allow including this file only once
#pragma once


#include <stdint.h>		// For int8, ...


//! Macro for getting a pointer to a NetPDL element given its index in the global elements array.
#define nbNETPDL_GET_ELEMENT(nbprotodb, index) (nbprotodb->GlobalElementsList[index])


#ifdef __cplusplus
extern "C" {
#endif


/*********************************************************/
/*  LinkLayerTypes                                       */
/*********************************************************/

/*!
	\brief Enumeration of supported Link Layer types

	These values reflect the ones found in the encapsulation section of the startproto protocol
	description (in the NetPDL protocol database), and are usually also equal to the values
	defined in the libpcap/WinPcap libraries.
*/
typedef enum nbNetPDLLinkLayer_t
{
	nbNETPDL_LINK_LAYER_ETHERNET = 1,		//!< Ethernet Link Layer
	nbNETPDL_LINK_LAYER_TOKEN_RING = 6,		//!< Token Ring Link Layer
	nbNETPDL_LINK_LAYER_FDDI = 10,			//!< FDDI Link Layer
	nbNETPDL_LINK_LAYER_HCI = 13,			//!< HCI Link Layer
	nbNETPDL_LINK_LAYER_IEEE_80211 = 105	//!< IEEE 802.11 Layer
} nbNetPDLLinkLayer_t;


	/*!
	\brief Index (in the NetPDLDatabase->GlobalElementsList array) of a NULL element.

	This value is used in the members of struct _nbNetPDLElementBase as the index that will indicate
	that the referred element is not valid. For instance, if an element does not have a child, the
	->FirstChild member is set to nbNETPDL_INVALID_ELEMENT.
	The corresponding element in the NetPDLDatabase->GlobalElementsList array is set to NULL.
	*/
#define nbNETPDL_INVALID_ELEMENT 0



// This is a trick for creating all child structures which share the first part with
// the struct _nbNetPDLElementBase

//! Keeps the max length of of some strings allocated in the NetPDL datadase.
#define NETPDL_MAX_SHORTSTRING 20


	//! Trick for allowing all the structures inheriting from struct _nbNetPDLElementBase.
#define STRUCT_NBNETPDLELEMENT \
	/*! \brief It defines the type of the current XML element (protocol, field, ...) */ \
	uint16_t Type; \
	/*! \brief Index of the parent element in the Global Element List, or nbNETPDL_INVALID_ELEMENT if it does not have a parent node.*/ \
	uint32_t Parent; \
	/*! \brief Index of the first child element in the Global Element List, or nbNETPDL_INVALID_ELEMENT if it does not have a child node.*/ \
	uint32_t FirstChild; \
	/*! \brief Index of the next sibling element in the Global Element List, or nbNETPDL_INVALID_ELEMENT if it does not have a next sibling node.*/ \
	uint32_t NextSibling; \
	/*! \brief Index of the previous sibling element in the Global Element List, or nbNETPDL_INVALID_ELEMENT if it does not have a previous sibling node.*/ \
	uint32_t PreviousSibling; \
	/*! \brief Name of the XML element; present only in the _DEBUG version of the code. */ \
	char ElementName[NETPDL_MAX_SHORTSTRING]; \
	/*! \brief Ordinal number of this element within the global element list; present only in the _DEBUG version of the code. */ \
	int ElementNumber;	\
	/*! \brief Nesting level; present only in the _DEBUG version of the code. */ \
	int NestingLevel;	\
	/*! Pointer to a structure that manages calls to the external handles. */	\
	struct _nbCallHandlerInfo* CallHandlerInfo;


	/*!
	\brief Structure that contains the common members of all XML elements.

	This structure contains the members that are common to all the more specialized structures
	associated to any XML element.

	This structure is useful to access to any XML element without knowing, a priori, which is the
	correct element. Most of the functions of this module return a generic struct _nbNetPDLElementBase;
	then, you can determine the correct element type by checking the value of the 'Type' member
	of this structure, and then casting this pointer to the correct type.

	Example:
	<pre>
	struct _nbNetPDLElementBase *GenericPtr;
	struct _nbNetPDLElementProto *ProtocolPtr;
	GenericPtr= FunctionReturningNetPDLElement();
	if (GenericPtr->Type == nbNETPDL_EL_PROTO)
	ProtocolPtr= (struct _nbNetPDLElementProto *) GenericPtr;
	</pre>

	However, please take care that members of this structure are not pointers, but indexes
	(in the _nbNetPDLDatabase::GlobalElementsList array) of the corresponding element.
	Therefore, for accessing to a given element the nbNETPDL_GET_ELEMENT() macro is provided,
	such as:

	<pre>
	struct _nbNetPDLDatabase *NetPDLDatabase;
	struct _nbNetPDLElementBase *NetPDLElement, *ParentNetPDLElement;

	NetPDLDatabase= nbProtoDBXMLLoad(NetPDLFile, ErrBuf, sizeof(ErrBuf);
	...
	ParentNetPDLElement= nbNETPDL_GET_ELEMENT(NetPDLDatabase, NetPDLElement->Parent);
	</pre>
	*/
	struct _nbNetPDLElementBase
	{
		STRUCT_NBNETPDLELEMENT
	};




	/********************************************************************/
	/*    Codes used by the structures associated to each element       */
	/********************************************************************/


	/*!
	\brief Define a code that tells which event will trigger the external handle.
	*/
	typedef enum nbNetPDLCallHandlerEventTypes_t
	{
		//!	The externa handle must be called before processing the current element.
		nbNETPDL_CALLHANDLE_EVENT_BEFORE= 20,
		//!	The externa handle must be called after processing the current element.
		nbNETPDL_CALLHANDLE_EVENT_AFTER
	} nbNetPDLCallHandlerEventTypes_t;



	/*!
	\brief Define the different encodings we can have in NetPDL.
	*/
	typedef enum nbNetPDLFieldEncodings_t
	{
		//! The raw bytes wothout decoding.
		nbNETPDL_ID_ENCODING_RAW = 0,
		//! The US-ASCII textual encosing.
		nbNETPDL_ID_ENCODING_ASCII,
		//! The UTF-8 textual encoding.
		nbNETPDL_ID_ENCODING_UTF8,
		//! The UNICODE textual encoding.
		nbNETPDL_ID_ENCODING_UNICODE,
		//! The ASN.1-based Basic Encoding Rules.
		nbNETPDL_ID_ENCODING_BER,
		//! The ASN.1-based Distinguished Encoding Rules.
		nbNETPDL_ID_ENCODING_DER,
		//! The ASN.1-based Canonical Encoding Rules.
		nbNETPDL_ID_ENCODING_CER
	} nbNetPDLFieldEncodings_t;


	/*!
	\brief Define the different type of fields, cfields, subfields, and csubfields, we can have in NetPDL.
	*/
	typedef enum nbNetPDLFieldNodeTypes_t
	{
		//! The field has fixed length.
		nbNETPDL_ID_FIELD_FIXED = 50,
		//! The field is a 'bitfield'.
		nbNETPDL_ID_FIELD_BIT,
		//! The field has variable length.
		nbNETPDL_ID_FIELD_VARIABLE,
		//! The field is terminated by a given token.
		nbNETPDL_ID_FIELD_TOKENENDED,
		//! The field is delimited by a beginning and an ending token.
		nbNETPDL_ID_FIELD_TOKENWRAPPED,
		//! The newline-terminated field.
		nbNETPDL_ID_FIELD_LINE,
		//! The field matches a given regular expression.
		nbNETPDL_ID_FIELD_PATTERN,
		//! The field get all remaining bytes of the current scope.
		nbNETPDL_ID_FIELD_EATALL,
		//! The field is aligned to a given offset (i.e. padding).
		nbNETPDL_ID_FIELD_PADDING,
		//! The field can be managed only through a plugin.
		nbNETPDL_ID_FIELD_PLUGIN,
		//! The Type-Length-Value complex field.
		nbNETPDL_ID_CFIELD_TLV,
		//! The token-ended/token-surrounded complex field.
		nbNETPDL_ID_CFIELD_DELIMITED,
		//! The newline-terminated complex field.
		nbNETPDL_ID_CFIELD_LINE,
		////! The token-separator-token complex field.
		//nbNETPDL_ID_CFIELD_TST,
		//! The header-style line complex field.
		nbNETPDL_ID_CFIELD_HDRLINE,
		//! The complex field which deal with named subpattern.
		nbNETPDL_ID_CFIELD_DYNAMIC,
		//! The complex field that follow an ASN.1 encoding rule.
		nbNETPDL_ID_CFIELD_ASN1,
		//! The complex field that contains an entire XML document.
		nbNETPDL_ID_CFIELD_XML,
		//! The special value for simple subfields (this code is used only internally at run-time).
		nbNETPDL_ID_SUBFIELD_SIMPLE,
		//! The special value for simple subfields (this code is used only internally at run-time).
		nbNETPDL_ID_SUBFIELD_COMPLEX,
		////! The special value for run-time evaluated subfields.
		//nbNETPDL_ID_SUBFIELD_CHOICE,
		//! The special value for Processing Instructions map fields that are within XML complex field.
		nbNETPDL_ID_FIELD_MAP_XML_PI,
		////! The special value for Processing Instruction Attributes map fields that are within XML complex field.
		////nbNETPDL_ID_FIELD_MAP_XML_PI_ATTR,
		//! The special value for Document Types map fields that are within XML complex field.
		nbNETPDL_ID_FIELD_MAP_XML_DOCTYPE,
		//! The special value for Elements map fields that are within XML complex field.
		nbNETPDL_ID_FIELD_MAP_XML_ELEMENT,
		////! The special value for Element Attributes map fields that are within XML complex field.
		//nbNETPDL_ID_FIELD_MAP_XML_ELEMENT_ATTR,
		//! The special value for 'adtfield' elements.
		nbNETPDL_ID_ADTFIELD,
		//! The special value for run-time evaluated replacing of adt fields.
		nbNETPDL_ID_FIELD_REPLACE,
		//! The special value for simple fields listed within 'set'/'choice' elements.
		nbNETPDL_ID_FIELDMATCH,
		//! The special value for default elements listed within 'set'/'choice' elements.
		nbNETPDL_ID_DEFAULT_MATCH
	} nbNetPDLFieldNodeTypes_t;


	/*!
	\brief Define the different portion of complex fields we can have in NetPDL.
	*/
	typedef enum nbNetPDLSubfieldNodePortions_t
	{
		//! The 'tlv' subfield for 'type' portion.
		nbNETPDL_ID_SUBFIELD_PORTION_TLV_TYPE = 150,
		//! The 'tlv' subfield for 'length' portion.
		nbNETPDL_ID_SUBFIELD_PORTION_TLV_LENGTH,
		//! The 'tlv' subfield for 'value' portion.
		nbNETPDL_ID_SUBFIELD_PORTION_TLV_VALUE,
		//! The 'hdrline' subfield for 'header name' portion.
		nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HNAME,
		//! The 'headerline' subfield for 'header value' portion.
		nbNETPDL_ID_SUBFIELD_PORTION_HDRLINE_HVALUE,
		//! The 'dynamic' special subfield for all its portions.
		nbNETPDL_ID_SUBFIELD_PORTION_DYNAMIC
	} nbNetPDLSubfieldNodePortions_t;


	/*!
	\brief Define the four modes that can be used to terminate a 'loop'.
	*/
	typedef enum nbNetPDLLoopMode_t
	{
		//! The loop is terminated after a given amount of decoded data
		nbNETPDL_ID_LOOP_SIZE = 200,
		//! The loop is terminated after a given number of rounds
		nbNETPDL_ID_LOOP_T2R,
		//! The loop is terminated due to a condition (evaluated at the beginning)
		nbNETPDL_ID_LOOP_WHILE,
		//! The loop is terminated due to a condition (evaluated at the end)
		nbNETPDL_ID_LOOP_DOWHILE
	} nbNetPDLLoopMode_t;


	/*!
	\brief Define the modes supported by a 'loopctrl' element.
	*/
	typedef enum nbNetPDLLoopCtrlMode_t
	{
		//! The loopctrl does not exist (this code is used only internally at run-time)
		nbNETPDL_ID_LOOPCTRL_NONE = 0,	/* Warning: this value must be zero (otherwise some portions of the code do no work) */
		//! The loopctrl forces the loop to break
		nbNETPDL_ID_LOOPCTRL_BREAK,
		//! The loopctrl forces the loop to restart
		nbNETPDL_ID_LOOPCTRL_CONTINUE
	} nbNetPDLLoopCtrlMode_t;


	/*!
	\brief Defines the visualization modes of a field.
	*/
	typedef enum nbNetPDLFieldTemplateDigitTypes_t
	{
		//! Visualize the field using hex numbers
		nbNETPDL_ID_TEMPLATE_DIGIT_HEX= 400,
		//! Visualize the field using hex numbers, but without prepending the 'x' to it
		nbNETPDL_ID_TEMPLATE_DIGIT_HEXNOX,
		//! Visualize the field using dec numbers
		nbNETPDL_ID_TEMPLATE_DIGIT_DEC,
		//! Visualize the field using binary numbers
		nbNETPDL_ID_TEMPLATE_DIGIT_BIN,
		//! Visualize the field using floats
		nbNETPDL_ID_TEMPLATE_DIGIT_FLOAT,
		//! Visualize the field using doubles
		nbNETPDL_ID_TEMPLATE_DIGIT_DOUBLE,
		//! Visualize the field using ascii characters
		nbNETPDL_ID_TEMPLATE_DIGIT_ASCII
	} nbNetPDLFieldTemplateDigitTypes_t;


	/*!
	\brief Defines the type of native visualization funtion requested.
	*/
	typedef enum nbNetPDLFieldTemplateNativeFunctionTypes_t
	{
		//! Format the field using a native printing function for IPv4 addresses
		nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_IPV4= 450,
		//! Format the field using a native printing function for ASCII fields
		nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCII,
		//! Format the field using a native printing function for ASCII lines
		nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_ASCIILINE,
		//! Format the field using a native printing function for HTTP field (that keeps only the text after the ':' character)
		nbNETPDL_ID_TEMPLATE_NATIVEFUNCT_HTTPCONTENT
	} nbNetPDLFieldTemplateNativeFunctionTypes_t;


	/*!
	\brief Defines the return type of an expression.
	*/
	typedef enum nbNetPDLExprReturnTypes_t
	{
		//! The return value of this expression is a number.
		nbNETPDL_ID_EXPR_RETURNTYPE_NUMBER = 500,
		//! The return value of this expression is a string.
		nbNETPDL_ID_EXPR_RETURNTYPE_BUFFER,
		//! The return value of this expression is a boolean.
		nbNETPDL_ID_EXPR_RETURNTYPE_BOOLEAN,						//[icerrato]
		//! The return value of this expression is not important.
		nbNETPDL_ID_EXPR_RETURNTYPE_DONTMIND
	} nbNetPDLExprReturnTypes_t;


	/*!
	\brief Define the type of the operand we may have.
	*/
	typedef enum nbNetPDLExprOperandTypes_t
	{
		//! Operand is a number.
		nbNETPDL_ID_EXPR_OPERAND_NUMBER = 550,
		//! Operand is a string.
		nbNETPDL_ID_EXPR_OPERAND_STRING,
		//! Operand is a boolean.
		nbNETPDL_ID_EXPR_OPERAND_BOOLEAN,
		//! Operand is a reference to a protocol.
		nbNETPDL_ID_EXPR_OPERAND_PROTOREF,
		//! Operand is a NetPDL variable.
		nbNETPDL_ID_EXPR_OPERAND_VARIABLE,
		//! Operand is a NetPDL lookup table.
		nbNETPDL_ID_EXPR_OPERAND_LOOKUPTABLE,
		//! Operand is a reference to a NetPDL field.
		nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD,
		//! Operand is a subfield-related reference to a NetPDL field.
		nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD_2,
		//! Operand is the 'this' special NetPDL field.
		nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD_THIS,
		//! Operand is the subfield-related 'this' special NetPDL field.
		nbNETPDL_ID_EXPR_OPERAND_PROTOFIELD_THIS_2,
		//! Operand is another expression.
		nbNETPDL_ID_EXPR_OPERAND_EXPR,
		//! Operand is the hasstring() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_HASSTRING,
		//! Operand is the extractstring() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_EXTRACTSTRING,
		//! Operand is the ispresent() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISPRESENT,
		//! Operand is the isasn1type() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ISASN1TYPE,
		//! Operand is the buf2int() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_BUF2INT,
		//! Operand is the int2buf() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_INT2BUF,
		//! Operand is the ascii2int() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_ASCII2INT,
		//! Operand is the changebyteorder() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHANGEBYTEORDER,
		//! Operand is the checklookuptable() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_CHECKLOOKUPTABLE,
		//! Operand is the updatelookuptable() function.
		nbNETPDL_ID_EXPR_OPERAND_FUNCTION_UPDATELOOKUPTABLE
	} nbNetPDLExprOperandTypes_t;


	/*!
	\brief List of different types of variables supported by NetPDL.
	*/
	typedef enum nbNetPDLVariableTypes_t
	{
		//! Variable is an integer number.
		nbNETPDL_VARIABLE_TYPE_NUMBER = 580,
		//! Variable is a boolean value.
		nbNETPDL_VARIABLE_TYPE_BOOLEAN, //[icerrato]
		//! Variable is a buffer of a given size. Data is *copied* in this buffer (while variables of type nbNETPDL_VARIABLE_REFBUFFER do not copy data).
		nbNETPDL_VARIABLE_TYPE_BUFFER,
		//! Variable is a buffer passed by reference (e.g. the packet buffer).
		nbNETPDL_VARIABLE_TYPE_REFBUFFER,
		//! Variable is a protocol (or a pointer to a protocol).
		nbNETPDL_VARIABLE_TYPE_PROTOCOL
	} nbNetPDLVariableTypes_t;


	//! List of different types of 'validity' allowed for NetPDL variables.
	typedef enum nbNetPDLVariableValidityTypes_t
	{
		//! Variable has 'static' duration (it lasts forever).
		nbNETPDL_VARIABLE_VALIDITY_STATIC = 585,
		//! Variable looses its content after each packet.
		nbNETPDL_VARIABLE_VALIDITY_THISPACKET
	} nbNetPDLVariableValidityTypes_t;


	//! List of different types of fields (keys and data) within lookup tables supported by NetPDL.
	typedef enum nbNetPDLLookupTableKeyDataTypes_t
	{
		//! Value of key or data is an integer number.
		nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_NUMBER = 590,
		//! Value of key or data is a buffer of a given size.
		nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_BUFFER,
		//! Value of key or data is a protocol (or a pointer to a protocol).
		nbNETPDL_LOOKUPTABLE_KEYANDDATA_TYPE_PROTOCOL
	} nbNetPDLLookupTableKeyDataTypes_t;


	//! List of different types of 'action' allowed for NetPDL lookup tables.
	typedef enum nbNetPDLUpdateLookupTableActionTypes_t
	{
		//! We want to add a new item in the lookup table.
		nbNETPDL_UPDATELOOKUPTABLE_ACTION_ADD = 600,
		//! An existing item has to be purged from the lookup table.
		nbNETPDL_UPDATELOOKUPTABLE_ACTION_PURGE,
		//! An existing item is going to me marked as obsolete, and pretty soon it will be deleted.
		nbNETPDL_UPDATELOOKUPTABLE_ACTION_OBSOLETE
	} nbNetPDLUpdateLookupTableActionTypes_t;


	//! List of different types of 'add' we have in a lookup table.
	typedef enum nbNetPDLUpdateLookupTableAddValidityTypes_t
	{
		//! The new item must stay in the lookup table forever, unless deleted manually (e.g. through a PURGE action).
		nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPFOREVER = 650,
		//! The new item must stay in the lookup table for max time (in seconds).
		nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_KEEPMAXTIME,
		//! The new item will be replaced with the current timestamp at every hit.
		nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_UPDATEONHIT,
		//! The new, masked item, will be replaced by the first 'standard' item (without masks) that matches this record.
		nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_REPLACEONHIT,
		//! The new, masked item, will stay in the lookup table for max time (in seconds); in addition, a new exact item will be created on hit.
		nbNETPDL_UPDATELOOKUPTABLE_VALIDITY_ADDONHIT
	} nbNetPDLUpdateLookupTableAddValidityTypes_t;


	/*!
	\brief Define a code that tells the type of the operator contained into an 'expr' tag.
	*/
	typedef enum nbNetPDLExprOperatorTypes_t
	{
		// Math operators
		//! 'Plus' operator
		nbNETPDL_ID_EXPR_OPER_PLUS = 700,
		//! 'Minus' operator
		nbNETPDL_ID_EXPR_OPER_MINUS,
		//! 'Mul' operator
		nbNETPDL_ID_EXPR_OPER_MUL,
		//! 'Integer division' operator
		nbNETPDL_ID_EXPR_OPER_DIV,
		//! 'Modulus' operator
		nbNETPDL_ID_EXPR_OPER_MOD,
		//! 'Bitwise And' operator
		nbNETPDL_ID_EXPR_OPER_BITWAND,
		//! 'Bitwise Or' operator
		nbNETPDL_ID_EXPR_OPER_BITWOR,
		//! 'Bitwise Not' operator
		nbNETPDL_ID_EXPR_OPER_BITWNOT,

		// Boolean operators
		//! 'Equal' operator
		nbNETPDL_ID_EXPR_OPER_EQUAL,
		//! 'Not Equal' operator
		nbNETPDL_ID_EXPR_OPER_NOTEQUAL,
		//! 'Greather or Equal' operator
		nbNETPDL_ID_EXPR_OPER_GREATEQUAL,
		//! 'Greather than' operator
		nbNETPDL_ID_EXPR_OPER_GREAT,
		//! 'Less or Equal' operator
		nbNETPDL_ID_EXPR_OPER_LESSEQUAL,
		//! 'Less than' operator
		nbNETPDL_ID_EXPR_OPER_LESS,
		//! 'Boolean And' operator
		nbNETPDL_ID_EXPR_OPER_AND,
		//! 'Boolean Or' operator
		nbNETPDL_ID_EXPR_OPER_OR,
		//! 'Boolean Not' operator
		nbNETPDL_ID_EXPR_OPER_NOT
	} nbNetPDLExprOperatorTypes_t;




	/*!
		\brief Define a code that tells the type of attribute required by the 'pdmlfield'.
	*/
	typedef enum nbNetPDLShowCodePDMLFieldAttribTypes_t
	{
		//!	We want the 'value' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_VALUE= 1000,
		//!	We want the 'name' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_NAME,
		//!	We want the 'longname' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_LONGNAME,
		//!	We want the 'showmap' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWMAP,
		//!	We want the 'showdtl' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOWDTL,
		//!	We want the 'show' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SHOW,
		//!	We want the 'masked' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_MASK,
		//!	We want the 'position' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_POSITION,
		//!	We want the 'size' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PDMLFIELD_ATTRIB_SIZE
	} nbNetPDLShowCodePDMLFieldAttribTypes_t;


	/*!
		\brief Defines a code that tells the type of attribute required by the 'pkthdr'.
	*/
	typedef enum nbNetPDLShowCodePacketHdrAttribTypes_t
	{
		//!	We want the 'num' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_NUM= 1100,
		//!	We want the 'name' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_LEN,
		//!	We want the 'longname' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_CAPLEN,
		//!	We want the 'showmap' attribute
		nbNETPDL_ID_NODE_SHOWCODE_PKTHDR_ATTRIB_TIMESTAMP
	} nbNetPDLShowCodePacketHdrAttribTypes_t;

	/********************************************************************/
	/*    Definition of the structure associated to the global          */
	/*    protocol database                                             */
	/********************************************************************/


	/*! 
	\brief Values used for the initialization of the NetPDL Protocol Database.

	These flags are used to determine if the NetPDL database that is being loaded
	(or that has been loaded) is a 'full' database, containing visualization primitives,
	or a reduced database containing only protocol format and encapsulation.
	Furthermore, we can also specify if the NetPDL file has to be validated against
	the XSD schema.

	For instance, visualization primitives are usually required when the user ask to perform
	a complete decoding of network packets. However, they are useless in case of packet filters.
	Therefore, in the second case the user can force the load of a reduced version of the database.

	In case the user does not set these flags, the nbProtoDBXMLLoad() expects a full database.
*/
enum nbProtoDBFlags
{
	//! The NetBee Protocol Database must be a 'full' DB, i.e. it must include also visualization primitives.
	nbPROTODB_FULL= 1,

	//! The NetBee Protocol Database is requested to include only protocol format and encapsulation.
	nbPROTODB_MINIMAL= 2,

	//! The NetBee Protocol Database is requested to be validated against the XSD schema.
	nbPROTODB_VALIDATE= 4
};


	/*!
		\brief It organizes the content of the NetPDL Protocol Database.

		This structure contains all the items that are needed to access to the NetPDL Protocol Database.
		It contains a set of pointers to other structures which provide a more compact (and efficient)
		view of the protocol database, without needing an XML parser to get access to the data.
	*/
	struct _nbNetPDLDatabase
	{
		//! Version number of the current NetPDL database (major revision)
		uint32_t VersionMajor;
		//! Version number of the current NetPDL database (minor revision)
		uint32_t VersionMinor;

		//! Creation date of the current NetPDL database (in format "dd-mm-yyyy")
		char* CreationDate;
		//! Creator of the current NetPDL database.
		char* Creator;

		//! Type of the NetPDL protocol database we loaded. It can assume the values defined in #nbProtoDBFlags.
		uint32_t Flags;

		//! \brief List of the protocols defined in the NetPDL protocol database.
		//! Each item contains the most relevant elements of each protocol.
		struct _nbNetPDLElementProto **ProtoList;

		//! Number of protocols contained in the previous array.
		uint32_t ProtoListNItems;

		//! Index (in the ProtoList array) related to the starting protocol.
		uint32_t StartProtoIndex;

		//! Index (in the ProtoList array) related to the default protocol.
		uint32_t DefaultProtoIndex;

		//! Index (in the ProtoList array) related to the 'etherpadding' protocol.
		uint32_t EtherpaddingProtoIndex;

		//! List of visualization templates defined in the entire NetPDL database.
		struct _nbNetPDLElementShowTemplate **ShowTemplateList;

		//! Number of visualization templates contained in previous array.
		uint32_t ShowTemplateNItems;

		//! List of summary visualization templates defined in the entire NetPDL database.
		struct _nbNetPDLElementShowSumTemplate **ShowSumTemplateList;

		//! Number of summary visualization templates contained in previous array.
		uint32_t ShowSumTemplateNItems;

		//! List of items that defines the structure of the summary visualization template.
		struct _nbNetPDLElementShowSumStructure **ShowSumStructureList;

		//! Number of items contained in previous array.
		uint32_t ShowSumStructureNItems;

		//! List of items that defines the internal structure of the ADT fields.
		struct _nbNetPDLElementAdt **ADTList;

		//! Number of items contained in previous array.
		uint32_t ADTNItems;

		//! Local list of items that defines the internal structure of the ADT fields.
		struct _nbNetPDLElementAdt **LocalADTList;

		//! Number of items contained in previous array.
		uint32_t LocalADTNItems;

		/*! \brief Contains the complete list of NetPDL elements, also the ones which have not been structured in previous items.

		Please note that the first item (which corresponds to index nbNETPDL_INVALID_ELEMENT) is set to NULL; this is
		used to have a NULL element to refer to. This is needed when other elements in this list must refer to an
		element which does not exist (e.g. they do not have any child, or such).
		Therefore, the number of elements in this array is one more than the number of XML elements contained in
		the NetPDL file, and the first element is NULL.

		All the XML elements encountered in the XML file are associated to a struct _nbNetPDLElementBase, or 
		a structure derived from it. This allow scanning the XML file through these structures instead
		of using libraries (and structures) that manage XML files.

		However, the most important elements of the NetPDL file (e.g. protocols) are further
		organized in a more appropriate way (e.g. struct _nbNetPDLElementProto). This further
		easies the interaction with the NetPDL database.
		*/
		struct _nbNetPDLElementBase **GlobalElementsList;

		//! Contains the number of items in the GlobalElementsList array.
		unsigned int GlobalElementsListNItems;

		//! Contains the number of additional items in the GlobalElementsList array due to ADTs.
		unsigned int GlobalElementsListNItemsPlusADTCopies;

		//! Pointer to the first expression of the NetPDL file; expressions are linked to each other through a list of pointers.
		//! This allow to scan (outside the NetPDLDatabase module) all the expressions and to do some processing on them.
		struct _nbNetPDLExprBase* ExpressionList;
	};





	/********************************************************************/
	/*    Definition of the structures associated to each element,      */
	/*  which are needed to have faster access to the NetPDL database   */
	/********************************************************************/

	//! Structure that contains most of the data required to process the call to the external handle.
	struct _nbCallHandlerInfo
	{
		//!	String that contains the external call handle. Please note that the string here is made up of two
		//! pieces (namespace:function:), and that the separator character is not the ':', but it is
		//! the NULL character (which makes parsing simpler). The 'event' is stored in the CallHandleEvent member.
		char *FunctionName;

		//! Type of the event that triggers the call to the external handle.
		//! It can assume the values listed in #nbNetPDLCallHandlerEventTypes_t.
		int FireOnEvent;

		//! Pointer to a structure (of unknown type) that will contain the calling handle parameter. This structure
		//! is provided here for convenience, but it is filled in by the proper NetPDL engine. Therefore
		//! this pointer, as returned by this module, will be always void.
		void *CallHandler;
	};


	//! Structure associated to each 'protocol' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementProto
	{
		STRUCT_NBNETPDLELEMENT

		//! Name  of the current protocol
		char *Name;

		//! Long name  of the current protocol
		char *LongName;

		//! Name of the associated summary template.
		char *ShowSumTemplateName;

		//! Pointer to the first 'verify' section (within the execute-code); this element has a link to other 'verify' sections that may follow.
		struct _nbNetPDLElementExecuteX *FirstExecuteVerify;

		//! Pointer to the first 'init' section (within the execute-code); this element has a link to other 'init' sections that may follow.
		struct _nbNetPDLElementExecuteX *FirstExecuteInit;

		//! Pointer to the first 'before' section (within the execute-code); this element has a link to other 'before' sections that may follow.
		struct _nbNetPDLElementExecuteX *FirstExecuteBefore;

		//! Pointer to the first 'after' section (within the execute-code); this element has a link to other 'after' sections that may follow.
		struct _nbNetPDLElementExecuteX *FirstExecuteAfter;

		//! Pointer to the first field that has to be decoded.
		struct _nbNetPDLElementBase *FirstField;

		//! Pointer to the first 'element in the encapsulation' item (e.g. 'if', 'nextproto', etc.).
		struct _nbNetPDLElementBase *FirstEncapsulationItem;

		//! Pointer to the associated summary template.
		struct _nbNetPDLElementShowSumTemplate *ShowSumTemplateInfo;
	};


	//! Structure associated to each element within the 'execute-code' section (i.e. 'verify', 'init', 'after' and 'before').
	//! It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementExecuteX
	{
		STRUCT_NBNETPDLELEMENT

		//! Complete string of the 'when' expression contained in this element.
		char *WhenExprString;

		//! Pointer to the 'when' expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* WhenExprTree;

		//! Pointer to the next element of the same type (i.e. if this is an 'init' element, the next 'init' section, if it exists).
		struct _nbNetPDLElementExecuteX *NextExecuteElement;
	};


	//! Structure associated to element that declare a NetPDL variable. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementVariable
	{
		STRUCT_NBNETPDLELEMENT

		//! Type of the variable. It can assume the values listed in #nbNetPDLVariableTypes_t.
		nbNetPDLVariableTypes_t VariableDataType;

		//! Name of the variable.
		char* Name;

		//! A code that defines the validity of the packet. It can assume the values listed in #nbNetPDLVariableValidityTypes_t.
		nbNetPDLVariableValidityTypes_t Validity;

		/*!
		\brief Size of the variable. Its meaning depends on the variable type.

		In case the variable is:
		- number: this value is meaningless.
		- protocol: this value is meaningless.
		- buffer: it contains the maximum size of the buffer.
		- refbuffer: this value is meaningless.
		*/
		int Size;

		//! Initialization value (if any). It is valid only in case of a nbNETPDL_VARIABLE_TYPE_NUMBER.
		unsigned int InitValueNumber;
		
		//[icerrato]
		//! Initialization value (if any). It is valid only in case of a nbNETPDL_VARIABLE_TYPE_BOOLEAN.
		int InitValueBoolean;

		//! Initialization value (if any). It is valid only in case of a nbNETPDL_VARIABLE_TYPE_BUFFER.
		unsigned char *InitValueString;

		//! Size of previous buffer (if any). It is valid only in case of a nbNETPDL_VARIABLE_TYPE_BUFFER.
		unsigned int InitValueStringSize;
	};


	//! Structure associated to element that declare a NetPDL lookup table. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementLookupTable
	{
		STRUCT_NBNETPDLELEMENT

		//! Name of the lookup table.
		char* Name;

		//! Maximum number of exact entry records inside the lookup table.
		int NumExactEntries;

		//! Maximum number of masked entry records inside the lookup table.
		int NumMaskEntries;

		//! 'true' if the table allows dynamic entries (the ones that required the automatic purging, through the timestamp).
		int AllowDynamicEntries;

		//! Pointer to the first element of a linked list containing the keys of the lookup table.
		//! This member is valid only in case the variable is a lookup table.
		struct _nbNetPDLElementKeyData* FirstKey;

		//! Pointer to the first element of a linked list containing the data fields of the lookup table.
		//! This member is valid only in case the variable is a lookup table.
		struct _nbNetPDLElementKeyData* FirstData;
	};


	//! Structure associated to key and data of a lookup table variable. It inherits from struct _nbNetPDLElementBase.
	//! This structure is used ONLY when the lookup table is created; struct _nbNetPDLElementLookupKeyData is used at run-time.
	struct _nbNetPDLElementKeyData
	{
		STRUCT_NBNETPDLELEMENT

		//! Name of the key/data.
		char* Name;

		//! Data Type; It can assume the values listed in #nbNetPDLLookupTableKeyDataTypes_t.
		nbNetPDLLookupTableKeyDataTypes_t KeyDataType;

		//! Size of the key/data in bytes. It is present only for 'buffer' fields; the size of other fields
		//! must be computed at run-time according to the data type (e.g. numbers can be 4 bytes, etc).
		int Size;

		//! Pointer to the next element of the same type (i.e. next key if this is a key, next data if this is data).
		struct _nbNetPDLElementKeyData* NextKeyData;
	};


	//! Structure associated to aliases.
	struct _nbNetPDLElementAlias
	{
		STRUCT_NBNETPDLELEMENT

		//! Name of the key/data.
		char* Name;

		//! String that has to be used instead of the current alias.
		char* ReplaceWith;
	};


	//! Structure associated to each 'assign-variable' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementAssignVariable
	{
		STRUCT_NBNETPDLELEMENT

		//! Complete string of the expression contained in this element.
		char* ExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExprTree;

		//! It defines the name of the NetPDL variable (as an ascii string).
		//! This info is a duplication of the same info stored in struct _nbNetPDLElementVariable.
		char* VariableName;

		//! Type of the variable. This is a duplication of the same info stored in struct _nbNetPDLElementVariable.
		//! However, we can either put a pointer to the variable itself, or duplicate this info, and I believe
		//! the simplest way is to duplicate this info.
		int VariableDataType;

		//! It defines the first byte within this variable that has to be used.
		//! This field is valid only for variables that return a string.
		int OffsetStartAt;

		//! It defines the number of bytes that have to be used starting at previous offset.
		int OffsetSize;

		//! It defines a custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the run-time variable.
		void* CustomData;
	};


	//! Structure associated to each 'assign-lookuptable' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementAssignLookupTable
	{
		STRUCT_NBNETPDLELEMENT

		//! It defines the name of the NetPDL table (as an ascii string).
		char* TableName;

		//! It defines the name of the field that has to be modified. Valid only in case of the UPDATE action.
		char* FieldName;

		//! Complete string of the expression contained in this element. Valid only in case of the UPDATE action.
		char* ExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree. Valid only in case of the UPDATE action.
		struct _nbNetPDLExprBase* ExprTree;

		//! Type of the data field. This is a duplication of the same info which is associated to each key,
		//! but it makes processing faster, because (at run-time) we can immediately know the type of the
		//! variable, without having to dig inside the _nbNetPDLElementLookupKeyData to locate this info.
		int FieldDataType;

		//! It defines the first byte within this variable that has to be used.
		//! This field is valid only for variables that return a string.
		int OffsetStartAt;

		//! It defines the number of bytes that have to be used starting at previous offset.
		int OffsetSize;

		//! It defines a custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the lookup table.
		void* TableCustomData;

		//! It defines a second custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the member of the lookup table we want to get.
		void* FieldCustomData;
	};


	//! Structure associated to each 'update-lookuptable' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementUpdateLookupTable
	{
		STRUCT_NBNETPDLELEMENT

		//! It defines the name of the NetPDL table (as an ascii string).
		char* TableName;

		//! Action that has to be performed on the lookup table.
		//! It can assume the values listed in #nbNetPDLUpdateLookupTableActionTypes_t.
		nbNetPDLUpdateLookupTableActionTypes_t Action;

		//! 'true' if some of the keys are masked. This flag is used in order to turn on some custom processing.
		int KeysHaveMasks;

		//! In case we're going to add a 'masked' item, it contains how long this masked entry must stay in the lookup table.
		//! It can assume the values listed in #nbNetPDLUpdateLookupTableAddValidityTypes_t. It is valid only in case of the 'add' action.
		nbNetPDLUpdateLookupTableAddValidityTypes_t Validity;

		//! Value of the 'keeptime' attribute, when present.
		int KeepTime;

		//! Value of the 'hittime' attribute, when present.
		int HitTime;

		//! Value of the 'newhittime' attribute, when present.
		int NewHitTime;

		//! Pointer to the first element of a linked list containing the keys of the lookup table.
		struct _nbNetPDLElementLookupKeyData* FirstKey;

		//! Pointer to the first element of a linked list containing the data fields of the lookup table.
		struct _nbNetPDLElementLookupKeyData* FirstData;

		//! It defines a custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the lookup table.
		void* TableCustomData;
	};



	//! Structure associated to key when performing an update of a lookup table. It inherits from struct _nbNetPDLElementBase.
	//! This structure is used at run-time; struct _nbNetPDLElementKeyData is used at variable creation.
	struct _nbNetPDLElementLookupKeyData
	{
		STRUCT_NBNETPDLELEMENT

		//! Complete string of the expression contained in this element.
		char* ExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExprTree;

		//! Mask that has to be applied to the item, and it is valid only for keys (not for data).
		//! The value is stored in hex, and the size must be equal to the one of the field declared in the lookup table.
		unsigned char* Mask;

		//! Pointer to the next element of the same type (i.e. next key if this is a key, next data if this is data).
		struct _nbNetPDLElementLookupKeyData* NextKeyData;
	};



	//! Structure associated to each 'if' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementIf
	{
		STRUCT_NBNETPDLELEMENT

		//! It contains the first element that has to be executed in case the branch is true.
		struct _nbNetPDLElementBase *FirstValidChildIfTrue;

		//! It contains the first element that has to be executed in case the branch is false.
		struct _nbNetPDLElementBase *FirstValidChildIfFalse;

		//! It contains the first element that has to be executed in case the expression contained in the 'if' fails due to missing data.
		struct _nbNetPDLElementBase *FirstValidChildIfMissingPacketData;

		//! Complete string of the expression contained in this element.
		char *ExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExprTree;
	};



	//! Structure associated to each 'case' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementCase
	{
		STRUCT_NBNETPDLELEMENT

		//! It contains a pointer to the string defined within the 'show' attribute, if present.
		char *ShowString;

		//! It defines the value stored as number.
		unsigned int ValueNumber;

		//! It defines the maximum value allowed in this case expression.
		unsigned int ValueMaxNumber;

		//! It defines the value stored as string.
		//! Since the string can be an hex buffer, the 'ValueStringSize' contains its size.
		//! This string is not NULL terminated; you MUST use the memcmp() functions instead of strcmp().
		unsigned char *ValueString;

		//! It defines the size of the value stored as string.
		unsigned int ValueStringSize;

		//!	It contains a pointer to the next 'case' item, if present.
		//! Please note that a 'switch' element can have different types of child, therefore
		//! walking through the ->NextSibling pointer may return also other types of element.
		struct _nbNetPDLElementCase *NextCase;
	};



	//! Structure associated to each 'switch' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementSwitch
	{
		STRUCT_NBNETPDLELEMENT

		//! First 'case' element present in this switch.
		//! Please note that you can scan all the 'case' elements following the
		//! ->NextCase pointer (of struct _nbNetPDLElementCase)
		struct _nbNetPDLElementCase *FirstCase;

		//! It contains the 'default' element, in case no 'case' exist that are suitable for selection.
		struct _nbNetPDLElementCase *DefaultCase;

		//! Complete string of the expression contained in this element.
		char *ExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExprTree;

		//! '1' if we want to have case-sensitive matching (default), 0 otherwise.
		//! It is meaningful only in case of string-based matching.
		int CaseSensitive;
	};



	//! Structure associated to each 'loop' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementLoop
	{
		STRUCT_NBNETPDLELEMENT

		//! It contains the type of the loop.
		//! It assumes the values defined in #nbNetPDLLoopMode_t.
		nbNetPDLLoopMode_t LoopType;

		//! It contains the first element that has to be executed within the loop.
		struct _nbNetPDLElementBase *FirstValidChildInLoop;

		//! It contains the first element that has to be executed in case the expression contained in the 'loop' fails due to missing data.
		struct _nbNetPDLElementBase *FirstValidChildIfMissingPacketData;

		//! Complete string of the expression contained in this element.
		char *ExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExprTree;
	};



	//! Structure associated to each 'loopctrl' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementLoopCtrl
	{
		STRUCT_NBNETPDLELEMENT

		//! It contains the type of the loopctrl element. 
		//! It assumes the values defined in #nbNetPDLLoopCtrlMode_t.
		nbNetPDLLoopCtrlMode_t LoopCtrlType;
	};


	//! Structure associated to each 'includeblk' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementIncludeBlk
	{
		STRUCT_NBNETPDLELEMENT

		//! It keeps the name of block that has to be included instead of the 'includeblk' directive.
		//! It is valid only for 'includeblk' elements.
		char *IncludedBlockName;

		//! It contains a pointer to the block pointed by the current 'includeblk' element.
		struct _nbNetPDLElementBlock *IncludedBlock;
	};


	//! Structure associated to each 'block' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementBlock
	{
		STRUCT_NBNETPDLELEMENT

		//! Pointer to a string keeping the field name (i.e. 'name' attribute in the NetPDL definition).
		char *Name;

		//! Pointer to a string keeping the field long name (i.e. 'longname' attribute in the NetPDL definition).
		char *LongName;

		//! It keeps the name of the summary visualization template (or NULL if it does not exist).
		char *ShowSumTemplateName;

		//! Pointer to the summary visualization template (or NULL if it does not exist)
		struct _nbNetPDLElementShowSumTemplate *ShowSumTemplateInfo;
	};



	//! Structure associated to each 'set' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementSet
	{
		STRUCT_NBNETPDLELEMENT

		//! It points to the structure that contains information about field to decode through 'set' element.
		struct _nbNetPDLElementFieldBase *FieldToDecode;

		//! It points to the first element that has to be executed in case expressions evaluation fails due to missing data.
		struct _nbNetPDLElementBase *FirstValidChildIfMissingPacketData;

		//! It points to the structure that contains information about 'set'-related exit condition.
		struct _nbNetPDLElementExitWhen *ExitWhen;

		//! It points to the first element that has to be evaluated to perform 'set'-related selection.
		struct _nbNetPDLElementFieldmatch *FirstMatchElement;
	};



	//! Structure that represent the exit condition related to a 'set' element.
	struct _nbNetPDLElementExitWhen
	{
		STRUCT_NBNETPDLELEMENT

		//! Complete string of the matching expression contained in this element.
		char *ExitExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExitExprTree;
	};



	//! Structure associated to each 'choice' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementChoice
	{
		STRUCT_NBNETPDLELEMENT

		//! It points to the structure that contains information about field to decode through 'choice' element.
		struct _nbNetPDLElementFieldBase *FieldToDecode;

		//! It points to the first element that has to be executed in case expressions evaluation fails due to missing data.
		struct _nbNetPDLElementBase *FirstValidChildIfMissingPacketData;

		//! It points to the first element that has to be evaluated to perform 'choice'-related selection.
		struct _nbNetPDLElementFieldmatch *FirstMatchElement;
	};



	//! Structure associated to each 'adt' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementAdt
	{
		STRUCT_NBNETPDLELEMENT

		//! It keeps the name of the ADT
		char *ADTName;

		//! It keeps the name of the protocol if the ADT is local
		char *ProtoName;

		//! It points to the structure that contains information about native type of field.
		struct _nbNetPDLElementFieldBase *ADTFieldInfo;
	};




	//! Structure associated to each 'nextproto' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementNextProto
	{
		STRUCT_NBNETPDLELEMENT

		//! Complete string of the expression contained in this element.
		//! This element is present when the 'name' attribute is missing.
		char *ExprString;
		
		//[icerrato]
		//! Complete string of the expression contained in this element.
		char *ExprPreferred;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		//! This element is present when the 'name' attribute is missing.
		struct _nbNetPDLExprBase* ExprTree;
		
		//[icerrato]
		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase*  ExprPreferredTree;
	};



	/******************************************************/
	/*                Protocol Fields                     */
	/******************************************************/


	//! Trick for allowing all the field-related structures inheriting from struct _nbNetPDLElementFieldBase.
#define STRUCT_NBNETPDLFIELDELEMENT \
	\
	STRUCT_NBNETPDLELEMENT																									\
	\
	/*! \brief Type of the current field. It can assume the values listed in #nbNetPDLFieldNodeTypes_t. */					\
	nbNetPDLFieldNodeTypes_t FieldType;																						\
	\
	/*! \brief Encoding of the current field. It can assume the values listed in #nbNetPDLFieldEncodings_t. */				\
	nbNetPDLFieldEncodings_t Encoding;																						\
	\
	/*! \brief Pointer to a string keeping the field name (i.e. 'name' attribute in the NetPDL definition). */				\
	char *Name;																												\
	\
	/*! \brief Pointer to a string keeping the field long name (i.e. 'longname' attribute in the NetPDL definition). */		\
	char *LongName;																											\
	\
	/*! \brief 'true' if this field appears in network byte order (the default) in the packet dump, 'false' otherwise. */	\
	uint8_t IsInNetworkByteOrder;																								\
	\
	/*! \brief It keeps the name of the visualization template required by this field. */									\
	char *ShowTemplateName;																									\
	\
	/*! \brief Pointer to the visualization template required by this field. */												\
	struct _nbNetPDLElementShowTemplate *ShowTemplateInfo;																	\
	\
	/*! \brief Pointer to a string keeping the owner ADT. */														\
	char *ADTRef;																								\


	//! Trick for allowing all the map-related structures inheriting either struct _nbNetPDLElementFieldBase or from struct _nbNetPDLElementMapBase.
#define STRUCT_NBNETPDLMAPELEMENT \
	\
	STRUCT_NBNETPDLFIELDELEMENT																								\
	\
	/*! It keeps the name of the current element which map element refers to. */											\
	char *RefName;



	/*!
	\brief Common structure associated to each protocol field.

	This structure contains the common members associated to each NetPDL field. This structure
	aims at providing a general way to get access to field-related structures. For instance,
	the proper structure can be inferred by checking the value of the 'FieldType' member
	and using the proper cast.

	Therefore, if you want to get access to the real structure, you need to cast
	the pointer to _nbNetPDLElementFieldBase into the real structure, which will be
	determined by the value of the 'FieldType' member.	

	The usage of this structure is funcionally equivalent to the struct _nbNetPDLElementBase,
	although it is limited to NetPDL fields. The the list of such these elements is defined
	in #nbNetPDLFieldNodeTypes_t.
	*/
	struct _nbNetPDLElementFieldBase
	{
		STRUCT_NBNETPDLFIELDELEMENT
	};



	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a fixed field.
	struct _nbNetPDLElementFieldFixed
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Size (in bytes) of the current field.
		int Size;
	};

	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a bitmask field.
	struct _nbNetPDLElementFieldBit
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Pointer to a string keeping the mask (i.e. 'mask' attribute in the NetPDL definition), as an hex number (without any preceding '0x').
		char *BitMaskString;

		//! Value of the previous mask, if any (i.e. 'mask' attribute in the NetPDL definition).
		unsigned long BitMask;

		//! Size (in bytes) of the current field.
		int Size;

		/*!
		\brief 'true' if this bitfield is the last one of the group, 'false' otherwise.

		Bit fields are located within a 'master' field of fixed size (whose dimension is given in the 'Size' member
		of this structure). However, the problem is that we need to know when the last 'bitfield' has been located,
		because in this case the packet processor must advance its 'packet_offset' pointer. Therefore this flag
		can be used for that: when it is 'true', it means that all bits contained in 'Size' have been processed
		and the packet_offset must be updated in order to process the enxt field.
		*/
		int IsLastBitField;
	};
	
	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a variable field.
	struct _nbNetPDLElementFieldVariable
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Complete string of the expression contained in this element.
		char *ExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExprTree;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a tokenended field.
	struct _nbNetPDLElementFieldTokenEnded
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It contains the sentinel string of a token-ended field.
		//! This member points to an hex buffer (exactly how the token looks like in a packet dump),
		//! not to a char string.
		unsigned char *EndTokenString;

		//! It contains the lenght of previous token string.
		//! This value is required because the token can be any hex char, therefore the user
		//! cannot infer its value from previous buffer.
		unsigned int EndTokenStringSize;

		//! It contains the regular expression that has to be used for terminating the field.
		char *EndRegularExpression;
		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *EndPCRECompiledRegExp;

		//! Complete string of the expression contained in this element (if present).
		char *EndOffsetExprString;
		//! Pointer to the previous expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* EndOffsetExprTree;

		//! Complete string of the expression contained in this element (if present).
		char *EndDiscardExprString;
		//! Pointer to the previous expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* EndDiscardExprTree;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a tokenwrapped field.
	struct _nbNetPDLElementFieldTokenWrapped
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It contains the beginning string of a token-wrapped field.
		//! This member points to an hex buffer (exactly how the token looks like in a packet dump),
		//! not to a char string.
		unsigned char *BeginTokenString;

		//! It contains the lenght of the previous token string.
		//! This value is required because the token can be any hex char, therefore the user
		//! cannot infer its value from previous buffer.
		unsigned int BeginTokenStringSize;

		//! It contains the sentinel string of a token-wrapped field.
		//! This member points to an hex buffer (exactly how the token looks like in a packet dump),
		//! not to a char string.
		unsigned char *EndTokenString;

		//! It contains the lenght of the previous token string.
		//! This value is required because the token can be any hex char, therefore the user
		//! cannot infer its value from previous buffer.
		unsigned int EndTokenStringSize;

		//! It contains the regular expression that has to be used for beginning the field.
		char *BeginRegularExpression;
		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *BeginPCRECompiledRegExp;

		//! It contains the regular expression that has to be used for terminating the field.
		char *EndRegularExpression;
		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *EndPCRECompiledRegExp;

		//! Complete string of the expression contained in this element (if present).
		char *BeginOffsetExprString;
		//! Pointer to the previous expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* BeginOffsetExprTree;

		//! Complete string of the expression contained in this element (if present).
		char *EndOffsetExprString;
		//! Pointer to the previous expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* EndOffsetExprTree;

		//! Complete string of the expression contained in this element (if present).
		char *EndDiscardExprString;
		//! Pointer to the previous expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* EndDiscardExprTree;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a line field.
	struct _nbNetPDLElementFieldLine
	{
		STRUCT_NBNETPDLFIELDELEMENT
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a pattern field.
	struct _nbNetPDLElementFieldPattern
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It contains the regular expression that has to be used for beginning the field.
		char *PatternRegularExpression;

		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *PatternPCRECompiledRegExp;

		//! Boolean value to aim at tuning decoding of the pattern field.
		uint8_t OnPartialDecodingContinue;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to an eatall field.
	struct _nbNetPDLElementFieldEatall
	{
		STRUCT_NBNETPDLFIELDELEMENT
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a padding field.
	struct _nbNetPDLElementFieldPadding
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Size (in bytes) of the requested aligment. For instance, if 'Align == 4'
		//! the current field has to aligned to the next 32-bit boundary.
		int Align;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a plugin field.
	struct _nbNetPDLElementFieldPlugin
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It keeps the name of the plugin that has to be used to process this field.
		char *PluginName;

		//! It keeps the index corresponding to the required &lt;plugin&gt; handler.
		//! This member is meaningful only inside the NetBee library; therefore it must never be used by the user.
		int PluginIndex;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a TLV complex field.
	struct _nbNetPDLElementCfieldTLV
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Size (in bytes) of the 'Type' section.
		int TypeSize;

		//! Size (in bytes) of the 'Length' section.
		int LengthSize;

		//! Complete string of the value extension expression contained in this element.
		char *ValueExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ValueExprTree;

		//! It contains a pointer to the NetPDL element to decode as tlv.type portion.
		struct _nbNetPDLElementBase *TypeSubfield;
		//! It contains a pointer to the NetPDL element to decode as tlv.length portion.
		struct _nbNetPDLElementBase *LengthSubfield;
		//! It contains a pointer to the NetPDL element to decode as tlv.value portion.
		struct _nbNetPDLElementBase *ValueSubfield;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a delimited field.
	struct _nbNetPDLElementCfieldDelimited
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It contains the regular expression that has to be used for trivial beginning of the field.
		char *BeginRegularExpression;
		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *BeginPCRECompiledRegExp;

		//! It contains the regular expression that has to be used for trivial ending of the field.
		char *EndRegularExpression;
		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *EndPCRECompiledRegExp;

		//! Boolean value that aims at tuning decoding of the delimited complex field.
		uint8_t OnMissingBeginContinue;
		//! Boolean value that aims at tuning decoding of the delimited complex field.
		uint8_t OnMissingEndContinue;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a line field.
	struct _nbNetPDLElementCfieldLine
	{
		STRUCT_NBNETPDLFIELDELEMENT
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a hdrline field.
	struct _nbNetPDLElementCfieldHdrline
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It contains the regular expression that has to be used for the separator of the field.
		char *SeparatorRegularExpression;

		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *SeparatorPCRECompiledRegExp;

		//! It contains a pointer to the NetPDL element to decode as hdrline.hname portion.
		struct _nbNetPDLElementBase *HeaderNameSubfield;
		//! It contains a pointer to the NetPDL element to decode as hdrline.hvalue portion.
		struct _nbNetPDLElementBase *HeaderValueSubfield;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a dynamic field.
	struct _nbNetPDLElementCfieldDynamic
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It contains the regular expression that has to be used for the overall pattern of the field.
		char *PatternRegularExpression;

		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *PatternPCRECompiledRegExp;

		//! List of named subpatterns defined within the overall pattern of the field.
		char **NamesList;

		//! Number of visualization templates contained in previous array.
		uint32_t NamesListNItems;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a ASN.1 field.
	struct _nbNetPDLElementCfieldASN1
	{
		STRUCT_NBNETPDLFIELDELEMENT
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a XML field.
	struct _nbNetPDLElementCfieldXML
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Complete string of the expression contained in this element.
		char *SizeExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* SizeExprTree;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a Simple Subfield.
	struct _nbNetPDLElementSubfield
	{
		STRUCT_NBNETPDLFIELDELEMENT
			
		//! Portion of the current subfield. It can assume the values listed in #nbNetPDLSubfieldNodePortions_t.
		nbNetPDLSubfieldNodePortions_t Portion;
		
		//! It keeps the default name of the portion of the current subfield.
		const char *PortionName;

		//! It contains a pointer to the NetPDL complex field that represent the current complex subfield.
		struct _nbNetPDLElementFieldBase *ComplexSubfieldInfo;
	};


	//! Common structure associated to each mapping.
	struct _nbNetPDLElementMapBase
	{
		STRUCT_NBNETPDLMAPELEMENT
	};


	//! Structure that inherits from _nbNetPDLElementMapBase; it contains data related to a PI specific subfield within XML complex field.
	struct _nbNetPDLElementMapXMLPI
	{
		STRUCT_NBNETPDLMAPELEMENT

		//! It contains the regular expression that has to be used for match the Processing Instruction mapped.
		char *XMLPIRegularExpression;

		//! Compiledregular expression according to the internal representation of the PCRE library.
		void *XMLPIPCRECompiledRegExp;

		//! 'true' if the current 'map' element must decode XML attributes, 'false' otherwise.
		uint8_t ShowAttributes;
	};


	//! Structure that inherits from _nbNetPDLElementMapBase; it contains data related to a PI specific subfield within XML complex field.
	struct _nbNetPDLElementMapXMLDoctype
	{
		STRUCT_NBNETPDLMAPELEMENT

		//! It contains the regular expression that has to be used for match the Doctype Element mapped.
		char *XMLDoctypeRegularExpression;

		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *XMLDoctypePCRECompiledRegExp;
	};


	//! Structure that inherits from _nbNetPDLElementMapBase; it contains data related to a Doctype specific subfield within XML complex field.
	struct _nbNetPDLElementMapXMLElement
	{
		STRUCT_NBNETPDLMAPELEMENT

		//! It contains the regular expression that has to be used for match the XML Element mapped.
		char *XMLElementRegularExpression;

		//! Compiled regular expression according to the internal representation of the PCRE library.
		void *XMLElementPCRECompiledRegExp;

		//! 'true' if the current 'map' element must decode XML attributes, 'false' otherwise.
		uint8_t ShowAttributes;

		//! Complete string of the namespace of XML element to map to.
		char *NamespaceString;

		//! Complete string of the namespece of XML element to map to.
		char *HierarcyString;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a 'adtfield' element.
	struct _nbNetPDLElementAdtfield
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! It keeps the name of the ADT to include.
		char *CalledADTName;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data related to a 'rename' element.
	struct _nbNetPDLElementReplace
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Complete string of the NetPDL field to replace.
		char *FieldToRename;

		//!	It contains a pointer to the next 'replace' item, if present.
		struct _nbNetPDLElementReplace *NextReplace;
	};


	//! Structure that inherits from _nbNetPDLElementFieldBase; it contains data within 'set'/'choice' elements.
	struct _nbNetPDLElementFieldmatch
	{
		STRUCT_NBNETPDLFIELDELEMENT

		//! Complete string of the matching expression contained in this element.
		char *MatchExprString;

		//! Pointer to the expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* MatchExprTree;

		//! 'true' if the current 'set'/'choice' element can be selected more than once, 'false' otherwise.
		uint8_t Recurring;

		//!	It contains a pointer to the next 'fieldmatch' item, if present.
		struct _nbNetPDLElementFieldmatch *NextFieldmatch;
	};





	/******************************************************/
	/*             Structures associated to               */
	/*              visualization elements                */
	/******************************************************/



	//! Structure associated to each 'showtemplate' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementShowTemplate
	{
		STRUCT_NBNETPDLELEMENT

		//! Name of the current template.
		char *Name;

		//! It corresponds to the 'showtype' attribute in the NetPDL visualization template 
		//! and it can assume the values defined in #nbNetPDLFieldTemplateDigitTypes_t.
		nbNetPDLFieldTemplateDigitTypes_t ShowMode;

		//! It corresponds to the 'showfast' attribute in the NetPDL visualization template 
		//! and it can assume the values defined in #nbNetPDLFieldTemplateNativeFunctionTypes_t.
		nbNetPDLFieldTemplateNativeFunctionTypes_t ShowNativeFunction;

		//! It corresponds to the 'showgrp' attribute in the NetPDL visualization template.
		int DigitSize;

		//! It corresponds to the 'showsep' attribute in the NetPDL visualization template.
		//! It cannot assume the value of the empty string; in this case, its value is NULL.
		char *Separator;

		//! It corresponds to the 'showplg' attribute in the NetPDL visualization template.
		char *PluginName;

		//! Pointer to the 'showmap' element in the NetPDL visualization template (if any).
		//! Since the mapping table is basically a 'switch', we use here this structure.
		//! It is NULL in case the node is not a 'switch'. This structure must be allocated manually.
		struct _nbNetPDLElementSwitch *MappingTableInfo;

		//! Pointer to the first item of the 'showdtl' element in the NetPDL visualization template (if any).
		struct _nbNetPDLElementBase *CustomTemplateFirstField;


		//! It keeps the index corresponding to the wanted 'showplg' attribute in the NetPDL visualization template.
		//! This member is meaningful only inside the NetBee library; therefore it must never be used.
		int PluginIndex;
	};




	//! Structure associated to each 'showsumtemplate' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementShowSumTemplate
	{
		STRUCT_NBNETPDLELEMENT

		//! Name of the current template.
		char *Name;
	};


	/*!
	\brief Structure associated to each 'showsumstruct' element. It inherits from struct _nbNetPDLElementBase.

	This structure is associated to the items that define the structure of the summary visualization,
	i.e. the number of 'columns' present in it.
	*/
	struct _nbNetPDLElementShowSumStructure
	{
		STRUCT_NBNETPDLELEMENT

		//! Name of the current template.
		char *Name;

		//! Long name of the current template.
		char *LongName;
	};



	//! Structure associated to each 'section' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementSection
	{
		STRUCT_NBNETPDLELEMENT

		//! Pointer to a string keeping the name of the section that has to be used to place the summary.
		//! This field is valid only in case of a 'section' node, otherwise it is NULL.
		char *SectionName;

		//! Pointer to the section in which we want to print the summary.
		//! This field is valid only in case of a 'section' node.
		int SectionNumber;

		//! 'true' if the current 'section' node points to the next section, 'false' otherwise.
		//! This field is valid only in case of a 'section' node.
		uint8_t IsNext;
	};


	//! Structure associated to each 'protohdr' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementProtoHdr
	{
		STRUCT_NBNETPDLELEMENT

		//! It defines the type of the attribute of the PDML field we want to print.
		//! It can assume the values defined in #nbNetPDLShowCodePDMLFieldAttribTypes_t, even
		//! if some of them are not valid for protocols (only for PDML fields).
		//! This field is valid only in case of a 'pdmlproto' node.
		nbNetPDLShowCodePDMLFieldAttribTypes_t ProtoAttribType;
	};


	//! Structure associated to each 'protofield' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementProtoField
	{
		STRUCT_NBNETPDLELEMENT

		//! Pointer to a string keeping the name of the PDML field we want to print.
		//! This field is present only in case of a 'pdmlfield' node, otherwise it is NULL.
		char *FieldName;

		//! It defines the type of the attribute of the PDML field we want to print.
		//! It can assume the values defined in #nbNetPDLShowCodePDMLFieldAttribTypes_t.
		//! This field is valid only in case of a 'pdmlfield' node.
		nbNetPDLShowCodePDMLFieldAttribTypes_t FieldShowDataType;
	};


	//! Structure associated to each 'text' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementText
	{
		STRUCT_NBNETPDLELEMENT

		//! Pointer to a string keeping the string we want to print.
		//! This field is present only in case of a 'text' node, otherwise it is NULL.
		char *Value;

		//	//! It defines when the text has to be inserted. It can assume the values listed in #nbNetPDLElementTextWhenTypes_t.
		//	nbNetPDLElementTextWhenTypes_t When;

		//! Complete string of the expression contained in the 'text' element.
		//! This field is present only in case of a 'text' node (which contains an expression),
		//! otherwise it is NULL.
		char *ExprString;

		//! Pointer to the previous expression, structured according to the struct _nbNetPDLExprBase* syntax tree.
		struct _nbNetPDLExprBase* ExprTree;
	};


	//! Structure associated to each 'pkthdr' element. It inherits from struct _nbNetPDLElementBase.
	struct _nbNetPDLElementPacketHdr
	{
		STRUCT_NBNETPDLELEMENT

		//! It defines the type of the attribute of the packet header we want to print.
		//! It can assume the values defined in #nbNetPDLShowCodePacketHdrAttribTypes_t.
		//! This field is valid only in case of a 'pkthdr' node.
		nbNetPDLShowCodePacketHdrAttribTypes_t Value;
	};





	/******************************************************/
	/*                  Expressions                       */
	/******************************************************/


#ifndef _DEBUG
	/*!
	\brief This structure defines the first part of the structure associated to any operand.

	The use of a 'define' is due to the impossibility to use the inheritance in C code. This 'define'
	will be declared as the first member of any operand structure.
	*/
#define STRUCT_NBNETPDLOPERAND \
	/*! \brief It provides the type of operand; it assumes the values listed in #nbNetPDLExprOperandTypes_t. */			\
	nbNetPDLExprOperandTypes_t Type;																											\
	/*! \brief It tells if the operand / expression returns a number or a buffer; it assumes the values listed in #nbNetPDLExprReturnTypes_t. */			\
	char Token[NETPDL_MAX_SHORTSTRING];																					\
	/*! \brief It tells if the operand / expression returns a number or a buffer; it assumes the values listed in #nbNetPDLExprReturnTypes_t. */			\
	nbNetPDLExprReturnTypes_t ReturnType;																								\
	/*! \brief If 'true', the NetPDL engine should print the result of the expression on screen (in some way).	\
	This is useful to debug the value of an expression (and to check that the result matches				\
	user's expectation).																					\
	*/																											\
	int PrintDebug;																								\
	/*! \brief It is used to create a linked list of expressions.												\
	Helpful if we want to scan all the expressions created from a NetPDL file and do						\
	some processing on it.																					\
	This member is filled only in 'root' expressions, i.e. the first element of a complex expression.		\
	*/																											\
	struct _nbNetPDLExprBase *NextExpression;
#else
	/*!
	\brief This structure defines the first part of the structure associated to any operand.

	The use of a #define is due to the impossibility to use the inheritance in C code. This #define
	will be declared as the first member of any operand structure.
	*/
#define STRUCT_NBNETPDLOPERAND \
	/*! \brief It provides the type of operand; it assumes the values listed in #nbNetPDLExprOperandTypes_t. */			\
	nbNetPDLExprOperandTypes_t Type;																											\
	/*! \brief String (present only in _DEBUG mode) that contains the current token of the expression. */				\
	char Token[NETPDL_MAX_SHORTSTRING];																					\
	/*! \brief It tells if the operand / expression returns a number or a buffer; it assumes the values listed in #nbNetPDLExprReturnTypes_t. */			\
	nbNetPDLExprReturnTypes_t ReturnType;																								\
	/*! \brief If 'true', the NetPDL engine should print the result of the expression on screen (in some way).	\
	This is useful to debug the value of an expression (and to check that the result matches				\
	user's expectation).																					\
	*/																											\
	int PrintDebug;																								\
	/*! \brief It is used to create a linked list of expressions.												\
	Helpful if we want to scan all the expressions created from a NetPDL file and do						\
	some processing on it.																					\
	This member is filled only in 'root' expressions, i.e. the first element of a complex expression.		\
	*/																											\
	struct _nbNetPDLExprBase *NextExpression;
#endif


	/*!
	\brief 'Fake' structure associated to each expression item.

	This is a structure which is never used; however, all the expression elements
	(being them simple operand or complex expressions) share the first part of
	their data with this structure.

	Therefore, this structure is used only for providing a simple method to get
	access to the 'Type' member, which gives the 'real' structure we are handling. 
	Therefore, if you want to get access to the real structure, you need to cast
	the pointer to _nbNetPDLExprBase into the real structure, which will be
	determined by the value of the 'Type' member.	
	*/
	struct _nbNetPDLExprBase
	{
		STRUCT_NBNETPDLOPERAND
	};


	/*!
	\brief Structure associated to each expression.

	This element inherits from struct _nbNetPDLExprBase.
	*/
	struct _nbNetPDLExpression
	{
		STRUCT_NBNETPDLOPERAND

		//! Pointer to the first operand. It can be another expression.
		struct _nbNetPDLExprBase *Operand1;

		//! Pointer to the expression operator. It may be missing.
		struct _nbNetPDLExprOperator *Operator;

		//! Pointer to the second operand. It can be NULL or even another expression.
		struct _nbNetPDLExprBase *Operand2;
	};



	//! Element associated to operators.
	struct _nbNetPDLExprOperator
	{
		//! It provides the type of operator; it assumes the values listed in #nbNetPDLExprOperatorTypes_t.
		nbNetPDLExprOperatorTypes_t OperatorType;

#ifdef _DEBUG
		//! String (present only in _DEBUG mode) that contains the current token of the expression.
		char Token[NETPDL_MAX_SHORTSTRING];
#endif
	};



	//! Element associated to numbers. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprNumber
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the value stored in a 'number' field (in decimal).
		int Value;
	};



	//! Element associated to strings. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprString
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the value stored in a 'string' field. It is a generic string in hex chars and it is not NULL terminated.
		char *Value;

		//! It defines the size of the previous string.
		unsigned int Size;
	};

	//! Element associated to booleans. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprBoolean
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the value stored in a 'boolean' field. It can be 1 ("true"), or different from 1 ("false")
		int Value;
	};


	//! Element associated to protocol references. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprProtoRef
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the value stored in a 'number' field (in decimal).
		int Value;

		//! It defines the name of the protocol we're referring to.
		char *ProtocolName;
	};


	//! Element associated to NetPDL variables. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprVariable
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the name of the run-time variable (as an ascii string).
		//! It is valid only for 'variable' fields.
		char* Name;

		//! It defines a custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the run-time variable itself.
		void* CustomData;

		//! It defines the first byte within this variable that has to be used.
		//! This field is valid only for variables that return a string.
		struct _nbNetPDLExprBase *OffsetStartAt;

		//! It defines the number of bytes that have to be used starting at previous offset.
		struct _nbNetPDLExprBase *OffsetSize;
	};


	//! Element associated to NetPDL lookup tables (used in expressions). This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprLookupTable
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the name of the table that defines the run-time variable (as an ascii string).
		char* TableName;

		//! It defines the name of the field (within the table) that defines the run-time variable (as an ascii string).
		char* FieldName;

		//! It defines a custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the lookup table itself.
		void* TableCustomData;

		//! It defines a second custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the member of the lookup table we want to get.
		void* FieldCustomData;

		//! It defines the first byte within this variable that has to be used.
		//! This field is valid only for variables that return a string.
		struct _nbNetPDLExprBase *OffsetStartAt;

		//! It defines the number of bytes that have to be used starting at previous offset.
		struct _nbNetPDLExprBase *OffsetSize;
	};


	//! Element associated to field references. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFieldRef
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the value stored in a 'string' field.
		char *Value;
		//! It defines the name of the field this element points to (as an ascii string).

		char *FieldName;
		//! It defines the name of the protocol this element points to (as an ascii string).

		char *ProtoName;

		//! It defines the first byte the fieldref that has to be used.
		struct _nbNetPDLExprBase *OffsetStartAt;

		//! It defines the number of bytes that have to be used starting at previous offset.
		struct _nbNetPDLExprBase *OffsetSize;
	};


	//! Element associated to field references. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionRegExp
	{
		STRUCT_NBNETPDLOPERAND

		//! Buffer in which the regular expression search has to be performed.
		struct _nbNetPDLExprBase *SearchBuffer;

		//! It contains the regular expression that has to be used for searching.
		char *RegularExpression;

		//! Compiled regular expression according to the internal representation of the PCRE library
		void *PCRECompiledRegExp;

		//! 'true' if the match has to be done in a 'case sensitive' mode, 'false' otherwise.
		int CaseSensitiveMatch;

		//! Number of the match (complete = 0, 1st partial = 1, ...) that has to be returned to the called.
		//! This parameter is useful only for the extractstring() function.
		int MatchToBeReturned;
	};


	//! Element associated to the isasn1type() function. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionIsASN1Type
	{
		STRUCT_NBNETPDLOPERAND

		//! To be detailed
		struct _nbNetPDLExprBase *StringExpression;

		//! To be detailed
		struct _nbNetPDLExprBase *ClassNumber;

		//! To be detailed
		struct _nbNetPDLExprBase *TagNumber;
	};


	//! Element associated to the buf2int() function. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionBuf2Int
	{
		STRUCT_NBNETPDLOPERAND

		//! Buffer in which the regular expression search has to be performed.
		struct _nbNetPDLExprBase *StringExpression;
	};


	//! Element associated to the int2buf() function. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionInt2Buf
	{
		STRUCT_NBNETPDLOPERAND

		//! Buffer in which the regular expression search has to be performed.
		struct _nbNetPDLExprBase *NumericExpression;

		//! Number of valid bytes within the Result buffer.
		int ResultSize;

		//! Static structure that contains the result of the translation.
		unsigned char Result[sizeof(int)];
	};


	//! Element associated to the ispresent() function. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionIsPresent
	{
		STRUCT_NBNETPDLOPERAND

		//! Pointer to the field that has to be checked (if present).
		struct _nbNetPDLExprFieldRef *NetPDLField;
	};


	//! Element associated to the ascii2int() function. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionAscii2Int
	{
		STRUCT_NBNETPDLOPERAND

		//! Buffer that contains the ascii value that has to be transformed into an hex buffer.
		struct _nbNetPDLExprBase *AsciiStringExpression;
	};


	//! Element associated to the changebyteorder() function. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionChangeByteOrder
	{
		STRUCT_NBNETPDLOPERAND

		//! Buffer that contains the ascii value that has to be transformed into an hex buffer.
		struct _nbNetPDLExprBase *OriginalStringExpression;

		//! Temporary buffer used to keep the result of the computation.
		//! This buffer will be statically allocated by the NetPDL engine at initialization time.
		unsigned char ResultBuffer[sizeof(int64_t)];
	};


	//! Element that is used in struct _nbNetPDLExprFunctionCheckLookupTable for defining a variable list of parameters.
	struct _nbParamsLinkedList
	{
		//! Expression that returns the value of the current parameter.
		struct _nbNetPDLExprBase* Expression;

		//! Pointer to the next structure that contains the value of the next parameter.
		struct _nbParamsLinkedList* NextParameter;
	};


	//! Element associated to the checklookuptable() function. This element inherits from struct _nbNetPDLExprBase.
	struct _nbNetPDLExprFunctionCheckUpdateLookupTable
	{
		STRUCT_NBNETPDLOPERAND

		//! It defines the name of the lookup table (as an ascii string).
		char* TableName;

		//! It defines a custom data pointer that can be used by any NetPDL engine to put some data 
		//! for speeding up the access to the lookup table itself.
		void* LookupTableCustomData;

		//! List of the parameters of this function
		//! (please note that checklookuptable() has a variable number of parameters).
		struct _nbParamsLinkedList* ParameterList;
	};



#ifdef __cplusplus
}
#endif



