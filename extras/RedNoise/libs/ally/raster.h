#pragma once

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

#pragma once

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

int positiveOrNegative(float value);
CanvasPoint getCanvasIntersectionPoint (glm::vec3 c, glm::mat3 o, glm::vec3 v, float f, float s);
void lineDraw(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour, float** &d);
void textureDraw (DrawingWindow &window, CanvasPoint from, CanvasPoint to, TextureMap texture, float** d);
void textureSurfaceLineDraw(DrawingWindow &window, CanvasPoint from, CanvasPoint to,  glm::vec3 *c, glm::mat3 *o, float *f, float s, TextureMap texture, float** d);
void strokedTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour, float** &d);
void flatTriangleColourFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2, Colour colour, float** &d);
void flatTriangleTextureFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2, TextureMap texture, float** &d);
void flatTriangleTextureSurfaceFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2, glm::vec3 *c, glm::mat3 *o, float *f, float s, TextureMap texture, float** &d);
void filledTriangleDraw (DrawingWindow &window, CanvasTriangle triangle, Colour colour, float** &d) ;
void texturedTriangleDraw(DrawingWindow &window, CanvasTriangle triangle, const std::string &filename, float** &d);
void texturedSurfaceDraw(DrawingWindow &window, ModelTriangle sur, glm::vec3 *c, glm::mat3 *o, float *f, float s, float **&d, TextureMap texture);
void objVerticesDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 c, glm::mat3 o, float f, float s) ;
void objEdgeDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 c, glm::mat3 o, float f, float s, float** &d);
void objFaceDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3* c, glm::mat3* o, float* f, float s, float** &d, const std::string &filename);