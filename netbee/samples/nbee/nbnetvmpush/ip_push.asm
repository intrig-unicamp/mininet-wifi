ethertype_offset	equ	12
ethertype_ip		equ	0x0800

ip_start			equ	14
ip_src_offset		equ	12


;														
; the port table of this PE							
;														
segment .ports											
	push_input in1		; decalre a push input port
    push_output out1	; declare a push output port
ends													


;														
; this segment contains initialization code for this PE
;														
segment .init
        set.mem        2048             ; allocate the data memory 
        ret                             ; end of init section                    
ends													


;														
; this is the push segment.							
;														
segment .push											
.locals 5
.maxstacksize 10
        pop
		push 12
		upload.16
    	push 2048 				;0x800
    	jcmp.eq send_packet
		ret
	
send_packet:
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
