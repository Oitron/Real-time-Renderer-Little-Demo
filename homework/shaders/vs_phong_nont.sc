$input a_position, a_texcoord0, a_normal
$output v_fragPos, v_fragPosLightSpace, v_texcoord0, v_normal

# include "../../bgfx/examples/common/common.sh"

uniform vec4 u_lightPos;
uniform vec4 u_viewPos;

uniform mat4 u_lightSpaceMatrix;

void main() {
	//convert position to world space
	v_fragPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;

	//world space -> light view space
	v_fragPosLightSpace = mul(u_lightSpaceMatrix, vec4(v_fragPos, 1.0));
	
	//convert normal and tangent to world space
	//vec4 os_normal = a_normal; //for unpacked normal
	vec4 os_normal = a_normal * 2.0 - 1.0; //[0,1] to [-1,1] for packed normal

	v_normal = os_normal.xyz;
	
	//pass texture coord
	v_texcoord0 = a_texcoord0;

	mat4 MVP = mul(u_viewProj, u_model[0]);
	gl_Position = mul(MVP, vec4(a_position, 1.0));
}