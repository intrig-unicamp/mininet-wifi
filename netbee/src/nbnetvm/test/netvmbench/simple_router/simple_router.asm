; simple_router.asm
; Implements a simple routing application.
;
; Reads a complete packet (EthernetII + IP + Payload).
; Checks for payload type (if not IP, drop it).
; Checks the destination address to find a next hop with a routing table lookup. This lookup
;	uses an associative memory (TCAM-like) to do a longest prefix match.
; The next hop is specified by its source/dest. MAC addresses and ethertype.
; Modifies the packet with the correct src/dst mac addresses.
;
; Needed lookups and way of doing them:
; 16 bits -> 1 bit (true/false): packet memory check for the correct Ethertype	| jcmp.eq
; 32 bits -> 112 bits (IP to Ethernet header): associative memory				| associative coproc.
;
; written by Pierluigi Rolando, 2007-05-08

segment .coprocessors
	use_coprocessor associative
ends

segment .ports
	push_input in1
	push_output out1
ends

segment .init
	.maxstacksize	10
	.locals			1
	set.mem			0
	
	; coprocessor intialization
	push 0
	copro.out		associative, 0		; using table 0
	
	; destination address range 20.1.1.0/24 ->
	;						Ethertype	0x8000
	;						dst MAC		00:11:22:33:44:55
	;						src MAC		00:01:02:03:04:05
	; 4 bytes -> 20 bytes, beacause we store it internally this way:
	; 0x00000800	Ethertype							line 11
	; 0x00112233	dst MAC, part 0							 12
	; 0x44550001	dst MAC, part 1	+ src MAC part 0		 13
	; 0x02030405	src MAC, part 1							 14
	
	push 20.1.1.0	
	copro.out		associative, 3		; writes the key	
	push 3
	copro.out		associative, 1		; writes the key length as well
	
	; Now we save the value
	; First goes the EtherType
	push 0x0800							; WRONG VALUE DUE TO ENDIANESS ISSUES (???)
	copro.out		associative, 11

	push 00:11:22:33
	push 44:55:00:01
	push 02:03:04:05			

	copro.out		associative, 14
	copro.out		associative, 13
	copro.out		associative, 12

	push 20								; load the value size as well
	copro.out		associative, 2
		
	copro.invoke	associative, 1		; does the insertion

	; destination address 20.2.2.2 (/32, single address) ->
	;						Ethertype	0x8000
	;						dst MAC		00:aa:bb:cc:dd:ee
	;						src MAC		00:01:02:03:04:05
	; 4 bytes -> 20 bytes, beacause we store it internally this way:
	; 0x00000800	Ethertype							line 11
	; 0x00aabbcc	dst MAC, part 0							 12
	; 0xddee0001	dst MAC, part 1	+ src MAC part 0		 13
	; 0x02030405	src MAC, part 1							 14

	push 20.2.2.2	
	copro.out		associative, 3		; writes the key
	push 4
	copro.out		associative, 1		; writes the key length as well
	
	push 0x0800							; ethertype
	copro.out		associative, 11
	
	push 00:aa:bb:cc
	push dd:ee:00:01
	push 02:03:04:05
	
	copro.out		associative, 14
	copro.out		associative, 13
	copro.out		associative, 12

	push 20								; load the value size as well
	copro.out		associative, 2
		
	copro.invoke	associative, 1		; does the insertion

	ret
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop

	; Read the EtherType field
	push 12
	upload.16
	push 0x0800
	jcmp.neq END

	; Now we load the destination IP address and do the lookup
	push 14								; beginnning of the payload
	push 16								; offset of the destination address in the IP header
	add
	spload.32
	
	; Prepare the coprocessor
	push 0
	copro.out		associative, 0		; table selector
	push 4
	copro.out		associative, 1		; key length
	copro.out		associative, 3		; the key itself
	
	; Do the lookup and fetch the result
	copro.invoke	associative, 2

	copro.in		associative, 2
	push 0
	jcmp.eq END		; if we had no match, not even with the longest prefix algorithm,
					; then we drop the packet
					
	; Update the EthernetII header
	copro.in		associative, 12		; dst address
	push 0
	pstore.32
	
	copro.in		associative, 13		; dst + src
	push 4
	pstore.32
	
	copro.in		associative, 14		; src address
	push 8
	pstore.32
	
	copro.in		associative, 11		; EtherType
	push 12
	pstore.16

	; Packet forwarding stage
	pkt.send	out1

END:
	ret
ends

; ---- Not used - this NetPE works in 'push' mode ----
segment .pull
.locals 0
.maxstacksize 10
	pop
	ret
ends;
