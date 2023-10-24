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

void textureLoading(const std::string &filename){
    TextureMap textureMap = TextureMap(filename);
    //std::vector<std::vector<Colour>> points;
    Colour points[textureMap.height][textureMap.width];
    Colour colour;
    for (int i = 0; i < textureMap.pixels.size(); ++ i){
        int y = i/textureMap.width;
        int x = i - (textureMap.width * y);

        uint8_t intColour = textureMap.pixels[i];
        int blue = intColour & 0xFF;
        int green = (intColour >> 8) & 0xFF;
        int red = (intColour >> 16) & 0xFF;
     //    (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue
        colour.blue = blue;
        colour.green = green;
        colour.red = red;

        points[y][x] = colour;
    }
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

//    std::vector<CanvasPoint> l;
//    CanvasPoint point;
//    for (int i = 0; i < numberOfSteps; ++i ) {
//        point.x = from.x + (xStepSize*i);
//        point.y = from.y + (yStepSize*i);
//        l.push_back(point);
//    }
//    l.push_back(to);
//    for (int i = 0; i < l.size(); ++i) {
//        CanvasPoint point = l[i];
//        window.setPixelColour(point.x, point.y, colour);
//    }

    CanvasPoint point;
    for (int i = 0; i < numberOfSteps; ++i ) {
        point.x = from.x + (xStepSize*i);
        point.y = from.y + (yStepSize*i);
        window.setPixelColour(point.x, point.y, colour);
    }
}


void strokedTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    lineDraw(window, triangle.v0(), triangle.v1(), colour);
    lineDraw(window, triangle.v1(), triangle.v2(), colour);
    lineDraw(window, triangle.v0(), triangle.v2(), colour);
}


void filledTriangle (DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    CanvasPoint p0 = triangle.v0(), p1 = triangle.v1(), p2 = triangle.v2();
    // sort points
    if (p0.y > p1.y)   std::swap(p0, p1);
    if (p1.y > p2.y)   std::swap(p1, p2);
    if (p0.y > p1.y)   std::swap(p0, p1);

    CanvasPoint pk = CanvasPoint(((p1.y-p0.y)*p2.x + (p2.y-p1.y)*p0.x)/(p2.y-p0.y),p1.y);

    float xDiff_k =  pk.x - p0.x;
    float xDiff_1 =  p1.x - p0.x;
    float yDiff = p1.y - p0.y;
    float numberOfSteps = std::max(abs(xDiff_k), abs(xDiff_1));
    numberOfSteps = std::max(numberOfSteps, yDiff);

    float xStepSize_k = xDiff_k/numberOfSteps;
    float xStepSize_1 = xDiff_1/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;

    float x1, x2, y;
    for (int i = 0; i < numberOfSteps; ++i ) {
        x1 = p0.x + (xStepSize_k*i);
        x2 = p0.x + (xStepSize_1*i);
        y = p0.y + (yStepSize*i);
        lineDraw(window, CanvasPoint(x1, y), CanvasPoint(x2, y), colour);
    }

    lineDraw(window, p1, pk, colour);

    xDiff_k =  pk.x - p2.x;
    xDiff_1 =  p1.x - p2.x;
    yDiff = p1.y - p2.y;
    numberOfSteps = std::max(abs(xDiff_k), abs(xDiff_1));
    numberOfSteps = std::max(numberOfSteps, abs(yDiff));

    xStepSize_k = xDiff_k/numberOfSteps;
    xStepSize_1 = xDiff_1/numberOfSteps;
    yStepSize = yDiff/numberOfSteps;

    for (int i = 0; i < numberOfSteps; ++i ) {
        x1 = p2.x + (xStepSize_k*i);
        x2 = p2.x + (xStepSize_1*i);
        y = p2.y + (yStepSize*i);
        lineDraw(window, CanvasPoint(x1, y), CanvasPoint(x2, y), colour);
    }

    Colour white = Colour(255, 255, 255);
    strokedTriangle(window, triangle, white);
}


void draw(DrawingWindow &window) {
//	window.clearPixels();
//    glm::vec3 red(255, 0, 0);
//    glm::vec3 blue(0, 0, 255);
//    glm::vec3 green(0, 255, 0);
//    glm::vec3 yellow(255, 255, 0);

    // gradation rainbow
//    std::vector<glm::vec3> from_list = interpolateThreeElementValues(topLeft, bottomLeft, window.width);
//    std::vector<glm::vec3> to_list = interpolateThreeElementValues(topRight, bottomRight, window.width);
//    for (size_t y = 0; y < window.height; y++) {
//        std::vector<glm::vec3> colour_gradation = interpolateThreeElementValues(  from_list[y], to_list[y], window.width);
//		for (size_t x = 0; x < window.width; x++) {
//            glm::vec3 point = colour_gradation[x];
//            uint32_t colour = (255 << 24) + (int(point.x) << 16) + (int(point.y) << 8) + int(point.z);
//			window.setPixelColour(x, y, colour);
//		}
//	}

    Colour white = Colour(255, 255, 255);
    // topLeft_centre
    lineDraw( window,
              CanvasPoint(0,0),
              CanvasPoint((window.width/2), (window.height/2)), white);
    // topRight_centre
    lineDraw( window,
              CanvasPoint(window.width-1, 0),
            // CanvasPoint(window.width-1, 0),
            // 320,0 not on visible screen area
              CanvasPoint((window.width/2), (window.height/2)), white);
    // middle
    lineDraw( window,
              CanvasPoint((window.width/2), 0),
              CanvasPoint((window.width/2),window.height-1), white);
    // third_horizontal
    lineDraw( window,
              CanvasPoint((window.width/3), (window.height/2)),
              CanvasPoint(2*(window.width/3), (window.height/2)), white);
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

    // check interpolateSingleFloats
//    std::vector<float> result;
//    result = interpolateSingleFloats(2.2, 8.5, 7);
//    for(size_t i=0; i<result.size(); i++) std::cout << result[i] << " ";
//    std::cout << std::endl;

    // check interpolateThreeElementValues
//    std::vector<glm::vec3> result3;
//    glm::vec3 from(1.0, 4.0, 9.2);
//    glm::vec3 to(4.0, 1.0, 9.8);
//    result3 = interpolateThreeElementValues(from, to, 4);
//    for(size_t i=0; i<result.size(); i++) std::cout << result[i];
//    std::cout << std::endl;
    // Print the result to the command line
//    for (const auto& vec : result3) {
//        std::cout << "Result: (" << vec.x << ", " << vec.y << ", " << vec.z << ")" << std::endl;
//    }

    while (true) {
        // We MUST poll for events - otherwise the window will freeze !
        if (window.pollForInputEvents(event)) handleEvent(event, window);
        // draw(window);
        // Need to render the frame at the end, or nothing actually gets shown on the screen !
        window.renderFrame();
    }
}