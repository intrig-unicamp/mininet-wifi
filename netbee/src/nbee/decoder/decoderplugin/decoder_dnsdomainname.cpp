/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "decoder_plugin.h"
#include "../../globals/globals.h"


/*!
	\brief This function implements the custom decoding plugin for 
	decoding DNS Domain Names.

	DNS Domain Names are rather complicated to handle because of the
	'compression' mechanism: if a name is already present in the DNS packet,
	the Domain Name refers to it (through a pointer) and this is not supported
	by standard NetPDL primitives.

	\param PluginParams: structure (of type \ref _PluginParams) that contains the 
	parameters passed from the calling program.

	\return Standard values for a \ref nbPluginHandler function.
*/
int DecodeDNSResourseRecord(struct _PluginParams *PluginParams)
{
unsigned char *ResourceRecordPtr;

	ResourceRecordPtr= (unsigned char *) &(PluginParams->Packet[PluginParams->FieldOffset]);

	while (*ResourceRecordPtr)
	{
	int BytesToCopy;

		// Check if the first two bits are '0'. If so, we have to decode a standard string
		// and this value is the length of the string itself.
		if ((*ResourceRecordPtr & '\xC0') == 0)
		{
			BytesToCopy= *ResourceRecordPtr & '\x3F';

			// '+1' becuse we have to take into account current byte (which contains the name length)
			ResourceRecordPtr+= (BytesToCopy + 1);

			// In case the packet dump ended, let's return right now
			if ((ResourceRecordPtr) >= &PluginParams->Packet[PluginParams->CapturedLength])
			{
				PluginParams->Size= PluginParams->CapturedLength - PluginParams->FieldOffset;
				return nbSUCCESS;
			}

			continue;
		}

		// If the first two bits are '11', then a pointer to a name follows
		if ((*ResourceRecordPtr & '\xC0') == (unsigned char) '\xC0')
		{
			// We have to take into account also of the byte that follows the current one, which
			//   defines the pointer within the packet; hence the '+1' at the end
			// This code does not have any 'overrun' problem, since the pointer iun the DNS packet 
			//   points always to some information that was present *before* in the packet.
			PluginParams->Size= (unsigned int) ((ResourceRecordPtr + 1) - &(PluginParams->Packet[PluginParams->FieldOffset]) + 1);

			return nbSUCCESS;
		}

		// If we're here, it means that the DNS packet is not well formed, because it does not respec
		// the DNS packet format. In this case, we can set a wrong size (and we should raise an alert)
		PluginParams->Size= PluginParams->CapturedLength - PluginParams->FieldOffset;
		return nbSUCCESS;
	}


	PluginParams->Size= (unsigned int) ((ResourceRecordPtr + 1) - &(PluginParams->Packet[PluginParams->FieldOffset]));

	return nbSUCCESS;
}

