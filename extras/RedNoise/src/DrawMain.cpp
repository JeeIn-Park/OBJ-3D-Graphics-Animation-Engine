#include <DrawingWindow.h>
#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>
#include <fstream>

// TODO : check if I can include this library
#include <sstream>

#include "ally/raster.h"
#include "ally/camera_move.h"

// TODO : check if it's allowed to use this library
#include <unordered_map>
#include <thread>

#define WIDTH 320
#define HEIGHT 240

glm::vec3 lightPosition;

std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
    float gap;
    int numberOfGap = numberOfValues - 1;
    gap = (to - from)/numberOfGap;

    std::vector<float> result;
    result.push_back(from);
    float x = from;
    for (int i = 0; i < (numberOfGap-1); ++i) {
        x = x + gap;
        result.push_back(x);
    }
    result.push_back(to);
    //use to as last value, unrepresented gap can happen between two last values
    return result;
}



std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues){
    std::vector<float> x_list = interpolateSingleFloats(from.x, to.x, numberOfValues);
    std::vector<float> y_list = interpolateSingleFloats(from.y, to.y, numberOfValues);
    std::vector<float> z_list = interpolateSingleFloats(from.z, to.z, numberOfValues);

    glm::vec3 vec;
    std::vector<glm::vec3> result;
    for(int i = 0; i < numberOfValues; i ++){
        vec.x = x_list[i];
        vec.y = y_list[i];
        vec.z = z_list[i];
        result.push_back(vec);
    }

    //use calculated last value as to if the gap cannot be represented by float
    return result;
}



CanvasTriangle randomTriangle() {
    CanvasTriangle triangle;

    while (true) {
        triangle.v0().x = rand() % WIDTH;
        triangle.v0().y = rand() % HEIGHT;

        triangle.v1().x = rand() % WIDTH;
        triangle.v1().y = rand() % HEIGHT;

        triangle.v2().x = rand() % WIDTH;
        triangle.v2().y = rand() % HEIGHT;

        // Calculate the cross product of vectors (v0, v1) and (v0, v2)
        int crossProduct = (triangle.v1().x - triangle.v0().x) * (triangle.v2().y - triangle.v0().y) -
                           (triangle.v1().y - triangle.v0().y) * (triangle.v2().x - triangle.v0().x);

        // Check if the cross product is non-zero (not collinear)
        if (crossProduct != 0) {
            return triangle;
        }
    }
}



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
    bool assignLight = false;
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

        if (token == "o"){
            assignTexture = false;
            assignLight = false;
            textures.clear();
            std::string objectName;
            iss >> objectName;
            if (objectName == "light") {
                assignLight = true;
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
            if (assignLight) {
                float x, y, z;
                size_t verticesNumber = vertices.size();
                for (size_t i = 0; i < verticesNumber; ++i) {
                    x = x + vertices[i].x;
                    y = y + vertices[i].y;
                    z = z + vertices[i].z;
                }
                x = x / (2 * verticesNumber);
                y = y / (2 * verticesNumber);
                z = z / (2 * verticesNumber);
                lightPosition = glm::vec3(x, y, z);
            }

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

                // when all vertex indices are valid
            if (vertexIndices[0] >= 0 && vertexIndices[1] >= 0 && vertexIndices[2] >= 0) {
                ModelTriangle triangle;
                for (int i = 0; i < 3; ++i) {
                    triangle.vertices[i] = vertices[vertexIndices[i]];
                    if (assignTexture) {
                        for (size_t j = 0; j < textures.size(); ++j) {
                            if (textures[j].z == vertexIndices[i]) {
                                triangle.texturePoints[i] = TexturePoint(textures[j].y, textures[j].x);
                            }
                        }
                    } else {
                        triangle.texturePoints[i] = TexturePoint(-1, -1);
                    }
                }
                triangle.colour = currentColour;
                triangles.push_back(triangle);

            }
        }

    }
    objFile.close();
    return triangles;
}



/**
   *  @param  obj  : list of object facets
  */
RayTriangleIntersection getClosestValidIntersection(glm::vec3 rayStartingPoint, glm::vec3 rayDirection, std::vector<ModelTriangle> obj, bool shadow) {
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = std::numeric_limits<float>::infinity();

    for (size_t i = 0; i < obj.size(); ++i) {
        ModelTriangle triangle = obj[i];
        glm::vec3 SPVector = rayStartingPoint - triangle.vertices[0];
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        if (shadow && possibleSolution.x < 0.001f ) continue;

        if (possibleSolution.x > 0 && possibleSolution.y >= 0 && possibleSolution.z >= 0
            && possibleSolution.x <= closestIntersection.distanceFromCamera
            && possibleSolution.y <= 1 && possibleSolution.z <= 1
            && possibleSolution.y + possibleSolution.z <= 1)  {

            glm::vec3 intersection = rayStartingPoint + possibleSolution.x * rayDirection;
            closestIntersection = RayTriangleIntersection(intersection, possibleSolution.x, triangle, i);
        }
    }
    return closestIntersection;
}


Colour checkShadow(glm::vec3 intersection, ModelTriangle triangle, std::vector<ModelTriangle> obj){
    Colour colour = triangle.colour;
    glm::vec3 light = glm::normalize( lightPosition - intersection);
    float lightDistance = glm::length(lightPosition - intersection);
    RayTriangleIntersection shadowIntersection = getClosestValidIntersection( intersection, light, obj, true);

    if (shadowIntersection.distanceFromCamera <= lightDistance){
        //std::cout << "Found shadow point" << std::endl;
        colour = Colour(0,0,0);
    }
    return colour;
}


void drawRayTracedScene(DrawingWindow &window, glm::vec3 c, glm::mat3 o, float f, std::vector<ModelTriangle> obj){
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            glm::vec3 rayDirection = glm::vec3 (-WIDTH/2 + x, HEIGHT/2 - y, -f*HEIGHT/2);
            rayDirection = glm::normalize(rayDirection - c);
            RayTriangleIntersection intersection = getClosestValidIntersection(c, rayDirection, obj, false);

            if (intersection.distanceFromCamera < std::numeric_limits<float>::infinity()) {
                ModelTriangle triangle = intersection.intersectedTriangle;
                Colour colour = checkShadow(intersection.intersectionPoint, triangle, obj);
                window.setPixelColour(x, y, colour);
            }
        }
    }
}


bool handleEvent(SDL_Event event, DrawingWindow &window, glm::vec3* c, glm::mat3* o, float** &d, bool* &p) {
    float translate = 0.07;

    Colour colour(rand() % 256, rand() % 256, rand() % 256);
    if (event.type == SDL_KEYDOWN) {
        // translate
        if (event.key.keysym.sym == SDLK_LEFT) {(*c).x =  (*c).x - translate;}
        else if (event.key.keysym.sym == SDLK_RIGHT) {(*c).x =  (*c).x + translate;}
        else if (event.key.keysym.sym == SDLK_UP) {(*c).y =  (*c).y + translate;}
        else if (event.key.keysym.sym == SDLK_DOWN) {(*c).y =  (*c).y - translate;}
        else if (event.key.keysym.sym == SDLK_KP_MINUS) {(*c).z =  (*c).z + translate;}
        else if (event.key.keysym.sym == SDLK_KP_PLUS) {(*c).z =  (*c).z - translate;}

        //rotate
        else if (event.key.keysym.sym == SDLK_a) rotate(c,'a');
        else if (event.key.keysym.sym == SDLK_d) rotate(c,'d');
        else if (event.key.keysym.sym == SDLK_w) rotate(c,'w');
        else if (event.key.keysym.sym == SDLK_s) rotate(c,'s');

        //orientation
        else if (event.key.keysym.sym == SDLK_KP_1) { orientRotate(o, '1');}
        else if (event.key.keysym.sym == SDLK_KP_3) { orientRotate(o, '3');}
        else if (event.key.keysym.sym == SDLK_KP_5) { orientRotate(o, '5');}
        else if (event.key.keysym.sym == SDLK_KP_2) { orientRotate(o, '2');}

        else if (event.key.keysym.sym == SDLK_q) return true;
        else if (event.key.keysym.sym == SDLK_SPACE) *p = !*p;

        else if (event.key.keysym.sym == SDLK_u) {strokedTriangleDraw(window, randomTriangle(), colour, d);}
        else if (event.key.keysym.sym == SDLK_f) {filledTriangleDraw(window, randomTriangle(), colour, d);}
        else if (event.key.keysym.sym == SDLK_t) {
            CanvasPoint v0 = CanvasPoint(160, 10);
            CanvasPoint v1 = CanvasPoint(300, 230);
            CanvasPoint v2 = CanvasPoint(10, 150);
            v0.texturePoint = TexturePoint(195, 5);
            v1.texturePoint = TexturePoint(395, 380);
            v2.texturePoint = TexturePoint(65, 330);
            texturedTriangleDraw(window, CanvasTriangle(v0, v1, v2), "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm", d);
        }

    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        window.savePPM("output.ppm");
        window.saveBMP("output.bmp");
    }
    return false;
}



int main(int argc, char *argv[]) {
    DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
    SDL_Event event;
    bool terminate = false;
    bool* pause = new bool(true);

    // texture
//    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/textured-cornell-box.mtl");
//    std::vector<ModelTriangle> obj = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/textured-cornell-box.obj", mtl, 0.35);

    // no texture
    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.mtl");
    std::vector<ModelTriangle> obj = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.obj", mtl, 0.35);


    glm::vec3* cameraToVertex = new glm::vec3 (0.0, 0.0, 4.0);
    float* f = new float(2.0);

    // TODO : study heap/memory allocation
    // TODO : study pointer
    float** depthBuffer = new float*[WIDTH];
    for (int i = 0; i < WIDTH; ++i) {
        depthBuffer[i] = new float [HEIGHT];
    }
    for (int i = 0; i < WIDTH; ++i) {
        for (int j = 0; j < HEIGHT; ++j) {
            depthBuffer[i][j] = 0;
        }
    }

    glm::mat3* cameraOrientation = new glm::mat3(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
    );

    while (!terminate) {
        if (window.pollForInputEvents(event)) terminate = handleEvent(event, window, cameraToVertex, cameraOrientation, depthBuffer, pause);
        for (int i = 0; i < WIDTH; ++i) {
            for (int j = 0; j < HEIGHT; ++j) {
                depthBuffer[i][j] = 0;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (!*pause){
            orbit(cameraToVertex);
        }
        lookAt(cameraToVertex, cameraOrientation);
        objFaceDraw(window, obj, cameraToVertex, cameraOrientation, f, HEIGHT, depthBuffer, "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");
        drawRayTracedScene(window, *cameraToVertex, *cameraOrientation, *f, obj);
        window.renderFrame();
    }


    // TODO : study memory de-allocation
    for (int i = 0; i < WIDTH; ++i) {
        delete[] depthBuffer[i];
    }
    delete[] depthBuffer;
    delete cameraToVertex;
    delete f;
    delete cameraOrientation;
    delete pause;
}