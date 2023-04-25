/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef GENERIC_BACKEND_H
#define GENERIC_BACKEND_H

/*!
 * \file genericbackend.h
 * \brief this file contains the definition of the interface implemented by each backend
 */

#include "mirnode.h"
#include "cfg.h"
#include <string>
#include <iostream>

namespace jit {

	//! the class implemented by each backend
	class GenericBackend {
		public:

		//! a function implemented by a backend that emits native code in a buffer
		virtual uint8_t *emitNativeFunction();
		/*!
		 * \brief a function implemeted by a backend that emits native assembler code in a file
		 * \param prefix string to prepend to the name of files emitted
		 */
		virtual void emitNativeAssembly(std::string prefix);

		/*!
		 * \brief a function implemeted by a backend that emits native assembler code to an output stream
		 * \param  str reference to an output stream
		 */
		virtual void emitNativeAssembly(std::ostream &str);
		//! destructor
		virtual ~GenericBackend();
	};
}

#endif
