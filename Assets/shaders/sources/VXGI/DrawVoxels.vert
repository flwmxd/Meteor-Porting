#version 450
//#extension GL_ARB_shader_image_load_store : require
//#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0,rgba8) uniform image3D uVoxelBuffer;
layout(set = 0, binding = 1) uniform UniformBufferObjectVert 
{    
	vec4 colorChannels;
    int volumeDimension;
	int type;
	int padd1;
	int padd2;
} ubo;

layout(location = 0) out vec4 albedo;

void main()
{
	vec3 position = vec3
	(
		gl_VertexIndex % ubo.volumeDimension,
		(gl_VertexIndex / ubo.volumeDimension) % ubo.volumeDimension,
		gl_VertexIndex / (ubo.volumeDimension * ubo.volumeDimension)
	);

	ivec3 texPos = ivec3(position);
	
	albedo = imageLoad(uVoxelBuffer, texPos);

	uvec4 channels = uvec4(floor(ubo.colorChannels));

	albedo = vec4(albedo.rgb * channels.rgb, albedo.a);
	if(all(equal(channels.rgb, uvec3(0))) && channels.a > 0) 
	{
		albedo = vec4(albedo.a);
	}

	gl_Position = vec4(position, 1.0f);
}