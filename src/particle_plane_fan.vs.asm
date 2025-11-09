-- const[0-3]: transform matrix
-- const[4].x: -2.0
-- input[0]: texture coordinate

--
-- dot(m[0], v), dot(m[1], v), dot(m[2], v), dot(m[3], v)
--

-- calculate position from texture coordinate
-- x = y * -2 + 1
-- y = x * -2 + 1
temp[1].xyzw = VE_MAD  input[0].yx00  const[4].xx00  input[0].1101 ;

temp[2].x    = VE_DOT  const[0].xyzw   temp[1].xyzw ;
temp[2].y    = VE_DOT  const[1].xyzw   temp[1].xyzw ;
temp[2].z    = VE_DOT  const[2].xyzw   temp[1].xyzw ;
temp[2].w    = VE_DOT  const[3].xyzw   temp[1].xyzw ;

out[0].xyzw  = VE_MAD   temp[2].xyzw   temp[2].1111 temp[2].0000 ;
out[1].xyzw  = VE_MAX  input[0].xy00  input[0].xy00 ;
