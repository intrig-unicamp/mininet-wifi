/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include <string>
#include <list>
#include <algorithm>
#include <assert.h>

#define SPACES " \r\n"
#define CRLF "\r\n"

inline std::string trim_right(const std::string &s, const std::string & t = SPACES)
{
	std::string d (s);
	std::string::size_type i(d.find_last_not_of (t));
	if (i == std::string::npos)
		return "";
	else
		return d.erase (d.find_last_not_of (t) + 1) ;
}


inline std::string remove_chars(const std::string &s, const std::string & t = CRLF, const std::string & w = " ")
{
	std::string d (s);
	std::string::size_type i;
	while ((i = d.find_first_of(t)) != std::string::npos)
			d.replace(i, 1, w) ;

	return d;
}


inline std::string left(const std::string &s, std::string::size_type n_chars)
{
	if (n_chars >= s.size())
		return s;
	std::string d(s);
	return d.erase(n_chars, s.size());
}


inline std::string ToUpper(const std::string &s)
{
	std::string d(s);

	transform (d.begin (), d.end (), d.begin (), (int(*)(int)) toupper);
	return d;
}


inline std::string ToLower(const std::string &s)
{
	std::string d(s);

	transform (d.begin (), d.end (), d.begin (), (int(*)(int)) tolower);
	return d;
}


class StringSplitter
{
	std::string	m_String;
	unsigned int	m_Pos;
	unsigned int 	m_NChars;

public:
	StringSplitter(std::string s, int nChars)
		:m_String(s), m_Pos(0), m_NChars(nChars){}

	bool HasMoreTokens()
	{
		return m_Pos < m_String.size();
	}

	std::string NextToken()
	{
		std::string s;
		s.assign(m_String, m_Pos, m_NChars);
		m_Pos += m_NChars;
		return s;
	}

};
