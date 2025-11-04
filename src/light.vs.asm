-- input[0] -- position
-- input[1] -- color

-- position clip space
temp[1].x   = VE_DOT   const[0].xyzw    input[0].xyzw ;
temp[1].y   = VE_DOT   const[1].xyzw    input[0].xyzw ;
temp[1].z   = VE_DOT   const[2].xyzw    input[0].xyzw ;
temp[1].w   = VE_DOT   const[3].xyzw    input[0].xyzw ;

-- position (clip space)
out[0].xyzw = VE_ADD    temp[1].xyzw    const[0].0000 ;
-- color
out[1].xyzw = VE_ADD   const[4].xyzw    const[4].0000 ;
