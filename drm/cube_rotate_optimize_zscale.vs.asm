-- CONST[0] = {theta1, theta2, 0.159155, 0.5}
-- CONST[1] = {6.283185, -3.141593, 0.2, 0.5}

-- ME_SIN and ME_COS clamp their inputs to [-π,+π] prior to the sin/cos
-- calculation.
--
-- This 3-instruction sequence remaps the range [-∞,+∞] to [-π,+π]
temp[0].xy   = VE_MAD   const[0].xy__   const[0].zz__  const[0].ww__ ;
temp[0].xy   = VE_FRC   temp[0].xy__ ;
temp[0].xy   = VE_MAD   temp[0].xy__   const[1].xx__  const[1].yy__ ;

-- sin and cos
temp[3].x     = ME_SIN   temp[0].___x ;
temp[3].y     = ME_COS   temp[0].___x ;
alt_temp[3].z = ME_SIN   temp[0].___y ;

-- temp[3] now contains:
-- temp[3] = {sin(theta1), cos(theta1), sin(theta2), cos(theta2)}

-------------------------------------------------------------------------
-- first rotation: X-axis rotation:
-------------------------------------------------------------------------

-- y_ = (-z0 * st1)
-- z_ = ( z0 * ct1)
temp[1].yz    = VE_MUL   input[0]._-zz_  temp[3]._xy_ ,
alt_temp[3].w = ME_COS   temp[0].y_ ;

-- x1 = (x0 *   1 +      0)
-- y1 = (y0 * ct1 + nz0st1)
-- z1 = (y0 * st1 +  z0ct1)
temp[1].xyz   = VE_MAD   input[0].xyy_   temp[3].1yx_   temp[1].0yz_ ;

-------------------------------------------------------------------------
-- second rotation: Y-axis rotation:
-------------------------------------------------------------------------

-- x_ = (-z1 * st2)
-- z_ = ( z1 * ct2)
temp[2].xz    = VE_MUL   temp[1].-z_z_   alt_temp[3].z_w_ ;

-- x2 = (x1 * ct2 + nz1st2)
-- y2 = (y1 *   1 +      0)
-- z2 = (x1 * st2 +  z1ct2)
temp[2].xyz   = VE_MAD   temp[1].xyx_    alt_temp[3].w1z_   temp[2].x0z_ ;

-------------------------------------------------------------------------
-- scale
-------------------------------------------------------------------------

-- remap Z from (-1.75, 1.75) to (0.001, 0.999)
-- 0.285714 (z + 1.75) + 0.001

-- remap W from (-1.75, 1.75) to (1, 2)
-- 0.285714 (z + 1.75) + 1

temp[3].xyz   = VE_MAD   temp[2].xyz_    const[2].xx1_  const[2].00y_ ;

-------------------------------------------------------------------------
-- output
-------------------------------------------------------------------------

out[0].xyzw   = VE_MAD   temp[3].xyzz    const[2].11zz  const[2].00w1 ;
out[1].xyzw   = VE_ADD   input[1].xyzw   input[1].0000 ;
