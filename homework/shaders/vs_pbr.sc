$input a_position, a_texcoord0, a_normal, a_tangent
$output v_fragPos, v_fragPosLightSpace, v_texcoord0, v_normal, v_tsLightPos, v_tsViewPos, v_tsFragPos

# include "../../bgfx/examples/common/common.sh"

uniform vec4 u_lightPos;
uniform vec4 u_viewPos;

uniform mat4 u_lightSpaceMatrix;

void main() {
	//convert position to world space
	v_fragPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;

	//camera view space -> light view space
	v_fragPosLightSpace = mul(u_lightSpaceMatrix, vec4(v_fragPos, 1.0));
	
	//convert normal and tangent to world space
	//vec4 os_normal = a_normal; //for unpacked normal
	vec4 os_normal = a_normal * 2.0 - 1.0; //[0,1] to [-1,1] for packed normal
	vec3 ws_normal = mul(u_model[0], os_normal).xyz;

	v_normal = normalize(ws_normal);

	//vec4 os_tangent = a_tangent; //for unpacked tangent
	vec4 os_tangent = a_tangent * 2.0 - 1.0; //[0,1] to [-1,1] for packed tangent
	vec3 ws_tangent = mul(u_model[0], os_tangent).xyz;

	vec3 N = normalize(ws_normal);
	vec3 T = normalize(ws_tangent);
	vec3 B = cross(N, T) * os_tangent.w;

	mat3 TBN = mtxFromCols(T, B, N);

	//convert to tangent space

	v_tsFragPos  = 	mul(v_fragPos, TBN);
	v_tsLightPos =  mul(u_lightPos.xyz, TBN);
	v_tsViewPos  = 	mul(u_viewPos.xyz, TBN);
	
	//pass texture coord
	v_texcoord0 = a_texcoord0;

	mat4 MVP = mul(u_viewProj, u_model[0]);
	gl_Position = mul(MVP, vec4(a_position, 1.0));
}