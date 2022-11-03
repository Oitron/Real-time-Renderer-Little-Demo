$input v_fragPos, v_fragPosLightSpace, v_texcoord0, v_normal, v_tsLightPos, v_tsViewPos, v_tsFragPos


# include "../../bgfx/examples/common/common.sh"



SAMPLERCUBE(s_texEnvDiff, 0);
SAMPLERCUBE(s_texEnvSpec, 1);
SAMPLER2D(s_texBrdfLut, 2);
SAMPLER2D(s_texShadowMap, 3);

SAMPLER2D(s_texColor,  4);
SAMPLER2D(s_texNormal, 5);
SAMPLER2D(s_texRM,     6);





uniform vec4 u_lightPos;
uniform vec4 u_lightColor;
uniform vec4 u_viewPos;

uniform vec4 u_ambientIntensity;


#define PI  3.14159265359
#define F_MIN  0.0001 //avoid divided by 0




//PBR

//Diffuse BRDF function
vec3 diffuse_brdf(vec3 albedo, float metallic){
    return (1.0 - metallic) * albedo / PI;
}


//Fresnel Equation
vec3 fresnel(vec3 f0, vec3 halfWayDir, vec3 viewDir){
    return f0 + pow(clamp(1.0 - max(dot(halfWayDir, viewDir), 0.0), 0.0, 1.0), 5.0) * (vec3(1.0, 1.0, 1.0) - f0);
}

vec3 fresnel_roughness(float NdotV, vec3 f0, float roughness){
    return f0 + (max(vec3(1.0-roughness), f0) - f0) * pow(clamp(1.0-NdotV, 0.0, 1.0), 5.0);
}


//Normal Distribution function
float dtbNormal(float roughness, vec3 halfWayDir, vec3 normal){
    float roughness_2 = roughness * roughness;
    float cosTheta = max(dot(halfWayDir, normal),0.0);
    float cosTheta_2 = cosTheta * cosTheta;
    return roughness_2 / (PI * pow(cosTheta_2 * (roughness_2-1) + 1, 2));
}

//Geometry function
float geo(float roughness, vec3 dir, vec3 normal){
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float cosTheta = max(dot(dir, normal), 0.0);
    return cosTheta / (cosTheta * (1.0-k) + k);
}

float geometry(float roughness, vec3 lightDir, vec3 viewDir, vec3 normal){
    return geo(roughness, lightDir, normal) * geo(roughness, viewDir, normal);
}


//Specular BRDF function
vec3 specular_brdf(vec3 f0, float roughness, float metallic, vec3 lightDir, vec3 viewDir, vec3 normal){
    vec3 halfWayDir = normalize(lightDir + viewDir);
    return fresnel(f0, halfWayDir, viewDir) *
           dtbNormal(roughness, halfWayDir, normal) * 
           geometry(roughness, lightDir, viewDir, normal) /
           (4.0 * max(dot(normal, lightDir),0.0) * max(dot(normal, viewDir), 0.0) + F_MIN); 
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

    vec3 base_f0 = vec3(0.04, 0.04, 0.04);

    //compute attenuation
    float distance = length(u_lightPos.xyz - v_fragPos);
    float attenuation = 1.0 / (distance * distance);

    vec3 lightDir = normalize(u_lightPos.xyz - v_fragPos);
    vec3 viewDir = normalize(u_viewPos.xyz - v_fragPos);

    
    //get tangent space normal from normal texture
    vec3 tsNormal = normalize(texture2D(s_texNormal, v_texcoord0).xyz * 2.0 - 1.0); //[0,1] to [-1,1]
    //tsNormal.z = sqrt(1.0 - dot(tsNormal.xy, tsNormal.xy));


    vec3 tsLightDir = normalize(v_tsLightPos - v_tsFragPos);
    vec3 tsViewDir = normalize(v_tsViewPos - v_tsFragPos);
    
    
    vec3 albedo = toLinear(texture2D(s_texColor, v_texcoord0)).xyz; //sRGB -> linear

    float ao = texture2D(s_texRM, v_texcoord0).x;
    float roughness = texture2D(s_texRM, v_texcoord0).y;
    float metallic = texture2D(s_texRM, v_texcoord0).z;
    

    vec3 f0 = mix(base_f0, albedo, metallic);
    
    
    vec3 direct_lighting = (diffuse_brdf(albedo, metallic) + 
                            specular_brdf(f0, roughness, metallic, tsLightDir, tsViewDir, tsNormal)) 
                          * u_lightColor.xyz * max(dot(tsNormal, tsLightDir), 0.0);
    
    //---IBL---//
    
    vec3 ibl_diff = textureCube(s_texEnvDiff, v_normal).xyz * albedo;

    const float MAX_REFLECTION_LOD = 9.0; //max level of mipmap
    vec3 prefilteredColor = textureCubeLod(s_texEnvSpec, v_normal, roughness * MAX_REFLECTION_LOD).xyz;
    float NdotV = max(dot(v_normal, viewDir), 0.0);
    vec2 sampCoord = vec2(NdotV, roughness);
    vec2 brdf = texture2D(s_texBrdfLut, sampCoord).xy;
    vec3 f = fresnel_roughness(NdotV, base_f0, roughness);
    vec3 ibl_spec = prefilteredColor * (f*brdf.x + brdf.y);

    vec3 ambient = (1.0-f) * ibl_diff + ibl_spec;

    //---shadow---//
    float visible = Basic_ShadowCalculation(v_fragPosLightSpace, lightDir);
    
    vec3 lighting = u_ambientIntensity.xyz * ambient + visible * attenuation * direct_lighting;
    //vec3 lighting = ambient;
    //vec3 lighting = 2.0 * direct_lighting;

    //HDR tonemapping
    lighting = lighting / (lighting + vec3(1.0, 1.0, 1.0));
    //to gamma
    lighting = pow(lighting, vec3(1.0/2.2, 1.0/2.2, 1.0/2.2));
    
    gl_FragColor = vec4(lighting, 1.0);

    //gl_FragColor = vec4(vec3(texture2D(s_texRM, v_texcoord0).w), 1.0);

}