#ifndef SPORE_H
#define SPORE_H

struct Spore {
    vec3 position;   // Position vector: x, y, z
    float padding;   // #DEFINE_REMOVE_FROM_SHADER
    vec3 direction;  // Direction vector: dir_x, dir_y, dir_z
    float padding2;  // #DEFINE_REMOVE_FROM_SHADER
};

#endif //SPORE_H
