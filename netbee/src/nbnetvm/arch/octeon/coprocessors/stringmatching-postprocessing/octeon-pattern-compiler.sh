#!/bin/bash

#$1 num di file in input

if [ $# -le 0 ]; then
    echo 'Usage: $0 [num of input files]'
    exit 1
fi

NUM_GRAPH=$1
REPLICATION=1
DFA_TYPE=36
n=0

rm dfa_info.h

n=`ls *pat | wc -l`
echo $0: $n graphs found

for ((i=0;i<n;i++)) # in `ls *.pat`
do
	FILE=stringmatching${i}.pat
	echo $FILE
	dfapp -v $FILE > _dfac_input
	dfac -replication=$REPLICATION
	cp dfa$DFA_TYPE.llm dfa$i\x$DFA_TYPE\x$REPLICATION.llm
	cp dfa-info.h dfa-info$i\x$REPLICATION.h
	#n=$(( $n + 1 ))
done

FILES=""
for ((i=0; i < n; i++))
do
FILES="$FILES dfa${i}x${DFA_TYPE}x${REPLICATION}.llm"
done
echo $n
./make-compiled-graph $n graph.h $FILES

k=0
for (( k=0; k < n; k++ )) #i in `ls dfa-info*`
do
	gawk -v num=$k -F " " -f info.awk dfa-info${k}\x${REPLICATION}.h >> dfa_info.h
done
t=0
k=$(( $k - 1 ))
#size of ex li
echo "const uint32_t *size_of_ex_li [] ={" >> dfa_info.h
for ((t=0;t <  k; t++))
do
echo "&size_of_ex_li$t," >> dfa_info.h
done
echo "&size_of_ex_li$t};" >> dfa_info.h

t=0
#k=$(( $k - 1 ))
#size of ex li ov
echo "const uint32_t *size_of_ex_li_ov [] ={" >> dfa_info.h
for ((t=0;t <  k; t++))
do
echo "&size_of_ex_li_ov$t," >> dfa_info.h
done
echo "&size_of_ex_li_ov$t};" >> dfa_info.h

t=0
#k=$(( $k - 1 ))
#marked
echo "const uint32_t *num_marked_node [] ={" >> dfa_info.h
for ((t=0;t <  k; t++))
do
echo "&num_marked_node$t," >> dfa_info.h
done
echo "&num_marked_node$t};" >> dfa_info.h

t=0
#terminal
echo "const uint32_t *num_terminal_node [] ={" >> dfa_info.h
for ((t=0;t <  k; t++))
do
echo "&num_terminal_node$t," >> dfa_info.h
done
echo "&num_terminal_node$t};" >> dfa_info.h

t=0
#delta_map_lg
echo "const uint32_t *terminal_node_delta_map_lg [] ={" >> dfa_info.h
for ((t=0;t <  k; t++))
do
echo "&terminal_node_delta_map_lg$t," >> dfa_info.h
done
echo "&terminal_node_delta_map_lg$t};" >> dfa_info.h

t=0
#node_to_expression_list
echo "uint64_t *node_to_expression_lists[] ={" >> dfa_info.h
for ((t=0;t <  k; t++))
do
echo "node_to_expression_lists$t," >> dfa_info.h
done
echo "node_to_expression_lists$t};" >> dfa_info.h

t=0
#node_to_expression_list_overflow
echo "uint64_t *node_to_expression_lists_overflow[] ={" >> dfa_info.h
for ((t=0;t <  k; t++))
do
echo "node_to_expression_lists_overflow$t," >> dfa_info.h
done
echo "node_to_expression_lists_overflow$t};" >> dfa_info.h




cat >>dfa_info.h <<END
info_dfa	info ={
					num_marked_node,
					num_terminal_node,
					terminal_node_delta_map_lg,
					node_to_expression_lists,
					node_to_expression_lists_overflow,
					size_of_ex_li,
					size_of_ex_li_ov};
END

exit 0
