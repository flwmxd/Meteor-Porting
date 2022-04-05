#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#define GAMMA 2.2

layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform UniformBuffer
{
	float gamma;
	float exposure;
	int toneMapIndex;
	int ssaoEnable;
	int reflectEnable;
	int cloudEnable;
	int padding1;
	int padding2;
} ubo;

layout(set = 0, binding = 0)  uniform sampler2D uScreenSampler;
layout(set = 0, binding = 1)  uniform sampler2D uReflectionSampler;
layout(set = 0, binding = 2)  uniform sampler2D uBloomSampler;
//layout(set = 0, binding = 2)  uniform sampler2D uDepthSampler;
//layout(set = 0, binding = 2)  uniform sampler2D uCloudSampler;


layout(location = 0) out vec4 outFrag;


// Based on http://www.oscars.org/science-technology/sci-tech-projects/aces
vec3 ACESTonemap(vec3 color)
{
	mat3 m1 = mat3(
				   0.59719, 0.07600, 0.02840,
				   0.35458, 0.90834, 0.13383,
				   0.04823, 0.01566, 0.83777
				   );
	mat3 m2 = mat3(
				   1.60475, -0.10208, -0.00327,
				   -0.53108, 1.10813, -0.07276,
				   -0.07367, -0.00605, 1.07602
				   );
	vec3 v = m1 * color;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

vec3 simpleReinhardToneMapping(vec3 color)
{
	float exposure = 1.5;
	color *= exposure/(1. + color / exposure);
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 lumaBasedReinhardToneMapping(vec3 color)
{
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 whitePreservingLumaBasedReinhardToneMapping(vec3 color)
{
	float white = 2.;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 RomBinDaHouseToneMapping(vec3 color)
{
    color = exp( -1.0 / ( 2.72*color + 0.15 ) );
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}

vec3 filmicToneMapping(vec3 color)
{
	color = max(vec3(0.), color - vec3(0.004));
	color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
	return color;
}

vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(color, vec3(1. / ubo.gamma));
	return color;
}


vec3 finalGamma(vec3 color)
{
	color = vec3(1.0) - exp(-color * ubo.exposure);
	return pow(color, vec3(1.0 / ubo.gamma));
}


void main()
{
	vec4 albedo = texture(uScreenSampler, inUV);
	vec4 bloom = texture(uBloomSampler, inUV);
	vec3 color = albedo.rgb + bloom.rgb;
	
	if( ubo.ssaoEnable == 1 && albedo.a >= 0.1)
	{
		//float ssao = texture(uSSAOSampler,inUV).r;
		//color = color * ssao;
	}
	
	if( ubo.reflectEnable == 1 )
	{
		vec4 SSR = texture(uReflectionSampler,inUV);
		color = SSR.rgb * SSR.a + color * (1.0 - SSR.a);
	}

	//ivec2 size = textureSize(uCloudSampler,0);

	if(ubo.cloudEnable == 1)
	{
		//vec3 cloud = texture(uCloudSampler, inUV).xyz;
		//float mixVal = (texture(uDepthSampler, inUV).r < 1.0 ? 0.0 : 1.0);
		//color = mix(color.xyz, cloud, mixVal);
	}
	//hdr
	//col *= ubo.exposure;

	int i = ubo.toneMapIndex;
	if (i == 1) color = finalGamma(color);
	if (i == 2) color = simpleReinhardToneMapping(color);
	if (i == 3) color = lumaBasedReinhardToneMapping(color);
	if (i == 4) color = whitePreservingLumaBasedReinhardToneMapping(color);
	if (i == 5) color = RomBinDaHouseToneMapping(color);		
	if (i == 6) color = filmicToneMapping(color);
	if (i == 7) color = Uncharted2ToneMapping(color);
	if (i == 8) color = ACESTonemap(color);
	
	outFrag = vec4(color, 1.0);
}