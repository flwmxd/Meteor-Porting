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
layout( location = 3 ) in vec4 inColor;

layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;

layout(set = 1 , binding = 1) uniform UniformMaterialAlbedo
{
	vec4 albedoColor;
	float usingAlbedoMap;
	float padding0;
	float padding1;
	float padding2;
}ubo;

layout(set = 2, binding = 0 ) uniform LightUBO
{
    Light light;
}uboLight;


layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

vec4 gammaCorrectTexture(vec4 samp)
{
	return vec4(pow(samp.rgb, vec3(2.2)), samp.a);
}

vec4 getAlbedo()
{
	return (1.0 - ubo.usingAlbedoMap) * ubo.albedoColor + ubo.usingAlbedoMap * texture(uAlbedoMap, inUV);
}

float lengthSquared(vec3 vec)
{
	return pow(length(vec),2);
}

void main()
{
	vec4 diffuse = gammaCorrectTexture(getAlbedo() * inColor);
	float intensity = pow(uboLight.light.intensity,1.4) + 0.1;
	vec4 flux = vec4( ( uboLight.light.color.rgb * diffuse.rgb * intensity ) , 1.0 );
	outColor = flux;
	outPosition = inPosition;
	outNormal = vec4(inNormal,1.0);
}