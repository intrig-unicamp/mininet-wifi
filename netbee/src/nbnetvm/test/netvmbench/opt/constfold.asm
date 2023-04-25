segment .ports
	push_input in1
	push_output out1
ends

segment .init
	.datamemsize        0
	ret
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop
	push 1 
	spload.32
	inc1
	push 1
	inc1
	jump.w NEXT1

NEXT1:
	
	push 2
	jump.w NEXT

NEXT:	
	add
	pstore.32

	pkt.send		out1
	ret
ends

; ---- Not used - this NetPE works in 'push' mode ----
segment .pull
.locals 0
.maxstacksize 10
	pop
	ret
ends;
