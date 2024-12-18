#ifndef SPORESDATA_H
#define SPORESDATA_H
#include <cstdint>
#include <linmath.h>

struct SimulationSettings {
    int spore_count;       // Number of spores
    int grid_size;   // Size of the simulation grid
    float spore_speed;          // Speed of spores
    float decay_speed;          // Decay speed
    float turn_speed;           // Turning speed for spores
    float sensor_distance;      // Distance spores "sense"
};

struct Spore {
    alignas(16) vec3 position;   // Position vector: x, y, z
    alignas(16) vec3 direction;  // Direction vector: dir_x, dir_y, dir_z
};


#endif //SPORESDATA_H
