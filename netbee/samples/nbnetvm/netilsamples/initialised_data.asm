segment .data
	.byte_order big_endian

	b01	db	0xAA
	b02	db	0xaa
	b03	db	0xBB
	b04	db	0xbb
	b05	db	0xCC
	b06	db	0xcc
	b07	db	0xDD
	b08	db	0xdd
	b09	db	0xEE
	b10	db	0xee
	b11	db	0xFF
	b12	db	0xff
	b13	db	0x00
	b14	db	0xC0
	b15	db	0xff
	b16	db	0xEE

	w01	dw	0xAAaa
	w02	dw	0xBBbb
	w03	dw	0xCCcc
	w04	dw	0xDDdd
	w05	dw	0xEEee
	w06	dw	0xFFff
	w07	dw	0x00C0
	w08	dw	0xffEE

	d01	dd	0xAAaaBBbb
	d02	dd	0xCCccDDdd
	d03	dd	0xEEeeFFff
	d04	dd	0x00C0ffEE

	s01	db	"StRiNg-TeSt/1234"
ends

; Port Segment
; the port table of this PE
;
segment .ports
	push_input in		; declare a push input port
	push_output out		; declare a push output port
ends

; Init Segments
; this segment contains initialization code for this PE
;
segment .init
.maxstacksize 50
	.datamemsize        2048             ; allocate the data memory

	ret                             ; end of init section
ends

; Code Segment
; this is the push segment.
;
segment .push
.locals 5				; local variables area sizes
.maxstacksize 10			; define the maximum operand's stack
	pop				; delete the first element of the stack

	; Copy initialised data section to packet buffer
	push 0
	push 0
	push 64
	ipcopy

	; Copy last 32 bytes again, to test offset data section to packet buffer
	push 32
	push 64
	push 32
	ipcopy

send_packet:				;Jump label
	pkt.send out
	ret
ends

;
; this is the pull segment, empty at the moment
;
segment .pull
.locals 5
.maxstacksize 10
        pop
        ret
ends
