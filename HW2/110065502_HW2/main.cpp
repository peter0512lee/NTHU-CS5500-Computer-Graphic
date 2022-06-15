#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

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
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 800;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	LightEdit = 3,
	ShininessEdit = 4,
};

// struct Uniform
// {
//     GLint iLocMVP;
// };
// Uniform uniform;

GLint iLocMVP;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;
	GLfloat shininess; // [NOTE] add shininess
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);    // Euler form

	vector<Shape> shapes;
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

TransMode cur_trans_mode = GeoTranslation;

Matrix4 model_matrix; // [NOTE] model_matrix
Matrix4 view_matrix;
Matrix4 project_matrix;

int current_x, current_y;
int diff_x, diff_y;

int cur_idx = 0; // represent which model should be rendered now

// [NOTE] AS02: Shader attributes for uniform variables
GLuint iLocV;
GLuint iLocM;
GLuint iLocLightType;
GLuint iLocKa;
GLuint iLocKd;
GLuint iLocKs;
GLuint iLocShininess;

bool light_edit = false;
int goraud_or_phong = 0;

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

void set_basic_variables(GLuint p) {
	goraud_or_phong = glGetUniformLocation(p, "goraud_or_phong");

	iLocMVP = glGetUniformLocation(p, "mvp");
	iLocV = glGetUniformLocation(p, "view_matrix");
	iLocM = glGetUniformLocation(p, "model_matrix");
	iLocLightType = glGetUniformLocation(p, "light_type");
	iLocKa = glGetUniformLocation(p, "material.Ka");
	iLocKd = glGetUniformLocation(p, "material.Kd");
	iLocKs = glGetUniformLocation(p, "material.Ks");
	iLocShininess = glGetUniformLocation(p, "material.shininess");
}

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


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
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

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
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


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;

	val = val * PI / 180;

	mat = Matrix4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, cos(val), -sin(val), 0.0f,
		0.0f, sin(val), cos(val), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;

	val = val * PI / 180;

	mat = Matrix4(
		cos(val), 0.0f, sin(val), 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		-sin(val), 0.0f, cos(val), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;

	val = val * PI / 180;

	mat = Matrix4(
		cos(val), -sin(val), 0.0f, 0.0f,
		sin(val), cos(val), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	Vector3 P1 = main_camera.position;
	Vector3 P2 = main_camera.center;
	Vector3 P3 = main_camera.up_vector;

	Matrix4 T = translate(-main_camera.position);

	GLfloat Rx[3];
	GLfloat Ry[3];
	GLfloat Rz[3];
	GLfloat p1p2[3], p1p3[3];
	for (int i = 0; i < 3; i++) p1p2[i] = (P2 - P1)[i];
	for (int i = 0; i < 3; i++) p1p3[i] = (P3 - P1)[i];

	Normalize(p1p2);
	Rz[0] = -p1p2[0];
	Rz[1] = -p1p2[1];
	Rz[2] = -p1p2[2];

	Cross(p1p2, p1p3, Rx);
	Normalize(Rx);

	Cross(Rz, Rx, Ry);

	Matrix4 R = Matrix4(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1
	);

	view_matrix = R * T;
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	GLfloat M00 = (1 / (tan(deg2rad(proj.fovy) / 2) * proj.aspect));
	GLfloat M11 = 1 / tan(deg2rad(proj.fovy) / 2);
	GLfloat M22 = (proj.nearClip + proj.farClip) / (proj.nearClip - proj.farClip);
	GLfloat M23 = 2 * proj.nearClip * proj.farClip / (proj.nearClip - proj.farClip);

	project_matrix = Matrix4(
		M00, 0.0f, 0.0f, 0.0f,
		0.0f, M11, 0.0f, 0.0f,
		0.0f, 0.0f, M22, M23,
		0.0f, 0.0f, -1.0f, 0.0f
	);
}

void setGLMatrix(GLfloat* glm, Matrix4& m) {
	glm[0] = m[0];  glm[4] = m[1];   glm[8] = m[2];    glm[12] = m[3];
	glm[1] = m[4];  glm[5] = m[5];   glm[9] = m[6];    glm[13] = m[7];
	glm[2] = m[8];  glm[6] = m[9];   glm[10] = m[10];   glm[14] = m[11];
	glm[3] = m[12];  glm[7] = m[13];  glm[11] = m[14];   glm[15] = m[15];
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	// [TODO] change your aspect ratio
	proj.aspect = (float)width / (float)height;
	setPerspective();
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;
	glViewport(0, 0, width, height);
}

// Render function for display rendering
void RenderScene(void) {
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// [NOTE] Update light
	updateLightData();

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix
	model_matrix = T * R * S;
	MVP = project_matrix * view_matrix * model_matrix;
	// row-major ---> column-major
	setGLMatrix(mvp, MVP);

	// use uniform to send view_matrix to vertex shader
	glUniformMatrix4fv(iLocV, 1, GL_FALSE, view_matrix.getTranspose());

	// use uniform to send model_matrix to vertex shader
	glUniformMatrix4fv(iLocM, 1, GL_FALSE, model_matrix.getTranspose());

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);

	// draw left model (per-vertex)
	glUniform1i(goraud_or_phong, 0);
	glViewport(0, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		// set glViewport and draw twice ...
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}

	// draw right model (per-pixel)
	glUniform1i(goraud_or_phong, 1);
	glViewport(WINDOW_WIDTH / 2, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		glUniform3fv(iLocKa, 1, &models[cur_idx].shapes[i].material.Ka[0]);
		glUniform3fv(iLocKd, 1, &models[cur_idx].shapes[i].material.Kd[0]);
		glUniform3fv(iLocKs, 1, &models[cur_idx].shapes[i].material.Ks[0]);
		glUniform1f(iLocShininess, models[cur_idx].shapes[i].material.shininess);
		// set glViewport and draw twice ...
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		// switch the model
		cur_idx = (cur_idx >= 4) ? 0 : cur_idx + 1;
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
		// switch the model
		cur_idx = (cur_idx <= 0) ? 4 : cur_idx - 1;
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
		// switch to translation mode
		cur_trans_mode = GeoTranslation;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		// switch to scale mode
		cur_trans_mode = GeoScaling;
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		// switch to rotation mode
		cur_trans_mode = GeoRotation;
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
	{
		light_type += 1;
		if (light_type > 2) light_type -= 3;
		if (light_type == 0) cout << "Light Mode: " << "Directional Light" << endl;
		if (light_type == 1) cout << "Light Mode: " << "Positional Light" << endl;
		if (light_type == 2) cout << "Light Mode: " << "Spot Light" << endl;
	}
	if (key == GLFW_KEY_J && action == GLFW_PRESS)
		cur_trans_mode = ShininessEdit;
	if (key == GLFW_KEY_K && action == GLFW_PRESS)
		cur_trans_mode = LightEdit;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	if (yoffset < 0)
	{
		switch (cur_trans_mode) {
		case GeoTranslation:
			models[cur_idx].position.z -= 0.1;
			break;
		case GeoScaling:
			models[cur_idx].scale.z -= 0.01;
			break;
		case GeoRotation:
			models[cur_idx].rotation.z -= (PI / 180.0) * 3;
			break;
			// AS02
		case LightEdit:
			if (light_type == 0)
			{
				LightData[0].diffuse -= Vector3(0.1f, 0.1f, 0.1f);
			}
			else if (light_type == 1)
			{
				LightData[1].diffuse -= Vector3(0.1f, 0.1f, 0.1f);
			}
			else if (light_type == 2)
			{
				LightData[2].spotCutoff -= 0.005;
			}
			break;
		case ShininessEdit:
			for (int i = 0; i < 5; i++)
				for (int j = 0; j < models[i].shapes.size(); j++)
					models[i].shapes[j].material.shininess -= 5;
			break;
		default:
			break;
		}
	}
	else
	{
		switch (cur_trans_mode) {
		case GeoTranslation:
			models[cur_idx].position.z += 0.1;
			break;
		case GeoScaling:
			models[cur_idx].scale.z += 0.01;
			break;
		case GeoRotation:
			models[cur_idx].rotation.z += (PI / 180.0) * 3;
			break;
			// AS02
		case LightEdit:
			if (light_type == 0)
			{
				LightData[0].diffuse += Vector3(0.1f, 0.1f, 0.1f);
			}
			else if (light_type == 1)
			{
				LightData[1].diffuse += Vector3(0.1f, 0.1f, 0.1f);
			}
			else if (light_type == 2)
			{
				LightData[2].spotCutoff += 0.005;
			}
			break;
		case ShininessEdit:
			for (int i = 0; i < 5; i++)
				for (int j = 0; j < models[i].shapes.size(); j++)
					models[i].shapes[j].material.shininess += 5;
			break;
		default:
			break;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	double xpos, ypos;
	// [TODO] mouse press callback function
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		current_x = xpos;
		current_y = ypos;

	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		current_x = xpos;
		current_y = ypos;
	}
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	diff_x = xpos - current_x;
	diff_y = ypos - current_y;
	current_x = xpos;
	current_y = ypos;
	int left_mouse_click = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	int right_mouse_click = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

	if (left_mouse_click == GLFW_PRESS || right_mouse_click == GLFW_PRESS)
	{
		switch (cur_trans_mode) {

		case GeoTranslation:
			models[cur_idx].position.x += diff_x * 0.0025;
			models[cur_idx].position.y -= diff_y * 0.0025;
			break;

		case GeoScaling:
			models[cur_idx].scale.x += diff_x * 0.025;
			models[cur_idx].scale.y += diff_y * 0.025;
			break;

		case GeoRotation:
			models[cur_idx].rotation.x += PI / 180.0 * diff_y;
			models[cur_idx].rotation.y += PI / 180.0 * diff_x;
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
		default:
			break;
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

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

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
	glAttachShader(p, f);
	glAttachShader(p, v);

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
	set_basic_variables(p);
	set_directional_light_var(p);
	set_point_light_var(p);
	set_spot_light_var(p);

	// uniform.iLocMVP = glGetUniformLocation(p, "mvp");

	if (success)
		glUseProgram(p);
	else
	{
		system("pause");
		exit(123);
	}
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
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
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
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
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

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

	printf("Load Models Success ! Shapes size %d Material size %d\n", int(shapes.size()), int(materials.size()));
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	// [TODO] Setup some parameters if you need
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();    //set default projection matrix as perspective matrix
	initiliaze_light_data(); // [NOTE] initialize light data
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj" };
	// [TODO] Load five model at here
	for (int i = 0; i < 5; i++)
		LoadModels(model_list[i]);

	// [NOTE] set shininess
	for (int i = 0; i < 5; i++)
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
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "110065502 HW2", NULL, NULL);
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
		RenderScene();

		// swap buffer from back to front
		glfwSwapBuffers(window);

		// Poll input event
		glfwPollEvents();
	}

	// just for compatibiliy purposes
	return 0;
}
