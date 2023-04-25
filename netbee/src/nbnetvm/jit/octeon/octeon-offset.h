/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef OCTEON_OFFSET_H
#define OCTEON_OFFSET_H

#include "offsets.h"

namespace jit {
	namespace octeon {

		class octeonOffset : public nvmStructOffsets<intptr_t>
		{
			public:
				struct octeonCoprocessorMapEntryOffsets
				{
					AddrType create;
					AddrType size;
				};

				struct octeonCoprocessorMapEntryOffsets octeon_copro_map_entry;

				void init();
		};

		extern octeonOffset offsets;
	}//namespace octeon
}//namespace jit

#endif
