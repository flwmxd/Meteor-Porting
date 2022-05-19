#ifndef _VXGI_
#define _VXGI_

const float HALF_PI = 1.57079f;

const ivec3 anisoOffsets[] = ivec3[8]
(
	ivec3(1, 1, 1),
	ivec3(1, 1, 0),
	ivec3(1, 0, 1),
	ivec3(1, 0, 0),
	ivec3(0, 1, 1),
	ivec3(0, 1, 0),
	ivec3(0, 0, 1),
	ivec3(0, 0, 0)
);

vec3 voxelToWorld(vec3 pos, vec3 worldMinPoint, float voxelTexSize)
{
	return pos * voxelTexSize + worldMinPoint;
}

vec3 encodeNormal(vec3 normal)
{
    return normal * 0.5f + vec3(0.5f);
}

vec3 decodeNormal(vec3 normal)
{
    return normal * 2.0f - vec3(1.0f);
}

vec3 worldToVoxel(vec3 position, vec3 worldMinPoint, float voxelScale)
{
    vec3 voxelPos = position - worldMinPoint;
    return voxelPos * voxelScale;
}

#endif
