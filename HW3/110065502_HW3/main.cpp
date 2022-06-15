#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>


#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

#define PI 3.14159
#define deg2rad(x) ((x)*((3.1415926f)/(180.0f)))

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
// current window size
int screenWidth = WINDOW_WIDTH, screenHeight = WINDOW_HEIGHT;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	ViewCenter = 3,
	ViewEye = 4,
	ViewUp = 5,
	LightEdit = 6,
	ShininessEdit = 7,
};


vector<string> filenames; // .obj filename list

typedef struct _Offset {
	GLfloat x;
	GLfloat y;
	struct _Offset(GLfloat _x, GLfloat _y) {
		x = _x;
		y = _y;
	};
} Offset;

typedef struct
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;

	GLuint diffuseTexture;

	// eye texture coordinate 
	GLuint isEye;
	vector<Offset> offsets;

	GLfloat shininess; // [NOTE] add shininess

} PhongMaterial;

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	GLuint p_texCoord;
	PhongMaterial material;
	int indexCount;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;

	bool hasEye;
	GLint max_eye_offset = 7;
	GLint cur_eye_offset_idx = 0;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
	Orthogonal = 0,
	Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

Shape m_shpae;

int cur_idx = 0; // represent which model should be rendered now
vector<string> model_list{ "../TextureModels/Fushigidane.obj", "../TextureModels/Mew.obj","../TextureModels/Nyarth.obj","../TextureModels/Zenigame.obj", "../TextureModels/laurana500.obj", "../TextureModels/Nala.obj", "../TextureModels/Square.obj" };

GLuint program;


// uniforms location
GLuint iLocP;
GLuint iLocV;
GLuint iLocM;
// [NOTE] AS02: Shader attributes for uniform variables
GLuint iLocLightType;
GLuint iLocKa;
GLuint iLocKd;
GLuint iLocKs;
GLuint iLocShininess;

GLuint iLocTex;

bool light_edit = false;
int iLocPervex = 0;

bool mag_filter = false;
bool min_filter = false;


struct iLocLightData
{
	GLuint position;
	GLuint ambient;
	GLuint diffuse;
	GLuint specular;
	GLuint spotDirection;
	GLuint spotCutoff;
	GLuint spotExponent;
	GLuint constantAttenuation;
	GLuint linearAttenuation;
	GLuint quadraticAttenuation;
} iLocLightData[3];

struct LightData
{
	Vector3 position;
	Vector3 spotDirection;
	Vector3 ambient;
	Vector3 diffuse;
	Vector3 specular;
	GLfloat spotExponent;
	GLfloat spotCutoff;
	GLfloat constantAttenuation;
	GLfloat linearAttenuation;
	GLfloat quadraticAttenuation;
} LightData[3];

int light_type = 0;

void set_directional_light_var(GLuint p) {
	iLocLightData[0].position = glGetUniformLocation(p, "light[0].position");
	iLocLightData[0].ambient = glGetUniformLocation(p, "light[0].Ambient");
	iLocLightData[0].diffuse = glGetUniformLocation(p, "light[0].Diffuse");
	iLocLightData[0].specular = glGetUniformLocation(p, "light[0].Specular");
	iLocLightData[0].spotDirection = glGetUniformLocation(p, "light[0].spotDirection");
	iLocLightData[0].spotCutoff = glGetUniformLocation(p, "light[0].spotCutoff");
	iLocLightData[0].spotExponent = glGetUniformLocation(p, "light[0].spotExponent");
	iLocLightData[0].constantAttenuation = glGetUniformLocation(p, "light[0].constantAttenuation");
	iLocLightData[0].linearAttenuation = glGetUniformLocation(p, "light[0].linearAttenuation");
	iLocLightData[0].quadraticAttenuation = glGetUniformLocation(p, "light[0].quadraticAttenuation");
}

void set_point_light_var(GLuint p) {
	iLocLightData[1].position = glGetUniformLocation(p, "light[1].position");
	iLocLightData[1].ambient = glGetUniformLocation(p, "light[1].Ambient");
	iLocLightData[1].diffuse = glGetUniformLocation(p, "light[1].Diffuse");
	iLocLightData[1].specular = glGetUniformLocation(p, "light[1].Specular");
	iLocLightData[1].spotDirection = glGetUniformLocation(p, "light[1].spotDirection");
	iLocLightData[1].spotCutoff = glGetUniformLocation(p, "light[1].spotCutoff");
	iLocLightData[1].spotExponent = glGetUniformLocation(p, "light[1].spotExponent");
	iLocLightData[1].constantAttenuation = glGetUniformLocation(p, "light[1].constantAttenuation");
	iLocLightData[1].linearAttenuation = glGetUniformLocation(p, "light[1].linearAttenuation");
	iLocLightData[1].quadraticAttenuation = glGetUniformLocation(p, "light[1].quadraticAttenuation");
}

void set_spot_light_var(GLuint p) {
	iLocLightData[2].position = glGetUniformLocation(p, "light[2].position");
	iLocLightData[2].ambient = glGetUniformLocation(p, "light[2].Ambient");
	iLocLightData[2].diffuse = glGetUniformLocation(p, "light[2].Diffuse");
	iLocLightData[2].specular = glGetUniformLocation(p, "light[2].Specular");
	iLocLightData[2].spotDirection = glGetUniformLocation(p, "light[2].spotDirection");
	iLocLightData[2].spotCutoff = glGetUniformLocation(p, "light[2].spotCutoff");
	iLocLightData[2].spotExponent = glGetUniformLocation(p, "light[2].spotExponent");
	iLocLightData[2].constantAttenuation = glGetUniformLocation(p, "light[2].constantAttenuation");
	iLocLightData[2].linearAttenuation = glGetUniformLocation(p, "light[2].linearAttenuation");
	iLocLightData[2].quadraticAttenuation = glGetUniformLocation(p, "light[2].quadraticAttenuation");
}

void initiliaze_light_data() {

	// Initialize light data for directional light
	LightData[0].position = Vector3(1.0f, 1.0f, 1.0f);
	LightData[0].ambient = Vector3(0.15f, 0.15f, 0.15f);
	LightData[0].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	LightData[0].specular = Vector3(1.0f, 1.0f, 1.0f);

	// Initialize light data for point light
	LightData[1].position = Vector3(0.0f, 2.0f, 1.0f);
	LightData[1].ambient = Vector3(0.15f, 0.15f, 0.15f);
	LightData[1].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	LightData[1].specular = Vector3(1.0f, 1.0f, 1.0f);

	LightData[1].constantAttenuation = 0.01f;
	LightData[1].linearAttenuation = 0.8f;
	LightData[1].quadraticAttenuation = 0.1f;

	// Initialize light data for spot light
	LightData[2].position = Vector3(0.0f, 0.0f, 2.0f);
	LightData[2].ambient = Vector3(0.15f, 0.15f, 0.15f);
	LightData[2].diffuse = Vector3(1.0f, 1.0f, 1.0f);
	LightData[2].specular = Vector3(1.0f, 1.0f, 1.0f);
	LightData[2].spotDirection = Vector3(0.0f, 0.0f, -1.0f);

	LightData[2].spotExponent = 50;
	LightData[2].spotCutoff = deg2rad(30);
	LightData[2].constantAttenuation = 0.05f;
	LightData[2].linearAttenuation = 0.3f;
	LightData[2].quadraticAttenuation = 0.6f;
}

void updateLightData() {

	glUniform1i(iLocLightType, light_type);

	for (int i = 0; i <= 2; i++)
		glUniform3fv(iLocLightData[i].ambient, 1, &LightData[0].ambient[0]);

	for (int i = 0; i <= 2; i++)
		glUniform3fv(iLocLightData[i].diffuse, 1, &LightData[0].diffuse[0]);

	for (int i = 0; i <= 2; i++)
		glUniform3fv(iLocLightData[i].specular, 1, &LightData[0].specular[0]);


	glUniform3fv(iLocLightData[0].position, 1, &LightData[0].position[0]);

	glUniform3fv(iLocLightData[1].position, 1, &LightData[1].position[0]);
	glUniform1f(iLocLightData[1].constantAttenuation, LightData[1].constantAttenuation);
	glUniform1f(iLocLightData[1].linearAttenuation, LightData[1].linearAttenuation);
	glUniform1f(iLocLightData[1].quadraticAttenuation, LightData[1].quadraticAttenuation);

	glUniform3fv(iLocLightData[2].position, 1, &LightData[2].position[0]);
	glUniform3fv(iLocLightData[2].spotDirection, 1, &LightData[2].spotDirection[0]);
	glUniform1f(iLocLightData[2].spotExponent, LightData[2].spotExponent);
	glUniform1f(iLocLightData[2].spotCutoff, LightData[2].spotCutoff);
	glUniform1f(iLocLightData[2].constantAttenuation, LightData[2].constantAttenuation);
	glUniform1f(iLocLightData[2].linearAttenuation, LightData[2].linearAttenuation);
	glUniform1f(iLocLightData[2].quadraticAttenuation, LightData[2].quadraticAttenuation);
}

static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}

Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		1.0f, 0.0f, 0.0f, vec.x,
		0.0f, 1.0f, 0.0f, vec.y,
		0.0f, 0.0f, 1.0f, vec.z,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;

	mat = Matrix4(
		vec.x, 0.0f, 0.0f, 0.0f,
		0.0f, vec.y, 0.0f, 0.0f,
		0.0f, 0.0f, vec.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, cosf(val), -sinf(val), 0.0f,
		0.0f, sinf(val), cosf(val), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cosf(val), 0.0f, sinf(val), 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		-sinf(val), 0.0f, cosf(val), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	mat = Matrix4(
		cosf(val), -sinf(val), 0.0f, 0.0f,
		sinf(val), cosf(val), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

void setViewingMatrix()
{
	float F[3] = { main_camera.position.x - main_camera.center.x, main_camera.position.y - main_camera.center.y, main_camera.position.z - main_camera.center.z };
	float U[3] = { main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z };
	float R[3];
	Normalize(F);
	Cross(U, F, R);
	Normalize(R);
	Cross(F, R, U);
	Normalize(U);

	view_matrix[0] = R[0];
	view_matrix[1] = R[1];
	view_matrix[2] = R[2];
	view_matrix[3] = 0;
	view_matrix[4] = U[0];
	view_matrix[5] = U[1];
	view_matrix[6] = U[2];
	view_matrix[7] = 0;
	view_matrix[8] = F[0];
	view_matrix[9] = F[1];
	view_matrix[10] = F[2];
	view_matrix[11] = 0;
	view_matrix[12] = 0;
	view_matrix[13] = 0;
	view_matrix[14] = 0;
	view_matrix[15] = 1;

	view_matrix = view_matrix * translate(-main_camera.position);
}

void setOrthogonal()
{
	cur_proj_mode = Orthogonal;
	// handle side by side view
	float right = proj.right / 2;
	float left = proj.left / 2;
	project_matrix[0] = 2 / (right - left);
	project_matrix[1] = 0;
	project_matrix[2] = 0;
	project_matrix[3] = -(right + left) / (right - left);
	project_matrix[4] = 0;
	project_matrix[5] = 2 / (proj.top - proj.bottom);
	project_matrix[6] = 0;
	project_matrix[7] = -(proj.top + proj.bottom) / (proj.top - proj.bottom);
	project_matrix[8] = 0;
	project_matrix[9] = 0;
	project_matrix[10] = -2 / (proj.farClip - proj.nearClip);
	project_matrix[11] = -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip);
	project_matrix[12] = 0;
	project_matrix[13] = 0;
	project_matrix[14] = 0;
	project_matrix[15] = 1;
}

void setPerspective()
{
	const float tanHalfFOV = tanf((proj.fovy / 2.0) / 180.0 * acosf(-1.0));
	
	cur_proj_mode = Perspective;
	project_matrix[0] = 1.0f / (tanHalfFOV * proj.aspect);
	project_matrix[1] = 0;
	project_matrix[2] = 0;
	project_matrix[3] = 0;
	project_matrix[4] = 0;
	project_matrix[5] = 1.0f / tanHalfFOV;
	project_matrix[6] = 0;
	project_matrix[7] = 0;
	project_matrix[8] = 0;
	project_matrix[9] = 0;
	project_matrix[10] = -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip);
	project_matrix[11] = -(2 * proj.farClip * proj.nearClip) / (proj.farClip - proj.nearClip);
	project_matrix[12] = 0;
	project_matrix[13] = 0;
	project_matrix[14] = -1;
	project_matrix[15] = 0;
}

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	// glViewport(0, 0, width, height);
	proj.aspect = (float)(width / 2) / (float)height;
	if (cur_proj_mode == Perspective) {
		setPerspective();
	}

	screenWidth = width;
	screenHeight = height;
}

void Vector3ToFloat4(Vector3 v, GLfloat res[4])
{
	res[0] = v.x;
	res[1] = v.y;
	res[2] = v.z;
	res[3] = 1;
}

vector<Vector2> eye_tex_offset_list {Vector2(0.0f, 0.0f) , Vector2(0.0f, -0.25f), Vector2(0.0f, -0.5f), Vector2(0.0f, -0.75f), Vector2(0.5f, 0.0f), Vector2(0.5f, -0.25f), Vector2(0.5f, -0.5f)};


// Render function for display rendering
void RenderScene(int per_vertex_or_per_pixel) {	
	Vector3 modelPos = models[cur_idx].position;

	// [NOTE] Update light
	updateLightData();
	
	Vector2 eye_tex_offset;
	Vector2 body_tex_offset = Vector2(0.0f, 0.0f);

	Matrix4 T, R, S;
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	// render object
	Matrix4 model_matrix = T * R * S;
	glUniformMatrix4fv(iLocM, 1, GL_FALSE, model_matrix.getTranspose());
	glUniformMatrix4fv(iLocV, 1, GL_FALSE, view_matrix.getTranspose());
	glUniformMatrix4fv(iLocP, 1, GL_FALSE, project_matrix.getTranspose());

	glUniform1i(iLocPervex, per_vertex_or_per_pixel);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glUniform3fv(iLocKa, 1, &models[cur_idx].shapes[i].material.Ka[0]);
		glUniform3fv(iLocKd, 1, &models[cur_idx].shapes[i].material.Kd[0]);
		glUniform3fv(iLocKs, 1, &models[cur_idx].shapes[i].material.Ks[0]);
		glUniform1f(iLocShininess, models[cur_idx].shapes[i].material.shininess);

		glBindVertexArray(models[cur_idx].shapes[i].vao);

		// [TODO] Bind texture and modify texture filtering & wrapping mode
		// Hint: glActiveTexture, glBindTexture, glTexParameteri
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, models[cur_idx].shapes[i].material.diffuseTexture);

		if (models[cur_idx].shapes[i].material.isEye == 1) {
			eye_tex_offset = eye_tex_offset_list[models[cur_idx].cur_eye_offset_idx];
			glUniform2fv(iLocTex, 1, &eye_tex_offset[0]);
		}
		else {
			glUniform2fv(iLocTex, 1, &body_tex_offset[0]);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (mag_filter)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		if (min_filter)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}
	
}

// Call back function for keyboard
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			exit(0);
			break;
		case GLFW_KEY_Z:
			cur_idx = (cur_idx + 1) % model_list.size();
			break;
		case GLFW_KEY_X:
			cur_idx = (cur_idx - 1 + model_list.size()) % model_list.size();
			break;
		case GLFW_KEY_O:
			if (cur_proj_mode == Perspective)
			{
				proj.farClip -= 3.0f;
				setViewingMatrix();
				setOrthogonal();
			}
			break;
		case GLFW_KEY_P:
			if (cur_proj_mode == Orthogonal)
			{
				proj.farClip += 3.0f;
				setViewingMatrix();
				setPerspective();
			}
			break;
		case GLFW_KEY_T:
			cur_trans_mode = GeoTranslation;
			break;
		case GLFW_KEY_S:
			cur_trans_mode = GeoScaling;
			break;
		case GLFW_KEY_R:
			cur_trans_mode = GeoRotation;
			break;
		case GLFW_KEY_E:
			cur_trans_mode = ViewEye;
			break;
		case GLFW_KEY_C:
			cur_trans_mode = ViewCenter;
			break;
		case GLFW_KEY_U:
			cur_trans_mode = ViewUp;
			break;
		case GLFW_KEY_I:
			cout << endl;
			break;
		case GLFW_KEY_L:
			light_type += 1;
			if (light_type > 2) light_type -= 3;
			if (light_type == 0) cout << "Light Mode: " << "Directional Light" << endl;
			if (light_type == 1) cout << "Light Mode: " << "Positional Light" << endl;
			if (light_type == 2) cout << "Light Mode: " << "Spot Light" << endl;
			break;
		case GLFW_KEY_J:
			cur_trans_mode = ShininessEdit;
			break;
		case GLFW_KEY_K:
			cur_trans_mode = LightEdit;
			break;
		case GLFW_KEY_G:
			mag_filter = !mag_filter;
		case GLFW_KEY_B:
			min_filter = !min_filter;
			break;
		case GLFW_KEY_LEFT:
			models[cur_idx].cur_eye_offset_idx = (models[cur_idx].cur_eye_offset_idx - 1 + models[cur_idx].max_eye_offset) % models[cur_idx].max_eye_offset;
			break;
		case GLFW_KEY_RIGHT:
			models[cur_idx].cur_eye_offset_idx = (models[cur_idx].cur_eye_offset_idx + 1) % models[cur_idx].max_eye_offset;
			break;
		default:
			break;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// scroll up positive, otherwise it would be negtive
	switch (cur_trans_mode)
	{
	case ViewEye:
		main_camera.position.z -= 0.025 * (float)yoffset;
		setViewingMatrix();
		printf("Camera Position = ( %f , %f , %f )\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
		break;
	case ViewCenter:
		main_camera.center.z += 0.1 * (float)yoffset;
		setViewingMatrix();
		printf("Camera Viewing Direction = ( %f , %f , %f )\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
		break;
	case ViewUp:
		main_camera.up_vector.z += 0.33 * (float)yoffset;
		setViewingMatrix();
		printf("Camera Up Vector = ( %f , %f , %f )\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
		break;
	case GeoTranslation:
		models[cur_idx].position.z += 0.1 * (float)yoffset;
		break;
	case GeoScaling:
		models[cur_idx].scale.z += 0.01 * (float)yoffset;
		break;
	case GeoRotation:
		models[cur_idx].rotation.z += (acosf(-1.0f) / 180.0) * 5 * (float)yoffset;
		break;
	case LightEdit:
		if (light_type == 0)
		{
			LightData[0].diffuse += Vector3(0.1f, 0.1f, 0.1f) * (float)yoffset;
		}
		else if (light_type == 1)
		{
			LightData[1].diffuse += Vector3(0.1f, 0.1f, 0.1f) * (float)yoffset;
		}
		else if (light_type == 2)
		{
			LightData[2].spotCutoff += 0.005 * (float)yoffset;
		}
		break;
	case ShininessEdit:
		for (int i = 0; i < 5; i++)
			for (int j = 0; j < models[i].shapes.size(); j++)
				models[i].shapes[j].material.shininess += 5 * (float)yoffset;
		break;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		mouse_pressed = true;
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouse_pressed = false;
		starting_press_x = -1;
		starting_press_y = -1;
	}
		
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (mouse_pressed) {
		if (starting_press_x < 0 || starting_press_y < 0) {
			starting_press_x = (int)xpos;
			starting_press_y = (int)ypos;
		}
		else {
			float diff_x = starting_press_x - (int)xpos;
			float diff_y = starting_press_y - (int)ypos;
			starting_press_x = (int)xpos;
			starting_press_y = (int)ypos;
			switch (cur_trans_mode)
			{
			case ViewEye:
				main_camera.position.x += diff_x * (1.0 / 400.0);
				main_camera.position.y += diff_y * (1.0 / 400.0);
				setViewingMatrix();
				printf("Camera Position = ( %f , %f , %f )\n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
				break;
			case ViewCenter:
				main_camera.center.x += diff_x * (1.0 / 400.0);
				main_camera.center.y -= diff_y * (1.0 / 400.0);
				setViewingMatrix();
				printf("Camera Viewing Direction = ( %f , %f , %f )\n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
				break;
			case ViewUp:
				main_camera.up_vector.x += diff_x * 0.1;
				main_camera.up_vector.y += diff_y * 0.1;
				setViewingMatrix();
				printf("Camera Up Vector = ( %f , %f , %f )\n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
				break;
			case GeoTranslation:
				models[cur_idx].position.x += -diff_x * (1.0 / 400.0);
				models[cur_idx].position.y += diff_y * (1.0 / 400.0);
				break;
			case GeoScaling:
				models[cur_idx].scale.x += diff_x * 0.001;
				models[cur_idx].scale.y += diff_y * 0.001;
				break;
			case GeoRotation:
				models[cur_idx].rotation.x += acosf(-1.0f) / 180.0*diff_y*(45.0 / 400.0);
				models[cur_idx].rotation.y += acosf(-1.0f) / 180.0*diff_x*(45.0 / 400.0);
				break;
			case LightEdit:
				LightData[0].position[0] += diff_x * 0.0025;
				LightData[0].position[1] -= diff_y * 0.0025;
				LightData[1].position[0] += diff_x * 0.0025;
				LightData[1].position[1] -= diff_y * 0.0025;
				LightData[2].position[0] += diff_x * 0.0025;
				LightData[2].position[1] -= diff_y * 0.0025;

				cout << "Current Light Position : " << LightData[light_type].position << endl;
				break;
			}
		}
	}
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs.glsl");
	fs = textFileRead("shader.fs.glsl");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	// [NOTE] set variables
	set_directional_light_var(p);
	set_point_light_var(p);
	set_spot_light_var(p);

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }

	program = p;
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, vector<GLfloat>& textureCoords, vector<int>& material_id, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}
	
	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i)/ scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
			normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
			normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			// Optional: texture coordinate
			textureCoords.push_back(attrib->texcoords[2 * idx.texcoord_index + 0]);
			textureCoords.push_back(attrib->texcoords[2 * idx.texcoord_index + 1]);
			// The material of this vertex
			material_id.push_back(shape->mesh.material_ids[f]);
		}
		index_offset += fv;
	}
}

static string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

GLuint LoadTextureImage(string image_path)
{
	// printf("Loading image %s\n", image_path.c_str());
	int channel, width, height;
	int require_channel = 4;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc *data = stbi_load(image_path.c_str(), &width, &height, &channel, require_channel);
	if (data != NULL)
	{
		GLuint tex = 0;

		// [TODO] Bind the image to texture
		// Hint: glGenTextures, glBindTexture, glTexImage2D, glGenerateMipmap
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image from memory after binding to texture
		stbi_image_free(data);
		return tex;
	}
	else
	{
		cout << "LoadTextureImage: Cannot load image from " << image_path << endl;
		return -1;
	}
}

vector<Shape> SplitShapeByMaterial(vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, vector<GLfloat>& textureCoords, vector<int>& material_id, vector<PhongMaterial>& materials)
{
	vector<Shape> res;
	for (int m = 0; m < materials.size(); m++)
	{
		vector<GLfloat> m_vertices, m_colors, m_normals, m_textureCoords;
		for (int v = 0; v < material_id.size(); v++) 
		{
			// extract all vertices with same material id and create a new shape for it.
			if (material_id[v] == m)
			{
				m_vertices.push_back(vertices[v * 3 + 0]);
				m_vertices.push_back(vertices[v * 3 + 1]);
				m_vertices.push_back(vertices[v * 3 + 2]);

				m_colors.push_back(colors[v * 3 + 0]);
				m_colors.push_back(colors[v * 3 + 1]);
				m_colors.push_back(colors[v * 3 + 2]);

				m_normals.push_back(normals[v * 3 + 0]);
				m_normals.push_back(normals[v * 3 + 1]);
				m_normals.push_back(normals[v * 3 + 2]);

				m_textureCoords.push_back(textureCoords[v * 2 + 0]);
				m_textureCoords.push_back(textureCoords[v * 2 + 1]);
			}
		}

		if (!m_vertices.empty())
		{
			Shape tmp_shape;
			glGenVertexArrays(1, &tmp_shape.vao);
			glBindVertexArray(tmp_shape.vao);

			glGenBuffers(1, &tmp_shape.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
			glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(GL_FLOAT), &m_vertices.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
			tmp_shape.vertex_count = m_vertices.size() / 3;

			glGenBuffers(1, &tmp_shape.p_color);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
			glBufferData(GL_ARRAY_BUFFER, m_colors.size() * sizeof(GL_FLOAT), &m_colors.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glGenBuffers(1, &tmp_shape.p_normal);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
			glBufferData(GL_ARRAY_BUFFER, m_normals.size() * sizeof(GL_FLOAT), &m_normals.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

			glGenBuffers(1, &tmp_shape.p_texCoord);
			glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_texCoord);
			glBufferData(GL_ARRAY_BUFFER, m_textureCoords.size() * sizeof(GL_FLOAT), &m_textureCoords.at(0), GL_STATIC_DRAW);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);

			tmp_shape.material = materials[m];
			res.push_back(tmp_shape);
		}
	}

	return res;
}

void LoadTexturedModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;
	vector<GLfloat> textureCoords;
	vector<int> material_id;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", shapes.size(), materials.size());
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);

		material.diffuseTexture = LoadTextureImage(base_dir + string(materials[i].diffuse_texname));
		if (material.diffuseTexture == -1)
		{
			cout << "LoadTexturedModels: Fail to load model's material " << i << endl;
			system("pause");
			
		}

		if (string(materials[i].diffuse_texname).find("Eye") != std::string::npos) {
			material.isEye = 1;
			cout << "Material is Eye : " << material.isEye << endl;
		}
		else 
			material.isEye = 0;
		
		allMaterial.push_back(material);
	}
	
	for (int i = 0; i < shapes.size(); i++)
	{
		vertices.clear();
		colors.clear();
		normals.clear();
		textureCoords.clear();
		material_id.clear();

		normalization(&attrib, vertices, colors, normals, textureCoords, material_id, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		// split current shape into multiple shapes base on material_id.
		vector<Shape> splitedShapeByMaterial = SplitShapeByMaterial(vertices, colors, normals, textureCoords, material_id, allMaterial);

		// concatenate splited shape to model's shape list
		tmp_model.shapes.insert(tmp_model.shapes.end(), splitedShapeByMaterial.begin(), splitedShapeByMaterial.end());
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)(WINDOW_WIDTH / 2) / (float)WINDOW_HEIGHT; // adjust width for side by side view

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
	initiliaze_light_data(); // [NOTE] initialize light data

}

void setUniformVariables()
{
	iLocP = glGetUniformLocation(program, "um4p");
	iLocV = glGetUniformLocation(program, "um4v");
	iLocM = glGetUniformLocation(program, "um4m");

	iLocPervex = glGetUniformLocation(program, "iLocPervex");
	iLocLightType = glGetUniformLocation(program, "light_type");
	iLocKa = glGetUniformLocation(program, "material.Ka");
	iLocKd = glGetUniformLocation(program, "material.Kd");
	iLocKs = glGetUniformLocation(program, "material.Ks");
	iLocShininess = glGetUniformLocation(program, "material.shininess");

	// [TODO] Get uniform location of texture
	iLocTex = glGetUniformLocation(program, "tex_offset");
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();
	setUniformVariables();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);

	for (string model_path : model_list){
		LoadTexturedModels(model_path);
	}
	// [NOTE] set shininess
	for (int i = 0; i < 7; i++)
		for (int j = 0; j < models[i].shapes.size(); j++)
			models[i].shapes[j].material.shininess = 64;
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{

    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "110065502 HW3", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

	glPrintContextInfo(false);
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		// render left view
		glViewport(0, 0, screenWidth / 2, screenHeight);
        RenderScene(1);
		// render right view
		glViewport(screenWidth / 2, 0, screenWidth / 2, screenHeight);
		RenderScene(0);
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
