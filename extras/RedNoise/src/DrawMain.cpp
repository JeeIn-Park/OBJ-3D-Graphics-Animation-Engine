#include <DrawingWindow.h>
#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>

#include "../libs/ally/obj_reader.h"
#include "../libs/ally/raster.h"
#include "../libs/ally/camera_move.h"

// TODO : check if it's allowed to use this library
#include <unordered_map>
#include <thread>

#define WIDTH 320
#define HEIGHT 240

std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
    std::vector<float> result;
    result.push_back(from);

    float gap;
    int numberOfGap = numberOfValues - 1;
    gap = (to - from)/numberOfGap;

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
    std::vector<glm::vec3> result;
    // result.push_back(from);

    std::vector<float> x_list = interpolateSingleFloats(from.x, to.x, numberOfValues);
    std::vector<float> y_list = interpolateSingleFloats(from.y, to.y, numberOfValues);
    std::vector<float> z_list = interpolateSingleFloats(from.z, to.z, numberOfValues);

    glm::vec3 vec;
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


/**
   *  @param  c  camera position
   *  @param  o  camera orientation
   *  @param  v  vertex position
   *  @param  f  focal length
   *  @param  s  scaling factor
  */
CanvasPoint m (glm::vec3 c, glm::mat3 o, glm::vec3 v, float f, float s) {
    glm::vec3 r = o * (v - c);
    return CanvasPoint(s/2 * -f * r.x/r.z + WIDTH/2, s/2 * f * r.y/r.z + HEIGHT/2, 1/-r.z);
}


/**
   *  @param  c           camera position
   *  @param  direction   ray direction
   *  @param  obj         list of object facets
  */
RayTriangleIntersection getClosestValidIntersection(glm::vec3 c, glm::vec3 direction, std::vector<ModelTriangle> obj) {
    // search through the all the triangles in the current scene and return details of the closest intersected triangle
    // (if indeed there is an intersection)
    RayTriangleIntersection closestIntersection;
    closestIntersection.distanceFromCamera = std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < obj.size(); ++i) {
        ModelTriangle triangle = obj[i];
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = c - triangle.vertices[0];
        glm::mat3 DEMatrix(-direction, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        if (possibleSolution.y >= 0 && possibleSolution.z >= 0 &&
            possibleSolution.y <= 1 && possibleSolution.z <= 1 &&
            possibleSolution.y       + possibleSolution.z <= 1) {

//            closestIntersection = RayTriangleIntersection(c + possibleSolution.x * direction, possibleSolution.x, triangle, i);
            closestIntersection = RayTriangleIntersection(possibleSolution, possibleSolution.x, triangle, i);
            break;
        }
    }
    return closestIntersection;
}

void drawRayTracedScene(DrawingWindow &window, glm::vec3 c, glm::mat3 o, float f, std::vector<ModelTriangle> obj){

    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            glm::vec3 rayDirection = o * glm::normalize(glm::vec3(2*x/WIDTH -1 - c.x, 1 - 2*y/HEIGHT  - c.y, -f));
            RayTriangleIntersection intersection = getClosestValidIntersection(c, rayDirection, obj);
            std::cout << x << "," << y << ":" << intersection.distanceFromCamera << std::endl;
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                ModelTriangle triangle = intersection.intersectedTriangle;
                Colour colour = triangle.colour;
                window.setPixelColour(x, y, triangle.colour);
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

    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/textured-cornell-box.mtl");
    std::vector<ModelTriangle> obj = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/textured-cornell-box.obj", mtl, 0.35);

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
//        objFaceDraw(window, obj, cameraToVertex, cameraOrientation, f, 240, depthBuffer, "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");
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