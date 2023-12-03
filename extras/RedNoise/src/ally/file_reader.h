#pragma once
#include "file_reader.h"

#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <Colour.h>
#include <TexturePoint.h>
#include <ModelTriangle.h>
#include <fstream>

// TODO : check if I can include this library
#include <sstream>


// TODO : check if it's allowed to use this library
#include <unordered_map>
#include <thread>

#define WIDTH 320
#define HEIGHT 240


std::unordered_map<std::string, Colour> readMTL (const std::string &filename);
std::vector<ModelTriangle> readOBJ(const std::string &filename, std::unordered_map<std::string, Colour> colourMap, float s);

