segment .ports
	push_input in1
	push_output out1
ends

segment .metadata
	.netpe_name data1
	.datamem_size 50
ends

segment .init
	ret
ends

segment .push
	.locals 0
	.maxstacksize 10
	
	pop

	; legal counter update
	push 0				; load the address unto the stack
	umload.32			; load from the data memory
;	dup

	push 7
	add					; increment with a constant value

	push 0				; store again at the same offset
	mstore.32
	
;	push 5
;	pstore.32
	
	; illegal memory access
;	push 15
	
;	push 40
;	umload.32
	
;	add
	
;	push 41				; the offset is different than before
;	mstore.32

	ret
ends

; ---- Not used - this NetPE works in 'push' mode ----
segment .pull
.locals 0
.maxstacksize 10
	pop
	ret
ends;
