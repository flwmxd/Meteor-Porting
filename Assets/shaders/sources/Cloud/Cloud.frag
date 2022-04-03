#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0)  uniform sampler2D uCloudSampler;

layout(location = 0) out vec4 outFrag;

#define GAMMA 2.2

vec4 gammaCorrectTexture(vec4 samp)
{
	return vec4(pow(samp.rgb, vec3(GAMMA)), samp.a);
}

void main()
{
	outFrag = gammaCorrectTexture(vec4(texture(uCloudSampler,inUV).rgb, 1.0));
}