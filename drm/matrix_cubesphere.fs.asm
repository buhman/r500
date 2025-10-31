-- temp[0] -- position (world space)
-- temp[1] -- normal
-- temp[2] -- light pos (world space)
-- temp[3] -- texture

-- PIXSIZE 4

TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[3].rgba = LD tex[0].rgba temp[3].rgaa ;

-- normal = normalize(normal)
-- normal = (1.0 / sqrt(dot(normal, normal))) * normal
src0.rgb = temp[1] :
              DP3 src0.rgb src0.rgb ,
  temp[1].a = DP ;
src0.a = temp[1] :
  temp[1].a   = RSQ |src0.a| ;
src0.a = temp[1], src0.rgb = temp[1] :
  temp[1].rgb = MAD src0.rgb src0.aaa src0.000 ;

-- light_dir = normalize((f_light_pos - f_world_pos))
src1.rgb = temp[2] , -- f_light_pos
src0.rgb = temp[0] , -- f_world_pos
srcp.rgb = neg :     -- (f_light_pos - f_world_pos)
              DP3 srcp.rgb srcp.rgb ,
  temp[2].a = DP ;
src0.a = temp[2] :
  temp[2].a   = RSQ |src0.a| ;
src0.a = temp[2], src0.rgb = temp[2] :
  temp[2].rgb = MAD src0.rgb src0.aaa src0.000 ;

-- dot(normal, light_dir)
src0.rgb = temp[2] ,
src1.rgb = temp[1] :
                DP3 src0.rgb src1.rgb ,
   temp[4].a  = DP ;

src0.a = temp[4] :
  temp[4].a = MAX src0.a src0.0 ;

src0.a = temp[4] ,
src1.a = float(32) :
  temp[4].a = MAD src0.a src0.1 src1.a ;

OUT TEX_SEM_WAIT
src0.a = temp[3],
src0.rgb = temp[3] ,
src1.a = temp[4] :
  out[0].a    = MAX src0.a src0.a ,
  out[0].rgb  = MAD src0.rgb src1.aaa src2.000 ;
