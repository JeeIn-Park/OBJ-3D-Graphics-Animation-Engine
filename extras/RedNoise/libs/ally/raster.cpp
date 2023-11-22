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
// TODO : check if I can include this library
#include <sstream>

#include "raster.h"

#define WIDTH 320
#define HEIGHT 240



int positiveOrNegative(float value) {
    if (value > 0) {
        return 1;
    } else if (value < 0) {
        return -1;
    } else {
        return 0;
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
    int corrector = positiveOrNegative(c.z);
    return CanvasPoint(s/2 * f * corrector * r.x/r.z + WIDTH/2, s/2 * f * corrector * r.y/r.z + HEIGHT/2, 1/-r.z);
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
    glm::vec3 vecFrom = *o * (glm::vec3(from.x, from.y, from.depth) - *c);
    CanvasPoint canvasFrom = CanvasPoint(s / 2 * -*f * vecFrom.x / vecFrom.z + WIDTH / 2, s / 2 * *f * vecFrom.y / vecFrom.z + HEIGHT / 2);
    glm::vec3 vecTo = *o * (glm::vec3(to.x, to.y, to.depth) - *c);
    CanvasPoint canvasTo = CanvasPoint(s / 2 * -*f * vecTo.x / vecTo.z + WIDTH / 2, s / 2 * *f * vecTo.y / vecTo.z + HEIGHT / 2);

    float CxDiff = canvasTo.x - canvasFrom.x;
    float CyDiff = canvasTo.y - canvasFrom.y;
    float dDiff = to.depth - from.depth;
    float numberOfSteps = std::max(std::max(abs(CxDiff), abs(CyDiff)), abs(dDiff));

    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize = dDiff/numberOfSteps;

    float t_xDiff = to.texturePoint.x - from.texturePoint.x;
    float t_yDiff = to.texturePoint.y - from.texturePoint.y;
    float t_xStepSize = t_xDiff/numberOfSteps;
    float t_yStepSize = t_yDiff/numberOfSteps;

    uint32_t intColour;
    for (int i = 0; i < numberOfSteps; ++i ) {
        CanvasPoint v = CanvasPoint(from.x + (xStepSize*i), from.y + (yStepSize*i));
        v.texturePoint.x = from.texturePoint.x + t_xStepSize*i;
        v.texturePoint.y = from.texturePoint.y + t_yStepSize*i;
        v.depth = from.depth + (dStepSize*i);
        intColour = texture.pixels[int(v.texturePoint.x) + int(v.texturePoint.y) * texture.width];
        glm::vec3 vertex = glm::vec3 (v.x, v.y, v.depth);
        v = getCanvasIntersectionPoint(*c, *o, vertex, *f, s);
        int intX = static_cast<int>(v.x);
        int intY = static_cast<int>(v.y);
        if ((intX > 0 && intX < WIDTH && intY > 0 && intY < HEIGHT) && v.depth >= d[intX][intY]) {
            Colour colour = Colour((intColour >> 16) & 0xFF, (intColour >> 8) & 0xFF, intColour & 0xFF);
            window.setPixelColour(intX, intY, colour);
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
    float t_yDiff_1 = bot1.texturePoint.y - top.texturePoint.y;
    float t_yDiff_2 = bot2.texturePoint.y - top.texturePoint.y;

    float t_xStepSize_1 = t_xDiff_1/numberOfSteps;
    float t_xStepSize_2 = t_xDiff_2/numberOfSteps;
    float t_yStepSize_1 = t_yDiff_1/numberOfSteps;
    float t_yStepSize_2 = t_yDiff_2/numberOfSteps;

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
        from.texturePoint.y = top.texturePoint.y + (t_yStepSize_1*i);
        to.texturePoint.y = top.texturePoint.y + (t_yStepSize_2*i);

        textureDraw(window, from, to, texture, d);
    }
}

void flatTriangleTextureSurfaceFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2,
                                     glm::vec3 *c, glm::mat3 *o, float *f, float s, TextureMap texture, float** &d){

    glm::vec3 vecTop = *o * (glm::vec3(top.x, top.y, top.depth) - *c);
    CanvasPoint canvasTop = CanvasPoint(s / 2 * -*f * vecTop.x / vecTop.z + WIDTH / 2, s / 2 * *f * vecTop.y / vecTop.z + HEIGHT / 2);
    glm::vec3 vecBot1 = *o * (glm::vec3(bot1.x, bot1.y, bot1.depth) - *c);
    CanvasPoint canvasBot1 = CanvasPoint(s / 2 * -*f * vecBot1.x / vecBot1.z + WIDTH / 2, s / 2 * *f * vecBot1.y / vecBot1.z + HEIGHT / 2);
    glm::vec3 vecBot2 = *o * (glm::vec3(bot2.x, bot2.y, bot2.depth) - *c);
    CanvasPoint canvasBot2 = CanvasPoint(s / 2 * -*f * vecBot2.x / vecBot2.z + WIDTH / 2, s / 2 * *f * vecBot2.y / vecBot2.z + HEIGHT / 2);

    float CxDiff_1 = canvasBot1.x - canvasTop.x;
    float CxDiff_2 = canvasBot2.x - canvasTop.x;
    float CyDiff = canvasBot1.y - canvasTop.y;
    float dDiff_1 = bot1.depth - top.depth;
    float dDiff_2 = bot2.depth - top.depth;

    float numberOfSteps = std::max(std::max(std::max(abs(CxDiff_1), abs(CxDiff_2)), std::max(abs(dDiff_1), abs(dDiff_2))), abs(CyDiff));

    float xDiff_1 = bot1.x - top.x;
    float xDiff_2 = bot2.x - top.x;
    float yDiff = bot1.y - top.y;

    float xStepSize_1 = xDiff_1/numberOfSteps;
    float xStepSize_2 = xDiff_2/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float dStepSize_1 = dDiff_1/numberOfSteps;
    float dStepSize_2 = dDiff_2/numberOfSteps;

    // texture value
    float t_xDiff_1 = bot1.texturePoint.x - top.texturePoint.x;
    float t_xDiff_2 = bot2.texturePoint.x - top.texturePoint.x;
    float t_yDiff_1 = bot1.texturePoint.y - top.texturePoint.y;
    float t_yDiff_2 = bot2.texturePoint.y - top.texturePoint.y;

    float t_xStepSize_1 = t_xDiff_1/numberOfSteps;
    float t_xStepSize_2 = t_xDiff_2/numberOfSteps;
    float t_yStepSize_1 = t_yDiff_1/numberOfSteps;
    float t_yStepSize_2 = t_yDiff_2/numberOfSteps;

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
        from.texturePoint.y = top.texturePoint.y + (t_yStepSize_1*i);
        to.texturePoint.y = top.texturePoint.y + (t_yStepSize_2*i);

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
    CanvasPoint p0 = CanvasPoint(sur.vertices[0].x, sur.vertices[0].y, sur.vertices[0].z);
    p0.texturePoint = TexturePoint(  ((sur.texturePoints[0].x) * texture.width),  ((sur.texturePoints[0].y) * texture.height) );

    CanvasPoint p1 = CanvasPoint(sur.vertices[1].x, sur.vertices[1].y, sur.vertices[1].z);
    p1.texturePoint = TexturePoint(  ((sur.texturePoints[1].x) * texture.width),  ((sur.texturePoints[1].y) * texture.height) );

    CanvasPoint p2 = CanvasPoint(sur.vertices[2].x, sur.vertices[2].y, sur.vertices[2].z);
    p2.texturePoint = TexturePoint(  ((sur.texturePoints[2].x) * texture.width),  ((sur.texturePoints[2].y) * texture.height) );

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