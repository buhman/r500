-- temp[0] -- color

OUT TEX_SEM_WAIT
src0.rgb = temp[0] :
  out[0].a    = MAX src0.1 src0.1 ,
  out[0].rgb  = MAX src0.rgb src0.rgb ;
