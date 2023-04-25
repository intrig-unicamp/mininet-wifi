/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _OCTEON_COPROH
#define _OCTEON_COPROH
#include "coprocessor.h"
#include "nbnetvm.h"

namespace jit {
namespace octeon {

	typedef struct _copro_map_entry {
		char copro_name[MAX_COPRO_NAME];
		nvmCoproInitFunct *copro_init;
	} copro_map_entry;

	extern copro_map_entry octeon_copro_map[AVAILABLE_COPROS_NO];
}
}
#endif

