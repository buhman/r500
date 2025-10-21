# CONSTS[0] = (-0.1, 0, 0, 0)

# d = length(uv)
src0.rgb = temp[0]  :
  temp[0].r    = DP3 src0.rg0 src0.rg0 ;
src0.rgb = temp[0]  :
  temp[0].a    = RSQ |src0.r| ;
src0.a   = temp[0]  :
  temp[0].a    = RCP src0.a ;

# d -= 0.5
src0.a   = temp[0]  :
  temp[0].r    = MAD src0.a00 src0.100 -src0.h00 ;

# d = abs(d) * 1 + -0.1
src0.rgb = temp[0] , src1.rgb = const[0] :
  temp[0].r    = MAD |src0.r00| src0.100 src1.r00 ;

# out.r = (d >= 0.0) ? 1.0 : 0.0
OUT
src0.rgb = temp[0]  :
  out[0].r    = CMP src0.100 src0.000 src0.r00 ;

# out.a = 1
# out.gb = vec2(0, 0)
OUT TEX_SEM_WAIT
 :
  out[0].a    = MAX src0.1 src0.1 ,
  out[0].gb   = MAX src0.000 src0.000 ;
