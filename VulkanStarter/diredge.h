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
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> defaultPositions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> faceNormals;
		std::vector<glm::vec3> tempNormals;

        std::vector<uint32_t> faceVertices;
        std::vector<uint32_t> otherHalf;
        std::vector<uint32_t> firstDirectedEdge;
    };

    // Makes a half edge mesh data structure from a triangle soup.
    diredgeMesh createMesh(std::vector<glm::vec3>, std::vector<glm::vec3>, std::vector<uint32_t>);

    // Makes a triangle soup from the half edge mesh data structure.
    std::vector<glm::vec3> makeSoup(diredgeMesh);

    // Computes mesh.position and mesh.normal and mesh.faceVertices
    void makeFaceIndices(std::vector<glm::vec3> raw_vertices, diredgeMesh&);

    // Computes mesh.firstDirectedEdge and mesh.firstDirectedEdge, given mesh.position and mesh.normal and mesh.faceVertices
    void makeDirectedEdges(diredgeMesh&);

	void makeFaceNormals(diredgeMesh&);

	void restoreDefaultPositions(diredgeMesh&);
}
