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
    int sdf_reduction;
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


float sense(vec3 position, vec3 direction, int gridSize, float sensorDistance) {
    // Calculate the sampling position
    vec3 samplePosition = position + normalize(direction) * sensorDistance;

    // Clamp the sampling position to the grid boundaries
    ivec3 clampedPosition = ivec3(clamp(samplePosition, vec3(0.0), vec3(gridSize - 1)));

    // Compute the linear index
    int idx = clampedPosition.z * gridSize * gridSize + clampedPosition.y * gridSize + clampedPosition.x;

    // Test line for debugging sensor position
//    voxelData[idx] = 0.5;

    // Return the voxel data at the sampled position
    return voxelData[idx];
}

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

    vec3 forward = spore.direction;
    vec3 right = normalize(cross(forward, vec3(0.0, 1.0, 0.0))); // Generate right vector
    if (length(right) == 0.0) right = vec3(1.0, 0.0, 0.0);       // Handle parallel case
    vec3 up = normalize(cross(right, forward));                  // Generate up vector

    float forwardWeight = sense(spore.position, forward, settings.grid_size, settings.sensor_distance);
    float rightWeight = sense(spore.position, right, settings.grid_size, settings.sensor_distance);
    float leftWeight = sense(spore.position, -right, settings.grid_size, settings.sensor_distance);
    float upWeight = sense(spore.position, up, settings.grid_size, settings.sensor_distance);
    float downWeight = sense(spore.position, -up, settings.grid_size, settings.sensor_distance);

    // Adjust direction based on sensed weights
    vec3 directionChange = vec3(0.0);
    // If one of the
    if (forwardWeight < rightWeight || forwardWeight < leftWeight) {
        // Blend between -right and right based on the normalized weights
        float weightSum = rightWeight + leftWeight;
        vec3 horizontalDirection = mix(-right, right, rightWeight / weightSum);
        directionChange += horizontalDirection;
    }

    if (forwardWeight < upWeight || forwardWeight < downWeight) {
        // Blend between -up and up based on the normalized weights
        float weightSum = upWeight + downWeight;
        vec3 verticalDirection = mix(-up, up, upWeight / weightSum);
        directionChange += verticalDirection;
    }

    newDirection = normalize(forward + directionChange * settings.turn_speed * deltaTime);

    // Boundary Hit Handler
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
