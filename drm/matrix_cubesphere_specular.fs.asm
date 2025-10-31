-- temp[0] -- position (world space)
-- temp[1] -- normal
-- temp[2] -- light pos (world space)
-- temp[3] -- view pos (world space)
-- temp[4] -- texture

-- PIXSIZE 4

TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[4].rgba = LD tex[0].rgba temp[4].rgaa ;

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

-- diff = dot(normal, light_dir)
src0.rgb = temp[2] ,
src1.rgb = temp[1] :
                DP3 src0.rgb src1.rgb ,
   temp[5].a  = DP ;

-- diff = max(diff, 0)
src0.a = temp[5] :
  temp[5].a = MAX src0.a src0.0 ;

-- intensity = diff + 0.125
src0.a = temp[5] ,
src1.a = float(32) : -- 0.125
  temp[5].a = MAD src0.a src0.1 src1.a ;

--
-- specular
--
-- temp[3] -- view pos (world space)
-- view_dir = normalize(f_view_pos - f_world_pos)
src1.rgb = temp[3] , -- f_light_pos
src0.rgb = temp[0] , -- f_world_pos
srcp.rgb = neg :     -- (f_light_pos - f_world_pos)
              DP3 srcp.rgb srcp.rgb ,
  temp[3].a = DP ;
src0.a = temp[3] :
  temp[3].a   = RSQ |src0.a| ;
src0.a = temp[3], src0.rgb = temp[3] :
  temp[3].rgb = MAD src0.rgb src0.aaa src0.000 ;

-- reflect(I, N)
-- I - 2.0 * dot(N, I) * N
-- reflect_dir = reflect(-light_dir, norm)
-- reflect_dir = reflect(-temp[2], temp[1])
-- I - 2.0 * dot(N, I) * N
-- - (2.0 * dot(N, I)) * temp[1] + -temp[2]
src0.rgb = temp[1] , -- N=normal
src1.rgb = temp[2] : -- I=light_dir   dot(N, -I)
  temp[6].r = DP3 src0.rgb -src1.rgb ;
src0.rgb = temp[6] ,
src1.rgb = float(64) : -- 2.0
  temp[6].r = MAD src0.r00 src1.r00 src0.000 ;
src0.rgb = temp[6] ,
src1.rgb = temp[1] , -- N
src2.rgb = temp[2] : -- I
  temp[6].rgb = MAD -src0.rrr src1.rgb -src2.rgb ;

-- spec = max(dot(view_dir, reflect_dir), 0.0)
src0.rgb = temp[6] , -- reflect_dir
src1.rgb = temp[3] : -- view_dir
  temp[6].r = DP3 src0.rgb src1.rgb ;
src0.rgb = temp[6] :
  temp[6].a = MAX src0.r src0.0 ;

-- spec = pow(spec, 32)
src0.a = temp[6] :
  temp[6].a = LN2 src0.a ;
src0.a = temp[6] ,
src1.a = float(72) : -- 32
  temp[6].a = MAD src0.a src1.a src1.0 ;
src0.a = temp[6] :
  temp[6].a = EX2 src0.a ;

OUT TEX_SEM_WAIT
src0.a = temp[4],
src0.rgb = temp[4] ,
src1.a = temp[6] ,
src1.rgb = temp[6]   :
  out[0].a    = MAX src0.a src0.a ,
  out[0].rgb  = MAD src0.111 src1.aaa src1.000 ;
