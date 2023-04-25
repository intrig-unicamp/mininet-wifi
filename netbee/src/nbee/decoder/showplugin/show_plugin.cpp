/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "show_plugin.h"

//! Array that contains the list of available visualization plugins, according
//! to the structure defined in struct \ref _ShowPluginList.
struct _ShowPluginList ShowPluginList[] =
{
	{"IP46Name",		&PrintIPv46LiteralNames},
	{"NetBIOSName",		&PrintNetBIOSNames},
	{"DomainName",		&PrintDNSDomainName},

	//! Please note that last member must be NULL (it is used to terminate the structure)
	{NULL,				NULL},
};

