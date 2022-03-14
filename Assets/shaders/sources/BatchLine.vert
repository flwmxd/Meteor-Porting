#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

layout(set = 0,binding = 0) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

layout (location = 0) out vec4 outColor;

void main()
{
	gl_Position =  ubo.projView * vec4(position,1.0);
	outColor = color;
}