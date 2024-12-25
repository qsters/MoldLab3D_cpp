#ifndef SPORE_H
#define SPORE_H

struct Spore {
    alignas(16) vec3 position;   // Position vector: x, y, z
    alignas(16) vec3 direction;  // Direction vector: dir_x, dir_y, dir_z
};

#endif //SPORE_H
