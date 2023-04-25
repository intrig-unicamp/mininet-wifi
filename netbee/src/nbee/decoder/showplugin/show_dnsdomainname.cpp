/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#include <memory.h>		// for memcpy
#include <ctype.h>
#include <nbprotodb_defs.h>
#include <nbee_netpdlutils.h>
#include "show_plugin.h"
#include "../../globals/globals.h"
#include "../../globals/utils.h"


/*!
	\brief This function implements the custom visualization plugin for 
	printing DNS Domain Names.

	DNS Domain Names are rather complicated to handle because of the
	'compression' mechanism: if a name is already present in the DNS packet,
	the name refers to it (through a pointer) and this is not supported by
	standard NetPDL primitives.

	\param ShowPluginParams: structure (of type \ref _ShowPluginParams) that contains the 
	parameters passed from the calling program.

	\return Standard values for a \ref nbShowPluginHandler function.
*/
int PrintDNSDomainName(struct _ShowPluginParams *ShowPluginParams)
{
unsigned char *ResourceRecordPtr;
char DNSName[1024];	// Each single name cannot be larger than 63 bytes, so this should be enough
int DNSNameIndex;
bool IsFirstTime;

	ResourceRecordPtr= (unsigned char *) &(ShowPluginParams->Packet[ShowPluginParams->PDMLElement->Position]);
	IsFirstTime= true;
	DNSNameIndex= 0;
	DNSName[DNSNameIndex]= 0;

	// Note.
	// In this function I cannot use the ShowPluginParams->AsciiBuffer
	// to store data, because I do not have 'zero' terminated strings in the packet dump.
	// Therefore, we have to use the standard sstrncpy() (for instance, for the same
	// reason we cannot use the sstrncat() ).
	while (*ResourceRecordPtr)
	{
	int BytesToCopy;

		// Check if the first two bits are '0'. If so, we have to decode a standard string
		// and this value is the length of the string itself.
		if ((*ResourceRecordPtr & '\xC0') == 0)
		{
			if (IsFirstTime)
			{
				IsFirstTime= false;
			}
			else
			{
				DNSName[DNSNameIndex]= '.';
				DNSNameIndex++;
				DNSName[DNSNameIndex]= 0;
			}

			BytesToCopy= *ResourceRecordPtr & '\x3F';
			ResourceRecordPtr++;

			// Check that we're not going to exit form the current packet dump
			if ((ResourceRecordPtr + BytesToCopy) >= &ShowPluginParams->Packet[ShowPluginParams->CapturedLength])
				BytesToCopy= (int) ( &ShowPluginParams->Packet[ShowPluginParams->CapturedLength] - ResourceRecordPtr );

			memcpy(&DNSName[DNSNameIndex], ResourceRecordPtr, BytesToCopy);
			DNSNameIndex+= BytesToCopy;
			DNSName[DNSNameIndex]= 0;

			ResourceRecordPtr+= BytesToCopy;

			// In case the packet dump ended, let's return right now
			if ((ResourceRecordPtr) >= &ShowPluginParams->Packet[ShowPluginParams->CapturedLength])
				return nbSUCCESS;

			continue;
		}

		// If the first two bits are '11', then a pointer to a name follows
		if ((*ResourceRecordPtr & '\xC0') == (unsigned char) '\xC0')
		{
		int NewPosition;

			NewPosition= ShowPluginParams->PDMLElement->ParentProto->Position + (*ResourceRecordPtr & '\x3F') * 256 + *(ResourceRecordPtr+1);

			if (NewPosition >= ShowPluginParams->CapturedLength)
			{
				sstrncpy(DNSName, "(Truncated name)", sizeof(DNSName));
				break;
			}

			ResourceRecordPtr= (unsigned char *) &(ShowPluginParams->Packet[NewPosition]);

			continue;
		}
		// If we're here, it means that the DNS packet is not well formed, because it does not respect
		// the DNS packet format. In this case, we can set a wrong size (and we should raise an alert)
		sstrncpy(DNSName, "(Malformed DNS field)", sizeof(DNSName));
		return nbSUCCESS;
	}
	
	// In this case, we're the 'root' domain
	if (DNSName[0] == 0)
		sstrncpy(DNSName, "Root", sizeof(DNSName));

	ShowPluginParams->PDMLElement->ShowValue= ShowPluginParams->AsciiBuffer->Store(DNSName);
	if (ShowPluginParams->PDMLElement->ShowValue == NULL)
	{
		ssnprintf(ShowPluginParams->ErrBuf, ShowPluginParams->ErrBufSize, "%s", 
			ShowPluginParams->AsciiBuffer->GetLastError() );
		return nbFAILURE;
	}

	return nbSUCCESS;
}

