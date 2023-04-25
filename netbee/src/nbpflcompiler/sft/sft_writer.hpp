/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#ifndef _SFT_WRITER_H
#define _SFT_WRITER_H

#include "sft_defs.h"
#include "sft.hpp"
#include <ostream>
#include <string>


/*!
	\brief This class provides functionalities for dumping on an output stream (e.g. a file)
			the structure of an FSA in Dot format (see http://www.graphviz.org/)
 */

template <class FSAType>
class sftDotWriter
{
	std::ostream &m_Stream;	//!< the output stream
	bool	m_Horiz;	//!< if true the fsa will be layd out horizontally
	uint32	m_TransLblSize;	//!< size in points of a transition label
	uint32	m_StateLblSize;	//!< size in points of a state label
	double  m_StateSize;	//!< size of a state circle


public:


	/*!
		\brief constructor
		\param o_str the output stream (e.g. a file)
		\param horiz flag for the fsa layout (horizontal if true, vertical otherwise)
		\param trLblSz size in points of a transition label
		\param stLblSz size in points of a state label
		\param stSz size of a state circle

	 */
	sftDotWriter(std::ostream &o_str, bool horiz = true, uint32 trLblSz = 10, uint32 stLblSz = 11, double stSz = 0.65)
		:m_Stream(o_str), m_Horiz(horiz), m_TransLblSize(trLblSz), m_StateLblSize(stLblSz), m_StateSize(stSz){}

	/*!
	 * \brief Dump an FSA
	 * \param f the FSA
	 */
	void DumpFSA(FSAType *f);
	
	/*!
	 * \brief Dump the legend of the FSA
	 */
	void DumpLegend();
};


template <class FSAType>
void sftDotWriter<FSAType>::DumpFSA(FSAType *f)
{
	typedef FSAType fsa_t;
    std::set<pair<typename fsa_t::ExtendedTransition*,string> > ET_set;
    
	std::string dir;
	if (m_Horiz)
		dir = "LR";
	else
		dir = "TB";
	typename fsa_t::StateIterator initial = f->GetInitialState();
	m_Stream << "Digraph fsa{" << std::endl;
	m_Stream << "\trankdir=" << dir << std::endl;
	m_Stream << "\tedge[fontsize=" << m_TransLblSize << "];" << std::endl;
	m_Stream <<	"\tnode[fontsize=" << m_StateLblSize << ", fixedsize=true, shape=circle, ";
	m_Stream << "width=" << m_StateSize << ",heigth=" << m_StateSize << "];" << std::endl;
	m_Stream << "\tstart__[style=invis];" << std::endl;

	for (typename fsa_t::StateIterator i = f->FirstState(); i != f->LastState(); i++)
	{
          m_Stream << "\tst_" << (*i).GetID() << "[label=\"(" << (*i).GetID() <<
                   ")" << (*i).ToString() << "\"";
        //accepting state           
		if ((*i).isAccepting() && !(*i).isAction())
			m_Stream << ",shape=doublecircle";
		//action state
		else if((*i).isAction() && !(*i).isAccepting())
			m_Stream << ",shape=octagon";
		//action and accepting state
		else if((*i).isAction() && (*i).isAccepting())
			m_Stream << ",shape=doubleoctagon";
		m_Stream << "];" << std::endl;
	}
	m_Stream << "\tstart__->" << "st_" << (*initial).GetID() << ";" << std::endl;

	for (typename fsa_t::TransIterator j = f->FirstTrans(); j != f->LastTrans(); j++)
	{
	    if ((*j).getIncludingET())
	    {
			ET_set.insert(make_pair((*j).getIncludingET(),(*j).ToString()));
		    continue;
		}
	
		m_Stream << "\tst_" << (*(*j).FromState()).GetID() << " ->" << "st_" << (*(*j).ToState()).GetID();
		m_Stream << "[label=\"" << (*j).ToString() << "\"];" << std::endl;
	}

        for (typename std::set<pair<typename fsa_t::ExtendedTransition*,string> >::iterator et = ET_set.begin();
             et != ET_set.end();
             et++){
          ((*et).first)->dumpToStream(m_Stream,(*et).second);
        }
        

	m_Stream << "}" << std::endl;

}

        
/*
*	Creates the legend for the files showing the automaton - Ivano Cerrato - 07/31/2012
*/
template <class FSAType>
void sftDotWriter<FSAType>::DumpLegend(void)
{
	m_Stream << "Digraph{" << std::endl;
	m_Stream << "\t{ rank = sink; " << endl;
	m_Stream << "\tLegend [shape=none, margin=0, label=< " << endl;
	m_Stream << "\t<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\" ALIGN=\"RIGHT\"> " << endl;
	m_Stream << "\t <TR> " << endl;
	m_Stream << "\t  <TD COLSPAN=\"2\"><b>STATES LEGEND</b></TD> " << endl;
	m_Stream << "\t </TR> " << endl;
	m_Stream << "\t <TR>" << endl;
  	m_Stream << "\t  <TD><FONT COLOR=\"red\"><i>Shape</i></FONT></TD>" << endl;
    m_Stream << "\t  <TD><FONT COLOR=\"red\">Meaning</FONT></TD>" << endl;
	m_Stream << "\t </TR>" << endl;
	m_Stream << "\t <TR>" << endl;
	m_Stream << "\t  <TD>circle</TD>" << endl;
	m_Stream << "\t  <TD>normal state</TD>" << endl;
	m_Stream << "\t </TR>" << endl;
	m_Stream << "\t <TR>" << endl;
	m_Stream << "\t  <TD>double circle</TD>" << endl;
	m_Stream << "\t  <TD>accepting state</TD>" << endl;
	m_Stream << "\t </TR>" << endl;
	m_Stream << "\t <TR>" << endl;
	m_Stream << "\t  <TD>octagon</TD>" << endl;
	m_Stream << "\t  <TD>action state</TD>" << endl;
	m_Stream << "\t </TR>" << endl;
	m_Stream << "\t <TR>" << endl;
	m_Stream << "\t  <TD>double octagon</TD>" << endl;
	m_Stream << "\t  <TD>accepting and action state</TD>" << endl;
	m_Stream << "\t </TR>" << endl;
	m_Stream << "\t</TABLE> " << endl;
	m_Stream << "\t>]; " << endl;
	m_Stream << "\t} " << endl;

	m_Stream << "}" << std::endl;

}


#endif
