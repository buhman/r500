-- const[0-3]: transform matrix
-- input[0]: position coordinate
-- input[1]: texture coordinate

--
-- dot(m[0], v), dot(m[1], v), dot(m[2], v), dot(m[3], v)
--

temp[1].x   = VE_DOT  const[0].xyzw  input[0].xyzw ;
temp[1].y   = VE_DOT  const[1].xyzw  input[0].xyzw ;
temp[1].z   = VE_DOT  const[2].xyzw  input[0].xyzw ;
temp[1].w   = VE_DOT  const[3].xyzw  input[0].xyzw ;

out[0].xyzw = VE_MAX   temp[1].xyzw   temp[1].xyzw ;
out[1].xyzw = VE_MAX  input[1].xyzw  input[1].xyzw ;
