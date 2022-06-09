#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in Vertex
{
	vec2 fragTexCoord;
	vec3 fragNormal;
} vertexIn[3];

layout(location = 0) out GeometryOut
{
	vec3 wsPosition;
    vec3 position;
    vec3 normal;
    vec2 texCoord;
    flat vec4 triangleAABB;
} Out;


layout(set = 3, binding = 0) uniform UniformBufferGemo
{
	mat4 viewProjections[3];
	mat4 viewProjectionsI[3];
	
	vec3 worldMinPoint;
	float voxelScale;

	float volumeDimension;
	float padding1;
	float padding2;
	float padding3;
} ubo;


int calculateAxis(vec3 faceNormal)
{

	float nDX = abs(faceNormal.x);
	float nDY = abs(faceNormal.y);
	float nDZ = abs(faceNormal.z);

	if( nDX > nDY && nDX > nDZ )
    {
		return 0; //x
	}
	
	else if( nDY > nDX && nDY > nDZ  )
    {
	    return 1; //y
    }
	return 2;//z
}

vec4 calculateAABB(vec4 pos[3], vec2 pixelDiagonal)
{
	vec4 aabb;

	aabb.xy = min(pos[2].xy, min(pos[1].xy, pos[0].xy));
	aabb.zw = max(pos[2].xy, max(pos[1].xy, pos[0].xy));

	// enlarge by half-pixel
	aabb.xy -= pixelDiagonal;
	aabb.zw += pixelDiagonal;

	return aabb;
}

void main()
{
	//project to the plane which you can see
	/**
	 ______
	/	  /|
	------ |	
    |	 | |
	|____|/

 	*/

	vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 faceNormal = cross(p1, p2);
	int selectedIndex = calculateAxis(faceNormal);

	mat4 viewProjection = ubo.viewProjections[selectedIndex];
	mat4 viewProjectionI = ubo.viewProjectionsI[selectedIndex];

	vec2 texCoord[3];
	vec4 pos[3];

	for(int i = 0;i<3;i++)
	{
		texCoord[i] = vertexIn[i].fragTexCoord;
		pos[i] = viewProjection * gl_in[i].gl_Position;
	}

	vec4 trianglePlane;
	trianglePlane.xyz = normalize(cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz)); // the normal after projection
	trianglePlane.w = -dot(pos[0].xyz, trianglePlane.xyz);

    // change winding, otherwise there are artifacts for the back faces.
    if (dot(trianglePlane.xyz, vec3(0.0, 0.0, 1.0)) < 0.0)
    {
        vec4 vertexTemp = pos[2];
        vec2 texCoordTemp = texCoord[2];
        //swap 
        pos[2] = pos[1];
        texCoord[2] = texCoord[1];
    
        pos[1] = vertexTemp;
        texCoord[1] = texCoordTemp;
    }

	vec2 halfPixel = vec2(1.0f / ubo.volumeDimension); // default volumeDimension is 256.f;

	if(trianglePlane.z == 0.0f) return;
	// expanded aabb for triangle
	Out.triangleAABB = calculateAABB(pos, halfPixel);
	// calculate the plane through each edge of the triangle
	// in normal form for dilatation of the triangle
	vec3 planes[3] = vec3[3](
		cross(pos[0].xyw - pos[2].xyw, pos[2].xyw),
		cross(pos[1].xyw - pos[0].xyw, pos[0].xyw),
		cross(pos[2].xyw - pos[1].xyw, pos[1].xyw)
	);

	for(int i = 0;i<3;i++)
	{
		planes[i].z -= dot(halfPixel, abs(planes[i].xy));
	}
	// calculate intersection between translated planes
	vec3 intersection[3];
	intersection[0] = cross(planes[0], planes[1]);
	intersection[1] = cross(planes[1], planes[2]);
	intersection[2] = cross(planes[2], planes[0]);
	for(int i = 0;i<3;i++)
	{
		intersection[i] /= intersection[i].z;
	}

	// calculate dilated triangle vertices
	for(int i = 0;i<3;i++)
	{
		float z = -(intersection[i].x * trianglePlane.x + intersection[i].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
		pos[i].xyz = vec3(intersection[i].xy, z);
	}

	for(int i = 0; i < 3; ++i)
	{
		vec4 voxelPos = viewProjectionI * pos[i];
		voxelPos.xyz /= voxelPos.w;
		voxelPos.xyz -= ubo.worldMinPoint;
		voxelPos *= ubo.voxelScale;

		gl_Position = pos[i];
		Out.position = pos[i].xyz;
		Out.normal = vertexIn[i].fragNormal;
		Out.texCoord = texCoord[i];
		Out.wsPosition = voxelPos.xyz * ubo.volumeDimension;

		EmitVertex();
	}

	EndPrimitive();
}