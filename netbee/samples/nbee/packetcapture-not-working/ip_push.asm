;
;define constant value decimal or hexadecimal is the same.
;
ethertype_offset	equ	12
ethertype_ip		equ	0x0800

ip_start			equ	14
ip_src_offset		equ	12


; Port Segment														
; the port table of this PE							
;														
segment .ports											
	push_input in1		; decalre a push input port
    push_output out1	; declare a push output port
ends													


; Init Segments														
; this segment contains initialization code for this PE
;														
segment .init
        set.mem        2048             ; allocate the data memory 
        ret                             ; end of init section                    
ends													


; Code Segment														
; this is the push segment.							
;														
segment .push											
.locals 5						;local variables area sizes 
.maxstacksize 10				;define the maximum operand's stack 
        pop						;delete the first element of the stack
		push 12					;push integer 12 in the stack puntatore per le upload successive.
		upload.16				;carica nello stack i due byte a partire dal 12 del pacchetto.
    	push 2048 				;0806 arp pkt
    	jcmp.eq send_packet		;Jump label if the first and the second element of the stack are equal
		ret
	
send_packet:					;Jump label 
		pkt.send out1
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
