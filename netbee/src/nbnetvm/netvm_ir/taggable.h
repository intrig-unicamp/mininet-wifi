/*!
 * \file taggable.h
 * \brief This file contains the implementation of a class to tag some other structure
 */
#ifndef _TAGGABLE_H
#define _TAGGABLE_H
#include <map>
#include <string>
#include <iostream>
#include <cassert>

namespace jit {

/*!
 * \brief Every structure which needs to be taggable inherits this class
 *
 * This class contains a map indexed by a string. You can add a property to the map
 * using setProperty and can get a property value from the map usig the getProperty
 */
/*!
 * \todo
 * Do we really need to put a map in every node? Shouldn't we find a simpler way?
 * See the boost library implementation for some ideas.
 */
class Taggable
{
	private:
		struct Container {
			virtual ~Container() {};
			virtual Container *clone() const = 0;
		};
		
		template<typename T>
		struct ActualContainer : public Container {
			T content;
			
			T get() const {return content;};
			
			ActualContainer(const T &t) : content(t) {};
			
			virtual ActualContainer<T> *clone() const;
		};

	protected:
		std::map<std::string, Container *> properties; //!<the map which keep the properties

	public:
		Taggable() {};
		Taggable(const Taggable &t);
		Taggable &operator=(const Taggable &t);

		virtual ~Taggable() {};

		//!<get a property of type T
		template<typename T>
			T getProperty(std::string key) const
			{
				typename std::map<std::string, Container *>::const_iterator i;
				if((i = properties.find(key)) == properties.end()) {
					return (T)0;
				}
				
				assert(i->second != 0);

				const ActualContainer<T> *ac(dynamic_cast<const ActualContainer<T> *>(i->second));
				assert(ac != 0 && "Wrong type required from container!");
				return ac->get();
			}

		//!< set a property of type T
		template<typename T>
			void setProperty(std::string key, T value)
			{
				typename std::map<std::string, Container *>::iterator i;
				if((i = properties.find(key)) != properties.end())
					delete i->second;
	
				properties[key] = new ActualContainer<T>(value);
			}

#if 0
		//!<generic implementation 
		void* getProperty(std::string key)
		{
			return getProperty<void *>(key);
		}

		//!generic implementation
		void setProperty(std::string key, void* value)
		{
			properties[key] = new ActualContainer<void *>(value);
		}
#endif
		
		void dump_keys() {
			std::map<std::string, Container *>::iterator i;
			for(i = properties.begin(); i != properties.end(); ++i) {
				std::cout << '|' << i->first << "| -> " << i->second << '\n';
			}
			return;
		}
};

template<typename T>
Taggable::ActualContainer<T> *Taggable::ActualContainer<T>::clone() const
{
	return new ActualContainer<T>(content);
}

} /* namespace jit */
#endif /* _TAGGABLE_H_ */
