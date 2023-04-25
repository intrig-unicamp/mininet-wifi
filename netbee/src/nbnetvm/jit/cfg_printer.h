/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#ifndef CFG_PRINTER_H
#define CFG_PRINTER_H

/*!
 * \file cfg_printer.h
 * \brief this file contains some class used to print a CFG in different ways
 */

#include "cfg.h"
#include "mirnode.h"
#include "jit_internals.h"
#include <ostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <set>

namespace jit {

	/*!
	 * \brief a simple effector class that does nothing
	 */
	template<class T>
	class NormalPrinter
	{
		private:
		T& t;

		public:
		NormalPrinter(T& t): t(t) {}

		template<class _T>
		friend std::ostream& operator<<(std::ostream& os, NormalPrinter<_T>& p);
	};
	
	template<class T>
	class NonSSAPrinter
	{
	  private:
		T& t;
		
	  public:
		NonSSAPrinter(T& t) : t(t) {}
		
		template<class _T>
		friend std::ostream & operator<<(std::ostream &os, NonSSAPrinter<_T>& p);
	};
	
	template<class T>
	class SSAPrinter
	{
	  private:
		T& t;
		
	  public:
		SSAPrinter(T& t) : t(t) {}
		
		template<class _T>
		friend std::ostream & operator<<(std::ostream &os, SSAPrinter<_T>& p);
	};

	template<class T>
	class SignaturePrinter
	{
	  private:
		T& t;
		
	  public:
		SignaturePrinter(T& t) : t(t) {}
		
		template<class _T>
		friend std::ostream & operator<<(std::ostream &os, SignaturePrinter<_T>& p);
	};

	template<class T>
	std::ostream &operator<<(std::ostream &os, SignaturePrinter<T> &p)
	{
//		os << "Stampo NON SSA\n";
		return p.t.printSignature(os);
	}

	
	template<class T>
	std::ostream &operator<<(std::ostream &os, NonSSAPrinter<T> &p)
	{
//		os << "Stampo NON SSA\n";
		return p.t.printNode(os, false);
	}
	
	template<class T>
	std::ostream &operator<<(std::ostream &os, SSAPrinter<T> &p)
	{
//		os << "Stampo SSA\n";
		return p.t.printNode(os, true);
	}

	template<class T>
		std::ostream& operator<<(std::ostream& os, NormalPrinter<T>& p)
		{
			return os << p.t;
		}

	/*!
	 * \brief an interface for a class printing a CFG
	 */
	template<class T>
	class CFGPrinter
	{
		protected:
			CFG<T>& cfg; //!<The CFG to print

		public:

			/*!
			 * \brief this function simply iterates on all the nodes of the graph and calls printNode
			 * \param os the output stream
			 */
			void printNodes(std::ostream &os)
			{
				typedef typename std::map<uint16_t, BasicBlock<T>*>::iterator iterator_t;
				typedef typename BasicBlock<T>::BBIterator node_iterator_t;

				for(iterator_t i = CFGPrinter<T>::cfg.allBB.begin(); i != CFGPrinter<T>::cfg.allBB.end(); i++)
				{
					BasicBlock<T> *BB = i->second;
					printNode(os, *BB);
				}
			}

			//! printing action called on all nodes
			virtual void printNode(std::ostream &os, BasicBlock<T>& BB) = 0;
			//! called upon starting printing the graph
			virtual void beginPrint(std::ostream &os) {};
			//! called upon finishing printing the graph
			virtual void endPrint(std::ostream &os) {};

		public:
			/*!
			 * \brief constructor
			 * \param cfg the control flow graph to print
			 */
			CFGPrinter(CFG<T>& cfg): cfg(cfg) {}
			//!destructor
			virtual ~CFGPrinter() {}

		template <class _T>
			friend	std::ostream& operator<< (std::ostream& os, CFGPrinter<_T>& cfgPrinter);

	};

	/*!
	 * \brief the printing operation
	 * 
	 * Calls in order:
	 * <ul>
	 * <li>beginPrint</li>
	 * <li>printNodes</li>
	 * <li>endPrint</li>
	 * </ul>
	 */
	template <class _T>
		std::ostream& operator<< (std::ostream& os, CFGPrinter<_T>& cfgPrinter)
		{

			cfgPrinter.beginPrint(os);
			cfgPrinter.printNodes(os);
			cfgPrinter.endPrint(os);

			return os;
		}

	/*!
	 * \brief a simple effector for printing a basic block
	 * 
	 * This class prints a basic block code. You can specify how to print the code by 
	 * passing a NodePrinter class to the template.
	 */
	template<class _T, class _NodePrinter>
	class BBPrinter
	{
		private:
			BasicBlock<_T>& BB;//!<the basic block to print

		public:
			/*!
			 * \brief constructor
			 * \param BB the basic block to print
			 */
			BBPrinter(BasicBlock<_T>& BB): BB(BB){}

		template<class T, class NodePrinter>
			friend std::ostream& operator<<(std::ostream& os, BBPrinter<T, NodePrinter>);
	};

	/*!
	 * \brief printing action
	 *
	 * Prints the basic block Id then prints every instruction in the basic block using
	 * the effector NodePrinter.
	 */
	template<class T, class NodePrinter>
		std::ostream& operator<<(std::ostream& os, BBPrinter<T, NodePrinter> bbPrinter)
		{
			typedef typename BasicBlock<T>::IRStmtNodeIterator code_iterator_t;

			BasicBlock<T>& BB = bbPrinter.BB;

			os << "lbl" << BB.getId() << ":" << std::endl;

			for(code_iterator_t code = BB.codeBegin(); code != BB.codeEnd(); code++)
			{
				NodePrinter n(**code);
				os << "\t" << n << std::endl;
				//os << **code << '\n';
			}

			return os;
		}

	//! an effector to print the information stored in a Basic Block property map
	template<class _T>
	class DumpInfo
	{
		private:
			BasicBlock<_T>& BB; //!<basic block to print

		public:

			/*!
			 * \brief constructor
			 * \param BB the basic block to print
			 */
			DumpInfo(BasicBlock<_T>& BB): BB(BB) {}

		template<class T>
			friend std::ostream& operator<<(std::ostream& os, DumpInfo<T>);
	};

	/*!
	 * \brief printing action
	 * 
	 * This function prints the direct dominator of a Basic Block, its successors
	 * in the dominator tree and its dominator frontier.
	 */
	template<class IR>
		std::ostream& operator<<(std::ostream& os, DumpInfo<IR> bbPrinter)
		{
			typedef typename std::set< BasicBlock<IR>* >::iterator BBIt;
			BasicBlock<IR>* idom;
			os << "BB ID: " << bbPrinter.BB.getId() << std::endl;
			os << "IDOM:";
			if( (idom = (BasicBlock<IR>*)bbPrinter.BB.getProperty("IDOM")) )
				os << idom->getId() << std::endl;
			else
				os << "??" << std::endl;

			os << "DomSuccessors: ";
			std::set< BasicBlock<IR>* > *doms;
			doms = (std::set< BasicBlock<IR>* > *)bbPrinter.BB.getProperty("DomSuccessors");
			if(doms)
				for(BBIt i = doms->begin(); i != doms->end(); i++)
					os << **i;
			os << std::endl;
			os << "Dom Frontier: " << std::endl;
			std::set< BasicBlock<IR>* > *fronts;
			fronts = (std::set< BasicBlock<IR>* > *)bbPrinter.BB.getProperty("DomFrontier");
			if(fronts)
				for(BBIt i = fronts->begin(); i != fronts->end(); i++)
					os << **i;
			os << std::endl << std::endl << std::endl;

			return os;
		}

	//! a CFGPrinter to print a graph node in dot notation
	template<class T>
	class DotPrint : public CFGPrinter<T>
	{
		public:

			/*!
			 * \brief print action on a Basic Block
			 */
			void printNode(std::ostream& os, BasicBlock<T>& BB);

			void beginPrint(std::ostream& os)
			{
				os << "digraph G {" << std::endl;
			}

			void endPrint(std::ostream& os)
			{
				os << "}" << std::endl;
			}

			DotPrint(CFG<T>& cfg): CFGPrinter<T>(cfg)
			{}
			
	};

	template<class T>
		class GenericDiGraphDotPrinter 
	{
		private:
			DiGraph<T> & _graph;
		public:

			void printNodes(std::ostream& os);

			void beginPrint(std::ostream & os);

			void endPrint(std::ostream & os);
		template <class _T>
			friend	std::ostream& operator<< (std::ostream& os, GenericDiGraphDotPrinter<T>& );
	};
/*
	template<class T>
		void GenericDiGraphDotPrinter::printNodes(std::ostream& os)
		{
		
		}
	template<class T>
		void GenericDiGraphDotPrinter::beginPrint(std::ostream& os)
		{
			os << "digraph G {" << std::endl;
		}
	template<class T>
		void GenericDiGraphDotPrinter::endPrint(std::ostream& os)
		{
			os << "}" << std::endl;
		}

	template <class _T>
		friend	std::ostream& operator<< (std::ostream& os, GenericDiGraphDotPrinter<T>& gp)
		{
			gp.beginPrint();
			gp.printNodes();
			gp.endPrint();
		}
*/
	/*! 
	 * For every nodes prints the edges that start from it in dot notation.
	 * Theh prints a label for the node with its Id.
	 */
	template<class T>
		void DotPrint<T>::printNode(std::ostream& os, BasicBlock<T>& BB)
		{
			typedef typename BasicBlock<T>::BBIterator node_iterator_t;

			for (node_iterator_t j = BB.succBegin(); j!= BB.succEnd(); j++)
			{
				os << BB.getId() << " -> " << (*j)->getId() << std::endl;
			}

			os << BB.getId() << " [ label = \"" << BB << "\"]" << std::endl;

		}

	/*!
	 * \brief a CFG printer that prints the code stored in eveery basic block of the graph
	 *
	 * You can specify how to print the code by passing an effector for the instruction as the
	 * NodePrinter parameter of this template class.
	 */
	template<class T, class NodePrinter = NormalPrinter<T> >
		class CodePrint : public CFGPrinter<T>
	{
		public:
			void printNode(std::ostream& os, BasicBlock<T>& BB)
			{
//				os << "Stampo il BB " << BB.getId() << '\n';
				os << BBPrinter<T, NodePrinter>(BB) << std::endl;
			}

			CodePrint(CFG<T>& cfg): CFGPrinter<T>(cfg)
			{ }
	};


	//!a simple effector to print the information stored in the basic blocks' property map 
	template<class T>
	class InfoPrinter : public CFGPrinter<T>
	{
		public:

			void printNode(std::ostream& os, BasicBlock<T>& BB)
			{
					os << DumpInfo<T>(BB);
			}

			void beginPrint(std::ostream& os) { os << "Inizio Info Printer" << std::endl; }

			void endPrint(std::ostream& os) { os << "Fine infoprinter" << std::endl; }

			InfoPrinter(CFG<T>& cfg): CFGPrinter<T>(cfg) {} 
	};


	/*!
	 * \brief A simple class that uses a CFGPrinter to print on a stream
	 */
	template <class IR>
		class DoPrint: public nvmJitFunctionI
		{
			private:
				std::string _filename;
				CFGPrinter<IR> *_printer;
				std::string _print_name;

			public:
				/*! 
				 * \brief Constructor
				 *
				 * \param filename that contains the filename. If you want to print on stderr or stdout just pass string "stdout" "stderr"
				 * \param printer CFPrinter to be used.
				 * \param print_name this string is written to the stream before the real output
				 */
				DoPrint(std::string filename, CFGPrinter<IR> *printer, std::string print_name): nvmJitFunctionI("Do Print"), _filename(filename), _printer(printer), _print_name(print_name) {}
				~DoPrint();

				bool run();
				void setPrintName(std::string);
		};

	template <class IR>
		bool DoPrint<IR>::run()
		{
			if(_filename.empty())
			{
				std::cout << _print_name << std::endl;
				std::cout << *_printer;
			}
			else
			{
				if(_filename == "stdout")
				{
					std::cout << _print_name << std::endl;
					std::cout << *_printer;
				}
				else if(_filename == "stderr")
				{
					std::cerr << _print_name << std::endl;
					std::cerr << *_printer;
				}
				else
				{
					std::ofstream _ofs(_filename.c_str());
					_ofs << _print_name << std::endl;
					_ofs << *_printer;
				}
			}
			return true;
		};
	template <class IR>
		void DoPrint<IR>::setPrintName(std::string print_name)
		{
			this->_print_name = print_name;
		}

	template <class IR>
		DoPrint<IR>::~DoPrint()
		{
			delete _printer;
		}
}

#endif
