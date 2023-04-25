/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x11-util.h"
using namespace std;

namespace jit {
namespace X11 {

map<RegisterInstance, operand_size_t> *sizes;

operand_size_t size(MIRNode::RegType r)
{
	jit::operand_size_t s(r.getProperty<jit::operand_size_t>("size"));

	if(!s) {
		s = (*sizes)[r];
	}
		
	return s;
}


} /* namespace X11 */
} /* namespace jit */
