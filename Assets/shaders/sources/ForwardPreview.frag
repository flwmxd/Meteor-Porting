#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2

const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

const float EPSILON = 0.00001;
// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct Light
{
	vec4 color;
	vec4 position;
	vec4 direction;
	float intensity;
	float radius;
	float type;
	float angle;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;

layout(set = 2, binding = 0) uniform UniformBufferLight
{
	Light light;
	vec4 cameraPosition;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D uMetallicMap;
layout(set = 1, binding = 2) uniform sampler2D uRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D uNormalMap;
layout(set = 1, binding = 4) uniform sampler2D uAOMap;
layout(set = 1, binding = 5) uniform sampler2D uEmissiveMap;

layout(set = 1, binding = 6) uniform UniformMaterialData
{
	vec4  albedoColor;
	vec4  roughnessColor;
	vec4  metallicColor;
	vec4  emissiveColor;
	float usingAlbedoMap;
	float usingMetallicMap;
	float usingRoughnessMap;
	float usingNormalMap;
	float usingAOMap;
	float usingEmissiveMap;
	float workflow;
	float padding;
} materialProperties;


layout(location = 0) out vec4 outColor;


struct Material
{
	vec4 albedo;
	vec3 metallic;
	float roughness;
	vec3 emissive;
	vec3 normal;
	float ao;
	vec3 view;
	float normalDotView;
};

vec4 gammaCorrectTexture(vec4 samp)
{
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

vec3 gammaCorrectTextureRGB(vec4 samp)
{
	return vec3(pow(samp.rgb, vec3(GAMMA)));
}

vec4 getAlbedo()
{
	return (1.0 - materialProperties.usingAlbedoMap) * materialProperties.albedoColor + materialProperties.usingAlbedoMap * gammaCorrectTexture(texture(uAlbedoMap, fragTexCoord));
}

vec3 getMetallic()
{
	return (1.0 - materialProperties.usingMetallicMap) * materialProperties.metallicColor.rgb + materialProperties.usingMetallicMap * gammaCorrectTextureRGB(texture(uMetallicMap, fragTexCoord)).rgb;
}

float getRoughness()
{
	return (1.0 - materialProperties.usingRoughnessMap) *  materialProperties.roughnessColor.r + materialProperties.usingRoughnessMap * gammaCorrectTextureRGB(texture(uRoughnessMap, fragTexCoord)).r;
}

float getAO()
{
	return (1.0 - materialProperties.usingAOMap) + materialProperties.usingAOMap * gammaCorrectTextureRGB(texture(uAOMap, fragTexCoord)).r;
}

vec3 getEmissive()
{
	return (1.0 - materialProperties.usingEmissiveMap) * materialProperties.emissiveColor.rgb + materialProperties.usingEmissiveMap * gammaCorrectTextureRGB(texture(uEmissiveMap, fragTexCoord));
}

vec3 getNormalFromMap()
{
	if (materialProperties.usingNormalMap < 0.1)
		return normalize(fragNormal);
	
	vec3 tangentNormal = texture(uNormalMap, fragTexCoord).xyz * 2.0 - 1.0;
	
	vec3 Q1 = dFdx(fragPosition.xyz);
	vec3 Q2 = dFdy(fragPosition.xyz);
	vec2 st1 = dFdx(fragTexCoord);
	vec2 st2 = dFdy(fragTexCoord);
	
	vec3 N = normalize(fragNormal);
	vec3 T = normalize(Q1*st2.t - Q2*st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	
	return normalize(TBN * tangentNormal);
}
// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;
	
	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickroughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 lighting(vec3 F0, vec3 wsPos, Material material)
{
	vec3 result = vec3(0.0);
	Light light = ubo.light;
	float value = 1.0;
	
	vec3 Li = light.direction.xyz;
	vec3 Lradiance = light.color.xyz * light.intensity;
	vec3 Lh = normalize(Li + material.view);
	
	// Calculate angles between surface normal and various light vectors.
	float cosLi = max(0.0, dot(material.normal, Li));
	float cosLh = max(0.0, dot(material.normal, Lh));
	
	//vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, material.view)));
	vec3 F = fresnelSchlickroughness(F0, max(0.0, dot(Lh,  material.view)), material.roughness);
	
	float D = ndfGGX(cosLh, material.roughness);
	float G = gaSchlickGGX(cosLi, material.normalDotView, material.roughness);
	
	vec3 kd = (1.0 - F) * (1.0 - material.metallic.x);
	vec3 diffuseBRDF = kd * material.albedo.xyz;
	
	// Cook-Torrance
	vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLi * material.normalDotView);
	
	result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi * value * material.ao;
	return result;
}

vec3 gammaCorrectTextureRGB(vec3 texCol)
{
	return vec3(pow(texCol.rgb, vec3(GAMMA)));
}

float attentuate( vec3 lightData, float dist )
{
	float att =  1.0 / ( lightData.x + lightData.y*dist + lightData.z*dist*dist );
	float damping = 1.0;
	return max(att * damping, 0.0);
}

void main() 
{
	vec4 texColor = getAlbedo();
	if(texColor.w < 0.1)
		discard;
	float metallic = 0.0;
	float roughness = 0.0;
	
	if(materialProperties.workflow == PBR_WORKFLOW_SEPARATE_TEXTURES)
	{
		metallic  = getMetallic().x;
		roughness = getRoughness();
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS)
	{
		vec3 tex = gammaCorrectTextureRGB(texture(uMetallicMap, fragTexCoord));
		metallic = tex.b;
		roughness = tex.g;
	}
	else if( materialProperties.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS)
	{
		//TODO
		vec3 tex = gammaCorrectTextureRGB(texture(uMetallicMap, fragTexCoord));
		metallic = tex.b;
		roughness = tex.g;
	}
	
	Material material;
    material.albedo    	= texColor;
    material.metallic  	= vec3(metallic);
    material.roughness 	= roughness;
    material.normal    	= getNormalFromMap();
	material.ao			= getAO();
	material.emissive  	= getEmissive();
	
	vec3 wsPos = fragPosition.xyz;
	material.view 	 			= normalize(ubo.cameraPosition.xyz - wsPos);
	material.normalDotView     	= max(dot(material.normal, material.view), 0.0);

	vec3 Lr =  reflect(-material.view,material.normal); 
	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, material.albedo.xyz, material.metallic.x);
	
	vec3 lightContribution = lighting(F0, wsPos, material);
	vec3 finalColor = lightContribution + material.emissive;
	outColor = vec4(finalColor, 1.0);
}