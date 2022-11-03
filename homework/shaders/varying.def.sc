vec3 a_position         : POSITION;
vec2 a_texcoord0        : TEXCOORD0;   
vec4 a_normal           : NORMAL;
vec4 a_tangent          : TANGENT;

vec3 v_dir              : TEXCOORD2 = vec3(0.0, 0.0, 0.0);
vec3 v_fragPos          : TEXCOORD1 = vec3(0.0, 0.0, 0.0);
vec4 v_fragPosLightSpace: TEXCOORD6 = vec4(0.0, 0.0, 0.0, 1.0);
vec2 v_texcoord0        : TEXCOORD0 = vec2(0.0, 0.0);
vec3 v_normal           : NORMAL    = vec3(0.0, 0.0, 1.0);
vec3 v_tsFragPos        : TEXCOORD3 = vec3(0.0, 0.0, 0.0);
vec3 v_tsLightPos       : TEXCOORD4 = vec3(0.0, 0.0, 0.0);
vec3 v_tsViewPos        : TEXCOORD5 = vec3(0.0, 0.0, 0.0);