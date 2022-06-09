#version 450
//#extension GL_ARB_separate_shader_objects : enable

#include "VXGI.glsl"

layout(points) in;

//one square
layout(triangle_strip, max_vertices = 24) out;

layout(location = 0) in vec4 inAlbedo[];
layout(location = 0) out vec4 outVoxelColor;

layout(set = 1,binding = 1) uniform UniformBufferObjectGemo 
{    
    mat4 mvp;
    vec4 frustumPlanes[6];
    vec3 worldMinPoint;
    float voxelSize;
} ubo;

bool voxelInFrustum(vec3 center, vec3 extent)
{
	vec4 plane;
	for(int i = 0; i < 6; i++)
	{
		plane = ubo.frustumPlanes[i];
		float d = dot(extent, abs(plane.xyz));
		float r = dot(center, plane.xyz) + plane.w;

		if(d + r < 0.0f)
			return false;
	}
	return true;
}

void main()
{
	const vec4 cubeVertices[8] = vec4[8] 
	(
		vec4( 0.5f,  0.5f,  0.5f, 0.0f),
		vec4( 0.5f,  0.5f, -0.5f, 0.0f),
		vec4( 0.5f, -0.5f,  0.5f, 0.0f),
		vec4( 0.5f, -0.5f, -0.5f, 0.0f),
		vec4(-0.5f,  0.5f,  0.5f, 0.0f),
		vec4(-0.5f,  0.5f, -0.5f, 0.0f),
		vec4(-0.5f, -0.5f,  0.5f, 0.0f),
		vec4(-0.5f, -0.5f, -0.5f, 0.0f)
	);

	const int cubeIndices[24]  = int[24] 
	(
		0, 2, 1, 3, // right
		6, 4, 7, 5, // left
		5, 4, 1, 0, // up
		6, 7, 2, 3, // down
		4, 6, 0, 2, // front
		1, 3, 5, 7  // back
	);

	vec3 center = voxelToWorld(
        gl_in[0].gl_Position.xyz,
        ubo.worldMinPoint,
        ubo.voxelSize
    );

	vec3 extent = vec3(ubo.voxelSize);
	const float EPSILON = 1e-30;

	if(inAlbedo[0].a <= EPSILON || !voxelInFrustum(center, extent)) { return; }

	vec4 projectedVertices[8];

	for(int i = 0; i < 8; i++)
	{
		vec4 vertex = gl_in[0].gl_Position + cubeVertices[i];
		projectedVertices[i] = ubo.mvp * vertex;
	}

	for(int face = 0; face < 6; face++)
	{
		for(int vertex = 0; vertex < 4; vertex++)
		{
			gl_Position = projectedVertices[cubeIndices[face * 4 + vertex]];
			outVoxelColor = inAlbedo[0];
			outVoxelColor.a = 1;
			EmitVertex();
		}
	}
	EndPrimitive();
}