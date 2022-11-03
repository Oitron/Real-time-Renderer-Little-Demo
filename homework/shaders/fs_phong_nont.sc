$input v_fragPos, v_fragPosLightSpace, v_texcoord0, v_normal


# include "../../bgfx/examples/common/common.sh"


SAMPLER2D(s_texShadowMap, 3);

uniform vec4 u_objColor;

uniform vec4 u_lightPos;
uniform vec4 u_lightColor;
uniform vec4 u_viewPos;

uniform vec4 u_phongSet;



//blinn phong

vec3 ambient_part(vec3 objColor){
    //calcule ambient
    float ka = u_phongSet.x;
    vec3 ambient = ka * u_lightColor.xyz * objColor;
    return ambient;
}

vec3 diffuse_part(vec3 lightDir, vec3 normal, vec3 objColor){
    //calcule diffuse
    float kd = u_phongSet.y;
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = kd * u_lightColor.xyz * objColor * diff;
    return diffuse;
}

vec3 specular_part(vec3 lightDir, vec3 viewDir, vec3 normal, float roughness){
    //calcule specular
    float ks = u_phongSet.z;
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfWayDir, normal), 0.0), roughness); 
    vec3 specular = ks * u_lightColor.xyz * spec;
    return specular;
}

vec3 blinn_phong(vec3 lightDir, vec3 viewDir, vec3 normal, float roughness, vec3 objColor, float visible){
    vec3 lighting = ambient_part(objColor) 
                  + visible * diffuse_part(lightDir, normal, objColor)
                  + visible * specular_part(lightDir, viewDir, normal, roughness);
    return lighting;
}


/*******-------------------- Basic calculation (PCF) --------------------******/
// basic code from learnOpenGL, shadow Chapter.
float Basic_ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDir)
{
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// Transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture2D(s_texShadowMap, projCoords.xy).x;
	// Get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if (currentDepth > 1.0) {
		return 1.0;
	}
    
	// Calculate bias (based on depth map resolution and slope)
	float bias = max(0.05 * (1.0 - dot(v_normal, lightDir)), 0.005);
    if (projCoords.x <= 0.01 || projCoords.x >= 0.99 || projCoords.y <= 0.01 || projCoords.y >= 0.99){
        return 1.0;
    }
	// Check whether current frag pos is in shadow
    float visible = 0.0;

    //visible = currentDepth - bias > closestDepth ? 0.0 : 1.0;
    
    vec2 texSize = vec2(1024.0, 1024.0);

	vec2 texelSize = 1.0 / vec2(texSize.x, texSize.y);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture2D(s_texShadowMap, projCoords.xy + vec2(x, y) * texelSize).x;
			visible += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
		}
	}
	visible /= 9.0;
    
	return visible;
}


void main(){
    
    //compute attenuation
    float distance = length(u_lightPos.xyz - v_fragPos);
    float attenuation = 1.0 / (distance * distance);

    //get light direction and view direction in tangent space
    vec3 lightDir = normalize(u_lightPos.xyz - v_fragPos);
    vec3 viewDir = normalize(u_viewPos.xyz - v_fragPos);
    float shiniess = u_phongSet.w; 
    
    vec3 objColor = u_objColor.xyz;


    //---shadow---//
    float visible = Basic_ShadowCalculation(v_fragPosLightSpace, lightDir);
    //vec3 objColor = texture2D(s_texColor, v_texcoord0).xyz;
    vec3 lighting = attenuation * blinn_phong(lightDir, viewDir, v_normal, shiniess, objColor, visible);
    
    //HDR tonemapping
    lighting = lighting / (lighting + vec3(1.0, 1.0, 1.0));
    //to gamma
    lighting = pow(lighting, vec3(1.0/2.2, 1.0/2.2, 1.0/2.2));
    
    gl_FragColor = vec4(lighting, 1.0);
    //gl_FragColor = vec4(tangentNormal, 1.0);
}