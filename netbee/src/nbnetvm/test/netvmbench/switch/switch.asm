segment .ports
	push_input in1
	push_output out1
ends

segment .init
	ret
ends

segment .metadata
	.netpe_name SwitchTest
	.datamem_size 0
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop

	push 0
	upload.8

	switch 4:
		0: case0
		1: case1
		129: case129
		131: case131
		default: discard
	
case0:
	push 1
	push 0
	pstore.8
	jump.w send
	
case1:
	push 129
	push 0
	pstore.8
	jump.w send

case129:
	push 131
	push 0
	pstore.8
	jump.w send

case131:
	push 0
	push 0
	pstore.8

send:
	pkt.send out1
discard:
	ret
ends

segment .pull
	.locals 0
	.maxstacksize 10
	pop
	ret
ends
