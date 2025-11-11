-- temp[0].xy: texture coordinate
-- temp[0].a : age

TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[2].rgb = LD tex[0].rgba temp[0].rgaa ;

-- i = vec3(0.25, 0.40625, 0.5625);
src0.a = float(40)   , -- 0.25
src1.a = float(45)   , -- 0.40625
src2.rgb = float(49) : -- 0.5625
  temp[1].rgb = MAD src0.a10 src1.1a0 src2.00r ;

-- t = i + vec3(td)
src0.a   = temp[0] ,
src0.rgb = temp[1] ,
src1.rgb = temp[0] :
  temp[1].rgb = MAD src1.bbb src0.aaa src0.rgb ;

-- j = fract(t)
src0.rgb = temp[1] :
  temp[1].rgb = FRC src0.rgb ;

-- k = cos(j * 2π)
src0.rgb = temp[1] :
              COS src0.r ,
  temp[1].r = SOP ;
src0.rgb = temp[1] :
              COS src0.g ,
  temp[1].g = SOP ;
src0.rgb = temp[1] :
              COS src0.b ,
  temp[1].b = SOP ;

-- l = k * vec3(0.5, 0.5, 0.5) + vec3(0.5, 0.5, 0.5)
src0.rgb = temp[1] ,
src1.rgb = float(48) : -- 0.5
  temp[1].rgb = MAD src0.rgb src1.rrr src1.rrr ;

OUT TEX_SEM_WAIT
src0.rgb = temp[2] ,
src1.rgb = temp[1] :
  out[0].a    = MAX src0.1 src0.1 ,
  out[0].rgb  = MAD src0.rgb src1.rgb src0.000 ;
