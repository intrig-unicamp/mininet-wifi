#ifndef _FUNCTION_INTERFACE_H
#define _FUNCTION_INTERFACE_H

#include <string>


//!this class defines a definition for every algorithm of the jit
class nvmJitFunctionI{
	private:
		std::string alg_name; //!<algorithm name
	public:
		//!<the main algorithm entry point
		virtual bool run() = 0;
		//!destructor
		virtual ~nvmJitFunctionI() {};
		//!constructor
		nvmJitFunctionI(std::string name = std::string("name not specified")): alg_name(name) {}
		//!function to get the algorithm name
		const std::string & getName() const { return alg_name; } 
};


#endif
