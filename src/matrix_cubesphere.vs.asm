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

-- normal world space
temp[3].x   = VE_DOT   const[4].xyz0    input[2].xyz0 ;
temp[3].y   = VE_DOT   const[5].xyz0    input[2].xyz0 ;
temp[3].z   = VE_DOT   const[6].xyz0    input[2].xyz0 ;

-- position (clip space)
out[0].xyzw = VE_ADD    temp[1].xyzw    const[0].0000 ;
-- position (world space)
out[1].xyzw = VE_ADD    temp[2].xyzw    const[0].0000 ;
-- normal
out[2].xyzw = VE_ADD    temp[3].xyz0    const[0].0000 ;
-- light pos (world space)
out[3].xyzw = VE_ADD   const[8].xyzw    const[8].0000 ;
-- texture
out[4].xyzw = VE_ADD   input[1].xy00    const[0].0000 ;
