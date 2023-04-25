/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


/*!
	\file show_plugin.h

	This file contains some common structures needed for implementing visualization plugins.
*/



#include <nbee_pxmlreader.h>
#include "../netpdlvariables.h"
#include "../../utils/asciibuffer.h"


/*!
	\defgroup ShowPlugin Creating Custom Visualization Plugins

	This Section contains some hints for implementing new custom visualization plugins.
	In addition, the most important structures are shown.

	For implementing a new plugin, you have to follow these steps.

	<dl>
	<dt><b>1. Implement the plugin handler function in a new file</b></dt>
	<dd>
	The function should be implemented within a new file, which should (possibly) placed into the 
	<code>ShowPlugin</code> folder (in the NetBee source tree)

	The prototype for the new function looks like the following:
	<pre>
	  int YourFunction(struct \ref _ShowPluginParams *ShowPluginParams);
	</pre>
	For implementing your function, you have to understand how the NetPDL decoding engine
	passes parameters to your function. For this, you have to read the documentation
	related to the function prototype (\ref nbShowPluginHandler) and the structure
	that contains all the elements (struct \ref _ShowPluginParams) that are exchanged
	between th NetDPL decoding engine and your plugin function.

	Please note that some of the required parameters (e.g. the field offset within
	the packet dump) are not contained directy in structure \ref _ShowPluginParams;
	however, they can easily obtained by digging into some of its members. Please
	check at the documentation of this structure for further details.

	In order to increase compatibility, plugins are implemented through plain C code.
	</dd>

	<dt><b>2. Add this file to the NetBee project</b></dt>
	<dd></dd>

	<dt><b>3. Add the prototype of your function at the end of file <code>ShowPlugin.h</code></b></dt>
	<dd></dd>

	<dt><b>4. Modify the \ref ShowPluginList array.</b></dt>
	
	<dd>This array, implemented in file <code>ShowPlugin.cpp</code>, must be modified by adding
	the information related to your function (i.e. plugin name, as it must be specified
	in the <code>showplg</code> attribute of the NetPDL file, and a pointer to the function itself).

	This array is used internally by the NetPDL decoding engine to locate the function
	related to the custom visualization plugin.

	Please note that \ref ShowPluginList array MUST have the last item set to NULL, which
	is used to detect the end of the plugin list.
	</dd>
	</dl>


	\{
*/



/*!
	\brief Prototype for functions that implement visualization plugins.

	These functions may have three return values:
	- nbSUCCESS if the function succeedes
	- nbWARNING if the function fails with non-critical errors and the program can
	go on with standard processing (e.g. using the definitions specified in the
	showtype, showsep and showgrp attributes of the template)
	- nbFAILURE if the funtcion fails. In this case, an error message is generated
	in the ErrBuf parameter of struct \ref _ShowPluginParams.
	This error message will always be zero-terminated.
*/
typedef int (*nbShowPluginHandler)(struct _ShowPluginParams *ShowPluginParams);


/*!
	\brief Structure used by the NetPDL decoding engine to pass the required parameters
	to the visualization plugins.

	This structure contains the parameters that may be used by a custom plugin for doing
	its job. The programmer has to undestand very well these members in order to implement
	new visualization plugins.

	Please note that depending on the implementation of the plugin, some members
	(e.g. the access to the internal run-time variables) may not be required.

	\note Some required information are not included in this structure because they
	are already known in other ways. For instance, three very important information
	(which are the starting offset of current protocol, the starting offset of current
	field, and the field size) are already known through the PDMLElement member.
	Particularly:

	- PDMLElement->ParentProto->Position: contains the offset (from the beginning
	of the packet) of this protocol. In other words, every field belonging to an IP 
	packet contained into an Ethernet frame have this member equal to 14.

	- PDMLElement->Position: contains the offset (from the beginning of the packet)
	of this field. In other words, the first field belonging to an IP packet contained
	into an Ethernet frame has this member equal to 14.

	- PDMLElement->Size: contains the size of the current field, taking into account
	also of truncation problems. In other words, if the field size should be 16 (e.g.
	an IPv6 address) but the packet has been truncated to the 12th byte, this member
	will keep value '12'.
*/
typedef struct _ShowPluginParams
{
	/*!
		\brief Pointer to the structure that contains PDML element that has to be modified.
		
		Please note that this element is the last element appended to the PDML fragment.
		Therefore, it is possible to access to all the other PDML elements previously 
		created by means of the appropriate links.
		
		Note that the PDMLElement has already been filled with all the other information 
		but the 'show' attribute.
		Therefore, it is possible to access to all the other attributes, which are 
		already been set by the calling method.
	*/
	_nbPDMLField *PDMLElement;

	/*!
		\brief Number of bytes (referred to the current packet) captured.

		This value is very important because it gives the number of significant bytes
		within the packet data ('Packet' member). Since the packet can be truncated
		(e.g. in case only a snapshot of the packet is captured), the implementation 
		must be careful to access only valid data; in other words, any offset after 
		'CapturedLength' can bring to an invalid memory access.
	*/
	int CapturedLength;

	/*!
		\brief Pointer to the buffer that contains the packet data (in the pcap format).

		This is needed in order to have a complete access to the protocol data.
		This buffer contains the whole packet.
	*/
	const unsigned char *Packet;

	/*!
		\brief Pointer to the class which contains NetPDL variables.
		
		Run-time variables that must be shared with visualization plugins in case
		the plugin has to get access to some variable modified by the NetPDL parser.
	*/
	CNetPDLVariables *NetPDLVariables;

	/*!
		\brief Pointer to the CAsciiBuffer class.
		
		Most of the field values are stored in as an ascii string pointed by the PDMLElement.
		However, in order to avoid allocating / deallocating memory for each element, the
		CAsciiBuffer class is defined, which provides a way to store some temp ASCII data.

		By calling this class, you are able to store some temp ASCII data in a global buffer
		and get a pointer to the stored data. This means that there is no need to deallocate
		such this memory, because it will be deallocated automatically as soon as the
		CAsciiBuffer object will got deleted.

		Please check at some implemented plugins in order to see how to use this class.
	*/
	CAsciiBuffer *AsciiBuffer;

	/*!
		\brief Pointer to a user-allocated buffer (of length 'ErrBufSize') that will 
		keep an error message (if one).

		This buffer belongs to the calling class.
		Data stored in this buffer will always be NULL terminated.
	*/
	char *ErrBuf;

	//! Size of the previous buffer.
	int ErrBufSize;
} _ShowPluginParams;


/*!
	\brief Structure that contains the list of the implemented visualization plugins.
*/
typedef struct _ShowPluginList
{
	//! Name of the plugin, as it appears in the NetPDL 'showplg' attribute.
	const char *PluginName;
	//! Pointer to the function that implements the plugin.
	nbShowPluginHandler ShowPluginHandler;
} _ShowPluginList;



/*!
	\}
*/


// Function list
int PrintIPv46LiteralNames(struct _ShowPluginParams *ShowPluginParams);
int PrintNetBIOSNames(struct _ShowPluginParams *ShowPluginParams);
int PrintDNSDomainName(struct _ShowPluginParams *ShowPluginParams);


// Native function lists
int PrintIPv4Address(struct _ShowPluginParams *ShowPluginParams);
int PrintAsciiField(struct _ShowPluginParams *ShowPluginParams);
