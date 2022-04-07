#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

const int MAX_BONES = 100;

layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

layout(set = 0,binding = 0) uniform UniformBufferObject
{
    mat4 projView;
	mat4 boneTransforms[MAX_BONES];
} ubo;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec4 inBoneIndices;
layout(location = 6) in vec4 inBoneWeights;

mat4 getSkinMat()
{
    mat4 boneTransform = ubo.boneTransforms[int(inBoneIndices[0])] * inBoneWeights[0];
    boneTransform += ubo.boneTransforms[int(inBoneIndices[1])] * inBoneWeights[1];
    boneTransform += ubo.boneTransforms[int(inBoneIndices[2])] * inBoneWeights[2];
    boneTransform += ubo.boneTransforms[int(inBoneIndices[3])] * inBoneWeights[3];
    return boneTransform;
}

void main()
{
    gl_Position = ubo.projView * pushConsts.transform *  (getSkinMat() * vec4(inPosition, 1.0)); 
}