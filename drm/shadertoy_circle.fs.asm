-- d = length(uv)
src0.rgb = temp[0] :
  temp[0].r    = DP3 src0.rg0 src0.rg0 ;
src0.rgb = temp[0] :
  temp[0].a    = RSQ |src0.r| ;
NOP
src0.a   = temp[0] :
  temp[0].a    = RCP src0.a ;

-- d = abs(d - 0.5) * 1 + -0.1
src0.a = float(48), -- 0.5
src1.a = temp[0],   -- d
src2.a = float(29), -- 0.1015625
srcp.a = sub :      -- (src1.a - src0.a)
  temp[0].a    = MAD |srcp.a| src0.1 -src2.a ;

-- d = (d >= 0.0) ? 1.0 : 0.0
-- out.rgba = vec4(d, 0, 0, 1)
OUT TEX_SEM_WAIT
src0.a = temp[0] :
  out[0].a    = MAX src0.1 src0.1 ,
  out[0].rgb  = CMP src0.100 src0.000 src0.a00 ;
