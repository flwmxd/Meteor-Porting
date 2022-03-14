#ifndef LPV_COMMON_GLSL
#define LPV_COMMON_GLSL

#define SH_C0 0.282094792f // 1 / 2sqrt(pi)
#define SH_C1 0.488602512f // sqrt(3/pi) / 2

#define SH_cosLobe_C0 0.886226925f // sqrt(pi)/2 
#define SH_cosLobe_C1 1.02332671f // sqrt(pi/3) 

#define PI 3.1415926535897932384626433832795

#define DEG_TO_RAD PI / 180.0f

#define SH_F_TO_I 1000
#define SH_I_TO_F 0.001


#define imgStoreAdd(img, pos, data) \
  imageAtomicAdd(img, ivec3(pos.x * 4 + 0, pos.y, pos.z ), uint(data.r * SH_F_TO_I)); \
  imageAtomicAdd(img, ivec3(pos.x * 4 + 1, pos.y, pos.z ), uint(data.g * SH_F_TO_I)); \
  imageAtomicAdd(img, ivec3(pos.x * 4 + 2, pos.y, pos.z ), uint(data.b * SH_F_TO_I)); \
  imageAtomicAdd(img, ivec3(pos.x * 4 + 3, pos.y, pos.z ), uint(data.a * SH_F_TO_I));


#define imgLoad(img, pos) \
  vec4( \
    imageLoad(img, ivec3(pos.x * 4 + 0, pos.y, pos.z)).r * SH_I_TO_F , \
    imageLoad(img, ivec3(pos.x * 4 + 1, pos.y, pos.z)).r * SH_I_TO_F , \
    imageLoad(img, ivec3(pos.x * 4 + 2, pos.y, pos.z)).r * SH_I_TO_F , \
    imageLoad(img, ivec3(pos.x * 4 + 3, pos.y, pos.z)).r * SH_I_TO_F )


#define texelFetch2(txt,pos) \
  vec4( \
    texelFetch(txt, ivec3(pos.x * 4 + 0, pos.y, pos.z ),0).r  * SH_I_TO_F, \
    texelFetch(txt, ivec3(pos.x * 4 + 1, pos.y, pos.z ),0).r  * SH_I_TO_F, \
    texelFetch(txt, ivec3(pos.x * 4 + 2, pos.y, pos.z ),0).r  * SH_I_TO_F, \
    texelFetch(txt, ivec3(pos.x * 4 + 3, pos.y, pos.z ),0).r  * SH_I_TO_F )


vec4 evalCosineLobeToDir(vec3 dir)
{
	return vec4( SH_cosLobe_C0, -SH_cosLobe_C1 * dir.y, SH_cosLobe_C1 * dir.z, -SH_cosLobe_C1 * dir.x );
}

// Get SH coeficients out of direction
vec4 dirToSH(vec3 dir)
{
    return vec4(SH_C0, -SH_C1 * dir.y, SH_C1 * dir.z, -SH_C1 * dir.x);
}

#endif // LPV_COMMON_GLSL