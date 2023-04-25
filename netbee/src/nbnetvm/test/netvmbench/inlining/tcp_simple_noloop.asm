segment .ports
  push_input in
  push_output out1
ends


segment .init
  ret
ends


segment .pull
  ret
ends


; Code for filter: "tcp.dport == 80 returnpacket on port 1"

segment .push
.maxstacksize 30
.locals 41
  pop  ; discard the "calling" port id

l9:
  ; PROTOCOL startproto: FORMAT
  push 1
  locstore 0  ; $linklayer_t0

  push 0
  locstore 3  ; $currentoffset_t3

  
  
  ; PROTOCOL startproto: ENCAPSULATION
  ; SWITCH '$linklayer'
  jump.w case_l11
case_l11:
  jump.w jump_true_l12
jump_true_l12:
  ; PROTOCOL ethernet: FORMAT
  push 6
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  push 6
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  upload.16
  locstore 20  ; type_value_t20

  locload 3  ; $currentoffset_t3
  push 2
  add
  locstore 3  ; $currentoffset_t3

  
  
  ; PROTOCOL ethernet: ENCAPSULATION
  ; SWITCH 'buf2int(type)'
    locload 20  ; type_value_t20
switch 1:
  2048: case_l15
  default: DISCARD_l6  
case_l15:
  jump.w jump_true_l16
jump_true_l16:
  ; PROTOCOL ip: FORMAT
  locload 3  ; $currentoffset_t3
  locstore 22  ; bit_union_0_ind_t22

  locload 3  ; $currentoffset_t3
  push 1
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  push 1
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  upload.16
  locstore 24  ; tlen_value_t24

  locload 3  ; $currentoffset_t3
  push 2
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  push 2
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  locstore 26  ; bit_union_1_ind_t26

  locload 3  ; $currentoffset_t3
  push 2
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  push 1
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  upload.8
  locstore 28  ; nextp_value_t28

  locload 3  ; $currentoffset_t3
  push 1
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  push 2
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  push 4
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  push 4
  add
  locstore 3  ; $currentoffset_t3

  ; WHEN '$linklayer == 1'
  jump.w if_true_l18
if_true_l18:
if_false_l19:
end_if_l20:
  ; END WHEN
  
  
  ; PROTOCOL ip: ENCAPSULATION
  ; IF 'buf2int(foffset) == 0'
    locload 26  ; bit_union_1_ind_t26
  upload.16
  push 8191
  and
  push 0
  jcmp.neq if_false_l22
if_true_l21:
  ; IF: TRUE
  ; SWITCH 'buf2int(nextp)'
    locload 28  ; nextp_value_t28
switch 2:
  6: case_l25
  17: case_l27
  default: DISCARD_l6  
case_l27:
  jump.w DISCARD_l6
if_false_l22:
end_if_l23:
  ; END IF
  jump.w DISCARD_l6
case_l25:
  jump.w jump_true_l26
jump_true_l26:
  ; PROTOCOL tcp: FORMAT
  locload 3  ; $currentoffset_t3
  push 2
  add
  locstore 3  ; $currentoffset_t3

  locload 3  ; $currentoffset_t3
  locstore 34  ; dport_ind_t34

  ; condition
    locload 34  ; dport_ind_t34
  upload.16
  push 80
  jcmp.eq SEND_PACKET_l7
DISCARD_l6:
  ret
SEND_PACKET_l7:
  pkt.send out1
ends
