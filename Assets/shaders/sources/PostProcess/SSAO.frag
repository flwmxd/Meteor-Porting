#version 450

const int SSAO_KERNEL_SIZE = 64;
const float SSAO_RADIUS = 0.25;

layout (set = 0,binding = 0) uniform sampler2D uViewPositionSampler;
layout (set = 0,binding = 1) uniform sampler2D uViewNormalSampler;
layout (set = 0,binding = 2) uniform sampler2D uSsaoNoise;
layout (set = 0,binding = 4) uniform UBOSSAOKernel
{
	vec4 samples[SSAO_KERNEL_SIZE];
} uboSSAOKernel;

layout (set = 0,binding = 5) uniform UBO 
{
	mat4 projection;
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out float outColor;

void main() 
{
	ivec2 noiseDim = textureSize(uSsaoNoise, 0);
	ivec2 texDim = textureSize(uViewPositionSampler, 0); 

	vec3 fragPos = texture(uViewPositionSampler, inUV).xyz;
	vec3 normal = normalize(texture(uViewNormalSampler, inUV).xyz);
	
	// Get a random vector using a noise lookup

	const vec2 noiseScale = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y));  
	
	// Create TBN matrix
 	vec3 randomVec = normalize(texture(uSsaoNoise, inUV * noiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

	// Calculate occlusion value
	float occlusion = 0.0f;
	// remove banding
	const float bias = 0.025;
	for(int i = 0; i < SSAO_KERNEL_SIZE; i++)
	{		
		vec3 samplePos = TBN * uboSSAOKernel.samples[i].xyz; 
		samplePos = fragPos + samplePos * SSAO_RADIUS; 
		
		// project
		vec4 offset = vec4(samplePos, 1.0f);
		offset = ubo.projection * offset; 
		offset.xyz /= offset.w; 
		offset.xyz = offset.xyz * 0.5f + 0.5f; 
		
		float sampleDepth = textureLod(uViewPositionSampler, offset.xy, 0).z;

		float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= (samplePos.z + bias) ? 1.0f : 0.0f) * rangeCheck;           
	}

	occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));

	outColor =  occlusion;//texture(uViewPositionSampler, inUV);;
}
