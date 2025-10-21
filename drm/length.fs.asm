src0.rgb = temp[0] :
  temp[0].r    = DP3 src0.rg0 src0.rg0 ;

src0.rgb = temp[0] :
  temp[0].a    = RSQ |src0.r| ;

OUT
src0.a = temp[0] :
                 RCP src0.a ,
  out[0].r     = SOP ;

OUT TEX_SEM_WAIT
 :
  out[0].a     = MAX src0.1 src0.1 ,
  out[0].gb    = MAX src0.000 src0.000 ;
