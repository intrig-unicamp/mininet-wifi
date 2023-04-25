/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x11-ir.h"
#include "x11-util.h"
#include "x11-emit.h"


#if 0
#include "x11-data.h"
#endif

#include <cassert>
#include <algorithm>

//using LLIR::CnstIRNode;
//using LLIR::Sequence;
using namespace std;

namespace {
void decrease_version(const set<jit::X11::X11IRNode::RegType> &uses,
	set<jit::X11::X11IRNode::RegType> &defs)
{
	set<jit::X11::X11IRNode::RegType>::iterator i;
	set<jit::X11::X11IRNode::RegType> clone;
	
	for(i = defs.begin(); i != defs.end(); ++i) {
		jit::X11::X11IRNode::RegType c(*i);
		if(i->version()) {
			--c.version();
		}
		
		clone.insert(c);
	}
	
	defs = clone;
	
	return;
}
} /* namespace */

namespace jit {
namespace X11 {

X11IRNode::_iterator::_iterator(_iterator_if *n) : node(n)
{
	// Nop
	return;
}

X11IRNode::_iterator::~_iterator()
{
	delete node;
}
		
X11IRNode * X11IRNode::_iterator::operator->()
{
	return node->operator->();
}

X11IRNode *X11IRNode::_iterator::operator*()
{
	return node->operator*();
}
		
X11IRNode::_iterator &X11IRNode::_iterator::operator++()
{
	node->operator++();
	return *this;
}

X11IRNode::_iterator &X11IRNode::_iterator::operator++(int a)
{
	node->operator++(a);
	return *this;
}
	
bool X11IRNode::_iterator::operator!=(const _iterator &b)
{
	return *node != *(b.node);
}

X11IRNode::_iterator &X11IRNode::_iterator::operator=(const _iterator &o)
{
	if(node)
		delete node;
		
	node = o.node->clone();
	return *this;
}

X11IRNode::_iterator::_iterator(const X11IRNode::_iterator &o) : node(o.node->clone())
{
	// Nop
	return;
}


// ------------------------------------------------------------------------------------


X11IRNode::_iterator_impl::_iterator_impl(X11IRNode *n) : node(n), end(false)
{
	// Nop
	return;
}

X11IRNode::_iterator_impl::~_iterator_impl()
{
	// Nop
	return;
}

X11IRNode::_iterator_impl *X11IRNode::_iterator_impl::clone() const
{
	_iterator_impl *t(new _iterator_impl(node));
	t->end = end;
	return t;
}
		
X11IRNode *X11IRNode::_iterator_impl::operator->()
{
	return node;
}

X11IRNode *X11IRNode::_iterator_impl::operator*()
{
	return node;
}
		
X11IRNode::_iterator_impl &X11IRNode::_iterator_impl::operator++()
{
	end = true;
	return *this;
}

X11IRNode::_iterator_impl &X11IRNode::_iterator_impl::operator++(int)
{
//	assert(1 == 0 && "Post increment is not supported");
//	_iterator copy(*this);
	end = true;
//	return copy;
	return *this;
}
		
bool X11IRNode::_iterator_impl::operator!=(const _iterator_if &oe)
{
	const _iterator_impl *poe(dynamic_cast<const _iterator_impl *>(&oe));
	assert(poe && "Comparison between iterators of different types!");
	
	
	const _iterator_impl &e(*poe);
	assert(node == e.node && "Comparison between iterators from different sequences!");

	if(end && e.end) {
//		cout << "FALSE\n";
		return false;
	}
	
//	cout << "TRUE\n";
	return true;
}

// ------------------------------------------------------------------------------------

bool X11IRNode::overwrite_analysis(false);

X11IRNode::X11IRNode(BBId_t bb) : IRNode<RegisterInstance>(bb), merge(false)
{
	// Nop
	return;
}

X11IRNode::~X11IRNode()
{
	// Nop
	return;
}

void X11IRNode::set_merge(bool m)
{
	merge = m;
	return;
}

bool X11IRNode::get_merge() const
{
	return merge;
}

set<X11IRNode::unit_t> X11IRNode::get_units()
{
	assert(1 == 0 && "get_units called on non-hardware node!");
	return set<unit_t>();
}


void X11IRNode::kill_constants(set<X11IRNode::RegType> &s)
{
	set<RegType>::iterator i;
	
	for(i = s.begin(); i != s.end();) {
		if(i->get_model()->get_space() == sp_const) {
			set<RegType>::iterator j(i);
			++i;
			s.erase(j);
		} else {
			++i;
		}
	}
	
	return;
}

X11IRNode::IRNodeIterator X11IRNode::nodeBegin()
{
	IRNodeIterator ir(new _iterator_impl(this));
//	cout << "this: " << this << ", node: " << ir.node << '\n';
	return ir;
}

X11IRNode::IRNodeIterator X11IRNode::nodeEnd()
{
	IRNodeIterator ir(new _iterator_impl(this));
	++ir;
	return ir;
}

bool X11IRNode::isPhi() const
{
	return false;
}

set<X11IRNode::RegType> X11IRNode::getSources()
{
	return getUses();
}

void X11IRNode::enable_overwrite_analysis_mode(bool enable)
{
	overwrite_analysis = enable;
	return;
}

list<pair<X11IRNode::RegType, X11IRNode::RegType> > X11IRNode::getCopiedPair(bool)
{
	return list<pair<RegType, RegType> >();
}

set<X11IRNode::RegType> X11IRNode::getAllUses()
{
	return getUses();
}

set<X11IRNode::RegType> X11IRNode::getAllDefs()
{
	return getDefs();
}

X11IRNode::RegType X11IRNode::getDefReg()
{
	return *getOwnReg();
}

void X11IRNode::setDefReg(RegType)
{
	// Nop
	return;
}

X11IRNode::RegType *X11IRNode::getOwnReg()
{
	return 0;
}

Phi *X11IRNode::make_phi(uint16_t b, RegType r)
{
	return new Phi(b, r);
}

Copy *X11IRNode::make_copy(uint16_t b, RegType src, RegType dst)
{
	return new Copy(b, dst, src);
}

bool X11IRNode::isCopy(bool) const
{
	return false;
}

X11IRNode::ContType X11IRNode::get_reverse_instruction_list()
{
	ContType l;
	l.push_back(this);
	return l;
}

bool X11IRNode::isJump() const
{
	return false;
}

#if 0
void X11IRNode::rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id)
{
	return;
}
#endif


set<X11IRNode::BBId_t> X11IRNode::get_dest()
{
	static set<BBId_t> empty_set;
	return empty_set;
}

bool X11IRNode::is_branch_always() const
{
	return false;
}

void X11IRNode::rewrite_use(Branch::RegType, Branch::RegType)
{
	// Nop
	return;
}


void X11IRNode::rewrite_aliases(map<RegType, RegType> &alias_map)
{
#if 0
	set<RegType> uses(getUses());

	set<RegType>::iterator i;
	for(i = uses.begin(); i != uses.end(); ++i) {
		map<RegType, RegType>::iterator j;
		
//		cout << "rewrite_aliases: examining the use of " << (*i)->emit() << '\n';
		
		j = alias_map.find(*i);
		if(j != alias_map.end()) {
//			cout << "Rewriting " << (*i)->emit() << " with " << j->second->emit() << '\n';
			(*i).rewrite(*(j->second));
		} else {
//			cout << "Register " << (*i)->emit() << " not found...\n";
		}
	}
	
	set<RegType> defs(get_defs());

//	set<x11_data *>::iterator i;
	for(i = defs.begin(); i != defs.end(); ++i) {
		map<RegType, RegType>::iterator j;
		
//		cout << "rewrite_aliases: examining the def of " << (*i)->emit() << '\n';
		
		j = alias_map.find(*i);
		if(j != alias_map.end()) {
//			cout << "Rewriting " << (*i)->emit() << " with " << j->second->emit() << '\n';
			(*i).rewrite(*(j->second));
		} else {
//			cout << "Register " << (*i)->emit() << " not found...\n";
		}
	}
#endif

	return;
}

set<pair<X11IRNode::RegType, X11IRNode::RegType> > X11IRNode::get_aliases()
{
	static set<pair<RegType, RegType> > empty_set;
	return empty_set;
}

bool X11IRNode::end_sequence() const
{
	return false;
}



Alu::Alu(BBId_t bb, RegType res, RegType left_op, RegType right_op) : X11IRNode(bb)
{
	kids.push_back(res);
	kids.push_back(left_op);
	kids.push_back(right_op);
	
	if(res.get_model()->get_space() == sp_reg_mem) {
//		std::cerr << "Uso anche il registro di offset off_reg\n";
		extra_uses.insert(RegisterInstance(X11::REG_SPACE(sp_offset), X11::OFF_SPACE(off_reg), 0));
	}

	if(res.get_model()->get_space() == sp_pkt_mem) {
//		std::cerr << "Uso anche il registro di offset off_pkt\n";
		extra_uses.insert(RegisterInstance(X11::REG_SPACE(sp_offset), X11::OFF_SPACE(off_pkt), 0));
	}

	return;
}

Alu::Alu(BBId_t bb, RegType res, RegType left_op) : X11IRNode(bb)
{
	kids.push_back(res);
	kids.push_back(left_op);

	if(res.get_model()->get_space() == sp_reg_mem) {
//		std::cerr << "Uso anche il registro di offset off_reg\n";
		extra_uses.insert(RegisterInstance(X11::REG_SPACE(sp_offset), X11::OFF_SPACE(off_reg), 0));
	}

	if(res.get_model()->get_space() == sp_pkt_mem) {
//		std::cerr << "Uso anche il registro di offset off_pkt\n";
		extra_uses.insert(RegisterInstance(X11::REG_SPACE(sp_offset), X11::OFF_SPACE(off_pkt), 0));
	}

	return;
}

bool Alu::has_side_effects()
{
	return kids[0].get_model()->get_space() != sp_virtual;
}


set<Alu::unit_t> Alu::get_units()
{
	set<unit_t> s;
	s.insert(alu_u);
	return s;
}


void Alu::rewrite_use(Alu::RegType old, Alu::RegType version)
{
	assert(version.get_model()->get_space() != 0);

	vector<RegType>::iterator i;
	for(i = ++kids.begin(); i != kids.end(); ++i) {
		if(old == *i)
			*i = version;
	}
	
	if(extra_uses.find(old) != extra_uses.end()) {
		extra_uses.erase(old);
		extra_uses.insert(version);
	}
	
	return;
}


void Alu::setDefReg(Alu::RegType r)
{
	kids[0] = r;
	return;
}

Alu::RegType *Alu::getOwnReg()
{
	return &(kids.front());
}

ostream& Alu::printNode(ostream& os, bool)
{
	os << get_opcode() << " / ";
	vector<RegType>::iterator i;
	for(i = kids.begin(); i != kids.end(); ++i)
		os << printReg(*i) << " ";
	return os;
}

unsigned Alu::get_opcount()
{
	return kids.size();
}

Alu::~Alu()
{
	return;
}

set<Alu::RegType> Alu::getUses()
{
	set<RegType> uses(++kids.begin(), kids.end());
	uses.insert(extra_uses.begin(), extra_uses.end());
	kill_constants(uses);

	if(overwrite_analysis) {
		uses.clear();
#if 0
		set<RegType> t, defs(getDefs());
		/* XXX HACK TO MAKE IT WORK QUICKLY */
		decrease_version(uses, defs);
		set_union(uses.begin(), uses.end(), defs.begin(), defs.end(),
			insert_iterator<set<RegType> >(t, t.begin()));
		uses = t;
#endif
	}

	return uses;
}

set<Alu::RegType> Alu::getDefs()
{
	set<RegType> defs(kids.begin(), ++kids.begin());
	kill_constants(defs);

	if(overwrite_analysis)
		defs.clear();
	
	return defs;
}

void Alu::emit(X11EmissionContext *ec)
{
	vector<string> t;
	
	t.push_back(get_opcode());
	vector<RegType>::iterator i;
	for(i = kids.begin(); i != kids.end(); ++i) {
		t.push_back(X11Register::emit(*i));
	}
	
	ec->emit(t, get_merge());
	return;
}

Copy::Copy(BBId_t bb, RegType d, RegType s) : X11IRNode(bb), src(s), dst(d)
{
	RegType po(REG_SPACE(sp_offset), off_pkt);
	RegType ro(REG_SPACE(sp_offset), off_reg);
	RegType pm(REG_SPACE(sp_pkt_mem), 0);
	RegType rm(REG_SPACE(sp_reg_mem), 0);

	// Be careful to present the complete uses of this instruction
	if(src == pm || dst == pm) {
		cout << "Lavoro con la memoria\n";
		helpers.insert(po);
	}
	
	if(src == rm || dst == rm) {
		helpers.insert(ro);
	}

	return;
}

Copy::~Copy()
{
	return;
}

list<pair<Copy::RegType, Copy::RegType> > Copy::getCopiedPair(bool assignement)
{
	list<pair<RegType, RegType> > copies;
	if(assignement || src.get_model()->get_space() == dst.get_model()->get_space())
	copies.push_back(make_pair(dst, src));
	return copies;
}

void Copy::rewrite_use(Copy::RegType old, Copy::RegType version)
{
	if(old == src)
		src = version;
	
	// Search in helpers
	if(helpers.find(old) != helpers.end()) {
		cout << "Rewriting HELPER\n";
		helpers.erase(old);
		helpers.insert(version);
	}
	
	return;
}

void Copy::setDefReg(Copy::RegType r)
{
	dst = r;
	return;
}

Copy::RegType *Copy::getOwnReg()
{
	return &dst;
}

ostream &Copy::printNode(ostream &os, bool)
{
	os << printReg(dst) << " <- " << printReg(src);	
	if(helpers.size()) {
		os << " / (uses ";
		set<RegType>::iterator i;
		for(i = helpers.begin(); i != helpers.end(); ++i)
			os << printReg(*i) << ' ';
		os << ')';
	}
	return os;
}

bool Copy::isCopy(bool is_assignement) const
{
	// This operation is always an assignement
	if(is_assignement)
		return true;

	// This operation is a copy only if src and dst belong to the same space
#if 0
	if(src.get_model()->get_space() != REG_SPACE(sp_const) &&
		src.get_model()->get_space() != REG_SPACE(sp_pkt_mem) &&
		(dst.get_model()->get_space() == REG_SPACE(sp_virtual) ||
		 dst.get_model()->get_space() == REG_SPACE(sp_sw_help))
	  )
#endif
	if(src.get_model()->get_space() == dst.get_model()->get_space())
		return true;

	return false;
}

set<Copy::RegType> Copy::getSources()
{
	set<RegType> uses;
	uses.insert(src);
	return uses;
}

set<Copy::RegType> Copy::getUses()
{
	set<RegType> uses;
	uses.insert(src);
	kill_constants(uses);

	set<RegType> t;
	set_union(uses.begin(), uses.end(), helpers.begin(), helpers.end(),
			insert_iterator<set<RegType> >(t, t.begin()));

	return t;
}

set<Copy::RegType> Copy::getDefs()
{
	set<RegType> defs;
	defs.insert(dst);
	kill_constants(defs);

	if(overwrite_analysis)
		defs.clear();

	return defs;
}


void Copy::rewrite_aliases(std::map<RegType, RegType> &)
{
	// We do not overwrite moves, ever
	return;
}


set<pair<Copy::RegType, Copy::RegType> > Copy::get_aliases()
{
	set<pair<RegType, RegType> > alias;
	
	// FIXME: for now let's consider only the register file and constants
//	if((src->get_reg_type() == rf || src->get_reg_type() == cnst) &&
//		(dst->get_reg_type() == rf || dst->get_reg_type() == cnst)) {
	//if(src->get_reg_type() == rf /* && dst->get_reg_type() == rf */) {
//		alias.insert(make_pair(dst, src));
	//} else if(dst->get_reg_type() == rf) {
	//	alias.insert(make_pair(src, dst));
//	}

	/* FIXME: convert to the new register model, where the type is indicated by the
	 * the register space */
#if 0
	/* The main idea here is that only normal register are aliases of something else.
	 * Moreover, switch helper can be aliased by normal registers */
	if(((dst->get_reg_type() == rf || dst->get_reg_type() == rf_sw) && src->get_reg_type() == rf) ||
		(dst->get_reg_type() == rf_sw && src->get_reg_type() == rf_sw)) {
		alias.insert(make_pair(dst, src));
	}
#endif

	return alias;
}

bool Copy::has_side_effects()
{
	uint32_t space(dst.get_model()->get_space());
	
	// Play it safe. Could be less stringent, though.
	if(space == REG_SPACE(sp_virtual) || space == REG_SPACE(sp_sw_help))
		return false;
		
	if(space == src.get_model()->get_space())
		return false;
		
	return true;
}

set<Copy::unit_t> Copy::get_units()
{
	set<unit_t> s;

	if(src.get_model()->get_space() != sp_const && src.get_model()->get_space() != sp_reg_mem &&
		dst.get_model()->get_space() != sp_reg_mem && src.get_model()->get_space() != sp_device &&
		src.get_model()->get_space() != sp_device) {
		s.insert(copy_u);
	} else {
		s.insert(alu_u);
	}

	return s;
}

void Copy::emit(X11EmissionContext *ec)
{
#if 0
	/* FIXME HACK: avoid to emit the instruction if it is not useful, while we
	 * do not have a decent dead code elimination in place.
	 */
	if(src == dst)
		return;
#endif

	vector<string> t;
	
	/* FIXME: here we have to emit the right opcode. For now I just assume that the
	 * destination is the right size and that the source does not cause any trouble */
	/* FIXME: [2] the code I wrote was not enough when the source operand is a register and
	 * the destination is packet memory, so I'll modify it a bit to fit that case, but
	 * sooner or later I'll have to fix it for real */

	bool done = false;
	operand_size_t size =
		(dst.get_model()->get_space() == REG_SPACE(sp_virtual) ||
		 dst.get_model()->get_space() == REG_SPACE(sp_sw_help)) ?
			dst.getProperty<operand_size_t>("size") : src.getProperty<operand_size_t>("size");
			
#if 0
	cout << "Emitting copy:\n";
	printNode(cout, true);
	cout << "\ndesignated size: " << size << '\n';
	
	cout << "source size: " << src.getProperty<operand_size_t>("size") << ", dst size: " <<
		dst.getProperty<operand_size_t>("size") << '\n';
#endif

	if(src.get_model()->get_space() != sp_const && src.get_model()->get_space() != sp_reg_mem &&
		dst.get_model()->get_space() != sp_reg_mem && src.get_model()->get_space() != sp_device &&
		src.get_model()->get_space() != sp_device) {
		do {
			switch(size) {
				case byte8:
					t.push_back("movb");
					done = true;
					break;
				case word16:
					t.push_back("movw");
					done = true;
					break;
				case dword32:
					t.push_back("movdw");
					done = true;
					break;
				default:
					// FIXME: this is obviously wrong.
					if(!done && size != src.getProperty<operand_size_t>("size")) {
						size = src.getProperty<operand_size_t>("size");
					} else {
						t.push_back("mov");
						done = true;
					}
			}
		} while(!done);
	} else
		t.push_back("mov");

	t.push_back(X11Register::emit(dst));
	t.push_back(X11Register::emit(src));

	ec->emit(t, get_merge());
	return;
}


Load::Load(BBId_t bb, RegType d, RegType s) : X11IRNode(bb), src(s), dst(d)
{
	assert(dst.get_model()->get_space() == sp_offset);
	return;
}

Load::~Load()
{
	return;
}

set<Load::unit_t> Load::get_units()
{
	set<unit_t> s;
	s.insert(load_u);
	return s;
}

bool Load::isCopy(bool is_assignement) const
{
	return is_assignement;
}

void Load::rewrite_use(Load::RegType old, Load::RegType version)
{
	if(old == src)
		src = version;
	return;
}

list<pair<Load::RegType, Load::RegType> > Load::getCopiedPair(bool assignement)
{
	if(assignement) {
		list<pair<RegType, RegType> > l;
		l.push_back(make_pair(dst, src));
		return l;
	}

	return list<pair<RegType, RegType> >();
}


void Load::setDefReg(Load::RegType r)
{
	dst = r;
	return;
}

Load::RegType *Load::getOwnReg()
{
	return &dst;
}

ostream& Load::printNode(ostream& os, bool)
{
	os << "ld / " << printReg(dst) << " <- "  << printReg(src);	
//	os << " / uses ";
//	set<RegType> u(getUses());
//	set<RegType>::iterator i;
//	for(i = u.begin(); i != u.end(); ++i) {
//		os << printReg(*i) << ' ';
//	}
	
//	os << "/ defs ";
//	u = getDefs();
//	for(i = u.begin(); i != u.end(); ++i) {
//		os << printReg(*i) << ' ';
//	}
	
	return os;
}

set<Load::RegType> Load::getUses()
{
	set<RegType> uses;
	uses.insert(src);
	kill_constants(uses);
	
	/* XXX we enable usage of the overwritten register so that a phi will be 
	 * inserted in the BB this instruction belongs to */
	if(overwrite_analysis) {
		set<RegType> t, defs(getDefs());
		
		/* XXX HACK TO MAKE IT WORK QUICKLY */
		decrease_version(uses, defs);
		
		set_union(uses.begin(), uses.end(), defs.begin(), defs.end(),
			insert_iterator<set<RegType> >(t, t.begin()));
		uses = t;
	}

	return uses;
}

set<Load::RegType> Load::getSources()
{
	set<RegType> srcs;
	srcs.insert(src);
	return srcs;
}

set<Load::RegType> Load::getDefs()
{
	set<RegType> defs;
	defs.insert(dst);
	kill_constants(defs);
	return defs;
}

void Load::emit(X11EmissionContext *ec)
{
	vector<string> t;
	
	t.push_back("ld");
	t.push_back(X11Register::emit(dst));
	t.push_back(X11Register::emit(src));
	
	ec->emit(t, get_merge());

	return;
}
	

Branch::Branch(BBId_t bb, BBId_t dst, enum branch_condition_t c) : X11IRNode(bb), condition(c),
	dst_label(dst)
{
	return;
}

Branch::~Branch()
{
	return;
}

set<Branch::unit_t> Branch::get_units()
{
	/* FIXME: since we don't keep track of the flag register usage/definition yet,
	 * we say that the branch instruction uses the ALU unit so we don't get merged with
	 * the previous ALU instruction.
	 */
	set<unit_t> s;
	s.insert(branch_u);
	s.insert(alu_u);
	return s;
}

bool Branch::isJump() const
{
	return true;
}

set<Branch::BBId_t> Branch::get_dest()
{
	set<BBId_t> dest;
	dest.insert(dst_label);
	return dest;
}

bool Branch::is_branch_always() const
{
	return condition == always;
}

ostream& Branch::printNode(std::ostream& os, bool)
{
	os << "branch / ";
	
	switch(condition) {
		case always:
			os << "always / ";
			break;
		case equal:
			os << "equal / ";
			break;
		case not_equal:
			os << "not_equal / ";
			break;
		case not_zero:
			os << "not zero / ";
			break;
		default:
			os << "?? / ";
	}
	
	os << dst_label;
	
	return os;
}


/* FIXME: this is not quite correct. The branch instruction can use the processor status
 * register, which by the way should be versioned as well.
 */
set<Branch::RegType> Branch::getUses()
{
	set<RegType> uses;
	return uses;
}

set<Branch::RegType> Branch::getDefs()
{
	set<RegType> defs;
	return defs;
}

void Branch::emit(X11EmissionContext *ec)
{
	vector<string> t;
	
	switch(condition) {
		case always:
			t.push_back("bra");
			break;
		case equal:
			t.push_back("beq");
			break;
		case not_equal:
			t.push_back("bne");
			break;
		case not_zero:
			t.push_back("bnz");
			break;
		default:
			t.push_back("<unknown branch>");
	}
	
	t.push_back(make_label(stringify(dst_label)));
	
	ec->emit(t, get_merge());
}

/* Define specific operations within each unit */

Not::Not(BBId_t bb, RegType res, RegType op0) :
	Alu(bb, res, op0)
{
	// Nop
	return;
}

Not::~Not()
{
	// Nop
	return;
}

string Not::get_opcode() const
{
	return "not";
}


Boolean::Boolean(BBId_t bb, RegType res, RegType op0, RegType op1, boolean_t op_t) :
	Alu(bb, res, op0, op1), op_type(op_t)
{
	// Nop
	return;
}

Boolean::~Boolean()
{
	// Nop
	return;
}

string Boolean::get_opcode() const
{
	switch(op_type) {
		case alu_and:
			return "and";
			break;
		case alu_or:
			return "or";
			break;
		case alu_xor:
			return "xor";
			break;
		default:
			;
	}
	
	return "<unknown boolean operator>";
}

Shift::Shift(BBId_t bb, RegType res, RegType op0, RegType op1, shift_direction_t d) :
	Alu(bb, res, op0, op1), dir(d)
{
	// Nop
	return;
}

Shift::~Shift()
{
	// Nop
	return;
}

string Shift::get_opcode() const
{
	switch(dir) {
		case left:
			return "sll";
			break;
		case right:
			return "srl";
			break;
		default:
			;
	}
	
	return "<unknown shift operator>";
}


Add::Add(BBId_t bb, RegType res, RegType op0, RegType op1) : Alu(bb, res, op0, op1)
{

	// Nop
	return;
}

string Add::get_opcode() const
{
	return "add";
}

Add::~Add()
{
	// Nop
	return;
}

Addc::Addc(BBId_t bb, RegType res, RegType op0, RegType op1) : Alu(bb, res, op0, op1)
{
	extra_uses.insert(res);
	return;
}

string Addc::get_opcode() const
{
	return "addc";
}

Addc::~Addc()
{
	// Nop
	return;
}

Subc::Subc(BBId_t bb, RegType res, RegType op0, RegType op1) : Alu(bb, res, op0, op1)
{
	extra_uses.insert(res);
	return;
}

string Subc::get_opcode() const
{
	return "subc";
}

Subc::~Subc()
{
	// Nop
	return;
}


Sub::Sub(BBId_t bb, RegType res, RegType op0, RegType op1) : Alu(bb, res, op0, op1)
{
	// Nop
	return;
}

string Sub::get_opcode() const
{
	return "sub";
}

Sub::~Sub()
{
	// Nop
	return;
}


Compare::Compare(BBId_t bb, RegType op0, RegType op1) : Alu(bb, op0, op1)
{
	// Nop
	return;
}

string Compare::get_opcode() const
{
	return "cmp";
}

Compare::~Compare()
{
	// Nop
	return;
}

set<Compare::RegType> Compare::getUses()
{
	set<RegType> uses(kids.begin(), kids.end());
	kill_constants(uses);

	if(overwrite_analysis) {
		uses.clear();
#if 0
		set<RegType> t, defs(getDefs());

		/* XXX HACK TO MAKE IT WORK QUICKLY */
		decrease_version(uses, defs);
	
		set_union(uses.begin(), uses.end(), defs.begin(), defs.end(),
			insert_iterator<set<RegType> >(t, t.begin()));
		uses = t;
#endif
	}
	
	return uses;
}

set<Compare::RegType> Compare::getDefs()
{
	set<RegType> defs;
	return defs;
}

Prepare::Prepare(BBId_t bb, string dr, Prepare::RegType d, set<Prepare::RegType> s, bool jump)
	: X11IRNode(bb), driver(dr), dst(d), src(s), is_jump(jump), addr(RegType::invalid)
{
	// Nop
	return;
}

Prepare::Prepare(BBId_t bb, std::string dr, RegType d, std::set<RegType> s, RegType a,
	string e, string o, bool jump)
	: X11IRNode(bb), driver(dr), dst(d), src(s), is_jump(jump), addr(a), engine(e), op(o)
{
	// Nop
	return;
}

Prepare::~Prepare()
{
	// Nop
	return;
}

void Prepare::setExtraUses(std::set<RegType> s)
{
	extra_uses = s;
	return;
}

bool Prepare::has_side_effects()
{
	return true;
}

set<Prepare::unit_t> Prepare::get_units()
{
	// FIXME: do the right thing here!!!
	set<unit_t> s;
	s.insert(alu_u);
	s.insert(copy_u);
	s.insert(load_u);
	s.insert(branch_u);
	return s;
}

void Prepare::rewrite_use(Prepare::RegType old, Prepare::RegType version)
{
	if(src.find(old) != src.end()) {
		src.erase(old);
		src.insert(version);
	}
	
	if(extra_uses.find(old) != extra_uses.end()) {
		extra_uses.erase(old);
		extra_uses.insert(version);
	}
	
	return;
}

bool Prepare::isJump() const
{
	/* FIXME: we should check if this engine access generates a jump or not! */
	return is_jump;
}


void Prepare::setDefReg(Prepare::RegType r)
{
	dst = r;
	return;
}

Prepare::RegType *Prepare::getOwnReg()
{
	return &dst;
}

ostream& Prepare::printNode(std::ostream& os, bool)
{
	os << "prepare / " << driver << " / " << printReg(dst) << " / ";
	set<RegType>::iterator i;
	set<RegType> u(getUses());
	for(i = u.begin(); i != u.end(); ++i) {
		os << printReg(*i) << ' ';
	}
	return os;
}

	
set<Prepare::RegType> Prepare::getUses()
{
	set<RegType> uses(src);
	uses.insert(extra_uses.begin(), extra_uses.end());
	kill_constants(uses);
	
	if(overwrite_analysis) {
		uses.clear();
#if 0
		set<RegType> t, defs(getDefs());
		
		/* XXX HACK TO MAKE IT WORK QUICKLY */
		decrease_version(uses, defs);

		set_union(uses.begin(), uses.end(), defs.begin(), defs.end(),
			insert_iterator<set<RegType> >(t, t.begin()));
		uses = t;
#endif
	}
	
	return uses;
}

set<Prepare::RegType> Prepare::getDefs()
{
	set<RegType> defs;
	defs.insert(dst);
	kill_constants(defs);
	return defs;
}

/* FIXME: for now every engine access is seen as a jump */
void Prepare::emit(X11EmissionContext *ec)
{
	vector<string> t;
	
	t.push_back("prepare");
	t.push_back(driver);
	
	ec->emit(t, get_merge());
	
	string source("none");
	if(src.size()) {
		RegType reg(*(src.begin()));
		source = X11Register::emit(reg, false);
	}
	
	
	
//	reg.setProperty<reg_part_t>("part", r_none);
//	dst.setProperty<reg_part_t>("part", r_none);

	// Hack to make it work quickly preserving the old interface
	if(addr == RegType::invalid) {
		// Emit the driver declaration, getting the information from the emission context
		ec->add_driver(X11Driver(is_jump, driver, "EAP_" + ec->get_vpisc(), "TCAMEngine", "Lookup80",
			X11Register::emit(dst), source));
	} else {
		ec->add_driver(X11Driver(is_jump, driver, "EAP_" + ec->get_vpisc(), engine, op,
			X11Register::emit(dst), source, stringify("none"),
			X11Register::emit(addr)));
	}

	return;
}

bool Prepare::end_sequence() const
{
	return true;
}


Phi::Phi(Phi::BBId_t bbid, Phi::RegType r) : X11IRNode(bbid), dst(r)
{
	// Nop
	return;
}

Phi::~Phi()
{
	// Nop
	return;
}

set<Phi::RegType> Phi::getUses()
{
	params_iterator i;
	set<RegType> uses;
	for(i = paramsBegin(); i != paramsEnd(); ++i) {
		uses.insert(i->second);
	}
		
	return uses;
}

set<Phi::RegType> Phi::getDefs()
{
	set<RegType> s;
	s.insert(dst);
	return s;
}

ostream& Phi::printNode(std::ostream& os, bool)
{
	os << "phi / " << printReg(dst) << " / ";
	params_iterator i;
	for(i = paramsBegin(); i != paramsEnd(); ++i)
		os << printReg(i->second) << " from " << i->first << ' ';

	return os;
}

bool Phi::isPhi() const
{
	return true;
}

list<pair<Phi::RegType, Phi::RegType> > Phi::getCopiedPair(bool assignement)
{
	list<pair<RegType, RegType> > l;
	
	if(assignement) {
		params_iterator i;
	
		for(i = paramsBegin(); i != params.end(); ++i) {
			l.push_back(make_pair(dst, i->second));
		}
	}
	
	return l;
}

void Phi::emit(X11EmissionContext *)
{
	assert(1 == 0 && "Tried to emit a PHI!");
	return;
}

Phi::params_iterator Phi::paramsBegin()
{
	return params.begin();
}

Phi::params_iterator Phi::paramsEnd()
{
	return params.end();
}

void Phi::setDefReg(RegType r)
{
	dst = r;
	return;
}

Phi::RegType *Phi::getOwnReg()
{
	return &dst;
}

Phi::RegType Phi::getDefReg() const
{
	return dst;
}

void Phi::addParam(RegType r, uint16_t bb)
{
	params[bb] = r;
	return;
}

OverwriteAnalysisWrap::FakeNode::FakeNode(BBId_t bb, X11IRNode *r) : X11IRNode(bb)
{
	set<RegType>::iterator i;
	set<RegType> defs(r->getDefs());
	
	for(i = defs.begin(); i != defs.end(); ++i) {
		cout << "Inserisco " << *i << '\n';
		my_uses[*(i->get_model())] = *i;	
	}

	return;
}

OverwriteAnalysisWrap::FakeNode::~FakeNode()
{
	// Nop
	return;
}

set<OverwriteAnalysisWrap::FakeNode::RegType> OverwriteAnalysisWrap::FakeNode::getUses()
{
	set<RegType> local_uses;
	map<RegisterModel, RegType>::iterator i;

	for(i = my_uses.begin(); i != my_uses.end(); ++i) {
		local_uses.insert(i->second);
	}
	
	return local_uses;
}

set<OverwriteAnalysisWrap::FakeNode::RegType> OverwriteAnalysisWrap::FakeNode::getDefs()
{
	set<RegType> defs;
	return defs;
}
	
void OverwriteAnalysisWrap::FakeNode::emit(X11EmissionContext *)
{
	return;
}
	
ostream& OverwriteAnalysisWrap::FakeNode::printNode(std::ostream& os, bool)
{
	os << "fake node";
	return os;
}

void OverwriteAnalysisWrap::FakeNode::setDefReg(RegType r)
{
	assert(1 == 0 && "Cannot set default register");
	return;
}
	
OverwriteAnalysisWrap::FakeNode::RegType *OverwriteAnalysisWrap::FakeNode::getOwnReg()
{
	// FIXME: temporary HACK to make it work with the existing SSA code.
	if(my_uses.size())
		return &(my_uses.begin()->second);
	return 0;
}

void OverwriteAnalysisWrap::FakeNode::rewrite_use(RegType old, RegType version)
{
	if(my_uses.find(*(old.get_model())) != my_uses.end()) {
		my_uses[*(old.get_model())] = version;
	}
	return;
}

OverwriteAnalysisWrap::OverwriteAnalysisWrap(BBId_t bb, X11IRNode *r) : X11IRNode(bb),
	real(r), fake_node(bb, r) 
{
	// Nop
	return;
}

OverwriteAnalysisWrap::~OverwriteAnalysisWrap()
{
	// Nop
	return;
}
	
void OverwriteAnalysisWrap::setDefReg(RegType r)
{
	real->setDefReg(r);
}

OverwriteAnalysisWrap::RegType *OverwriteAnalysisWrap::getOwnReg()
{
	return real->getOwnReg();
}
	
void OverwriteAnalysisWrap::rewrite_use(RegType old, RegType version)
{
	fake_node.rewrite_use(old, version);	
	real->rewrite_use(old, version);
	return;
}

set<OverwriteAnalysisWrap::RegType> OverwriteAnalysisWrap::getReachingDefs()
{
	return fake_node.getUses();
}
	
set<OverwriteAnalysisWrap::RegType> OverwriteAnalysisWrap::getUses()
{
	return real->getUses();
	
#if 0
	set<RegType> local_uses, defs(real->getDefs()), uses(real->getUses()), t;
	set<RegType>::iterator i;
	
	for(i = defs.begin(); i != defs.end(); ++i) {
		cout << "cerco " << *i << '\n';
		assert(my_uses.find(*(i->get_model())) != my_uses.end() && "A register disappeared!");
		local_uses.insert(my_uses[*(i->get_model())]);
	}
	
	set_union(uses.begin(), uses.end(), local_uses.begin(), local_uses.end(),
		insert_iterator<set<RegType> >(t, t.begin()));
	
	return t;
#endif
}

std::set<OverwriteAnalysisWrap::RegType> OverwriteAnalysisWrap::getAllUses()
{
	set<RegType> mine(getUses()), fake(fake_node.getUses()), t;
	set_union(mine.begin(), mine.end(), fake.begin(), fake.end(),
		insert_iterator<set<RegType> >(t, t.begin()));
		
	return t;
}

std::set<OverwriteAnalysisWrap::RegType> OverwriteAnalysisWrap::getAllDefs()
{
	set<RegType> mine(getDefs()), fake(fake_node.getDefs()), t;
	set_union(mine.begin(), mine.end(), fake.begin(), fake.end(),
		insert_iterator<set<RegType> >(t, t.begin()));
		
	return t;
}

std::set<OverwriteAnalysisWrap::RegType> OverwriteAnalysisWrap::getDefs()
{
	return real->getDefs();
}

void OverwriteAnalysisWrap::emit(X11EmissionContext *ec)
{
	real->emit(ec);
	return;
}

X11IRNode *OverwriteAnalysisWrap::getReal()
{
	return real;
}
	
std::ostream& OverwriteAnalysisWrap::printNode(std::ostream& os, bool b)
{
	os << "oaw / reaching defs ";
	set<RegType> my_uses(fake_node.getUses());
	set<RegType>::iterator i;
	for(i = my_uses.begin(); i != my_uses.end(); ++i) {
		os << printReg(*i) << ' ';
	}
	os << '\n';
	return real->printNode(os, b);
}


OverwriteAnalysisWrap::IRNodeIterator OverwriteAnalysisWrap::nodeBegin()
{
	IRNodeIterator i(new _fake_iterator(this));
	return i;
}

OverwriteAnalysisWrap::IRNodeIterator OverwriteAnalysisWrap::nodeEnd()
{
	IRNodeIterator i(new _fake_iterator(this));
	
	// Go to the end of the sequence
	++i;
	++i;
	return i;
}


// ------------------------------------------------------------------------------------


OverwriteAnalysisWrap::_fake_iterator::_fake_iterator(OverwriteAnalysisWrap *n) :
	counter(0), node(n)
{
	// Nop
	return;
}

OverwriteAnalysisWrap::_fake_iterator::~_fake_iterator()
{
	// Nop
	return;
}

OverwriteAnalysisWrap::_fake_iterator *OverwriteAnalysisWrap::_fake_iterator::clone() const
{
	_fake_iterator *o(new _fake_iterator(node));
	o->counter = counter;
	return o;
}

OverwriteAnalysisWrap::_fake_iterator &
	OverwriteAnalysisWrap::_fake_iterator::operator++() {
	++counter;
	return *this;
}

OverwriteAnalysisWrap::_fake_iterator &
	OverwriteAnalysisWrap::_fake_iterator::operator++(int)
{
	++counter;
	return *this;
}
	
bool OverwriteAnalysisWrap::_fake_iterator::operator!=(const _iterator_if &oe)
{
	const _fake_iterator *poe(dynamic_cast<const _fake_iterator *>(&oe));
	assert(poe && "Comparison between iterators of different types!");
		
	const _fake_iterator &e(*poe);
	assert(node == e.node && "Comparison between iterators from different sequences!");

	return counter != e.counter;
}

X11IRNode *OverwriteAnalysisWrap::_fake_iterator::operator->()
{
	X11IRNode *n(0);

	switch(counter) {
		case 0:
			n = &(node->fake_node);
			break;
		case 1:
			n = node->real;
			break;
		default:
			assert(1 == 0 && "Iterator out of range");
	}
	
	return n;
}

X11IRNode *OverwriteAnalysisWrap::_fake_iterator::operator*()
{
	return operator->();
}


// ------------------------------------------------------------------------------------



} /* namespace X11 */
} /* namespace jit */
