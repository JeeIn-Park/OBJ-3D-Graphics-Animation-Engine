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




void rotate(glm::vec3* c, char t){
    double angle = 1.0 * M_PI / 180.0;
    if (t == 'a'){
        glm::mat3 rotationMatrix = glm::mat3 (
                cos(angle), 0, -sin(angle),
                0, 1, 0,
                sin(angle), 0, cos(angle)
        );
        *c = rotationMatrix * *c;
    }
    else if (t == 'd'){
        glm::mat3 rotationMatrix =glm::mat3 (
                cos(-angle), 0, -sin(-angle),
                0, 1, 0,
                sin(-angle), 0, cos(-angle)
        );
        *c = rotationMatrix * *c;
    }
    else if (t == 'w'){
        glm::mat3 rotationMatrix = glm::mat3 (
                1, 0, 0,
                0, cos(angle), sin(angle),
                0, -sin(angle), cos(angle)
        );
        *c = rotationMatrix * *c;
    }
    else if (t == 's'){
        glm::mat3 rotationMatrix = glm::mat3 (
                1, 0, 0,
                0, cos(-angle), sin(-angle),
                0, -sin(-angle), cos(-angle)
        );
        *c = rotationMatrix * *c;
    }
}

void orientRotate(glm::mat3* o, char t) {
    double angle = 1.0 * M_PI / 180.0;
    if (t == '1'){
        glm::mat3 rotationMatrix =glm::mat3 (
                cos(angle), 0, -sin(angle),
                0, 1, 0,
                sin(angle), 0, cos(angle)
        );
        *o = rotationMatrix * *o;
    }
    else if (t == '3'){
        glm::mat3 rotationMatrix = glm::mat3 (
                cos(-angle), 0, -sin(-angle),
                0, 1, 0,
                sin(-angle), 0, cos(-angle)
        );
        *o = rotationMatrix * *o;
    }
    else if (t == '5'){
        glm::mat3 rotationMatrix = glm::mat3 (
                1, 0, 0,
                0, cos(angle), sin(angle),
                0, -sin(angle), cos(angle)
        );
        *o = rotationMatrix * *o;
    }
    else if (t == '2'){
        glm::mat3 rotationMatrix = glm::mat3 (
                1, 0, 0,
                0, cos(-angle), sin(-angle),
                0, -sin(-angle), cos(-angle)
        );
        *o = rotationMatrix * *o;
    }
}

void orbit(glm::vec3* c){
    rotate(c, 'd');
}

void lookAt(glm::vec3* c, glm::mat3* o){
    glm::vec3 right = glm::vec3((*o)[0][0], (*o)[1][0], (*o)[2][0]);
    glm::vec3 up = glm::vec3((*o)[0][1], (*o)[1][1], (*o)[2][1]);
    glm::vec3 forward = glm::normalize(*c);
    right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
    up = glm::normalize(glm::cross(forward, right));

    (*o)[0][0] =   right.x; (*o)[1][0] = right.y;   (*o)[2][0] = right.z;
    (*o)[0][1] =      up.x; (*o)[1][1] = up.y;      (*o)[2][1] = up.z;
    (*o)[0][2] = forward.x; (*o)[1][2] = forward.y; (*o)[2][2] = forward.z;
}


/**
   *  @param  c  camera position
   *  @param  direction  ray direction
  */
RayTriangleIntersection getClosestIntersection(glm::vec3 c, glm::vec3 direction, std::vector<ModelTriangle> obj) {
    // search through the all the triangles in the current scene and return details of the closest intersected triangle
    // (if indeed there is an intersection)
    RayTriangleIntersection closestIntersection;
    for (size_t i = 0; i < obj.size(); ++i) {
        ModelTriangle triangle = obj[i];
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = c - triangle.vertices[0];
        glm::mat3 DEMatrix(-direction, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        if (possibleSolution.x >= 0 && possibleSolution.y >= 0 && possibleSolution.z >= 0
            && possibleSolution.x <= closestIntersection.distanceFromCamera
            && possibleSolution.y <= glm::length(e0) && possibleSolution.z <= glm::length(e1)) {

            closestIntersection = RayTriangleIntersection(c + possibleSolution.x * direction, possibleSolution.x, triangle, i);
        }
    }
    return closestIntersection;
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

//    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.mtl");
//    std::vector<ModelTriangle> obj = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.obj", mtl, 0.35);

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
        objFaceDraw(window, obj, cameraToVertex, cameraOrientation, f, 240, depthBuffer, "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");
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