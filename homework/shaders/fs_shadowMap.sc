
# include "../../bgfx/examples/common/common.sh"

void main(){
	float depth = gl_FragCoord.z;
    //float depth = 1.0;
    //float depth_2 = depth * depth;
    //store shadow map and square shadow map in one texture
    // R channel for shadow map, G channel for square shadow map
    gl_FragColor = vec4(depth, 0.0, 0.0, 0.0);
    //gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}



