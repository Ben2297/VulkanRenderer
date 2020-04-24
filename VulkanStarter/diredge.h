#pragma once

#include <string>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp> //includes the GLM library
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

// define a macro for "not used" flag
#define NO_SUCH_ELEMENT -1

// use macros for the "previous" and "next" IDs
#define PREVIOUS_EDGE(x) ((x) % 3) ? ((x) - 1) : ((x) + 2)
#define NEXT_EDGE(x) (((x) % 3) == 2) ? ((x) - 2) : ((x) + 1)

namespace diredge 
{
    struct diredgeMesh
    {
        std::vector<glm::vec3> normal;
        std::vector<glm::vec3> position;
		std::vector<glm::vec3> triangleNormal;

        std::vector<long> faceVertices;
        std::vector<long> otherHalf;
        std::vector<long> firstDirectedEdge;
    };

	struct Vertex 
	{
		glm::vec3 pos; //vertex position
		glm::vec3 color; //vertex color
		glm::vec2 texCoord; //texture coordinates
		glm::vec3 normal; //vertex normal
	};

    // Makes a half edge mesh data structure from a triangle soup.
    diredgeMesh createMesh(std::vector<Vertex>, std::vector<uint32_t>);

    // Makes a triangle soup from the half edge mesh data structure.
    std::vector<glm::vec3> makeSoup(diredgeMesh);

    // Computes mesh.position and mesh.normal and mesh.faceVertices
    void makeFaceIndices(std::vector<glm::vec3> raw_vertices, diredgeMesh&);

    // Computes mesh.firstDirectedEdge and mesh.firstDirectedEdge, given mesh.position and mesh.normal and mesh.faceVertices
    void makeDirectedEdges(diredgeMesh&);

}
