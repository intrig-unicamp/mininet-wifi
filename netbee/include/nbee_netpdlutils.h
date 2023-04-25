/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*!
	\file nbee_netpdlutils.h

	Keeps the definitions and exported functions related to the module that adds some utilities of
	general use, which are often used especially coupled with the packet decoder module of the NetBee library.
*/


#ifdef NBEE_EXPORTS
	// We may have include header files of some other libraries, which do not have the right xxxx_EXPORTS
	// and then defined DLL_EXPORT as ""; So, let's define this in any case, and let's suppress the warning
	#pragma warning(disable: 4005)	
	#define DLL_EXPORT __declspec(dllexport)
#else
	#ifndef DLL_EXPORT		// to avoid duplicate definitions
	#define DLL_EXPORT
	#endif
#endif




/*!
	\defgroup NetBeeUtils NetBee NetPDL-related utilities
*/

/*! \addtogroup NetBeeUtils
	\{
*/


/*!
	\brief This class defines some methods that can be of general use for NetPDL-based tools.

	Please note that this is an abstract class; please use the nbAllocateNetPDLUtils() method in order to get
	an instance of this class. For cleanup, the programmer must use the nbDeallocateNetPDUtils() function.

*/
class DLL_EXPORT nbNetPDLUtils
{
public:
	nbNetPDLUtils() {};
	virtual ~nbNetPDLUtils() {};


/*!
	\brief It formats the value of a field according to the NetPDL description.

	Given the hex value of an header field and the NetPDL definition of the field itself,
	this method formats the value according to the NetPDL directives (e.g. an IP address becomes 
	'10.11.12.13'). This method is able to automatically locate the definition of the field within 
	the NetPDL description.

	Unfortunately, this method has some limitations and some elements may not be available in this 
	printing process (e.g. run-time variables, because we do not decode	the whole protocol headers here).
	This method generates only the content of the 'show' attribute of the PDML.

	This method does the opposite of GetHexValueNetPDLField().

	\param ProtoName: the name of the protocol in which the field is contained (e.g. 'ip').
	This name must be equal to the protocol name (attribute 'name') contained into the NetPDL description.

	\param FieldName: the name of the field that we want to format (e.g. 'hlen').
	This name must be equal to the field name (attribute 'name') contained into the NetPDL description.

	\param FieldDumpPtr: a pointer to the buffer that contains the dump (in the pcap format)
	of the current field (i.e. the field value in binary hex dump format).

	\param FieldSize: keeps the length (in bytes) of the decoded field.

	\param FormattedField: user-allocated buffer that will keep the result of the transformation.
	This buffer will be always '\\0' terminated.

	\param FormattedFieldSize: size of the previous user-allocated buffer.

	\return nbSUCCESS if the field has been found (therefore the 'FormattedField' is valid), nbFAILURE
	otherwise. In case of error, the error message can be retrieved by the GetLastError() method.

	\warning This method is able to format a given field with some limitations. Basically, it receives 
	an hex string containing the field value and it returns a buffer containing the formatted field.
	However, the input field comes alone, without the rest of the PDML fragment; therefore this class
	cannot make use of other information present in other parts of the packet (the a PDML fragment), 
	neither can make use of some global variable.
	This method does its best, but it cannot always provide an affordable result.
*/
	virtual int FormatNetPDLField(const char *ProtoName, const char *FieldName, const unsigned char *FieldDumpPtr,
		unsigned int FieldSize, char *FormattedField, int FormattedFieldSize)= 0;



/*!
	\brief It formats the value of a field according to the NetPDL description.

	Given the hex value of an header field and the NetPDL definition of the field itself,
	this method formats the value according to the NetPDL directives (e.g. an IP address becomes 
	'10.11.12.13'). This method is able to automatically locate the definition of the field within 
	the NetPDL description.

	Unfortunately, this method has some limitations and some elements may not be available in this 
	printing process (e.g. run-time variables, because we do not decode	the whole protocol headers here).
	This method generates only the content of the 'show' attribute of the PDML.

	This method does the opposite of GetHexValueNetPDLField().

	\param TemplateName: name of the NetPDL visualization template that has to be used for formatting
	the given field.

	\param FieldDumpPtr: a pointer to the buffer that contains the dump (in the pcap format)
	of the current field (i.e. the field value in binary hex dump format).

	\param FieldSize: keeps the length (in bytes) of the decoded field.

	\param FieldIsInNetworkByteOrder: 'true' if the field stores data in network byte order (the default),
	'false' otherwise. This parameter is needed because we're using the visualization template for formatting,
	and the information related to byte ordering is actually unknown since it is stored in the field definition.

	\param FormattedField: user-allocated buffer that will keep the result of the transformation.
	This buffer will be always '\\0' terminated.

	\param FormattedFieldSize: size of the previous user-allocated buffer.

	\return nbSUCCESS if the field has been found (therefore the 'FormattedField' is valid), nbFAILURE
	otherwise. In case of error, the error message can be retrieved by the GetLastError() method.

	\warning This method is able to format a given field with some limitations. Basically, it receives 
	an hex string containing the field value and it returns a buffer containing the formatted field.
	However, the input field comes alone, without the rest of the PDML fragment; therefore this class
	cannot make use of other information present in other parts of the packet (the a PDML fragment), 
	neither can make use of some global variable.
	This method does its best, but it cannot always provide an affordable result.
*/
	virtual int FormatNetPDLField(const char *TemplateName, const unsigned char *FieldDumpPtr,
		unsigned int FieldSize, int FieldIsInNetworkByteOrder, char *FormattedField, int FormattedFieldSize)= 0;


/*!
	\brief It formats the value of a field according to the NetPDL description.

	Given the hex value of an header field and the NetPDL definition of the field itself,
	this method formats the value according to the NetPDL directives (e.g. an IP address becomes 
	'10.11.12.13').
	
	This method operates only for fields that are associated to a fast printing function,
	which is one of the parameters. Please refer to the GetFastPrintingFunctionCode() method in 
	order to retrieve the code of the fast printing function referred to your field.

	This method does the opposite of GetHexValueNetPDLField().

	\param FastPrintingFunctionCode: internal code of the fast printing function associated
	to your field. Please refer to the GetFastPrintingFunctionCode() method in 
	order to retrieve that code.

	\param FieldDumpPtr: a pointer to the buffer that contains the dump (in the pcap format)
	of the current field (i.e. the field value in binary hex dump format).

	\param FieldSize: keeps the length (in bytes) of the decoded field.

	\param FormattedField: user-allocated buffer that will keep the result of the transformation.
	This buffer will be always '\\0' terminated.

	\param FormattedFieldSize: size of the previous user-allocated buffer.

	\return nbSUCCESS if the field has been found (therefore the 'FormattedField' is valid), nbFAILURE
	otherwise (e.g. the FastPrintingFunctionCode is invalid).In case of error, the error message can be retrieved by the GetLastError() method.

	\warning This method is able to format a given field with some limitations. Basically, it receives 
	an hex string containing the field value and it returns a buffer containing the formatted field.
	However, the input field comes alone, without the rest of the PDML fragment; therefore this class
	cannot make use of other information present in other parts of the packet (the a PDML fragment), 
	neither can make use of some global variable.
	This method does its best, but it cannot always provide an affordable result.
*/
	virtual int FormatNetPDLField(int FastPrintingFunctionCode, const unsigned char *FieldDumpPtr,
		unsigned int FieldSize, char *FormattedField, int FormattedFieldSize)= 0;


/*!
	\brief Checks if the given field is associated to a fast printing function.

	This function checks if the give field is associated to a fast printing function and it returns
	the code that can be used to refer to this function. This code can be used in the proper 
	FormatNetPDLField() in order to transform the hex dump of the field into a printable format.

	\param ProtoName Name of the protocol the field belongs to.

	\param FieldName Name of the requested field.

	\param FastPrintingFunctionCode When the function returns, this variable will contain
	a code (valid internally to the library) that can be used to call the formatting funcion
	directly, through the proper FormatNetPDLField() function.

	\return This function returns nbSUCCESS if the requested field is associated to a proper
	fast printing function, nbWARNING in case the field is not, and nbFAILURE in case of errors.
	In case of error, the error message can be retrieved by the GetLastError() method.
*/
	virtual int GetFastPrintingFunctionCode(const char *ProtoName, const char *FieldName, int *FastPrintingFunctionCode)= 0;


/*!
	\brief It gets a formatted value of a given field and it returns its hexadecimal counterpart.

	Given the formatted value of a field, it returns its value in hex depending on the value of
	the 'BinaryEncoding' variable. 

	The 'BinaryEncoding' variable determines if the resulting hex string is formatted in binary
	(therefore a 16-bit value will be encoded using 2 bytes of the FieldHexValue buffer) or
	4 bytes. The second case is useful in case we want to print the hex dump of the field, therefore
	we need something formatted in ascii.

	For example, in case 'BinaryEncoding' is false, an IP address in its dotted decimal form 
	(e.g. 10.1.1.1) will returnd the string '0A010101', whose length (the value of variable 
	'FieldHexSize') is 8 bytes.	In case a binary packet dump (i.e. pcap format) is required
	(i.e. 'BinaryEncoding' is 'true'), the length of the returned binary buffer will be only
	4 bytes (according to the example before).

	The input value (i.e. the string 'FormattedField') must be compiant with the description of that field
	within the NetPDL description file.

	This method does the opposite of FormatNetPDLField().

	Unfortunately, this method has some limitations and some elements may not be available in this 
	printing process (e.g. run-time variables, because we do not decode	the whole protocol headers here).
	This method generates only the content of the 'show' attribute of the PDML.

	\param ProtoName: the name of the protocol in which the field is contained (e.g. 'ip').
	This name must be equal to the protocol name (attribute 'name') contained into the NetPDL description.

	\param FieldName: the name of the field that we want to format (e.g. 'hlen').
	This name must be equal to the field name (attribute 'name') contained into the NetPDL description.

	\param FormattedField: value of the field as an ascii string (e.g. '10.1.1.1' for an IP address).

	\param FieldHexValue: a pointer to the buffer that will contain the dump (in pcap format)
	of the current field (i.e. the field value in binary hex dump format).

	\param FieldHexSize: keeps the length (in bytes) of the buffer that will contain the FieldHexValue.
	When the function returns, it will keep the number of bytes that have currently been used in that buffer.

	\param BinaryEncoding: 'true' if we want to return a binary buffer, 'false' if we want to return an ascii
	buffer. The size (i.e. the value of variable 'FieldHexSize') of the same data returned as ascii buffer 
	has a size which is two times the size of the same data returned as binary buffer.

	\return nbSUCCESS if the field has been found (therefore the 'FormattedField' is valid), nbFAILURE
	otherwise. In case of error, the error message can be retrieved by the GetLastError() method.

	\warning This method is able to operate on fields that have 'standard' visualization primitives. In other words, 
	it is not able to convert fields that use 'showplg' or such directives in the NetPDL protocol definition file.
	Basically, this problem originates from the same limits of the FormatNetPDLField() function.

	\warning Please note that the the field value (i.e. the content of variable 'FieldHexValue')
	must be typed in the expected format according to the NetPDL database. For instance, if the NetPDL
	database tells you that the field must be printed as a hex number, the content of the 'FieldHexValue'
	variable must be an hexadecimal string.
*/
	virtual int GetHexValueNetPDLField(const char *ProtoName, const char *FieldName, const char *FormattedField, 
		unsigned char *FieldHexValue, unsigned int *FieldHexSize, bool BinaryEncoding)= 0;


/*!
	\brief It gets a formatted value of a given field and it returns its hexadecimal counterpart.

	Given the formatted value of a field, it returns its value in hex depending on the value of
	the 'BinaryEncoding' variable. 

	The 'BinaryEncoding' variable determines if the resulting hex string is formatted in binary
	(therefore a 16-bit value will be encoded using 2 bytes of the FieldHexValue buffer) or
	4 bytes. The second case is useful in case we want to print the hex dump of the field, therefore
	we need something formatted in ascii.

	For example, in case 'BinaryEncoding' is false, an IP address in its dotted decimal form 
	(e.g. 10.1.1.1) will returnd the string '0A010101', whose length (the value of variable 
	'FieldHexSize') is 8 bytes.	In case a binary packet dump (i.e. pcap format) is required
	(i.e. 'BinaryEncoding' is 'true'), the length of the returned binary buffer will be only
	4 bytes (according to the example before).

	The input value (i.e. the string 'FormattedField') must be compiant with the description of that field
	within the NetPDL description file.

	This method does the opposite of FormatNetPDLField().

	Unfortunately, this method has some limitations and some elements may not be available in this 
	printing process (e.g. run-time variables, because we do not decode	the whole protocol headers here).
	This method generates only the content of the 'show' attribute of the PDML.

	\param TemplateName: name of the NetPDL visualization template that has to be used for formatting
	the given field.

	\param FormattedField: value of the field as an ascii string (e.g. '10.1.1.1' for an IP address).

	\param FieldHexValue: a pointer to the buffer that will contain the dump (in pcap format)
	of the current field (i.e. the field value in binary hex dump format).

	\param FieldHexSize: keeps the length (in bytes) of the buffer that will contain the FieldHexValue.
	When the function returns, it will keep the number of bytes that have currently been used in that buffer.

	\param BinaryEncoding: 'true' if we want to return a binary buffer, 'false' if we want to return an ascii
	buffer. The size (i.e. the value of variable 'FieldHexSize') of the same data returned as ascii buffer 
	has a size which is two times the size of the same data returned as binary buffer.

	\return nbSUCCESS if the field has been found (therefore the 'FormattedField' is valid), nbFAILURE
	otherwise. In case of error, the error message can be retrieved by the GetLastError() method.

	\warning This method is able to operate on fields that have 'standard' visualization primitives. In other words, 
	it is not able to convert fields that use 'showplg' or such directives in the NetPDL protocol definition file.
	Basically, this problem originates from the same limits of the FormatNetPDLField() function.

	\warning Please note that the the field value (i.e. the content of variable 'FieldHexValue')
	must be typed in the expected format according to the NetPDL database. For instance, if the NetPDL
	database tells you that the field must be printed as a hex number, the content of the 'FieldHexValue'
	variable must be an hexadecimal string.
*/
	virtual int GetHexValueNetPDLField(const char *TemplateName, const char *FormattedField, 
		unsigned char *FieldHexValue, unsigned int *FieldHexSize, bool BinaryEncoding)= 0;



/*!
	\brief It translates a single hex char (in the range '0' - 'F') into its
	decimal value.

	\param HexChar: an ascii character that contains the Hex digit to be converted

	\return The decimal value (e.g. '10' if HexChar is equal to 'A' or 'a') of the given
	character.
*/
	static char HexCharToDec(char HexChar);


/*!
	\brief Converts an hex dump in ascii chars into a binary hex dump.

	This method converts an ascii dump (e.g. "AD810A") into the corresponding binary
	dump (the same, but "AD" will be a single 8bit char).

	\param HexDumpAscii: a string that contains the hex dump in ascii format.
	
	\param HexDumpBin: a buffer that will contain the binary dump (according 
	to the pcap format) when the function ends.
	This buffer must be allocated by the user.

	\param HexDumpBinSize: size of the previous buffer.

	\return The number of bytes currently written into the HexDumpBin buffer, or
	nbFAILURE in case the target buffer is not large enough.
*/
	static int HexDumpAsciiToHexDumpBin(char *HexDumpAscii, unsigned char *HexDumpBin, int HexDumpBinSize);


/*!
	\brief Converts a binary hex dump into a hex dump made up of ascii chars.

	This method converts a binary dump (e.g. 'AD810A') into the corresponding ascii
	dump (the same, but the previous string will use 6 bytes instead of three).

	\param HexDumpBin: a buffer that contains the hex dump in binary format.
	
	\param HexDumpBinSize: the number of valid bytes in the HexDumpBin buffer. This parameter
	is needed because we can no longer rely on the string terminator (the NULL character)
	which is used to terminate ascii strings.

	\param HexDumpIsInNetworkByteOrder: 'true' if the hex dump is in network byte order (the
	default case), 'false' otherwise. For instance, this parameter is used to convert the hex dump
	related to fields that are not in network byte order, such as some fields in Bluetooth protocol.

	\param HexDumpAscii: a string that will contain the ascii dump when the function ends,
	This string must be allocated by the user.

	\param HexDumpAsciiSize: size of the previous string. Be carefully that the HexDumpAscii string
	will be 'zero' terminated; therefore the maximum number of valid characters that are allowed
	into the ascii string are (HexDumpAsciiSize - 1).

	\return The number of bytes currently written into the HexDumpAscii buffer, or
	nbFAILURE in case the target buffer is not large enough.
*/
	static int HexDumpBinToHexDumpAscii(char *HexDumpBin, int HexDumpBinSize, int HexDumpIsInNetworkByteOrder, char *HexDumpAscii, int HexDumpAsciiSize);


/*! 
	\brief Return a string keeping the last error message that occurred within the current instance of the class

	\return A buffer that keeps the last error message.
	This buffer will always be NULL terminated.
*/
	char *GetLastError() { return m_errbuf; }

protected:
	//! Buffer that keeps the error message (if any)
	char m_errbuf[2048];
};



/*!
	\brief It creates a new instance of a nbNetPDLUtils class and it returns it to the caller.

	This function is needed in order to create the proper object derived from nbNetPDLUtils.
	For instance, nbNetPDLUtils is a virtual class and it cannot be instantiated.
	The returned object has the same interface of nbNetPDLUtils, but it is a real object.

	nbNetPDLUtils is a class that contains some utilities that can be of general use for a NetPDL-based application.

	\param ErrBuf: user-allocated buffer (of length 'ErrBufSize') that will keep an error message (if one).
	This buffer will always be NULL terminated.

	\param ErrBufSize: size of the previous buffer.

	\return A pointer to a nbNetPDLUtils object, or NULL if something was wrong.
	In that case, the error message will be returned in the ErrBuf buffer.

	\warning Be carefully that the returned object must be deallocated through the nbDeallocateNetPDLUtils().
*/
DLL_EXPORT nbNetPDLUtils* nbAllocateNetPDLUtils(char *ErrBuf, int ErrBufSize);


/*!
	\brief It deallocates the instance of nbNetPDLUtils created through the nbAllocateNetPDLUtils().

	\param NetPDLUtils: pointer to the object that has to be deallocated.
*/
DLL_EXPORT void nbDeallocateNetPDLUtils(nbNetPDLUtils* NetPDLUtils);


/*!
	\}
*/

