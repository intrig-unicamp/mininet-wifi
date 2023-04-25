;
; Define some useful constants
; Both decimal and hexadecimal values are supported.
;
ethertype_offset	equ	12
ethertype_ip		equ	0x0800
ethertype_arp		equ	0x0806

ip_start		equ	14
ip_src_offset		equ	12


;
; the port table of this PE
;
segment .ports
	pull_input in		; declare a pull input port
	pull_output out		; declare a pull output port
ends


;
; this segment contains initialization code for this PE
;
segment .init
.maxstacksize 1
        .datamemsize        2048             ; allocate the data memory

        ret                             ; end of init section
ends


;
; this is the push segment.
;
segment .push
.locals 5
.maxstacksize 10
        pop
	ret
ends



;
; this is the pull segment.
;
segment .pull
.locals 5
.maxstacksize 10
	pop

	pkt.receive in
	push ethertype_offset
	upload.16
	push ethertype_ip
	jcmp.eq packet_passed
	exbuf.delete

packet_passed:
	ret
ends
