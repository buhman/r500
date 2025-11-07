-- const[0-3]: transform matrix
-- const[4]: particle_position
-- const[5]: dx
-- const[6]: dy
-- input[0]: position coordinate
-- input[1]: texture coordinate

--
-- dot(m[0], v), dot(m[1], v), dot(m[2], v), dot(m[3], v)
--

-- ppos = particle_position
temp[0].xyz  = VE_ADD  const[4].xyz_  const[4].000_ ;

-- ppos = position.xxx * dx + ppos
temp[0].xyz  = VE_MAD  input[0].xxx_  const[5].xyz_ temp[0].xyz_ ;
-- ppos = position.yyy * dy + ppos
temp[0].xyz  = VE_MAD  input[0].yyy_  const[6].xyz_ temp[0].xyz_ ;

-- scale
temp[0].xyzw = VE_MUL  temp[0].xyz1   temp[0].1111 ;

-- ppos = transform_matrix * ppos
temp[1].x    = VE_DOT  const[0].xyzw  temp[0].xyzw ;
temp[1].y    = VE_DOT  const[1].xyzw  temp[0].xyzw ;
temp[1].z    = VE_DOT  const[2].xyzw  temp[0].xyzw ;
temp[1].w    = VE_DOT  const[3].xyzw  temp[0].xyzw ;

out[0].xyzw  = VE_MAD   temp[1].xyzw   temp[1].1111 temp[1].0000 ;
out[1].xyzw  = VE_MAX  input[1].xyzw  input[1].xyzw ;
