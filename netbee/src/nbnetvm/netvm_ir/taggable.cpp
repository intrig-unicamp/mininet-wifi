#include "taggable.h"

#include <iostream>
using namespace std;

namespace jit {

Taggable::Taggable(const Taggable &t)
{
	map<std::string, Container *>::const_iterator i;
	for(i = t.properties.begin(); i != t.properties.end(); ++i)
		properties[i->first] = i->second->clone();

	return;
}

Taggable &Taggable::operator=(const Taggable &t)
{
	map<string, Container *>::const_iterator i;
	for(i = properties.begin(); i != properties.end(); ++i)
		delete i->second;

	for(i = t.properties.begin(); i != t.properties.end(); ++i)
		properties[i->first] = i->second->clone();

	return *this;
}

} /* namespace jit */
