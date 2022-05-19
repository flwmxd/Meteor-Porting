#ifndef LIGHT_GLSL
#define LIGHT_GLSL

/**
 * DirectionalLight = 0,
 * SpotLight = 1,
 * PointLight = 2,
 */

struct Light
{
	vec4 color;
	vec4 position;
	vec4 direction;
	float intensity;
	float radius;
	float type;
	float angle;
};

#define MAX_LIGHTS 32

#define MAX_SHADOWMAPS 4

int calculateCascadeIndex(mat4 viewMatrix, vec3 wsPos, int shadowCount, vec4 splitDepths[MAX_SHADOWMAPS])
{
	int cascadeIndex = 0;
	vec4 viewPos = viewMatrix * vec4(wsPos, 1.0) ;

	for(int i = 0; i < shadowCount - 1; ++i)
	{
		if(viewPos.z < splitDepths[i].x)
		{
			cascadeIndex = i + 1;
		}
	}
	return cascadeIndex;
}

#endif