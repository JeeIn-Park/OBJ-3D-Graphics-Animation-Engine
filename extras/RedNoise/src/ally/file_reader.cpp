#include "file_reader.h"

#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <Colour.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <fstream>

// TODO : check if I can include this library
#include <sstream>


// TODO : check if it's allowed to use this library
#include <unordered_map>

#define WIDTH 320
#define HEIGHT 240

struct TriangleInfo {
    std::array<int, 3> triangleIndices;
    Colour colour{};

    TriangleInfo();
    TriangleInfo(std::array<int, 3> indices, Colour col);

    explicit TriangleInfo(const std::array<int, 3> &triangleIndices);
};

TriangleInfo::TriangleInfo() = default;
TriangleInfo::TriangleInfo(std::array<int, 3> indices, Colour col) :
        triangleIndices(indices), colour(col) {}

TriangleInfo::TriangleInfo(const std::array<int, 3> &triangleIndices) : triangleIndices(triangleIndices) {}


std::unordered_map<std::string, Colour> readMTL (const std::string &filename) {
    std::unordered_map<std::string, Colour> colourMap;
    std::string line;
    std::ifstream mtlFile(filename);
    std::string colourName;

    // handle error : when file is not opened
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
//            std::cout << "RGB: " << rgb[0] << " " << rgb[1] << " " << rgb[2] << std::endl;

            if (rgb[0] >= 0 && rgb[1] >= 0 && rgb[2] >= 0) {
                colourMap[colourName] = Colour(colourName ,255 * rgb[0], 255 * rgb[1], 255 * rgb[2]);
            }

        }

        else if (token == "map_Kd") {
            // TODO : store what texture is used for this colour
            // TODO : study this code bit
            // Check if the key exists in the map
            auto it = colourMap.find(colourName);
            if (it != colourMap.end()) {
                // Get the value corresponding to the key
                Colour old_value = it->second;
                // Define new key and value
                std::string new_key;
                iss >> new_key;
                // Remove the old key-value pair
                colourMap.erase(it);
                // Update the map with the new key-value pair
                colourMap[new_key] = Colour(255, 255, 255);
            }
        }

    }

    mtlFile.close();
    return colourMap;
}


std::vector<ModelTriangle> readOBJ(const std::string &filename, std::unordered_map<std::string, Colour> colourMap, float s){
    std::vector<ModelTriangle> triangles;
    std::vector<TriangleInfo> triangleInfos;
    std::vector<glm::vec3> vertices;
    int vertexSetSize = 0;
    std::vector<glm::vec3> textures;
    bool assignTexture = false;
//    bool assignLight = false;
    Colour currentColour;

    std::string line;
    std::ifstream objFile(filename);
    float xMax=0, yMax=0, zMax=0, xMin=0, yMin=0, zMin=0;


    // handle error : when file is not opened
    if (!objFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return triangles;
    }

    while (getline(objFile, line)){
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "o"){
            assignTexture = false;
//            assignLight = false;
            textures.clear();
            std::string objectName;
            iss >> objectName;
            if (objectName == "light") {
//                assignLight = true;
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
//            if (assignLight) {
//                float x, y, z;
//                size_t verticesNumber = vertices.size();
//                for (size_t i = 0; i < verticesNumber; ++i) {
//                    x = x + vertices[i].x;
//                    y = y + vertices[i].y;
//                    z = z + vertices[i].z;
//                }
//                x = x / (2 * verticesNumber);
//                y = y / (2 * verticesNumber);
//                z = z / (2 * verticesNumber);
//              lightPosition = glm::vec3(x, y, z);
//            }

            vertexSetSize = 0;
            std::string vertex;
            std::array<int, 3> vertexIndices;

            // extract vertex indices and put it in vertexIndices
            for (int i = 0; i < 3; ++i) {
                iss >> vertex;
                size_t pos = vertex.find('/');
                if (pos != std::string::npos) { // TODO : npos study
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
    for (auto& triangleInfo : triangleInfos){
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
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        triangle.normal = glm::normalize(glm::cross(e0,e1));
        triangle.colour = triangleInfo.colour;
        triangles.push_back(triangle);
    }

//    std::cout << vertices.size() << std::endl;
    std::cout << triangles.size() << std::endl;

    objFile.close();
    return triangles;
}

