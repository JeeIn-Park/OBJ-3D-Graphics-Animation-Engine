#include <Utils.h>
#include <glm/glm.hpp>

#include "camera_move.h"

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
