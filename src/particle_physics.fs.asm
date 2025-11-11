-- temp[0].rgb : position
-- temp[0].a   : age
-- temp[1].rgb : velocity
-- temp[1].a   : delta

-- temp[2].rgb : reset__position
-- temp[2].a   : reset__age
-- temp[3].rgb : reset__velocity
-- temp[3].a   : reset__delta

-- temp[4].rgb : update__position
-- temp[4].a   : update__age
-- temp[5].rgb : update__velocity
-- temp[5].a   : update__delta

-- temp[6].rgb : temp

-- velocity_scale.rgb = vec3(0.003 , 0.01, 0.003)
-- delta_age = 0.01
-- const[0] = { velocity_scale.rgb, delta_age }
-- gravity = -0.05
-- velocity_attenuation = -0.7
-- const[1] = { velocity_attenuation, gravity, max_age, 0 }

-- out[0].rgb  : position
-- out[0].a    : age

TEX
  temp[0].rgba = LD tex[0].rgba temp[0].rgaa ;

TEX TEX_SEM_WAIT TEX_SEM_ACQUIRE
  temp[1].rgba = LD tex[1].rgba temp[0].rgaa ;

-- update_particle (position)
TEX_SEM_WAIT
src0.a   = temp[0]  , -- age
src0.rgb = temp[0]  , -- position
src1.a   = const[0] , -- delta_age
src1.rgb = const[0] , -- scale
src2.rgb = temp[1]  : -- velocity
  temp[4].a   = MAD src0.a   src0.1   src1.a   , -- update__age      = (age * 1) - delta_age
  temp[4].rgb = MAD src2.rgb src1.rgb src0.rgb ; -- update__position = (velocity * scale) + position

-- update_particle (velocity gravity)
-- p.velocity.y += -0.05;
src0.rgb = temp[1]  , -- velocity
src1.rgb = const[1] : -- gravity (g)
  temp[5].rgb = MAD src0.rgb src1.111 src1.0g0 ;

-- update_particle (velocity bounce)
-- p.velocity.y *= -0.7;
src0.rgb = temp[5]  , -- velocity <gravity>
src1.rgb = const[1] : -- velocity_attenuation (r)
  temp[6].rgb = MAD src0.rgb src1.1r1 src1.000 ;

-- update_particle (velocity bounce)
-- p.velocity = (p.position.y >= 0) ? temp[5] : temp[6]
src0.rgb = temp[5] , -- velocity <gravity>
src1.rgb = temp[6] , -- velocity <bounce>
src2.rgb = temp[4] : -- position
  temp[5].rgb = CMP src0.rgb src1.rgb src2.ggg ;

-- position.y = abs(position.y)
src0.rgb = temp[4] :
  temp[4].g = MAX |src0.0g0| |src0.0g0| ;

--
-- reset
--

-- normalize(vec3(velocity.x, 0, velocity.z))
src0.rgb = temp[1] : -- velocity
              DP3 src0.r0b src0.r0b ,
  temp[2].a = DP ;
src0.a   = temp[2] :
  temp[2].a   = RSQ |src0.a| ;
src0.a   = temp[2] ,
src0.rgb = temp[1] , -- velocity
src1.a   = temp[1] : -- delta
  temp[2].rgb = MAD src0.r0b src0.a0a src1.0a0 ;

-- age = age + max_age
-- reset__position = reset__position * 20
src0.a   = temp[0]  , -- age
src1.rgb = const[1] , -- max_age
src2.a   = const[1] , -- reset_radius
src0.rgb = temp[2]  : -- reset__position
  temp[2].a   = MAD src0.a src0.1 src1.b ,
  temp[2].rgb = MAD src0.rgb src2.aaa src2.000 ;

-- reset__velocity
-- (p.velocity.x * 0.5 + 0.5)
-- velocity.xz = velocity.xz
src0.rgb = temp[1]   , -- velocity.x
src1.a   = float(48) : -- 0.5
  temp[3].a   = MAD src0.r src1.a src1.a ,
  temp[3].rgb = MAX src0.r0b src0.r0b ;

-- reset__velocity
-- velocity.y = (temp[3].a * delta + 2.0)
src0.a   = temp[1]   , -- delta
src1.a   = float(56) , -- 1.0
src2.a   = temp[3]   : --
  temp[3].g = MAD src2.0a0 src0.0a0 src1.0a0 ;

OUT
src0.a   = temp[4]  , -- update__age
src1.a   = temp[2]  , -- reset__age
src2.a   = temp[0]  , -- age
src0.rgb = temp[4]  , -- update__position
src1.rgb = temp[2]  : -- reset__position
  out[0].a    = CMP src0.a src1.a src2.a ,
  out[0].rgb  = CMP src0.rgb src1.rgb src2.aaa ;

OUT TEX_SEM_WAIT
src0.a   = temp[1]  , -- delta
src2.a   = temp[0]  , -- age
src0.rgb = temp[5]  , -- update__velocity
src1.rgb = temp[3]  : -- reset__velocity
  out[1].a    = MAX src0.a src0.a , -- constant
  out[1].rgb  = CMP src0.rgb src1.rgb src2.aaa ;
