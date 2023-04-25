/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include <nbsockutils.h>
#include <nbprotodb_defs.h>
#include <nbee_netpdlutils.h>
#include "show_plugin.h"
#include "../../globals/globals.h"
#include "../../globals/utils.h"
#include "../../globals/debug.h"


#include <ctype.h>



/*!
	\brief This function implements the custom visualization plugin for 
	printing NetBIOS names.

	This method gets a NetBIOS name and it prints it (NetBIOS names are stored in an
	"encrypted" way).

	\param ShowPluginParams: structure (of type _ShowPluginParams) that contains the 
	parameters passed from the calling program.

	\return Standard values for a \ref nbShowPluginHandler function.
*/
int PrintNetBIOSNames(struct _ShowPluginParams *ShowPluginParams)
{
char HostName[NETPDL_MAX_STRING + 1];
const unsigned char *HexDumpBin;
int i= 0;

	// Let's point to the first byte of this field in the packet dump
	HexDumpBin= &(ShowPluginParams->Packet[ShowPluginParams->PDMLElement->Position]);

	// Reset Hostname
	memset(HostName, 0, NETPDL_MAX_STRING + 1);

	while (*HexDumpBin)
	{
		// Check if the first two bits are '0'. If so, we have to decode a standard string
		// and this value is the length of the string itself.
		if ((*HexDumpBin & '\xC0') == 0)
		{
			HexDumpBin++;

			// Let's translate the binary dump into a netbios name, using the 
			// strange encoding algorithm defined in NetBIOS
			while (HexDumpBin[2*i])
			{
			int Value;

				Value= HexDumpBin[2*i] - 'A';
				HostName[i]= Value << 4;

				Value= HexDumpBin[2*i+1] - 'A';
				HostName[i]+= Value;

				// Checks if the resulting char is a printable one; if not (and it is neither
				// a space nor a string terminator), it replaces that char with a space.
				// This is because sometimes the last char of the string is a strange one
				// (no idea why; probably due to some strange version of NetBIOS)
				if ( (!isgraph( (unsigned char) HostName[i])) && (HostName[i] != ' ') && (HostName[i] != '\0') )
					HostName[i]= 0;

				i++;
			}
			break;
		}

		// If the first two bits are '11', then a pointer to a name follows
		if ((*HexDumpBin & '\xC0') == (unsigned char) '\xC0')
		{
		int NewPosition;

			NewPosition= ShowPluginParams->PDMLElement->ParentProto->Position + (*HexDumpBin & '\x3F') * 256 + *(HexDumpBin+1);

			if (NewPosition >= ShowPluginParams->CapturedLength)
			{
				sstrncpy(HostName, "(Truncated name)", sizeof(HostName));
				break;
			}

			HexDumpBin= (unsigned char *) &(ShowPluginParams->Packet[NewPosition]);

			continue;
		}

		// If we're here, it means that the DNS packet is not well formed, because it does not respect
		// the NetBIOS packet format. In this case, we can set a wrong size (and we should raise an alert)
		sstrncpy(HostName, "(Malformed NetBIOS field)", sizeof(HostName));
		return nbSUCCESS;
	}

	if (HostName[0] == 0)
	{
		sstrncpy(HostName, "(Error in decoding the NetBIOS hostname)", sizeof(HostName));
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, NULL, 0, "(Error in decoding the NetBIOS hostname)");
	}

	ShowPluginParams->PDMLElement->ShowValue= ShowPluginParams->AsciiBuffer->Store(HostName);
	if (ShowPluginParams->PDMLElement->ShowValue == NULL)
	{
		ssnprintf(ShowPluginParams->ErrBuf, ShowPluginParams->ErrBufSize, "%s", ShowPluginParams->AsciiBuffer->GetLastError() );
		return nbFAILURE;
	}

	return nbSUCCESS;
}
