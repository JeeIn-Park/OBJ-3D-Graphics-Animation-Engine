#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include "glm/glm.hpp"
#include <CanvasTriangle.h>

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

std::vector<CanvasPoint>  lineDraw(CanvasPoint from, CanvasPoint to, std::vector<CanvasPoint> line){
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;

//    float xDiff;
//    if (to.x > from.x){
//        xDiff = to.x - from.x;
//    } else {
//        xDiff = from.x - to.x;
//    };
//
//    float yDiff = to.y - from.y;
//    if (to.y > from.y){
//        yDiff = to.y - from.y;
//    } else {
//        yDiff = from.y - to.y;
//    };

    float numberOfSteps;
    if (xDiff > yDiff){
        numberOfSteps = abs(xDiff);
    } else {
        numberOfSteps = abs(yDiff);
    };

    float xStepSize = xDiff/numberOfSteps;
    float yStepSize = yDiff/numberOfSteps;

    CanvasPoint point;
    for (int i = 0; i < numberOfSteps; ++i ) {
        point.x = from.x + (xStepSize*i);
        point.y = from.y + (yStepSize*i);
        line.push_back(point);
    }

    return line;
}

void strokedTriangle (DrawingWindow &window) {
    CanvasTriangle triangle;
    std::vector<CanvasPoint> line;

    triangle.v0().x = rand() % window.width;
    triangle.v0().y = rand() % window.height;
    triangle.v1().x = rand() % window.width;
    triangle.v1().y = rand() % window.height;
    triangle.v2().x = rand() % window.width;
    triangle.v2().y = rand() % window.height;
    line = lineDraw(triangle.v0(), triangle.v1(), line);
    line = lineDraw(triangle.v1(), triangle.v2(), line);
    line = lineDraw(triangle.v0(), triangle.v2(), line);

    uint32_t colour = (rand() % 256 << 24) + (rand() % 256 << 16) + (rand() % 256 << 8) + rand() % 256;

    for (int i = 0; i < line.size(); ++i) {
        CanvasPoint point = line[i];
        window.setPixelColour(point.x, point.y,colour);
    }
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

      // line draw
//    std::vector<CanvasPoint> line;
//    // topLeft_centre
//            line = lineDraw(
//            CanvasPoint(0,0),
//            CanvasPoint((window.width/2), (window.height/2)), line);
//    // topRight_centre
//            line = lineDraw(
//            CanvasPoint(window.width-1, 0),
//            // CanvasPoint(window.width-1, 0),
//            // 320,0 not on visible screen area
//            CanvasPoint((window.width/2), (window.height/2)), line);
//    // middle
//            line = lineDraw(
//            CanvasPoint((window.width/2), 0),
//            CanvasPoint((window.width/2),window.height), line);
//    // third_horizontal
//            line = lineDraw(
//            CanvasPoint((window.width/3), (window.height/2)),
//            CanvasPoint(2*(window.width/3), (window.height/2)), line);
//
//    for ( int i = 0; i < line.size() ; ++ i){
//        CanvasPoint point = line[i];
//        // uint32_t colour = (255 << 24) + (255 << 16) + (255 << 8) + 255;
//        window.setPixelColour(point.x, point.y, (255 << 24) + (255 << 16) + (255 << 8) + 255);
//    };

}


void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
        else if (event.key.keysym.sym == SDLK_u) strokedTriangle(window);
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
