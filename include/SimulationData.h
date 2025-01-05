#ifndef SIMULATIONDATA_H
#define SIMULATIONDATA_H

struct SimulationData {
    int spore_count;       // Number of spores
    int grid_size;   // Size of the simulation grid
    int sdf_reduction;
    float spore_speed;          // Speed of spores
    float decay_speed;          // Decay speed
    float turn_speed;           // Turning speed for spores
    float sensor_distance;      // Distance spores "sense"
    float sensor_angle;
    vec4 camera_position; // Must be aligned on 16 bytes!!
    vec4 camera_focus; // Must be aligned on 16 bytes!!
    float delta_time;
};

#endif //SIMULATIONDATA_H