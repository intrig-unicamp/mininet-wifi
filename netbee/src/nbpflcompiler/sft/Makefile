include Definitions.mk

all: fsa_test
.PHONY: all

fsa_test: exceptions.o fsa_test.o
	$(CPP) $(CPPFLAGS) -o fsa_test fsa_test.o exceptions.o

fsa_test.o: sft.hpp sft_basic_iterator.hpp sft_defs.h sft_generic_set.hpp sft_tools.hpp sft_traits.hpp sft_writer.hpp fsa_test.cpp
	$(CPP) $(CPPFLAGS) -c fsa_test.cpp

exceptions.o: $(STLPDIR)/containers_fixes.hpp $(STLPDIR)/digraph.hpp $(STLPDIR)/digraph.tpp $(STLPDIR)/exceptions.hpp $(STLPDIR)/safe_iterator.hpp $(STLPDIR)/safe_iterator.tpp $(STLPDIR)/exceptions.cpp
	$(CPP) $(CPPFLAGS) -c $(STLPDIR)/exceptions.cpp

.PHONY: clean
clean:
	-rm *.o
	-rm fsa_test

.PHONY: srcclean
srcclean:
	-rm *~
