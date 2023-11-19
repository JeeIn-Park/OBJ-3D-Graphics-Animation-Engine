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
#include <RayTriangleIntersection.h>

// TODO : check if it's allowed to use this library
#include <unordered_map>
#include <sstream>
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

        // TODO : use object name
        if (token == "o"){
            assignTexture = false;
            textures.clear();
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
//            std::cout << vertices.size() << std::endl;
//            std::cout << vertexSetSize << std::endl;
//            std::cout << texture.z << std::endl;
        }


            // f - face
        else if (token == "f") {
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
                vertexIndices[i] = std::stoi(vertex) -1;
            }

            // when all vertex indices are valid
            if (vertexIndices[0] >= 0 && vertexIndices[1] >= 0 && vertexIndices[2] >= 0) {
                ModelTriangle triangle;
                for (int i = 0; i < 3; ++i) {
                    triangle.vertices[i] = vertices[vertexIndices[i]];
                    if (assignTexture){
                        for (size_t j = 0; j < textures.size(); ++j){
                            if (textures[j].z == vertexIndices[i]){
                                triangle.texturePoints[i] = TexturePoint(textures[j].x, textures[j].y);
//                            std::cout << "texture assigned" << std::endl;
                            }
                        }
                    } else {
                        triangle.texturePoints[i] = TexturePoint(-1, -1);
                    }
                }
                triangle.colour = currentColour;
                triangles.push_back(triangle);
            }


//            int bookMark = 0;
//            int currentTextureBook = textures[0].z;
//            for (size_t i = 0; i < textures.size(); ++i) {
//                if (textures[i].z == currentTextureBook){
//
//                } else {
//                    bookMark = 0;
//                    currentTextureBook = textures[i].z;
//
//                    //TODO : make it possible to assign texture point if there are multiple textures as well
//                }
//            }
        }

    }
    objFile.close();

//    for (const glm::vec3 & vertex : vertices) {
//        std::cout << vertex.x << "," << vertex.y << "," << vertex.z << std::endl;
//    }
//    for (const ModelTriangle& triangle : triangles) {
//        std::cout << triangle << std::endl;
//    }
    return triangles;
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

CanvasPoint getTexturedCanvasIntersectionPoint(glm::vec3 c, glm::mat3 o, glm::vec3 v, TexturePoint t, float f, float s, TextureMap textureMap) {
    glm::vec3 rv = o * (v - c);
    CanvasPoint result = CanvasPoint(s / 2 * -f * rv.x / rv.z + WIDTH / 2,
                                     s / 2 *  f * rv.y / rv.z + HEIGHT / 2,
                                     1 / -rv.z);

    glm::vec3 rt = glm::vec3(2 * (t.x -0.5), 2 * (t.y -0.5), v.z);
    rt = o * (rt - c);
    CanvasPoint textureResult = CanvasPoint( (textureMap.height/2) * -f * rt.x / rt.z + textureMap.width/2,
                                             (textureMap.height/2) *  f * rt.y / rt.z + textureMap.height/2);
    result.texturePoint = TexturePoint(textureResult.x, textureResult.y);

//    uint32_t intColour = textureMap.pixels[textureResult.x + textureResult.y * textureMap.width];
//    Colour((intColour >> 16) & 0xFF, (intColour >> 8) & 0xFF, intColour & 0xFF);
    return result;
}


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

void textureSurfaceLineDraw(DrawingWindow &window, CanvasPoint from, CanvasPoint to, TextureMap texture, float** d){
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
//        CanvasPoint v = getTexturedCanvasIntersectionPoint(*c, *o, sur.vertices[0], sur.texturePoints[0], *f, s, texture);

        textureSurfaceLineDraw(window, from, to, texture, d);
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
    pk.texturePoint.y = tp0.y + t * (tp2.y - tp0.y);


    flatTriangleTextureFill(window, p0, pk, p1, texture, d);
    textureDraw(window, p1, pk, texture, d);
    flatTriangleTextureFill(window, p2, pk, p1, texture, d);
    Colour white = Colour(255, 255, 255);
}


void texturedSurfaceDraw(DrawingWindow &window, ModelTriangle sur, glm::vec3 *c, glm::mat3 *o, float *f, float s, float **&d, TextureMap texture) {
    CanvasPoint v1 = getTexturedCanvasIntersectionPoint(*c, *o, sur.vertices[0], sur.texturePoints[0], *f, s, texture);
    CanvasPoint v2 = getTexturedCanvasIntersectionPoint(*c, *o, sur.vertices[1], sur.texturePoints[1], *f, s, texture);
    CanvasPoint v3 = getTexturedCanvasIntersectionPoint(*c, *o, sur.vertices[2], sur.texturePoints[2], *f, s, texture);

    CanvasPoint p0 = CanvasPoint(sur.vertices[0].x, sur.vertices[0].y);
    p0.texturePoint = sur.texturePoints[0];
    p0.depth = sur.vertices[0].z;
    CanvasPoint p1 = CanvasPoint(sur.vertices[1].x, sur.vertices[1].y);
    p1.texturePoint = sur.texturePoints[1];
    p1.depth = sur.vertices[1].z;
    CanvasPoint p2 = CanvasPoint(sur.vertices[2].x, sur.vertices[2].y);
    p2.texturePoint = sur.texturePoints[2];
    p2.depth = sur.vertices[2].z;

    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    TexturePoint tp0 = p0.texturePoint, tp2 = p2.texturePoint;
    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),p1.y);
    float t = (pk.x - p0.x) / (p2.x - p0.x);
    pk.texturePoint.y = tp0.y + t * (tp2.y - tp0.y);

    flatTriangleTextureSurfaceFill(window, p0, pk, p1, c, o, f, s, texture, d);
    textureDraw(window, p1, pk, texture, d);
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
//            CanvasPoint v1 = getTexturedCanvasIntersectionPoint(*c, *o, obj[i].vertices[0], obj[i].texturePoints[0], *f, s);
//            CanvasPoint v2 = getTexturedCanvasIntersectionPoint(*c, *o, obj[i].vertices[1], obj[i].texturePoints[1], *f, s);
//            CanvasPoint v3 = getTexturedCanvasIntersectionPoint(*c, *o, obj[i].vertices[2], obj[i].texturePoints[2], *f, s);
//            v1.texturePoint = obj[i].texturePoints[0];
//            v2.texturePoint = obj[i].texturePoints[1];
//            v3.texturePoint = obj[i].texturePoints[2];
//
//            // TODO : change it to use other files as well
//            // TODO : get texture canvas point
//                // for each point to be drawn on the canvas, find relevant 3D coordinate of the texture point, find a canvas point, draw it on screen
//            // TODO : assign it in texturedTriangleDraw before it call
////            std::cout << "texture triangle" << std::endl;
//            texturedTriangleDraw(window, CanvasTriangle(v1, v2, v3), "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm", d);
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