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

float sense(vec3 position, vec3 direction, int gridSize, float sensorDistance, bool debug) {
    vec3 samplePosition = position + normalize(direction) * sensorDistance;

    // Clamp the sampling position to the grid boundaries
    ivec3 clampedPosition = ivec3(clamp(samplePosition, vec3(0.0), vec3(gridSize - 1)));

    // TODO: Remove this test line when done testing
    if(debug) {
        imageStore(voxelData, clampedPosition, vec4(0.5));
    }


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

    vec3 forward = spore.orientation[2];
    vec3 right = spore.orientation[0];
    vec3 up = spore.orientation[1];

    // Sense the voxel grid in the forward direction and the four cardinal directions
    float forwardWeight = sense(sporePosition, forward, settings.grid_size, settings.sensor_distance, true);
    float rightWeight = sense(sporePosition, right, settings.grid_size, settings.sensor_distance, true);
    float leftWeight = sense(sporePosition, -right, settings.grid_size, settings.sensor_distance);
    float upWeight = sense(sporePosition, up, settings.grid_size, settings.sensor_distance, true) + 1.;
    float downWeight = sense(sporePosition, -up, settings.grid_size, settings.sensor_distance);

    // Initialize rotation matrices to identity
    mat3 horizontalRotationMatrix = mat3(1.0); // Identity matrix for horizontal tilt
    mat3 verticalRotationMatrix = mat3(1.0);   // Identity matrix for vertical tilt

    float rotationAngle = 6.2838 * settings.turn_speed * settings.delta_time;

    // Horizontal rotation (around up vector)
    if (forwardWeight < rightWeight || forwardWeight < leftWeight) {
        float angle = 0.0;
        if (rightWeight > leftWeight) {
            angle = rotationAngle;
        } else if (leftWeight > rightWeight) {
            angle = -rotationAngle;
        }

        if (angle != 0.0) {
            float s = sin(angle);
            float c = cos(angle);
            mat3 rotation = mat3(
            vec3(c, 0.0, -s),
            vec3(0.0, 1.0, 0.0),
            vec3(s, 0.0, c)
            );
            forward = rotation * forward;
            right = rotation * right;
        }
    }

    // Vertical rotation (around right vector)
    if (forwardWeight < upWeight || forwardWeight < downWeight) {
        float angle = 0.0;
        if (upWeight > downWeight) {
            angle = rotationAngle;
        } else if (downWeight > upWeight) {
            angle = -rotationAngle;
        }

        if (angle != 0.0) {
            float s = sin(angle);
            float c = cos(angle);
            mat3 rotation = mat3(
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, c, -s),
            vec3(0.0, s, c)
            );
            forward = rotation * forward;
            up = rotation * up;
        }
    }

    // Reorthogonalize the basis after rotation
    forward = normalize(forward);
    right = normalize(cross(up, forward));
    up = normalize(cross(forward, right));

    // Update the orientation matrix
    spore.orientation[0] = right;
    spore.orientation[1] = up;
    spore.orientation[2] = forward;

    // Move forward in the direction of the spore's forward vector
    vec3 newPosition = sporePosition + forward * settings.spore_speed * settings.delta_time;

    // Reflect the forward direction if any boundary is hit
    if (newPosition.x < 0.0 || newPosition.x >= settings.grid_size) {
        forward.x = -forward.x; // Reverse direction along the X-axis
        newPosition.x = clamp(newPosition.x, 0.0, settings.grid_size - 1.0); // Clamp position to bounds
    }
    if (newPosition.y < 0.0 || newPosition.y >= settings.grid_size) {
        forward.y = -forward.y; // Reverse direction along the Y-axis
        newPosition.y = clamp(newPosition.y, 0.0, settings.grid_size - 1.0); // Clamp position to bounds
    }
    if (newPosition.z < 0.0 || newPosition.z >= settings.grid_size) {
        forward.z = -forward.z; // Reverse direction along the Z-axis
        newPosition.z = clamp(newPosition.z, 0.0, settings.grid_size - 1.0); // Clamp position to bounds
    }

    // Normalize the forward vector after reflection
    forward = normalize(forward);

    // Recompute the orientation matrix
    spore.orientation[2] = forward; // Update forward vector
    spore.orientation[0] = normalize(cross(up, forward)); // Recompute right vector
    spore.orientation[1] = normalize(cross(forward, spore.orientation[0])); // Recompute up vector

    // Update the spore's position
    spore.position = vec4(newPosition, 0.0);

    // Write the updated spore back to the buffer
    spores[sporeID] = spore;

}
