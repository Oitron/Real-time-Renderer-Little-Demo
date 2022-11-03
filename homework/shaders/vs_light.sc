$input a_position

# include "../../bgfx/examples/common/common.sh"

void main(){
    mat4 MVP = mul(u_viewProj, u_model[0]);
    gl_Position = mul(MVP, vec4(a_position, 1.0));
}