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

    // Calculate the new position by moving forward in the direction of the spore's direction vector
    vec3 newPosition = spore.position + spore.direction * settings.spore_speed * settings.delta_time;
    vec3 newDirection = spore.direction;

    vec3 forward = spore.direction;
    vec3 right = -normalize(cross(forward, vec3(0.0, 1.0, 0.0))); // Generate right vector
    if (length(right) == 0.0) right = vec3(1.0, 0.0, 0.0);        // Handle parallel case
    vec3 up = normalize(cross(right, forward));                   // Generate up vector

    // Compute mix factors using trigonometric functions
    float normalFactor = sin(settings.sensor_angle); // [0 at 0, 1 at PI/2, 0 at PI]
    float reverseFactor = cos(settings.sensor_angle); // [1 at 0, 0 at PI/2, -1 at PI]

    // Interpolate sensor positions based on sensor_angle
    vec3 sensor_right = normalize(mix(forward, right, normalFactor) * (1.0 - abs(reverseFactor)) + forward * reverseFactor);
    vec3 sensor_left = normalize(mix(forward, -right, normalFactor) * (1.0 - abs(reverseFactor)) + forward * reverseFactor);
    vec3 sensor_up = normalize(mix(forward, up, normalFactor) * (1.0 - abs(reverseFactor)) + forward * reverseFactor);
    vec3 sensor_down = normalize(mix(forward, -up, normalFactor) * (1.0 - abs(reverseFactor)) + forward * reverseFactor);

    // Sense weights at the interpolated sensor positions
    float forwardWeight = sense(spore.position, forward, settings.grid_size, settings.sensor_distance);
    float rightWeight = sense(spore.position, sensor_right, settings.grid_size, settings.sensor_distance);
    float leftWeight = sense(spore.position, sensor_left, settings.grid_size, settings.sensor_distance);
    float upWeight = sense(spore.position, sensor_up, settings.grid_size, settings.sensor_distance);
    float downWeight = sense(spore.position, sensor_down, settings.grid_size, settings.sensor_distance);

    // Adjust direction based on sensed weights
    vec3 directionChange = vec3(0.0);
    if (forwardWeight < rightWeight || forwardWeight < leftWeight) {
        if (rightWeight > leftWeight) {
            directionChange += right;
        } else if (leftWeight > rightWeight) {
            directionChange -= right;
        }
    }
    if (forwardWeight < upWeight || forwardWeight < downWeight) {
        if (upWeight > downWeight) {
            directionChange += up;
        } else if (downWeight > upWeight) {
            directionChange -= up;
        }
    }

    newDirection = normalize(forward + directionChange * (settings.turn_speed * float(3.1415)) * settings.delta_time * 3);

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
