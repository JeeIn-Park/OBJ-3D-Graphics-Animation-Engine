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

#define WIDTH 640
#define HEIGHT 480

glm::vec3 lightSource = glm::vec3(0.0, 0.4, 0.25);
std::vector<glm::vec3> lightPositions;
std::vector<TriangleInfo> triangleIndices;
std::vector<glm::vec3> vertexNorms;
Colour metalSurfaceColour;

bool box = true;
bool texture = false;

bool proximityLight = true;
bool angleOfIncidenceLight = true;
bool specularLight = true;
bool shadowLight = true;

bool mirror = true;
bool metal = true;
bool glass = true;

bool rayTrace = false;

Colour metalReflected (RayTriangleIntersection originalIntersection, glm::vec3 inputRay, RayTriangleIntersection intersection, std::vector<ModelTriangle> obj, float brightness);
Colour mirrorReflected (RayTriangleIntersection originalIntersection, glm::vec3 inputRay, RayTriangleIntersection intersection, std::vector<ModelTriangle> obj, float brightness);
Colour refracted (RayTriangleIntersection originalIntersection, glm::vec3 inputRay, RayTriangleIntersection intersection, std::vector<ModelTriangle> obj, float brightness);


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


glm::vec3 barycentricCoordinates(glm::vec3 point, ModelTriangle triangle) {
    glm::vec3 v1 = triangle.vertices[0];
    glm::vec3 v2 = triangle.vertices[1];
    glm::vec3 v3 = triangle.vertices[2];
    glm::vec3 v0 = v2 - v1;
    glm::vec3 v1v2 = v3 - v1;
//    glm::vec3 v2v1 = v2 - v3;
    glm::vec3 v2point = point - v1;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1v2);
    float d11 = glm::dot(v1v2, v1v2);
    float d20 = glm::dot(v2point, v0);
    float d21 = glm::dot(v2point, v1v2);

    float denom = d00 * d11 - d01 * d01;

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}



glm::vec3 calculatePointNormal(glm::vec3 normalV0, glm::vec3 normalV1, glm::vec3 normalV2, glm::vec3 barycentricCoords) {
    glm::vec3 interpolatedNormal;

    interpolatedNormal.x = normalV0.x * barycentricCoords.x +
                           normalV1.x * barycentricCoords.y +
                           normalV2.x * barycentricCoords.z;

    interpolatedNormal.y = normalV0.y * barycentricCoords.x +
                           normalV1.y * barycentricCoords.y +
                           normalV2.y * barycentricCoords.z;

    interpolatedNormal.z = normalV0.z * barycentricCoords.x +
                           normalV1.z * barycentricCoords.y +
                           normalV2.z * barycentricCoords.z;

    return glm::normalize(interpolatedNormal);
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
//                std::cout << lighting << std::endl;
    return lighting;
}


float specularLighting(glm::vec3 intersection, glm::vec3 normal, glm::vec3 cameraRayDirection) {
    glm::vec3 lightDirection = glm::normalize(lightSource - intersection);
    glm::vec3 reflectionDirection = glm::reflect(-lightDirection, normal);

    float specularFactor = glm::dot(reflectionDirection, -cameraRayDirection);
    specularFactor = glm::max(specularFactor, 0.0f);
    float lighting;
    if (box) { lighting =  pow(specularFactor, 265);} else {lighting =  pow(specularFactor, 64);}
//                std::cout << lighting << std::endl;
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


Colour light(Colour colour, glm::vec3 intersection, ModelTriangle triangle, glm::vec3 cameraRayDirection, std::vector<ModelTriangle>& obj, size_t i) {
    std::vector<float> lightingFactors;
    glm::vec3 b = barycentricCoordinates(intersection, triangle);
    glm::vec3 normal = calculatePointNormal(vertexNorms[triangleIndices[i].triangleIndices[0]], vertexNorms[triangleIndices[i].triangleIndices[1]], vertexNorms[triangleIndices[i].triangleIndices[2]],b);
//glm::vec3 normal = triangle.normal;

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
    if(lightingFactors.empty()) { return colour; } else {lighting = ambientLighting(lightingFactors, 0.1);}
    return Colour(lighting * colour.red, lighting * colour.green, lighting * colour.blue);
}




Colour metalReflected (RayTriangleIntersection originalIntersection, glm::vec3 inputRay, RayTriangleIntersection intersection, std::vector<ModelTriangle> obj, float brightness){
    Colour colour = Colour(0,0,0);
    if (brightness < 0.2) return colour;
    ModelTriangle triangle = intersection.intersectedTriangle;
    glm::vec3 reflectedRay = inputRay - 2.0f*(glm::dot(inputRay, triangle.normal)) * triangle.normal;
    RayTriangleIntersection reflectedIntersection = getClosestValidIntersection(intersection.intersectionPoint, reflectedRay, obj, true);

    if (metal && reflectedIntersection.intersectedTriangle.colour.name == "Cyan") {
        metalSurfaceColour = reflectedIntersection.intersectedTriangle.colour;
        return metalReflected(originalIntersection, reflectedRay, reflectedIntersection, obj, brightness*0.8f);
    } else if (mirror && (reflectedIntersection.intersectedTriangle.colour.name == "Yellow" || reflectedIntersection.intersectedTriangle.colour.name == "Magenta")) {
        return mirrorReflected(originalIntersection, reflectedRay, reflectedIntersection, obj, brightness*0.8f);
    }
    else {
        if (reflectedIntersection.distanceFromCamera < std::numeric_limits<float>::infinity()) {
            Colour reflectedColour = reflectedIntersection.intersectedTriangle.colour;
            colour = Colour(brightness * ((metalSurfaceColour.red + reflectedColour.red) / 2), brightness * ((metalSurfaceColour.green + reflectedColour.green) / 2), brightness * ((metalSurfaceColour.blue + reflectedColour.blue) / 2));
            colour = light(colour, reflectedIntersection.intersectionPoint, reflectedIntersection.intersectedTriangle, reflectedRay, obj, reflectedIntersection.triangleIndex);
        }
    }
    return colour;
}

Colour mirrorReflected (RayTriangleIntersection originalIntersection, glm::vec3 inputRay, RayTriangleIntersection intersection, std::vector<ModelTriangle> obj, float brightness){
    Colour colour = Colour(0,0,0);
    if (brightness < 0.2) return colour;
    ModelTriangle triangle = intersection.intersectedTriangle;
    glm::vec3 reflectedRay = inputRay - 2.0f*(glm::dot(inputRay, triangle.normal)) * triangle.normal;
    RayTriangleIntersection reflectedIntersection = getClosestValidIntersection(intersection.intersectionPoint, reflectedRay, obj, true);

    if (metal && reflectedIntersection.intersectedTriangle.colour.name == "Cyan") {
        metalSurfaceColour = reflectedIntersection.intersectedTriangle.colour;
        return metalReflected(originalIntersection, reflectedRay, reflectedIntersection, obj, brightness*0.8f);
    } else if (mirror && (reflectedIntersection.intersectedTriangle.colour.name == "Yellow" || reflectedIntersection.intersectedTriangle.colour.name == "Magenta")) {
        return mirrorReflected(originalIntersection, reflectedRay, reflectedIntersection, obj, brightness*0.8f);
    }
    else {
        if (reflectedIntersection.distanceFromCamera < std::numeric_limits<float>::infinity()) {
            Colour reflectedColour = reflectedIntersection.intersectedTriangle.colour;
            colour = Colour(brightness*reflectedColour.red, brightness*reflectedColour.green, brightness*reflectedColour.blue);
            colour = light(colour, reflectedIntersection.intersectionPoint, reflectedIntersection.intersectedTriangle, reflectedRay, obj, reflectedIntersection.triangleIndex);
        }
    }
    return colour;
}

//Colour refracted (RayTriangleIntersection originalIntersection, glm::vec3 inputRay, RayTriangleIntersection intersection, std::vector<ModelTriangle> obj, float brightness) {
//    float cosI = -glm::dot(inputRay, intersection.intersectedTriangle.normal);
//    float eta = 1.0f / 1.5f;
//    float k = 1.0f - eta * eta * (1.0f - cosI * cosI);
//
//    if (k < 0.0f) {
//        // Total internal reflection, handle this case if necessary
//        return glm::vec3(0.0f); // Return some default value or handle accordingly
//    } else {
//        glm::vec3 refractedRay = eta * inputRay + (eta * cosI - sqrtf(k)) * intersection.intersectedTriangle.normal;
//        RayTriangleIntersection refractedIntersection = getClosestValidIntersection(intersection.intersectionPoint, refractedRay, obj, true);
//    }
//
//}



void drawRayTracedScene(DrawingWindow &window, glm::vec3 c, glm::mat3 o, float f, std::vector<ModelTriangle> obj){
    window.clearPixels();
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            glm::vec3 rayDirection = glm::vec3 (-WIDTH/2 + x, HEIGHT/2 - y, -f * HEIGHT/2);
//          glm::vec3 rayDirection = o * glm::vec3 (2*x/HEIGHT - 2*WIDTH/HEIGHT, 1 - 2*y/HEIGHT, -f);
            rayDirection = glm::normalize(rayDirection - c);
            rayDirection = glm::normalize(glm::inverse(o) * rayDirection);
            RayTriangleIntersection intersection = getClosestValidIntersection(c, rayDirection, obj, false);

            if (intersection.distanceFromCamera < std::numeric_limits<float>::infinity()) {
                ModelTriangle triangle = intersection.intersectedTriangle;

                if (metal && triangle.colour.name == "Cyan") {
                    metalSurfaceColour = triangle.colour;
                    Colour colour = metalReflected(intersection, rayDirection, intersection, obj, 0.9f);
                    window.setPixelColour(x, y, colour);
                } else if (mirror && (triangle.colour.name == "Yellow" || triangle.colour.name == "Magenta")) {
                    Colour colour = mirrorReflected(intersection, rayDirection, intersection, obj, 0.9f);
                    window.setPixelColour(x, y, colour);
                }
//                } else if (glass && triangle.colour.name == "Red") {
//                    Colour colour = refracted(intersection, rayDirection, intersection, obj, 0.9f);
//                    window.setPixelColour(x, y, colour);
//                }
                else {
                    Colour colour = intersection.intersectedTriangle.colour;
                    colour = light(colour, intersection.intersectionPoint, intersection.intersectedTriangle, rayDirection, obj, intersection.triangleIndex);
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

//        else if (event.key.keysym.sym == SDLK_LEFT) {lightSource = glm::vec3 (lightSource.x + 0.5, lightSource.y, lightSource.z);}
//        else if (event.key.keysym.sym == SDLK_RIGHT) {lightSource = glm::vec3 (lightSource.x - 0.5, lightSource.y, lightSource.z);}
//        else if (event.key.keysym.sym == SDLK_UP) {lightSource = glm::vec3 (lightSource.x, lightSource.y + 0.5, lightSource.z);}
//        else if (event.key.keysym.sym == SDLK_DOWN) {lightSource = glm::vec3 (lightSource.x, lightSource.y - 0.5, lightSource.z);}
//        else if (event.key.keysym.sym == SDLK_KP_MINUS) {lightSource = glm::vec3 (lightSource.x, lightSource.y, lightSource.z + 0.5);}
//        else if (event.key.keysym.sym == SDLK_KP_PLUS) {lightSource = glm::vec3 (lightSource.x, lightSource.y, lightSource.z - 0.5);}

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
        else if (event.key.keysym.sym == SDLK_7) texture = !texture;

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
            texturedTriangleDraw(window, CanvasTriangle(v0, v1, v2), "/computer_graphics/extras/RedNoise/src/texture.ppm", d);
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
    std::unordered_map<std::string, Colour> t_mtl = readMTL("/computer_graphics/extras/RedNoise/src/textured-cornell-box.mtl");
    std::vector<ModelTriangle> t_boxes = readTextureOBJ("/computer_graphics/extras/RedNoise/src/textured-cornell-box.obj", t_mtl, 0.35);

    // no texture
    std::unordered_map<std::string, Colour> mtl = readMTL("/computer_graphics/extras/RedNoise/src/cornell-box.mtl");
    std::tuple<std::vector<ModelTriangle>, std::vector<TriangleInfo>, std::vector<glm::vec3>> boxes = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.obj", mtl, 0.35);
    std::tuple<std::vector<ModelTriangle>, std::vector<TriangleInfo>, std::vector<glm::vec3>> sphere = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/sphere.obj", mtl, 1);


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

    std::vector<ModelTriangle> obj = std::get<0>(boxes);
    triangleIndices = std::get<1>(boxes);
    vertexNorms = std::get<2>(boxes);

    lightInitialisation(std::get<0>(boxes));
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
        if (box && !texture) {
            lightSource = glm::vec3(0.0, 0.4, 0.25);
            obj = std::get<0>(boxes);
            triangleIndices = std::get<1>(boxes);
            vertexNorms = std::get<2>(boxes);
        } else if (box && texture){
            lightSource = glm::vec3(0.0, 0.4, 0.25);
            obj = t_boxes;
            triangleIndices = std::get<1>(boxes);
            vertexNorms = std::get<2>(boxes);
        }
        else {obj = std::get<0>(sphere);
            lightSource = glm::vec3(4, 6, 5);
            triangleIndices = std::get<1>(sphere);
            vertexNorms = std::get<2>(sphere);}
        if (rayTrace) { drawRayTracedScene(window, *cameraToVertex, *cameraOrientation, *f, obj); }
        else {
            objFaceDraw(window, obj, cameraToVertex, cameraOrientation, f, HEIGHT, depthBuffer,
                        "/computer_graphics/extras/RedNoise/src/texture.ppm");
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