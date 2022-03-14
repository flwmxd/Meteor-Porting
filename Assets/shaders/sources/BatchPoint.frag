#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 color;

layout (location = 0) in Data
{
	vec3 position;
	vec4 color;
	vec2 size;
	vec2 uv;
} fs_in;

void main()
{

	float distance = dot(fs_in.uv, fs_in.uv);
	if (distance > 1.0)
	{
		discard;
	}
	color = fs_in.color;
}
