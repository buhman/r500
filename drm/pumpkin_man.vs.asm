-- CONST[0] = {0.159155, 0.5, 6.283185, -3.141593}
-- CONST[1] = {theta1, theta2, 0.2, 0.5}
-- CONST[2] = {1.33333, 0, 0, 0}

-- each instruction is only allowed to use a single unique `const` address
--
-- instructions may use multiple `temp` addresses, so const[1] is moved to
-- temp[0]:
--
temp[0].xy   = VE_ADD  const[1].xy__ const[1].00__ ;

-- ME_SIN and ME_COS clamp their inputs to [-π,+π] prior to the sin/cos
-- calculation.
--
-- This 3-instruction sequence remaps the range [-∞,+∞] to [-π,+π]
temp[0].xy   = VE_MAD   temp[0].xy__   const[0].xx__  const[0].yy__ ;
temp[0].xy   = VE_FRC   temp[0].xy__ ;
temp[0].xy   = VE_MAD   temp[0].xy__   const[0].zz__  const[0].ww__ ;

-- sin and cos
temp[3].x    = ME_SIN   temp[0].___x ;
temp[3].y    = ME_COS   temp[0].___x ;
temp[3].z    = ME_SIN   temp[0].___y ;
temp[3].w    = ME_COS   temp[0].___y ;

-- temp[3] now contains:
-- temp[3] = {sin(theta1), cos(theta1), sin(theta2), cos(theta2)}

-------------------------------------------------------------------------
-- first rotation: X-axis rotation:
-------------------------------------------------------------------------

-- y_ = (-z0 * st1)
-- z_ = ( z0 * ct1)
temp[1].yz    = VE_MUL   input[0]._-zz_  temp[3]._xy_ ;

-- x1 = (x0 *   1 +      0)
-- y1 = (y0 * ct1 + nz0st1)
-- z1 = (y0 * st1 +  z0ct1)
temp[1].xyz   = VE_MAD   input[0].xyy_   temp[3].1yx_   temp[1].0yz_ ;

-------------------------------------------------------------------------
-- second rotation: Y-axis rotation:
-------------------------------------------------------------------------

-- x_ = (-z1 * st2)
-- z_ = ( z1 * ct2)
temp[2].xz    = VE_MUL   temp[1].-z_z_   temp[3].z_w_ ;

-- x2 = (x1 * ct2 + nz1st2)
-- y2 = (y1 *   1 +      0)
-- z2 = (x1 * st2 +  z1ct2)
temp[2].xyz   = VE_MAD   temp[1].xyx_    temp[3].w1z_   temp[2].x0z_ ;

-------------------------------------------------------------------------
-- scale
-------------------------------------------------------------------------

temp[3].xyz   = VE_MAD   temp[2].xyz_    const[1].zzz_  const[1].00w_ ;
temp[3].x     = VE_MUL   temp[3].x___    const[2].x___;

-------------------------------------------------------------------------
-- output
-------------------------------------------------------------------------

out[0].xyzw      = VE_MUL   temp[3].xyzz    temp[3].11-z1 ;
out[1].xyzw      = VE_ADD   input[1].xyzw   input[1].0000 ;
