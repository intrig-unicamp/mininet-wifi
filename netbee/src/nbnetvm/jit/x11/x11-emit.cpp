/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x11-emit.h"
#include "x11-util.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <list>
using namespace std;

namespace jit {
namespace X11 {

#if 0
string choose_mod(enum x11_register_type t)
{
	switch(t) {
		case octet:
			return ".byte.low";
			break;
		case low:
			return ".word.low";
			break;
		case high:
			return ".word.high";
			break;
		default:
			return ".full";
	}

	return "<unknown modifier>";
}
#endif

X11Header::X11Header()
{
	return;
}

X11Header::~X11Header()
{
	return;
}

void X11Header::dump_header(ostream &out)
{
	// Creates and fills in the X11 structure definition file
	{
		ostream &type_header(out);

		type_header << "struct PacketControlDeviceRegisters {\n";
		type_header << "\tstruct {\n";
		type_header << "\t\tuint16_t Base;\n";
		type_header << "\t\tuint16_t Length;\n";
		type_header << "\t} Memory;\n";
		type_header << "\tuint16_t Drop;\n";
		type_header << "};\n";
		type_header << endl;
		type_header << "enum { DropPacket = 1 };\n";
		type_header << endl;
	}

	// Creates and fills in the program-specific header file
	{
		ostream &prog_header(out);

		//prog_header << "union Register32Type {\n";
		//prog_header << "\tuint32_t full;\n";
		//prog_header << "\tstruct {\n";
		//prog_header << "\t\tuint16_t high;\n";
		//prog_header << "\t\tuint16_t low;\n";
		//prog_header << "\t} word;\n";
		//prog_header << "\tstruct {\n";
		//prog_header << "\t\tuint8_t ba;\n";
		//prog_header << "\t\tuint8_t bb;\n";
		//prog_header << "\t\tuint8_t high;\n";
		//prog_header << "\t\tuint8_t low;\n";
		//prog_header << "\t} byte;\n";
		//prog_header << "};\n";
		//prog_header << endl;

		prog_header << "struct uint16_sw_t {\n";
		prog_header << "	uint16_t number;\n";
		prog_header << "	uint16_t data;\n";
		prog_header << "};\n";

		prog_header << "struct uint8_sw_t {\n";
		prog_header << "	uint16_t number;\n";
		prog_header << "	uint8_t data;\n";
		prog_header << "    uint8_t pad;\n";
		prog_header << "};\n";

		prog_header << "struct uint32_result_t {\n";
		prog_header << "	uint16_t result_0;\n";
		prog_header << "	uint32_t result_1;\n";
		prog_header << "};\n";

		prog_header << "struct uint64_t {\n";
		prog_header << "    uint32_t result_0;\n";
		prog_header << "    uint32_t result_1;\n";
		prog_header << "};\n";
	}

	return;
}


X11Sequence::X11Sequence(std::string n, string vp, bool r) : name(n), vpisc(vp), root(r)
{
	return;
}


X11Sequence::~X11Sequence()
{
	return;
}

#if 0
void X11Sequence::clean_vars()
{
	old_vars.clear();
	return;
}
#endif

void X11Sequence::dump_code(ostream &code)
{
	// Sanity check
	assert(exit != "");

	// Sequence prologue
	{
		// FIXME: the VPISC identifier is always main
		code << (root ? "root " : "sequence ") << name << " at " << vpisc << '\n';
	}

	// Dump the code listing
	{
		vector<string>::iterator i;
		for(i = inst.begin(); i != inst.end(); ++i)
			code << *i << '\n';
	}

	// Sequence epilogue
	{
		code << "exit to " << exit << '\n';
	}

	code << endl;

	return;
}

void X11Sequence::link_next(std::string next)
{
	exit = next;
	return;
}

void X11Sequence::add_insn(std::vector<std::string> code, bool merge)
{
	string instr;

	instr = (merge ? "<=" : "") + stringify('\t');
	instr += code.at(0) + stringify('\t');

	vector<string>::const_iterator i = ++code.begin();

	while(i != code.end()) {
		instr += (*i);
		if(++i != code.end())
			instr += ", \t";
	}

	inst.push_back(instr);

	return;
}



X11Register::X11Register(RegisterInstance ri) :
	number(ri)
{
	assert(number.getProperty<operand_size_t>("size") == ri.getProperty<operand_size_t>("size") &&
		"Incorrect register copy detected");

	// Nop
	return;
}

X11Register::~X11Register()
{
	// Nop
	return;
}

bool X11Register::must_be_declared() const
{
	return true;
}

string X11Register::get_suffix() const
{
	switch(number.getProperty<reg_part_t>("part")) {
		case r_none:
			break;
		case r_low:
			return ".lsb";
			break;
		case r_high:
			return ".msb";
			break;
		case r_16lo:
			return ".lo";
			break;
		case r_16hi:
			return ".hi";
			break;
		case r_result_0:
			return ".result_0";
			break;
		case r_result_1:
			return ".result_1";
			break;
		default:
			return ".??";
	}

	return string();
}

string X11Register::get_name(bool suffix) const
{
	return stringify("reg") + stringify(number.get_model()->get_name()) +
		(suffix ? get_suffix() : "");
}

string X11Register::get_decl()
{
	string s("var ");

	s += get_name(false);

	s += " : ";

	switch(number.getProperty<operand_size_t>("size")) {
		case byte8:
			s += "uint8_t";
			break;
		case word16:
			s += "uint16_t";
			break;
		case dword32:
			s += "uint32_t";
			break;
		case qword64:
			s += "struct uint64_t";
			break;
		default:
			s += "<SIZE ERROR: " + stringify((unsigned)number.getProperty<operand_size_t>("size")) + '>';
	}

	s += '\n';

	return s;
}

X11Register X11Register::make(RegisterInstance &xd)
{
	// Sanity check - we are only interested in the register file here (at least for now)
	assert(xd.get_model()->get_space() == sp_virtual);

	X11Register xr(xd);

	return xr;
}

string X11Register::emit(RegisterInstance &xd, bool suffix)
{
	switch(xd.get_model()->get_space()) {
		case sp_virtual:
			return X11Register::make(xd).get_name(suffix);
			break;
		case sp_device:
			return X11DeviceRegister::make(xd).get_name(suffix);
			break;
		case sp_lk_key64:
			return X11LookupHelper::make(xd).get_name(suffix);
			break;
		case sp_sw_help:
			return X11SwitchHelper::make(xd).get_name(suffix);
			break;
		case sp_sw_table:
		case sp_lk_table:
			return X11SwitchHelper::make(xd).get_table();
			break;
		case sp_offset:
			return X11OffsetRegister::make(xd).get_name(suffix);
			break;
		case sp_const:
			return stringify(xd.get_model()->get_name());
			break;
		case sp_pkt_mem:
			return "pkt[po]";
			break;
		case sp_reg_mem:
			return "rf[ro]";
			break;
		case sp_lk_result:
			return X11ResultRegister::make(xd).get_name(suffix);
			break;
		default:
			;
	}

	return "<unknown register type>";
}

string X11Register::emit_decl(RegisterInstance xd)
{
	switch(xd.get_model()->get_space()) {
		case sp_virtual:
			return X11Register::make(xd).get_decl();
			break;
		case sp_sw_help:
			return X11SwitchHelper::make(xd).get_decl();
			break;
		case sp_lk_key64:
			return X11LookupHelper::make(xd).get_decl();
			break;
		case sp_lk_result:
			return X11ResultRegister::make(xd).get_decl();
			break;
		default:
			;
	}

	return "";
}

X11LookupHelper::X11LookupHelper(RegisterInstance r) : X11Register(r), is_hi(number.getProperty<reg_part_t>("part") == r_result_0)
{
	number.setProperty<reg_part_t>("part", r_none);
	return;
}

X11LookupHelper::~X11LookupHelper()
{
	// Nop
	return;
}

string X11LookupHelper::get_name(bool suffix) const
{
	string name(X11Register::get_name(suffix) + "_lk");
	if(suffix) {
		name += ".key.result";
		if(is_hi)
			name += "0";
		else
			name += "1";
	}
	return name;
}

string X11LookupHelper::get_decl()
{
	if(number.get_model()->get_space() != sp_lk_key64 && number.getProperty<reg_part_t>("part") != r_result_0) {
		return string();
	}

	string s("var ");

	s += X11Register::get_name(false);
	s += "_lk : struct uint64_lk_t\n";

	return s;
}

bool X11LookupHelper::must_be_declared() const
{
	return number.get_model()->get_space() == sp_lk_key64 && number.getProperty<reg_part_t>("part") == r_result_0;
}

X11LookupHelper X11LookupHelper::make(RegisterInstance &xd)
{
	// Sanity check - we are only interested in the register file here (at least for now)
	assert(xd.get_model()->get_space() == sp_lk_key64);
	return X11LookupHelper(xd);
}



X11SwitchHelper::X11SwitchHelper(RegisterInstance r) : X11Register(r)
{
	// Nop
	return;
}

X11SwitchHelper::~X11SwitchHelper()
{
	// Nop
	return;
}

string X11SwitchHelper::get_name(bool suffix) const
{
	string name(X11Register::get_name(suffix) + "_sh");
	if(suffix)
		name += ".data";
	return name;
}

string X11SwitchHelper::get_table() const
{
	string table(X11Register::get_name(false));

	if(number.get_model()->get_space() == sp_lk_table)
		table += "_lk";
	else
		table += "_sh";

	return table + ".number";
}

string X11SwitchHelper::get_decl()
{
	if(number.get_model()->get_space() != sp_sw_help) {
		return string();
	}

	string s("var ");

	s += X11Register::get_name(false);

	s += "_sh : struct ";

	switch(number.getProperty<operand_size_t>("size")) {
		case byte8:
			s += "uint8";
			break;
		case word16:
			s += "uint16";
			break;
		case dword32:
			s += "uint32";
			break;
		default:
			s += "<SIZE ERROR: " + stringify((unsigned)number.getProperty<operand_size_t>("size")) + '>';
	}

	s += "_sw_t\n";

	return s;
}

bool X11SwitchHelper::must_be_declared() const
{
	return number.get_model()->get_space() == sp_sw_help;
}


#if 0
bool X11SwitchHelper::operator<(const X11Register &other) const
{
	if(!other.operator <(*this) && !this->operator <(other)) {
		return get_name() < other.get_name();
	}

	return this->operator <(other);
}
#endif


X11SwitchHelper X11SwitchHelper::make(RegisterInstance &xd)
{
	// Sanity check - we are only interested in the register file here (at least for now)
	assert(xd.get_model()->get_space() == sp_sw_help || xd.get_model()->get_space() == sp_sw_table ||
		xd.get_model()->get_space() == sp_lk_table);
	return X11SwitchHelper(xd);
}


X11ResultRegister::X11ResultRegister(RegisterInstance ri) : X11Register(ri)
{
	// Nop
	return;
}

X11ResultRegister::~X11ResultRegister()
{
	// Nop
	return;
}

string X11ResultRegister::get_name(bool suffix) const
{
	return "result_" + X11Register::get_name(suffix);
}

string X11ResultRegister::get_decl() const
{
	string s("var ");

	s += "result_";
	s += X11Register::get_name(false);

	s += " : struct ";

	switch(number.getProperty<operand_size_t>("size")) {
		case byte8:
			s += "uint8";
			break;
		case word16:
			s += "uint16";
			break;
		case dword32:
			s += "uint32";
			break;
		default:
			s += "<SIZE ERROR: " + stringify((unsigned)number.getProperty<operand_size_t>("size")) + '>';
	}

	s += stringify("_result_t\n");

	return s;
}

X11ResultRegister X11ResultRegister::make(RegisterInstance &xd)
{
	// Sanity check - we are only interested in the register file here (at least for now)
	assert(xd.get_model()->get_space() == sp_lk_result);
	return X11ResultRegister(xd);
}



X11OffsetRegister::X11OffsetRegister(RegisterInstance ri) : X11Register(ri)
{
	// Nop
	return;
}

X11OffsetRegister::~X11OffsetRegister()
{
	// Nop
	return;
}

bool X11OffsetRegister::must_be_declared() const
{
	return false;
}

string X11OffsetRegister::get_name(bool suffix) const
{
	switch(number.get_model()->get_name()) {
		case off_pkt:
			return "po";
			break;
		case off_reg:
			return "ro";
			break;
		default:
			;
	}
	return "<unknown offset register>";
}

string X11OffsetRegister::get_decl()
{
	assert(1 == 0 && "Offset register must not be declared!");
	return string();
}

X11OffsetRegister X11OffsetRegister::make(RegisterInstance &r)
{
	assert(r.get_model()->get_space() == sp_offset);
	return X11OffsetRegister(r);
}

X11DeviceRegister::X11DeviceRegister(RegisterInstance ri)  : X11Register(ri)
{
	// Nop
	return;
}

X11DeviceRegister::~X11DeviceRegister()
{
	// Nop
	return;
}

bool X11DeviceRegister::must_be_declared() const
{
	return false;
}

string X11DeviceRegister::get_name(bool suffix) const
{
	return "dr" + stringify(number.get_model()->get_name());
}

string X11DeviceRegister::get_decl()
{
	assert(1 == 0 && "Device registers must not be declared!");
	return string();
}

X11DeviceRegister X11DeviceRegister::make(RegisterInstance &r)
{
	assert(r.get_model()->get_space() == sp_device);
	return X11DeviceRegister(r);
}


X11Tcam::X11Tcam() : table(0), address(0)
{
	// Nop
	return;
}

X11Tcam::~X11Tcam()
{
	// Nop
	return;
}

X11Tcam::tcam_table_t X11Tcam::new_table()
{
	return table++;
}

X11Tcam::TcamEntry::TcamEntry(vector<uint8_t> m, vector<uint8_t> k, string v, uint32_t a) :
	mask(m), key(k), value(v), address(a)
{
	// Nop
	return;
}

void X11Tcam::add_entry(vector<uint8_t> mask, vector<uint8_t> key, std::string value, tcam_table_t table)
{
	vector<uint8_t> real_mask;
	real_mask.push_back(0xFF);
	real_mask.push_back(0xFF);
	real_mask.insert(real_mask.end(), mask.begin(), mask.end());

	vector<uint8_t> real_key;
	real_key.push_back(((table-1) >> 8) & 0xFF);
	real_key.push_back((table-1) & 0xFF);
	real_key.insert(real_key.end(), key.begin(), key.end());

	cout << "mask: ";
	vector<uint8_t>::iterator i;
	for(i = real_mask.begin(); i != real_mask.end(); ++i)
		cout << (uint32_t)*i << ' ';
	cout << '\n';

	cout << "key: ";
	for(i = real_key.begin(); i != real_key.end(); ++i)
		cout << (uint32_t)*i << ' ';
	cout << '\n';

	cout << "value: " << value << '\n';

	entries.push_back(TcamEntry(real_mask, real_key, value, address++));
}

void X11Tcam::dump(std::ostream &out)
{
	_dump_f dump_f(out);
	for_each(entries.begin(), entries.end(), dump_f);
	return;
}

namespace {
struct _printvector_f {
	ostream *r;
	void operator()(uint8_t d) {
		*r << setw(2) << setfill('0') << (uint32_t)d;
		return;
	}
} printvector_f;

void print_vector(ostream &out, vector<uint8_t> &v)
{
	printvector_f.r = &out;
	for_each(v.begin(), v.end(), printvector_f);
	return;
}

} /* namespace */


void X11Tcam::_dump_f::operator()(TcamEntry t)
{
	out << "access engine TCAMEngine op WriteTCAMMaskAndDisable80 addr 0x" << left<< hex <<
		t.address << " data ";
	print_vector(out, t.mask);
	out << '\n';

	out << "access engine TCAMEngine op WriteRAMData addr 0x" << left << hex << t.address <<
		" data " << t.value << '\n';

	out << "access engine TCAMEngine op WriteTCAMDataAndEnable80 addr 0x" << left << hex <<
		t.address << " data ";
	print_vector(out, t.key);
	out << '\n';

	return;
}


X11Tcam::_dump_f::_dump_f(std::ostream &o) : out(o)
{
	// Nop
	return;
}


X11EmissionContext::X11EmissionContext(X11EmissionContext::CFG & c, std::ostream &os) :
	cfg(c), out(os), active_sequence(0)
{
	// Nop
	return;
}

X11EmissionContext::~X11EmissionContext()
{
	// Nop
	return;
}

X11Tcam &X11EmissionContext::get_tcam()
{
	return tcam;
}

string X11EmissionContext::get_vpisc()
{
	return vpisc;
}

void X11EmissionContext::set_vpisc(string vp)
{
	vpisc = vp;
	return;
}

void X11EmissionContext::emit(std::vector<std::string> insns, bool merge)
{
	active_sequence->add_insn(insns, merge);
	return;
}

void X11EmissionContext::add_driver(X11Driver d)
{
	drivers.insert(d);
}

void X11EmissionContext::dump_drivers()
{
	set<X11Driver>::iterator i;
	for(i = drivers.begin(); i != drivers.end(); ++i) {
		i->dump(out);
		out << '\n';
	}
	return;
}

X11Driver::X11Driver(bool _jump, string _name, string _vpisc, string _target_name, string _op_name,
	string _result_name, string _data0_name, string _data1_name, string _address_name)
	: name(_name), vpisc(_vpisc), target_name(_target_name), op_name(_op_name),
		result_name(_result_name), data0_name(_data0_name), data1_name(_data1_name),
		address_name(_address_name), jump(_jump)
{
	// Nop
	return;
}

#if 0
string &X11Driver::data0()
{
	return data0_name;
}

string &X11Driver::data1()
{
	return data1_name;
}

string &X11Driver::address()
{
	return address_name;
}

string &X11Driver::result()
{
	return result_name;
}

std::string &sequence();
#endif

void X11Driver::dump(std::ostream &out) const
{
	out << "driver " << name << " at " << vpisc << ':' << target_name << '\n';
	out << '\t' << "call" << '\t' << op_name << ",\n";
	out << "\t\t" << result_name << ",\n";
	out << "\t\t" << data0_name << ",\n";
	out << "\t\t" << data1_name << ",\n";
	out << "\t\t" << address_name << '\n';

	out << "exit" << (jump ? " to " +  result_name : "") << '\n';

	return;
}

bool X11Driver::operator<(const X11Driver &oth) const
{
	return name < oth.name;
}


void X11EmissionContext::init_sequences()
{
	std::list<BasicBlock<X11IRNode> *> *bblist(cfg.getBBList());
	std::list<BasicBlock<X11IRNode> *>::iterator BBiter;

	for(BBiter = bblist->begin(); BBiter != bblist->end(); ++BBiter ) {
		BasicBlock<X11IRNode> *bb = *BBiter;
		if ( (bb->getId() != 0) /* && (bb->BCBasicBlockLen != 0) */) {
			bb->setProperty<bool>("root", bb->getId() == 1);
			bb->setProperty<uint32_t>("exit_to", (uint32_t)-1);

			// Initialize the VPISC number so we don't have to special-case it later
			bb->setProperty<uint32_t>("vpisc", 0);

			// Overwrite the successor's name if it is the case
			std::list<BasicBlock<CFG::IRType>::node_t *> &succ(bb->getSuccessors());
			if(succ.size()) {
				BasicBlock<X11IRNode> *bb2 = (*(succ.begin()))->NodeInfo;
				if(((int)bb2->getId()) > 0) {
					bb->setProperty<uint32_t>("exit_to", bb2->getId());
				}
			}

		}
	}

	delete bblist;

	return;
}

void X11EmissionContext::number_vpiscs()
{
	list<BasicBlock<X11IRNode> *> *bblist(cfg.getBBList());
	list<BasicBlock<X11IRNode> *>::iterator BBiter;

	for(BBiter = bblist->begin(); BBiter != bblist->end(); ++BBiter ) {
		BasicBlock<X11IRNode> *bb = *BBiter;

		if(bb->getId() == 0)
			continue;

		uint32_t my_vpisc(bb->getProperty<uint32_t>("vpisc"));

		bool accessed(false);
		_find_accesses accesses(accessed);
		for_each(bb->getCode().begin(), bb->getCode().end(), accesses);

		list<BasicBlock<CFG::IRType>::node_t *> &succ(bb->getSuccessors());
		list<BasicBlock<CFG::IRType>::node_t *>::iterator u;

		for(u = succ.begin(); u != succ.end(); ++u) {
			BasicBlock<CFG::IRType> *bb2 = (*u)->NodeInfo;

//			cout << "BB " << bb2->getId() << " e' successore di BB " << bb->getId() << '\n';

			uint32_t old_vpisc(bb2->getProperty<uint32_t>("vpisc"));

			// FIXME: this might not work in every case.
			if(accessed) {
				bb2->setProperty<uint32_t>("vpisc", max(my_vpisc + 1, old_vpisc));
				bb2->setProperty<bool>("root", true);
			} else {
				bb2->setProperty<uint32_t>("vpisc", max(my_vpisc, old_vpisc));
			}
		}
	}

	// Patch nodes so that the follower happens to be in a different sequence than the predecessor it
	// is correctly marked as root.
	for(BBiter = bblist->begin(); BBiter != bblist->end(); ++BBiter ) {
		BasicBlock<X11IRNode> *bb = *BBiter;

		if(bb->getId() == 0)
			continue;

		uint32_t my_vpisc(bb->getProperty<uint32_t>("vpisc"));

		list<BasicBlock<CFG::IRType>::node_t *> &succ(bb->getSuccessors());
		list<BasicBlock<CFG::IRType>::node_t *>::iterator u;

		for(u = succ.begin(); u != succ.end(); ++u) {
			BasicBlock<CFG::IRType> *bb2 = (*u)->NodeInfo;

			uint32_t old_vpisc(bb2->getProperty<uint32_t>("vpisc"));

			cout << "PISC predecessore [" << bb->getId() << "]: " << my_vpisc << ", PISC successore[" << bb2->getId() << "]: " << old_vpisc << '\n';

			if(my_vpisc != old_vpisc)
				bb2->setProperty<bool>("root", true);
		}
	}

	delete bblist;

	return;
}

void X11EmissionContext::cull_trailing_jumps()
{
	list<BasicBlock<X11IRNode> *> *bblist(cfg.getBBList());
	list<BasicBlock<X11IRNode> *>::iterator BBiter;

	for(BBiter = bblist->begin(); BBiter != bblist->end(); ++BBiter ) {
		BasicBlock<X11IRNode> *bb = *BBiter;

		if (bb->getId() == 0)
			continue;

		/* We want to work from end to beginning, but erase does not support reverse
		 * iterators, se we reverse the whole list */
		list<CFG::IRType *> &code_list(bb->getCode());
		list<CFG::IRType *>::iterator r;
		code_list.reverse();

		for(r = code_list.begin(); r != code_list.end() && (*r)->is_branch_always(); ++r) {
			bb->setProperty<uint32_t>("exit_to", *((*r)->get_dest().begin()));
		}

		/* erase from r to the end of the jump sequence */
		code_list.erase(code_list.begin(), r);

		code_list.reverse();
	}

	return;
}



void X11EmissionContext::run()
{
	/* Preliminary step to annotate and modify slightly the CFG */
	init_sequences();
	cull_trailing_jumps();
	number_vpiscs();

	out << "----- BEGIN ASM LISTING -----\n";

	list<BasicBlock<X11IRNode> *> *bblist(cfg.getBBList());
	list<BasicBlock<X11IRNode> *>::iterator BBiter;

	for(BBiter = bblist->begin(); BBiter != bblist->end(); ++BBiter ) {
		BasicBlock<X11IRNode> *bb = *BBiter;
		if (bb->getId() == 0)
			continue;

		// Find our name
		string new_sequence("L" + stringify(bb->getId()));

		// Find our VPISC block name
		set_vpisc("VPISC" + stringify(bb->getProperty<uint32_t>("vpisc")));

		// Decide if we are root or not
		bool is_root(bb->getProperty<bool>("root"));

		// Find the successor's name
		string succ("end");
		uint32_t successor(bb->getProperty<uint32_t>("exit_to"));
		if((int)successor > 0)
			succ = make_label(stringify(successor));

		// Emit the actual code
		process(bb->getCode().begin(), bb->getCode().end(), new_sequence,
				succ, is_root);
	}

	dump_drivers();

	out << "----- END ASM LISTING -----\n\n";

	// Emit the header
	X11Header header;
	out << "----- BEGIN HEADERS LISTING -----\n";
	header.dump_header(out);
	out << "----- END HEADERS LISTING -----\n\n";

	// Emit the TCAM entries
	out << "----- BEGIN TCAM CONFIGURATION LISTING -----\n";
	get_tcam().dump(out);
	out << "----- END TCAM CONFIGURATION LISTING -----\n";

	return;
}

} /* namespace X11 */
} /* namespace jit */
