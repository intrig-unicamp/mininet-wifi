segment .ports
	push_input in1
	push_output out1
ends

segment .init
	ret
ends

segment .metadata
	.netpe_name BigSwitchTest
	.datamem_size 0
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop

	push 0
	spload.32

	switch 12:
		8: case0
		16: case0
		33: case0
		37: case0
		41: case0
		60: case0
		144: case1
		264: case1
		291: case1
		1032: case2
		2048: case2
		2082: case3
		default: discard
	
case0:
	push 0
	push 0
	pstore.32
	jump.w send
	
case1:
	push 1
	push 0
	pstore.32
	jump.w send

case2:
	push 2
	push 0
	pstore.32
	jump.w send

case3:
	push 3
	push 0
	pstore.32

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
