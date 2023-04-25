segment .ports
	push_input in1
	push_output out1
ends

segment .init
	ret
ends

segment .metadata
	.netpe_name VeryBigSwitchTest
	.datamem_size 0
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop

	push 0
	spload.32

	switch 29:
		10: case10
		11: case11
        12: case12
        13: case13
        14: case14
        15: case15
        16: case16
        17: case17
        18: case18
        19: case19
		1: case1
		2: case2
		3: case3
		4: case4
		5: case5
		6: case6
		7: case7
		8: case8
		9: case9
		20: case20
		21: case21
		22: case22
		23: case23
		24: case24
		25: case25
		26: case26
		27: case27
		28: case28
		29: case29
		default: discard

case1 : 
	push 1
	push 0
	pstore.32
	jump.w send

case2 : 
	push 2
	push 0
	pstore.32
	jump.w send

case3 : 
	push 3
	push 0
	pstore.32
	jump.w send

case4 : 
	push 4
	push 0
	pstore.32
	jump.w send

case5 : 
	push 5
	push 0
	pstore.32
	jump.w send

case6 : 
	push 6
	push 0
	pstore.32
	jump.w send

case7 : 
	push 7
	push 0
	pstore.32
	jump.w send

case8 : 
	push 8
	push 0
	pstore.32
	jump.w send


case9 : 
	push 9
	push 0
	pstore.32
	jump.w send

case10 :
	push 10
	push 0
	pstore.32
	jump.w send

case11 :
	push 11
	push 0
	pstore.32
	jump.w send

case12 :
	push 12
	push 0
	pstore.32
	jump.w send

case13 :
	push 13
	push 0
	pstore.32
	jump.w send

case14 :
	push 14
	push 0
	pstore.32
	jump.w send

case15 :
	push 15
	push 0
	pstore.32
	jump.w send

case16 :
	push 16
	push 0
	pstore.32
	jump.w send

case17 :
	push 17
	push 0
	pstore.32
	jump.w send

case18 :
	push 18
	push 0
	pstore.32
	jump.w send

case19 :
	push 19
	push 0
	pstore.32
	jump.w send

case20 :
	push 20
	push 0
	pstore.32
	jump.w send

case21 :
	push 21
	push 0
	pstore.32
	jump.w send

case22 :
	push 22
	push 0
	pstore.32
	jump.w send

case23 :
	push 23
	push 0
	pstore.32
	jump.w send

case24 :
	push 24
	push 0
	pstore.32
	jump.w send

case25 :
	push 25
	push 0
	pstore.32
	jump.w send

case26 :
	push 26
	push 0
	pstore.32
	jump.w send

case27 :
	push 27
	push 0
	pstore.32
	jump.w send

case28 :
	push 28
	push 0
	pstore.32
	jump.w send

case29 :
	push 29
	push 0
	pstore.32
	jump.w send

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
