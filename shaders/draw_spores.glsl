#version 430

// Spores Data Structure
struct Spore {
    vec3 position;
    vec3 direction;
};

// Simulation Settings
struct SimulationSettings {
    int spore_count;
    int grid_size;
    float spore_speed;
    float decay_speed;
    float turn_speed;
    float sensor_distance;
};

layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[]; // 1D array representing the voxel grid
};

// Buffers
layout(std430, binding = 1) buffer SporesBuffer {
    Spore spores[];
};

layout(std430, binding = 2) buffer SettingsBuffer {
    SimulationSettings settings;
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

    // Convert the voxel coordinates to a 1D index
    int voxelIndex = voxelCoord.x + gridSize * (voxelCoord.y + gridSize * voxelCoord.z);

    // Ensure the index is valid and update the voxel grid
    if (voxelIndex >= 0 && voxelIndex < gridSize * gridSize * gridSize) {
        voxelData[voxelIndex] = 1.0; // Mark the voxel as occupied by the spore
    }
}
