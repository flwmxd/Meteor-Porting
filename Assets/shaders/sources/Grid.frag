#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
//#extension GL_EXT_debug_printf : enable
const float Epsilon = 0.001;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform UniformBuffer
{
	vec4  cameraPos;
	float near;
	float far;
	float maxDistance;
	float p1;
}ubo;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 nearPoint;
layout(location = 3) in vec3 farPoint; 
layout(location = 4) in mat4 fragView;
layout(location = 8) in mat4 fragProj;

const float step = 100.0f;
const float subdivisions = 10.0f;

vec4 grid(vec3 fragPos3D, float scale, bool drawAxis) 
{
    vec2 coord = fragPos3D.xz * scale;
	
	vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.35, 0.35, 0.35, 1.0 - min(line, 1.0));
    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color = vec4(0.0, 0.0, 1.0, 1.0);
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color = vec4(1.0, 0.0, 0.0, 1.0);
    return color;
}
float computeDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}
float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos =  fragProj * fragView * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * ubo.near * ubo.far) / (ubo.far + ubo.near - clip_space_depth * (ubo.far - ubo.near)); // get linear value between 0.01 and 100
    return linearDepth / ubo.far; // normalize
}

int roundToPowerOfTen(float n)
{
    return int(pow(10.0, floor( (1 / log(10)) * log(n))));
}

void main()
{
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
	if (t < 0.)
        discard;
	
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);
	
    gl_FragDepth = computeDepth(fragPos3D);
	
    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0, (0.5 - linearDepth));
	
	float decreaseDistance = ubo.far * 1.5;
	vec3 pseudoViewPos = vec3(ubo.cameraPos.x, fragPos3D.y, ubo.cameraPos.z);
	
	  float dist, angleFade;
  /* if persp */
  if (fragProj[3][3] == 0.0) {
    vec3 viewvec = ubo.cameraPos.xyz - fragPos3D;
    dist = length(viewvec);
    viewvec /= dist;

    float angle;
      angle = viewvec.y;
    angle = 1.0 - abs(angle);
    angle *= angle;
		angleFade= 1.0 - angle * angle;
		angleFade*= 1.0 - smoothstep(0.0, ubo.maxDistance, dist - ubo.maxDistance);
  }
  else {
    dist = gl_FragCoord.z * 2.0 - 1.0;
    /* Avoid fading in +Z direction in camera view (see T70193). */
    dist = clamp(dist, 0.0, 1.0);// abs(dist);
		angleFade = 1.0 - smoothstep(0.0, 0.5, dist - 0.5);
    dist = 1.0; /* avoid branch after */

  }
	
	float distanceToCamera = abs(ubo.cameraPos.y - fragPos3D.y);
	int powerOfTen = roundToPowerOfTen(distanceToCamera);
	powerOfTen = max(1, powerOfTen);
	float divs = 1.0 / float(powerOfTen);
	float secondFade = smoothstep(subdivisions / divs, 1 / divs, distanceToCamera);
	
	vec4 grid1 = grid(fragPos3D, divs/subdivisions, true);
	vec4 grid2 = grid(fragPos3D, divs, true);
	
	grid2.a *= secondFade;
	grid1.a *= (1 - secondFade);

    outColor =  grid1 + grid2;  // adding multiple resolution for the grid
	outColor *= float(t > 0);
	outColor.a *= fading;
	outColor.a *= angleFade;
}