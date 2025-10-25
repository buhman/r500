-- CONST[0] = { time, 0, 0, 0 }
-- CONST[1] = { PI_2, I_PI_2, 0, 0 },

-- d = length(uv)
src0.rgb = temp[0] :
  temp[0].r    = DP3 src0.rg0 src0.rg0 ;
src0.rgb = temp[0] :
  temp[0].a    = RSQ |src0.r| ;
src0.a   = temp[0] :
  temp[0].a    = RCP src0.a ;

-- i = vec3(0.25, 0.40625, 0.5625);
-- td = time + d
src0.a = float(40)   , -- 0.25
src1.a = float(45)   , -- 0.40625
src2.rgb = float(49) : -- 0.5625
  temp[0].rgb = MAD src0.a10 src1.1a0 src2.00r ;

src0.rgb = const[0]  ,
src2.a = temp[0] :
  temp[1].a   = MAD src0.1 src0.r src2.a ;

-- t = i + vec3(td)
src0.a   = temp[1] ,
src0.rgb = temp[0] :
  temp[0].rgb = MAD src0.111 src0.rgb src0.aaa ;

-- j = fract(t)
src0.rgb = temp[0] :
  temp[0].rgb = FRC src0.rgb ;

-- k = cos(j * 2π)
src0.rgb = temp[0] :
              COS src0.r ,
  temp[0].r = SOP ;
src0.rgb = temp[0] :
              COS src0.g ,
  temp[0].g = SOP ;
src0.rgb = temp[0] :
              COS src0.b ,
  temp[0].b = SOP ;

-- l = k * vec3(0.5, 0.5, 0.5) + vec3(0.5, 0.5, 0.5)
src0.rgb = temp[0] ,
src1.rgb = float(48) : -- 0.5
  temp[0].rgb = MAD src0.rgb src1.rrr src1.rrr ;

OUT TEX_SEM_WAIT
src0.a   = temp[0] ,
src0.rgb = temp[0] :
  out[0].a     = MAX src0.1 src0.1 ,
  out[0].rgb   = MAX src0.rgb src0.rgb ;
