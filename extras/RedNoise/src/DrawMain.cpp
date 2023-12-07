#include <DrawingWindow.h>
#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>

#include "ally/raster.h"
#include "ally/camera_move.h"
#include "ally/file_reader.h"

#include <unordered_map>
#include <thread>

#define WIDTH 320
#define HEIGHT 240

glm::vec3 lightSource = glm::vec3(0.0, 0.4, 0.25);
std::vector<glm::vec3> lightPositions;

bool box = true;

bool proximityLight = true;
bool angleOfIncidenceLight = true;
bool specularLight = true;
bool shadowLight = true;

bool mirror = true;
bool metal = true;

bool rayTrace = false;


void lightInitialisation(std::vector<ModelTriangle> obj) {
    lightPositions.clear();
    glm::vec3 lightPositionsMean;
    float verticesNumber = 0;
    for (const auto& face : obj){
        if(face.colour.name == "White"){
            lightPositionsMean += face.vertices[0];
            lightPositions.push_back(0.5f * face.vertices[0]);
            lightPositions.push_back(0.25f * face.vertices[0] + 0.25f * face.vertices[1]);
            lightPositionsMean += face.vertices[1];
            lightPositions.push_back(0.5f * face.vertices[1]);
            lightPositions.push_back(0.25f * face.vertices[1] + 0.25f * face.vertices[2]);
            lightPositionsMean += face.vertices[2];
            lightPositions.push_back(0.5f * face.vertices[2]);
            lightPositions.push_back(0.25f * face.vertices[2] + 0.25f * face.vertices[0]);
            verticesNumber += 6;
        }
    }
    lightPositionsMean = lightPositionsMean/(2*verticesNumber);
    lightPositions.push_back(lightPositionsMean);
//    lightSource = lightPositionsMean;
    std::cout << lightSource.x << "," << lightSource.y << "," << lightSource.z << std::endl;
//    std::cout << lightSource << std::endl;
}



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


glm::vec3 barycentric(const ModelTriangle& triangle, int x, int y) {
    glm::vec3 v0 = triangle.vertices[0];
    glm::vec3 v1 = triangle.vertices[1];
    glm::vec3 v2 = triangle.vertices[2];

    glm::vec3 v0v1 = v1 - v0;
    glm::vec3 v0v2 = v2 - v0;
    glm::vec3 v0p = glm::vec3(x, y, 0) - v0;

    float dot00 = glm::dot(v0v2, v0v2);
    float dot01 = glm::dot(v0v2, v0v1);
    float dot02 = glm::dot(v0v2, v0p);
    float dot11 = glm::dot(v0v1, v0v1);
    float dot12 = glm::dot(v0v1, v0p);

    float d = 1/(dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * d;
    float v = (dot00 * dot12 - dot01 * dot02) * d;
    return glm::vec3(u, v, 1 - (u + v));
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


float proximityLighting (glm::vec3 intersection) {
    float distance = glm::length(lightSource - intersection) +1;
    float lighting = 50*(1/( 4 * M_PI * distance * distance));
    return lighting;
}


float angleOfIncidenceLighting (glm::vec3 intersection, glm::vec3 normal) {
    glm::vec3 lightDirection = glm::normalize(lightSource - intersection);
    float lighting = glm::dot(normal, lightDirection);
    return lighting;
}


float specularLighting(glm::vec3 intersection, glm::vec3 normal, glm::vec3 cameraRayDirection) {
    glm::vec3 lightDirection = glm::normalize(lightSource - intersection);
    glm::vec3 reflectionDirection = glm::reflect(-lightDirection, normal);

    float specularFactor = glm::dot(reflectionDirection, -cameraRayDirection);
    specularFactor = glm::max(specularFactor, 0.0f);
    float lighting = pow(specularFactor, 256); //specular exponent
    return lighting;
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


//Colour hardShadow(Colour colour, glm::vec3 intersection, std::vector<ModelTriangle> obj){
//    glm::vec3 light = glm::normalize( lightSource - intersection);
//    float lightDistance = glm::length( lightSource - intersection);
//    RayTriangleIntersection shadowIntersection = getClosestValidIntersection( intersection, light, obj, true);
//
//    if (shadowIntersection.distanceFromCamera <= lightDistance){
//        return Colour(0.5 * colour.red, 0.5 * colour.green, 0.5 * colour.blue);
//    } else return colour;
//}


float softShadow (glm::vec3 intersection, std::vector<ModelTriangle>& obj) {
    std::vector<float> lightingFactors;
    float lightLevel = 0.5/static_cast<int>(lightPositions.size());
    float lighting = 1;

    if (lightPositions.size() == 1) {
        glm::vec3 light = glm::normalize(lightPositions[0] - intersection);
        float lightDistance = glm::length(lightPositions[0] - intersection);
        RayTriangleIntersection shadowIntersection = getClosestValidIntersection(intersection, light, obj, true);

        if (shadowIntersection.distanceFromCamera <= lightDistance) {
            return 0.5;
        }
    } else {
        for (const auto& lightPos : lightPositions) {
            glm::vec3 light = glm::normalize(lightPos - intersection);
            float lightDistance = glm::length(lightPos - intersection);
            RayTriangleIntersection shadowIntersection = getClosestValidIntersection(intersection, light, obj, true);
            if (shadowIntersection.distanceFromCamera <= lightDistance) {
//                std::cout << "found shadow factor" << std::endl;
                lighting -= lightLevel;
//                std::cout << lighting << std::endl;
            }
        }
//        std::cout << lighting << std::endl;
    }
    return lighting;
}

float ambientLighting(std::vector<float> lightingFactors, float ambientThreshold) {
    if (lightingFactors.empty()) {return 1;}
    float s = 0.0f;
    for (const float& factor : lightingFactors) {s += factor;}
    float lighting = s / lightingFactors.size();
    if (lighting < ambientThreshold) {lighting = ambientThreshold;} else if(lighting > 1) {lighting = 1;}
//    std::cout << lighting << std::endl;
    return lighting;
}


Colour light(Colour colour, glm::vec3 intersection, glm::vec3 normal, glm::vec3 cameraRayDirection, std::vector<ModelTriangle>& obj) {
    std::vector<float> lightingFactors;
    float lighting;
    if(proximityLight){
        lighting = proximityLighting(intersection);
        lightingFactors.push_back(lighting);
    }
    if(angleOfIncidenceLight){
        lighting = angleOfIncidenceLighting(intersection, normal);
        lightingFactors.push_back(lighting);
    }
    if(specularLight){
        lighting = specularLighting(intersection, normal, cameraRayDirection);
        lightingFactors.push_back(lighting);
    }
    if(shadowLight){
        lighting = softShadow(intersection, obj);
        lightingFactors.push_back(lighting);
    }
    if(lightingFactors.empty()) { return colour; } else {lighting = ambientLighting(lightingFactors, 0.2);}
    return Colour(lighting * colour.red, lighting * colour.green, lighting * colour.blue);
}






void drawRayTracedScene(DrawingWindow &window, glm::vec3 c, glm::mat3 o, float f, std::vector<ModelTriangle> obj){
    window.clearPixels();
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            glm::vec3 rayDirection = glm::vec3 (-WIDTH/2 + x, HEIGHT/2 - y, -f*HEIGHT/2);
//            glm::vec3 rayDirection = o * glm::vec3 (2*x/HEIGHT - 2*WIDTH/HEIGHT, 1 - 2*y/HEIGHT, -f);
            rayDirection = glm::normalize(rayDirection - c);
            rayDirection = glm::normalize(glm::inverse(o) * rayDirection);
            RayTriangleIntersection intersection = getClosestValidIntersection(c, rayDirection, obj, false);

            if (intersection.distanceFromCamera < std::numeric_limits<float>::infinity()) {
                ModelTriangle triangle = intersection.intersectedTriangle;

                if (metal && triangle.colour.name == "Cyan") {
                    glm::vec3 initial = glm::normalize(intersection.intersectionPoint - c);
                    glm::vec3 reflected = initial - 2.0f*(glm::dot(initial, triangle.normal)) * triangle.normal;
                    RayTriangleIntersection reflectedInt = getClosestValidIntersection(intersection.intersectionPoint, reflected, obj,true);
                    if (reflectedInt.distanceFromCamera < std::numeric_limits<float>::infinity()) {
                        ModelTriangle reflectedT = reflectedInt.intersectedTriangle;
                        Colour OriginalColour = triangle.colour;
                        Colour ReflectedColour = reflectedInt.intersectedTriangle.colour;
                        Colour colour = Colour((OriginalColour.red + ReflectedColour.red)/2, (OriginalColour.green + ReflectedColour.green)/2, (OriginalColour.blue + ReflectedColour.blue)/2);
                        colour = light(colour, reflectedInt.intersectionPoint, reflectedInt.intersectedTriangle.normal, reflected, obj);
                        window.setPixelColour(x, y, colour);
                    }
                } else if (mirror && triangle.colour.name == "Blue") {
                    glm::vec3 initial = glm::normalize(intersection.intersectionPoint - c);
                    glm::vec3 reflected = initial - 2.0f*(glm::dot(initial, triangle.normal)) * triangle.normal;
                    RayTriangleIntersection reflectedInt = getClosestValidIntersection(intersection.intersectionPoint, reflected, obj,true);
                    if (reflectedInt.distanceFromCamera < std::numeric_limits<float>::infinity()) {
                        ModelTriangle reflectedT = reflectedInt.intersectedTriangle;
//                        Colour OriginalColour = triangle.colour;
                        Colour ReflectedColour = reflectedInt.intersectedTriangle.colour;
                        Colour colour = Colour((ReflectedColour.red)*0.8, (ReflectedColour.green)*0.8, (ReflectedColour.blue)*0.8);
                        colour = light(colour, reflectedInt.intersectionPoint, reflectedInt.intersectedTriangle.normal, reflected, obj);
                        window.setPixelColour(x, y, colour);
                    }
                }
                else {
                    Colour colour = intersection.intersectedTriangle.colour;
                    colour = light(colour, intersection.intersectionPoint, intersection.intersectedTriangle.normal, rayDirection, obj);
                    window.setPixelColour(x, y, colour);
                }
            }
        }
    }
}


bool handleEvent(SDL_Event event, DrawingWindow &window, glm::vec3* c, glm::mat3* o, float** &d, bool* &p) {
    float translate = 0.07;

    Colour colour(rand() % 256, rand() % 256, rand() % 256);
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_0) {rayTrace = !rayTrace;}
        else if (event.key.keysym.sym == SDLK_9) {box = !box;}

        // translate
        else if (event.key.keysym.sym == SDLK_LEFT) {(*c).x =  (*c).x - translate;}
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

        else if (event.key.keysym.sym == SDLK_1) proximityLight = !proximityLight;
        else if (event.key.keysym.sym == SDLK_2) angleOfIncidenceLight = !angleOfIncidenceLight;
        else if (event.key.keysym.sym == SDLK_3) specularLight = !specularLight;
        else if (event.key.keysym.sym == SDLK_4) shadowLight = !shadowLight;
        else if (event.key.keysym.sym == SDLK_5) mirror = !mirror;
        else if (event.key.keysym.sym == SDLK_6) metal = !metal;

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
    std::cout << "1, 2, 3, 4 to control light!" << std::endl;

    // texture
//    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/textured-cornell-box.mtl");
//    std::vector<ModelTriangle> obj = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/textured-cornell-box.obj", mtl, 0.35);

    // no texture
    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.mtl");
    std::vector<ModelTriangle> boxes = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.obj", mtl, 0.35);
    std::vector<ModelTriangle> sphere = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/sphere.obj", mtl, 1);

//    lightInitialisation(obj);

    glm::vec3* cameraToVertex = new glm::vec3 (0.0, 0.0, 4.0);
    float* f = new float(2.0);

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

    std::vector<ModelTriangle> obj;
    while (!terminate) {
        if (window.pollForInputEvents(event)) terminate = handleEvent(event, window, cameraToVertex, cameraOrientation, depthBuffer, pause);
        for (int i = 0; i < WIDTH; ++i) {
            for (int j = 0; j < HEIGHT; ++j) {
                depthBuffer[i][j] = 0;
            }
        }
        if (!*pause){
            orbit(cameraToVertex);
        }
        lookAt(cameraToVertex, cameraOrientation);
        if (box) {obj = boxes;} else {obj = sphere;}
        if (rayTrace) { drawRayTracedScene(window, *cameraToVertex, *cameraOrientation, *f, obj); }
        else {
            objFaceDraw(window, obj, cameraToVertex, cameraOrientation, f, HEIGHT, depthBuffer,
                        "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");
        }
        window.renderFrame();
    }

    for (int i = 0; i < WIDTH; ++i) {
        delete[] depthBuffer[i];
    }
    delete[] depthBuffer;
    delete cameraToVertex;
    delete f;
    delete cameraOrientation;
    delete pause;
}