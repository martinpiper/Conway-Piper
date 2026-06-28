#version 430 core

layout (std430, binding = 2) buffer shader_data {
	int mapw;
	int maph;
	int mapd;

	vec3 palette[10];

	int data[];
};

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBloom;

in vec2 fragPos;

uniform vec2 u_Resolution;

uniform float u_Time;

uniform mat4 u_InverseProjection;
uniform mat4 u_InverseView;

uniform bool useFresnel;

uniform int u_SPP;
uniform int u_Bounces;

uniform int u_FrameSinceLastReset;

uniform sampler2D u_LastColors;
uniform sampler2D u_LastBloom;

float tseed = 0;
uint rngState = uint(uint(gl_FragCoord.x) * uint(19873) + uint(gl_FragCoord.y) * uint(92787) + uint(tseed * 100) * uint(26699) + u_FrameSinceLastReset) | uint(1);

uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}
float RandomFloat01(inout uint state) {
    return float(wang_hash(state)) / 4294967296.0;
}
vec3 RandomUnitVector(inout uint state) {
	float z = 1.0 - 2.0 * RandomFloat01(state);
	float r = sqrt(1.0 - z * z);
	float phi = 6.28318530718 * RandomFloat01(state);
	float x = r * cos(phi);
	float y = r * sin(phi);
	return vec3(x, y, z);
}

// --------------- Structs ---------------

struct Material {
	vec3 diffuse;
	vec3 specular;
	vec3 emissive;
	float smoothness;
	float specularChance;
};

struct Sphere {
	vec3 pos;
	float radius;
	Material material;
};

struct Plane {
	vec3 pos;
	vec3 normal;
	Material material;
};

struct intersection {
	float t;
	vec3 pos;
	vec3 normal;
	Material material;
	bool hit;
};


// --------------- Scene ---------------


const int numSpheres = 4;

Sphere spheres[numSpheres] = Sphere[] (
	Sphere(vec3(20, 30, 40), 1, Material(vec3(0.95, 0.5, 0.95), vec3(1.0, 0.80, 0.80), vec3(0), 1.0, 0.9)),
	Sphere(vec3(8, 8, 8), 1, Material(vec3(0.5, 0.95, 0.95), vec3(0.80, 1.0, 0.80), vec3(0), 0.9, 0.9)),
	Sphere(vec3(16, 16, 16), 1, Material(vec3(0.95, 0.95, 0.95), vec3(1, 1, 1), vec3(5), 0, 0)),
	Sphere(vec3(0, 0, 0), 1, Material(vec3(0.95, 0.95, 0.95), vec3(0.50, 0.50, 0.95), vec3(0), 0.9, 0.9))
);

const int numPlanes = 1;

Plane planes[numPlanes] = Plane[] (
	Plane(vec3(0, -1, 0), vec3(0, 1, 0), Material(vec3(1.0, 1.0, 1.0), vec3(0), vec3(0), 0.2, 0.5))
);

// --------------- Intersections ---------------

float sphereIntersect(Sphere sphere, vec3 pos, vec3 dir) {
	vec3 oc = pos - sphere.pos;
	float b = dot(oc, dir);
	float c = dot(oc, oc) - sphere.radius * sphere.radius;
	float h = b * b - c;
	if (h < 0) return -1;
	return -b - sqrt(h);
}

float planeIntersect(vec3 pos, vec3 dir, vec3 planeNormal, vec3 planePos) {
	float denom = dot(dir, -planeNormal);
	if (denom > 1e-6) {
		vec3 p0l0 = planePos - pos;
		float t = dot(p0l0, -planeNormal) / denom;
		if (t >= 0) return t;
	}
	return -1;
}

uint testVoxel(int x, int y, int z) {
	if(x < 0 || x >= mapw || y < 0 || y >= maph || z < 0 || z >= mapd) return 0;
	return data[x + y * mapw + z * mapw * maph];
}

float projectToCube(vec3 ro, vec3 rd) {
	
	float tx1 = (0 - ro.x) / rd.x;
	float tx2 = (mapw - ro.x) / rd.x;

	float ty1 = (0 - ro.y) / rd.y;
	float ty2 = (maph - ro.y) / rd.y;

	float tz1 = (0 - ro.z) / rd.z;
	float tz2 = (mapd - ro.z) / rd.z;

	float tx = max(min(tx1, tx2), 0);
	float ty = max(min(ty1, ty2), 0);
	float tz = max(min(tz1, tz2), 0);

	float t = max(tx, max(ty, tz));
	
	return t;
}

float voxel_traversal(vec3 orig, vec3 direction, inout vec3 normal, inout uint blockType) {
	vec3 origin = orig;
	
	float t1 = max(projectToCube(origin, direction) - 0.001, 0);
	origin += t1 * direction;

	int mapX = int(floor(origin.x));
	int mapY = int(floor(origin.y));
	int mapZ = int(floor(origin.z));

	float sideDistX;
	float sideDistY;
	float sideDistZ;

	float deltaDX = abs(1 / direction.x);
	float deltaDY = abs(1 / direction.y);
	float deltaDZ = abs(1 / direction.z);
	float perpWallDist = -1;

	int stepX;
	int stepY;
	int stepZ;

	int hit = 0;
	int side;

	if (direction.x < 0) {
		stepX = -1;
		sideDistX = (origin.x - mapX) * deltaDX;
	} else {
		stepX = 1;
		sideDistX = (mapX + 1.0 - origin.x) * deltaDX;
	}
	if (direction.y < 0) {
		stepY = -1;
		sideDistY = (origin.y - mapY) * deltaDY;
	} else {
		stepY = 1;
		sideDistY = (mapY + 1.0 - origin.y) * deltaDY;
	}
	if (direction.z < 0) {
		stepZ = -1;
		sideDistZ = (origin.z - mapZ) * deltaDZ;
	} else {
		stepZ = 1;
		sideDistZ = (mapZ + 1.0 - origin.z) * deltaDZ;
	}

	int step = 1;

	for (int i = 0; i < 6000; i++) {
		if ((mapX >= mapw && stepX > 0) || (mapY >= maph && stepY > 0) || (mapZ >= mapd && stepZ > 0)) break;
		if ((mapX < 0 && stepX < 0) || (mapY < 0 && stepY < 0) || (mapZ < 0 && stepZ < 0)) break;

		if (sideDistX < sideDistY && sideDistX < sideDistZ) {
			sideDistX += deltaDX;
			mapX += stepX * step;
			side = 0;
		} else if(sideDistY < sideDistX && sideDistY < sideDistZ){
			sideDistY += deltaDY;
			mapY += stepY * step;
			side = 1;
		} else {
			sideDistZ += deltaDZ;
			mapZ += stepZ * step;
			side = 2;
		}

		uint block = testVoxel(mapX, mapY, mapZ);
		if (block != 0u) {
			blockType = block;
			hit = 1;

			if (side == 0) {
				perpWallDist = (mapX - origin.x + (1 - stepX * step) / 2) / direction.x + t1;
				normal = vec3(1, 0, 0) * -stepX;
			}
			else if (side == 1) {
				perpWallDist = (mapY - origin.y + (1 - stepY * step) / 2) / direction.y + t1;
				normal = vec3(0, 1, 0) * -stepY;
			}
			else {
				perpWallDist = (mapZ - origin.z + (1 - stepZ * step) / 2) / direction.z + t1;
				normal = vec3(0, 0, 1) * -stepZ;
			}
			break;
		}
	}

	return perpWallDist;
}


void sceneIntersect(vec3 pos, vec3 dir, out intersection closest) {
	closest.t = 1000000;
	closest.hit = false;
	for (int i = 0; i < numSpheres; i++) {
		float t = sphereIntersect(spheres[i], pos, dir);
		if (t > 0 && t < closest.t) {
			closest.t = t;
			closest.pos = pos + dir * t;
			closest.normal = -normalize(spheres[i].pos - closest.pos);
			closest.material = spheres[i].material;
			closest.hit = true;
		}
	}
	for (int i = 0; i < numPlanes; i++) {
		float t = planeIntersect(pos, dir, planes[i].normal, planes[i].pos);
		if (t > 0 && t < closest.t) {
			closest.t = t;
			closest.pos = pos + dir * t;
			closest.normal = planes[i].normal;
			closest.material = planes[i].material;
			closest.hit = true;
		}
	}
	
	vec3 normal;
	uint blockType;
	float t = voxel_traversal(pos, dir, normal, blockType);

	if (t > 0 && t < closest.t) {
		closest.t = t;
		closest.pos = pos + dir * t;
		closest.normal = normal;
		closest.material = Material(palette[blockType], vec3(1), blockType==1 ? vec3(2) : vec3(0), 0, 0);
		closest.hit = true;
	}
}

float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

vec3 lerp(vec3 a, vec3 b, float t) {
	return a + (b - a) * t;
}

struct environment {
	vec3 SkyColorZenith;
	vec3 SkyColorHorizon;
	vec3 GroundColor;

	vec3 SunColor;
	vec3 SunDirection;
	float SunFocus;
	float SunIntensity;
};

environment env = environment(
	vec3(0.5, 0.7, 0.9) * 0.5, // SkyColorZenith
	vec3(1.0, 0.8, 0.6) * 0.5, // SkyColorHorizon
	vec3(0.7, 0.6, 0.4) * 0.5, // GroundColor
	vec3(1, 1, 1),
	normalize(vec3(0.5, 0.5, 0.5)),
	500,
	50

);

float FresnelReflectAmount(float n1, float n2, vec3 normal, vec3 incident, float f0, float f90) {
        // Schlick aproximation
        float r0 = (n1-n2) / (n1+n2);
        r0 *= r0;
        float cosX = -dot(normal, incident);
        if (n1 > n2) {
            float n = n1/n2;
            float sinT2 = n*n*(1.0-cosX*cosX);
            // Total internal reflection
            if (sinT2 > 1.0)
                return f90;
            cosX = sqrt(1.0-sinT2);
        }
        float x = 1.0-cosX;
        float ret = r0+(1.0-r0)*x*x*x*x*x;
 
        // adjust reflect multiplier for object reflectivity
        return mix(f0, f90, ret);
}


// simple background environment lighting with sun
vec3 skycolor(vec3 dir) {

	float skyGradientT = pow(smoothstep(0., 0.4, dir.y), 0.35);
	vec3 skyGradient = lerp(env.SkyColorHorizon, env.SkyColorZenith, skyGradientT);
	float sun = pow(max(0, dot(dir, env.SunDirection)), env.SunFocus) * env.SunIntensity;

	float groundToSkyT = smoothstep(-0.01, 0., dir.y);
	float sunMask = groundToSkyT >= 1 ? 1 : 0;

	return lerp(env.GroundColor, skyGradient, groundToSkyT) + sun * env.SunColor * sunMask;
}

float compMax(vec3 color) {
    return max(max(color.x, color.y), color.z);
}

void main() {
	vec3 finalColor = vec3(0);

	for(int spp = 0; spp < u_SPP; spp++) {
		vec2 ScreenSpace = (gl_FragCoord.xy + vec2(RandomFloat01(rngState), RandomFloat01(rngState))) / u_Resolution.xy;
		vec4 Clip = vec4(ScreenSpace.xy * 2.0f - 1.0f, -1.0, 1.0);
		vec4 Eye = vec4(vec2(u_InverseProjection * Clip), -1.0, 0.0);

		vec3 RayOrigin = u_InverseView[3].xyz;
		vec3 RayDirection = vec3(u_InverseView * Eye);
		RayDirection = normalize(RayDirection);
	
		vec3 rayColor = vec3(1);
		vec3 incomingLight = vec3(0);

		for (int i = 0; i < u_Bounces; i++) {
			intersection closest;

			sceneIntersect(RayOrigin, RayDirection, closest);
			if (closest.hit) {
				RayOrigin = closest.pos;
				vec3 diffuseDir = normalize(closest.normal + RandomUnitVector(rngState));
				vec3 specularDir = reflect(RayDirection, closest.normal);
				bool ifSpecular = RandomFloat01(rngState) < (useFresnel ? FresnelReflectAmount(1, 1.5, RayDirection, closest.normal, closest.material.specularChance, 1) : closest.material.specularChance);

				RayDirection = lerp(diffuseDir, specularDir, ifSpecular ? closest.material.smoothness : 0) + 0.001 * closest.normal;
				incomingLight += rayColor * closest.material.emissive;
				rayColor *= lerp(closest.material.diffuse, closest.material.specular, ifSpecular ? closest.material.smoothness : 0);
			} else {
				incomingLight += rayColor * skycolor(RayDirection);
				break;
			}
		}
		
		finalColor += incomingLight / float(u_SPP);
	}
	
	outColor = vec4(finalColor, 1);

	int bloomSamples = u_SPP * 2;

	vec3 bloom = vec3(0.0);

	for(int i = 0; i < bloomSamples; i++) {
		// select offset based on gaussian distribution
		float u = RandomFloat01(rngState);
		float v = RandomFloat01(rngState);
		float r = sqrt(-2.0 * log(u)) * 0.1;
		float theta = 2.0 * 3.1415926535897932384626433832795 * v;
		vec2 offset = vec2(r * cos(theta), r * sin(theta));

		vec3 sampleColor = texture(u_LastColors, fragPos * 0.5 + 0.5 + offset * 0.5).rgb;
		if(compMax(sampleColor) > 1.5) bloom += sampleColor;
	}

	outBloom.xyz = bloom / float(bloomSamples);


	if(u_FrameSinceLastReset > 0) {
		vec4 lastColor =  texture(u_LastColors, (gl_FragCoord.xy + 0.5) / u_Resolution.xy);
		outColor = (outColor + lastColor * u_FrameSinceLastReset) / (u_FrameSinceLastReset + 1);

		// Add to the bloom buffer

		vec3 lastBloom = texture(u_LastBloom, (gl_FragCoord.xy + 0.5) / u_Resolution.xy).rgb;
		outBloom = vec4((outBloom.xyz + lastBloom * u_FrameSinceLastReset) / (u_FrameSinceLastReset + 1), 1);
	}
}
