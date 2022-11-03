$input v_dir

# include "../../bgfx/examples/common/common.sh"

SAMPLERCUBE(s_texEnvSpec, 0);

void main(){
    vec3 color = toLinear(textureCubeLod(s_texEnvSpec, v_dir, 0.0)).xyz;
    //HDR tonemapping
    color = color / (color + vec3(1.0, 1.0, 1.0));
    //to gamma
    color = pow(color, vec3(1.0/2.2, 1.0/2.2, 1.0/2.2));
    //gl_FragColor = vec4(color, 1.0);
    gl_FragColor = vec4(v_dir, 1.0);
}