-- uv = uv * 1.5;
src0.rgb = temp[1] , -- uv
src0.a = float(60) : -- 1.5
  temp[1].rg  = MAD src0.rg_ src0.aa_ src0.00_ ;

-- uv = fract(uv);
src0.rgb = temp[1] : -- uv
  temp[1].rg  = FRC src0.rg_ ;

-- uv = uv - 0.5;
src0.rgb = temp[1] ,
src1.a = float(48) : -- 0.5
  temp[1].rg  = MAD src0.rg_ src0.11_ -src1.aa_ ;

-- l = length(uv0);
src0.rgb = temp[0] : -- uv0
                 DP3 src0.rg0 src0.rg0 ,
  temp[0].a    = DP ;
src0.a   = temp[0] :
  temp[0].a    = RSQ |src0.a| ;
src0.a   = temp[0] :
  temp[0].a    = RCP src0.a ;

-- d = i * 0.4 + l;
src0.a = const[0] , -- 0.4
src1.a = temp[0]  , -- l
src2.a = temp[3]  : -- i
  temp[1].a   = MAD src2.a src0.a src1.a ;

-- d = time * 0.4 + d;
src0.a = const[0]   , -- 0.4
src1.a = temp[1]    , -- d
src0.rgb = const[0] : -- time (r)
  temp[1].a   = MAD src0.r src0.a src1.a ;

--------------------------------------------------------------------------------
-- start of 'palette' function
--------------------------------------------------------------------------------

-- v = d + (vec3(0.25, 0.40625, 0.5625) + 0.5)
src0.a = temp[1]     , -- d
src0.rgb = const[2]  , -- vec3(0.25, 0.40625, 0.5625)
src1.rgb = float(48) , -- 0.5
srcp.rgb = add       : -- (vec3(0.25, 0.40625, 0.5625) + 0.5)
  temp[3].rgb = MAD src0.111 src0.aaa srcp.rgb ;

-- v = frac(v)
src0.rgb = temp[3] : -- v
  temp[3].rgb = FRC src0.rgb ;

-- v = v - 0.5
src0.rgb = temp[3]   , -- v
src1.rgb = float(48) : -- 0.5
  temp[3].rgb = MAD src0.111 src0.rgb -src1.rgb ;

-- v = cos(v)
src0.rgb = temp[3] : -- v
              COS src0.r ,
  temp[3].r = SOP ;
src0.rgb = temp[3] : -- v
              COS src0.g ,
  temp[3].g = SOP ;
src0.rgb = temp[3] : -- v
              COS src0.b ,
  temp[3].b = SOP ;

-- col = vec3(0.5, 0.5, 0.5) * v + vec3(0.5, 0.5, 0.5)
src0.rgb = temp[3]   , -- v
src1.rgb = float(48) : -- 0.5
  temp[3].rgb = MAD src1.rgb src0.rgb src1.rgb;

--------------------------------------------------------------------------------
-- end of 'palette' function
--------------------------------------------------------------------------------

-- d = ex2(-l);
src0.a = temp[0] : -- l
  temp[1].a   = EX2 -src0.a ;

-- l = length(uv);
src0.rgb = temp[1] : -- uv
                 DP3 src0.rg0 src0.rg0 ,
  temp[0].a    = DP ;
src0.a   = temp[0] :
  temp[0].a    = RSQ |src0.a| ;
src0.a   = temp[0] :
  temp[0].a    = RCP src0.a ;

-- d = l * d;
src0.a = temp[0] , -- l
src1.a = temp[1] : -- d
  temp[1].a   = MAD src0.a src1.a src0.0 ;

-- d = d * 8.0 + time;
src0.a   = temp[1]   , -- d
src1.a   = float(80) , -- 8.0
src2.rgb = const[0]  : -- time (r)
  temp[1].a   = MAD src0.a src1.a src2.r ;

-- d = 0.125 * sin(d); <OMOD>
-- d = d * 0.159154936671257019043 + 0.5; // 48
src0.a   = temp[1]   , -- d
src1.rgb = const[1]  , -- I_PI_2 (g)
src2.a   = float(48) : -- 0.5
  temp[1].a  = MAD src0.a src1.g src2.a ;
-- d = fract(d);
src0.a   = temp[1] : -- d
  temp[1].a  = FRC src0.a ;
-- d = d - 0.5;
src0.a = temp[1]   , -- d
src1.a = float(48) : -- 0.5
  temp[1].a = MAD src0.1 src0.a -src1.a ;
-- d = 0.125 * sin(d * PI_2);
src0.a = temp[1] :
  temp[1].a = 0.125 * SIN src0.a ;

-- d = 1.0 / abs(d);
src0.a = temp[1] : -- d
  temp[1].a = RCP |src0.a|;

-- d = 0.01 * d;
src0.a = temp[1]    , -- d
src1.rgb = const[0] : -- 0.01 (b)
  temp[1].a = MAD src0.a src1.b src0.0 ;

-- d = pow(d, 1.2);
src0.a = temp[1] : -- d
  temp[1].a = LN2 src0.a ;
src0.a = temp[1] ,
src1.rgb = const[0] : -- 1.2 (g)
  temp[1].a = MAD src0.a src1.g src0.0 ;
src0.a = temp[1] :
  temp[1].a = EX2 src0.a ;

-- final_color = col * d + final_color
src0.rgb = temp[3] , -- col
src1.a   = temp[1] , -- d
src2.rgb = temp[2] : -- final_color
  temp[2].rgb = MAD src0.rgb src1.aaa src2.rgb ;

-- i = i + 1
src0.a = temp[3] :
  temp[3].a   = MAD src0.1 src0.a src0.1 ;
