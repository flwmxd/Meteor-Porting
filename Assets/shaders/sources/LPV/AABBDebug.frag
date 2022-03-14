#version 450

#include "LPVCommon.glsl"

layout (location = 0) in vec3 inPosition; // in view space
layout (location = 1) in vec3 inNormal;

layout(set = 1, binding = 0) uniform UniformBufferObjectFrag
{
	vec3 minAABB;
	float cellSize;
}ubo;

layout(set = 1, binding = 1) uniform usampler3D uRAccumulatorLPV;
layout(set = 1, binding = 2) uniform usampler3D uGAccumulatorLPV;
layout(set = 1, binding = 3) uniform usampler3D uBAccumulatorLPV;

layout (location = 0) out vec4 outColor;

ivec3 convertPointToGridIndex(vec3 vPos) {
	return ivec3((vPos - ubo.minAABB) / ubo.cellSize);
}

void main()
{
	vec4 shIntensity = dirToSH(normalize(inNormal));
	ivec3 cellCoords = convertPointToGridIndex(inPosition);
	vec3 lpvIntensity = vec3( 
			dot(shIntensity, texelFetch2(uRAccumulatorLPV, cellCoords) ),
			dot(shIntensity, texelFetch2(uGAccumulatorLPV, cellCoords) ),
			dot(shIntensity, texelFetch2(uBAccumulatorLPV, cellCoords) )
		);

	outColor = vec4(max(lpvIntensity, 0) ,1);
}
