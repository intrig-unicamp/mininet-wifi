netvmburg
this tool is an iburg like code generator generator based on the source code of MonoBurg, a similar tool developed by Dietmar Maurer for the Mono Project.
The input of the tool is represented by a textual description of all the possible mappings between a set of trees of a tree-based intermediate representation and the instructions of a target assembly language. From this file, the BURG creates a program for performing tree pattern matching by optimally selecting a sequence of target instructions.
More details can be found in the paper from
C. W. Fraser, D. R. Hanson, and T. A. Proebsting,
"Engineering a Simple, Efficient Code Generator Generator",
ACM Letters on Programming Languages and Systems  1, 3 (Sep. 1992).

The usage of the tool is the following:

# netvmburg inssel-ia32.brg > inssel-ia32.cpp