segment .data
	.byte_order little_endian

	pgroups			dw	6


	; --- REGEXP DATA ---
	regexps			dw	4

	; --- RegExp 0, belonging to rule 1 ---
	lfre00000		dw	4
	fre00000		db	"smiR"
	lre00000		dw	2
	pre00000		db	".."

	; --- RegExp 1, belonging to rule 2 ---
	lfre00001		dw	3
	fre00001		db	"smi"
	lre00001		dw	4
	pre00001		db	"HttP"

	; --- RegExp 2, belonging to rule 3 ---
	lfre00002		dw	3
	fre00002		db	"smi"
	lre00002		dw	4
	pre00002		db	"USER"

	; --- RegExp 3, belonging to rule 3 ---
	lfre00003		dw	4
	fre00003		db	"smiR"
	lre00003		dw	4
	pre00003		db	" ftp"

ends

segment .metadata
		.netpe_name ProvaRegexp
		.datamem_size 10240		; WARNING: Make sure this is enough to hold match data!
		.use_coprocessor regexp
ends

segment .ports
		push_input in		; 0
		push_output out		; 1
ends

segment .init
		.maxstacksize 5
	copro.init regexp, regexps
		ret

ends

segment .push
		.locals 4
		.maxstacksize 50

		pop
		;copro.exbuf  regexp, 999
		;push 0
		;copro.out 	 regexp, 0
		;copro.invoke regexp, 1
		;copro.invoke regexp, 3
		;push 16
		;copro.out 	 regexp, 1
		;copro.invoke regexp, 2
		;copro.invoke regexp, 3
		
ret
ends
segment .pull
		.locals 4
		.maxstacksize 50

		pop
ret
ends
