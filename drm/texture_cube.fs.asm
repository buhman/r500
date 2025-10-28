TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[0].rgba = LD tex[0].rgba temp[0].rgaa ;

OUT TEX_SEM_WAIT
src0.a = temp[0], src0.rgb = temp[0] :
  out[0].a    = MAX src0.a src0.a ,
  out[0].rgb  = MAX src0.rgb src0.rgb ;
