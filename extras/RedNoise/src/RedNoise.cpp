#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <TextureMap.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>

// TODO : check if it's allowed to use this library
#include <unordered_map>
#include <sstream>

// own libraries
#include "libs/Reader_OBJ_MTL.h"

#define WIDTH 320
#define HEIGHT 240
using PixelScreen = float [WIDTH][HEIGHT];

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
   *  @param  c  cameraPosition
   *  @param  v  vertexPosition
   *  @param  f  focalLength
  */
CanvasPoint getCanvasIntersectionPoint (glm::vec3 c, glm::vec3 v, float f, float s) {
    // model coordinate system -> camera coordinate system
//    std::cout << "original point : " << v.x << ", " << v.y << ", " << v.z << std::endl;
    CanvasPoint r = CanvasPoint(s * f * ((v.x - c.x)/(v.z - c.z)) + WIDTH/2,
                                s * f * ((v.y - c.y)/(v.z - c.z)) + HEIGHT/2,
                                1/-(v.z - c.z));
//    std::cout << "view point : " << r.x - WIDTH/2 << ", " << r.y - HEIGHT/2 << std::endl;
//    std::cout << "shifted point : " << r.x << ", " << r.y << std::endl;
    return r;

    // original point : 0.973346, 0.962418, 0.980711
}

void lineDraw(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour){
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;

    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;

    CanvasPoint point;
    for (int i = 0; i < numberOfSteps; ++i ) {
        point.x = from.x + (xStepSize*i);
        point.y = from.y + (yStepSize*i);
        window.setPixelColour(point.x, point.y, colour);
    }
}

void lineDraw(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour, PixelScreen &d){
    glm::vec3 f = glm::vec3(from.x, from.y, from.depth);
    glm::vec3 t = glm::vec3(to.x, to.y, to.depth);

    float numberOfSteps = std::max(std::max(abs( to.x - from.x), abs(to.y - from.y)), abs(to.depth - from.depth));

    std::vector<glm::vec3> result = interpolateThreeElementValues(f, t, numberOfSteps);

    for (const auto& vec : result) {
        if (vec[2] > d[static_cast<int>(vec[0])][static_cast<int>(vec[1])]) {
            d[static_cast<int>(vec[0])][static_cast<int>(vec[1])] = vec[2];
            window.setPixelColour(vec[0], vec[1], colour);
        } else {
            std::cout << "point rejected to be drawn : " << vec[0] << ", " << vec[1] << ", " << vec[2] << std::endl;
            std::cout << "existing upper point : " << window.getPixelColour(vec[0], vec[1]) << std::endl;
        }
    }
}


void textureDraw (DrawingWindow &window, CanvasPoint from, CanvasPoint to, TextureMap texture){
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;

    float numberOfSteps = std::max(abs(xDiff), abs(yDiff));

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float t_xDiff = to.texturePoint.x - from.texturePoint.x;
    float t_yDiff = to.texturePoint.y - from.texturePoint.y;
    float t_xStepSize = t_xDiff/numberOfSteps;
    float t_yStepSize = t_yDiff/numberOfSteps;

    uint32_t intColour;
    for (int i = 0; i < numberOfSteps; ++i ) {
        intColour = texture.pixels[int(from.texturePoint.x + (t_xStepSize*i)) +
                int(from.texturePoint.y + (t_yStepSize*i)) * texture.width];
        window.setPixelColour(from.x + (xStepSize*i), from.y + (yStepSize*i),
                              Colour((intColour >> 16) & 0xFF, (intColour >> 8) & 0xFF, intColour & 0xFF));
    }
}


void strokedTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    lineDraw(window, triangle.v0(), triangle.v1(), colour);
    lineDraw(window, triangle.v1(), triangle.v2(), colour);
    lineDraw(window, triangle.v0(), triangle.v2(), colour);
}

void strokedTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour, PixelScreen &d) {
    lineDraw(window, triangle.v0(), triangle.v1(), colour, d);
    lineDraw(window, triangle.v1(), triangle.v2(), colour, d);
    lineDraw(window, triangle.v0(), triangle.v2(), colour, d);
}

void flatTriangleColourFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2, Colour colour){
    float xDiff_1 = bot1.x - top.x;
    float xDiff_2 = bot2.x - top.x;
    float yDiff = bot1.y - top.y;
    float numberOfSteps = std::max(std::max(abs(xDiff_1), abs(xDiff_2)), abs(yDiff));

    float xStepSize_1 = xDiff_1/numberOfSteps;
    float xStepSize_2 = xDiff_2/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;

    float x1, x2, y;
    for (int i = 0; i < numberOfSteps; ++i ) {
        x1 = top.x + (xStepSize_1*i);
        x2 = top.x + (xStepSize_2*i);
        y = top.y + (yStepSize*i);
        lineDraw(window, CanvasPoint(x1, y), CanvasPoint(x2, y), colour);
    }
}

void flatTriangleColourFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2, Colour colour, PixelScreen &d){
    float xDiff_1 = bot1.x - top.x;
    float xDiff_2 = bot2.x - top.x;
    float yDiff = bot1.y - top.y;
    float dDiff_1 = bot1.depth - top.depth;
    float dDiff_2 = bot2.depth - top.depth;


    float numberOfSteps = std::max( std::max(std::max(abs(xDiff_1), abs(xDiff_2)),
                                   std::max(abs(dDiff_1), abs(dDiff_2))), abs(yDiff));

    float xStepSize_1 = xDiff_1/numberOfSteps;
    float xStepSize_2 = xDiff_2/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize_1 = dDiff_1/numberOfSteps;
    float dStepSize_2 = dDiff_2/numberOfSteps;

    float x1, x2, y, d1, d2;
    for (int i = 0; i < numberOfSteps; ++i ) {
        x1 = top.x + (xStepSize_1*i);
        x2 = top.x + (xStepSize_2*i);
        y = top.y + (yStepSize*i);
        d1 = top.depth + (dStepSize_1*i);
        d2 = top.depth + (dStepSize_2*i);
        lineDraw(window, CanvasPoint(x1, y, d1), CanvasPoint(x2, y, d2), colour, d);
    }
}


void flatTriangleTextureFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2,
                              TextureMap texture){
    // triangle value
    float xDiff_1 = bot1.x - top.x;
    float xDiff_2 = bot2.x - top.x;
    float yDiff = bot1.y - top.y;
    float numberOfSteps = std::max(std::max(abs(xDiff_1), abs(xDiff_2)), abs(yDiff));

    float xStepSize_1 = xDiff_1/numberOfSteps;
    float xStepSize_2 = xDiff_2/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;

    // texture value
    float t_xDiff_1 = bot1.texturePoint.x - top.texturePoint.x;
    float t_xDiff_2 = bot2.texturePoint.x - top.texturePoint.x;
    float t_yDiff = bot1.texturePoint.y - top.texturePoint.y;

    float t_xStepSize_1 = t_xDiff_1/numberOfSteps;
    float t_xStepSize_2 = t_xDiff_2/numberOfSteps;
    float t_yStepSize = t_yDiff/numberOfSteps;


    CanvasPoint from, to;

    for (int i = 0; i < numberOfSteps; ++i ) {
        from.x = top.x + (xStepSize_1*i);
        to.x = top.x + (xStepSize_2*i);
        from.y = top.y + (yStepSize*i);
        to.y = from.y;
        from.texturePoint.x = top.texturePoint.x + (t_xStepSize_1*i);
        to.texturePoint.x = top.texturePoint.x + (t_xStepSize_2*i);
        from.texturePoint.y = top.texturePoint.y + (t_yStepSize*i);
        to.texturePoint.y = from.texturePoint.y ;

        textureDraw(window, from, to, texture);
    }
}


void filledTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    CanvasPoint p0 = triangle.v0(), p1 = triangle.v1(), p2 = triangle.v2();
    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),p1.y);

    flatTriangleColourFill(window, p0, pk, p1, colour);
    lineDraw(window, p1, pk, colour);
    flatTriangleColourFill(window, p2, pk, p1, colour);

//    Colour white = Colour(255, 255, 255);
//    strokedTriangleDraw(window, triangle, white);

    strokedTriangleDraw(window, triangle, colour);
}


void filledTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour, PixelScreen &d) {
    CanvasPoint p0 = triangle.v0(), p1 = triangle.v1(), p2 = triangle.v2();
    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),p1.y);

    flatTriangleColourFill(window, p0, pk, p1, colour, d);
    lineDraw(window, p1, pk, colour, d);
    flatTriangleColourFill(window, p2, pk, p1, colour, d);

//    Colour white = Colour(255, 255, 255);
//    strokedTriangleDraw(window, triangle, white);

    strokedTriangleDraw(window, triangle, colour, d);
}




void texturedTriangleDraw(DrawingWindow &window, CanvasTriangle triangle, const std::string &filename){
    TextureMap texture = TextureMap(filename);
    CanvasPoint p0 = triangle.v0(), p1 = triangle.v1(), p2 = triangle.v2();
    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    TexturePoint tp0 = p0.texturePoint, tp2 = p2.texturePoint;
    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),p1.y);
    float t = (pk.x - p0.x) / (p2.x - p0.x);
    pk.texturePoint.x = tp0.x + t * (tp2.x - tp0.x);
    pk.texturePoint.y = tp0.y + t * (tp2.y - tp0.y);


    flatTriangleTextureFill(window, p0, pk, p1, texture);
    textureDraw(window, p1, pk, texture);
    flatTriangleTextureFill(window, p2, pk, p1, texture);
    Colour white = Colour(255, 255, 255);
    strokedTriangleDraw(window, triangle, white);
}


void objVerticesDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 c, float f, float s) {
    CanvasPoint v;
    for (int i = 0; i < static_cast<int>(obj.size()); ++ i) {
        for (int ii = 0; ii < 3; ++ ii){
            v = getCanvasIntersectionPoint(c, obj[i].vertices[ii], f, s);
            std::cout << v << std::endl;
            window.setPixelColour(v.x, v.y, obj[i].colour);
            std::cout << obj[i].colour << std::endl;
        }
    }
}


void objEdgeDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 c, float f, float s) {
    for (int i = 0; i < static_cast<int>(obj.size()); ++ i) {
        CanvasPoint v1 = getCanvasIntersectionPoint(c, obj[i].vertices[0], f, s);
        CanvasPoint v2 = getCanvasIntersectionPoint(c, obj[i].vertices[1], f, s);
        CanvasPoint v3 = getCanvasIntersectionPoint(c, obj[i].vertices[2], f, s);
        strokedTriangleDraw(window, CanvasTriangle(v1, v2, v3), obj[i].colour);
    }
}


void objFaceDraw(DrawingWindow &window, PixelScreen &d, std::vector<ModelTriangle> obj, glm::vec3 c, float f, float s) {
    for (int i = 0; i < static_cast<int>(obj.size()); ++ i) {
        CanvasPoint v1 = getCanvasIntersectionPoint(c, obj[i].vertices[0], f, s);
        CanvasPoint v2 = getCanvasIntersectionPoint(c, obj[i].vertices[1], f, s);
        CanvasPoint v3 = getCanvasIntersectionPoint(c, obj[i].vertices[2], f, s);
        filledTriangleDraw(window, CanvasTriangle(v1, v2, v3), obj[i].colour, d);
    }
}

bool handleEvent(SDL_Event event, DrawingWindow &window) {
    Colour colour(rand() % 256, rand() % 256, rand() % 256);
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
        else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
        else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
        else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
        else if (event.key.keysym.sym == SDLK_c) window.clearPixels();
        else if (event.key.keysym.sym == SDLK_q) return true;

        else if (event.key.keysym.sym == SDLK_u) {
            strokedTriangleDraw(window, randomTriangle(), colour);
        }

        else if (event.key.keysym.sym == SDLK_f) {
            filledTriangleDraw(window, randomTriangle(), colour);
        }

        else if (event.key.keysym.sym == SDLK_t) {
            CanvasPoint v0 = CanvasPoint(160, 10);
            CanvasPoint v1 = CanvasPoint(300, 230);
            CanvasPoint v2 = CanvasPoint(10, 150);
            v0.texturePoint = TexturePoint(195, 5);
            v1.texturePoint = TexturePoint(395, 380);
            v2.texturePoint = TexturePoint(65, 330);
            texturedTriangleDraw(window, CanvasTriangle(v0, v1, v2), "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");

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

    // read mtl and obj files
    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.mtl");
    std::vector<ModelTriangle> obj = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.obj", mtl, 0.35);

    // camera point
    glm::vec3 c = glm::vec3 (0.0,0.0,4.0);
    // focal length
    float f = 2.0;

    PixelScreen depthBuffer;
//    for (int i = 0; i < WIDTH; ++i) {
//        for (int j = 0; j < HEIGHT; ++j) {
//            depthBuffer[i][j] = 0.0f;
//        }
//    }
//
    objFaceDraw(window, depthBuffer, obj, c, f, 150);
    while (!terminate) {
        if (window.pollForInputEvents(event)) terminate = handleEvent(event, window);
        window.renderFrame();
    }
}