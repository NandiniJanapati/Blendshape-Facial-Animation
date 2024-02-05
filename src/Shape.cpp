#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "Shape.h"
#include "GLSL.h"
#include "Program.h"

using namespace std;
using namespace glm;

Shape::Shape() :
	prog(NULL),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
}

Shape::~Shape()
{
}

void Shape::loadObj(const string &filename, vector<float> &pos, vector<float> &nor, vector<float> &tex, bool loadNor, bool loadTex)
{
	
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
	if(!warn.empty()) {
		//std::cout << warn << std::endl;
	}
	if(!err.empty()) {
		std::cerr << err << std::endl;
	}
	if(!ret) {
		return;
	}
	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			// Loop over vertices in the face.
			for(size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				pos.push_back(attrib.vertices[3*idx.vertex_index+0]);
				pos.push_back(attrib.vertices[3*idx.vertex_index+1]);
				pos.push_back(attrib.vertices[3*idx.vertex_index+2]);
				if(!attrib.normals.empty() && loadNor) {
					nor.push_back(attrib.normals[3*idx.normal_index+0]);
					nor.push_back(attrib.normals[3*idx.normal_index+1]);
					nor.push_back(attrib.normals[3*idx.normal_index+2]);
				}
				if(!attrib.texcoords.empty() && loadTex) {
					tex.push_back(attrib.texcoords[2*idx.texcoord_index+0]);
					tex.push_back(attrib.texcoords[2*idx.texcoord_index+1]);
				}
			}
			index_offset += fv;
		}
	}
}

void Shape::loadMesh(const string &meshName)
{
	// Load geometry
	meshFilename = meshName;
	loadObj(meshFilename, posBuf, norBuf, texBuf);
}

void Shape::init()
{
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
	
	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void Shape::draw() const
{
	assert(prog);

	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	// Draw
	int count = (int)posBuf.size()/3; // number of indices to be rendered
	glDrawArrays(GL_TRIANGLES, 0, count);
	
	glDisableVertexAttribArray(h_tex);
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

//void Shape::drawWithBlendShapes(vector<shared_ptr<Shape>>& blendshapes) const
void Shape::drawWithBlendShapes(vector<shared_ptr<Shape>>& blendshapes) const
{
	assert(prog); //but this prog should be prog2 for the head

	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int h_tex = prog->getAttribute("aTex");
	glEnableVertexAttribArray(h_tex);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	//replace with a for loop to loop through 3 blendshapes
	int attribute_index_for_delta_xa = prog->getAttribute("delta_xa"); //we can also get the string "delta_xa" by calling blendshapes[0]->delta_x (or this should work)
	glEnableVertexAttribArray(attribute_index_for_delta_xa);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[0]->posBufID);
	glVertexAttribPointer(attribute_index_for_delta_xa, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int attribute_index_for_delta_na = prog->getAttribute("delta_na"); 
	glEnableVertexAttribArray(attribute_index_for_delta_na);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[0]->norBufID);
	glVertexAttribPointer(attribute_index_for_delta_na, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int attribute_index_for_delta_xb = prog->getAttribute("delta_xb");
	glEnableVertexAttribArray(attribute_index_for_delta_xb);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[1]->posBufID);
	glVertexAttribPointer(attribute_index_for_delta_xb, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int attribute_index_for_delta_nb = prog->getAttribute("delta_nb");
	glEnableVertexAttribArray(attribute_index_for_delta_nb);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[1]->norBufID);
	glVertexAttribPointer(attribute_index_for_delta_nb, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int attribute_index_for_delta_xc = prog->getAttribute("delta_xc");
	glEnableVertexAttribArray(attribute_index_for_delta_xc);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[2]->posBufID);
	glVertexAttribPointer(attribute_index_for_delta_xc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int attribute_index_for_delta_nc = prog->getAttribute("delta_nc");
	glEnableVertexAttribArray(attribute_index_for_delta_nc);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[2]->norBufID);
	glVertexAttribPointer(attribute_index_for_delta_nc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	/*int attribute_index_for_delta_xc = prog->getAttribute("delta_xc");
	glEnableVertexAttribArray(attribute_index_for_delta_xc);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[2]->posBufID);
	glVertexAttribPointer(attribute_index_for_delta_xc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	int attribute_index_for_delta_nc = prog->getAttribute("delta_nc");
	glEnableVertexAttribArray(attribute_index_for_delta_nc);
	glBindBuffer(GL_ARRAY_BUFFER, blendshapes[2]->norBufID);
	glVertexAttribPointer(attribute_index_for_delta_nc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);*/

	/*for (int i = 0; i < blendshapes.size(); i++) {
		int attribute_index_for_delta_x = prog->getAttribute(blendshapes[i]->delta_x);
		glEnableVertexAttribArray(attribute_index_for_delta_x);
		glBindBuffer(GL_ARRAY_BUFFER, blendshapes[i]->posBufID);
		glVertexAttribPointer(attribute_index_for_delta_x, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

		int attribute_index_for_delta_n = prog->getAttribute(blendshapes[i]->delta_n);
		glEnableVertexAttribArray(attribute_index_for_delta_n);
		glBindBuffer(GL_ARRAY_BUFFER, blendshapes[i]->norBufID);
		glVertexAttribPointer(attribute_index_for_delta_n, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);
	}*/
	


	// Draw
	int count = (int)posBuf.size() / 3; // number of indices to be rendered
	glDrawArrays(GL_TRIANGLES, 0, count);

	glDisableVertexAttribArray(h_tex);
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glDisableVertexAttribArray(attribute_index_for_delta_xa);
	glDisableVertexAttribArray(attribute_index_for_delta_na);
	glDisableVertexAttribArray(attribute_index_for_delta_xb);
	glDisableVertexAttribArray(attribute_index_for_delta_nb);
	glDisableVertexAttribArray(attribute_index_for_delta_xc);
	glDisableVertexAttribArray(attribute_index_for_delta_nc);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);

}


void Shape::calcDeltas(const std::vector<float>& positionBuffer, const std::vector<float>& normalBuffer) {
	for (int i = 0; i < positionBuffer.size(); i++) {
		float xa = posBuf[i];
		float x0 = positionBuffer[i];
		float deltax = xa - x0;
		posBuf[i] = deltax;
	}

	for (int i = 0; i < normalBuffer.size(); i++) {
		float na = norBuf[i];
		float n0 = normalBuffer[i];
		float deltan = na - n0;
		norBuf[i] = deltan;
	}
}
