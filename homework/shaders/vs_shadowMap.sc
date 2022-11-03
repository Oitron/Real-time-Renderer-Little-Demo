


$input a_position

# include "../../bgfx/examples/common/common.sh"

uniform mat4 u_lightSpaceMatrix;

void main(){
	mat4 ML = mul(u_lightSpaceMatrix, u_model[0]);
	gl_Position = mul(ML, vec4(a_position, 1.0));
}
