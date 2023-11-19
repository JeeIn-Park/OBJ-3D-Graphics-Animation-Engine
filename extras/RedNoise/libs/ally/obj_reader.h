#pragma once

#include "Utils.h"
#include <fstream>
#include <vector>
#include "glm/glm.hpp"
#include "Colour.h"
#include "ModelTriangle.h"
#include <unordered_map>

// TODO : check if I can include this library
#include <sstream>

std::unordered_map<std::string, Colour> readMTL(const std::string &filename);
std::vector<ModelTriangle> readOBJ(const std::string &filename, std::unordered_map<std::string, Colour> colourMap, float s);