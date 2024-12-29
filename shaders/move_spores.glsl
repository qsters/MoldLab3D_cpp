#version 430

#define SPORE_STRUCT

// Simulation Settings
#define SIMULATION_SETTINGS

layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

// Buffers
layout(std430, binding = 0) buffer SporesBuffer {
    Spore spores[];
};

layout(std430, binding = 1) buffer SettingsBuffer {
    SimulationData settings;
};

layout(binding = 0, r32f) uniform image3D voxelData;

float sense(vec3 position, vec3 direction, int gridSize, float sensorDistance) {
    // Calculate the sampling position
    vec3 samplePosition = position + normalize(direction) * sensorDistance;

    // Clamp the sampling position to the grid boundaries
    ivec3 clampedPosition = ivec3(clamp(samplePosition, vec3(0.0), vec3(gridSize - 1)));

    // TODO: Remove this test line
//    imageStore(voxelData, clampedPosition, vec4(0.5));

    // Return the voxel data at the sampled position
    return imageLoad(voxelData, clampedPosition).x;
}

void main() {
    uint sporeID = gl_GlobalInvocationID.x;

    // Check bounds
    if (sporeID >= settings.spore_count) {
        return;
    }

    Spore spore = spores[sporeID];
    vec3 sporePosition = spore.position.xyz;

    vec3 forwardDirection = spore.orientation[2];

    // Calculate the new position by moving forward in the direction of the spore's direction vector
    vec3 newPosition = sporePosition + forwardDirection * settings.spore_speed * settings.delta_time;

    // Store the current position before clamping
    vec3 storePosition = newPosition;

    // Clamp the position within the grid bounds
    newPosition = clamp(newPosition, 0.0f, float(settings.grid_size));

    // Determine if the spore hit any bounds
    bvec3 hitMask = notEqual(newPosition, storePosition);

    // Reflect the forward direction if a boundary was hit
    forwardDirection *= mix(vec3(1.0), vec3(-1.0), vec3(hitMask));

    // Update the orientation matrix to reflect the new forward direction
    spore.orientation[2] = normalize(forwardDirection);

    // Recompute the right and up vectors to ensure the orientation matrix stays orthogonal
    vec3 right = normalize(cross(spore.orientation[1], spore.orientation[2])); // Recalculate right
    spore.orientation[0] = right; // Update right vector
    spore.orientation[1] = normalize(cross(spore.orientation[2], spore.orientation[0])); // Recalculate up vector

    // Update the spore's position
    spore.position = vec4(newPosition, 0.0);

    // Write the updated spore back to the buffer
    spores[sporeID] = spore;
}
