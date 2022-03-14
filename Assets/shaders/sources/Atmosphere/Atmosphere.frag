#version 450

#define PI 3.1415926535897932384626433832795

layout(location = 0) in vec2 inUV;// Position of the fragment

layout(location = 0) out vec4 finalColor;


layout(set = 0, binding = 0) uniform UniformBuffer
{
    mat4 invProj;

    vec4 rayleighScattering;// rayleigh + surfaceRadius
    vec4 mieScattering;// mieScattering + atmosphereRadius
    vec4 sunDirection;//sunDirection + sunIntensity
    vec4 centerPoint;
} ubo;





// Global variables, needed for size
vec2 totalDepthRM;
vec3 I_R, I_M;

// Calculate densities $\rho$.
// Returns vec2(rho_rayleigh, rho_mie)
// Note that intro version is more complicated and adds clouds by abusing Mie scattering density. That's why it's a separate function
vec2 densitiesRM(vec3 p) {
	float h = max(0., length(p - ubo.centerPoint.xyz) - ubo.rayleighScattering.w); // calculate height from Earth surface
	return vec2(exp(-h / 8e3), exp(-h / 12e2));
}

// Basically a ray-sphere intersection. Find distance to where rays escapes a sphere with given radius.
// Used to calculate length at which ray escapes atmosphere
float escape(vec3 p, vec3 d, float R) {
    // f(x) = a(x^2) + bx + c
	vec3 v = p - ubo.centerPoint.xyz;
	float b = dot(v, d);
	float det = b * b - dot(v, v) + R * R;
	if (det < 0.) return -1.;
	det = sqrt(det);

	float t1 = -b - det, t2 = -b + det;

	return (t2 >= 0.) ? t2 : t1;
}

// Calculate density integral for optical depth for ray starting at point `p` in direction `d` for length `L`
// Perform `steps` steps of integration
// Returns vec2(depth_int_rayleigh, depth_int_mie)
vec2 scatterDepthInt(vec3 o, vec3 d, float L, float steps) {
	// Accumulator
	vec2 depthRMs = vec2(0.);

	// Set L to be step distance and pre-multiply d with it
	L /= steps; 
    d *= L;
	// Go from point P to A
	for (float i = 0.; i < steps; ++i)
		// Simply accumulate densities
		depthRMs += densitiesRM(o + d * i);

	return depthRMs * L;
}




// Calculate in-scattering for ray starting at point `o` in direction `d` for length `L`
// Perform `steps` steps of integration
void scatterIn(vec3 o, vec3 d, float L, float steps) {

    vec3 bMe = ubo.mieScattering.xyz * 1.1;
	// Set L to be step distance and pre-multiply d with it
	L /= steps; d *= L;

	// Go from point O to B
	for (float i = 0.; i < steps; ++i) {

		// Calculate position of point P_i
		vec3 p = o + d * i;

		// Calculate densities
		vec2 dRM = densitiesRM(p) * L;

		// Accumulate T(P_i -> O) with the new P_i
		totalDepthRM += dRM;

		// Calculate sum of optical depths. totalDepthRM is T(P_i -> O)
		// scatterDepthInt calculates integral part for T(A -> P_i)
		// So depthRMSum becomes sum of both optical depths
        // atmosphereRadius
		vec2 depthRMsum = totalDepthRM + scatterDepthInt(p, ubo.sunDirection.xyz, escape(p, ubo.sunDirection.xyz, ubo.mieScattering.w), 4.);

		// Calculate e^(T(A -> P_i) + T(P_i -> O)
		vec3 A = exp(-ubo.rayleighScattering.xyz * depthRMsum.x - bMe * depthRMsum.y);

		// Accumulate I_R and I_M
		I_R += A * dRM.x;
		I_M += A * dRM.y;
	}
}

// Final scattering function
// O = o -- starting point
// B = o + d * L -- end point
// Lo -- end point color to calculate extinction for
vec3 scatter(vec3 o, vec3 d, float L, vec3 Lo) {

    vec3 bMe = ubo.mieScattering.xyz * 1.1;
	// Zero T(P -> O) accumulator
	totalDepthRM = vec2(0.);
	
	// Zero I_M and I_R
	I_R = I_M = vec3(0.);
	
	// Compute T(P -> O) and I_M and I_R
	scatterIn(o, d, L, 32.);
	
	// mu = cos(alpha)
	float mu = dot(d, ubo.sunDirection.xyz);
	float mumu = mu * mu;

    const float g = ubo.centerPoint.w;
    const float g2 = g * g;

    //PhaseFunction Fr(sita) = 
    /**
    *             3
    *Fr(sita) = ----(1 + cos^2(sita))
    *           16Pi   
    */
    const float phaseR =  3.0/ (16.0 * PI) * (1 + mumu);

    /**
    *                   1 - g^2
    * Fr(sita) = ------------------------
    *            4Pi(1 + g^2 - 2g * cos) ^ 1.5   
    */
    const float phaseM =  (1 - g2) / ( 4 * PI * pow( 1 + g2 - 2 * g * mu,1.5) );


    const float phase = phaseR + phaseM;

	// Calculate Lo extinction
	return Lo * exp(-ubo.rayleighScattering.xyz * totalDepthRM.x - bMe * totalDepthRM.y)
	
		// Add in-scattering
        //sunIntensity
		+ ubo.sunDirection.w  * (
                I_R * ubo.rayleighScattering.xyz * phaseR +
                I_M * ubo.mieScattering.xyz * phaseM
            );
}


void main()
{
	// Might add camera position in here.
	// It was already in there, I removed it.
	vec3 O = vec3(0., 0., 0.);
	vec2 NDC = inUV * 2.0 - 1;
	vec4 camSpace = ubo.invProj * vec4(vec3(NDC, 1.0), 1.0);
	vec3 direction = normalize(camSpace.xyz);
	vec3 D = direction;

	vec3 col = vec3(0.0);
    //atmosphereRadius 
	float L = escape(O, D, ubo.mieScattering.w);
	col = scatter(O, D, L, col);
	finalColor = vec4(col, 1.);
}