#version 450

layout (location = 0) in vec2 inUV;

layout (set = 0,binding = 0) uniform sampler2D uRenderTexture;

layout (set = 0,binding = 1) uniform UBO
{
    float fxaaSpanMax;
    float fxaaReduceMin;
    float fxaaReduceMul;
} ubo;


layout (location = 0) out vec4 outColor;

void main()
{
    vec2 texelSize = 1.0 / textureSize(uRenderTexture, 0);
	
	vec3 luma = vec3(0.299, 0.587, 0.114);	
	float lumaTL = dot(luma, texture(uRenderTexture, inUV.xy + (vec2(-1.0, -1.0) * texelSize)).xyz);
	float lumaTR = dot(luma, texture(uRenderTexture, inUV.xy + (vec2(1.0, -1.0) * texelSize)).xyz);
	float lumaBL = dot(luma, texture(uRenderTexture, inUV.xy + (vec2(-1.0, 1.0) * texelSize)).xyz);
	float lumaBR = dot(luma, texture(uRenderTexture, inUV.xy + (vec2(1.0, 1.0) * texelSize)).xyz);
	float lumaM  = dot(luma, texture(uRenderTexture, inUV.xy).xyz);

	
	vec2 dir;
	dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
	dir.y = ((lumaTL + lumaBL) - (lumaTR + lumaBR));
	
	float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (ubo.fxaaReduceMul * 0.25), ubo.fxaaReduceMin);
	float inverseDirAdjustment = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = min(vec2(ubo.fxaaSpanMax, ubo.fxaaSpanMax), 
		max(vec2(-ubo.fxaaSpanMax, -ubo.fxaaSpanMax), dir * inverseDirAdjustment));
	
	dir.x = dir.x * step(1.0, abs(dir.x));
	dir.y = dir.y * step(1.0, abs(dir.y));
	
	dir = dir * texelSize;

	vec3 result1 = (1.0/2.0) * (
		texture(uRenderTexture, inUV.xy + (dir * vec2(1.0/3.0 - 0.5))).xyz +
		texture(uRenderTexture, inUV.xy + (dir * vec2(2.0/3.0 - 0.5))).xyz);

	vec3 result2 = result1 * (1.0/2.0) + (1.0/4.0) * (
		texture(uRenderTexture, inUV.xy + (dir * vec2(0.0/3.0 - 0.5))).xyz +
		texture(uRenderTexture, inUV.xy + (dir * vec2(3.0/3.0 - 0.5))).xyz);

	float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
	float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));
	float lumaResult2 = dot(luma, result2);
	
	vec4 color;
	if(lumaResult2 < lumaMin || lumaResult2 > lumaMax)
		color = vec4(result1, 1.0);
	else
		color = vec4(result2, 1.0);

	outColor = color;
}