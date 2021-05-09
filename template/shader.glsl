#version 330

uniform int     gTimelinePoint;
uniform float   gPointTime;
uniform float   gTime;
uniform vec2    gRes;


#define RAY_MAX_LENGTH 100.0
#define NUM_MAX_STEPS  100
#define MIN_DISTANCE   0.01



float sphereSDF(vec3 p, vec3 pos, float size) {
	return length(p - pos) - size;
}

mat3 rotxy(vec2 a) {
	vec2 c = cos(a);
	vec2 s = sin(a);
	return mat3(c.y,  0.0, -s.y, s.y * s.x, c.x, c.y * s.x, s.y * c.x, -s.x, c.y * c.x);
}

float distort(vec3 p, float a, float v) {
	return sin(a * p.x) * cos(a * p.y) * sin(a * p.z) * v;
}

vec3 repeat(vec3 p, vec3 f) {
	return mod(p, f) - 0.5 * f;
}

vec2 repeat_xz(vec3 p, vec2 f) {
	vec2 r = mod(p.xz, f) - 0.5 * f;
	return vec3(r.x, p.y, r.y);
}




float create_world(vec3 p) {
	float sphere = sphereSDF(p, vec3(0.0, 0.0, 5.0), 1.0);

	return sphere;
}



vec3 compute_normal(vec3 p) {
	float dist = create_world(p);
	vec2 s = vec2(0.01, 0.0);
	vec3 n = vec3(
				create_world(p - s.xyy),
				create_world(p - s.yxy),
				create_world(p - s.yyx)
			);

	return normalize(dist - n);
}

vec3 compute_light(vec3 p) {

	vec3 diffuse_color = vec3(0.8, 0.8, 0.8);
	vec3 ambient_color = vec3(0.03, 0.03, 0.03);


	vec3 light_pos = vec3(2.0, 3.0, 1.0);
	vec3 v = normalize(light_pos - p);
	vec3 norm = compute_normal(p);

	vec3 diffuse = diffuse_color * max(dot(v, norm), 0.0);
	return diffuse + ambient_color;
}



void ray_march(vec3 ro, vec3 rd) {
	
	float ray_length = 0.0;
	vec3 color = vec3(0.0, 0.0, 0.0);

	for(int i = 0; i < NUM_MAX_STEPS; i++) {
		vec3 p = ro + rd * ray_length;
		float dist = create_world(p);
		ray_length += dist;
		if(dist <= MIN_DISTANCE) {
			color = compute_light(p);
			break;
		}
		else if(ray_length >= RAY_MAX_LENGTH) {
			break;
		}

	}


	if(gTimelinePoint == 0) {
		color *= min(1.0, gPointTime*0.3);
	}



	gl_FragColor = vec4(color, 1.0);
}


vec3 get_raydir(float fov) {
	vec2 xy = gl_FragCoord.xy - gRes.xy * 0.5;

	if(gTimelinePoint == 0) {
	}


	float hf = tan((90.0 - fov * 0.5) * (3.14159/180.0));
	return normalize(vec3(xy, (gRes.y * 0.5 * hf)));
}



void main() {

	vec3 ro = vec3(0.0, 1.0, -2.0);
	vec3 rd = get_raydir(60.0);
	//rd *= rotxy(vec2(0.0, cos(gTime)*0.5));


	ray_march(ro, rd);
}






