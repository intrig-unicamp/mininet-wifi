/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


/*!
	\file decoder_plugin.h

	This file contains some common structures needed for implementing decoding plugins.
*/


#include <nbee_pxmlreader.h>
#include "../netpdlvariables.h"



/*!
	\defgroup Plugin Creating Custom Decoding Plugins

	This Section contains some hints for implementing new custom decoding plugins.
	In addition, the most important structures are shown.

	For implementing a new plugin, you have to follow these steps.

	<dl>
	<dt><b>1. Implement the plugin handler function in a new file</b></dt>
	<dd>
	The function should be implemented within a new file, which should (possibly) placed into the 
	<code>DecoderPlugin</code> folder (in the NetBee source tree)

	The prototype for the new function looks like the following:
	<pre>
	  int YourFunction(struct \ref _PluginParams *PluginParams);
	</pre>
	For implementing your function, you have to understand how the NetPDL decoding engine
	passes parameters to your function. For this, you have to read the documentation
	related to the function prototype (\ref nbPluginHandler) and the structure
	that contains all the elements (struct \ref _PluginParams) that are exchanged
	between th NetDPL decoding engine and your plugin function.

	Please note that some of the required parameters (e.g. the offset of the current protocol
	within the packet dump) are not contained directy in structure \ref _PluginParams;
	however, they can easily obtained by digging into some of its members. Please
	check at the documentation of this structure for further details.

	In order to increase compatibility, plugins are implemented through plain C code.
	</dd>

	<dt><b>2. Add this file to the NetBee project</b></dt>
	<dd></dd>

	<dt><b>3. Add the prototype of your function at the end of file <code>Plugin.h</code></b></dt>
	<dd></dd>

	<dt><b>4. Modify the \ref PluginList array.</b></dt>
	
	<dd>This array, implemented in file <code>Plugin.cpp</code>, must be modified by adding
	the information related to your function (i.e. plugin name, as it must be specified
	in the <code>&lt;plugin&gt;</code> element of the NetPDL file, and a pointer to the function itself).

	This array is used internally by the NetPDL decoding engine to locate the function
	related to the custom decoding plugin.

	Please note that \ref PluginList array MUST have the last item set to NULL, which
	is used to detect the end of the plugin list.
	</dd>
	</dl>


	\{
*/


/*!
	\brief Prototype for functions that implement decoding plugins.

	These functions may have three return values:
	- nbSUCCESS if the function succeedes
	- nbFAILURE if the funtcion fails. In this case, an error message is generated
	in the ErrBuf parameter of struct \ref _PluginParams.
	This error message will always be zero-terminated.
*/
typedef int (*nbPluginHandler)(struct _PluginParams *PluginParams);


/*!
	\brief Structure used by the NetPDL decoding engine to pass the required parameters
	to the decoding plugins.

	This structure contains the parameters that may be used by a custom plugin for doing
	its job. The programmer has to undestand very well these members in order to implement
	new decoding plugins.

	Please note that depending on the implementation of the plugin, some members
	(e.g. the access to the internal run-time variables) may not be required.

	\note Some required information are not included in this structure because they
	are already known in other ways. For instance, three very important information
	(e.g. the starting offset of current protocol and the previously decoded fields)
	are already known through the PDMLProt member.
	Particularly:

	- PDMLProto->Position: contains the offset (from the beginning
	of the packet) of this protocol. In other words, every field belonging to an IP 
	packet contained into an Ethernet frame have ProtoStartOffset equal to 14.

	- PDMLProto->FirstField: contains a pointer to the first field decoded within
	this protocol; through this field you can have access to all the fields that have 
	been decoded so far.
*/
typedef struct _PluginParams
{
	/*!
		\brief Pointer to the structure that contains PDML protocol under decoding.
		
		This element contains the data available related to the protocol that is
		currently under decoding. Some of the members of this structure may not be avaiable
		at this time; for instance, the total size of the protocol can be set only when
		all its fields have been decoded.
	*/
	struct _nbPDMLProto *PDMLProto;

	/*!
		\brief Number of bytes (referred to the current packet) captured.

		This value is very important because it gives the number of significant bytes
		within the packet data ('Packet' member). Since the packet can be truncated
		(e.g. in case only a snapshot of the packet is captured), the implementation 
		must be careful to access only valid data; in other words, any offset after 
		'CapturedLength' can bring to an invalid memory access.
	*/
	unsigned int CapturedLength;

	/*!
		\brief Offset (from the beginning of the packet) of this field.
	
		This variable defines the first byte that has to be examined within the
		packet dump that belongs to the current packet.

		For instance, the first field belonging to an IP packet contained
		into an Ethernet frame has CurrentOffset equal to 14.
	*/
	unsigned int FieldOffset;

	/*!
		\brief Pointer to the buffer that contains the packet data (in the pcap format).

		This is needed in order to have a complete access to the protocol data.
		This buffer contains the whole packet.
	*/
	const unsigned char *Packet;

	/*!
		\brief Pointer to the class which contains NetPDL variables.
		
		Run-time variables that must be shared with decoding plugins in case
		the plugin has to get access (or to modify) some of the variables
		used by the NetPDL parser.
	*/
	CNetPDLVariables *NetPDLVariables;

	/*!
		\brief Contains the actual size of this field.

		This member is used to return the actual size of the field back to the calling
		program. Therefore, the value of this field is meaningless when the function
		gets called; it is under the responsability of the plugin to fill in this
		member with the proper value (after decoding)
	*/
	unsigned int Size;

	/*!
		\brief Pointer to a user-allocated buffer (of length 'ErrBufSize') that will 
		keep an error message (if one).

		This buffer belongs to the calling class.
		Data stored in this buffer will always be NULL terminated.
	*/
	char *ErrBuf;

	//! Size of the previous buffer.
	int ErrBufSize;
} _PluginParams;



/*!
	\brief Structure that contains the list of the implemented decoding plugins.
*/
typedef struct _PluginList
{
	//! Name of the plugin, as it appears in the NetPDL &lt;plugin&gt; element.
	const char *PluginName;
	//! Pointer to the function that implements the plugin.
	nbPluginHandler PluginHandler;
} _PluginList;



/*!
	\}
*/



// Function list
int DecodeDNSResourseRecord(struct _PluginParams *PluginParams);

