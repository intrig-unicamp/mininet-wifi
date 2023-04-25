/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef _SWITCH_LOWERING
#define _SWITCH_LOWERING

#include <nbnetvm.h>
#include <vector>
#include <map>
#include "mirnode.h"

namespace jit {

	struct Window;
	/*!
	 * \brief interface used by the switchemitter to emit the switch lowered
	 * \todo rename the methods since the are a bit obscure
	 */
	class ISwitchHelper
	{
		public:
			/*!
			 * \brief emit final jump when there is only one case left
			 * \param case_value value of this case in the switch
			 * \param jt true target
			 * \param jf false target
			 */
			virtual void emit_jcmp_eq(uint32_t case_value, uint32_t jt, uint32_t jf) = 0;

			/*!
			 * \brief emit a jump if less than (used by binary tree lowering method)
			 * \param case_value value of this case in the switch
			 * \param jt true target
			 * \param jf false target
			 */
			virtual void emit_jcmp_l(uint32_t case_value, uint32_t jt, uint32_t jf) = 0;

			/*!
			 * \brief emit a jump if greater than (used by binary tree lowering method)
			 * \param case_value value of this case in the switch
			 * \param jt true target
			 * \param jf false target
			 */
			virtual void emit_jcmp_g(uint32_t case_value, uint32_t jt, uint32_t jf) = 0;

			/*!
			 * \brief mask the register of the switch and emit jump into the following jump table
			 * \param w Window of this jump
			 */
			virtual void emit_jmp_to_table_entry(Window& w) = 0;

			/*!
			 * \brief subtrac the minimum and emit indirect jump in following jump table (MRST and Jump table)
			 * \param min minimum value between the switch cases
			 */
			virtual void emit_jmp_to_table_entry(uint32_t min) = 0;

			/*!
			 * \brief emit the the starting of the code of an entry in the table (binary tree)
			 * \param name name of this node in the jumps tree
			 */
			virtual void emit_start_table_entry_code(uint32_t name) = 0;

			/*!
			 * \brief emit a table entry if deftarget emit jump to default statement (MRST)
			 * \param name of this node in the jumps tree
			 * \param defTarget true if this is a jump to the default target
			 */
			virtual void emit_table_entry(uint32_t name, bool defTarget = false) = 0;

			/*!
			 * \brief emit a table entry with address of the bb target (jump table)
			 * \param target id of the destination basic block
			 */
			virtual void emit_jump_table_entry(uint32_t target) = 0;

			/*!
			 * \brief emit a comparison with a value in a binary search tree
			 * \param case_value value to compare
			 * \param my_name name of this node
			 * \param target_name name of the destination node when the comparison returns true
			 */
			virtual void emit_binary_jump(uint32_t case_value, std::string& my_name, std::string& target_name) = 0;
			/*!
			 * \brief emit an end comparison with a value in a binary search tree
			 * \param case_value value to compare
			 * \param jt id of the true destination bb 
			 * \param jf id of the false destination bb 
			 * \param my_name name of this node
			 */
			virtual void emit_binary_end_jump(uint32_t case_value, uint32_t jt, uint32_t jf, std::string& my_name) = 0;

			//!destructor
			virtual ~ISwitchHelper() {}
	};

	//!class that every backend whishing to use the SwitchEmitter should implement
	template<typename _IR>
		class SwitchHelper : public ISwitchHelper
	{
		public:

			typedef _IR IR; //!< type of the emitted instructions

			//!constructor
			SwitchHelper(BasicBlock<IR>& bb, SwitchMIRNode& insn): bb(bb), insn(insn) {}; 

		protected:
			BasicBlock<IR>& bb;  //!<basic block in which emit the instruction
			SwitchMIRNode& insn; //!<switch node to emit
	};

	//!class implementing MRST algorithm to lower a switch statement
	class SwitchEmitter 
	{
		public:

			//!constructor
			SwitchEmitter(ISwitchHelper& helper, SwitchMIRNode& insn);
			//!destructor
			virtual ~SwitchEmitter();

			/*!
			 * \brief main function of the algorithm
			 *
			 * backends can override this method to implement their euristics for selecting 
			 * switch lowering methods
			 */
			virtual void run();

			typedef std::pair< uint32_t, uint32_t> case_pair;
			typedef std::vector< case_pair > cases_set;
			typedef cases_set::const_iterator cases_set_iterator_t;

		protected:

			ISwitchHelper& helper; //!<helper used by the algorithm
			SwitchMIRNode& insn;   //!<instruction to emit

			//!launch the algorithm MRST on set P
			void MRST(cases_set& P);
			//!launch the algorithm of binary seach tree on set c
			void binary_switch(cases_set& c);
			//!emit a jump table for the set c
			void emit_jump_table(cases_set& c);

			struct CaseCompLess
			{
				bool operator()(const case_pair &x, const case_pair &y)
				{
					if (x.first >= y.first)
						return false;
					if (x.second >= y.second)
						return false;
					return true;
				}
			};

		private:
			//!find the most crititcal window in the set c
			Window find_critical(cases_set& c);
			//!recursive routine used for lowering as a binary search tree
			void emit_binary_node_branch(cases_set& c, uint32_t begin, uint32_t end);

			static std::string make_name(uint32_t from, uint32_t to);

			uint32_t name;//!<variable used to keep track of recursive methods
	};

	//!structure representing a window of bit in the cases
	struct Window
	{
		typedef SwitchEmitter::case_pair case_pair;
		typedef SwitchEmitter::cases_set cases_set;
		typedef SwitchEmitter::cases_set_iterator_t cases_set_iterator_t;

		static const int max_length = 32; //!<max length of a window

		uint32_t mask; //!<mask of this window
		uint8_t l;     //!<leftmost bit set in the window
		uint8_t r;     //!<rightmost bit set in the window

		//!constructor
		Window(uint32_t m = 0);
		//!copy constructor
		Window(const Window& w);

		//!get masked value of c
		uint32_t get_val(const uint32_t c) const;

		//!get cardinality of {VAL(s,W)}
		uint32_t get_cardinality(const cases_set& c) const;

		//!is this window critical for this set
		bool is_critical(const cases_set& c) const;
		//!add a bit to the rigth of the window
		void add_right();
		//!remove a bit from the left of this window
		void remove_left();
	};

} //namespace jit
#endif

