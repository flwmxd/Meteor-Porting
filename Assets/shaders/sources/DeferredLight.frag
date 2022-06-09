#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "Common/Light.h"
#include "Common/Math.h"

#define GAMMA 2.2

const int NUM_PCF_SAMPLES = 16;
const bool FADE_CASCADES = false;
const float EPSILON = 0.00001;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct Material
{
	vec4 albedo;
	vec3 metallic;
	float roughness;
	vec3 normal;
	float ao;
	float ssao;
	vec3 view;
	float normalDotView;
};


layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0)  uniform sampler2D uColorSampler;
layout(set = 0, binding = 1)  uniform sampler2D uPositionSampler;
layout(set = 0, binding = 2)  uniform sampler2D uNormalSampler;
layout(set = 0, binding = 3)  uniform sampler2D uDepthSampler;
layout(set = 0, binding = 4)  uniform sampler2D uSSAOSampler;//blur
layout(set = 0, binding = 5)  uniform sampler2DArray uShadowMap;
layout(set = 0, binding = 6)  uniform sampler2D uPBRSampler;
layout(set = 0, binding = 7)  uniform samplerCube uIrradianceMap;
layout(set = 0, binding = 8)  uniform samplerCube uPrefilterMap;
layout(set = 0, binding = 9)  uniform sampler2D uPreintegratedFG;
layout(set = 0, binding = 10) uniform sampler2D uIndirectLight;
layout(set = 0, binding = 11) uniform UniformBufferLight
{
	Light lights[MAX_LIGHTS];
	mat4 shadowTransform[MAX_SHADOWMAPS];
	mat4 viewMatrix;
	mat4 lightView;
	mat4 biasMat;
	vec4 cameraPosition;
	vec4 splitDepths[MAX_SHADOWMAPS];
	
	float shadowMapSize;
	float initialBias;

	int lightCount;
	int shadowCount;
	int mode;
	int cubeMapMipLevels;
	
	int ssaoEnable;
	int enableLPV;
	int enableShadow;
	int shadowMethod;

	float padd;
	float padd2;
} ubo;

float RayMarch(vec3 startPos, vec3 viewDir, vec3 normal, vec3 cameraPos, Light light)
{
    // 观察到的点与观察位置之间的向量
    vec3 view2DestDir = startPos - cameraPos;

    // 观察到的点与观察位置之间的距离
    float view2DestDist= length(view2DestDir);

    // 以观察点与观察位置之间的距离为总的循环次数，每次递进 距离的倒数
    // 两种步进规则
    const int stepNum = 25;    //floor(view2DestDist)+1;
    float oneStep = view2DestDist / stepNum;    // 1 / stepNum

    float finalLight = 0;

    for (int k = 0; k < stepNum; k++)
    {
        // 采样的位置点
        vec3 samplePos = startPos + viewDir * oneStep * k;     // * k ;

        // 累计递进的距离
        float stepDist = length(viewDir * oneStep * k);
        // 采样点到体积光源的位置的向量  指向光源
        vec3 sample2Light = samplePos - light.position.xyz;
        vec3 sample2LightNorm = normalize(sample2Light);

        // 体积光源的照射方向和采样点到体积光源的方向的点积
        float litfrwdDotSmp2lit = dot(sample2LightNorm, -light.direction.xyz);
        
        // angle为体积光的张角的一半，如果 litfrwdDotSmp2lit 大于 cos(angle) ，则表示该采样点在体积光范围内
        float isInLight = smoothstep((1.0f - light.angle), 1, litfrwdDotSmp2lit);

        // 采样点到体积光源的距离
        float sample2LightDist = length(sample2Light) + 1;
        // 当距离小于于1 时 取倒数后光强会非常大，因此将得到得距离+1
        float sample2LightDistInv = 1.0 / sample2LightDist;
        
        // 采样点的光强， 与采样点到体积光源的距离平方成反比
        float sampleLigheIntensity = sample2LightDistInv * sample2LightDistInv * light.intensity;

        // shadow
       // float shadow = 1;//ShadowCalculation(float4(samplePos, 1));
		//int cascadeIndex = calculateCascadeIndex(samplePos);
		float shadow = 1;//calculateShadow(samplePos, cascadeIndex, light.direction.xyz, normal);
        // final
        finalLight += sampleLigheIntensity * isInLight * shadow; 
    }

    return finalLight;
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

float textureProj(vec4 shadowCoord, vec2 offset, int cascadeIndex)
{
	float shadow = 1.0;
	float ambient = 0.00;
	
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 && shadowCoord.w > 0)
	{
		float dist = texture(uShadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (dist < (shadowCoord.z - ubo.initialBias))
		{
			shadow = ambient;//dist;
		}
	}
	return shadow;
}

float PCFShadow(vec4 sc, int cascadeIndex)
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
			shadowFactor += textureProj(sc, vec2(dx.x * x, dx.y * y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}


float getShadowBias(vec3 lightDirection, vec3 normal, int shadowIndex)
{
	float minBias = ubo.initialBias;
	float bias = max(minBias * (1.0 - dot(normal, lightDirection)), minBias);
	return bias;
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

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
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
  	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 lighting(vec3 F0, vec3 wsPos, Material material,vec2 fragTexCoord)
{
	vec3 result = vec3(0.0);
	
	if( ubo.lightCount == 0)
	{
		return material.albedo.rgb;
	}
	
	for(int i = 0; i < ubo.lightCount; i++)
	{
		Light light = ubo.lights[i];
		float value = 1.0;

		float intensity = pow(light.intensity,1.4) + 0.1;

		vec3 lightColor = light.color.xyz * intensity;
		vec3 indirect = vec3(0,0,0);
		vec3 lightDir = vec3(0,0,0);
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
			lightDir = L;
		}
		else if (light.type == 1.0)
		{
			vec3 L = light.position.xyz - wsPos;
			float cutoffAngle   = 1.0f - light.angle;      
			float dist          = length(L);
			L = normalize(L);
			float theta         = dot(L.xyz, L);
			float epsilon       = cutoffAngle - cutoffAngle * 0.9f;
			float attenuation 	= ((theta - cutoffAngle) / epsilon); // atteunate when approaching the outer cone
			attenuation         *= light.radius / (pow(dist, 2.0) + 1.0);//saturate(1.0f - dist / light.range);
			float intensity 	= attenuation * attenuation;
			// Erase light if there is no need to compute it
			intensity *= step(theta, cutoffAngle);
			value = clamp(attenuation, 0.0, 1.0);
			lightDir = L;
		}
		else
		{
			lightDir = light.direction.xyz * -1;
			if(ubo.enableShadow == 1.0)
			{
				int cascadeIndex = calculateCascadeIndex(
					ubo.viewMatrix,wsPos,ubo.shadowCount,ubo.splitDepths
				);
				vec4 shadowCoord = (ubo.biasMat * ubo.shadowTransform[cascadeIndex]) * vec4(wsPos, 1.0);
				shadowCoord = shadowCoord * ( 1.0 / shadowCoord.w);
				value = PCFShadow(shadowCoord , cascadeIndex);
			}
		}

		vec3 Li = lightDir.xyz;
		vec3 Lradiance = lightColor;
		vec3 Lh = normalize(Li + material.view);
		
		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(material.normal, Li));
		float cosLh = max(0.0, dot(material.normal, Lh));
		
		vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, material.view)));
		
		float D = ndfGGX(cosLh, material.roughness);
		float G = gaSchlickGGX(cosLi, material.normalDotView, material.roughness);
		
		vec3 kd = (1.0 - F) * (1.0 - material.metallic.x);
		vec3 diffuseBRDF = kd * material.albedo.xyz / PI;
		
		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * cosLi * material.normalDotView);
		
		vec4 indirectShading = texture(uIndirectLight,fragTexCoord);

		if(ubo.enableLPV == 1)
		{
			vec3 indirect = ( diffuseBRDF + specularBRDF ) * indirectShading.rgb;
			indirectShading = vec4(indirect,1);
		}	

		vec3 directShading = (diffuseBRDF + specularBRDF) * Lradiance * cosLi * value;

		result += directShading + indirectShading.rgb;
	}
	return result ;
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
	vec3 F = fresnelSchlickRoughness(F0, material.normalDotView, material.roughness);
	vec3 kd = (1.0 - F) * (1.0 - material.metallic.r);
	vec3 diffuseIBL = irradiance * material.albedo.rgb;

	vec3 specularIrradiance = textureLod(uPrefilterMap, Lr, material.roughness * level).rgb;
	vec2 specularBRDF = texture(uPreintegratedFG, vec2(material.normalDotView, material.roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
	
	return kd * diffuseIBL + specularIBL;
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
	vec4 albedo = texture(uColorSampler, fragTexCoord);
	if (albedo.a < 0.1) {
		discard;
	}
	vec4 fragPosXyzw = texture(uPositionSampler, fragTexCoord);
	vec4 normalTex	 = texture(uNormalSampler, fragTexCoord);
	vec4 pbr		 = texture(uPBRSampler,	fragTexCoord);
	
	Material material;
    material.albedo			= albedo;
    material.metallic		= vec3(pbr.r);
    material.roughness		= pbr.g;
    material.normal			= normalize(normalTex.xyz);
	material.ao				= pbr.z;
	material.ssao			= 1;
	vec3 emissive 			= vec3(fragPosXyzw.w,normalTex.w,pbr.w);

	if(ubo.ssaoEnable == 1)
	{
		material.ssao = texture(uSSAOSampler,fragTexCoord).r;
	}

	vec3 wsPos = fragPosXyzw.xyz;
	material.view 			= normalize(ubo.cameraPosition.xyz -wsPos);
	material.normalDotView  = max(dot(material.normal, material.view), 0.0);

	vec4 viewPos = ubo.viewMatrix * vec4(wsPos, 1.0);
	
	vec3 Lr =  reflect(-material.view,material.normal); 
	//2.0 * material.normalDotView * material.normal - material.view;
	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, material.albedo.rgb, material.metallic.r);
	
	vec3 iblContribution = IBL(F0, Lr, material);
	vec3 lightContribution = lighting(F0, wsPos, material,fragTexCoord);

	vec3 finalColor = (lightContribution + iblContribution) * material.ao * material.ssao + emissive;

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
			outColor = vec4(texture(uSSAOSampler, fragTexCoord).rrr,1.0);
			break;
			case 6:
			outColor = vec4(material.normal,1.0);
			break;
            case 7:
			int cascadeIndex = calculateCascadeIndex(
				ubo.viewMatrix,fragPosXyzw.xyz,ubo.shadowCount,ubo.splitDepths
			);

			switch(cascadeIndex)
			{
				case 0 : outColor = outColor * vec4(0.8,0.2,0.2,1.0); break;
				case 1 : outColor = outColor * vec4(0.2,0.8,0.2,1.0); break;
				case 2 : outColor = outColor * vec4(0.2,0.2,0.8,1.0); break;
				case 3 : outColor = outColor * vec4(0.8,0.8,0.2,1.0); break;
			}
			break;
			case 8:
			outColor = texture(uDepthSampler,fragTexCoord);
			break;
			case 9:
			outColor = texture(uPositionSampler,fragTexCoord);
			break;
			case 10:
			outColor = texture(uPBRSampler,fragTexCoord);
			break;			
			case 11:
			outColor = vec4(texture(uShadowMap,vec3(fragTexCoord,0)).rgb,1);
			break;
		}
	}
}


