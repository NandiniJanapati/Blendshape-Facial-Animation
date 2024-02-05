#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLSL.h"
#include "Program.h"
#include "Camera.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "Texture.h"

using namespace std;

// Stores information in data/input.txt
class DataInput
{
public:
	vector<string> textureData;
	vector< vector<string> > meshData;
	vector< vector<string>> blendData;
};

DataInput dataInput;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the shaders are loaded from
string DATA_DIR = ""; // where the data are loaded from
bool keyToggles[256] = {false};

shared_ptr<Camera> camera = NULL;
vector< shared_ptr<Shape> > shapes;
map< string, shared_ptr<Texture> > textureMap;
shared_ptr<Program> prog = NULL;
shared_ptr<Program> prog2 = NULL;
double t, t0;

shared_ptr<Shape> baseHead;
vector<shared_ptr<Shape>> blendshapes; //the ones we'll use in the final blend
vector<shared_ptr<Shape>> blends; //all blend shapes in the input file
bool FACs = false; //is there an emotion line in input.txt
vector<vector<int>> emotion; //the facs numbers i need to combine
int curremotion = 0;

static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void switchEmotion() {
	curremotion++;
	if (curremotion >= emotion.size()) {
		curremotion = 0;
	}

	//getting the shapes from the blends list to put into blendshapes
	blendshapes.clear();
	for (int j = 0; j < emotion[curremotion].size(); j++) { //for each FAC number needed in the first emotion
		for (int i = 0; i < blends.size(); i++) { //search blends
			if (emotion[curremotion][j] == blends[i]->FACSencoding) { //if the blend's FAC number matches
				blendshapes.push_back(blends[i]); //add it to blendshapes
				break;
			}
		}
	}

	int missing_blendshapes = 3 - blendshapes.size();
	for (int i = 0; i < missing_blendshapes; i++) {
		auto blendshape = make_shared<Shape>();
		blendshapes.push_back(blendshape);
		blendshape->loadMesh(baseHead->getMeshFilename());
		blendshape->setTextureFilename(baseHead->getTextureFilename());
		blendshape->calcDeltas(baseHead->posBuf, baseHead->norBuf);
		blendshape->init();
	}

	int numblendshapes = blendshapes.size();
	for (int i = 0; i < numblendshapes; i++) {
		if (i == 0) {
			blendshapes[0]->delta_x = "delta_xa";
			blendshapes[0]->delta_n = "delta_na";
		}
		else if (i == 1) {
			blendshapes[1]->delta_x = "delta_xb";
			blendshapes[1]->delta_n = "delta_nb";
		}
		else if (i == 2) {
			blendshapes[2]->delta_x = "delta_xc";
			blendshapes[2]->delta_n = "delta_nc";
		}
	}

}

static void char_callback(GLFWwindow *window, unsigned int key)
{
	keyToggles[key] = !keyToggles[key];
	switch(key) {

		case 's':
			switchEmotion();
			break;
	}
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if(state == GLFW_PRESS) {
		camera->mouseMoved(xmouse, ymouse);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Get the current mouse position.
	double xmouse, ymouse;
	glfwGetCursorPos(window, &xmouse, &ymouse);
	// Get current window size.
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(action == GLFW_PRESS) {
		bool shift = mods & GLFW_MOD_SHIFT;
		bool ctrl  = mods & GLFW_MOD_CONTROL;
		bool alt   = mods & GLFW_MOD_ALT;
		camera->mouseClicked(xmouse, ymouse, shift, ctrl, alt);
	}
}



void init()
{
	keyToggles[(unsigned)'c'] = true;
	
	camera = make_shared<Camera>();
	
	// Create shapes
	for(const auto &mesh : dataInput.meshData) {
		auto shape = make_shared<Shape>();
		shapes.push_back(shape);
		shape->loadMesh(DATA_DIR + mesh[0]);
		shape->setTextureFilename(mesh[1]);
		if ((mesh[0]).compare("Victor_headGEO.obj") == 0) {
			baseHead = shape;
		}
	}

	for (const auto& mesh : dataInput.blendData) { //add all DELTA detected to blends
		auto blendshape = make_shared<Shape>();
		blends.push_back(blendshape);
		blendshape->loadMesh(DATA_DIR + mesh[1]);
		blendshape->setTextureFilename(baseHead->getTextureFilename());
		blendshape->FACSencoding = stoi(mesh[2]);
	}
	
	// GLSL programs
	prog = make_shared<Program>();
	prog->setShaderNames(RESOURCE_DIR + "phong_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
	prog->setVerbose(true);

	prog2 = make_shared<Program>();
	prog2->setShaderNames(RESOURCE_DIR + "blend_vert.glsl", RESOURCE_DIR + "phong_frag.glsl");
	prog2->setVerbose(true);
	
	// Set background color
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// Enable z-buffer test
	glEnable(GL_DEPTH_TEST);
	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	for(auto shape : shapes) {
		shape->init();
	}
	for (auto shape : blends) { //init all DELTAs detected
		shape->calcDeltas(baseHead->posBuf, baseHead->norBuf);
		shape->init();
	}

	if (FACs) {// if we are trying to make a specific emotion
		for (int j = 0; j < emotion[0].size(); j++) { //for each FAC number needed in the first emotion
			for (int i = 0; i < blends.size(); i++) { //search blends
				if (emotion[0][j] == blends[i]->FACSencoding) { //if the blend's FAC number matches
					blendshapes.push_back(blends[i]); //add it to blendshapes
					break;
				}
			}
		}
	}
	else {
		for (int i = 0; i < blends.size(); i++) {
			if (i == 3) { //once we reach i == 3, that's too much
				break;
			}
			else {
				blendshapes.push_back(blends[i]); //just take the first 3 or less deltas found
			}
		}
	}
	
	//if there are less than 3 deltas in blendshapes, fill in the remaining with 0s
	int missing_blendshapes = 3 - blendshapes.size();
	for (int i = 0; i < missing_blendshapes; i++) {
		auto blendshape = make_shared<Shape>();
		blendshapes.push_back(blendshape);
		blendshape->loadMesh(baseHead->getMeshFilename());
		blendshape->setTextureFilename(baseHead->getTextureFilename());
		blendshape->calcDeltas(baseHead->posBuf, baseHead->norBuf);
		blendshape->init();
	}

	//blendshapes[0]->delta_x = "delta_xa";
	//blendshapes[0]->delta_n = "delta_na";

	int numblendshapes = blendshapes.size();
	for (int i = 0; i < numblendshapes; i++) {
		if (i == 0) {
			blendshapes[0]->delta_x = "delta_xa";
			blendshapes[0]->delta_n = "delta_na";
		}
		else if (i == 1) {
			blendshapes[1]->delta_x = "delta_xb";
			blendshapes[1]->delta_n = "delta_nb";
		}
		else if (i == 2) {
			blendshapes[2]->delta_x = "delta_xc";
			blendshapes[2]->delta_n = "delta_nc";
		}
	}
	
	prog->init();
	prog->addAttribute("aPos");
	prog->addAttribute("aNor");
	prog->addAttribute("aTex");
	prog->addUniform("P");
	prog->addUniform("MV");
	prog->addUniform("ka");
	prog->addUniform("ks");
	prog->addUniform("s");
	prog->addUniform("kdTex");
	
	// Bind the texture to unit 1.
	int unit = 1;
	prog->bind();
	glUniform1i(prog->getUniform("kdTex"), unit);
	prog->unbind();

	prog2->init();
	prog2->addAttribute("aPos");
	prog2->addAttribute("aNor");
	prog2->addAttribute("aTex");
	prog2->addUniform("P");
	prog2->addUniform("MV");
	prog2->addUniform("ka");
	prog2->addUniform("ks");
	prog2->addUniform("s");
	prog2->addUniform("kdTex");
	prog2->addAttribute("delta_xa");
	prog2->addAttribute("delta_na");
	prog2->addAttribute("delta_xb");
	prog2->addAttribute("delta_nb");
	prog2->addAttribute("delta_xc");
	prog2->addAttribute("delta_nc");
	prog2->addUniform("a_t");
	prog2->addUniform("b_t");
	prog2->addUniform("c_t");

	prog2->bind();
	glUniform1i(prog2->getUniform("kdTex"), unit);
	prog2->unbind();


	/*
	each blendshape is a Shape object. When we called blendshape->init(), 
	it created the buffers (on the GPU) for the positions (pointed to by PosBufID) 
	and normal (pointed to by NorBufID) and filled those buffers in with the data in
	posBuf and norBuf (but in this case, posBuf is delta_x and delta_n).
	
	Before drawing the head, we need to do extra steps to make sure that we can reach the posBufID and 
	norBufID (for calling glBindBuffer) of each blendshape because we need to assign thier buffers to 
	the extra attributes delta_xa, delta_na, (etc. when needed) in the blend_vert program.
	*/

	
	for(const auto &filename : dataInput.textureData) {
		auto textureKd = make_shared<Texture>();
		textureMap[filename] = textureKd;
		textureKd->setFilename(DATA_DIR + filename);
		textureKd->init();
		textureKd->setUnit(unit); // Bind to unit 1
		textureKd->setWrapModes(GL_REPEAT, GL_REPEAT);
	}

	// Initialize time.
	glfwSetTime(0.0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void render()
{
	// Update time.
	double t1 = glfwGetTime();
	float dt = (t1 - t0);
	if(keyToggles[(unsigned)' ']) {
		t += dt;
	}
	t0 = t1;

	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	// Use the window size for camera.
	glfwGetWindowSize(window, &width, &height);
	camera->setAspect((float)width/(float)height);
	
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(keyToggles[(unsigned)'c']) {
		glEnable(GL_CULL_FACE);
	} else {
		glDisable(GL_CULL_FACE);
	}
	if(keyToggles[(unsigned)'z']) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	
	auto P = make_shared<MatrixStack>();
	auto MV = make_shared<MatrixStack>();
	
	// Apply camera transforms
	P->pushMatrix();
	camera->applyProjectionMatrix(P);
	MV->pushMatrix();
	camera->applyViewMatrix(MV);
	
	// Apply global transform
	MV->pushMatrix();
	MV->translate(0.0, -18.5, 0.0);
	
	// Draw shapes
	prog->bind();
	for(const auto &shape : shapes) {
		MV->pushMatrix();
		textureMap[shape->getTextureFilename()]->bind(prog->getUniform("kdTex"));
		glLineWidth(1.0f); // for wireframe
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
		glUniform3f(prog->getUniform("ka"), 0.1f, 0.1f, 0.1f);
		glUniform3f(prog->getUniform("ks"), 0.1f, 0.1f, 0.1f);
		glUniform1f(prog->getUniform("s"), 200.0f);
		shape->setProgram(prog);
		if (shape != baseHead) { //draw all the meshes besides the head
			shape->draw();
		}
		MV->popMatrix();
	}
	prog->unbind();

	//drawing the head
	prog2->bind();
	MV->pushMatrix();
	textureMap[baseHead->getTextureFilename()]->bind(prog2->getUniform("kdTex"));
	glLineWidth(1.0f); // for wireframe
	glUniformMatrix4fv(prog2->getUniform("P"), 1, GL_FALSE, glm::value_ptr(P->topMatrix()));
	glUniformMatrix4fv(prog2->getUniform("MV"), 1, GL_FALSE, glm::value_ptr(MV->topMatrix()));
	glUniform3f(prog2->getUniform("ka"), 0.1f, 0.1f, 0.1f);
	glUniform3f(prog2->getUniform("ks"), 0.1f, 0.1f, 0.1f);
	glUniform1f(prog2->getUniform("s"), 200.0f);
	float wieghta_t = (0.5 * cos(t - 3.141592) + 0.5);
	float wieghtb_t = (0.5 * cos(t - 3.141592) + 0.5);
	float wieghtc_t = (0.5 * cos(t - 3.141592) + 0.5);
	glUniform1f(prog2->getUniform("a_t"), wieghta_t);
	glUniform1f(prog2->getUniform("b_t"), wieghtb_t);
	glUniform1f(prog2->getUniform("c_t"), wieghtc_t);
	baseHead->setProgram(prog2);
	//draw all the meshes besides the head
	baseHead->drawWithBlendShapes(blendshapes);
	MV->popMatrix();

	prog2->unbind();
	
	// Undo global transform
	MV->popMatrix();

	// Pop matrix stacks.
	MV->popMatrix();
	P->popMatrix();

	GLSL::checkError(GET_FILE_LINE);
}

void loadDataInputFile()
{
	string filename = DATA_DIR + "input.txt";
	ifstream in;
	in.open(filename);
	if(!in.good()) {
		cout << "Cannot read " << filename << endl;
		return;
	}
	cout << "Loading " << filename << endl;
	
	string line;
	while(1) {
		getline(in, line);
		if(in.eof()) {
			break;
		}
		if(line.empty()) {
			continue;
		}
		// Skip comments
		if(line.at(0) == '#') {
			continue;
		}
		// Parse lines
		string key, value;
		stringstream ss(line);
		// key
		ss >> key;
		if(key.compare("TEXTURE") == 0) {
			ss >> value;
			dataInput.textureData.push_back(value);
		} 
		else if(key.compare("MESH") == 0) {
			vector<string> mesh;
			ss >> value;
			mesh.push_back(value); // obj
			ss >> value;
			mesh.push_back(value); // texture
			dataInput.meshData.push_back(mesh);
		} 
		else if (key.compare("DELTA") == 0) {
			vector<string> blend;
			string FACSnumber;
			ss >> value; // the action unit number (idk skip for now)
			FACSnumber = value;
			ss >> value;
			blend.push_back(value); //get base mesh first
			ss >> value;
			blend.push_back(value); //the blendspace obj
			blend.push_back(FACSnumber); //the facs encoding number
			dataInput.blendData.push_back(blend);
		}
		else if (key.compare("EMOTION") == 0) {
			FACs = true;
			vector<int> FAC_for_emotion;
			ss >> value; // the name of the emotion
			while (ss) {
				ss >> value; //the FACS number 
				FAC_for_emotion.push_back(stoi(value));
			}
			FAC_for_emotion.pop_back();
			if (FAC_for_emotion.size() > 3) {
				FAC_for_emotion.resize(3);
			}
			emotion.push_back(FAC_for_emotion);
		}
		else {
			cout << "Unkown key word: " << key << endl;
		}
	}
	in.close();
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		cout << "Usage: A3 <SHADER DIR> <DATA DIR>" << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");
	DATA_DIR = argv[2] + string("/");
	loadDataInputFile();
	
	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(640, 480, "Nandini Janapati", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	// Set char callback.
	glfwSetCharCallback(window, char_callback);
	// Set cursor position callback.
	glfwSetCursorPosCallback(window, cursor_position_callback);
	// Set mouse button callback.
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// Initialize scene.
	init();
	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		if(!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
			// Render scene.
			render();
			// Swap front and back buffers.
			glfwSwapBuffers(window);
		}
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
