/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#pragma once

#include <map>
#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
#include <cassert>
#include <stdint.h>

#include "taggable.h"

/** @file registers.h
 *	\brief This file contains the classes that implement the NetVM JIT register model.
 */

namespace jit {

/*!
 * \brief This class represents the base, unversioned register name.
 */
class RegisterModel : public Taggable {
  private:
	struct _model {
		_model(uint32_t space, uint32_t name);
	
		uint32_t reg_space; //!< This register's naming space
		uint32_t reg_name;  //!< This register's name (unique within the reg. space)
	};
	
	struct _proxy {
		_proxy(_model *m);
	
		_model *model;
		uint32_t reg_space() const;
		uint32_t reg_name() const;
	};
	
	friend struct _proxy;
	friend struct _model;
	
	static std::map<uint32_t, uint32_t> &latest_name(); //!< last name produced, for every space
	static std::map<uint32_t,
		std::map<uint32_t, _model *> > &models(); //!< every model built ends up in here
	static std::map<uint32_t,
		std::map<uint32_t, _proxy *> > &proxies(); //!< every proxy built ends up in here
		
	_proxy *proxy;
	
	RegisterModel(uint32_t reg_space, uint32_t reg_name);

  public:
	static RegisterModel *get_new(uint32_t reg_space); //!< get a new model in the specified space
	static RegisterModel *get(uint32_t reg_space, uint32_t reg_name); //!< get a new or existing model

	static uint32_t get_latest_name(uint32_t space); //!<get the latest name generated for this space 
	
	static void reset(); //!<reset the register model internal data.
	static void reset(uint32_t space); //!<reset the specified space

	RegisterModel(const RegisterModel &o) :
		Taggable(o), proxy(o.proxy) {
//		std::cout << "----------------------------RegisterModel cctor\n";
		return;
	}
		
	~RegisterModel();
	
	void rename(uint32_t new_name, uint32_t new_space); //!< rename all occurences of this model to another name
	
	uint32_t get_space() const; //!< get this model's space
	uint32_t get_name() const;  //!< get this model's name
	
	const static RegisterModel invalid; //!< invalid register model
	
	bool operator<(const RegisterModel &other) const; //!< comparison between models
	bool operator==(const RegisterModel &other) const; //!< equality between instances
};

/*!
 * \brief This class represents an instance of a register model, w/ a version number.
 */
class RegisterInstance : public Taggable {
  private:
	RegisterModel *model;	//!< pointer to the model this instance refers to
	uint32_t _version;		//!< version of this instance
	
	void init_instance(RegisterModel *model, uint32_t version); //!< helper for constructors

  public:
	typedef RegisterModel ModelType;
	
	static RegisterInstance get_new(uint32_t reg_space);

	RegisterInstance(uint32_t space, uint32_t name, uint32_t version = 0);
	RegisterInstance(RegisterModel *model, uint32_t version = 0);
	RegisterInstance(const RegisterInstance& other) :
		Taggable(other), model(other.model), _version(other._version) {
//		std::cout << "---------------------------------- RegisterInstance cctor\n";
		return;
	}
	RegisterInstance();
	~RegisterInstance() {};
	
	RegisterModel *get_model();
	const RegisterModel *get_model() const;
	uint32_t &version();
	const uint32_t &version() const;
	
	RegisterInstance operator=(RegisterInstance other);	//!< rewrite with a different register
	
	bool operator<(const RegisterInstance &other) const; //!< comparison between instances
	bool operator==(const RegisterInstance &other) const; //!< equality between instances
	bool operator!=(const RegisterInstance &other) const; //!< equality between instances
	
	const static RegisterInstance invalid; //!< get a default, invalid register
};

} /* namespace jit */

namespace std {

std::ostream & operator<<(std::ostream &, const jit::RegisterInstance &);
std::ostream & operator<<(std::ostream &, const jit::RegisterModel &);

/*!
 \brief Comparison between 2 models. Needed to be able to put them into a set or map.
 */
template<>
struct less<jit::RegisterModel *> :
	binary_function<jit::RegisterModel *, jit::RegisterModel *, bool>
{
    bool operator()(const jit::RegisterModel* a, const jit::RegisterModel * b) const {
		return *a < *b;
	};
};

/*!
 \brief Comparison between 2 instances. Needed to be able to put them into a set or map.
 */
template<>
struct less<jit::RegisterInstance *> :
	binary_function<jit::RegisterInstance *, jit::RegisterInstance *, bool>
{
    bool operator()(const jit::RegisterInstance* a, const jit::RegisterInstance * b) const {
		return *a < *b;
	};
};

template<>
struct less<jit::RegisterInstance > :
	binary_function<jit::RegisterInstance, jit::RegisterInstance, bool>
{
    bool operator()(const jit::RegisterInstance a, const jit::RegisterInstance b) const {
//		std::cout << a << " < " << b << " ? " << (a < b ? "vero" : "falso") << '\n';
		return a < b;
	};
};

} /* namespace std */

namespace jit {

/*!
 * \brief this class keeps track of each register in a compilation unit
 *
 * Every register is associated to a number. So we map the register space to a dense number space.
 * This is needed by the register allocation.
 */
class RegisterManager {
  public:
	typedef RegisterInstance RegType; //!<the register type

	//!constructor
	RegisterManager(uint32_t output_s, std::set<uint32_t> ignore_s);

	/*!
	 * \brief get the number of register used
	 * \return the number of register used
	 */
	uint32_t getCount() const;

	/*!
	 * \brief add a new register to the manager
	 * \param r a reference to the register to add
	 *
	 * The new register is added with at the position count. count is incremented.
	 */
	void addNewRegister(RegType r);

	/*!
	 * \brief get a register from is name
	 * \param name the name of the register
	 * \return the register searched
	 */
	RegType operator[](RegType name);

	/*!
	 * \brief get the name of a register
	 * \param r the register
	 * \return the name associated to that register
	 */
	RegType getName(const RegType& r);

	protected:
		uint32_t count; //!<register counter
		uint32_t output_space; //!<output register space
		std::set<uint32_t> ignore_set; //!<set of ignored (pass-through) register spaces
		std::map<RegType, RegType> regToInt; //!< map register->name
		std::vector<RegType> intToReg; //!< map name->register
}; /* class RegisterManager */

} /* namespace jit */

inline uint32_t jit::RegisterModel::_proxy::reg_space() const
{
	return model->reg_space;
}

inline uint32_t jit::RegisterModel::_proxy::reg_name() const
{
	return model->reg_name;
}

inline uint32_t jit::RegisterModel::get_name() const
{
	return proxy->reg_name();
}

inline uint32_t jit::RegisterModel::get_space() const
{
	return proxy->reg_space();
}

inline bool jit::RegisterModel::operator<(const RegisterModel &other) const
{
	uint32_t a, b;
	
	if((a = get_space()) == (b = other.get_space())) {
		a = get_name();
		b = other.get_name();
	}
	
	return a < b;
}

inline bool jit::RegisterModel::operator==(const RegisterModel &other) const
{
	return !(*this < other) && !(other < *this);
}


inline jit::RegisterInstance jit::RegisterInstance::operator=(RegisterInstance other)
{
	model = other.get_model();
	version() = other.version();
	return *this;
}

inline jit::RegisterModel *jit::RegisterInstance::get_model()
{
	assert(model != 0);
	return model;
}

inline uint32_t& jit::RegisterInstance::version()
{
	return _version;
}

inline const jit::RegisterModel *jit::RegisterInstance::get_model() const
{
	assert(model != 0);
	return model;
}

inline const uint32_t &jit::RegisterInstance::version() const
{
	return _version;
}

inline bool jit::RegisterInstance::operator<(const RegisterInstance &other) const
{
//	cout << "modello mio: " << get_model();

	if(!(*get_model() < *other.get_model()) && !(*other.get_model() < *get_model())) {
//		cout << "confronto le versioni ";
		return version() < other.version();
	}
	
//	cout << "confronto i modelli ";
	return *get_model() < *other.get_model();
}

inline bool jit::RegisterInstance::operator==(const RegisterInstance &other) const
{
	return !(*this < other) && !(other < *this);
}

inline bool jit::RegisterInstance::operator!=(const RegisterInstance &other) const
{
	return !(*this == other);
}

