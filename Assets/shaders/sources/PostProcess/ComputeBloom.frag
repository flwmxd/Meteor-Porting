#version 450

layout(binding = 0) uniform sampler2D uScreenSampler;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

void main()
{
	vec3 color = texture(uScreenSampler, inUV).rgb;
    float l = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float threshold = 0.75;
    outColor.rgb = (l > threshold) ? color : vec3(0.0);
    outColor.a = 1.0;
}