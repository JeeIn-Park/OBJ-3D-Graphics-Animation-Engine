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

// TODO : check if it's allowed to use this library
#include <unordered_map>
#include <thread>

#define WIDTH 320
#define HEIGHT 240

glm::vec3 lightSource = glm::vec3(0.0, 0.5, 0.25);
std::vector<glm::vec3> lightPositions;

void lightInitialisation(std::vector<ModelTriangle> obj) {
    lightPositions.clear();
    glm::vec3 lightPositionsMean;
    float verticesNumber;
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
    lightSource = lightPositionsMean;
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


Colour proximityLighting (Colour colour, glm::vec3 intersection) {
    float distance = glm::length(lightSource - intersection) +1;
    float lighting = 30*(1/( 4 * M_PI * distance * distance));
    std::cout << lighting << std::endl;
    return Colour (lighting * colour.red, lighting * colour.green, lighting * colour.blue);
}


Colour angleOfIncidenceLighting (Colour colour, glm::vec3 intersection, ModelTriangle triangle) {
    glm::vec3 lightDirection = glm::normalize(lightSource - intersection);
    float lighting = glm::dot(triangle.normal, lightDirection);
    std::cout << lighting << std::endl;
    return Colour(lighting * colour.red, lighting * colour.green, lighting * colour.blue);

}


Colour specularLighting(Colour colour, glm::vec3 intersection, ModelTriangle& triangle, glm::vec3 cameraRayDirection, float specularExponent) {
    glm::vec3 lightDirection = glm::normalize(lightSource - intersection);
    glm::vec3 reflectionDirection = glm::reflect(-lightDirection, triangle.normal);

    float specularFactor = glm::dot(reflectionDirection, -cameraRayDirection);
    specularFactor = glm::max(specularFactor, 0.0f);
    float lighting = pow(specularFactor, specularExponent);
    std::cout << lighting << std::endl;
    return Colour(lighting * colour.red, lighting * colour.green, lighting * colour.blue);
}


Colour AmbientLighting(Colour colour, std::vector<float> lightingFactors, float ambientThreshold) {
    if (lightingFactors.empty()) {return Colour(0,0,0);}
    float s = 0.0f;
    for (const float& factor : lightingFactors) {s += factor;}

    float lighting = s / lightingFactors.size();

    if (lighting < ambientThreshold) {lighting = ambientThreshold;} else if(lighting > 1) {lighting = 1;}
    std::cout << lighting << std::endl;
    return Colour(lighting * colour.red, lighting * colour.green, lighting * colour.blue);
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


Colour hardShadow(Colour colour, glm::vec3 intersection, ModelTriangle triangle, std::vector<ModelTriangle> obj){
    glm::vec3 light = glm::normalize( lightSource - intersection);
    float lightDistance = glm::length( lightSource - intersection);
    RayTriangleIntersection shadowIntersection = getClosestValidIntersection( intersection, light, obj, true);

    if (shadowIntersection.distanceFromCamera <= lightDistance){
        return Colour(0.5 * colour.red, 0.5 * colour.green, 0.5 * colour.blue);
    } else return colour;
}


Colour softShadow (Colour colour, glm::vec3 intersection, ModelTriangle triangle, const std::vector<ModelTriangle>& obj, float ambientThreshold) {
    std::vector<float> lightingFactors;
    float lightLevel = 0.5/static_cast<int>(lightPositions.size());
    float lighting = 1;

    if (lightPositions.size() == 1) {
        glm::vec3 light = glm::normalize(lightPositions[0] - intersection);
        float lightDistance = glm::length(lightPositions[0] - intersection);
        RayTriangleIntersection shadowIntersection = getClosestValidIntersection(intersection, light, obj, true);

        if (shadowIntersection.distanceFromCamera <= lightDistance) {
            return Colour(0.5f * colour.red, 0.5f * colour.green, 0.5f * colour.blue);
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
        if(lighting < 0.2) {lighting = 0.2;} else if(lighting >1) {lighting = 1.0;}
        return Colour(lighting * colour.red, lighting * colour.green, lighting * colour.blue);
    }
    return colour;
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

                if (triangle.colour.name == "Magenta") {
                    glm::vec3 initial = glm::normalize(intersection.intersectionPoint - c);
                    glm::vec3 reflected = initial - 2.0f*(glm::dot(initial, triangle.normal)) * triangle.normal;
                    RayTriangleIntersection reflectedInt = getClosestValidIntersection(intersection.intersectionPoint, reflected, obj,true);
                    if (reflectedInt.distanceFromCamera < std::numeric_limits<float>::infinity()) {
                        ModelTriangle reflectedT = intersection.intersectedTriangle;
                        Colour pixelColour = intersection.intersectedTriangle.colour;
//                        pixelColour = hardShadow(pixelColour, reflectedInt.intersectionPoint, reflectedT, obj);
                        pixelColour = softShadow(pixelColour, reflectedInt.intersectionPoint, intersection.intersectedTriangle, obj, 0.2);
//                        pixelColour = proximityLighting(pixelColour, intersection.intersectionPoint);
//                        pixelColour = angleOfIncidenceLighting(pixelColour, intersection.intersectionPoint, intersection.intersectedTriangle);
//                        pixelColour = specularLighting(pixelColour, intersection.intersectionPoint, intersection.intersectedTriangle, rayDirection, 64);
                        window.setPixelColour(x, y, pixelColour);
                    }

                }
                else {
                    Colour pixelColour = intersection.intersectedTriangle.colour;
//                    pixelColour = hardShadow(pixelColour, intersection.intersectionPoint, triangle, obj);
                    pixelColour = softShadow(pixelColour, intersection.intersectionPoint, intersection.intersectedTriangle, obj, 0.2);
//                    pixelColour = proximityLighting(pixelColour, intersection.intersectionPoint);
//                    pixelColour = angleOfIncidenceLighting(pixelColour, intersection.intersectionPoint, intersection.intersectedTriangle);
//                    pixelColour = specularLighting(pixelColour, intersection.intersectionPoint, intersection.intersectedTriangle, rayDirection, 64);

                    window.setPixelColour(x, y, pixelColour);
                }
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
    lightInitialisation(obj);

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
//        objFaceDraw(window, obj, cameraToVertex, cameraOrientation, f, HEIGHT, depthBuffer, "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");
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