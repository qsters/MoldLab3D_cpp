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

uniform float deltaTime;

void main() {
    uint sporeID = gl_GlobalInvocationID.x;

    // Check bounds
    if (sporeID >= settings.spore_count) {
        return;
    }

    Spore spore = spores[sporeID];

    // Calculate the new position by moving forward in the direction of the spore's direction vector
    vec3 newPosition = spore.position + spore.direction * settings.spore_speed * deltaTime;
    vec3 newDirection = spore.direction;

    vec3 storePosition = newPosition;

    newPosition = clamp(newPosition, 0.0f, float(settings.grid_size));

    bvec3 hitMask = notEqual(newPosition, storePosition);

    newDirection *= mix(vec3(1), vec3(-1), vec3(hitMask));

    // Update the spore's position
    spore.position = newPosition;
    spore.direction = newDirection;

    // Write the updated spore back to the buffer
    spores[sporeID] = spore;
}
