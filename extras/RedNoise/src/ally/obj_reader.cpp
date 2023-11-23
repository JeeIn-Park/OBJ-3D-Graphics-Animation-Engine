#include "Utils.h"
#include <fstream>
#include <vector>
#include "glm/glm.hpp"
#include "Colour.h"
#include "ModelTriangle.h"
#include <unordered_map>

// TODO : check if I can include this library
#include <sstream>

#include "obj_reader.h"


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
                colourMap[colourName] = Colour(255 * rgb[0], 255 * rgb[1], 255 * rgb[2]);
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
    std::vector<glm::vec3> vertices;
    int vertexSetSize = 0;
    std::vector<glm::vec3> textures;
    bool assignTexture = false;
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

        // TODO : use object name
        if (token == "o"){
            assignTexture = false;
            textures.clear();
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
//            std::cout << vertices.size() << std::endl;
//            std::cout << vertexSetSize << std::endl;
//            std::cout << texture.z << std::endl;
        }


            // f - face
        else if (token == "f") {
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
                vertexIndices[i] = std::stoi(vertex) -1;
            }

            // when all vertex indices are valid
            if (vertexIndices[0] >= 0 && vertexIndices[1] >= 0 && vertexIndices[2] >= 0) {
                ModelTriangle triangle;
                for (int i = 0; i < 3; ++i) {
                    triangle.vertices[i] = vertices[vertexIndices[i]];
                    if (assignTexture){
                        for (size_t j = 0; j < textures.size(); ++j){
                            if (textures[j].z == vertexIndices[i]){
                                triangle.texturePoints[i] = TexturePoint(textures[j].y, textures[j].x);
//                            std::cout << "texture assigned" << std::endl;
                            }
                        }
                    } else {
                        triangle.texturePoints[i] = TexturePoint(-1, -1);
                    }
                }
                triangle.colour = currentColour;
                triangles.push_back(triangle);
            }


//            int bookMark = 0;
//            int currentTextureBook = textures[0].z;
//            for (size_t i = 0; i < textures.size(); ++i) {
//                if (textures[i].z == currentTextureBook){
//
//                } else {
//                    bookMark = 0;
//                    currentTextureBook = textures[i].z;
//
//                    //TODO : make it possible to assign texture point if there are multiple textures as well
//                }
//            }
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
