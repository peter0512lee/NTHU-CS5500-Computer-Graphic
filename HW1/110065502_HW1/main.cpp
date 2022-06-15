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
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// custom parameters
bool isPolygon = false;
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
};

GLint iLocMVP;

vector<string> filenames; // .obj filename list

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form
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

int current_x, current_y;
int diff_x, diff_y;

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	int materialId;
	int indexCount;
	GLuint m_texture;
} Shape;
Shape quad;
Shape m_shpae;
vector<Shape> m_shape_list;
int cur_idx = 0; // represent which model should be rendered now


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

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
	cur_proj_mode = Orthogonal;

	GLfloat M00 = 2.0f / (proj.right - proj.left) / proj.aspect;
	GLfloat M03 = -((proj.right + proj.left) / (proj.right - proj.left));
	GLfloat M11 = 2.0f / (proj.top - proj.bottom);
	GLfloat M13 = -((proj.top + proj.bottom) / (proj.top - proj.bottom));
	GLfloat M22 = -2.0f / (proj.farClip - proj.nearClip);
	GLfloat M23 = -((proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip));
	
	project_matrix = Matrix4(
		M00, 0.0f, 0.0f, M03,
		0.0f, M11, 0.0f, M13,
		0.0f, 0.0f, M22, M23,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	

}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	cur_proj_mode = Perspective;

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


// Vertex buffers
GLuint VAO, VBO, VBO_COLOR;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	// [TODO] change your aspect ratio
	proj.aspect = (float)width / (float)height;

	if (cur_proj_mode == Orthogonal)
		setOrthogonal();
	else
		setPerspective();

	glViewport(0, 0, width, height);

}

void drawPlane()
{
	GLfloat vertices[18]{ 
		1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0,
		1.0, -0.9,  1.0,
		-1.0, -0.9,  1.0,
		-1.0, -0.9, -1.0 
	};

	GLfloat colors[18]{ 
		0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,1.0,0.0,
		0.0,0.5,0.8,
		0.0,0.5,0.8,
		0.0,1.0,0.0 
	};


	// [TODO] draw the plane with above vertices and color

	Matrix4 MVP = project_matrix * view_matrix;
	GLfloat mvp[16];
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);

	// VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// VBO
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	// VBO_COLOR
	glGenBuffers(1, &VBO_COLOR);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_COLOR);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(1);

	// draw
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;

	// [TODO] update translation, rotation and scaling
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);

	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix
	MVP = project_matrix * view_matrix * T * R * S;

	// [TODO] row-major ---> column-major
	mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
	mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
	mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];  mvp[14] = MVP[11];
	mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];  mvp[15] = MVP[15];

	if (isPolygon) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// use uniform to send mvp to vertex shader
	glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	glBindVertexArray(m_shape_list[cur_idx].vao);
	glDrawArrays(GL_TRIANGLES, 0, m_shape_list[cur_idx].vertex_count);
	drawPlane();

}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		// switch between solid and wireframe mode
		isPolygon = !isPolygon;
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		// switch the model
		cur_idx = (cur_idx >= 4) ? 0 : cur_idx+1;
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
		// switch the model
		cur_idx = (cur_idx <= 0) ? 4 : cur_idx-1;
	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		// switch to Orthogonal projection
		setOrthogonal();
		printf("[INFO] Orthogonal projection mode\n");

	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		// switch to NDC Perspective projection
		setPerspective();
		printf("[INFO] Perspective projection mode\n");

	}
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
		// switch to translation mode
		cur_trans_mode = GeoTranslation;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		// switch to scale mode
		cur_trans_mode = GeoScaling;
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		// switch to rotation mode
		cur_trans_mode = GeoRotation;
	if (key == GLFW_KEY_E && action == GLFW_PRESS)
		// switch to translate eye position mode
		cur_trans_mode = ViewEye;
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
		// switch to translate viewing center position mode
		cur_trans_mode = ViewCenter;
	if (key == GLFW_KEY_U && action == GLFW_PRESS)
		// switch to translate camera up vector position mode
		cur_trans_mode = ViewUp;
	if (key == GLFW_KEY_I && action == GLFW_PRESS)
	{
		// print information
		cout << "Translation Matrix:\n" << translate(models[cur_idx].position) << endl;
		cout << "Rotation Matrix:\n" << rotate(models[cur_idx].rotation) << endl;
		cout << "Scaling Matrix:\n" << scaling(models[cur_idx].scale) << endl;
		cout << "Viewing Matrix:\n" << view_matrix << endl;
		cout << "Projection Matrix:\n" << project_matrix << endl;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	if (yoffset < 0)
	{
		switch (cur_trans_mode) {
		case ViewEye:
			main_camera.position.z += 0.1;
			setViewingMatrix();
			break;
		case ViewCenter:
			main_camera.center.z -= 0.1;
			setViewingMatrix();
			break;
		case ViewUp:
			main_camera.up_vector.z -= 0.33;
			setViewingMatrix();
			break;
		case GeoTranslation:
			models[cur_idx].position.z -= 0.1;
			break;
		case GeoScaling:
			models[cur_idx].scale.z -= 0.01;
			break;
		case GeoRotation:
			models[cur_idx].rotation.z -= (PI / 180.0) * 3;
			break;
		default:
			break;
		}
	}
	else
	{
		switch (cur_trans_mode) {
		case ViewEye:
			main_camera.position.z -= 0.1;
			setViewingMatrix();
			break;
		case ViewCenter:
			main_camera.center.z += 0.1;
			setViewingMatrix();
			break;
		case ViewUp:
			main_camera.up_vector.z += 0.33;
			setViewingMatrix();
			break;
		case GeoTranslation:
			models[cur_idx].position.z += 0.1;
			break;
		case GeoScaling:
			models[cur_idx].scale.z += 0.01;
			break;
		case GeoRotation:
			models[cur_idx].rotation.z += (PI / 180.0) * 3;
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
		case ViewEye:
			main_camera.position.x += diff_x * 0.0025;
			main_camera.position.y += diff_y * 0.0025;
			setViewingMatrix();
			break;

		case ViewCenter:
			main_camera.center.x += diff_x * 0.0025;
			main_camera.center.y += diff_y * 0.0025;
			setViewingMatrix();
			break;

		case ViewUp:
			main_camera.up_vector.x += diff_x * 0.1;
			main_camera.up_vector.y += diff_y * 0.1;
			setViewingMatrix();
			break;

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

	iLocMVP = glGetUniformLocation(p, "mvp");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, tinyobj::shape_t* shape)
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
	vertices.reserve(shape->mesh.num_face_vertices.size() * 3);
	colors.reserve(shape->mesh.num_face_vertices.size() * 3);
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
		}
		index_offset += fv;
	}
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;

	string err;
	string warn;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());
	
	normalization(&attrib, vertices, colors, &shapes[0]);

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

	m_shape_list.push_back(tmp_shape);
	model tmp_model;
	models.push_back(tmp_model);


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	shapes.clear();
	materials.clear();
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
	proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../ColorModels/bunny5KC.obj", "../ColorModels/dragon10KC.obj", "../ColorModels/lucy25KC.obj", "../ColorModels/teapot4KC.obj", "../ColorModels/dolphinC.obj"};
	
	// [TODO] Load five model at here
	for (int i = 0; i < 5; i++) LoadModels(model_list[i]);
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
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "110065502 HW1", NULL, NULL);
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
