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

    // handle error : when file is not opened
    if (!mtlFile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return colourMap;
    }

    while (getline(mtlFile, line)) {
        std::string colourName;

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        // newmtl - new colour
        if (token == "newmtl") {
            iss >> colourName;
        }

        // Kd - RGB value
        if (token == "Kd") {
            std::string value;
            std::array<int, 3> rgb;

            for (int i = 0; i < 3; ++i) {
                iss >> value;
                rgb[i] = std::stoi(value) * 255;
            }

            if (rgb[0] >= 0 && rgb[1] >= 0 && rgb[2] >= 0) {
                colourMap[colourName] = Colour(rgb[0], rgb[1], rgb[2]);
            }

        }

    }

    mtlFile.close();
    return colourMap;
}

void drawModel (DrawingWindow &window, ModelTriangle triangle){

}

Colour stringToColour (std::string colourName) {
    // TODO : use hashmap instead of adding function which deal with colour
    Colour colour;
    return colour;
}

std::vector<ModelTriangle> readOBJ(const std::string &filename){
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
            //TODO : use hashmap to find colour
            currentColour = stringToColour(colourName);
        }

        // v - vertex
        else if (token == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
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
    return triangles;
}


CanvasTriangle randomTriangle(){
    CanvasTriangle triangle;

    triangle.v0().x = rand() % WIDTH;
    triangle.v0().y = rand() % HEIGHT;
    triangle.v1().x = rand() % WIDTH;
    triangle.v1().y = rand() % HEIGHT;
    while ((triangle.v0().x == triangle.v1().x) && ((triangle.v0().y == triangle.v1().y))) {
        triangle.v1().x = rand() % WIDTH;
        triangle.v1().y = rand() % HEIGHT;
    }
    triangle.v2().x = rand() % WIDTH;
    triangle.v2().y = rand() % HEIGHT;
    while (((triangle.v0().x == triangle.v2().x) && ((triangle.v0().y == triangle.v2().y)))
           || ((triangle.v1().x == triangle.v2().x) && ((triangle.v1().y == triangle.v2().y)))) {
        triangle.v2().x = rand() % WIDTH;
        triangle.v2().y = rand() % HEIGHT;
    }

    return triangle;
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

void strokedTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    lineDraw(window, triangle.v0(), triangle.v1(), colour);
    lineDraw(window, triangle.v1(), triangle.v2(), colour);
    lineDraw(window, triangle.v0(), triangle.v2(), colour);
}

void flatTriangleFill (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2, Colour colour){
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

void flatTriangleTexture (DrawingWindow &window, CanvasPoint top, CanvasPoint bot1, CanvasPoint bot2,
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


void filledTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    CanvasPoint p0 = triangle.v0(), p1 = triangle.v1(), p2 = triangle.v2();
    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),p1.y);

    flatTriangleFill(window, p0, pk, p1, colour);
    lineDraw(window, p1, pk, colour);
    flatTriangleFill(window, p2, pk, p1, colour);

    Colour white = Colour(255, 255, 255);
    strokedTriangle(window, triangle, white);
}


void texturedTriangle(DrawingWindow &window, CanvasTriangle triangle, const std::string &filename){
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


    flatTriangleTexture(window, p0, pk, p1, texture);
    textureDraw(window, p1, pk, texture);
    flatTriangleTexture(window, p2, pk, p1, texture);
    Colour white = Colour(255, 255, 255);
    strokedTriangle(window, triangle, white);
}

void draw(DrawingWindow &window) {
    CanvasPoint v0 = CanvasPoint(160, 10);
    CanvasPoint v1 = CanvasPoint(300, 230);
    CanvasPoint v2 = CanvasPoint(10, 150);
    v0.texturePoint = TexturePoint(195, 5);
    v1.texturePoint = TexturePoint(395, 380);
    v2.texturePoint = TexturePoint(65, 330);
    texturedTriangle(window, CanvasTriangle(v0, v1, v2), "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");

    readOBJ("/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/cornell-box.obj");

//    uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
//    uint32_t red = (255 << 24) + (255 << 16) + (0 << 8) + 0;
//    uint32_t green = (255 << 24) + (0 << 16) + (255 << 8) + 0;
//    uint32_t blue = (255 << 24) + (0 << 16) + (0 << 8) + 255;
//    uint32_t white = (255 << 24) + (255 << 16) + (255 << 8) + 255;

}


void handleEvent(SDL_Event event, DrawingWindow &window) {
    Colour colour(rand() % 256, rand() % 256, rand() % 256);
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
        else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
        else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
        else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
        else if (event.key.keysym.sym == SDLK_u) {
            strokedTriangle(window, randomTriangle(), colour);
        }
        else if (event.key.keysym.sym == SDLK_f) {
            filledTriangle(window, randomTriangle(), colour);
        }
        else if (event.key.keysym.sym == SDLK_c) window.clearPixels();
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        window.savePPM("output.ppm");
        window.saveBMP("output.bmp");
    }
}


int main(int argc, char *argv[]) {
    DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
    SDL_Event event;
    while (true) {
        if (window.pollForInputEvents(event)) handleEvent(event, window);
            draw(window);
        window.renderFrame();
    }
}