#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(set = 0,binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    vec3 lightPos;
    float radius;
} ubo;

layout(push_constant) uniform PushConsts 
{
	mat4 transfrom;
    mat4 lightView;
} pushConsts;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;



layout (location = 0) out vec4 outPos;
layout (location = 1) out vec3 outLightPos;
layout (location = 2) out float radius;

void main()
{

    vec4 pos = pushConsts.transfrom *  vec4(inPosition, 1.0);

    gl_Position = ubo.proj * pushConsts.lightView * pos;

	outPos = pos;
	outLightPos = ubo.lightPos; 
 
    radius = ubo.radius;
}