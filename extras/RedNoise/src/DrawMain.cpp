#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include<CanvasPoint.h>
#include<Colour.h>
#include<TextureMap.h>
#include <glm/gtc/matrix_transform.hpp>
#include<ModelTriangle.h>
#include <RayTriangleIntersection.h>
#include <unordered_map>

#include <string>
#include<sstream>


#define WIDTH  1000
#define HEIGHT  1000
float scalingFactor = 0.35;
glm::vec3 cameraPosition(0.0, 0.0, 4.0);
float focalLength = 2.0;
float delta = 0.1f;  // 移动或旋转的步长
float orbitAngle = 0.0f;  // 相机围绕轨道中心旋转的当前角度
float orbitRadius = 4.0f; // 相机离轨道中心的距离
glm::vec3 orbitCenter(0.0f, 0.0f, 0.0f); // 假设轨道中心在世界坐标的原点


void drawline(CanvasPoint from, CanvasPoint to, Colour colour, DrawingWindow &window, std::vector<std::vector<float>>& depthBuffer) {
    float xDiff = to.x - from.x;
    float yDiff = to.y - from.y;
    float zDiff = to.depth - from.depth;
    float numSteps = 10 * std::max(std::abs(xDiff), std::abs(yDiff));
    float xStepSize = xDiff / numSteps;
    float yStepSize = yDiff / numSteps;
    float zStepSize = zDiff / numSteps;

    for (float i = 0.0; i < numSteps; i++) {
        float x = from.x + (xStepSize * i);
        float y = from.y + (yStepSize * i);
        float z = -(from.depth + (zStepSize * i));

        int roundedX = round(x);
        int roundedY = round(y);

        // 确保x和y在合适的范围内
        if (roundedX >= 0 && roundedX < window.width && roundedY >= 0 && roundedY < window.height) {
            if (z < depthBuffer[roundedY][roundedX]) {
                uint32_t pixelColor = (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
                window.setPixelColour(roundedX, roundedY, Colour(int(colour.red), int(colour.green), int(colour.blue)));
                depthBuffer[roundedY][roundedX] = z;
            }
        }
    }
}



//void line(DrawingWindow &window){
//    window.clearPixels();
//    CanvasPoint startpoint1 = {160,0};
//    CanvasPoint endpoint1 = {160,240};
//    Colour Colourline1 = {255,255,255};
//
//
//    CanvasPoint startpoint2 = {0,0};
//    CanvasPoint endpoint2 = {160,120};
//
//    drawline(endpoint1,startpoint1,Colourline1,window);
//    drawline(endpoint2,startpoint2,Colourline1,window);
//    CanvasPoint startpoint3 = {320,0};
//    CanvasPoint endpoint3 = {160,120};
//
//    drawline(endpoint3,startpoint3,Colourline1,window);
//
//    CanvasPoint startpoint4 = {320/3,120};
//    CanvasPoint endpoint4 = {2 * 320/3,120};
//
//    drawline(endpoint4,startpoint4,Colourline1,window);
//
//}

//void drawtriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window){
//    drawline(triangle.v0(), triangle.v1(), colour, window);
//    drawline(triangle.v1(), triangle.v2(), colour, window);
//    drawline(triangle.v2(), triangle.v0(), colour, window);
//}
CanvasTriangle generateRandomTriangle(int width = WIDTH, int height = HEIGHT) {
    CanvasPoint v0 = CanvasPoint(rand() % width, rand() % height);
    CanvasPoint v1 = CanvasPoint(rand() % width, rand() % height);
    CanvasPoint v2 = CanvasPoint(rand() % width, rand() % height);//    rand()can give us a range from 0-width or o-height
    return CanvasTriangle(v0,v1,v2);
}

Colour trianglecoulour(int width = WIDTH, int height = HEIGHT){
    int red = rand() % 256;
    int green = rand() % 256;
    int blue = rand() % 256;
    return Colour(red, green, blue);

}



void fillTriangle(CanvasTriangle triangle, Colour colour, Colour outlineColour, DrawingWindow& window, std::vector<std::vector<float>>& depthBuffer) {
    CanvasPoint v0 = triangle.v0();
    CanvasPoint v1 = triangle.v1();
    CanvasPoint v2 = triangle.v2();

    // 按垂直位置排序顶点
    if (v0.y > v1.y) std::swap(v0, v1);
    if (v0.y > v2.y) std::swap(v0, v2);
    if (v1.y > v2.y) std::swap(v1, v2);

    CanvasPoint topVertex = v0;
    CanvasPoint middleVertex = v1;
    CanvasPoint bottomVertex = v2;

    // 计算左边的斜率
    float Slope1 = (middleVertex.x - topVertex.x) / (middleVertex.y - topVertex.y);
    // 计算右边的斜率
    float Slope2 = (bottomVertex.x - topVertex.x) / (bottomVertex.y - topVertex.y);
    float depthSlope1 = (middleVertex.depth - topVertex.depth) / (middleVertex.y - topVertex.y);
    float depthSlope2 = (bottomVertex.depth - topVertex.depth) / (bottomVertex.y - topVertex.y);


    // 初始化左边和右边的x坐标
    float left_x = topVertex.x;
    float right_x = topVertex.x;
    float left_depth = topVertex.depth;
    float right_depth = topVertex.depth;

    // 处理上半部分三角形
    for (float y = topVertex.y; y < middleVertex.y; y++) {
        CanvasPoint from(round(left_x),round(y),left_depth);
        CanvasPoint to(round(right_x),round(y),right_depth);
        drawline(from,to, colour, window, depthBuffer);
        right_x += Slope1;
        left_x += Slope2;
        left_depth += depthSlope2;
        right_depth += depthSlope1;
    }

    right_x = middleVertex.x;
    float slope3 = (bottomVertex.x - middleVertex.x) / (bottomVertex.y - middleVertex.y);
    float depthslope3 = (bottomVertex.depth - middleVertex.depth) / (bottomVertex.y - middleVertex.y);


    // 处理下半部分三角形
    for (float y = middleVertex.y; y <= bottomVertex.y; y++) {
        CanvasPoint from(round(left_x),round(y),left_depth);
        CanvasPoint to(round(right_x),round(y),right_depth);
        drawline(from, to, colour, window, depthBuffer);
        right_x += slope3;
        left_x += Slope2;
        left_depth += depthSlope2;
        right_depth += depthslope3;
    }

    drawline(v0, v1, outlineColour, window, depthBuffer);
    drawline(v1, v2, outlineColour, window, depthBuffer);
    drawline(v2, v0, outlineColour, window, depthBuffer);
}

//void drawTexturedTriangle(CanvasTriangle triangle, TextureMap texture, DrawingWindow& window) {
//    CanvasPoint v0 = triangle.v0();
//    CanvasPoint v1 = triangle.v1();
//    CanvasPoint v2 = triangle.v2();
//
//    // Sort vertices by their y-coordinates
//    if (v0.y > v1.y) {
//        std::swap(v0, v1);
//    }
//    if (v0.y > v2.y) {
//        std::swap(v0, v2);
//    }
//    if (v1.y > v2.y) {
//        std::swap(v1, v2);
//    }
//
//    CanvasPoint top = v0;
//    CanvasPoint middle = v1;
//    CanvasPoint bottom = v2;
//
//    // Calculate slopes for the top and bottom edges
//    float slope1 = (middle.x - top.x) / (middle.y - top.y);
//    float slope2 = (bottom.x - top.x) / (bottom.y - top.y);
//
//    float left_x = top.x;
//    float right_x = top.x;
//
//    // First part of triangle (top to middle)
//    for (int y = top.y; y <= middle.y; ++y) {
//        // Calculate the corresponding texturePoints for CanvasPoints
//        float t1 = (y - top.y) / (middle.y - top.y); //插值系数，用来插值top和middle两个点之间的值
//        float t2 = (y - top.y) / (bottom.y - top.y); //bottom和top两个点之间的值
//        float texX1 = top.texturePoint.x + (middle.texturePoint.x - top.texturePoint.x) * t1;
//        float texX2 = top.texturePoint.x + (bottom.texturePoint.x - top.texturePoint.x) * t2;
//        float texY1 = top.texturePoint.y + (middle.texturePoint.y - top.texturePoint.y) * t1;
//        float texY2 = top.texturePoint.y + (bottom.texturePoint.y - top.texturePoint.y) * t2;
//
//        // Draw the horizontal line with texture mapping
//        for (int x = round(left_x); x <= round(right_x); ++x) {
//            // Interpolate t based on the current x position
//            float t = (x - left_x) / (right_x - left_x);
//
//            // Calculate the interpolated texture coordinates
//            float texXf = texX1 + (texX2 - texX1) * t;
//            float texYf = texY1 + (texY2 - texY1) * t;
//
//            // Ensure that texXf and texYf are within the texture dimensions
//            if (texXf >= 0 && texXf < texture.width && texYf >= 0 && texYf < texture.height) {
//                // Convert texXf and texYf to integers for indexing
//                int texX = static_cast<int>(texXf);
//                int texY = static_cast<int>(texYf);
//
//                // Calculate the index to access the texture pixel
//                int index = texY * texture.width + texX;
//                window.setPixelColour(x, y, texture.pixels[index]);
//            }
//        }
//
//        left_x += slope1;
//        right_x += slope2;
//    }
//
//    // Initialize left and right x-coordinates for the bottom edge
//    left_x = middle.x;
//    float slope3 = (bottom.x - middle.x) / (bottom.y - middle.y);
//
//    // Second part of triangle (middle to bottom)
//    for (int y = middle.y + 1; y <= bottom.y; ++y) {
//        // Calculate the corresponding texture coordinates for Canvas Points
//        float t1 = (y - middle.y) / (bottom.y - middle.y);
//        float t2 = (y - top.y) / (bottom.y - top.y);
//        float texX1 = middle.texturePoint.x + (bottom.texturePoint.x - middle.texturePoint.x) * t1;
//        float texX2 = top.texturePoint.x + (bottom.texturePoint.x - top.texturePoint.x) * t2;
//        float texY1 = middle.texturePoint.y + (bottom.texturePoint.y - middle.texturePoint.y) * t1;
//        float texY2 = top.texturePoint.y + (bottom.texturePoint.y - top.texturePoint.y) * t2;
//
//        // Draw the horizontal line with texture mapping
//        for (int x = round(left_x); x <= round(right_x); ++x) {
//            // Interpolate t based on the current x position
//            float t = (x - left_x) / (right_x - left_x);
//
//            // Calculate the interpolated texture coordinates
//            float texXf = texX1 + (texX2 - texX1) * t;
//            float texYf = texY1 + (texY2 - texY1) * t;
//
//            // Ensure that texXf and texYf are within the texture dimensions
//            if (texXf >= 0 && texXf < texture.width && texYf >= 0 && texYf < texture.height) {
//                // Convert texXf and texYf to integers for indexing
//                int texX = static_cast<int>(texXf);
//                int texY = static_cast<int>(texYf);
//
//                // Calculate the index to access the texture pixel
//                int index = texY * texture.width + texX;
//                window.setPixelColour(x, y, texture.pixels[index]);
//            }
//        }
//        left_x += slope3;
//        right_x += slope2;
//    }
//    drawtriangle(triangle, Colour{255, 255, 255}, window);
//}



//--------task4---------
std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues) {
    float stepsSizeX = (to.x - from.x) / (numberOfValues - 1);
    float stepsSizeY = (to.y - from.y) / (numberOfValues - 1);
    float stepsSizeZ = (to.z - from.z) / (numberOfValues - 1);
    std::vector<glm::vec3> steps; // Create a 3D container of steps
    for (int i = 0; i < numberOfValues; i++) { // Traverse all steps
        glm::vec3 step(from.x + i * stepsSizeX, from.y + i * stepsSizeY, from.z + i * stepsSizeZ);
        steps.push_back(step);//The purpose of this line of code is to store each interpolation step in the steps vector for later use.
    }
    return steps;
}
//---------task4----------

//----------task2---------
std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
    std::vector<float> result;

    // Calculating step size
    float step = (to - from) / (numberOfValues - 1);

    // Fill the resulting vector with evenly spaced values
    for (int i = 0; i < numberOfValues; i++) {
        result.push_back(from + i * step);
    }

    return result;
}
//-----------task2-----------

void draw(DrawingWindow &window) {
    window.clearPixels();
    std::vector<float> gradientValues = interpolateSingleFloats(1.0f, 0.0f, window.width);
    for (size_t y = 0; y < window.height; y++) {
        for (size_t x = 0; x < window.width; x++) {
            float red = rand() % 256;
            float green = 0.0;
            float blue = 0.0;
//----------task3----------
            uint32_t grayValue = static_cast<uint8_t>(gradientValues[x] * 255);//灰色类型是uint8，这是类型转换
            uint32_t colour = (255 << 24) + (grayValue << 16) + (grayValue << 8) + grayValue;
            window.setPixelColour(x, y, Colour(grayValue, grayValue, grayValue));
        }
    }
}
//--------task3-------------

//void handleEvent(SDL_Event event, DrawingWindow &window) {
//    if (event.type == SDL_KEYDOWN) {
//        if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
//        else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
//        else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
//        else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
//    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
//        window.savePPM("output.ppm");
//        window.saveBMP("output.bmp");
//    }
//
//}

//-------task5---------
void drawRGBColour(DrawingWindow &window){
    window.clearPixels();
    glm::vec3 topLeft(255, 0, 0);        // red
    glm::vec3 topRight(0, 0, 255);       // blue
    glm::vec3 bottomRight(0, 255, 0);    // green
    glm::vec3 bottomLeft(255, 255, 0);   // yellow
    std::vector<glm::vec3> steps;
    std::vector<glm::vec3> Left = interpolateThreeElementValues(topLeft,bottomLeft,window.height);
    std::vector<glm::vec3> Right = interpolateThreeElementValues(topRight,bottomRight,window.height);
    for(size_t y=0; y<window.height; y++){
        for(size_t x=0; x<window.width; x++){
            glm::vec3 left = Left[y];
            glm::vec3 right = Right[y];
            std::vector<glm::vec3>row = interpolateThreeElementValues(left,right,window.width);
            glm::vec3 currentColour = row[x];
            uint32_t colour = (255<<24) +(int(currentColour.x)<<16) + (int(currentColour.y)<<8) +int(currentColour.z);
            window.setPixelColour(x,y,Colour(currentColour.x, currentColour.y, currentColour.z));


        }
    }

}
//void drawRGBColors(DrawingWindow& window, int width,int height){
//     std::vector<glm::vec3> result;
//     glm::vec3 topleft(255,0,0);//red
//     glm::vec3 topRight(0,0,255);//blue
//     glm::vec3 bottomRight(0,255,0);//green
//     glm::vec3 bottomLeft(255,255,0);//yellow
//
// for(size_t y=0; y< window.height; y++){//Iterate over each line of the window (vertical）
//     glm::vec3 from(255,y,0);//red
//     glm::vec3 to(0,y,255-y);//blue
//      result = interpolateThreeElementValues(from, to, width);//give the value of the result
//        for(size_t x=0; x < window.width; x++){//iterate vertical
//        float red = result[x].x;
//        float green = result[x].y;
//        float blue = result[x].z;
//        uint32_t colour = (255<<24) + (int(red) << 16) + (int(green) << 8) + int(blue);
//
//        window.setPixelColour(x, y, colour);
//
//     }
//
// }
//
//}
//-------task5------------

//int main(int argc, char *argv[]) {
//
//
////------task4--------
//    glm::vec3 from(1.0f, 4.0f, 9.2f);
//    glm::vec3 to(4.0f, 1.0f, 9.8f);
//    int numberOfValues = 4;
//
//    std::vector<glm::vec3> steps = interpolateThreeElementValues(from, to, numberOfValues);
//    std::cout << std::fixed << std::setprecision(1);
//    //We used std::fixed to indicate that std::setprecision retains zero digits after the decimal point. This will ensure that the output precision of floating-point numbers includes zero after the decimal point.
//    // Print interpolation result
//    for (const glm::vec3 &value: steps) {
//        std::cout << "(" << value.x << ", " << value.y << ", " << value.z << ")" << std::endl;
//    }
////-------task4--------
//
//
//
//
//
//
//
//
//
//
//
//
////--------task2--------
//    std::vector<float> result;
//    result = interpolateSingleFloats(2.2f, 8.5f, 7);
//
//    // 打印插值结果
//    for (size_t i = 0; i < result.size(); i++) {
//        std::cout << result[i] << " ";
//    }
//    std::cout << std::endl;
//    //-----task2---------
//
//    DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
//    SDL_Event event;
//    bool drawTriangle = false; // 新增一个变量，用于控制是否绘制三角形
//    while (true) {
//        // 首先轮询事件
//        if (SDL_PollEvent(&event)) {
//            if (event.type == SDL_QUIT) {
//                break; // 退出循环
//            } else if (event.type == SDL_KEYDOWN) {
//                if (event.key.keysym.sym == SDLK_q) {
//                    break; // 按下 'q' 键退出循环
//                } else if (event.key.keysym.sym == SDLK_u) {
//                    // 按下 'u' 键生成并绘制随机三角形
//                    drawTriangle = true; // 设置标志以绘制三角形
//                } else if (event.key.keysym.sym == SDLK_c) {
//                    // 按下 'c' 键清空屏幕
//                    window.clearPixels();
//                }
//            }
//            // 处理其他事件，例如鼠标事件
//        }
//
//        // 如果标志为 true，调用绘制三角形的函数
//        if (drawTriangle) {
//            CanvasTriangle triangle = generateRandomTriangle();
//            Colour color = trianglecoulour();
//            drawtriangle(triangle, color, window);
//            drawTriangle = false; // 绘制完成后将标志设置为 false
//        }
//
//        // 渲染图像和其他逻辑
//        // ...
//        bool drawRandomFillTriangle = false;
//        Colour randomOutlineColour = trianglecoulour();
//
//// 主循环中的事件处理
//        while (true) {
//            // 处理事件
//            SDL_Event event;
//            if (SDL_PollEvent(&event)) {
//                if (event.type == SDL_QUIT) {
//                    break; // 退出循环
//                } else if (event.type == SDL_KEYDOWN) {
//                    if (event.key.keysym.sym == SDLK_q) {
//                        break; // 按下 'q' 键退出循环
//                    } else if (event.key.keysym.sym == SDLK_f) {
//                        drawRandomFillTriangle = true; // 按下 'f' 键设置标志以绘制随机填充三角形
//                    }
//                    // 处理其他事件
//                }
//
//                // 其他事件处理代码
//            }
//
//            // 清空屏幕或执行其他渲染逻辑
//            // ...
//
//            // 如果标志为 true，绘制随机填充三角形
//            if (drawRandomFillTriangle) {
//                CanvasTriangle randomTriangle = generateRandomTriangle();
//                Colour randomColour = trianglecoulour();
//                fillTriangle(randomTriangle, randomColour, randomOutlineColour, window);
//                drawRandomFillTriangle = false; // 绘制完成后将标志设置为 false
//            }
//            TextureMap texture = TextureMap("/Users/lil/Desktop/CG2023-main/RedNoise/texture.ppm");
//
//            // Define the vertices of the triangle
//            CanvasPoint p1(160, 10);
//            CanvasPoint p2(300, 230);
//            CanvasPoint p3(10, 150);
//            p1.texturePoint = TexturePoint(195,5);
//            p2.texturePoint = TexturePoint(395,380);
//            p3.texturePoint = TexturePoint(65,330);
//
//
//
//
//            // Create a CanvasTriangle using the specified vertices
//            CanvasTriangle triangle(p1, p2, p3);
//
//            // Render the textured triangle
//            drawTexturedTriangle(triangle,texture,window);
//
//
//
//
//            // 渲染帧
//            window.renderFrame();
//        }
//        return 0;
//    }
//}


std::vector<ModelTriangle> loadObjFile(const std::string &filename, float scale, std::unordered_map<std::string, Colour> materials) {
    std::vector<glm::vec3> vertices;
    std::vector<ModelTriangle> faces;

    std::ifstream inputStr(filename, std::ifstream::in);
    std::string nextLine;
    Colour colour;
    while (std::getline(inputStr, nextLine)) { //extracts from inputStr and stores into nextLine
        std::vector<std::string> vector = split(nextLine, ' '); //split line by spaces
        if (vector[0] == "usemtl") {
            colour = materials[vector[1]];
        }else if (vector[0] == "v") {
            vertices.push_back(glm::vec3(
                    std::stof(vector[1]) * scale, //string to float
                    std::stof(vector[2]) * scale,
                    std::stof(vector[3]) * scale
            ));
        }
        else if (vector[0] == "f") { //indexed from 1
            faces.push_back(ModelTriangle(
                    vertices[std::stoi(split(vector[1], '/')[0]) - 1],
                    vertices[std::stoi(split(vector[2], '/')[0]) - 1],
                    vertices[std::stoi(split(vector[3], '/')[0]) - 1],
                    colour
            ));
        }
    }
    inputStr.close();
    return faces;
}
std::unordered_map<std::string, Colour> loadMtlFile(const std::string &filename) {
    std::unordered_map<std::string, Colour> colours;

    std::ifstream inputStream(filename, std::ifstream::in);
    std::string nextLine;
    std::string colour_name;

    while (std::getline(inputStream, nextLine)) {
        std::vector<std::string> line = split(nextLine, ' ');

        if (line[0] == "newmtl") {
            colour_name = line[1];
        } else if (line[0] == "Kd") {
            Colour colour(int(std::stof(line[1])*255),int(std::stof(line[2])*255),int(std::stof(line[3])*255));
            colours.insert({colour_name, colour});
        }
    }
    inputStream.close();
    return colours;
}
glm::mat3 cameraOrientation = glm::mat3(1.0f);//初始化单位矩阵
CanvasPoint getCanvasIntersection(const glm::vec3& cameraPosition, const glm::vec3& vertexPosition, const glm::mat3& cameraOrientation, float focalLength) {
    glm::vec3 adjustedPosition = cameraOrientation * (vertexPosition - cameraPosition); // 应用相机方向
    float scaleFactor = 240.0f;

    float x = -scaleFactor * adjustedPosition.x / adjustedPosition.z + WIDTH / 2;
    float y = scaleFactor * adjustedPosition.y / adjustedPosition.z + HEIGHT / 2;

    return CanvasPoint(x, y, adjustedPosition.z);
}


void drawPointCloud(DrawingWindow &window, glm::vec3 cameraPosition, std::vector<ModelTriangle> faces, float focalLength) {
    window.clearPixels();
    int scalingFactor = static_cast<int>(240);

    for (int i = 0; i < faces.size(); i++) {
        ModelTriangle face = faces[i];

        for (int j = 0; j < face.vertices.size(); j++) {
            glm::vec3 vertexPosition = face.vertices[j];

            // 计算在图像平面上的投影位置，应用缩放因子
            int x = static_cast<int>(round(focalLength * (vertexPosition.x - cameraPosition.x) /( vertexPosition.z - cameraPosition.z)*scalingFactor + (WIDTH / 2)));
            x = WIDTH - x;
            int y = static_cast<int>(round(focalLength * (vertexPosition.y - cameraPosition.y) / (vertexPosition.z - cameraPosition.z )*scalingFactor + (HEIGHT / 2)));

            // 绘制单个白色像素
            window.setPixelColour(x, y, Colour(255,255,255));
        }
    }
}
//void drawLine2(DrawingWindow &window, CanvasPoint start, CanvasPoint end, uint32_t color) {
//    float xDiff = end.x - start.x;
//    float yDiff = end.y - start.y;
//    float length = sqrt(xDiff*xDiff + yDiff*yDiff);
//
//    for (float t = 0; t <= 1; t += 1.0/length) {
//        float x = start.x + t * xDiff;
//        float y = start.y + t * yDiff;
//
//        int adjustedX = static_cast<int>(x);
//        int adjustedY = static_cast<int>(y);
//
//        if (adjustedX >= 0 && adjustedX < window.width && adjustedY >= 0 && adjustedY < window.height) {
//            window.setPixelColour(adjustedX, adjustedY, color);
//        }
//    }
//}
//void renderWireframe(DrawingWindow &window, const std::vector<std::vector<float>>& vertices, const std::vector<std::vector<int>>& faces, const glm::vec3& cameraPosition, float focalLength) {
//    uint32_t whiteColour = (255 << 16) + (255 << 8) + 255;  // RGB for white
//
//    for (const auto& face : faces) {
//        std::vector<CanvasPoint> projectedVertices;
//        for (size_t i = 0; i < face.size(); ++i) {
//            const auto& vertex3D = vertices[face[i] - 1];  // OBJ indices start from 1
//            projectedVertices.push_back(getCanvasIntersection(cameraPosition, glm::vec3(vertex3D[0], vertex3D[1], vertex3D[2]), focalLength));
//        }
//
//        drawLine(window, projectedVertices[0], projectedVertices[1], whiteColour);
//        drawLine(window, projectedVertices[1], projectedVertices[2], whiteColour);
//        drawLine(window, projectedVertices[2], projectedVertices[0], whiteColour);
//    }
//}



void renderFilledTriangles(DrawingWindow &window, glm::vec3 cameraPosition, std::vector<ModelTriangle> faces, float focalLength, const glm::mat3& cameraOrientation) {
    window.clearPixels();
    int scalingFactor = static_cast<int>(240);
    std::vector<std::vector<float>> depthBuffer(window.height, std::vector<float>(window.width, std::numeric_limits<float>::infinity()));

    for (const auto& face : faces) {
        std::vector<CanvasPoint> projectedPoints;

        // 投影三角形的每个顶点
        for (const auto& vertex : face.vertices) {
            // 将顶点转换到相机坐标系
            glm::vec3 cameraVertex = cameraOrientation * (vertex - cameraPosition);

            // 应用透视投影
            float x = focalLength * (cameraVertex.x) / (cameraVertex.z) * scalingFactor + (window.width / 2);
            float y = focalLength * (cameraVertex.y) / (cameraVertex.z) * scalingFactor + (window.height / 2);
            float depth = cameraVertex.z; // 用于深度缓冲

            CanvasPoint projectedVertex(window.width - x, y, depth); // 注意这里的x坐标翻转以匹配drawWireframe函数
            projectedPoints.push_back(projectedVertex);
        }

        // 检查是否有三个投影的顶点
        if (projectedPoints.size() == 3) {
            CanvasTriangle canvasTriangle(projectedPoints[0], projectedPoints[1], projectedPoints[2]);
            Colour fillColour = face.colour;  // 使用ModelTriangle中的颜色属性
            Colour outlineColour = fillColour;  // 轮廓颜色与填充颜色相同

            // 填充三角形
            fillTriangle(canvasTriangle, fillColour, outlineColour, window, depthBuffer);
        }
    }
}

//void handleEvent(SDL_Event event, DrawingWindow &window) {
//    if (event.type == SDL_KEYDOWN) {
//        if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
//        else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
//        else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
//        else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
//    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
//        window.savePPM("output.ppm");
//        window.saveBMP("output.bmp");
//    }
//
//}


// processInput函数
void handlevent(SDL_Event &event, DrawingWindow &window, glm::vec3 &cameraPosition,glm::mat3 &cameraOrientation, float delta,bool &autoRotate,bool &raytracingEnabled) {
    // 根据event处理输入
    switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {

                case SDLK_w:
                    cameraPosition.z -= delta;
                    break;
                case SDLK_s:
                    cameraPosition.z += delta;
                    break;
                case SDLK_d:
                    cameraPosition.x -= delta;
                    break;  // 向右移动
                case SDLK_a:
                    cameraPosition.x += delta;
                    break;  // 向左移动
                case SDLK_q: { // 绕Y轴旋转 - 左转
                    float angle = glm::radians(-1.0f); // 逆时针为负角度
                    glm::mat3 rotateY = glm::mat3(
                            glm::cos(angle), 0.0f, glm::sin(angle),
                            0.0f, 1.0f, 0.0f,
                            -glm::sin(angle), 0.0f, glm::cos(angle)
                    );
                    cameraOrientation = rotateY * cameraOrientation;
                    break;
                }
                case SDLK_e: { // 绕Y轴旋转 - 右转
                    float angle = glm::radians(1.0f); // 顺时针为正角度
                    glm::mat3 rotateY = glm::mat3(
                            glm::cos(angle), 0.0f, glm::sin(angle),
                            0.0f, 1.0f, 0.0f,
                            -glm::sin(angle), 0.0f, glm::cos(angle)
                    );
                    cameraOrientation = rotateY * cameraOrientation;
                    break;
                }
                case SDLK_r: { // 绕X轴旋转 - 向上倾斜
                    float angle = glm::radians(-1.0f); // 向上为负角度
                    glm::mat3 rotateX = glm::mat3(
                            1.0f, 0.0f, 0.0f,
                            0.0f, glm::cos(angle), -glm::sin(angle),
                            0.0f, glm::sin(angle), glm::cos(angle)
                    );
                    cameraOrientation = rotateX * cameraOrientation;
                    break;
                }
                case SDLK_f: { // 绕X轴旋转 - 向下倾斜
                    float angle = glm::radians(1.0f); // 向下为正角度
                    glm::mat3 rotateX = glm::mat3(
                            1.0f, 0.0f, 0.0f,
                            0.0f, glm::cos(angle), -glm::sin(angle),
                            0.0f, glm::sin(angle), glm::cos(angle)
                    );
                    cameraOrientation = rotateX * cameraOrientation;
                    break;
                }
                case SDLK_SPACE: {
                    autoRotate = !autoRotate;
                    break;
                }
                case SDLK_t: {
                    raytracingEnabled = !raytracingEnabled;
                    break;
                }

            }


            cameraPosition.x = std::min(cameraPosition.x, static_cast<float>(window.width));
            cameraPosition.y = std::min(cameraPosition.y, static_cast<float>(window.height));

            break;

    }
}


RayTriangleIntersection getClosestValidIntersection(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const std::vector<ModelTriangle> &triangles, bool forShadow = false) {
    RayTriangleIntersection r;
    r.distanceFromCamera = std::numeric_limits<float>::infinity();

    for (size_t i = 0; i < triangles.size(); ++i) {
        const ModelTriangle &triangle = triangles[i];
        glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
        glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
        glm::vec3 SPVector = rayOrigin - triangle.vertices[0];
        glm::mat3 DEMatrix(-rayDirection, e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;

        float t = possibleSolution.x, u = possibleSolution.y, v = possibleSolution.z;

        // 当我们为阴影计算时，忽略距离起点非常近的交点
        if (forShadow && t < 0.001f) continue;

        // 检查 u, v 是否在三角形边界内，t 必须为正
        if (t > 0 && u >= 0 && u <= 1.0 && v >= 0.0 && v <= 1.0 && (u + v) <= 1.0) {
            if (t < r.distanceFromCamera) {
                glm::vec3 intersectionPoint = cameraPosition + possibleSolution.x * rayDirection;
                r = RayTriangleIntersection(intersectionPoint, t, triangle, i);
            }
        }
    }

    return r;
}




uint32_t darkenColor(uint32_t originalColor) {
    // 分离颜色通道
    int alpha = (originalColor >> 24) & 0xFF;
    int red = (originalColor >> 16) & 0xFF;
    int green = (originalColor >> 8) & 0xFF;
    int blue = originalColor & 0xFF;

    // 暗化（例如，降低到原来的50%）
    red *= 0.5;
    green *= 0.5;
    blue *= 0.5;

    // 重新组合颜色并返回
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

void drawRaytracedScene(DrawingWindow &window, glm::vec3 cameraPosition, const std::vector<ModelTriangle> &triangles, float focalLength, const glm::mat3 &cameraOrientation, const glm::vec3 &lightPosition) {
    int imageWidth = window.width;
    int imageHeight = window.height;
    float aspectRatio = float(imageWidth) / float(imageHeight);

    for (int y = 0; y < imageHeight; y++) {
        for (int x = 0; x < imageWidth; x++) {
            // 将3D坐标转换为2D屏幕上的像素坐标
            float viewportX = (2.0f * (float(x) / imageWidth) - 1.0f) * aspectRatio;
            float viewportY = 1.0f - 2.0f * (float(y) / imageHeight);

            // 射线方向是视口位置到相机位置的向量
            glm::vec3 rayDirection = cameraOrientation * glm::normalize(glm::vec3(viewportX, viewportY, -focalLength));

            // 获取与射线相交的最近有效三角形
            RayTriangleIntersection intersection = getClosestValidIntersection(cameraPosition, rayDirection, triangles,false);

            // 如果找到有效交点，根据交点信息给像素上色
            if (intersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                ModelTriangle triangle = intersection.intersectedTriangle;
                uint32_t color = (255 << 24) + (triangle.colour.red << 16) + (triangle.colour.green << 8) + triangle.colour.blue;

                // 从交点向光源位置发送射线
                glm::vec3 toLight = glm::normalize(lightPosition - intersection.intersectionPoint);
                float lightDistance = glm::length(lightPosition - intersection.intersectionPoint);
                RayTriangleIntersection shadowIntersection = getClosestValidIntersection(intersection.intersectionPoint + toLight * 0.001f, toLight, triangles, true); // 避免自交

                if (shadowIntersection.distanceFromCamera != std::numeric_limits<float>::infinity()) {
                    if (shadowIntersection.distanceFromCamera < lightDistance) {
                        // 交点在阴影中
                        color = darkenColor(color);
                    }
                }

                // 边界检查，确保不会试图设置窗口外的像素
                if (x >= 0 && x < imageWidth && y >= 0 && y < imageHeight) {
                    window.setPixelColour(x, y, shadowIntersection.intersectedTriangle.colour);
                }
            } else {
                // 如果没有交点，您可以选择设置一个背景颜色
                Colour backgroundColor = Colour(255,255,255) ; // 黑色背景
                window.setPixelColour(x, y, backgroundColor);
            }
        }
    }
}




int main() {

    std::unordered_map<std::string, Colour> materials = loadMtlFile("cornell-box.mtl");
    std::vector<ModelTriangle> faces = loadObjFile("cornell-box.obj", 0.35, materials);

    DrawingWindow window(WIDTH, HEIGHT, false);
    std::vector<std::vector<float>> depthBuffer(window.height, std::vector<float>(window.width,
                                                                                  std::numeric_limits<float>::infinity()));
    bool shouldRedraw = true;
    bool autoRotate = false;
    bool raytracingEnabled = false; // 默认不启用光线追踪渲染
    glm::vec3 lightPosition(0.0f, 0.4f, 0.0f);
    Uint32 lastTime = SDL_GetTicks();


    // 主循环，处理事件和更新渲染
    while (true) {
        // 处理所有事件
        SDL_Event event;
        while (window.pollForInputEvents(event)) {
            handlevent(event, window, cameraPosition, cameraOrientation, delta,autoRotate, raytracingEnabled);
            shouldRedraw = true; // 事件处理可能更改了相机的位置或其他因素，因此我们设置重绘标志
        }

        // 自动更新旋转逻辑
        if (autoRotate) {
            Uint32 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f; // 转换为秒
            lastTime = currentTime;

            orbitAngle += deltaTime * 0.5f; // 每秒旋转0.5弧度
            cameraPosition.x = orbitCenter.x + orbitRadius * cos(orbitAngle);
            cameraPosition.z = orbitCenter.z + orbitRadius * sin(orbitAngle);

            // 基于新的cameraPosition重新计算cameraOrientation
            cameraOrientation = glm::mat3(glm::lookAt(cameraPosition, orbitCenter, glm::vec3(0, 1, 0)));

            // 设置重绘标志
            shouldRedraw = true;
        }


        // 如果发生了任何更改，需要重绘
        if (shouldRedraw) {
            window.clearPixels();
            depthBuffer = std::vector<std::vector<float>>(window.height, std::vector<float>(window.width,
                                                                                            std::numeric_limits<float>::infinity()));
            if (raytracingEnabled) {
                drawRaytracedScene(window, cameraPosition, faces, focalLength, cameraOrientation, lightPosition);
            } else {
                // 调用原来的渲染函数
                renderFilledTriangles(window, cameraPosition, faces, focalLength, cameraOrientation);
            }            }
        window.renderFrame();
        shouldRedraw = false; // 重置重绘标志
    }


}
