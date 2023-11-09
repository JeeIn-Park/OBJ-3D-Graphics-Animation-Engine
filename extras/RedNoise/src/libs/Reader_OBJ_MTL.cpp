#include "Utils.h"
#include <fstream>
#include <vector>
#include "glm/glm.hpp"
#include "Colour.h"
#include "ModelTriangle.h"
#include <unordered_map>
#include <sstream>

#include "Reader_OBJ_MTL.h"


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
        if (token == "Kd") {
            std::array<float, 3> rgb;
            iss >> rgb[0] >> rgb[1] >> rgb[2];
//            std::cout << "RGB: " << rgb[0] << " " << rgb[1] << " " << rgb[2] << std::endl;

            if (rgb[0] >= 0 && rgb[1] >= 0 && rgb[2] >= 0) {
                colourMap[colourName] = Colour(255 * rgb[0], 255 * rgb[1], 255 * rgb[2]);
            }

        }

    }

    mtlFile.close();
//    for (const auto& pair : colourMap) {
//        std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
//    }
    return colourMap;
}

std::vector<ModelTriangle> readOBJ(const std::string &filename, std::unordered_map<std::string, Colour> colourMap, float s){
    std::vector<ModelTriangle> triangles;
    std::vector<glm::vec3> vertices;
    Colour currentColour;

    std::string line;
    std::ifstream objFile(filename);

    // handle error : when file is not opened
    if (!objFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return triangles;
    }

    while (getline(objFile, line)){
        std::istringstream iss(line);
        std::string token;
        iss >> token;


        // usemtl - colour
        if (token == "usemtl") {
            std::string colourName;
            iss >> colourName;
            currentColour = colourMap[colourName];
        }

            // v - vertex
        else if (token == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(glm::vec3(s * vertex.x, s * vertex.y, s * vertex.z));
        }

            // f - face
        else if (token == "f") {
            std::string vertex;
            std::array<int, 3> vertexIndices;

            // extract vertex indices and put it in vertexIndices
            for (int i = 0; i < 3; ++i) {
                iss >> vertex;
                size_t pos = vertex.find('/');
                if (pos != std::string::npos) { // TODO : npos study
                    vertex = vertex.substr(0, pos);
                }
                vertexIndices[i] = std::stoi(vertex) -1;
            }

            // when all vertex indices are valid
            if (vertexIndices[0] >= 0 && vertexIndices[1] >= 0 && vertexIndices[2] >= 0) {
                ModelTriangle triangle;
                for (int i = 0; i < 3; ++i) {
                    triangle.vertices[i] = vertices[vertexIndices[i]];
                }
                triangle.colour = currentColour;
                triangles.push_back(triangle);
            }
        }

    }
    objFile.close();

//    for (const glm::vec3 & vertex : vertices) {
//        std::cout << vertex.x << "," << vertex.y << "," << vertex.z << std::endl;
//    }
//    for (const ModelTriangle& triangle : triangles) {
//        std::cout << triangle << std::endl;
//    }
    return triangles;
}

