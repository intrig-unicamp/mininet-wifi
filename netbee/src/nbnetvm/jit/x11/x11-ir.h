/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef __X11_IR_H__
#define __X11_IR_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <iostream>

#include "registers.h"
#include "irnode.h"
#include "basicblock.h"

namespace jit {
namespace X11 {

// Forward declaration to avoid circular inclusions - Pierluigi
class X11EmissionContext;

class Phi;
class Copy;
class X11IRNode;

class X11IRNode : public IRNode<RegisterInstance /*, X11X11EmissionContext */> {
  protected:
	class _iterator_if {
	  public:	
		virtual _iterator_if *clone() const = 0;	
		virtual ~_iterator_if() {};
		
		virtual X11IRNode *operator->() = 0;
		virtual X11IRNode *operator*() = 0;
		
		virtual _iterator_if &operator++() = 0;
		virtual _iterator_if &operator++(int) = 0;
	
		virtual bool operator!=(const _iterator_if &) = 0;

		
//		friend class OverwriteAnalysisWrap::_fake_iterator;
	};
	
  private:
	/* Dummy iterator class to satisfy the SSA algorithm interface */
	class _iterator {
	  private:
		_iterator_if *node;
		
	  public:
		_iterator(_iterator_if *);
		virtual ~_iterator();
		
		virtual X11IRNode *operator->();
		virtual X11IRNode *operator*();
		
		virtual _iterator &operator++();
		virtual _iterator &operator++(int);
	
		virtual bool operator!=(const _iterator &);
		
		_iterator &operator=(const _iterator &);
		_iterator(const _iterator &);
	};

	class _iterator_impl : public _iterator_if {
	  protected:
		X11IRNode *node;
		bool end;
		
		virtual _iterator_impl *clone() const;
	
	  public:
		_iterator_impl(X11IRNode *node);
		virtual ~_iterator_impl();
		
		virtual X11IRNode *operator->();
		virtual X11IRNode *operator*();
		
		virtual _iterator_impl &operator++();
		virtual _iterator_impl &operator++(int);
	
		virtual bool operator!=(const _iterator_if &);
	};

  protected:
	static bool overwrite_analysis;
	
	void kill_constants(std::set<RegType> &);
	
  private:
	bool merge;

  public:
	typedef uint16_t BBId_t;
	
	X11IRNode(BBId_t);
	virtual ~X11IRNode();
	
	// ----- Interface for merging -----
	
	virtual void set_merge(bool);
	virtual bool get_merge() const;
	
	typedef enum {
		alu_u, load_u, copy_u, branch_u
	} unit_t;
	
	virtual std::set<unit_t> get_units();
	
	// ------ THIS INTERFACE IS REQUIRED FOR THE SSA ALGORITHMS -------------------------
	typedef _iterator IRNodeIterator;
	virtual _iterator nodeBegin();
	virtual _iterator nodeEnd();
	
	virtual void setDefReg(RegType r);
	virtual RegType *getOwnReg();
	virtual RegType getDefReg();
	
	typedef class Phi PhiType; //!< Required interface for the SSA algorithm.
	static Phi *make_phi(uint16_t, RegType);
	virtual bool isPhi() const;

	typedef Copy CopyType;	//!< Type representing a copy operation
	static Copy *make_copy(uint16_t, RegType src, RegType dst); //!< ctor for a node that implements a register-to-register copy
			
//	virtual void rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id);
	virtual void rewrite_use(RegType old, RegType version);
			
	//! Needed for exit from SSA - true if this is a control flow operation, of any type
	virtual bool isJump() const;
	// ------ END OF INTERFACE REQUIRED BY THE SSA ALGORITHMS ---------------------------
			
	// ------ THIS INTERFACE IS REQUIRED FOR THE COPY FOLDING ALGORITHM -----------------
	typedef std::list<X11IRNode *> ContType;
	ContType get_reverse_instruction_list();
	virtual bool isCopy(bool is_assignement = false) const;
	// ------ END OF INTERFACE REQUIRED BY THE COPY FOLDING ALGORITHM -------------------
	
	// ------ THIS INTERFACE IS REQUIRED FOR THE DEAD CODE ELIMINATION ALGORITHM --------
	virtual std::list<std::pair<RegType,RegType> > getCopiedPair(bool assignement = false);
	// ------ END OF INTERFACE REQUIRED BY THE DEAD CODE ELIMINATION ALGORITHM ----------

	virtual X11IRNode * getTree() { return this;}

	virtual std::set<BBId_t> get_dest();
	virtual bool is_branch_always() const;

	virtual void rewrite_aliases(std::map<RegType, RegType> &);	
	virtual std::set<std::pair<RegType, RegType> > get_aliases();

	virtual std::string to_string() const = 0;
	
	virtual bool end_sequence() const;
	
	virtual std::ostream& printNode(std::ostream& os, bool) = 0;
	virtual void emit(X11EmissionContext *ec) = 0;

	virtual std::set<RegType> getAllUses();
	virtual std::set<RegType> getAllDefs();
	
	virtual std::set<RegType> getSources();
	
	static void enable_overwrite_analysis_mode(bool enable);
};

//typedef Sequence<X11IRNode, X11X11EmissionContext>	X11IRSequence;

class Phi : public X11IRNode {
  private:
	RegType dst;
	std::map<uint16_t, RegType> params;
  public:
	Phi(BBId_t, RegType);
	virtual ~Phi();

  	virtual std::ostream& printNode(std::ostream& os, bool);

	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();	

	virtual std::string to_string() const {
		return "phi";
	};

	virtual void emit(X11EmissionContext *ec);

	// ------ THIS INTERFACE IS REQUIRED FOR THE SSA ALGORITHMS -------------------------
	virtual bool isPhi() const;
	typedef std::map<uint16_t, RegType>::iterator params_iterator;
	
	params_iterator paramsBegin();
	params_iterator paramsEnd();
	void addParam(RegType param, uint16_t bb);
	
	virtual void setDefReg(RegType r);
	virtual RegType *getOwnReg();
	
	RegType getDefReg() const;
	// ------ END OF INTERFACE REQUIRED BY THE SSA ALGORITHMS ---------------------------
	
	// ------ THIS INTERFACE IS REQUIRED FOR THE DEAD CODE ELIMINATION ALGORITHM --------
	virtual std::list<std::pair<RegType,RegType> > getCopiedPair(bool assignement = false);
	// ------ END OF INTERFACE REQUIRED BY THE DEAD CODE ELIMINATION ALGORITHM ----------
};

/* Mimics the behaviour of another class but every reg. that the wrapped class defines
 * is also considered to be used by this class -> keep track of reaching definitions.
 */
class OverwriteAnalysisWrap : public X11IRNode {
  private:
  	X11IRNode *real;
  
	class FakeNode : public X11IRNode {
	  private:
		std::map<RegisterModel, RegisterInstance> my_uses;
		
	  public:	
		FakeNode(BBId_t, X11IRNode *r);	
		virtual ~FakeNode();
	
		virtual void setDefReg(RegType r);
		virtual RegType *getOwnReg();
	
		virtual void rewrite_use(RegType old, RegType version);
	
		virtual std::set<RegType> getUses();
		virtual std::set<RegType> getDefs();
	
		virtual void emit(X11EmissionContext *ec);
	
		virtual std::ostream& printNode(std::ostream& os, bool);
	
		virtual std::string to_string() const {
			return "fake_node";
		}
	} fake_node;
	
	class _fake_iterator : public _iterator_if {
	  private:
		uint8_t counter;
		OverwriteAnalysisWrap *node;
		
	  protected:
		virtual _fake_iterator *clone() const;
		
	  public:
		_fake_iterator(OverwriteAnalysisWrap *node);
		virtual ~_fake_iterator();
		
		virtual X11IRNode *operator->();
		virtual X11IRNode *operator*();

		virtual _fake_iterator &operator++();
		virtual _fake_iterator &operator++(int);
	
		virtual bool operator!=(const _iterator_if &);
	};
	
	friend class _fake_iterator;
	
  public:
	OverwriteAnalysisWrap(BBId_t, X11IRNode *r);	
	virtual ~OverwriteAnalysisWrap();
	
	virtual IRNodeIterator nodeBegin();
	virtual IRNodeIterator nodeEnd();
	
	virtual void setDefReg(RegType r);
	virtual RegType *getOwnReg();
	
	virtual void rewrite_use(RegType old, RegType version);
	
	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();
	
	virtual std::set<RegType> getAllUses();
	virtual std::set<RegType> getAllDefs();
	
	std::set<RegType> getReachingDefs();
	X11IRNode *getReal();

	virtual void emit(X11EmissionContext *ec);
	
  	virtual std::ostream& printNode(std::ostream& os, bool);
	
	virtual std::string to_string() const {
		return "oaw | " + real->to_string();
	}
};

/* Define the four VLIW units */

class Alu : public X11IRNode {
  protected:
	virtual unsigned get_opcount();
	std::vector<RegType> kids;
	std::set<RegType> extra_uses;
	
	virtual std::string get_opcode() const = 0;
	
  public:
	Alu(BBId_t bb_id, RegType res, RegType left_op, RegType right_op);
	Alu(BBId_t bb_id, RegType res, RegType left_op);
	virtual ~Alu();
	
	virtual std::set<unit_t> get_units();
	
	virtual void setDefReg(RegType r);
	virtual RegType *getOwnReg();
	
	virtual bool has_side_effects();
	
	virtual void rewrite_use(RegType old, RegType version);
	
	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();

	virtual void emit(X11EmissionContext *ec);
	
  	virtual std::ostream& printNode(std::ostream& os, bool);
};

class Copy : public X11IRNode {
  protected:
	RegType src;
	RegType dst;
	std::set<RegType> helpers;

  public:
	Copy(BBId_t, RegType dst, RegType src);
	virtual ~Copy();
	
	virtual std::set<unit_t> get_units();
	
	virtual void setDefReg(RegType r);
	virtual RegType *getOwnReg();
	
	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();
	virtual std::set<RegType> getSources();
	
	// ------ THIS INTERFACE IS REQUIRED FOR THE DEAD CODE ELIMINATION ALGORITHM --------
	virtual std::list<std::pair<RegType,RegType> > getCopiedPair(bool assignement = false);
	virtual bool has_side_effects();
	// ------ END OF INTERFACE REQUIRED BY THE DEAD CODE ELIMINATION ALGORITHM ----------

	virtual void rewrite_use(RegType old, RegType version);

	virtual bool isCopy(bool is_assignement = false) const;

	virtual void rewrite_aliases(std::map<RegType, RegType> &);	
	virtual std::set<std::pair<RegType, RegType> > get_aliases();
	
	virtual void emit(X11EmissionContext *ec);
	
	virtual std::string to_string() const {
		return "mov";
	};
	
  	virtual std::ostream& printNode(std::ostream& os, bool);
};

class Load : public X11IRNode {
  protected:
	RegType src;
	RegType dst;

  public:
	Load(BBId_t bb_id, RegType src, RegType dst);
	virtual ~Load();
	
	virtual std::set<unit_t> get_units();

	virtual void setDefReg(RegType r);
	virtual RegType *getOwnReg();

	// ------ THIS INTERFACE IS REQUIRED FOR THE DEAD CODE ELIMINATION ALGORITHM --------
	virtual std::list<std::pair<RegType,RegType> > getCopiedPair(bool assignement = false);
	// ------ END OF INTERFACE REQUIRED BY THE DEAD CODE ELIMINATION ALGORITHM ----------
	
	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();
	virtual std::set<RegType> getSources();
	
	virtual void rewrite_use(RegType old, RegType version);
	
//	virtual bool has_side_effects() {
//		return true;
//	}

	virtual void emit(X11EmissionContext *ec);
	
	virtual bool isCopy(bool) const;

	virtual std::string to_string() const {
		return "load";
	};
	
  	virtual std::ostream& printNode(std::ostream& os, bool);
};

/* FIXME: we need to keep track if this is a jump or not! */
class Prepare : public X11IRNode {
  private:
	std::string driver;
	RegType dst;
	std::set<RegType> src;
	bool is_jump;
	RegType addr;
	std::string engine;
	std::string op;
	std::set<RegType> extra_uses;

  public:
	Prepare(BBId_t bb, std::string driver, RegType dst, std::set<RegType> src, bool is_jump = true);
	Prepare(BBId_t bb, std::string driver, RegType dst, std::set<RegType> src, RegType addr,
		std::string engine, std::string op, bool is_jump = false);
	virtual ~Prepare();
	
	virtual void setDefReg(RegType r);
	virtual RegType *getOwnReg();
	
	virtual std::set<unit_t> get_units();
	
	virtual bool has_side_effects();
	
	virtual	bool isJump() const;
	
	void setExtraUses(std::set<RegType> );
	
	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();
	
	virtual void rewrite_use(RegType old, RegType version);

	virtual void emit(X11EmissionContext *ec);
	
	virtual bool end_sequence() const;

	virtual std::string to_string() const {
		return "prepare";
	};
	
  	virtual std::ostream& printNode(std::ostream& os, bool);
};

enum branch_condition_t {
	always, equal, not_equal, not_zero
};

class Branch : public X11IRNode {
  protected:
	enum branch_condition_t condition;
	BBId_t dst_label;

  public:
	Branch(BBId_t bb, BBId_t dst, enum branch_condition_t condition = always);
	virtual ~Branch();
	
	virtual std::set<unit_t> get_units();

//	virtual void rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id);

	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();
	virtual std::set<BBId_t> get_dest();
	
//	virtual void rewrite_use(RegType old, uint32_t version);
	
	virtual bool isJump() const;
	virtual bool is_branch_always() const;
	
	virtual void emit(X11EmissionContext *ec);
	
	virtual std::string to_string() const {
		return "branch";
	};
	
  	virtual std::ostream& printNode(std::ostream& os, bool);
};


/* Define specific operations within each unit */

class Not : public Alu {
  protected:
	virtual std::string get_opcode() const;

  public:
	Not(BBId_t, RegType res, RegType op0);
	virtual ~Not();

	virtual std::string to_string() const {
		return "not";
	}
};

enum boolean_t {
	alu_and, alu_or, alu_xor
};

class Boolean : public Alu {
  private:
	boolean_t op_type;

  protected:
	virtual std::string get_opcode() const;

  public:
	Boolean(BBId_t, RegType res, RegType op0, RegType op1, boolean_t op_t);
	virtual ~Boolean();
	
	virtual std::string to_string() const {
		return get_opcode();
	}
};

class Add : public Alu {
 protected:
 	virtual std::string get_opcode() const;
 
  public:
	Add(BBId_t, RegType res, RegType op0, RegType op1);
	virtual ~Add();
	
	virtual std::string to_string() const {
		return "add";
	};
};

class Addc : public Alu {
  protected:
	virtual std::string get_opcode() const;

  public:	
	Addc(BBId_t, RegType res, RegType op0, RegType op1);
	virtual ~Addc();
	
	virtual std::string to_string() const {
		return "addc";
	};
};

class Subc : public Alu {
  protected:
	virtual std::string get_opcode() const;

  public:	
	Subc(BBId_t, RegType op0, RegType op1, RegType res);
	virtual ~Subc();
	
	virtual std::string to_string() const {
		return "subc";
	};
};

class Sub : public Alu {
  protected:
	virtual std::string get_opcode() const;

  public:
	Sub(BBId_t, RegType res, RegType op0, RegType op1);
	virtual ~Sub();
	
	virtual std::string to_string() const {
		return "sub";
	};
};

enum shift_direction_t {
	left, right
};

class Shift : public Alu {
  private:
	shift_direction_t dir;

  protected:
	virtual std::string get_opcode() const;

  public:
	Shift(BBId_t, RegType dst, RegType src0, RegType src1, shift_direction_t dir);
	virtual ~Shift();
	
	virtual std::string to_string() const {
		return get_opcode();
	};
};

class Compare : public Alu {
 protected:
 	virtual std::string get_opcode() const;
 
  public:
	Compare(BBId_t, RegType op1, RegType op2);
	virtual ~Compare();
	
	virtual std::set<RegType> getUses();
	virtual std::set<RegType> getDefs();

	virtual std::string to_string() const {
		return "cmp";
	};	
};

} /* namespace X11 */
} /* namespace jit */

#endif /* __X11_IR_H__ */
