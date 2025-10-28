-- CONST[0] { -1/D, 1/D, -2/D, 2/D }
-- CONST[1] { -3/D, 3/D,    _,   _ }
-- CONST[2] { 0.2460, 0.2050, 0.1171, 0.0439 }

-- uv1 = vec4(vec2(uv0.x + const[1].x, uv0.y),
--            vec2(uv0.x + const[1].y, uv0.y))
src0.rgb = temp[0] ,
src1.rgb = const[0] :
  temp[1].rgb  = MAD src0.rgr src0.111 src1.r0g ,
  temp[1].a    = MAD src0.g   src0.1   src1.0   ;

-- uv2 = vec4(vec2(uv0.x + const[1].x, uv0.y),
--            vec2(uv0.x + const[1].y, uv0.y))
src0.rgb = temp[0] ,
src1.rgb = const[0] :
  temp[2].rgb  = MAD src0.rgr src0.111 src1.b0a ,
  temp[2].a    = MAD src0.g   src0.1   src1.0   ;

-- uv3 = vec4(vec2(uv0.x + const[1].x, uv0.y),
--            vec2(uv0.x + const[1].y, uv0.y))
src0.rgb = temp[0] ,
src1.rgb = const[1] :
  temp[3].rgb  = MAD src0.rgr src0.111 src1.r0g ,
  temp[3].a    = MAD src0.g   src0.1   src1.0   ;

-- s1n = texture2D(tex, uv1n)
-- s1p = texture2D(tex, uv1p)
TEX
  temp[4].rgba = LD tex[0].rgba temp[1].rgaa ;
TEX
  temp[5].rgba = LD tex[0].rgba temp[1].baaa ;

-- s2n = texture2D(tex, uv2n)
-- s2p = texture2D(tex, uv2p)
TEX
  temp[6].rgba = LD tex[0].rgba temp[2].rgaa ;
TEX
  temp[7].rgba = LD tex[0].rgba temp[2].baaa ;

-- s3n = texture2D(tex, uv3n)
-- s3p = texture2D(tex, uv3p)
TEX
  temp[8].rgba = LD tex[0].rgba temp[3].rgaa ;
TEX
  temp[9].rgba = LD tex[0].rgba temp[3].baaa ;

-- s0 = texture2D(tex, uv0)
TEX TEX_SEM_ACQUIRE TEX_SEM_WAIT
  temp[0].rgba = LD tex[0].rgba temp[0].rgaa ;

-- col = s0 * weight[2] + 0
TEX_SEM_WAIT
src0.rgb = temp[0] ,
src1.rgb = const[2] ,
src2.rgb = temp[0] :
  temp[0].rgb  = MAD src0.rgb src1.rrr src2.000 ;

-- col = s1p * weight[2] + col
src0.rgb = temp[4] ,
src1.rgb = const[2] ,
src2.rgb = temp[0] :
  temp[0].rgb  = MAD src0.rgb src1.ggg src2.rgb ;

-- col = s1p * weight[2] + col
src0.rgb = temp[5] ,
src1.rgb = const[2] ,
src2.rgb = temp[0] :
  temp[0].rgb  = MAD src0.rgb src1.ggg src2.rgb ;

-- col = s2n * weight[2] + col
src0.rgb = temp[6] ,
src1.rgb = const[2] ,
src2.rgb = temp[0] :
  temp[0].rgb = MAD src0.rgb src1.bbb src2.rgb ;

-- col = s2p * weight[2] + col
src0.rgb = temp[7] ,
src1.rgb = const[2] ,
src2.rgb = temp[0] :
  temp[0].rgb  = MAD src0.rgb src1.bbb src2.rgb ;

-- col = s3n * weight[3] + col
src0.rgb = temp[8] ,
src1.a = const[2] ,
src2.rgb = temp[0] :
  temp[0].rgb = MAD src0.rgb src1.aaa src2.rgb ;

-- col = s3p * weight[3] + col
OUT TEX_SEM_WAIT
src0.rgb = temp[9] ,
src1.a = const[2] ,
src2.rgb = temp[0] :
  out[0].rgb  = MAD src0.rgb src1.aaa src2.rgb ,
  out[0].a    = MAD src0.0 src0.0 src0.1 ;
