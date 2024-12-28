#version 430

// Spores Data Structure
struct Spore {
    vec3 position;
    vec3 direction;
};

// Simulation Settings
#define SIMULATION_SETTINGS

layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f) uniform image3D voxelData;

// Buffers
layout(std430, binding = 0) buffer SporesBuffer {
    Spore spores[];
};

layout(std430, binding = 1) buffer SettingsBuffer {
    SimulationData settings;
};


void main() {
    uint sporeID = gl_GlobalInvocationID.x;

    // Check bounds
    if (sporeID >= settings.spore_count) {
        return;
    }

    int gridSize = settings.grid_size;

    // Get the spore position
    vec3 sporePosition = spores[sporeID].position;

    // Determine the voxel grid coordinates closest to the spore position
    ivec3 voxelCoord = ivec3(
    clamp(int(floor(sporePosition.x)), 0, gridSize - 1),
    clamp(int(floor(sporePosition.y)), 0, gridSize - 1),
    clamp(int(floor(sporePosition.z)), 0, gridSize - 1)
    );


    imageStore(voxelData, voxelCoord, vec4(1.0)); // Mark the voxel as occupied by the spore
}
