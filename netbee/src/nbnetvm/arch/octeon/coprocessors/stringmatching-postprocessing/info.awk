BEGIN {
	flag = 0; 
	elrows=0; 
	elorows=0;
	flag2=0; 
	n=0; 
	k=0; 
	expression_list="";
	expression_over=""
}

/node_to_expression_lists\[\]/ {
	if (flag == 0) flag=1;
}

{ 
	if (flag == 1) {
		n++;
	}
	if (n > 1) {
		expression_list = expression_list "\n" $0;
		elrows = elrows + 1;
	}
}

#/node_to_expression_lists_overflow\[\]/{if (flag == 1) {flag=0; n=0;}}

/\};/{ 
	if( flag== 1) {
		flag=0; n=0;
	}
}

/node_to_expression_lists_overflow\[\]/ {
	if (flag2 == 0) 
		flag2 = 1;
}

{
	if (flag2 == 1) {
		k++;
	}
	if (k > 1) {
		expression_over = expression_over "\n" $0;
		elorows = elorows + 1;
	}
}

/\};/ {
	if (flag2 == 1) {
		flag2=0; k=0;
	}
}

/#define NUM_MARKED_NODES/ {nmn = $3 }
/#define NUM_TERMINAL_NODES/ {ntn =$3}
/#define TERMINAL_NODE_DELTA_MAP_LG/ {tndml = $3}	

END {
	print "\nuint64_t node_to_expression_lists" num "[]= {\n" expression_list;
	print  "\nuint32_t size_of_ex_li" num " = " elrows ";";

	elorows = split(expression_over, dummy, ",");
	print "\nuint64_t node_to_expression_lists_overflow" num \
		  "[]= {\n" expression_over;
	print  "\nuint32_t size_of_ex_li_ov" num " = " elorows ";";
	print "\nuint32_t num_marked_node" num " = " nmn";";
	print "\nuint32_t num_terminal_node" num " = " ntn";";
	print "\nuint32_t terminal_node_delta_map_lg" num " = " tndml";";
}
