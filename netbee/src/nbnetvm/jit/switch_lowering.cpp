/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "switch_lowering.h"
#include <iostream>
#include <sstream>
#include <iterator>
using namespace std;
using namespace jit;

//#define _DEBUG_SWITCH_LOWERING

Window::Window(const Window& w)
	:
		mask(w.mask),
		l(w.l),
		r(w.r)
{
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "\ttWindow copy mask: " << mask << " l: " << (uint32_t)l << " r: " << (uint32_t)r << endl;
#endif
}

Window::Window(uint32_t m)
: mask(m), l(0), r(0)
{
	uint8_t i;

	for(i=0; (i < max_length) && ((m % 2) == 0); i++)
	{
		m = m >> 1;
	}

	r = i;

	for(; (i < max_length) && ((m % 2) == 1); i++)
	{
		m = m >> 1;
	}

	l = i - 1;

#ifdef _DEBUG_SWITCH_LOWERING
	cout << "\tNew window mask: " << mask << " m: " << m << " l: " << (uint32_t)l << " r: " << (uint32_t)r << endl;
#endif
}

uint32_t Window::get_val(const uint32_t c) const
{
	if(l < r)
		return 0;

	return (c & mask) >> r;
}

uint32_t Window::get_cardinality(const cases_set& c) const
{
	//assert(l >= r);
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "\t\tget_cardinality" << endl;
#endif
	if(l < r)
		return 0;

	set<uint32_t> vals;
	cases_set_iterator_t i;

	for(i = c.begin(); i < c.end(); i++)
	{
		vals.insert( get_val(i->first) );
	}
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "\t\tresult: " << vals.size() << endl;
#endif
	return vals.size();
}

bool Window::is_critical(const cases_set& c) const
{	
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "is_critical ";
#endif
	bool result = get_cardinality(c) > (((uint32_t)1) << (l-r));
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "result: " << result << endl;
#endif
	return result;
}

void Window::add_right()
{
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "add_right" << endl;
#endif
	assert(r > 0);
	r--;
	mask |= (1 << r);

	if(l == max_length)
		l--;
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "mask: " << mask << " l: " << (uint32_t)l << " r: " << (uint32_t)r << endl;
#endif
}

void Window::remove_left()
{
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "remove_left" << endl;
#endif
	mask &= ~(1 << l);
	l--;
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "mask: " << mask << " l: " << (uint32_t)l << " r: " << (uint32_t)r << endl;
#endif
}

/*!
 * \param bb basic block in which the instruction will be emitted
 * \param insn switch statement to emit
 */
SwitchEmitter::SwitchEmitter(ISwitchHelper& helper, SwitchMIRNode& insn)
	:
		helper(helper),
		insn(insn),
		name(0)
{ 
}

SwitchEmitter::~SwitchEmitter() {}

struct Window SwitchEmitter::find_critical(cases_set& c)
{
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "\tfind_critical" << endl;
#endif
	Window w_max;

	Window res;

	for(int i = 0; i < Window::max_length; i++)
	{
		res.add_right();

		if(res.is_critical(c))
			w_max = res;
		else 
		{
			res.remove_left();
			if(res.get_cardinality(c) > w_max.get_cardinality(c))
				w_max = res;
		}
	}
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "\t\result mask: " << w_max.mask << " l: " << (uint32_t)w_max.l << " r: " << (uint32_t)w_max.r << endl;
#endif
	return w_max;
}

void SwitchEmitter::MRST(cases_set& P)
{
#ifdef _DEBUG_SWITCH_LOWERING
	cout << "\tlevel: " << name << endl << "\thandling cases: " << endl;
	for(cases_set_iterator_t p = P.begin(); p != P.end(); p++)
	{
		cout << "\t\t" << p->first << endl;
	}
#endif
	if(P.size() == 1)
	{
		helper.emit_jcmp_eq(P.front().first, P.front().second, insn.getDefaultTarget());
#ifdef _DEBUG_SWITCH_LOWERING
		cout << "emitting single jcmp for case " << P.front().first << endl;
#endif
		return;
	}

	Window wmax(find_critical(P));

	helper.emit_jmp_to_table_entry(wmax);

	vector< pair<cases_set, uint32_t> > sub_cases;
	uint32_t n = (1 << (wmax.l - wmax.r + 1));

	for(uint32_t j = 0; j < n; j++)
	{
		cases_set P_j;

		cases_set_iterator_t i;
		for(i = P.begin(); i != P.end(); i++)
		{
			if(wmax.get_val(i->first) == j)
			{
				P_j.push_back(case_pair(*i));
			}
		}

		if(P_j.empty())
		{
			helper.emit_table_entry(++name, true);
		}
		else
		{
			helper.emit_table_entry(++name);
			sub_cases.push_back(make_pair(P_j, name));
		}
	}

	vector< pair<cases_set, uint32_t> >::iterator j;

	for(j = sub_cases.begin(); j != sub_cases.end(); j++)
	{
		helper.emit_start_table_entry_code(j->second);
		MRST(j->first);
	}
	
}

string SwitchEmitter::make_name(uint32_t from, uint32_t to)
{
	ostringstream name;
	name << from << "." << to;
	return name.str();
}

void SwitchEmitter::emit_binary_node_branch(
		cases_set& c,
		uint32_t begin,
		uint32_t end
		)
{
#ifdef _DEBUG_SWITCH
	cout << "handling cases: \n";  
	for(uint32_t p = begin; p <= end; p++)
	{
		cout << c[p].first << endl;
	}
#endif
	if(begin == end)
	{
		string my_name( make_name(begin, end) );
		helper.emit_binary_end_jump(c[begin].first, c[begin].second, insn.getDefaultTarget(), my_name);
		return;
	}

	uint32_t pivot = (end - begin) / 2 + begin;
	//uint32_t next  = (end - pivot - 1) / 2 + pivot + 1;

	string my_name( make_name(begin, end));
	string target_name( make_name(pivot+1, end));

	helper.emit_binary_jump(c[pivot + 1].first, my_name, target_name);

	emit_binary_node_branch(c, begin, pivot);
	emit_binary_node_branch(c, pivot+1, end);
}

void SwitchEmitter::binary_switch(cases_set& c)
{
	emit_binary_node_branch(c, 0, c.size() - 1);
}

void SwitchEmitter::emit_jump_table(cases_set& c)
{
	uint32_t size = c.size();
	uint32_t min = c[0].first;
	uint32_t max = c[size -1].first;

	helper.emit_jcmp_l(min, insn.getDefaultTarget(), 0);
	helper.emit_jcmp_g(max, insn.getDefaultTarget(), 0);

	helper.emit_jmp_to_table_entry(min);

	uint32_t current = 0;
	for(uint32_t i = min; i <= max; i++)
	{
		if(c[current].first != i)
		{
			helper.emit_jump_table_entry(insn.getDefaultTarget());
		}
		else
		{
			helper.emit_jump_table_entry(c[current].second);
			current++;
		}
	}
}

void SwitchEmitter::run()
{
	cases_set c(insn.TargetsBegin(), insn.TargetsEnd());
	CaseCompLess less;
	sort(c.begin(), c.end(), less);
		
	uint32_t size = c.size();
	uint32_t density = size * 100 / (c[size - 1].first - c[0].first);

#ifdef _DEBUG_SWITCH_LOWERING	
	cases_set::iterator i = c.begin();
	for (i; i != c.end(); i++)
	{
		cout << (*i).first << " - " << (*i).second <<endl;
	}
#endif

	if(density >= 40)
	{
		#ifdef _DEBUG_SWITCH_LOWERING
			cout << "jump-table" << endl;
		#endif
		emit_jump_table(c);
	}
	/*
	Ivano: I disabled this algorithm, which causes segfoult with some filters
	else if (size < 20)
	{
		#ifdef _DEBUG_SWITCH_LOWERING
			cout << "MRST" << endl;
		#endif
		MRST(c);
	}*/
	else
	{
		#ifdef _DEBUG_SWITCH_LOWERING
			cout << "Bin search" << endl;
		#endif
		binary_switch(c);
	}
}
