#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(set = 0,binding = 0) uniform UniformBufferObject 
{    
	mat4 projView;
} ubo;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

out gl_PerVertex
{
	vec4 gl_Position;
};


void main() 
{
    const float outlineWidth = 0.03;
	// Extrude along normal
	vec4 position = pushConsts.transform * vec4(inPosition ,1.0);

    vec4 fragColor = inColor;
	vec2 fragTexCoord = inTexCoord;

	vec4 fragPosition = vec4(position.xyz +  inNormal * outlineWidth ,1.0);

   // vec3 fragNormal = transpose(inverse(mat3(pushConsts.transform))) * normal;

	gl_Position = ubo.projView * fragPosition;
}
