$input a_position
$output v_dir

# include "../../bgfx/examples/common/common.sh"

void main(){

    v_dir = a_position;

    mat4 MVP = mul(u_viewProj, u_model[0]);
    vec4 position = mul(MVP, vec4(a_position, 1.0));
    gl_Position = position.xyww;
}