/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "x11-coprocessor.h"
#include "../application.h"

#include <list>
#include <string>
#include <set>
#include <cassert>

using namespace std;

namespace jit {
namespace X11 {

void Coprocessor::emit(CopMIRNode *n, BasicBlock &bb, X11EmissionContext *ctx)
{
	// Look up the coprocessor name to see if we support it
	nvmCoprocessorState *cop(n->get_state());
	
	string name(cop->name);
	
	if(name == "lookupnew") {
		// We got games
		do_lookup(n, bb, ctx);
	} else {
		assert(1 == 0 && "A required coprocessor is unsupported!");
	}

	return;
}

unsigned Coprocessor::table(0);

void Coprocessor::do_lookup(CopMIRNode *n, BasicBlock &BB, X11EmissionContext *ctx)
{
	// Get a table
	// TODO: save the table somewhere so we can retrieve it when we need (e.g. to fill it!)
	if(table == 0)
		table = ctx->get_tcam().new_table()+1;

	// Reuse the same code for the switch instruction :-)
	
	// Load the prefix (table number) into the helper.
	RegType helperReg(X11::REG_SPACE(sp_lk_table), table);
	helperReg.setProperty<operand_size_t>("size", byte8);
	RegType tableReg(X11::REG_SPACE(sp_const), table);
	tableReg.setProperty<operand_size_t>("size", byte8);
	
	BB.addNewInstruction(new X11::Copy(BB.getId(), helperReg, tableReg));
	
	// Now load the lookup keys into the proper registers
	RegType srcReg0(X11::REG_SPACE(sp_virtual), n->getCoproReg(0, true).get_model()->get_name());
	srcReg0.setProperty<operand_size_t>("size", size(n->getCoproReg(0, true)));
	RegType srcReg1(X11::REG_SPACE(sp_virtual), n->getCoproReg(1, true).get_model()->get_name());
	srcReg1.setProperty<operand_size_t>("size", size(n->getCoproReg(1, true)));

	// Space used here: space lookup helper
	RegType lkReg0(X11::REG_SPACE(sp_lk_key64),helperReg.get_model()->get_name());
	lkReg0.setProperty<operand_size_t>("size", dword32);
	lkReg0.setProperty<reg_part_t>("part", r_result_0);

	RegType lkReg1(X11::REG_SPACE(sp_lk_key64),helperReg.get_model()->get_name());
	lkReg1.setProperty<operand_size_t>("size", dword32);
	lkReg1.setProperty<reg_part_t>("part", r_result_1);

	BB.addNewInstruction(new X11::Copy(BB.getId(), lkReg0, srcReg0));
	BB.addNewInstruction(new X11::Copy(BB.getId(), lkReg1, srcReg1));
	
	// Allocate somewhere to hold the result
	RegType dstReg0(RegisterModel::get_new(X11::REG_SPACE(sp_lk_result)));
	// We assume that the size will be the one of the defined registers of this coprocessors
	dstReg0.setProperty<operand_size_t>("size",	size(n->getCoproReg(5, false)));
	assert(dstReg0.getProperty<operand_size_t>("size") != 0);
	
	set<RegType> uses;
	uses.insert(helperReg);
	uses.insert(lkReg0);
	
	// Emit the prepare instruction
	BB.addNewInstruction(new X11::Prepare(BB.getId(), "lookup_" + stringify(table),
		dstReg0, uses, false));
		
	// Here we must load the proper registers with the lookup results
	/* The result is made of 2 parts. The first one is a 8 bit value that is not 0 if there's a 
	 * match. It must go into resReg1 (which is always 8 bits)
	 * The second part is a value as long as it is needed. It goes into resReg0.
	 */
	RegType	resReg0(X11::REG_SPACE(sp_virtual), n->getCoproReg(5, false).get_model()->get_name());
	resReg0.Taggable::operator=(n->getCoproReg(5, false));
	
	RegType resReg1(X11::REG_SPACE(sp_virtual), n->getCoproReg(7, false).get_model()->get_name());
	resReg1.Taggable::operator=(n->getCoproReg(7, false));
	resReg1.setProperty<operand_size_t>("size", word16);


//	list<BasicBlock::node_t *> &successors(BB.getSuccessors());
//	assert(successors.size() == 1 && "Coprocessor accesses with multiple successors not supported");
//	list<BasicBlock::node_t *>::iterator i(successors.begin());
//	BasicBlock *bb2((*i)->NodeInfo);
	
	dstReg0.setProperty<reg_part_t>("part", r_result_1);
//	bb2->addNewInstruction(new X11::Copy(bb2->getId(), resReg0, dstReg0));
	BB.addNewInstruction(new X11::Copy(BB.getId(), resReg0, dstReg0));
	
	dstReg0.setProperty<reg_part_t>("part", r_result_0);
	dstReg0.setProperty<operand_size_t>("size", word16);
//	bb2->addNewInstruction(new X11::Copy(bb2->getId(), resReg1, dstReg0));
	BB.addNewInstruction(new X11::Copy(BB.getId(), resReg1, dstReg0));
	
//	cout << "Dichiarazione di dstReg0: " << X11ResultRegister::make(dstReg0).get_decl() << '\n';
	
	return;
}


void DataMemory::add(uint16_t con, uint16_t address, RegType low, RegType high, BasicBlock &bb,
	X11EmissionContext *ctx)
{
	// Get the base address of this PE.
	uint16_t base((uint32_t)Application::getApp(bb).getMemDescriptor(Application::data).Base);
	
	/* The X11 is able to address 64 bits at a time only. This means that we have to shift the
	 * con value so that when adding it to the data found in memory the expected thing happens.
	 * Overflows yield an undefined result */

	/* For now we just consider every offset to refer to a different 64 bit memory word */
	
	// Get the real address
	base += address;

	// Emit the prepare for the correct driver
	RegType addrReg(RegisterModel::get_new(X11::REG_SPACE(sp_virtual)));
	RegType cnstReg(RegisterModel::get_new(X11::REG_SPACE(sp_virtual))); 
	addrReg.setProperty<operand_size_t>("size", word16);
	cnstReg.setProperty<operand_size_t>("size", word16);
	RegType addrReg0(RegisterInstance(X11::REG_SPACE(sp_const), base));
	RegType cnstReg0(RegisterInstance(X11::REG_SPACE(sp_const), con));
	RegType resReg(RegisterModel::get_new(X11::REG_SPACE(sp_virtual)));
	resReg.setProperty<operand_size_t>("size", qword64);
	set<RegType> src;
	src.insert(cnstReg);
	
	// Load the address and constant registers with the appropriate values
	bb.addNewInstruction(new Copy(bb.getId(), addrReg, addrReg0));
	bb.addNewInstruction(new Copy(bb.getId(), cnstReg, cnstReg0));
	
	// FIXME: do not use a TCAM table entry!!!
	uint32_t table(ctx->get_tcam().new_table()+1);
	
	Prepare *prepare(new Prepare(bb.getId(), stringify("add16_") + stringify(table), resReg, src, addrReg,
		"LA0", "Add16"));
	set<RegType> uses;
	uses.insert(addrReg);
	prepare->setExtraUses(uses);
	
	uses = prepare->getUses();
	assert(uses.find(cnstReg) != uses.end());
	
	bb.addNewInstruction(prepare);

	// Copy the results to the correct variables
	resReg.setProperty<operand_size_t>("size", dword32);
	resReg.setProperty<reg_part_t>("part", r_result_1);
	bb.addNewInstruction(new Copy(bb.getId(), high, resReg));
	
	// Subtraction has to be divided into 2 parts
	low.setProperty<reg_part_t>("part", r_16lo);
	high.setProperty<reg_part_t>("part", r_16lo);
	bb.addNewInstruction(new Sub(bb.getId(), low, high, cnstReg));
	
	low.setProperty<reg_part_t>("part", r_16hi);
	high.setProperty<reg_part_t>("part", r_16hi);
	bb.addNewInstruction(new Subc(bb.getId(), low, high, cnstReg));

	return;
}



} /* namespace X11 */
} /* namespace jit */
