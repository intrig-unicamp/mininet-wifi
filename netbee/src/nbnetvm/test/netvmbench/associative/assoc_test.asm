; assoc_test.asm
; Test program for the associative coprocessor.
;
; The associative coprocessor works somewhat like a TCAM+SRAM.
;
; It supports 2 kind of operations: insertions and lookups.
; For an insertion, you specify the key length (in bytes) and the value to associate
; with that key. The value has an associated length as well.
; The lookup operation wants a key and outputs the associated value (if any).
; If the exact key specified has not been inserted, a longest prefix match algorithm
; is used to extract a value from the coprocessor.
; Many tables should be supported by this coprocessor.
; The coprocessor memory is empty when the NetVM is instatiated.
;
; Bounds and limitations:
; - key maximum length: 32 bytes;
; - value maximum length: 32 bytes;
; - keys and values are 8-bit aligned;
; - only one table is present at the moment;
; - endianess issues are pending.
;
; Register map:
;                                    3
;  0                                 2
; +--------+--------+--------+--------+
; | Table index						  | r00
; +--------+--------+--------+--------+
; | Key length (in bytes)			  | r01
; +--------+--------+--------+--------+
; | Value length (in bytes)			  | r02
; +--------+--------+--------+--------+
; | Key bytes						  | r03
;	...
; |									  | r10
; +--------+--------+--------+--------+
; | Value bytes						  | r11
;   ...
; |									  | r19
; +--------+--------+--------+--------+
;
; Invoke opcodes:
; 0		- performs a reset of the associative memory
; 1		- performs an insertion
; 2		- performs a lookup
;
; Test program description:
; This program read the first 4-bytes in the exchange buffer (packet memory),
; does a lookup in the associative coprocessor and writes the result back into
; the first 4 bytes of packet memory.
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
	.maxstacksize 10
	.datamemsize        0
	
	; coprocessor intialization
	push 0
	copro.out		associative, 0
	
	; adds the mapping 192.168.1 -> 192.168.1.1
	push 0
	
	push 192							; first byte
	push 24
	shl
	or
	
	push 168							; second byte
	push 16
	shl
	or
	
	push 1								; third byte
	push 8
	shl
	or
	
	dup
	copro.out		associative, 3		; writes the key
	
	push 1
	or
	copro.out		associative, 11		; writes the value
	
	push 3
	copro.out		associative, 1		; sets the key length
	push 4
	copro.out		associative, 2		; sets the value length
	
	copro.invoke	associative, 1		; does the insertion

	; adds the mapping [default] -> 10.0.0.1
	push 0
	
	push 10								; sets the first byte
	push 24
	shl
	or
	
	push 1								; sets the last byte
	or
	
	copro.out		associative, 11		; writes the value
	
	push 0
	copro.out		associative, 1		; writes the key length
										; the value length is the same as before
	copro.invoke	associative, 1		; does another insertion
	ret
ends

segment .push
	.locals 0
	.maxstacksize 10

	pop
	
	; coprocessor setup
	push 0
	copro.out		associative, 0		; selects the first available table
	
	push 4
	copro.out		associative, 1		; our keys are always 4 bytes long

	; lookup stage
	push 0
	spload.32							; loads the first 4 bytes from packet memory
	copro.out		associative, 3		; puts those bytes in the coprocessor register
	copro.invoke	associative, 2		; does the look-up
	
	; write back stage
	copro.in		associative, 11		; reads the value from the coprocessor register
	push 0
	pstore.32							; writes those bytes at offset 0 in the packet memory
	
	; outputs the packet
	pkt.send		out1

	ret
ends

; ---- Not used - this NetPE works in 'push' mode ----
segment .pull
.locals 0
.maxstacksize 10
	pop
	ret
ends;
