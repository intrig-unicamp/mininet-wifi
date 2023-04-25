/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __X11_UTIL_H__
#define __X11_UTIL_H__

#include <string>
#include <sstream>
#include <map>
#include "registers.h"
#include "mirnode.h"

namespace jit {
namespace X11 {

extern std::map<RegisterInstance, operand_size_t> *sizes;

operand_size_t size(MIRNode::RegType r);

} /* namespace X11 */
} /* namespace jit */

inline std::string make_label(std::string s)
{
	if(s.compare("end") != 0)
		s = "L" + s;
	return s;
}

template<typename T>
inline std::string stringify(T t)
{
	std::ostringstream os;
	os << t;
	os.flush();
	return os.str();
}

template<>
inline std::string stringify<const char *>(const char *t)
{
	std::string s(t);
	return s;
}

namespace jit {
namespace X11 {

typedef enum {
	r_none,
	r_low,
	r_high,
	r_16lo,
	r_16hi,
	r_result_0,
	r_result_1,
} reg_part_t;

typedef enum {
	sp_const = 128,
	sp_virtual,
	sp_sw_help,			/* part of the switch helper representing a register */
	sp_sw_table,		/* part of the switch helper holding the table number */
	sp_lk_table,
	sp_offset,
	sp_pkt_mem,			/* packet memory addressed with the packet offset */
	sp_reg_mem,			/* register file addressed with the register offset */
	sp_flags,			/* processor flags */
	sp_device,			/* device registers */
	sp_lk_result,		/* hold a pair of values of the same size */
	sp_lk_key64,		/* 64-bit lookup key */
} reg_space_t;

typedef enum {
	off_pkt,
	off_reg,
} offset_reg_t;

inline std::string printReg(const RegisterInstance &ri)
{
	std::string s;
	std::ostringstream os;

	switch(ri.get_model()->get_space()) {
		case sp_const:
			os << ri.get_model()->get_name();
			os.flush();
			s = os.str();
			break;
		
		case sp_offset:
			switch(ri.get_model()->get_name()) {
				case off_pkt:
					s = "po";
					break;
				case off_reg:
					s = "ro";
					break;
				default:
					s = "??";
			}

			if(ri.version())
				s += "." + stringify(ri.version());

			break;
		
		case sp_pkt_mem:
			s = "pkt[po]";
			break;
			
		case sp_reg_mem:
			s = "rf[ro]";
			break;
			
		case sp_device:
			os << "dr" << ri.get_model()->get_name();
			os.flush();
			s = os.str();
			break;
		
		default:
			os << ri;
			os.flush();
			s = os.str();
	}

	return s;
}

inline uint32_t REG_SPACE(reg_space_t rs)
{
	return (uint32_t)(rs);
}

inline uint32_t OFF_SPACE(offset_reg_t rs)
{
	return (uint32_t)(rs);
}

} /* namespace X11 */
} /* namespace jit */

#endif /* __X11_UTIL_H__ */
