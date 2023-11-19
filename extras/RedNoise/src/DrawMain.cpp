#include <DrawingWindow.h>
#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <TextureMap.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>

#include "../libs/ally/obj_reader.h"

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
   *  @param  c  cameraPosition
   *  @param  v  vertexPosition
   *  @param  f  focalLength
   *  @param  s  scaling factor
  */
CanvasPoint getCanvasIntersectionPoint (glm::vec3 c, glm::mat3 o, glm::vec3 v, float f, float s) {
    glm::vec3 r = o * (v - c);
    return CanvasPoint(s/2 * -f * r.x/r.z + WIDTH/2, s/2 * f * r.y/r.z + HEIGHT/2, 1/-r.z);
}

//CanvasPoint getTexturedCanvasIntersectionPoint(glm::vec3 c, glm::mat3 o, CanvasPoint vertex, float f, float s, TextureMap texture) {
//    glm::vec3 v = glm::vec3(vertex.x/WIDTH, vertex.y/HEIGHT, vertex.depth/100);
//
//    glm::vec3 rv = o * (v - c);
//    CanvasPoint result = CanvasPoint(s / 2 * -f * rv.x / rv.z + WIDTH / 2,
//                                     s / 2 *  f * rv.y / rv.z + HEIGHT / 2,
//                                     1 / -rv.z);
//
//    TexturePoint t = TexturePoint(vertex.texturePoint.x/texture.width -0.5, vertex.texturePoint.y/texture.height -0.5);
//    glm::vec3 rt = glm::vec3( 2 * t.x,  2 * t.y, v.z);
//    rt = o * (rt - c);
//    CanvasPoint textureResult = CanvasPoint( (texture.width/2) * f * rt.x / rt.z + texture.width/2,
//                                             (texture.height/2) * - f * rt.y / rt.z + texture.height/2);
//    result.texturePoint = TexturePoint(textureResult.x, textureResult.y);
//
////    uint32_t intColour = textureMap.pixels[textureResult.x + textureResult.y * textureMap.width];
////    Colour((intColour >> 16) & 0xFF, (intColour >> 8) & 0xFF, intColour & 0xFF);
//    return result;
//}


void lineDraw(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour, float** &d){
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;
    float dDiff = to.depth - from.depth;

    float numberOfSteps = std::max(std::max(abs(xDiff), abs(yDiff)), abs(dDiff));

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize = dDiff/numberOfSteps;


    for (int i = 0; i < numberOfSteps; ++i ) {
        float x = from.x + (xStepSize*i);
        float y = from.y + (yStepSize*i);
        float depth = from.depth + (dStepSize*i);

        int intX = static_cast<int>(x);
        int intY = static_cast<int>(y);

        if ((intX > 0 && intX < WIDTH && intY > 0 && intY < HEIGHT) && depth >= d[intX][intY]) {
            window.setPixelColour(intX, intY, colour);
            d[intX][intY] = depth;
        }
    }
}


void textureDraw (DrawingWindow &window, CanvasPoint from, CanvasPoint to, TextureMap texture, float** d){
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;
    float dDiff = to.depth - from.depth;

    float numberOfSteps = std::max(std::max(abs(xDiff), abs(yDiff)), abs(dDiff));

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize = dDiff/numberOfSteps;
    float t_xDiff = to.texturePoint.x - from.texturePoint.x;
    float t_yDiff = to.texturePoint.y - from.texturePoint.y;
    float t_xStepSize = t_xDiff/numberOfSteps;
    float t_yStepSize = t_yDiff/numberOfSteps;

    uint32_t intColour;
    for (int i = 0; i < numberOfSteps; ++i ) {
        intColour = texture.pixels[int(from.texturePoint.x + (t_xStepSize*i)) +
                                   int(from.texturePoint.y + (t_yStepSize*i)) * texture.width];
        float x = from.x + (xStepSize*i);
        float y = from.y + (yStepSize*i);
        float depth = from.depth + (dStepSize*i);

        int intX = static_cast<int>(x);
        int intY = static_cast<int>(y);

        if ((intX > 0 && intX < WIDTH && intY > 0 && intY < HEIGHT) && depth >= d[intX][intY]) {
            window.setPixelColour(intX, intY,
                                  Colour((intColour >> 16) & 0xFF, (intColour >> 8) & 0xFF, intColour & 0xFF));
//            std::cout << "draw texture : " << intColour << std::endl;
            d[intX][intY] = depth;
        }

    }
}

void textureSurfaceLineDraw(DrawingWindow &window, CanvasPoint from, CanvasPoint to,
                            glm::vec3 *c, glm::mat3 *o, float *f, float s, TextureMap texture, float** d){
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;
    float dDiff = to.depth - from.depth;

    float numberOfSteps = std::max(std::max(abs(xDiff), abs(yDiff)), abs(dDiff));

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize = dDiff/numberOfSteps;

    float t_xDiff = to.texturePoint.x - from.texturePoint.x;
    float t_yDiff = to.texturePoint.y - from.texturePoint.y;

    float t_xStepSize = t_xDiff/numberOfSteps;
    float t_yStepSize = t_yDiff/numberOfSteps;

    uint32_t intColour = 0;
    for (int i = 0; i < numberOfSteps; ++i ) {
        CanvasPoint v = CanvasPoint(from.x + (xStepSize*i), from.y + (yStepSize*i));
        v.texturePoint.x = from.texturePoint.x + t_xStepSize*i;
        v.texturePoint.y = from.texturePoint.y + t_yStepSize*i;
        v.depth = from.depth + (dStepSize*i);
        intColour = texture.pixels[int(v.texturePoint.x) + int(v.texturePoint.y) * texture.width];
        glm::vec3 vertex = glm::vec3 (v.x/(WIDTH/2), v.y/(HEIGHT/2), v.depth/100);
        v = getCanvasIntersectionPoint(*c, *o, vertex, *f, s);
        int intX = static_cast<int>(v.x);
        int intY = static_cast<int>(v.y);
        if ((intX > 0 && intX < WIDTH && intY > 0 && intY < HEIGHT) && v.depth >= d[intX][intY]) {
            window.setPixelColour(intX, intY,
                                  Colour((intColour >> 16) & 0xFF, (intColour >> 8) & 0xFF, intColour & 0xFF));
//            std::cout << "draw texture : " << intColour << std::endl;
            d[intX][intY] = v.depth;
        }
    }
}


void strokedTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour, float** &d) {
    lineDraw(window, triangle.v0(), triangle.v1(), colour, d);
    lineDraw(window, triangle.v1(), triangle.v2(), colour, d);
    lineDraw(window, triangle.v0(), triangle.v2(), colour, d);
}


void flatTriangleColourFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2, Colour colour, float** &d){
    float xDiff_1 = bot1.x - top.x;
    float xDiff_2 = bot2.x - top.x;
    float yDiff = bot1.y - top.y;
    float dDiff_1 = bot1.depth - top.depth;
    float dDiff_2 = bot2.depth - top.depth;
    float numberOfSteps = std::max(std::max(std::max(abs(xDiff_1), abs(xDiff_2)), std::max(abs(dDiff_1), abs(dDiff_2))), abs(yDiff));

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
                              TextureMap texture, float** &d){
    // triangle value
    float xDiff_1 = bot1.x - top.x;
    float xDiff_2 = bot2.x - top.x;
    float yDiff = bot1.y - top.y;
    float dDiff_1 = bot1.depth - top.depth;
    float dDiff_2 = bot2.depth - top.depth;
    float numberOfSteps = std::max(std::max(std::max(abs(xDiff_1), abs(xDiff_2)), std::max(abs(dDiff_1), abs(dDiff_2))), abs(yDiff));

    float xStepSize_1 = xDiff_1/numberOfSteps;
    float xStepSize_2 = xDiff_2/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize_1 = dDiff_1/numberOfSteps;
    float dStepSize_2 = dDiff_2/numberOfSteps;

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
        from.depth = top.depth + (dStepSize_1*i);
        to.depth = top.depth + (dStepSize_2*i);
        from.texturePoint.x = top.texturePoint.x + (t_xStepSize_1*i);
        to.texturePoint.x = top.texturePoint.x + (t_xStepSize_2*i);
        from.texturePoint.y = top.texturePoint.y + (t_yStepSize*i);
        to.texturePoint.y = from.texturePoint.y ;

        textureDraw(window, from, to, texture, d);
    }
}

void flatTriangleTextureSurfaceFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2,
                                     glm::vec3 *c, glm::mat3 *o, float *f, float s, TextureMap texture, float** &d){
    // triangle value
    float xDiff_1 = bot1.x - top.x;
    float xDiff_2 = bot2.x - top.x;
    float yDiff = bot1.y - top.y;
    float dDiff_1 = bot1.depth - top.depth;
    float dDiff_2 = bot2.depth - top.depth;
    float numberOfSteps = std::max(std::max(std::max(abs(xDiff_1), abs(xDiff_2)), std::max(abs(dDiff_1), abs(dDiff_2))), abs(yDiff));

    float xStepSize_1 = xDiff_1/numberOfSteps;
    float xStepSize_2 = xDiff_2/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize_1 = dDiff_1/numberOfSteps;
    float dStepSize_2 = dDiff_2/numberOfSteps;

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
        from.depth = top.depth + (dStepSize_1*i);
        to.depth = top.depth + (dStepSize_2*i);
        from.texturePoint.x = top.texturePoint.x + (t_xStepSize_1*i);
        to.texturePoint.x = top.texturePoint.x + (t_xStepSize_2*i);
        from.texturePoint.y = top.texturePoint.y + (t_yStepSize*i);
        to.texturePoint.y = from.texturePoint.y ;

        textureSurfaceLineDraw(window, from, to, c, o, f, s, texture, d);
    }
}


void filledTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour, float** &d) {
    CanvasPoint p0 = triangle.v0(), p1 = triangle.v1(), p2 = triangle.v2();
    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),
                                 p1.y,
                                 ((p1.y-p0.y)*p2.depth + (p2.y-p1.y)*p0.depth)/(p2.y-p0.y));

    flatTriangleColourFill(window, p0, pk, p1, colour, d);
    lineDraw(window, p1, pk, colour, d);
    flatTriangleColourFill(window, p2, pk, p1, colour, d);
    strokedTriangleDraw(window, triangle, colour, d);
}


void texturedTriangleDraw(DrawingWindow &window, CanvasTriangle triangle, const std::string &filename, float** &d){
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


    flatTriangleTextureFill(window, p0, pk, p1, texture, d);
    textureDraw(window, p1, pk, texture, d);
    flatTriangleTextureFill(window, p2, pk, p1, texture, d);
    Colour white = Colour(255, 255, 255);
}


void texturedSurfaceDraw(DrawingWindow &window, ModelTriangle sur, glm::vec3 *c, glm::mat3 *o, float *f, float s, float **&d, TextureMap texture) {
    CanvasPoint p0 = CanvasPoint((WIDTH/2) * sur.vertices[0].x, (HEIGHT/2) * sur.vertices[0].y);
    p0.texturePoint = TexturePoint(  ((sur.texturePoints[0].x) * texture.width),  ((sur.texturePoints[0].y) * texture.height) );
    p0.depth = 100 * sur.vertices[0].z;

    CanvasPoint p1 = CanvasPoint((WIDTH/2) * sur.vertices[1].x, (HEIGHT/2) * sur.vertices[1].y);
    p1.texturePoint = TexturePoint(  ((sur.texturePoints[1].x) * texture.width),  ((sur.texturePoints[1].y) * texture.height) );
    p1.depth = 100 * sur.vertices[1].z;

    CanvasPoint p2 = CanvasPoint((WIDTH/2) * sur.vertices[2].x, (HEIGHT/2) * sur.vertices[2].y);
    p2.texturePoint = TexturePoint(  ((sur.texturePoints[2].x) * texture.width),  ((sur.texturePoints[2].y) * texture.height) );
    p2.depth = 100 * sur.vertices[2].z;

    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    TexturePoint tp0 = p0.texturePoint, tp2 = p2.texturePoint;
    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),
                                 p1.y,
                                 ((p1.y-p0.y)*p2.depth + (p2.y-p1.y)*p0.depth)/(p2.y-p0.y));

    if (p2.x != p0.x){
        float t = (pk.x - p0.x) / (p2.x - p0.x);
        pk.texturePoint.x = tp0.x + t * (tp2.x - tp0.x);
        pk.texturePoint.y = tp0.y + t * (tp2.y - tp0.y);
    } else {
        float t = (pk.y - p0.y) / (p2.y - p0.y);
        pk.texturePoint.x = tp0.x;
        pk.texturePoint.y = tp0.y + t * (tp2.y - tp0.y);
    }

    flatTriangleTextureSurfaceFill(window, p0, pk, p1, c, o, f, s, texture, d);
    textureSurfaceLineDraw(window, p1, pk, c, o, f, s, texture, d);
    flatTriangleTextureSurfaceFill(window, p2, pk, p1, c, o, f, s, texture, d);

    //    uint32_t intColour = textureMap.pixels[textureResult.x + textureResult.y * textureMap.width];
    //    Colour((intColour >> 16) & 0xFF, (intColour >> 8) & 0xFF, intColour & 0xFF);
}


void objVerticesDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 c, glm::mat3 o, float f, float s) {
    CanvasPoint v;
    for (int i = 0; i < static_cast<int>(obj.size()); ++ i) {
        for (int ii = 0; ii < 3; ++ ii){
            v = getCanvasIntersectionPoint(c, o, obj[i].vertices[ii], f, s);
            std::cout << v << std::endl;
            window.setPixelColour(v.x, v.y, obj[i].colour);
            std::cout << obj[i].colour << std::endl;
        }
    }
}


void objEdgeDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 c, glm::mat3 o, float f, float s, float** &d) {
    for (int i = 0; i < static_cast<int>(obj.size()); ++ i) {
        CanvasPoint v1 = getCanvasIntersectionPoint(c, o, obj[i].vertices[0], f, s);
        CanvasPoint v2 = getCanvasIntersectionPoint(c, o, obj[i].vertices[1], f, s);
        CanvasPoint v3 = getCanvasIntersectionPoint(c, o, obj[i].vertices[2], f, s);
        strokedTriangleDraw(window, CanvasTriangle(v1, v2, v3), obj[i].colour, d);
    }
}


void objFaceDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3* c, glm::mat3* o, float* f, float s, float** &d, const std::string &filename) {
    window.clearPixels();
    for (size_t i = 0; i < obj.size(); ++ i) {
        if (obj[i].texturePoints[0].x == -1){
            CanvasPoint v1 = getCanvasIntersectionPoint(*c, *o, obj[i].vertices[0], *f, s);
            CanvasPoint v2 = getCanvasIntersectionPoint(*c, *o, obj[i].vertices[1], *f, s);
            CanvasPoint v3 = getCanvasIntersectionPoint(*c, *o, obj[i].vertices[2], *f, s);
            filledTriangleDraw(window, CanvasTriangle(v1, v2, v3), obj[i].colour, d);
        } else {
            TextureMap texture = TextureMap(filename);
            texturedSurfaceDraw(window, obj[i], c, o, f, s, d, texture);
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