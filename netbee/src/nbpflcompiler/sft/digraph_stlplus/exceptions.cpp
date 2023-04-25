////////////////////////////////////////////////////////////////////////////////

//   Author: Andy Rushton
//   Copyright: (c) Andy Rushton, 2007
//   License:   BSD License, see ../docs/license.html
  
//   The set of general exceptions thrown by STLplus components

////////////////////////////////////////////////////////////////////////////////
#include "exceptions.hpp"

////////////////////////////////////////////////////////////////////////////////

stlplus::null_dereference::null_dereference(const std::string& description) throw() :
std::logic_error(std::string("stlplus::null_dereference: ") + description)
{
}

stlplus::null_dereference::~null_dereference(void) throw() 
{
}

////////////////////////////////////////////////////////////////////////////////

stlplus::end_dereference::end_dereference(const std::string& description) throw() :
  std::logic_error("stlplus::end_dereference: " + description)
{
}

stlplus::end_dereference::~end_dereference(void) throw()
{
}

////////////////////////////////////////////////////////////////////////////////

stlplus::wrong_object::wrong_object(const std::string& description) throw() :
  std::logic_error("stlplus::wrong_object: " + description)
{
}

stlplus::wrong_object::~wrong_object(void) throw()
{
}

////////////////////////////////////////////////////////////////////////////////

stlplus::illegal_copy::illegal_copy(const std::string& description) throw() :
  std::logic_error("stlplus::illegal_copy: " + description)
{
}

stlplus::illegal_copy::~illegal_copy(void) throw()
{
}

////////////////////////////////////////////////////////////////////////////////

