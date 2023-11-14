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

//TODO : going to delete this, only for the test
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
        if (token == "Kd") {
            std::array<float, 3> rgb;
            iss >> rgb[0] >> rgb[1] >> rgb[2];
//            std::cout << "RGB: " << rgb[0] << " " << rgb[1] << " " << rgb[2] << std::endl;

            if (rgb[0] >= 0 && rgb[1] >= 0 && rgb[2] >= 0) {
                colourMap[colourName] = Colour(255 * rgb[0], 255 * rgb[1], 255 * rgb[2]);
            }

        }

    }

    mtlFile.close();
    return colourMap;
}


std::vector<ModelTriangle> readOBJ(const std::string &filename, std::unordered_map<std::string, Colour> colourMap, float s){
    std::vector<ModelTriangle> triangles;
    std::vector<glm::vec3> vertices;
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


        // usemtl - colour
        if (token == "usemtl") {
            std::string colourName;
            iss >> colourName;
            currentColour = colourMap[colourName];
        }

        // v - vertex
        else if (token == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(glm::vec3(s * vertex.x, s * vertex.y, s * vertex.z));
        }

        // f - face
        else if (token == "f") {
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
                }
                triangle.colour = currentColour;
                triangles.push_back(triangle);
            }
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
CanvasPoint getCanvasIntersectionPoint (glm::vec3 c, glm::vec3 v, float f, float s) {
    glm::vec3 r = v - c;
    return CanvasPoint(s/2 * -f * r.x/r.z + WIDTH/2, s/2 * f * r.y/r.z + HEIGHT/2, 1/-r.z);
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


    flatTriangleTextureFill(window, p0, pk, p1, texture);
    textureDraw(window, p1, pk, texture);
    flatTriangleTextureFill(window, p2, pk, p1, texture);
    Colour white = Colour(255, 255, 255);
    strokedTriangleDraw(window, triangle, white, d);
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


void objEdgeDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 c, float f, float s, float** &d) {
    for (int i = 0; i < static_cast<int>(obj.size()); ++ i) {
        CanvasPoint v1 = getCanvasIntersectionPoint(c, obj[i].vertices[0], f, s);
        CanvasPoint v2 = getCanvasIntersectionPoint(c, obj[i].vertices[1], f, s);
        CanvasPoint v3 = getCanvasIntersectionPoint(c, obj[i].vertices[2], f, s);
        strokedTriangleDraw(window, CanvasTriangle(v1, v2, v3), obj[i].colour, d);
    }
}


void objFaceDraw(DrawingWindow &window, std::vector<ModelTriangle> obj, glm::vec3 *c, float* f, float s, float** &d) {
    window.clearPixels();
    for (int i = 0; i < static_cast<int>(obj.size()); ++ i) {
        CanvasPoint v1 = getCanvasIntersectionPoint(*c, obj[i].vertices[0], *f, s);
        CanvasPoint v2 = getCanvasIntersectionPoint(*c, obj[i].vertices[1], *f, s);
        CanvasPoint v3 = getCanvasIntersectionPoint(*c, obj[i].vertices[2], *f, s);
        filledTriangleDraw(window, CanvasTriangle(v1, v2, v3), obj[i].colour, d);
    }
}


void rotate(glm::vec3* c, char t){
    double o = 1.0 * M_PI / 180.0;
    glm::vec3 result;
    if (t == 'a'){
        result = glm::mat3 (
                cos(o), 0, -sin(o),
                0, 1, 0,
                sin(o), 0, cos(o)
        ) * *c;
    }
    else if (t == 'd'){
        result = glm::mat3 (
                cos(-o), 0, -sin(-o),
                0, 1, 0,
                sin(-o), 0, cos(-o)
        ) * *c;
    }
    else if (t == 'w'){
        result = glm::mat3 (
                1, 0, 0,
                0, cos(o), sin(o),
                0, -sin(o), cos(o)
                ) * *c;
    }
    else if (t == 's'){
        result = glm::mat3 (
                1, 0, 0,
                0, cos(-o), sin(-o),
                0, -sin(-o), cos(-o)
        ) * *c;
    }
    (*c).x = result.x; (*c).y = result.y; (*c).z = result.z;
}


bool handleEvent(SDL_Event event, DrawingWindow &window, glm::vec3* c, float** &d) {
    float translate = 0.07;

    Colour colour(rand() % 256, rand() % 256, rand() % 256);
    if (event.type == SDL_KEYDOWN) {
        // translate
        if (event.key.keysym.sym == SDLK_LEFT) {(*c).x =  (*c).x + translate;}
        else if (event.key.keysym.sym == SDLK_RIGHT) {(*c).x =  (*c).x - translate;}
        else if (event.key.keysym.sym == SDLK_UP) {(*c).y =  (*c).y + translate;}
        else if (event.key.keysym.sym == SDLK_DOWN) {(*c).y =  (*c).y - translate;}
        else if (event.key.keysym.sym == SDLK_KP_MINUS) {(*c).z =  (*c).z + translate;}
        else if (event.key.keysym.sym == SDLK_KP_PLUS) {(*c).z =  (*c).z - translate;}

        //rotate
        else if (event.key.keysym.sym == SDLK_a) rotate(c, 'a');
        else if (event.key.keysym.sym == SDLK_d) rotate(c, 'd');
        else if (event.key.keysym.sym == SDLK_w) rotate(c, 'w');
        else if (event.key.keysym.sym == SDLK_s) rotate(c, 's');

//        else if (event.key.keysym.sym == SDLK_c) window.clearPixels();
        else if (event.key.keysym.sym == SDLK_q) return true;

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

    std::unordered_map<std::string, Colour> mtl = readMTL("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.mtl");
    std::vector<ModelTriangle> obj = readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.obj", mtl, 0.35);

    glm::vec3* cameraToVertex = new glm::vec3 (0.0, 0.0, 4.0);
    float* f = new float(2.0);

    // TODO : study heap/memory allocation
    // TODO : study pointer
    float** depthBuffer = new float*[WIDTH];
    for (int i = 0; i < WIDTH; ++i) {
        depthBuffer[i] = new float [HEIGHT];
    }
//    for (int i = 0; i < WIDTH; ++i) {
//        for (int j = 0; j < HEIGHT; ++j) {
//            depthBuffer[i][j] = 0;
//        }
//    }

    glm::mat3* cameraOrientation = new glm::mat3(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
            );
    glm::vec3* adjustedVector = new glm::vec3;
    *adjustedVector = *cameraToVertex * *cameraOrientation;
    double o = 1.0 * M_PI / 180.0;

    while (!terminate) {
        if (window.pollForInputEvents(event)) terminate = handleEvent(event, window, adjustedVector, depthBuffer);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (int i = 0; i < WIDTH; ++i) {
            for (int j = 0; j < HEIGHT; ++j) {
                depthBuffer[i][j] = 0;
            }
        }
        *cameraOrientation = glm::mat3 (
                1, 0, 0,
                0, cos(o), sin(o),
                0, -sin(o), cos(o)
        ) * *cameraOrientation;
        *adjustedVector = *cameraToVertex * *cameraOrientation;
        objFaceDraw(window, obj, adjustedVector, f, 240, depthBuffer);
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
    delete adjustedVector;
}