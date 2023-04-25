#include <iostream>
#include <fstream>
#include "sft.hpp"
#include "sft_writer.hpp"
#include "sft_tools.hpp"
#include <string>

using namespace std;



typedef sftFsa<string, string, bool> fsa_t; 
typedef fsa_t::StateIterator state_t;
typedef fsa_t::TransIterator trans_t;
typedef fsa_t::Alphabet_t alphabet_t;
typedef sftDotWriter<fsa_t> fsa_dot_writer_t;
typedef sftTools<string, string, bool> tools_t;

int main(void)
{
  alphabet_t a;

  fsa_t my_fsa(a), my_next_fsa(a);

  state_t start = my_fsa.AddState("start");
  state_t eth = my_fsa.AddState("eth");
  state_t ip = my_fsa.AddState("ip");
  my_fsa.SetAccepting(ip);
  state_t sink = my_fsa.AddState("sink");
  my_fsa.SetInitialState(start);
  trans_t se = my_fsa.AddTrans(start, eth, "se");
  trans_t ei = my_fsa.AddTrans(eth, ip, "ei");
  trans_t ss = my_fsa.AddTrans(start, sink, "se", true);
  trans_t es = my_fsa.AddTrans(eth, sink, "ei", true);

  state_t start2 = my_next_fsa.AddState("start2");
  state_t eth2 = my_next_fsa.AddState("eth2");
  state_t ip2 = my_next_fsa.AddState("ip2");
  state_t tcp2 = my_next_fsa.AddState("tcp2");
  my_next_fsa.SetAccepting(tcp2);
  state_t sink2 = my_next_fsa.AddState("sink2");
  my_next_fsa.SetInitialState(start2);
  trans_t se2 = my_next_fsa.AddTrans(start2, eth2, "se2");
  trans_t ei2 = my_next_fsa.AddTrans(eth2, ip2, "ei2");
  trans_t it2 = my_next_fsa.AddTrans(ip2, tcp2, "it2");
  trans_t ss2 = my_next_fsa.AddTrans(start2, sink2, "se2", true);
  trans_t es2 = my_next_fsa.AddTrans(eth2, sink2, "ei2", true);
  trans_t is2 = my_next_fsa.AddTrans(ip2, sink2, "it2", true);

  fsa_t *last_fsa = tools_t::Merge(my_fsa, my_next_fsa);
	
  ofstream file("fsa.dot");
  fsa_dot_writer_t fsaWriter(file);
  fsaWriter.DumpFSA(last_fsa);
  cout << "FSA dumped on fsa.dot" << endl;
  return 0;
}
