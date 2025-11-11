-- CONST[0] = { time, 1.2, 0.01, 0.4 }
-- CONST[1] = { PI_2, I_PI_2, 0, 0 },
-- CONST[2] = { 0.25, 0.40625, 0.5625, 0 },

-- temp[0] : { uv0.xy , _, l }
-- temp[1] : { uv.xy  , _, d }
-- temp[2] : final_color.xyzw
-- temp[3] : {col.xyz , i }

-- vec2 uv = uv0; // temp[1]
src0.rgb = temp[0] : -- uv0
  temp[1].rg  = MAX src0.rg_ src0.rg_ ;

-- vec4 final_color = vec4(0, 0, 0, 1);
:
  temp[2].a   = MAX src0.1 src0.1 ,
  temp[2].rgb = MAX src0.000 src0.000 ;

-- i = 0;
:
  temp[3].a   = MAX src0.0 src0.0 ;

--------------------------------------------------------------------------------
-- loop start
--------------------------------------------------------------------------------
#include "shadertoy_palette_fractal_loop_inner.fs.asm"
#include "shadertoy_palette_fractal_loop_inner.fs.asm"
#include "shadertoy_palette_fractal_loop_inner.fs.asm"
#include "shadertoy_palette_fractal_loop_inner.fs.asm"
--------------------------------------------------------------------------------
-- loop end
--------------------------------------------------------------------------------

OUT TEX_SEM_WAIT
src0.rgb = temp[2] :
  out[0].a     = MAX src0.1 src0.1 ,
  out[0].rgb   = MAX src0.rgb src0.rgb ;
