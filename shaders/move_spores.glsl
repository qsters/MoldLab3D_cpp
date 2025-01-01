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

    // Return the voxel data at the sampled position
    return imageLoad(voxelData, clampedPosition).x;
}

// Creating overload so that when it isn't used, it will be removed by compiler and there won't be if checks normally
float sense(vec3 position, vec3 direction, int gridSize, float sensorDistance, bool debug) {
    // Calculate the sampling position
    vec3 samplePosition = position + normalize(direction) * sensorDistance;

    // Clamp the sampling position to the grid boundaries
    ivec3 clampedPosition = ivec3(clamp(samplePosition, vec3(0.0), vec3(gridSize - 1)));

    if (debug){
        imageStore(voxelData, clampedPosition, vec4(0.5));
    }
    // Return the voxel data at the sampled position
    return imageLoad(voxelData, clampedPosition).x;
}

// Function to keep orientation matrix orthogonal
mat3 normalizeMatrix(mat3 m) {
    m[2] = normalize(m[2]); // Forward
    m[0] = normalize(cross(m[1], m[2])); // Right
    m[1] = normalize(cross(m[2], m[0])); // Up
    return m;
}

// Function to rotate orientation matrix around a given axis
mat3 rotateOrientation(mat3 orientation, vec3 axis, float angle) {
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
    mat3 rotationMatrix = mat3(
    vec3(cosAngle + axis.x * axis.x * (1.0 - cosAngle), axis.x * axis.y * (1.0 - cosAngle) - axis.z * sinAngle, axis.x * axis.z * (1.0 - cosAngle) + axis.y * sinAngle),
    vec3(axis.y * axis.x * (1.0 - cosAngle) + axis.z * sinAngle, cosAngle + axis.y * axis.y * (1.0 - cosAngle), axis.y * axis.z * (1.0 - cosAngle) - axis.x * sinAngle),
    vec3(axis.z * axis.x * (1.0 - cosAngle) - axis.y * sinAngle, axis.z * axis.y * (1.0 - cosAngle) + axis.x * sinAngle, cosAngle + axis.z * axis.z * (1.0 - cosAngle))
    );
    return rotationMatrix * orientation;
}

void main() {
    uint sporeID = gl_GlobalInvocationID.x;

    // Check bounds
    if (sporeID >= settings.spore_count) {
        return;
    }

    Spore spore = spores[sporeID];
    vec3 sporePosition = spore.position.xyz;

    vec3 forward = spore.orientation[2];
    vec3 up = spore.orientation[1];
    vec3 right = spore.orientation[0];


    // Sense voxel data in all relevant directions
    float forwardWeight = sense(sporePosition, forward, settings.grid_size, settings.sensor_distance);
    float upWeight = sense(sporePosition, up, settings.grid_size, settings.sensor_distance);
    float downWeight = sense(sporePosition, -up, settings.grid_size, settings.sensor_distance);
    float rightWeight = sense(sporePosition, right, settings.grid_size, settings.sensor_distance);
    float leftWeight = sense(sporePosition, -right, settings.grid_size, settings.sensor_distance);

    // Determine the maximum weight and associated rotation axis
    float maxWeight = forwardWeight;
    vec3 rotationAxis = vec3(0.0); // No rotation by default
    float rotationAngle = 0.0;

    if (upWeight > maxWeight) {
        maxWeight = upWeight;
        rotationAxis = right; // Rotate around right axis for up
    }
    if (downWeight > maxWeight) {
        maxWeight = downWeight;
        rotationAxis = -right; // Rotate around negative right axis for down
    }
    if (rightWeight > maxWeight) {
        maxWeight = rightWeight;
        rotationAxis = -up; // Rotate around negative up axis for right
    }
    if (leftWeight > maxWeight) {
        maxWeight = leftWeight;
        rotationAxis = up; // Rotate around up axis for left
    }

    // Compute rotation angle if a direction other than forward has the maximum weight
    if (rotationAxis != vec3(0.0)) {
        rotationAngle = settings.turn_speed * settings.delta_time * 3.1415; // Turn speed in radians/sec
    }

    // Apply rotation to the orientation matrix
    if (rotationAngle > 0.0) {
        spore.orientation = rotateOrientation(spore.orientation, rotationAxis, rotationAngle);
        forward = spore.orientation[2]; // Update forward vector after rotation
    }

    // Calculate the new position by moving forward in the direction of the spore's direction vector
    vec3 newPosition = sporePosition + forward * settings.spore_speed * settings.delta_time;

    // Store the current position before clamping
    vec3 storePosition = newPosition;

    // Clamp the position within the grid bounds
    newPosition = clamp(newPosition, 0.0f, float(settings.grid_size));

    // Determine if the spore hit any bounds
    bvec3 hitMask = notEqual(newPosition, storePosition);

    // Update forward
    spore.orientation[2] = forward * mix(vec3(1.0), vec3(-1.0), vec3(hitMask));

    // Normalize the matrix:
    spore.orientation = normalizeMatrix(spore.orientation);

    // Update the spore's position
    spore.position = vec4(newPosition, 0.0);

    // Write the updated spore back to the buffer
    spores[sporeID] = spore;
}
