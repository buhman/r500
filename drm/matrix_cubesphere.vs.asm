--
-- dot(m[0], v), dot(m[1], v), dot(m[2], v), dot(m[3], v)
--

-- input[0] -- position
-- input[1] -- texture
-- input[2] -- normal

-- consts[0] -- trans
-- consts[4] -- world_trans
-- consts[8] -- normal_trans

-- out[0] -- position clip space
-- out[1] -- texture
-- out[2] -- normal
-- out[3] -- object position world space
-- out[4] -- light position world space

-- position clip space
temp[1].x   = VE_DOT   const[0].xyzw    input[0].xyzw ;
temp[1].y   = VE_DOT   const[1].xyzw    input[0].xyzw ;
temp[1].z   = VE_DOT   const[2].xyzw    input[0].xyzw ;
temp[1].w   = VE_DOT   const[3].xyzw    input[0].xyzw ;

-- position world space
temp[2].x   = VE_DOT   const[4].xyzw    input[0].xyzw ;
temp[2].y   = VE_DOT   const[5].xyzw    input[0].xyzw ;
temp[2].z   = VE_DOT   const[6].xyzw    input[0].xyzw ;
temp[2].w   = VE_DOT   const[7].xyzw    input[0].xyzw ;

temp[3].x   = VE_DOT   const[8].xyz0    input[2].xyz0 ;
temp[3].y   = VE_DOT   const[9].xyz0    input[2].xyz0 ;
temp[3].z   = VE_DOT  const[10].xyz0    input[2].xyz0 ;
--temp[3].xyzw = VE_MAX input[2].xyz0 input[2].xyz0 ;

out[0].xyzw = VE_MAX    temp[1].xyzw     temp[1].xyzw ; -- position clip space
out[1].xyzw = VE_MAX   input[1].xyzw    input[1].xyzw ; -- texture
out[2].xyzw = VE_MAX    temp[3].xyz0     temp[3].xyz0 ; -- normal
out[3].xyzw = VE_MAX    temp[2].xyzw     temp[2].xyzw ; -- position world space
out[4].xyzw = VE_MAX  const[12].xyzw   const[12].xyzw ; -- light position world space
