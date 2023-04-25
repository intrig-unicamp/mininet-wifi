#!/usr/bin/gawk -f

# XML functions
function xml_doc_header() {
	print("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
	      "<?xml-stylesheet type='text/xsl' href='netvm_instructionset.xsl'?>\n\n" \
	      "<InstructionSet>");
}

function xml_doc_line(token, name, args, opcode, name2, category, desc, exceptions, stack, variant) {
	if (name2 != last_inst) {
		/* New instruction (and maybe new section) */
		last_inst = name2;

		if (!first_inst) {
			/* Close previous instruction */
			print ("</Variants>\n" \
			       "</Instruction>");
		} else
			first_inst = 0;

		if (category != last_category) {
			catline = sprintf("<Section Name=\"%s\">\n", category);
			last_category = category;

			if (!first_section)
				catline = sprintf("</Section>\n%s", catline);
			else
				first_section = 0;
		} else
			catline = "";

		/* This line must always be present */
		instline = sprintf("<Instruction Name=\"%s\" Category=\"%s\">\n", name2, category);

		if (desc && desc != "")
			descline = sprintf("<Description>%s</Description>\n", desc);
		else
			descline = "<Description>No description available</Description>\n";

		if (exceptions && exceptions != "none") {
	# 		n = split (exceptions, arr, " - ");
	# 		excline = "<tr>\n<td width=\"30%\">Possible exceptions:</td>\n<td colspan=\"2\">\n<ul>\n"
	# 		for (i = 1; i <= n; i++)
	# 			excline = excline "<li>" arr[i] "</li>\n";
	# 		excline = excline "</ul>\n</td>\n<tr>\n";
			excline = sprintf("<Exception>%s</Exception>\n", exceptions);
		} else
			excline = "";

		if (stack && stack != "... --> ..." && stack != "...-->...")
			stackline = sprintf("<StackTransition>%s</StackTransition>\n", stack);
		else
			stackline = "";

		print (catline \
		       instline \
		       descline \
		       excline \
		       stackline \
		       "<Variants>");
	}

	/* Variant */
	varline = sprintf("<Variant CanonicalName=\"%s\" OpCode=\"%s\">\n", token, opcode);
	detline = sprintf("<Details>%s</Details>\n", variant);
	sigline = sprintf("<Signature>%s</Signature>\n", name);
	if (args == "T_1LABEL_INST") {
		argline = "&lt;label 32&gt;";
	} else if (args == "T_1_PUSH_PORT_ARG_INST" || args == "T_1_PULL_PORT_ARG_INST") {
		argline = "&lt;port&gt;";
	} else if (args == "T_1INT_ARG_INST") {
		argline = "&lt;int 32&gt;";
	} else if (args == "T_1BYTE_ARG_INST") {
		argline = "&lt;int 8&gt;";
	} else if (args == "T_1_SHORT_LABEL_INST") {
		argline = "&lt;label 16&gt;";
	} else if (args == "T_JMP_TBL_LKUP_INST") {
		argline = "&lt;int 32&gt; &lt;int 32&gt;:&lt;label 32&gt; ... &lt;int 32&gt;:&lt;label 32&gt; default:&lt;label 32&gt;";
	}else if (args == "T_COPRO_INIT_INST") {
		argline = "&lt;coproc_name&gt;, &lt;int 32&gt;"; 
	}else if (args == "T_2_COPRO_IO_ARG_INST") {
		argline = "&lt;coproc_name&gt;, &lt;int 32&gt;"; 
  }else if (args == "T_1_COPRO_IO_ARG_INST") {
		argline = "&lt;coproc_name&gt;";
  }else {
		argline = "&lt;no arguments&gt;";
	}
	argline = sprintf("<Parameters>%s</Parameters>\n", argline);
	print (varline \
	       detline \
	       sigline \
	       argline \
	       "</Variant>");
}

function xml_doc_footer() {
	print ("</Variants>\n" \
	       "</Instruction>\n" \
	       "</Section>\n" \
	       "</InstructionSet>");
}

function trim(str,   tmp) {
	/* Take away leading blanks */
	tmp = gensub("^[[:blank:]]+", "", "g", str);

	/* Take away leading and trimming quotes */
	tmp = gensub("^\"|\"$", "", "g", tmp);

	return(tmp);
}


BEGIN {
	FS = ",\t";
	last_category = "";
	last_inst = "";
	first_section = 1;
	first_inst = 1;
	inst_no = 0;
}

! /^nvmOPCODE/ {
	/* Ignore */
	;
}

/^nvmOPCODE/ {
	if (NF != 6) {
		printf ("Line %d: Invalid line (%d fields)\n", NR, NF) > "/dev/stderr";
	} else if (split(trim(gensub("\\).$", "", "", $6)), arr, "|") != 6) {	/* Extract the category and add all the instructions to an array */
		printf ("Line %d: Wrong format string\n", NR) > "/dev/stderr";
	} else {
		name2 = arr[1];
		category = arr[2];
		n = sprintf ("%04d", NR);

		printf ("Found %s (%s/%s/%s)\n", trim($2), category, name2, n) > "/dev/stderr";
		instructions[category, name2, n] = gensub("[\r\n]*$", "", "", $0);	/* We use all of these fields so that the array gets sorted correctly later */
		inst_no++;
	}
}

END {
	asorti (instructions, tags);

	xml_doc_header();
	for (i = 1; i <= inst_no; i++) {
# 		print i " -> " tags[i] " -> " instructions[tags[i]] > "/dev/stderr";
		split (instructions[tags[i]], linearr);
		token = gensub("^nvmOPCODE\\(", "", "", linearr[1]);
		name = trim(linearr[2]) ? trim(linearr[2]) : "Instruction not yet implemented";
		args = trim(linearr[3]);
		opcode = trim(linearr[4]);
		split(trim(gensub(".\\)$", "", "", linearr[6])), arr, "|");
		name2 = arr[1];
		category = arr[2];
		desc = arr[3];
		exceptions = arr[4] ? arr[4] : "Missing info";
		stack = arr[5] ? arr[5] : "Missing info";
		variant = arr[6] ? arr[6] : "N/A";

# 		print token "/" opcode "::" name "::" name2 " ---> " desc "/" variant > "/dev/stderr";
		xml_doc_line(token, name, args, opcode, name2, category, desc, exceptions, stack, variant);
	}
	xml_doc_footer();
}
