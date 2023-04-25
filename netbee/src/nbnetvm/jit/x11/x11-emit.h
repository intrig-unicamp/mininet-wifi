/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __X11_EMIT_H__
#define __X11_EMIT_H__

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <list>

#include "x11-ir.h"
#include "x11-util.h"
#include "cfg.h"
#include "registers.h"

namespace jit {
namespace X11 {

#if 0
/* Performs the actual instruction emission */
int32_t x11_nvmJitRetargetable_Instruction_Emission(jit::CFG<X11IRNode> *cfg);
#endif

/* Emits a common header that contains structure declarations and so on. */
class X11Header {
  public:
	X11Header();
	~X11Header();
	
	void dump_header(std::ostream &out);
};

/* Represents an X11 variable (which is then assigned to a register by the linker) */
class X11Register {
  protected:
	RegisterInstance number;
	
	std::string get_suffix() const;
//	operand_size_t size;
  public:
	X11Register(RegisterInstance ri);
	virtual ~X11Register();
  
	virtual std::string get_name(bool suffix = true) const;
	virtual std::string get_decl();
	
	virtual bool must_be_declared() const;
	
//	virtual bool operator<(const X11Register &other) const;
	
	static X11Register make(RegisterInstance &);
	static std::string emit(RegisterInstance &, bool suffix = true);
	static std::string emit_decl(RegisterInstance);
};

class X11DeviceRegister : public X11Register {
  public:
	X11DeviceRegister(RegisterInstance ri);
	virtual ~X11DeviceRegister();
	
	virtual std::string get_name(bool suffix = true) const;
	virtual std::string get_decl();
	
	virtual bool must_be_declared() const;
	
	static X11DeviceRegister make(RegisterInstance &);
};

class X11SwitchHelper : public X11Register {
  public:
	X11SwitchHelper(RegisterInstance ri);
	virtual ~X11SwitchHelper();
	
	virtual std::string get_name(bool suffix = true) const;
	std::string get_table() const;
	virtual std::string get_decl();
	
	virtual bool must_be_declared() const;

//	virtual bool operator<(const X11Register &other) const;

	static X11SwitchHelper make(RegisterInstance &);
};

class X11LookupHelper : public X11Register {
  private:
	bool is_hi;
  public:
	X11LookupHelper(RegisterInstance ri);
	virtual ~X11LookupHelper();

	virtual std::string get_name(bool suffix = true) const;
	virtual std::string get_decl();

	virtual bool must_be_declared() const;

	static X11LookupHelper make(RegisterInstance &);
};

class X11OffsetRegister : public X11Register {
  public:
	X11OffsetRegister(RegisterInstance ri);
	virtual ~X11OffsetRegister();
	
	virtual std::string get_name(bool suffix = true) const;
	virtual std::string get_decl();
	
	virtual bool must_be_declared() const;
	
	static X11OffsetRegister make(RegisterInstance &);
};

class X11ResultRegister : public X11Register {
  public:
	X11ResultRegister(RegisterInstance ri);
	virtual ~X11ResultRegister();
	
	virtual std::string get_name(bool suffix = true) const;
	
	virtual std::string get_decl() const;
	
	static X11ResultRegister make(RegisterInstance &);
};



/* Represents an X11 sequence (~ a basic block). */
class X11Sequence {
  private:
	std::string name;
	std::string vpisc;
	bool root;
	std::string exit;
	std::vector<std::string> inst;
	
  public:
	X11Sequence(std::string name, std::string vpisc, bool root);
	~X11Sequence();

	void dump_code(std::ostream &out);
	void link_next(std::string next);

	void add_insn(std::vector<std::string>, bool merge = false);
};

#if 0
class X11Eap
{
};

class X11Vpisc
{
};
#endif

/* Models an X11 TCAM + SRAM */
class X11Tcam
{
  private:
	uint32_t	table;
	uint32_t	address;
	
	struct TcamEntry {
		std::vector<uint8_t> mask;
		std::vector<uint8_t> key;
		std::string value;
		uint32_t address;
		
		TcamEntry(std::vector<uint8_t> mask, std::vector<uint8_t> key, std::string value,
			uint32_t address);
	};
	
	std::vector<TcamEntry> entries;
	
	struct _dump_f {
		std::ostream &out;
		void operator()(TcamEntry t);
		_dump_f(std::ostream &o);
	};
	
  public:
	typedef uint32_t tcam_table_t;
  
	X11Tcam();
	~X11Tcam();
	
	tcam_table_t new_table();
	
	void add_entry(std::vector<uint8_t> mask, std::vector<uint8_t> key, std::string value,
		tcam_table_t table);
	
	template<typename Value>
	void add_entry(Value key, std::string value, tcam_table_t table);

	void dump(std::ostream &);
};

template<typename Value>
void X11Tcam::add_entry(Value key, std::string value, tcam_table_t table)
{
	std::vector<uint8_t> mask, real_key;
	//uint8_t *p = (uint8_t *)&key;
	
	std::cout << "Printing " << (uint32_t) key << '\n';
	
	for(unsigned i = 0; i < sizeof(key); ++i) {
		real_key.push_back((key >> (8*((sizeof(key) - 1) - i))) & 0xFF);
		mask.push_back(0xFF);
	}
	
	add_entry(mask, real_key, value, table);
	
	return;
}

class X11Pipe
{
  private:
	uint32_t eap_count;
	uint32_t vpisc_count;
	
  public:
	X11Pipe(uint32_t eap_count, uint32_t vpisc_count);
	~X11Pipe();
	
	void dump(std::ostream &);
};

class X11Driver
{
  private:
	std::string name;			// Name of this driver
	std::string vpisc;			// VPISC this driver can be called from
	std::string target_name;	// Name of the external engine used by this driver
	std::string op_name;		// Operation name
	std::string result_name;	// Destination operand name
	std::string data0_name;		// Data source operand 0 name
	std::string data1_name;		// Data source operand 1 name
	std::string address_name;	// Address
	
	bool jump;

  public:
	X11Driver(bool jump, std::string name, std::string vpisc, std::string target_name,
		std::string op_name, std::string result_name = "none", std::string data0_name = "none",
		std::string data1_name = "none", std::string address_name = "none");
		
	//std::string &data0();
	//std::string &data1();
	//std::string &address();
	//std::string &result();
	//
	//std::string &sequence();
	
	void dump(std::ostream &) const;
	
	// Needed to insert the driver in a map
	bool operator<(const X11Driver &oth) const;
};


/* Emits an instruction list on a stream */
class X11EmissionContext
{
  public:
	typedef jit::CFG<X11IRNode> CFG;

  private:
	CFG &cfg;
	std::ostream &out;
	std::set<RegisterInstance> seen_vars;
	X11Sequence *active_sequence;
	std::set<RegisterInstance> new_vars;
	
	// Internal TCAM model
	X11Tcam tcam;
	
	// Driver list
	std::set<X11Driver> drivers;
	
	// Current VPISC block
	std::string vpisc;
	
	void init_sequences();
	
	struct _find_accesses {
		bool &accessed;
	
		void operator()(jit::X11::X11IRNode *n) {
//			std::cout << "esamino il nodo ";
//			n->printNode(std::cout, true);
//			std::cout << '\n';

			if(n->end_sequence()) {
				accessed = true;
//				std::cout << "imposto a true\n";
			}
			
			return;
		}
	
		_find_accesses(bool &acc) : accessed(acc) {};
	};

	void number_vpiscs();
	void cull_trailing_jumps();

  public:
	X11EmissionContext(CFG &cfg, std::ostream &);
	~X11EmissionContext();
	
	/* Performs the actual instruction emission */
	void run();

  
	// Instructs the instruction list to emit themselves
	template<typename Iter>
	void process(Iter begin, Iter end, const std::string &name,
		const std::string &default_exit, bool root = false);
	
	// Callback used by the IR nodes to emit themselves
	void emit(std::vector<std::string>, bool merge = false);
	
	X11Tcam &get_tcam();
	
	// Add a driver declaration
	void add_driver(X11Driver);
	
	// Dump all the driver declarations
	void dump_drivers();

	// Get/set the currently emitted sequence
	std::string get_vpisc();
	void set_vpisc(std::string);	
};

template<typename Iter>
void X11EmissionContext::process(
	Iter begin, Iter end, const std::string &name, const std::string &default_exit, bool root)
{
//	std::less<RegisterInstance *> compare_functor;

	// Initialize the sequence
	active_sequence = new X11Sequence(name, get_vpisc(), root);
	new_vars.clear();

	{
		Iter i;	
		for(i = begin; i != end; ++i) {
			std::set<RegisterInstance> uses((*i)->getUses());
			std::set<RegisterInstance> defs((*i)->getDefs());
		
			// Track the used variables
			new_vars.insert(uses.begin(), uses.end());
			
			// Track the defined variables
			new_vars.insert(defs.begin(), defs.end());
			
			(*i)->emit(this);
		}
	}
	
	// Complete the sequence
	active_sequence->link_next(default_exit);
	
	// Emit the new variables listing
	std::set<RegisterInstance> t;
	t = new_vars;
	new_vars.clear();
	
	//// DEBUG: stampa i set.
	//std::set<x11_data *>::iterator it;
	//
	//std::cout << "Set t:\n";
	//for(it = t.begin(); it != t.end(); ++it) {
	//	std::cout << (*it)->emit() << '\n';
	//}
	//
	//std::cout << "\nSet seen_vars:\n";
	//for(it = seen_vars.begin(); it != seen_vars.end(); ++it) {
	//	std::cout << (*it)->emit() << '\n';
	//}

	std::set_difference(t.begin(), t.end(), seen_vars.begin(), seen_vars.end(),
		std::insert_iterator<std::set<RegisterInstance> >(new_vars, new_vars.begin())
			/*, compare_functor*/);
		
	//std::cout << "\nDifferenza:\n";
	//for(it = new_vars.begin(); it != new_vars.end(); ++it) {
	//	std::cout << (*it)->emit() << '\n';
	//}
		
	// Dump the resulting set of variables
	{
		std::set<RegisterInstance>::iterator i;
		for(i = new_vars.begin(); i != new_vars.end(); ++i) {
			std::string s(X11Register::emit_decl(*i));
			if(s.size())
				out << X11Register::emit_decl(*i);
			
#if 0
			switch(i->get_model()->get_space()) {
				case sp_virtual:
					{
						// Build a X11Register from the x11_data and emit the declaration
						X11Register xr(*i);
						out << xr.get_decl();
					}
					break;
				case sp_sw_help:
					{
						X11SwitchHelper xs(*i);
						out << xs.get_decl();
					}
					break;
				default:
					;
			}
#endif
		}

	}
	
	t.clear();
	std::set_union(new_vars.begin(), new_vars.end(), seen_vars.begin(), seen_vars.end(),
		std::insert_iterator<std::set<RegisterInstance> >(t, t.begin()) /*, compare_functor */);
	
	seen_vars = t;
	
	// Emit the sequence
	active_sequence->dump_code(out);
	
	// Destroy it
	delete active_sequence;
	active_sequence = 0;

	return;
}

} /* namespace X11 */
} /* namespace jit */

#endif /* __X11_EMIT_H__ */
