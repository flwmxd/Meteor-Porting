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
    mat4 lightProjection;
    mat4 viewMatrix;
} ubo;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec4 fragColor;

void main()
{
    vec4 worldPosition =  pushConsts.transform * vec4(inPosition, 1.0);
    fragPosition = worldPosition;
    
    fragColor = inColor;
	fragTexCoord = inTexCoord;

    fragNormal =  normalize(transpose(inverse(mat3(  pushConsts.transform ) ) ) * inNormal);

	gl_Position = ubo.lightProjection * worldPosition;
}