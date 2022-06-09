#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0,binding = 0) uniform UniformBufferObjectVert 
{    
	mat4 projView;
} ubo;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out Vertex
{
	vec2 fragTexCoord;
	vec3 fragNormal;
}outVertex;

void main()
{
    vec4 fragPosition = pushConsts.transform * vec4(inPosition, 1.0f);

    vec4 fragColor = inColor;

    outVertex.fragTexCoord = inTexCoord;

    outVertex.fragNormal =  transpose(inverse(mat3(pushConsts.transform))) * normalize(inNormal);

    gl_Position = fragPosition;
}