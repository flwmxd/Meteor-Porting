#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inUV;


layout(set = 0,binding = 0) uniform UniformBufferObject
{
	mat4 invProj;
	mat4 invView;
    vec4 skyColorBottom;
    vec4 skyColorTop;
    vec4 lightDirection;
    vec4 viewPos;
} ubo;

layout(set = 0, binding = 1)  uniform sampler2D uPositionSampler;

void raySphereintersectionSkyMap(vec3 rd, float radius, out vec3 startPos)
{
	
	//f(x) = a(x^2) + bx + c
	float t;

	vec3 sphereCenter_ = vec3(0.0);

	float radius2 = radius*radius;

	vec3 L = - sphereCenter_;
	float a = dot(rd, rd);
	float b = 2.0 * dot(rd, L);
	float c = dot(L,L) - radius2;

	float discr = b*b - 4.0 * a * c;
	t = max(0.0, (-b + sqrt(discr))/2);

	startPos = rd*t;
}

vec3 getSun(const vec3 d, float powExp)
{
	float sun = clamp( dot(ubo.lightDirection.xyz,d), 0.0, 1.0 );
	vec3 col = 0.8*vec3(1.0,.6,0.1)*pow( sun, powExp );
	return col;
}

vec4 colorCubeMap(vec3 endPos, const vec3 d)
{
    // background sky     
	vec3 col = mix(ubo.skyColorBottom.rgb, ubo.skyColorTop.rgb, clamp(1 - exp(8.5-17.*clamp(normalize(d).y*0.5 + 0.5,0.0,1.0)),0.0,1.0));
	
	col += getSun(d, 350.0);

	return vec4(col, 1.0);
}

vec3 computeClipSpaceCoord(ivec2 fragCoord,vec2 resolution){
	vec2 ray_nds = 2.0*vec2(fragCoord.xy)/resolution.xy - 1.0;
	return vec3(ray_nds, 1.0);
}

void main()
{    
    //vec3 worldPos = texture(uPositionSampler,inUV).xyz;
    //vec3 worldDir = normalize(worldPos - ubo.viewPos);
   	vec2 textureResolution = textureSize(uPositionSampler,0);
	ivec2 fragCoord = ivec2(gl_FragCoord.xy);

	vec4 ray_clip = vec4(computeClipSpaceCoord(fragCoord,textureResolution), 1.0);
	
	//vec4 ray_clip = vec4(inUV,1.0, 1.0);
	vec4 ray_view = ubo.invProj * ray_clip;
	ray_view = vec4(ray_view.xy, -1.0, 0.0);
	vec3 worldDir = (ubo.invView * ray_view).xyz;
	worldDir = normalize(worldDir);

	vec3 startPos, endPos;
	vec4 v = vec4(0.0);

	vec3 cubeMapEndPos;

	raySphereintersectionSkyMap(worldDir, 0.5, cubeMapEndPos);

	vec4 bg = colorCubeMap(cubeMapEndPos, worldDir);
	vec2 red = vec2(1.0);

	outColor = vec4(bg.rgb,1.0);
}