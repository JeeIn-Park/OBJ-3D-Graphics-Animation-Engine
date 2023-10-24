#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <TextureMap.h>
#include <TexturePoint.h>


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

std::vector<std::vector<Colour>> textureLoading(const std::string &filename){
    TextureMap textureMap = TextureMap(filename);
    std::vector<std::vector<Colour>> pixesList;

    //Colour points[textureMap.height][textureMap.width];
    Colour colour;

    for (int y = 0; y < textureMap.height; ++y) {
        std::vector<Colour> pixes;
        for (int x = 0; x < textureMap.width; ++x) {
            uint32_t intColour = textureMap.pixels[y*textureMap.width + x];
            int blue = intColour & 0xFF;
            int green = (intColour >> 8) & 0xFF;
            int red = (intColour >> 16) & 0xFF;
            colour.blue = blue;
            colour.green = green;
            colour.red = red;
            pixes.push_back(colour);
        }
        pixesList.push_back(pixes);
    }
    return pixesList;
}


CanvasTriangle randomTriangle(DrawingWindow &window){
    CanvasTriangle triangle;

    triangle.v0().x = rand() % window.width;
    triangle.v0().y = rand() % window.height;
    triangle.v1().x = rand() % window.width;
    triangle.v1().y = rand() % window.height;
    while ((triangle.v0().x == triangle.v1().x) && ((triangle.v0().y == triangle.v1().y))) {
        triangle.v1().x = rand() % window.width;
        triangle.v1().y = rand() % window.height;
    }
    triangle.v2().x = rand() % window.width;
    triangle.v2().y = rand() % window.height;
    while (((triangle.v0().x == triangle.v2().x) && ((triangle.v0().y == triangle.v2().y)))
           || ((triangle.v1().x == triangle.v2().x) && ((triangle.v1().y == triangle.v2().y)))) {
        triangle.v2().x = rand() % window.width;
        triangle.v2().y = rand() % window.height;
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

void textureDraw (DrawingWindow &window, CanvasPoint from, CanvasPoint to, std::vector<std::vector<Colour>> texture){
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;
    float t_xDiff = to.texturePoint.x - from.texturePoint.x;
    float t_yDiff = to.texturePoint.y - from.texturePoint.y;

    float numberOfSteps = std::max(std::max(abs(xDiff), abs(yDiff)), std::max(abs(t_xDiff), abs(t_yDiff)));

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;
    float t_xStepSize = t_xDiff/numberOfSteps;
    float t_yStepSize = t_yDiff/numberOfSteps;

    CanvasPoint point;
    Colour colour;
    for (int i = 0; i < numberOfSteps; ++i ) {
        point.x = from.x + (xStepSize*i);
        point.y = from.y + (yStepSize*i);
        colour = texture[from.texturePoint.x + (t_xStepSize*i)][from.texturePoint.y + (t_yStepSize*i)];
        window.setPixelColour(point.x, point.y, colour);
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
                          std::vector<std::vector<Colour>> texture){
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
    numberOfSteps = std::max(std::max(abs(t_xDiff_1), abs(t_xDiff_2)), std::max(abs(t_yDiff), numberOfSteps));

    float t_xStepSize_1 = xDiff_1/numberOfSteps;
    float t_xStepSize_2 = xDiff_2/numberOfSteps;
    float t_yStepSize = yDiff/numberOfSteps;

    CanvasPoint from;
    CanvasPoint to;
    for (int i = 0; i < numberOfSteps; ++i ) {
        from.x = top.x + (xStepSize_1*i);
        to.x = top.x + (xStepSize_2*i);
        from.y = top.y + (yStepSize*i);
        to.y = top.y + (yStepSize*i);
        from.texturePoint.x = top.x + (t_xStepSize_1*i);
        to.texturePoint.x = top.x + (t_xStepSize_2*i);
        from.texturePoint.y = top.y + (t_yStepSize*i);
        to.texturePoint.y = top.y + (t_yStepSize*i);

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
    std::vector<std::vector<Colour>> texture = textureLoading(filename);
    CanvasPoint p0 = triangle.v0(), p1 = triangle.v1(), p2 = triangle.v2();
    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),p1.y);
    pk.texturePoint = TexturePoint(((p1.texturePoint.y-p0.texturePoint.y)*p2.texturePoint.x + (p2.texturePoint.y-p1.texturePoint.y)*p0.texturePoint.x)/(p2.texturePoint.y-p0.texturePoint.y),p1.texturePoint.y);

    flatTriangleTexture(window, p0, pk, p1, texture);
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
    CanvasTriangle triangle = CanvasTriangle(v0, v1, v2);
    texturedTriangle(window, triangle, "/home/jeein/Documents/CG/computer_graphics/extras/RedNoise/src/texture.ppm");


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
            strokedTriangle(window, randomTriangle(window), colour);
        }
        else if (event.key.keysym.sym == SDLK_f) {
            filledTriangle(window, randomTriangle(window), colour);
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