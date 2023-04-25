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


/*!
	\brief This function implements the custom visualization plugin for 
	printing IPv4/IPv6 addresses in literal form.

	This method gets an IP or IPv6 address and it translates it into a literal form (e.g.
	foo.bar.com).

	\param ShowPluginParams: structure (of type _ShowPluginParams) that contains the 
	parameters passed from the calling program.

	\return Standard values for a \ref nbShowPluginHandler function.
*/
int PrintIPv46LiteralNames(struct _ShowPluginParams *ShowPluginParams)
{
bool IsIPv4= true;
char HostName[NI_MAXHOST];			// NI_MAXHOST is currently defined as 1025
unsigned int Ret;
unsigned int FieldStartOffset;

	// Check if the user wants to translate addresses into hex form
	// If not, we have to return right now without any error code
	// (but with a warning, because we were not able to complete this process)
	ShowPluginParams->NetPDLVariables->GetVariableNumber(ShowPluginParams->NetPDLVariables->m_defaultVarList.ShowNetworkNames, &Ret);

	if (Ret == 0)
		return nbWARNING;

	// Checks if this is an IPv4 or IPv6 address
	if (ShowPluginParams->PDMLElement->Size == 4)
		IsIPv4= true;
	else
	{
		if (ShowPluginParams->PDMLElement->Size == 16)
			IsIPv4= false;
		else
			// Probably the field has been truncated, so we cannot perform our actions here
			return nbWARNING;
	}

	// Offset, within the packet dump, of the current field
	FieldStartOffset= ShowPluginParams->PDMLElement->Position;

	if (IsIPv4)
	{
	struct sockaddr_in SockAddr;

		memset(&SockAddr, 0, sizeof(SockAddr) );
		SockAddr.sin_family= AF_INET;
		memcpy(&SockAddr.sin_addr, &ShowPluginParams->Packet[FieldStartOffset], 4);	// IPv4 addresses are 4 bytes long

		Ret= sock_getascii_addrport((sockaddr_storage *) &SockAddr, HostName, NI_MAXHOST, NULL, 0, NI_NAMEREQD, NULL, 0);
	}
	else
	{
	struct sockaddr_in6 SockAddr;

		memset(&SockAddr, 0, sizeof(SockAddr) );
		SockAddr.sin6_family= AF_INET6;
		memcpy(&SockAddr.sin6_addr, &ShowPluginParams->Packet[FieldStartOffset], 16);	// IPv6 addresses are 16 bytes long

		Ret= sock_getascii_addrport((sockaddr_storage *) &SockAddr, HostName, NI_MAXHOST, NULL, 0, NI_NAMEREQD, NULL, 0);
	}

	// Set the 'show' attribute only if the previous function are working
	// Otherwise, let's stay on the standard value
	if (Ret == sockSUCCESS)
	{
		ShowPluginParams->PDMLElement->ShowValue= ShowPluginParams->AsciiBuffer->Store(HostName);
		if (ShowPluginParams->PDMLElement->ShowValue == NULL)
		{
			ssnprintf(ShowPluginParams->ErrBuf, ShowPluginParams->ErrBufSize, "%s", 
				ShowPluginParams->AsciiBuffer->GetLastError() );
			return nbFAILURE;
		}

		return nbSUCCESS;
	}

	// I was not able to convert the address into a name
	return nbWARNING;
}

