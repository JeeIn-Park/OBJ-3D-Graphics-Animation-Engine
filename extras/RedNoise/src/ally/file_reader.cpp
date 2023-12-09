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

#define WIDTH 320
#define HEIGHT 240

TriangleInfo::TriangleInfo() = default;
TriangleInfo::TriangleInfo(std::array<int, 3> i,  Colour c) :
        triangleIndices(i), colour(c) {}


bool shareCommonVertex(const ModelTriangle& tri1, const ModelTriangle& tri2) {
    int commonVertices = 0;
    for (const auto& vertex1 : tri1.vertices) {
        for (const auto& vertex2 : tri2.vertices) {
            if (glm::all(glm::equal(vertex1, vertex2))) {
                ++commonVertices;
            }
        }
    }
    return commonVertices >= 2;
}


std::vector<int> findAdjacentFacets(const std::vector<ModelTriangle>& obj, int facetIndex) {
    std::vector<int> adjacentFacets;

    const ModelTriangle& targetFacet = obj[facetIndex];

    for (int i = 0; i < obj.size(); ++i) {
        if (i != facetIndex) {
            if (shareCommonVertex(targetFacet, obj[i])) {
                adjacentFacets.push_back(i);
            }
        }
    }

    return adjacentFacets;
}


glm::vec3 computeTriangleNormal(const ModelTriangle& triangle) {
    glm::vec3 edge1 = triangle.vertices[1] - triangle.vertices[0];
    glm::vec3 edge2 = triangle.vertices[2] - triangle.vertices[0];
    return glm::normalize(glm::cross(edge1, edge2));
}



std::unordered_map<std::string, Colour> readMTL (const std::string &filename) {
    std::unordered_map<std::string, Colour> colourMap;
    std::string line;
    std::ifstream mtlFile(filename);
    std::string colourName;

    if (!mtlFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return colourMap;
    }

    while (getline(mtlFile, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        // newmtl - new colour
        if (token == "newmtl") {
            iss >> colourName;
//            std::cout << "New Material: " << colourName << std::endl;
        }

            // Kd - RGB value
        else if (token == "Kd") {
            std::array<float, 3> rgb;
            iss >> rgb[0] >> rgb[1] >> rgb[2];
            if (rgb[0] >= 0 && rgb[1] >= 0 && rgb[2] >= 0) {
                colourMap[colourName] = Colour(colourName ,255 * rgb[0], 255 * rgb[1], 255 * rgb[2]);
            }

        }

        else if (token == "map_Kd") {
            auto it = colourMap.find(colourName);
            if (it != colourMap.end()) {
                Colour old_value = it->second;
                std::string new_key;
                iss >> new_key;
                colourMap.erase(it);
                colourMap[new_key] = Colour(255, 255, 255);
            }
        }

    }
    mtlFile.close();
    return colourMap;
}


std::tuple<std::vector<ModelTriangle>, std::vector<TriangleInfo>, std::vector<glm::vec3>>
        readOBJ(const std::string &filename, std::unordered_map<std::string, Colour> colourMap, float s){

    std::vector<ModelTriangle> triangles;
    std::vector<TriangleInfo> triangleInfos;
    std::vector<glm::vec3> vertices;
    int vertexSetSize = 0;
    std::vector<glm::vec3> textures;
    bool assignTexture = false;
    Colour currentColour;

    std::string line;
    std::ifstream objFile(filename);
    float xMax=0, yMax=0, zMax=0, xMin=0, yMin=0, zMin=0;

    if (!objFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return  std::tuple<std::vector<ModelTriangle>, std::vector<TriangleInfo>, std::vector<glm::vec3>>();
    }

    while (getline(objFile, line)){
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "o"){
            assignTexture = false;
            textures.clear();
            std::string objectName;
            iss >> objectName;
            if (objectName == "light") {
            }
        }

            // usemtl - colour
        else if (token == "usemtl") {
            std::string colourName;
            iss >> colourName;
            currentColour = colourMap[colourName];
        }

            // v - vertex
        else if (token == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            xMax = std::max(xMax, vertex.x); yMax = std::max(yMax, vertex.y); zMax = std::max(zMax, vertex.z);
            xMin = std::min(xMin, vertex.x); yMin = std::min(yMin, vertex.y); zMin = std::min(zMin, vertex.z);

            vertices.push_back(glm::vec3(s * vertex.x, s * vertex.y, s * vertex.z));
            vertexSetSize = vertexSetSize + 1;
        }

            // vt - texture
        else if (token == "vt") {
            glm::vec3 texture;
            iss >> texture.x >> texture.y;
            texture.z = vertices.size() - (vertexSetSize-1) -1;
            textures.push_back(texture);
            vertexSetSize = vertexSetSize - 1;
            assignTexture = true;
        }

            // f - face
        else if (token == "f") {
            vertexSetSize = 0;
            std::string vertex;
            std::array<int, 3> vertexIndices;

            for (int i = 0; i < 3; ++i) {
                iss >> vertex;
                size_t pos = vertex.find('/');
                if (pos != std::string::npos) {
                    vertex = vertex.substr(0, pos);
                }
                vertexIndices[i] = std::stoi(vertex) - 1;
            }
            triangleInfos.push_back(TriangleInfo(vertexIndices, currentColour));
        }

    }
    float xMid = (xMax + xMin)/2, yMid = (yMax + yMin)/2, zMid = (zMax + zMin)/2;
    for (auto& vertex : vertices){
        vertex.x -= xMid; vertex.y -= yMid; vertex.z -= zMid;
    }
    ModelTriangle triangle;
    for (auto& triangleInfo : triangleInfos) {
        for (int i = 0; i < 3; ++i) {
            triangle.vertices[i] = vertices[triangleInfo.triangleIndices[i]];
            if (assignTexture) {
                for (size_t j = 0; j < textures.size(); ++j) {
                    if (textures[j].z == triangleInfo.triangleIndices[i]) {
                        triangle.texturePoints[i] = TexturePoint(textures[j].y, textures[j].x);
                    }
                }
            } else {
                triangle.texturePoints[i] = TexturePoint(-1, -1);
            }
        }
        triangle.colour = triangleInfo.colour;
        triangle.normal = computeTriangleNormal(triangle);
        triangles.push_back(triangle);
    }
    std::cout << vertices.size() << std::endl;
    std::cout << triangles.size() << std::endl;
    int vertexSize = static_cast<int>(vertices.size());
    std::vector<std::vector<glm::vec3>> tempVertexNormal(vertexSize);
    std::vector<glm::vec3> vertexNormal(vertexSize);

    for (size_t i = 0; i < triangles.size(); ++i) {
        glm::vec3 faceNormal = triangles[i].normal;
        for (int k= 0; k < 3; ++ k) {
            tempVertexNormal[triangleInfos[i].triangleIndices[k]].push_back(faceNormal);
        }
    }

    for (size_t i = 0; i < tempVertexNormal.size(); ++i) {
        glm::vec3 normalSum;
        size_t numberOfNormals = tempVertexNormal[i].size();
        for (size_t k = 0; k < numberOfNormals; ++k){
            normalSum += tempVertexNormal[i][k];
        }
        vertexNormal[i] = normalSum/(static_cast<float>(numberOfNormals));
//        std::cout << vertexNormal[i].x << "," << vertexNormal[i].y << "," << vertexNormal[i].z << std::endl;
    }

    objFile.close();
    return  std::tuple<std::vector<ModelTriangle>, std::vector<TriangleInfo>, std::vector<glm::vec3>>(triangles, triangleInfos, vertexNormal);
}

