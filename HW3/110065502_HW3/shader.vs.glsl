#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec2 texCoord;

uniform mat4 um4p;	
uniform mat4 um4v;
uniform mat4 um4m;

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
uniform LightData light[3];
uniform MaterialData material;

out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 vertex_view;

vec4 lightInView;
vec3 L, H;
float dc, sc;

// [TODO] passing uniform variable for texture coordinate offset
uniform vec2 tex_offset;

void main() 
{
	// [TODO]
	vec4 vertexInView = um4v * um4m * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	vec4 normalInView = transpose(inverse(um4v * um4m)) * vec4(aNormal, 0.0);

	vertex_view = vertexInView.xyz;
	vertex_normal = normalInView.xyz;

	vec3 N = normalize(vertex_normal);
	vec3 V = -vertex_view;

	// Handle lighting mode type
	if(light_type == 0){
		// directional light
		lightInView = um4v * vec4(light[0].position, 1.0f);	
		L = normalize(lightInView.xyz + V);			
		H = normalize(L + V);						

		vec3 ambient = light[0].Ambient * material.Ka;
		vec3 diffuse = light[0].Diffuse * max(dot(L,N), 0.0) *  material.Kd;
		vec3 specular = light[0].Specular * pow(max(dot(H, N), 0.0), material.shininess) * material.Ks;

		vertex_color = ambient + diffuse  + specular;
	}
	else if(light_type == 1){
		// point light
		lightInView = um4v * vec4(light[1].position,1.0f);	
		L = normalize(lightInView.xyz + V);			
		H = normalize(L + V);						

		float dis = length(lightInView.xyz + V);
		float f = 1/ (light[1].constantAttenuation + light[1].linearAttenuation*dis + pow(dis, 2)*light[1].quadraticAttenuation);

		vec3 ambient = light[1].Ambient * material.Ka;
		vec3 diffuse = light[1].Diffuse * max(dot(L,N), 0.0) *  material.Kd;
		vec3 specular = light[1].Specular * pow(max(dot(H, N), 0.0), material.shininess) * material.Ks;

		vertex_color = f * (ambient + diffuse  + specular);
	}
	else if(light_type == 2){
		//spot light
		lightInView = um4v * vec4(light[2].position,1.0f);	
		L = normalize(lightInView.xyz + V);			
		H = normalize(L + V);

		float spot = dot(-L, normalize(light[2].spotDirection.xyz));
		float dis = length(lightInView.xyz + V);
		float f = 1/ (light[2].constantAttenuation + light[2].linearAttenuation*dis + pow(dis, 2)*light[2].quadraticAttenuation);

		vec3 ambient = light[2].Ambient * material.Ka;
		vec3 diffuse = light[2].Diffuse * max(dot(L,N), 0.0) *  material.Kd;
		vec3 specular = light[2].Specular * pow(max(dot(H, N), 0.0), material.shininess) * material.Ks;

		if(spot < light[2].spotCutoff)
			vertex_color = ambient + f * 0 * (diffuse + specular);
		else
			vertex_color = ambient + f * pow(max(spot, 0), light[2].spotExponent) * (diffuse + specular);
	}

	texCoord = aTexCoord + tex_offset;
	gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0);
}
