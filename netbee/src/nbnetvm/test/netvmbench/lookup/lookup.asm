segment .ports
	push_input in1
	push_output out1
ends

segment .metadata
	.netpe_name Lookup
	.datamem_size 0
	.use_coprocessor lookup
ends

segment .init
	.locals 0
	.maxstacksize 10

	LOOKUP_INIT equ 0
	LOOKUP_ADD_DATA equ 1
	LOOKUP_ADD_VALUE equ 2
	LOOKUP_READ_VALUE equ 3
	LOOKUP_RESET equ 4

	LOOKUP_DATA_REG equ 0

	;initialization

	copro.invoke	lookup, LOOKUP_INIT ;init data

	push 0
	copro.out		lookup, LOOKUP_DATA_REG
	copro.invoke	lookup, LOOKUP_ADD_DATA

	push 4
	copro.out		lookup, LOOKUP_DATA_REG
	copro.invoke	lookup, LOOKUP_ADD_VALUE
	
	push 1
	copro.out		lookup, LOOKUP_DATA_REG
	copro.invoke	lookup, LOOKUP_ADD_DATA

	push 2
	copro.out		lookup, LOOKUP_DATA_REG
	copro.invoke	lookup, LOOKUP_ADD_VALUE

	ret
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop

	;Read first byte
	push 0
	upload.8

	;invoke coprocessor to retreive value
	copro.out		lookup, LOOKUP_DATA_REG
	copro.invoke	lookup, LOOKUP_READ_VALUE

	copro.in		lookup, LOOKUP_DATA_REG
	push 0
	pstore.8
	copro.exbuf		lookup, 0

	pkt.send 		out1
	ret
ends

segment .pull
	.maxstacksize 0
	.locals 0
	pop
	ret
ends
