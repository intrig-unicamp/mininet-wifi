#ifndef _SFT_TRAITS
#define _SFT_TRAITS


#include "./digraph_stlplus/digraph.hpp"
#include <string>
#include <sstream>


/*
 *	Some generic traits classes
 */

template <typename T>
struct isVoid
{ static const bool value = false; };

template <>
struct isVoid<void>
{ static const bool value = true; };


template <typename T>
struct isPointer
{ static const bool value = false; };

template <typename T>
struct isPointer<T*>
{ static const bool value = true; };



/* Some more sft-specific traits classes */

template <class SType>
struct STraits{};

template <class LType>
struct LTraits{};

template <class PType>
struct PTraits{};

template <class SPType>
struct SPTraits{};

template <>
struct STraits<std::string>
{
	static std::string null()
	{
		return std::string("");
	}

	static std::string ToString(std::string &s)
	{
		return s;
	}

	static std::string Union(std::string &s1, std::string &s2)
	{
		return s1 + ", " + s2;
	}
};

template <>
struct SPTraits<std::string>
{	
	static std::string null()
	{
		return std::string("");
	}

	static std::string ToString(int &i)
	{
		std::stringstream ss;
		ss << i;
		return ss.str();
	}

	static int Union(int &a, int &b)
	{
		return a;
	}
};

template <>
struct STraits<int>
{
	static int null()
	{
		return 0;
	}

	static std::string ToString(int &i)
	{
		std::stringstream ss;
		ss << i;
		return ss.str();
	}

	static int Union(int &a, int &b)
	{
		return a;
	}
};


template <>
struct LTraits<std::string>
{

	static std::string ToString(std::string &s)
	{
		return s;
	}
};

template <>
struct LTraits<char>
{

	static std::string ToString(char &c)
	{
		char buf[1];
		buf[0] = c;
		return std::string(buf);
	}
};

template <>
struct PTraits<bool>
{
	static bool One()
	{
		return true;
	}
	static bool Zero()
	{
		return false;
	}

	static std::string ToString(bool &b)
	{
		std::string s;
		if (b)
			s = "/t";
		else
			s = "/f";
		return s;
	}

	static bool AndOp(bool &a, bool &b)
	{
		return a && b;
	}

	static bool OrOp(bool &a, bool &b)
	{
		return a || b;
	}

	static bool NotOp(bool &a)
	{
		return !a;
	}

        static bool DumpNeeded(bool &b)
        {
          return true;
        }
};




template <typename CType, typename NT, typename AT>
struct stlplus_digraph_node
{

	typedef stlplus::digraph<NT, AT> AType;
	typedef typename stlplus::digraph<NT, AT>::iterator IType;
	typedef CType& deref_type;

	static IType begin(AType &a)
	{
		return a.begin();
	}

	static IType end(AType &a)
	{
		return a.end();
	}

	static IType next(AType &a, IType &it)
	{
		it++;
		return it;
	}

	static bool isend(AType &a, IType &it)
	{
		return it == a.end();
	}

	static deref_type deref(IType &it)
	{
		//(*it) returns a CType*
		return (*it);
	}

};

template <typename CType, typename NT, typename AT>
struct stlplus_digraph_node<CType*, NT, AT>: public stlplus_digraph_node<CType, NT, AT>
{
	typedef stlplus::digraph<NT, AT> AType;
	typedef typename stlplus::digraph<NT, AT>::iterator IType;
	typedef CType& deref_type;

	static deref_type deref(IType &it)
	{
		return *(*it);
	}

};




template <typename CType, typename NT, typename AT>
struct stlplus_digraph_arc
{

	typedef stlplus::digraph<NT, AT> AType;
	typedef typename AType::arc_iterator IType;
	typedef CType& deref_type;

	static IType begin(AType &a)
	{
		return a.arc_begin();
	}

	static IType end(AType &a)
	{
		return a.arc_end();
	}

	static IType next(AType &a, IType &it)
	{
		it++;
		return it;
	}

	static bool isend(AType &a, IType &it)
	{
		return it == a.arc_end();
	}

	static deref_type deref(IType &it)
	{
		//(*it) returns a CType*
		return (*it);
	}

};

template <typename CType, typename NT, typename AT>
struct stlplus_digraph_arc<CType*, NT, AT>: public stlplus_digraph_arc<CType, NT, AT>
{
	typedef stlplus::digraph<NT, AT> AType;
	typedef typename AType::arc_iterator IType;
	typedef CType& deref_type;

	static deref_type deref(IType &it)
	{
		//(*it) returns a CType*
		//*(*it) returns a CType&
		return *(*it);
	}

};



#endif
