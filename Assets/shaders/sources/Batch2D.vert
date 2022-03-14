#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inTexCoord;

layout(set = 0,binding = 0) uniform UniformBufferObject
{
	mat4 projView;
} ubo;

layout (location = 0) out OutData
{
	vec2 uv;
	vec4 color;
	flat int textureID;
} data;

void main()
{
	gl_Position =  ubo.projView * vec4(inPosition,1.0);
	
	data.color = inColor;
	data.uv = inTexCoord.xy;
	data.textureID = int(inTexCoord.z - 0.5);
}