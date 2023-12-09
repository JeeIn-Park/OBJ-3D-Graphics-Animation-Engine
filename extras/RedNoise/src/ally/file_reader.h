#pragma once
#include "file_reader.h"

#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <Colour.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <fstream>

#include <sstream>
#include <unordered_map>
#include <thread>

struct TriangleInfo {
    std::array<int, 3> triangleIndices;
    Colour colour;

    TriangleInfo();
    TriangleInfo(std::array<int, 3> indices, Colour colour);
};

bool shareCommonVertex(const ModelTriangle& tri1, const ModelTriangle& tri2);
std::vector<int> findAdjacentFacets(const std::vector<ModelTriangle>& obj, int facetIndex);
glm::vec3 computeTriangleNormal(const ModelTriangle& triangle);
std::unordered_map<std::string, Colour> readMTL(const std::string &filename);
std::tuple<std::vector<ModelTriangle>, std::vector<TriangleInfo>, std::vector<glm::vec3>> readOBJ(const std::string &filename, std::unordered_map<std::string, Colour> colourMap, float s);
