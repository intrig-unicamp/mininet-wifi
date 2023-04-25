; hello_world.asm
; Reads a hello world packet and processes it.
; +--------+--------+--------+
; | type   | value			 |
; +--------+--------+--------+
;  0      8
; If type is 0x00, let the packet through, unmodified
; If type is 0x01, increment value and forward the packet
; If type is 0x02, decrement value and forward the packet
; Otherwise drop the packet
;
; written by Pierluigi Rolando, 2007-05-04

segment .ports
	push_input in1
	push_output out1
ends

segment .init
	ret
ends

segment .metadata
	.netpe_name HelloWorld
	.datamem_size        0
ends

segment .push
	.locals 0
	.maxstacksize 10
	
	pop					; pops the input port number
						; (not needed here, could be used to decide which pipe the
						; packet belongs to)

	push 0				; loads the first byte from packet memory
	upload.8

	; switch-like construct.
	; The dups are needed because the jcmp.eq eats both of its operands
	dup
	push 0
	jcmp.eq FORWARD		; 0x00 -> no op

	dup
	push 1
	jcmp.eq INCREMENT	; 0x01 -> increment value

	dup
	push 2
	jcmp.eq DECREMENT	; 0x02 -> decrement value

	; otherwis, simply drop the packet
	ret

INCREMENT:
	push 1				; loads the second and third bytes of the packet onto the stack
	upload.16
	inc1				; increments the top-of-the-stack value
	jump.w WRITEBACK

DECREMENT:
	push 1
	upload.16
	dec1				; decrements the top-of-the-stack value

WRITEBACK:				; writes the updated value to the second and third bytes of pkt mem.
	push 1
	pstore.16

FORWARD:				; the packet is output on the first (and only) output port.
	pkt.send out1
ends

; ---- Not used - this NetPE works in 'push' mode ----
segment .pull
.locals 0
.maxstacksize 10
	pop
	ret
ends;
