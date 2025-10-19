; CONST[0] = {0.159155, 0.5, 6.283185, -3.141593}
; CONST[1] = {rotate, _, _, _}

; each instruction is only allowed to use a single unique `const` address
;
; instructions may use multiple `temp` addresses, so const[1] is moved to
; temp[0]:
;
temp[0].x    = VE_ADD  const[1].x___  const[1].0___


; ME_SIN and ME_COS are clamped to the range -π to +π prior to the sin/cos
; calculation.
;
; This 3-instruction sequence linearly remaps the range [-∞,+∞] to [-π,+π]
temp[1].x    = VE_MAD   temp[0].x___  const[0].x___  const[0].y___
temp[2].x    = VE_FRC   temp[1].x___
temp[3].x    = VE_MAD   temp[2].x___  const[0].z___  const[0].w___

temp[4].x    = ME_SIN   temp[3].___x
temp[4].y    = ME_COS   temp[3].___x

; with sin and cos calculated, this now ordinary two-dimensional rotation:
;
;  x = position.x * cost - position.y * sint
;  y = position.x * sint + position.y * cost
;  z = 0
;  w = 1
temp[5].xy   = VE_MUL  input[0].yy__   temp[4].xy__
 out[0].xyzw = VE_MAD  input[0].xx01   temp[4].yx01  temp[5].-xy00
