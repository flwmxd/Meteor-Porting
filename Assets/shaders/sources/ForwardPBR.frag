#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.1415926535897932384626433832795
#define GAMMA 2.2
#define MAX_LIGHTS 32
#define MAX_SHADOWMAPS 4

const float PBR_WORKFLOW_SEPARATE_TEXTURES = 0.0f;
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;

const int NUM_PCF_SAMPLES = 16;
const bool FADE_CASCADES = false;
const float EPSILON = 0.00001;

const float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio   

float ShadowFade = 1.0;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragPosition;
layout(location = 3) in vec3 fragNormal;
layout(location = 4) in vec3 fragTangent;

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


layout(set = 2, binding = 0) uniform sampler2D uPreintegratedFG;//BRDFLUT
layout(set = 2, binding = 1) uniform samplerCube uPrefilterMap;
layout(set = 2, binding = 2) uniform samplerCube uIrradianceMap;
layout(set = 2, binding = 3) uniform sampler2DArray uShadowMap;
layout(set = 2, binding = 4) uniform UniformBufferLight
{
	Light lights[MAX_LIGHTS];
	mat4 shadowTransform[MAX_SHADOWMAPS];
	mat4 viewMatrix;
	mat4 lightView;
	mat4 biasMat;
	vec4 cameraPosition;
	vec4 splitDepths[MAX_SHADOWMAPS];
	float shadowMapSize;
	float maxShadowDistance;
	float shadowFade;
	float cascadeTransitionFade;
	int lightCount;
	int shadowCount;
	int mode;
	int cubeMapMipLevels;

	float initialBias;
} ubo;

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
	return samp;
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

vec3 gammaCorrectTextureRGB(vec4 samp)
{
	return samp.xyz;
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

const vec2 PoissonDistribution16[16] = vec2[](
	  vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725), vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
	  vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464), vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
	  vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420), vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
	  vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590), vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);


const vec2 PoissonDistribution[64] = vec2[](
	vec2(-0.884081, 0.124488), vec2(-0.714377, 0.027940), vec2(-0.747945, 0.227922), vec2(-0.939609, 0.243634),
	vec2(-0.985465, 0.045534),vec2(-0.861367, -0.136222),vec2(-0.881934, 0.396908),vec2(-0.466938, 0.014526),
	vec2(-0.558207, 0.212662),vec2(-0.578447, -0.095822),vec2(-0.740266, -0.095631),vec2(-0.751681, 0.472604),
	vec2(-0.553147, -0.243177),vec2(-0.674762, -0.330730),vec2(-0.402765, -0.122087),vec2(-0.319776, -0.312166),
	vec2(-0.413923, -0.439757),vec2(-0.979153, -0.201245),vec2(-0.865579, -0.288695),vec2(-0.243704, -0.186378),
	vec2(-0.294920, -0.055748),vec2(-0.604452, -0.544251),vec2(-0.418056, -0.587679),vec2(-0.549156, -0.415877),
	vec2(-0.238080, -0.611761),vec2(-0.267004, -0.459702),vec2(-0.100006, -0.229116),vec2(-0.101928, -0.380382),
	vec2(-0.681467, -0.700773),vec2(-0.763488, -0.543386),vec2(-0.549030, -0.750749),vec2(-0.809045, -0.408738),
	vec2(-0.388134, -0.773448),vec2(-0.429392, -0.894892),vec2(-0.131597, 0.065058),vec2(-0.275002, 0.102922),
	vec2(-0.106117, -0.068327),vec2(-0.294586, -0.891515),vec2(-0.629418, 0.379387),vec2(-0.407257, 0.339748),
	vec2(0.071650, -0.384284),vec2(0.022018, -0.263793),vec2(0.003879, -0.136073),vec2(-0.137533, -0.767844),
	vec2(-0.050874, -0.906068),vec2(0.114133, -0.070053),vec2(0.163314, -0.217231),vec2(-0.100262, -0.587992),
	vec2(-0.004942, 0.125368),vec2(0.035302, -0.619310),vec2(0.195646, -0.459022),vec2(0.303969, -0.346362),
	vec2(-0.678118, 0.685099),vec2(-0.628418, 0.507978),vec2(-0.508473, 0.458753),vec2(0.032134, -0.782030),
	vec2(0.122595, 0.280353),vec2(-0.043643, 0.312119),vec2(0.132993, 0.085170),vec2(-0.192106, 0.285848),
	vec2(0.183621, -0.713242),vec2(0.265220, -0.596716),vec2(-0.009628, -0.483058),vec2(-0.018516, 0.435703)
);


vec2 samplePoisson(int index)
{
	return PoissonDistribution[index % 64];
}

vec2 samplePoisson16(int index)
{
	return PoissonDistribution16[index % 16];
}


float goldNoise(vec2 xy, float seed)
{
	return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

float rand(vec2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

float random(vec4 seed4)
{
	float dotProduct = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dotProduct) * 43758.5453);
}

float textureProj(vec4 shadowCoord, vec2 offset, int cascadeIndex, float bias)
{
	float shadow = 1.0;
	float ambient = 0.0;
	
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 && shadowCoord.w > 0)
	{
		float dist = texture(uShadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (dist < (shadowCoord.z - bias))
		{
			shadow = ambient;//dist;
		}
	}
	return shadow;
	
}

float PCFShadow(vec4 sc, int cascadeIndex, float bias, vec3 wsPos)
{
	ivec2 texDim = textureSize(uShadowMap, 0).xy;
	float scale = 0.75;
	
	vec2 dx = scale * 1.0 / texDim;
	
	float shadowFactor = 0.0;
	int count = 0;
	float range = 1.0;
	
	for (float x = -range; x <= range; x += 1.0) 
	{
		for (float y = -range; y <= range; y += 1.0) 
		{
			shadowFactor += textureProj(sc, vec2(dx.x * x, dx.y * y), cascadeIndex, bias);
			count++;
		}
	}
	return shadowFactor / count;
}

float poissonShadow(vec4 sc, int cascadeIndex, float bias, vec3 wsPos)
{
	ivec2 texDim = textureSize(uShadowMap, 0).xy;
	float scale = 0.8;
	
	vec2 dx = scale * 1.0 / texDim;
	
	float shadowFactor = 1.0;
	int count = 0;
	
	for(int i = 0; i < 8; i ++)
	{
		int index = i;// int(16.0*random(floor(wsPos*1000.0), count))%16;
		shadowFactor -= 0.1 * (1.0 - textureProj(sc, dx * PoissonDistribution16[index], cascadeIndex, bias));
		count++;
	}
	return shadowFactor;
}

float getShadowBias(vec3 lightDirection, vec3 normal, int shadowIndex)
{
	float minBias = ubo.initialBias;
	float bias = max(minBias * (1.0 - dot(normal, lightDirection)), minBias);
	return bias;
}

float getPCFShadowDirectionalLight(sampler2DArray shadowMap, vec4 shadowCoords, float uvRadius, vec3 lightDirection, vec3 normal, vec3 wsPos, int cascadeIndex)
{
	float bias = getShadowBias(lightDirection, normal, cascadeIndex);
	float sum = 0;
	
	for (int i = 0; i < NUM_PCF_SAMPLES; i++)
	{
		int index = int(float(NUM_PCF_SAMPLES)*random(vec4(wsPos.xyz, 1)))%NUM_PCF_SAMPLES;
		
		float z = texture(shadowMap, vec3(shadowCoords.xy + (samplePoisson(index) * uvRadius), cascadeIndex)).r;
		sum += step(shadowCoords.z - bias, z);
	}
	
	return sum / NUM_PCF_SAMPLES;
}

int calculateCascadeIndex(vec3 wsPos)
{
	int cascadeIndex = 0;
	vec4 viewPos = vec4(wsPos, 1.0) * ubo.viewMatrix;
	
	for(int i = 0; i < ubo.shadowCount - 1; ++i)
	{
		if(viewPos.z < ubo.splitDepths[i].x)
		{
			cascadeIndex = i + 1;
		}
	}
	return cascadeIndex;
}

float calculateShadow(vec3 wsPos, int cascadeIndex, vec3 lightDirection, vec3 normal)
{
	vec4 shadowCoord = vec4(wsPos, 1.0) * ubo.shadowTransform[cascadeIndex] * ubo.biasMat;
	shadowCoord = shadowCoord * ( 1.0 / shadowCoord.w);
	const float NEAR = 0.01;
	float uvRadius =  ubo.shadowMapSize * NEAR / shadowCoord.z;
	uvRadius = min(uvRadius, 0.002f);
	vec4 viewPos = vec4(wsPos, 1.0) * ubo.viewMatrix;
	
	float shadowAmount = 1.0;
		shadowCoord = vec4(wsPos, 1.0) * ubo.shadowTransform[cascadeIndex] * ubo.biasMat;
	shadowAmount = getPCFShadowDirectionalLight(uShadowMap, shadowCoord, uvRadius, lightDirection, normal, wsPos, cascadeIndex);
	
	return 1.0 - ((1.0 - shadowAmount) * ShadowFade);
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
	
	for(int i = 0; i < ubo.lightCount; i++)
	{
		Light light = ubo.lights[i];
		float value = 0.0;
		
		if(light.type == 2.0)
		{
		    // Vector to light
			vec3 L = light.position.xyz - wsPos;
			// Distance from light to fragment position
			float dist = length(L);
			
			// Light to fragment
			L = normalize(L);
			
			// Attenuation
			float atten = light.radius / (pow(dist, 2.0) + 1.0);
			
			value = atten;
			
			light.direction = vec4(L,1.0);
		}
		else if (light.type == 1.0)
		{
			vec3 L = light.position.xyz - wsPos;
			float cutoffAngle   = 1.0f - light.angle;      
			float dist          = length(L);
			L = normalize(L);
			float theta         = dot(L.xyz, light.direction.xyz * -1);
			float epsilon       = cutoffAngle - cutoffAngle * 0.9f;
			float attenuation 	= ((theta - cutoffAngle) / epsilon); // atteunate when approaching the outer cone
			attenuation         *= light.radius / (pow(dist, 2.0) + 1.0);//saturate(1.0f - dist / light.range);
			//float intensity 	= attenuation * attenuation;
			
			
			// Erase light if there is no need to compute it
			//intensity *= step(theta, cutoffAngle);
			
			value = clamp(attenuation, 0.0, 1.0);
		}
		else
		{
			int cascadeIndex = calculateCascadeIndex(wsPos);
			value = calculateShadow(wsPos,cascadeIndex, light.direction.xyz * -1, material.normal);
		}
		
		vec3 Li = light.direction.xyz * -1;
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
	}
	return result;
}

vec3 radianceIBLIntegration(float NdotV, float roughness, vec3 metallic)
{
	vec2 preintegratedFG = texture(uPreintegratedFG, vec2(roughness, 1.0 - NdotV)).rg;
	return metallic * preintegratedFG.r + preintegratedFG.g;
}

vec3 IBL(vec3 F0, vec3 Lr, Material material)
{
	float level = float(ubo.cubeMapMipLevels);
	if(textureSize(uIrradianceMap,0).x == 1)
	{
		return vec3(0,0,0);
	}
	vec3 irradiance = texture(uIrradianceMap, material.normal).rgb;
	vec3 F = fresnelSchlickroughness(F0, material.normalDotView, material.roughness);
	vec3 kd = (1.0 - F) * (1.0 - material.metallic.x);
	vec3 diffuseIBL = irradiance * material.albedo.rgb;

	vec3 specularIrradiance = textureLod(uPrefilterMap, Lr, material.roughness * level).rgb;
	vec2 specularBRDF = texture(uPreintegratedFG, vec2(material.normalDotView, material.roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
	
	return (kd * diffuseIBL + specularIBL) * material.ao;
}

vec3 finalGamma(vec3 color)
{
	return pow(color, vec3(1.0 / GAMMA));
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
	if(texColor.w < 0.4)
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

    float shadowDistance = ubo.maxShadowDistance;
	float transitionDistance = ubo.shadowFade;
	
	vec4 viewPos = ubo.viewMatrix * vec4(wsPos, 1.0);
	
	float distance = length(viewPos);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0);
	
	vec3 Lr =  reflect(-material.view,material.normal); 
	//2.0 * material.normalDotView * material.normal - material.view;
	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, material.albedo.xyz, material.metallic.x);
	
	vec3 lightContribution = lighting(F0, wsPos, material);
	vec3 iblContribution = IBL(F0, Lr, material) * 2.0;
	vec3 finalColor = lightContribution + iblContribution;// + material.emissive;
	outColor = vec4(finalColor, 1.0);
	
	if(ubo.mode > 0)
	{
		switch(ubo.mode)
		{
			case 1:
			outColor = material.albedo;
			break;
			case 2:
			outColor = vec4(material.metallic, 1.0);
			break;
			case 3:
			outColor = vec4(material.roughness, material.roughness, material.roughness,1.0);
			break;
			case 4:
			outColor = vec4(material.ao, material.ao, material.ao, 1.0);
			break;
			case 5:
			outColor = vec4(material.emissive, 1.0);
			break;
			case 6:
			outColor = vec4(material.normal,1.0);
			break;
            case 7:
			int cascadeIndex = calculateCascadeIndex(wsPos);
			switch(cascadeIndex)
			{
				case 0 : outColor = outColor * vec4(0.8,0.2,0.2,1.0); break;
				case 1 : outColor = outColor * vec4(0.2,0.8,0.2,1.0); break;
				case 2 : outColor = outColor * vec4(0.2,0.2,0.8,1.0); break;
				case 3 : outColor = outColor * vec4(0.8,0.8,0.2,1.0); break;
			}
			break;
		}
	}
}