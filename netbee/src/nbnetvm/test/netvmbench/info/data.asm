segment .ports
	push_input in1
	push_output out1
ends

segment .init
	.maxstacksize 10

	set.mem 10
	set.share 10
	ret
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop

	;Read first byte
	push 0
	upload.8

	;write byte from INFO
	push 0
	mstore.8

	;Read first byte
	push 0
	umload.8

	;write byte from INFO
	push 0
	pstore.8

	;push 0
	;jcmp.eq	send

send:
	pkt.send 		out1
	ret
ends

segment .pull
	.maxstacksize 0
	.locals 0
	pop
	ret
ends

