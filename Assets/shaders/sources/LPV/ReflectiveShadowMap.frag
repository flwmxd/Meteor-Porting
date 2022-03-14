#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

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

layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec2 inUV;
layout( location = 2 ) in vec3 inNormal;

layout(set = 1, binding = 0) uniform sampler2D uDiffuseMap;

layout(set = 1 , binding = 1) uniform UBO
{
    Light light;
	vec4 albedoColor;
	float usingAlbedoMap;
}ubo;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

vec4 getAlbedo()
{
	return (1.0 - ubo.usingAlbedoMap) * ubo.albedoColor + ubo.usingAlbedoMap * texture(uDiffuseMap, inUV);
}

float lengthSquared(vec3 vec)
{
	return pow(length(vec),2);
}

void main()
{
	vec4 diffuse = getAlbedo();
	vec4 flux = vec4( ( ubo.light.color.rgb * diffuse.rgb * ubo.light.intensity ) , 1.0 );
	outColor = flux;
	outPosition = inPosition;
	outNormal = vec4(inNormal,1.0);
}