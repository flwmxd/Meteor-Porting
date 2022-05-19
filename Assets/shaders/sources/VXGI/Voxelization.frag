#version 450
#extension GL_ARB_shader_image_load_store : require

#include "VXGI.glsl"

layout(location = 0) in GeometryOut
{
    vec3 wsPosition;
    vec3 position;
    vec3 normal;
    vec2 texCoord;
    flat vec4 triangleAABB;
} In;

layout (location = 0) out vec4 fragColor;
layout (pixel_center_integer) in vec4 gl_FragCoord;

layout(set = 2, binding = 0, r32ui) uniform volatile coherent uimage3D uVoxelAlbedo;
layout(set = 2, binding = 1, r32ui) uniform volatile coherent uimage3D uVoxelNormal;
layout(set = 2, binding = 2, r32ui) uniform volatile coherent uimage3D uVoxelEmission;
layout(set = 2, binding = 3, r8)    uniform image3D uStaticVoxelFlag;
layout(set = 2, binding = 4)        uniform UniformBufferObject
{
    uint flagStaticVoxels;
}ubo;

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



vec4 convRGBA8ToVec4(uint val)
{
    return vec4(float((val & 0x000000FF)), 
    float((val & 0x0000FF00) >> 8U), 
    float((val & 0x00FF0000) >> 16U), 
    float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
    return (uint(val.w) & 0x000000FF) << 24U | 
    (uint(val.z) & 0x000000FF) << 16U | 
    (uint(val.y) & 0x000000FF) << 8U | 
    (uint(val.x) & 0x000000FF);
}

#define imageAtomicRGBA8Avg(grid, coords, v)        \
{                                                   \
    vec4 value = v;                                 \
    value.rgb *= 255.0;                             \
    uint newVal = convVec4ToRGBA8(value);           \
    uint prevStoredVal = 0;                         \
    uint curStoredVal;                              \
    uint numIterations = 0;                         \
    while((curStoredVal = imageAtomicCompSwap(grid, coords, prevStoredVal, newVal)) \
            != prevStoredVal                        \
            && numIterations < 255)                 \
    {                                               \
        prevStoredVal = curStoredVal;               \
        vec4 rval = convRGBA8ToVec4(curStoredVal);  \
        rval.rgb = (rval.rgb * rval.a);             \
        vec4 curValF = rval + value;                \
        curValF.rgb /= curValF.a;                   \
        newVal = convVec4ToRGBA8(curValF);          \
        ++numIterations;                            \
    }                                               \
}

vec4 getAlbedo()
{
	return (1.0 - materialProperties.usingAlbedoMap) * materialProperties.albedoColor + materialProperties.usingAlbedoMap * texture(uAlbedoMap, In.texCoord);
}

vec3 getEmissive()
{
	return (1.0 - materialProperties.usingEmissiveMap) * materialProperties.emissiveColor.rgb + materialProperties.usingEmissiveMap * texture(uEmissiveMap, In.texCoord).rgb;
}

#define GAMMA 2.2

vec4 gammaCorrectTexture(vec4 samp)
{
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

vec3 gammaCorrectTextureRGB(vec3 samp)
{
	return vec3(pow(samp, vec3(GAMMA)));
}

void main()
{
    if( In.position.x < In.triangleAABB.x || In.position.y < In.triangleAABB.y || 
		In.position.x > In.triangleAABB.z || In.position.y > In.triangleAABB.w )
	{
		discard;
	}

    // writing coords position
    ivec3 position = ivec3(In.wsPosition);
    vec4 albedo = getAlbedo();
    float opacity = 1.0f;

    if(ubo.flagStaticVoxels == 0)
    {
        bool isStatic = imageLoad(uStaticVoxelFlag, position).r > 0.0f;
        if(isStatic) opacity = 0.0f;
    }

    if(opacity > 0.0f)
    {
        // premultiplied alpha
        albedo.rgb *= opacity;
        albedo.a = 1.0f;
        albedo = gammaCorrectTexture(albedo);
        // emission
        vec4 emissive = vec4(gammaCorrectTextureRGB(getEmissive()),1.0);
        // bring normal to 0-1 range
        vec4 normal = vec4(encodeNormal(normalize(In.normal)), 1.0f);
        imageAtomicRGBA8Avg(uVoxelNormal, position, normal);
        imageAtomicRGBA8Avg(uVoxelAlbedo, position, albedo);
        imageAtomicRGBA8Avg(uVoxelEmission, position, emissive);
        // doing a static flagging pass for static geometry voxelization
        if(ubo.flagStaticVoxels == 1)
        {
            imageStore(uStaticVoxelFlag, position, vec4(1.0));
        }
    }
}