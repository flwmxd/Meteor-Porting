#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0)  uniform sampler2D uCloudSampler;

layout(location = 0) out vec4 outFrag;


void main()
{
	outFrag = vec4(texture(uCloudSampler,inUV).rgb, 1.0);
}