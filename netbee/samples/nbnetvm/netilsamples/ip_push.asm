;
; Define some useful constants
; Both decimal and hexadecimal values are supported.
;
ethertype_offset	equ	12
ethertype_ip		equ	0x0800
ethertype_arp		equ	0x0806

ip_start		equ	14
ip_src_offset		equ	12


; Port Segment
; the port table of this PE
;
segment .ports
	push_input in		; declare a push input port
	push_output out		; declare a push output port
ends

; Metadata Segment
; Info about this netPe
segment .metadata
	.netpe_name PushIp  ; name of this PE
	.datamem_size 2048	; allocate data memory
ends

; Init Segments
; this segment contains initialization code for this PE
;
segment .init
.maxstacksize 50
	ret                             ; end of init section
ends

; Code Segment
; this is the push segment.
;
segment .push
.locals 5					; local variables area sizes
.maxstacksize 10			; define the maximum operand's stack
	pop						; delete the first element of the stack
	push ethertype_offset	; push integer 12 in the stack puntatore per le upload successive.
	upload.16				; carica nello stack i due byte a partire dal 12 del pacchetto.
	push ethertype_arp		; 0806 arp pkt
	jcmp.eq send_packet		; Jump label if the first and the second element of the stack are equal
	ret

send_packet:				;Jump label
	pkt.send out
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
