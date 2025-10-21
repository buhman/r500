-- d = length(uv)
src0.rgb = temp[0] :
  temp[0].r    = DP3 src0.rg0 src0.rg0 ;
src0.rgb = temp[0] :
  temp[0].a    = RSQ |src0.r| ;
src0.a   = temp[0] :
  temp[0].a    = RCP src0.a ;

-- d = d * 1.625 + time
src0.a = temp[0],
src1.a = float(61), -- 1.625
src2.rgb = const[0] :
  temp[0].a    = MAD src0.a src1.a src2.r ;

-- d = frc(d)
src0.a = temp[0] :
  temp[0].a    = FRC src0.a ;

-- d = cos(d * 2π)
OUT TEX_SEM_WAIT
src0.a = temp[0] :
                 COS src0.a ,
  out[0].rgb   = SOP ;
