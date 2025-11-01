-- temp[0] -- position (world space)
-- temp[1] -- normal
-- temp[2] -- light pos (world space)
-- temp[3] -- view pos (world space)
-- temp[4] -- texture

-- PIXSIZE 4

TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[4].rgba = LD tex[0].rgba temp[4].rgaa ;

--
-- normal = normalize(normal)
--
src0.rgb = temp[1] : -- normal
              DP3 src0.rgb src0.rgb ,
  temp[1].a = DP ;
src0.a = temp[1] :
  temp[1].a = RSQ |src0.a| ;
src0.a = temp[1] ,
src0.rgb = temp[1] : -- normal
  temp[1].rgb = MAD src0.rgb src0.aaa src0.000 ;

--
-- light_dir = light_pos - world_pos
--
src0.rgb = temp[2] , -- light pos
src1.rgb = temp[0] : -- world pos
  temp[2].rgb = MAD src0.111 src0.rgb -src1.rgb ;

--
-- light_dir = normalize(light_dir)
--
src0.rgb = temp[2] : -- light_dir
              DP3 src0.rgb src0.rgb ,
  temp[2].a = DP ;
src0.a = temp[2] :
  temp[2].a = RSQ |src0.a| ;
src0.a = temp[2] ,
src0.rgb = temp[2] : -- light_dir
  temp[2].rgb = MAD src0.rgb src0.aaa src0.000 ;

--
-- view_dir = view_pos - world_pos
--
src0.rgb = temp[3] , -- view pos
src1.rgb = temp[0] : -- world pos
  temp[3].rgb = MAD src0.111 src0.rgb -src1.rgb ;

--
-- view_dir = normalize(view_dir)
--
src0.rgb = temp[3] : -- view dir
              DP3 src0.rgb src0.rgb ,
  temp[3].a = DP ;
src0.a = temp[3] :
  temp[3].a = RSQ |src0.a| ;
src0.a = temp[3] ,
src0.rgb = temp[3] : -- view dir
  temp[3].rgb = MAD src0.rgb src0.aaa src0.000 ;

--
-- reflect_dir = reflect(light_dir, normal)
--
-- dotLN = dot(-light_dir, normal)
src0.rgb = temp[2] , -- light dir
src1.rgb = temp[1] : -- normal
              DP3 -src0.rgb src1.rgb ,
  temp[5].a = DP ;
-- dotLN = 2.0 * dotLN
src0.a = temp[5] ,   -- dotLN
src1.a = float(64) : -- 2.0
  temp[5].a = MAD src0.a src1.a src0.0 ;
-- dotLN = -dotLN * normal + -light_dir
src0.a = temp[5] , -- dotLN
src1.rgb = temp[1] , -- normal
src2.rgb = temp[2] : -- light dir
  temp[5].rgb = MAD -src0.aaa src1.rgb -src2.rgb ;

--
-- spec = max(dot(view_dir, reflect_dir), 0.0)
--
src0.rgb = temp[3] ,
src1.rgb = temp[5] :
              DP3 src0.rgb src1.rgb ,
  temp[5].a = DP ;
src0.a = temp[5] :
  temp[5].a = MAX src0.a src0.0 ;

--
-- spec = pow(spec, 32)
--
src0.a = temp[5] :    -- spec
  temp[5].a = LN2 src0.a ;
src0.a = temp[5] ,    -- spec
src1.a = float(96) :  -- 32
  temp[5].a = MAD src0.a src1.a src1.0 ;
src0.a = temp[5] :    -- spec
  temp[5].a = EX2 src0.a ;

OUT TEX_SEM_WAIT
src1.a = temp[5] ,
src1.rgb = temp[5] :
  out[0].a    = MAX src1.1 src0.1 ,
  out[0].rgb  = MAD src1.111 src1.aaa src1.000 ;
