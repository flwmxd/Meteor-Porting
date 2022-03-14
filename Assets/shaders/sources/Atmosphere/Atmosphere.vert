#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;


layout(location = 0) out vec2 fragPosition;

void main()
{
	gl_Position =  vec4(inPosition, 1.0);
	gl_Position.z = 1.0;
	vec4 testColor = inColor;
	fragPosition = inTexCoord;
}

