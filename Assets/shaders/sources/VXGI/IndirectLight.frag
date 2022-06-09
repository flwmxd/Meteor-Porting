#version 450

#include "../Common/Light.h"
#include "../Common/Math.h"
#include "VXGI.glsl"

layout(binding = 0)  uniform sampler3D uVoxelTex; 
layout(binding = 1)  uniform sampler3D uVoxelTexMipmap[6];
layout(binding = 7)  uniform sampler2D uColorSampler;
layout(binding = 8)  uniform sampler2D uPositionSampler;
layout(binding = 9)  uniform sampler2D uNormalSampler;
layout(binding = 10) uniform sampler2D uPBRSampler;
layout(binding = 11) uniform UniformBufferVXGI
{
	float voxelScale;
	float maxTracingDistanceGlobal;
	float aoFalloff;
	float bounceStrength;

    vec4 cameraPosition;

	vec3 worldMinPoint;
	float aoAlpha;

	vec3 worldMaxPoint;
	float samplingFactor;

	int volumeDimension;
	float worldSize; //max in aabb
	float padding;
	float padding1;
} uboVXGI;

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

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


const vec3 diffuseConeDirections[] =
{
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.5f, 0.866025f),
    vec3(0.823639f, 0.5f, 0.267617f),
    vec3(0.509037f, 0.5f, -0.7006629f),
    vec3(-0.50937f, 0.5f, -0.7006629f),
    vec3(-0.823639f, 0.5f, 0.267617f)
};

const float diffuseConeWeights[] =
{
    PI / 4.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
};


bool intersectRayWithWorldAABB(vec3 ro, vec3 rd, out float enter, out float leave)
{
    vec3 tempMin = (uboVXGI.worldMinPoint - ro) / rd; 
    vec3 tempMax = (uboVXGI.worldMaxPoint - ro) / rd;
    
    vec3 v3Max = max (tempMax, tempMin);
    vec3 v3Min = min (tempMax, tempMin);
    
    leave = min (v3Max.x, min (v3Max.y, v3Max.z));
    enter = max (max (v3Min.x, 0.0), max (v3Min.y, v3Min.z));    
    
    return leave > enter;
}

vec4 anistropicSample(vec3 coord, vec3 weight, uvec3 face, float lod)
{
    // anisotropic volumes level
    float anisoLevel = min(max(lod - 1.0f, 0.0f),7);
    // directional sample
	// 128 64 32 16 8 4 2 1
    vec4 anisoSample = weight.x * textureLod(uVoxelTexMipmap[face.x], coord, anisoLevel)
                     + weight.y * textureLod(uVoxelTexMipmap[face.y], coord, anisoLevel)
                     + weight.z * textureLod(uVoxelTexMipmap[face.z], coord, anisoLevel);
    // linearly interpolate on base level
    if(lod < 1.0f)
    {
        //ivec3 iCoord = ivec3(uboVXGI.voxelScale * coord);
        vec4 baseColor = texture(uVoxelTex, coord);
        anisoSample = mix(baseColor, anisoSample, clamp(lod, 0.0f, 1.0f));
    }
    return anisoSample;                    
}


vec4 traceCone(vec3 position, vec3 normal, vec3 direction, float aperture, bool traceOcclusion)
{
    uvec3 visibleFace;
    visibleFace.x = (direction.x < 0.0) ? 0 : 1;
    visibleFace.y = (direction.y < 0.0) ? 2 : 3;
    visibleFace.z = (direction.z < 0.0) ? 4 : 5;
    traceOcclusion = traceOcclusion && uboVXGI.aoAlpha < 1.0f;
    // world space grid voxel size
    float voxelWorldSize = 2.0 * uboVXGI.worldSize / uboVXGI.volumeDimension;
    // weight per axis for aniso sampling
    vec3 weight = direction * direction;
    // move further to avoid self collision
    float dst = voxelWorldSize;
    vec3 startPosition = position + normal * dst;
    // final results
    vec4 coneSample = vec4(0.0f);
    float occlusion = 0.0f;
    float maxDistance = uboVXGI.maxTracingDistanceGlobal * uboVXGI.worldSize;
    float falloff = 0.5f * uboVXGI.aoFalloff * uboVXGI.voxelScale;
    // out of boundaries check
    float enter = 0.0; float leave = 0.0;

    if(!intersectRayWithWorldAABB(position, direction, enter, leave))
    {
        coneSample.a = 1.0f;
    }

    while(coneSample.a < 1.0f && dst <= maxDistance)
    {
        vec3 conePosition = startPosition + direction * dst;
        // cone expansion and respective mip level based on diameter
        float diameter = 2.0f * aperture * dst;
        float mipLevel = log2(diameter / voxelWorldSize);
        // convert position to texture coord
        vec3 coord = worldToVoxel(conePosition,uboVXGI.worldMinPoint,uboVXGI.voxelScale);
        // get directional sample from anisotropic representation
        vec4 anisoSample = anistropicSample(coord, weight, visibleFace, mipLevel);
        // front to back composition
        coneSample += (1.0f - coneSample.a) * anisoSample;
        // ambient occlusion
        if(traceOcclusion && occlusion < 1.0)
        {
            occlusion += ((1.0f - occlusion) * anisoSample.a) / (1.0f + falloff * diameter);
        }
        // move further into volume
        dst += diameter * uboVXGI.samplingFactor;
    }

    return vec4(coneSample.rgb, occlusion);
}


vec4 calculateIndirectLightingFromVXGI(vec3 position, vec3 normal, vec3 viewDir, vec3 albedo, float roughness, bool ambientOcclusion)
{
    vec4 specularTrace = vec4(0.0f);
    vec4 diffuseTrace = vec4(0.0f);
    vec3 coneDirection = vec3(0.0f);

    if(roughness > 0)
    {
        vec3 coneDirection = reflect(-viewDir, normal);
        coneDirection = normalize(coneDirection);
		float aperture = tan (HALF_PI * roughness) + 0.01;
        specularTrace = traceCone(position, normal, coneDirection, aperture, false);
        specularTrace.rgb *= (1 - roughness);
    }

    // component greater than zero
    if(any(greaterThan(albedo, diffuseTrace.rgb)))
    {
        // diffuse cone setup
        const float aperture = 0.57735f;
        vec3 guide = vec3(0.0f, 1.0f, 0.0f);

        if (abs(dot(normal,guide)) == 1.0f)
        {
            guide = vec3(0.0f, 0.0f, 1.0f);
        }

        // Find a tangent and a bitangent
        vec3 right = normalize(guide - dot(normal, guide) * normal);
        vec3 up = cross(right, normal);

        for(int i = 0; i < 6; i++)
        {
            coneDirection = normal;
            coneDirection += diffuseConeDirections[i].x * right + diffuseConeDirections[i].z * up;
            coneDirection = normalize(coneDirection);
            // cumulative result
            diffuseTrace += traceCone(position, normal, coneDirection, aperture, ambientOcclusion) * diffuseConeWeights[i];
        }
        diffuseTrace.rgb *= albedo;
    }
    vec3 result = uboVXGI.bounceStrength * (diffuseTrace.rgb + specularTrace.rgb);
    return vec4(result, ambientOcclusion ? clamp(1.0f - diffuseTrace.a + uboVXGI.aoAlpha, 0.0f, 1.0f) : 1.0f);
}


void main()
{

	vec4 albedo      = texture(uColorSampler, inUV);
	vec4 fragPosXyzw = texture(uPositionSampler, inUV);
	vec4 normalTex	 = texture(uNormalSampler, inUV);
	vec4 pbr		 = texture(uPBRSampler, inUV);
	
	Material material;
    material.albedo		= albedo;
    material.metallic	= vec3(pbr.r);
    material.roughness	= pbr.g;
    material.normal		= normalize(normalTex.xyz);
	material.ao			= pbr.z;
    
	material.view 			= normalize(uboVXGI.cameraPosition.xyz - fragPosXyzw.xyz);
	material.normalDotView  = max(dot(material.normal, material.view), 0.0);
    vec4 color = calculateIndirectLightingFromVXGI(fragPosXyzw.xyz, material.normal, material.view, material.albedo.xyz, material.roughness ,true);
    outColor = vec4(color.rgb,1.0);
}