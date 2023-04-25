/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "decoder_plugin.h"


//! Array that contains the list of available decoding plugins, according
//! to the structure defined in struct \ref _PluginList.
struct _PluginList PluginList[] =
{
		{"DomainName",		&DecodeDNSResourseRecord},

	//! Please note that last member must be NULL (it is used to terminate the structure)
	{0,				0},
};

