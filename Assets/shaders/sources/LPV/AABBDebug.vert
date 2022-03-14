#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;


layout(set = 0, binding = 0) uniform UniformBufferObjectVert
{
	mat4 projView;
}ubo;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;


layout (location = 0) out vec3 outPosition; // in view space
layout (location = 1) out vec3 outNormal;


void main()
{
	vec4 testC = inColor;
	vec2 testCc = inTexCoord;

	vec4 fragPosInViewSpace =  pushConsts.transform * vec4(inPosition, 1.0f);
	gl_Position = ubo.projView * fragPosInViewSpace;
	outNormal = inNormal;	
	outPosition = vec3(fragPosInViewSpace);
}