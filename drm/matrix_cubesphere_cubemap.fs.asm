-- temp[0] -- position (world space)
-- temp[1] -- normal
-- temp[2] -- light pos (world space)
-- temp[3] -- texture

-- PIXSIZE 4

TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[3].rgba = LD tex[0].rgba temp[0].rgba ;

OUT TEX_SEM_WAIT
src0.rgb = temp[3] ,
src0.a = temp[3] :
  out[0].a    = MAX src0.a src0.a ,
  out[0].rgb  = MAX src0.rgb src0.rgb ;
