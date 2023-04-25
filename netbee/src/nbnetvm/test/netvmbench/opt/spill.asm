segment .ports
	push_input in1
	push_output out1
ends

segment .init
;	.datamemsize        0
	ret
ends

segment .push
	.locals 35
	.maxstacksize 35

	pop
 	push 0
 	locstore 1
	push 2
	locstore 2
	push 3
	locstore 3
	push 4
	locstore 4
	push 5 
	locstore 5
	push 6
	locstore 6
	push 1
	upload.8
	locstore 7
	push 1
	upload.8
	locstore 8
	push 1
	upload.8
	locstore 9
	push 1
	upload.8
	locstore 10
	push 1
	upload.8
	locstore 11
	push 1
	upload.8
	locstore 12
	push 1
	upload.8
	locstore 13
	push 1
	upload.8
	locstore 14
	push 1
	upload.8
	locstore 15
	push 1
	upload.8
	locstore 16
	push 1
	upload.8
	locstore 17
	push 1
	upload.8
	locstore 18
	push 1
	upload.8
	locstore 19
	push 1
	upload.8
	locstore 20
	push 1
	upload.8
	locstore 21
	push 1
	upload.8
	locstore 22
	push 1
	upload.8
	locstore 23
	push 1
	upload.8
	locstore 24
	push 1
	upload.8
	locstore 25
	push 1
	upload.8
	locstore 26
	push 1
	upload.8
	locstore 27
	push 1
	upload.8
	locstore 28
	push 1
	upload.8
	locstore 29
	push 1
	upload.8
	locstore 30
	
	locload 2
	locload 3

	add

	locload 4

	locload 5

	locload 16

	locload 27

	add

	add

	add

	add

	locload 8

	locload 9

	locload 10

	locload 11

	locload 21

	add

	sub

	sub

	sub

	add

	locload 13

	locload 14

	locload 15

	locload 6

	sub

	add

	add


	locload 17

	locload 18

	sub

	sub

	add

	locload 19

	locload 20

	locload 12

	locload 22

	add

	add

	add

	sub

	locload 23

	locload 24

	locload 25

	locload 26

	locload 7

	sub

	sub

	add

	add

	add

	locload 28

	locload 29

	locload 30

	locload 31

	locload 20

	add

	sub

	sub

	sub

	add

	locload 4

	locload 5

	locload 6

	locload 28

	add

	add

	add

	add

	locload 8

	locload 9

	locload 10

	locload 11

	add

	sub

	sub

	sub

	locload 12

	locload 13

	locload 14

	locload 15

	add

	add

	add

	sub

	locload 7

	locload 17

	locload 18

	locload 19

	locload 3

	sub

	sub

	add

	add

	add

	locload 21

	locload 22

	locload 23

	locload 24

	locload 25

	locload 26

	locload 27

	add

	sub

	sub

	sub

	add

	add

	add

	locload 16

	locload 29

	locload 30

	locload 31

	add

	sub

	sub

	sub



	locload 3

	locload 4

	locload 5

	locload 6

	add

	add

	add

	add

	locload 7

	locload 8

	locload 9

	locload 10

	add

	add

	sub

	sub

	locload 11

	locload 12

	locload 13

	locload 14

	sub

	add

	add

	add

	locload 15

	locload 16

	locload 17

	locload 18

	sub

	sub

	sub

	add

	locload 19

	locload 20

	locload 21

	locload 22

	add

	add

	add

	sub

	locload 23

	locload 24

	locload 25

	locload 26

	sub

	sub

	add

	add

	locload 27

	locload 28

	locload 29

	locload 30

	add

	add

	sub

	sub

	locload 31

	locload 31

	locload 30

	locload 29

	sub

	add

	add

	add

	locload 28

	locload 27

	locload 26

	locload 25

	add

	add

	sub

	sub

	locload 24

	locload 23

	locload 22
	locload 21

	sub

	add

	add


	add

	locload 20

	locload 19

	locload 18

	locload 17

	sub

	sub

	sub

	add

	locload 16

	locload 15

	locload 14

	locload 13

	add

	add

	add

	sub

	locload 12

	locload 11

	locload 10

	locload 9

	sub

	sub

	add

	add

	locload 8

	locload 7

	locload 6

	locload 5

	locload 4

	add

	add

	sub

	sub

	sub

	locload 1
	pstore.32
	pkt.send out1
	ret
ends

; ---- Not used - this NetPE works in 'push' mode ----
segment .pull
.locals 0
.maxstacksize 10
	pop
	ret
ends;
