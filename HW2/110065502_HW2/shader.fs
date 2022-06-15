#version 330 core

out vec4 FragColor;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_view;

struct MaterialData{
	vec3 Ka;
	vec3 Kd;
	vec3 Ks;
	float shininess;
};

struct LightData{
	vec3 position;
	vec3 spotDirection;
	vec3 Ambient;		
	vec3 Diffuse;			
	vec3 Specular;		
	float spotExponent;
	float spotCutoff;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};

uniform int light_type;		
uniform mat4 view_matrix;		
uniform LightData light[3];
uniform MaterialData material;
uniform int goraud_or_phong;

vec4 lightInView;
vec3 L, H;
float dc, sc;

void main() {
	// [TODO]
	vec3 N = normalize(vertex_normal);	
	vec3 V = -vertex_view;
	vec3 frag_color;

	// Handle lighting mode type
	if(light_type == 0){
		// directional light
		lightInView = view_matrix * vec4(light[0].position, 1.0f);	
		L = normalize(lightInView.xyz + V);			
		H = normalize(L + V);						

		vec3 ambient = light[0].Ambient * material.Ka;
		vec3 diffuse = light[0].Diffuse * max(dot(L,N), 0.0) *  material.Kd;
		vec3 specular = light[0].Specular * pow(max(dot(H, N), 0.0), material.shininess) * material.Ks;

		frag_color = ambient + diffuse  + specular;
	}
	else if(light_type == 1){
		// point light
		lightInView = view_matrix * vec4(light[1].position,1.0f);	
		L = normalize(lightInView.xyz + V);			
		H = normalize(L + V);						

		float dis = length(lightInView.xyz + V);
		float f = 1/ (light[1].constantAttenuation + light[1].linearAttenuation*dis + pow(dis, 2)*light[1].quadraticAttenuation);

		vec3 ambient = light[1].Ambient * material.Ka;
		vec3 diffuse = light[1].Diffuse * max(dot(L,N), 0.0) *  material.Kd;
		vec3 specular = light[1].Specular * pow(max(dot(H, N), 0.0), material.shininess) * material.Ks;

		frag_color = f * (ambient + diffuse  + specular);
	}
	else if(light_type == 2){
		//spot light
		lightInView = view_matrix * vec4(light[2].position,1.0f);	
		L = normalize(lightInView.xyz + V);			
		H = normalize(L + V);

		float spot = dot(-L, normalize(light[2].spotDirection.xyz));
		float dis = length(lightInView.xyz + V);
		float f = 1/ (light[2].constantAttenuation + light[2].linearAttenuation*dis + pow(dis, 2)*light[2].quadraticAttenuation);

		vec3 ambient = light[2].Ambient * material.Ka;
		vec3 diffuse = light[2].Diffuse * max(dot(L,N), 0.0) *  material.Kd;
		vec3 specular = light[2].Specular * pow(max(dot(H, N), 0.0), material.shininess) * material.Ks;

		if(spot < light[2].spotCutoff)
			frag_color = ambient + f * 0 * (diffuse + specular);
		else
			frag_color = ambient + f * pow(max(spot, 0), light[2].spotExponent) * (diffuse + specular);
	}

	if(goraud_or_phong == 0)
		FragColor = vec4(vertex_color, 1.0f);
	else
		FragColor = vec4(frag_color, 1.0f);
}
