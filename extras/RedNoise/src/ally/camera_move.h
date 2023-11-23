#pragma once

#include "Utils.h"
#include "glm/glm.hpp"

void rotate(glm::vec3* c, char t);
void orientRotate(glm::mat3* o, char t);
void orbit(glm::vec3* c);
void lookAt(glm::vec3* c, glm::mat3* o);