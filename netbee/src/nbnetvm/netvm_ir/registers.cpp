#include "registers.h"
#include <iostream>
#include <cassert>
using namespace std;
using jit::RegisterModel;
using jit::RegisterInstance;
using jit::RegisterManager;


// We must adopt 'constructor on first use' for latest_name, models and proxies
// This because the constructor of RegisterModel use these methods and we must be sure the
// instantiation of the following is performed at the first use of the constructor
// Declaring them as static variable make the compiler to instanciate them in ARBITRARY ORDER
// http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.13

map<uint32_t, uint32_t> &RegisterModel::latest_name()
{
	static map<uint32_t, uint32_t> *latest_name_ = new map<uint32_t, uint32_t>();
	return *latest_name_;
}

map<uint32_t, std::map<uint32_t, RegisterModel::_model *> > &RegisterModel::models()
{
	static map<uint32_t, std::map<uint32_t, RegisterModel::_model *> > *models_ = new map<uint32_t, std::map<uint32_t, RegisterModel::_model *> >();
	return *models_;
}

map<uint32_t, std::map<uint32_t, RegisterModel::_proxy *> > &RegisterModel::proxies() {
	static map<uint32_t, std::map<uint32_t, RegisterModel::_proxy *> > *proxies_ = new map<uint32_t, std::map<uint32_t, RegisterModel::_proxy *> >();
	return *proxies_;
}

RegisterModel::_model::_model(uint32_t space, uint32_t name) : reg_space(space), reg_name(name)
{
	// Nop
	return;
}

RegisterModel::_proxy::_proxy(_model *m) : model(m)
{
	// Nop
	return;
}

RegisterModel::RegisterModel(uint32_t _space, uint32_t _name) 
{
	// Every register model with the same space & name share the same proxy
	if(!(proxy = proxies()[_space][_name])) {
		_model *m;
		
		// Every proxy with the same space & name point to the singleton model
		if(!(m = models()[_space][_name])) {
			m = models()[_space][_name] = new _model(_space, _name);
		}
		
		proxy = proxies()[_space][_name] = new _proxy(m);
	}
	
	assert(proxy != 0);

	return;
}

RegisterModel::~RegisterModel()
{
	// Nop
	return;
}

RegisterModel *RegisterModel::get(uint32_t reg_space, uint32_t reg_name)
{
	RegisterModel *r(new RegisterModel(reg_space, reg_name));
	latest_name()[reg_space] = max(reg_name, latest_name()[reg_space]);	
	return r;
}

RegisterModel *RegisterModel::get_new(uint32_t reg_space)
{
	RegisterModel *rg(get(reg_space, ++latest_name()[reg_space]));
//	cout << "Nuovo modello: " << rg->get_space() << "." << rg->get_name() << '\n';
	
	assert(rg != 0);
	return rg;
}

void RegisterModel::rename(uint32_t new_space, uint32_t new_name)
{
	_model *new_model;
	if(!(new_model = models()[new_space][new_name])) {
		new_model = models()[new_space][new_name] = new _model(new_space, new_name);
	}

	// Change the used model for _every_ register that shares our proxy
	proxy->model = new_model;

	latest_name()[new_space] = max(latest_name()[new_space], new_name);
	return;
}

void RegisterInstance::init_instance(RegisterModel *_model, uint32_t __version)
{
	//assert(_model != 0);
	model = _model;
	_version = __version;
	return;
}

RegisterInstance::RegisterInstance(uint32_t space, uint32_t name, uint32_t version)
{
	init_instance(RegisterModel::get(space, name), version);
	return;
}

RegisterInstance::RegisterInstance(RegisterModel *model, uint32_t version)
{
	init_instance(model, version);
	return;
}

RegisterInstance::RegisterInstance()
{
	init_instance(invalid.model, invalid.version());
	return;
}

uint32_t RegisterModel::get_latest_name(uint32_t space)
{
	return latest_name()[space];
}

void RegisterModel::reset(uint32_t space)
{
	{
		map<uint32_t, _proxy *>::iterator i;

		for(i = proxies()[space].begin(); i != proxies()[space].end(); i++)
		{
			if(space != 0 && i->first != 0)
				delete i->second;
		}

		if(space != 0)
		{
			proxies()[space].clear();
		}
		else
		{
			_proxy *invalid(proxies()[0][0]);
			proxies()[space].clear();
			proxies()[0][0] = invalid;
		}
	}

	{
		map<uint32_t, _model *>::iterator i;
		for(i = models()[space].begin(); i != models()[space].end(); i++)
		{
			if(space != 0 && i->first != 0)
				delete i->second;
		}

		if(space != 0)
		{
			models()[space].clear();
		}
		else
		{
			_model *invalid(models()[0][0]);
			models()[space].clear();
			models()[0][0] = invalid;
		}
	}

	latest_name()[space] =  0;
}


void RegisterModel::reset()
{
	// Clear the proxy map
	{
		map<uint32_t, map<uint32_t, _proxy *> >::iterator i;
		map<uint32_t, _proxy *>::iterator j;
	
		for(i = proxies().begin(); i != proxies().end(); ++i)
			for(j = i->second.begin(); j != i->second.end(); ++j)
				if(j->first != 0 && i->first != 0)	 // XXX: do note erase (0, 0)
					delete j->second;
	
		_proxy *invalid(proxies()[0][0]);
		proxies().clear();
		proxies()[0][0] = invalid;
	}
	
	// Clear the models() map
	{
		map<uint32_t, map<uint32_t, _model *> >::iterator i;
		map<uint32_t, _model *>::iterator j;
		
		for(i = models().begin(); i != models().end(); ++i)
			for(j = i->second.begin(); j != i->second.end(); ++j)
				if(j->first != 0 && i->first != 0)
					delete j->second;

		_model *invalid(models()[0][0]);
		models().clear();
		models()[0][0] = invalid;
	}

//	invalid = RegisterModel(0, 0);
//	RegisterInstance::invalid = RegisterInstance(0, 0, 0);
	// Finally, clear the latest_name() map so that counting starts again from 0
	latest_name().clear();

	return;
}

RegisterInstance RegisterInstance::get_new(uint32_t reg_space)
{
	return RegisterInstance(RegisterModel::get_new(reg_space));
}

const RegisterInstance	jit::RegisterInstance::invalid(0, 0, 0);

const RegisterModel		jit::RegisterModel::invalid(0, 0);

std::ostream & std::operator<<(std::ostream &out, const jit::RegisterInstance &r)
{
	out << "r" << r.get_model()->get_space() << "." << r.get_model()->get_name() << "."
		<< r.version();
	return out;
}

std::ostream & std::operator<<(std::ostream &out, const jit::RegisterModel &r)
{
	out << "r" << r.get_space() << "." << r.get_name();
	return out;
}

RegisterManager::RegisterManager(uint32_t output_s, std::set<uint32_t> ignore_s) : count(0),
		output_space(output_s), ignore_set(ignore_s)
{
	// Nop
	return;
}

uint32_t RegisterManager::getCount() const {
	return count;
}

void RegisterManager::addNewRegister(RegisterManager::RegType r) 
{
	if(regToInt.find(r) != regToInt.end())
		// Alias already present
		return;

	if(ignore_set.find(r.get_model()->get_space()) != ignore_set.end()) {
		//cout << r << " is to be ignored.\n";
		//intToReg[r] = r;
		regToInt[r] = r;
	} else {
		RegisterInstance name(output_space, count++, 0);
		
/*		Taggable *nn(&name), *rr(&r);
		*nn = *rr; */
		
		//cout << "mapping " << r << " to " << name << '\n';
		intToReg.push_back(r);
		regToInt[r] = name;
	}
	return;
}

RegisterManager::RegType RegisterManager::operator[](RegisterManager::RegType name) {
	if(ignore_set.find(name.get_model()->get_space()) != ignore_set.end())
		return name;

	return intToReg[name.get_model()->get_name()];
}

RegisterManager::RegType RegisterManager::getName(const RegisterManager::RegType& r) {
	// Do not update the map interactively, now the 2-pass mode is used
	if(regToInt.find(r) == regToInt.end())
		return r;

//		addNewRegister(r);
		
	assert(!(regToInt[r] == RegisterManager::RegType::invalid));
		
	return regToInt[r];
}


