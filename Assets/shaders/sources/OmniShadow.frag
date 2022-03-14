#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inLightPos;
layout (location = 2) in float radius;


layout (location = 0) out vec4 outFragColor;

// Store distance to light as 32 bit float value
void main(void)
{
    vec3 lightVec = inPos.xyz - inLightPos;
    outFragColor = vec4(vec3(length(lightVec) / radius),1.0);

    //debugPrintfEXT("distance %f \n",outFragColor.r);


}