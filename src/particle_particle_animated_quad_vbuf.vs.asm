-- const[0-3]  : transform matrix
-- const[4]    : dx
-- const[5]    : dy
---- const[6].xyz: particle_position
-- const[6].w  : scale
-- const[7].x  : -2.0

-- input[0]: particle position
-- input[1]: texture coordinate

--
-- dot(m[0], v), dot(m[1], v), dot(m[2], v), dot(m[3], v)
--

-- calculate position from texture coordinate
-- x = y * -2 + 1
-- y = x * -2 + 1
temp[2].xy   = VE_MAD  input[1].yx__  const[7].xx__  input[1].11__ ;

-- ppos = particle_position
temp[0].xyz  = VE_ADD  input[0].xyz_  input[0].000_ ;

-- ppos = position.xxx * dx + ppos
temp[0].xyz  = VE_MAD   temp[2].xxx_  const[4].xyz_  temp[0].xyz_ ;
-- ppos = position.yyy * dy + ppos
temp[0].xyz  = VE_MAD   temp[2].yyy_  const[5].xyz_  temp[0].xyz_ ;

-- ppos *= scale
temp[0].xyzw = VE_MUL  temp[0].xyz1   const[6].www1 ;

-- ppos = transform_matrix * ppos
temp[1].x    = VE_DOT  const[0].xyzw  temp[0].xyzw ;
temp[1].y    = VE_DOT  const[1].xyzw  temp[0].xyzw ;
temp[1].z    = VE_DOT  const[2].xyzw  temp[0].xyzw ;
temp[1].w    = VE_DOT  const[3].xyzw  temp[0].xyzw ;

out[0].xyzw  = VE_MAD   temp[1].xyzw   temp[1].1111  temp[1].0000 ;
out[1].xyzw  = VE_MAX  input[1].xy00  input[1].xy00 ;
