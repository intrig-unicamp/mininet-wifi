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
	push 1 
	spload.32
	jump.w NEXT1

NEXT1:
	pop
	
	inc1

	push 1
	push 1
	jcmp.eq NEXT

NEXT:	
	push 1
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
