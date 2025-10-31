-- temp[0] -- texture
-- temp[1] -- normal
-- temp[2] -- (world space) fragment position
-- temp[3] -- (world space) light position

-- PIXSIZE 4

TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[0].rgba = LD tex[0].rgba temp[0].rgaa ;

-- normalize:
-- v * 1.0f / sqrt(dot(v, v))

-- norm = normalize(Normal)
src0.rgb = temp[1] :
                DP3 src0.rgb src0.rgb ,
  temp[1].a   = DP ;
src0.a = temp[1] :
  temp[1].a   = RSQ src0.a ;
src0.rgb = temp[1] ,
src0.a = temp[1] :
  temp[1].rgb = MAD src0.rgb src0.aaa src0.000 ;

-- temp[2] -- (world space) fragment position
-- temp[3] -- (world space) light position
-- lightDir = normalize(lightPos - fragPos)
-- srcp.rgb = (src1.rgb - src0.rgb)
src1.rgb = temp[3] ,
src0.rgb = temp[2] ,
srcp.rgb = neg :
              DP3 srcp.rgb srcp.rgb ,
  temp[3].a = DP ;
src0.a = temp[3] :
  temp[3].a   = RSQ src0.a ;
src0.rgb = temp[3] ,
src0.a = temp[3] :
  temp[3].rgb = MAD src0.rgb src0.aaa src0.000 ;

-- diff = dot(norm, lightDir)
-- diff = dot(temp[1].rgb, temp[3].rgb)
src0.rgb = temp[1] ,
src1.rgb = temp[3] :
  temp[4].r = DP3 src0.rgb src1.rgb ;

src0.rgb = temp[4] :
  temp[4].r = MAX src0.r00 src0.000 ;

OUT TEX_SEM_WAIT
src0.a = temp[0], src0.rgb = temp[0] ,
src1.rgb = temp[4] ,
src2.rgb = temp[1] :
  out[0].a    = MAD src0.a src1.1 src1.0 ,
  out[0].rgb  = MAD src0.rgb src1.rrr src1.000 ;
  --out[0].rgb  = MAD src2.rgb src2.100 src1.000 ;
  --out[0].rgb  = MAD src2.r00 src1.rrr src1.000 ;
