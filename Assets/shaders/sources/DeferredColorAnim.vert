#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UniformBufferObject 
{    
	mat4 projView;
    mat4 view;
	mat4 projViewOld;
} ubo;

const int MAX_BONES = 100;

layout(set = 0,binding = 1) uniform UniformBufferObjectAnim
{    
	mat4 boneTransforms[MAX_BONES];
} boneUbo;


layout(push_constant) uniform PushConsts
{
	mat4 transform;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in ivec4 inBoneIndices;
layout(location = 6) in vec4 inBoneWeights;


layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragPosition;
layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragTangent;
layout(location = 5) out vec4 fragProjPosition;
layout(location = 6) out vec4 fragOldProjPosition;
layout(location = 7) out vec4 fragViewPosition;



out gl_PerVertex
{
    vec4 gl_Position;
};

mat4 getSkinMat()
{
     mat4 boneTransform = boneUbo.boneTransforms[int(inBoneIndices[0])] * inBoneWeights[0];
    boneTransform += boneUbo.boneTransforms[int(inBoneIndices[1])] * inBoneWeights[1];
    boneTransform += boneUbo.boneTransforms[int(inBoneIndices[2])] * inBoneWeights[2];
    boneTransform += boneUbo.boneTransforms[int(inBoneIndices[3])] * inBoneWeights[3];
    return boneTransform;
}

void main() 
{
	fragPosition = pushConsts.transform * (getSkinMat() * vec4(inPosition, 1.0));
    vec4 pos =  ubo.projView * fragPosition;
    gl_Position = pos;
    
    fragColor = inColor;
	fragTexCoord = inTexCoord;
    fragNormal =  transpose(inverse(mat3(pushConsts.transform))) * normalize(inNormal);
    
    fragTangent = inTangent;

    fragProjPosition = pos;
    fragOldProjPosition = ubo.projViewOld * pushConsts.transform * vec4(inPosition, 1.0);;
    fragViewPosition = ubo.view * fragPosition;
}