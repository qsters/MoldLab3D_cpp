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

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main() {
    uint sporeID = gl_GlobalInvocationID.x;

    // Check bounds
    if (sporeID >= settings.spore_count) {
        return;
    }

    Spore spore = spores[sporeID];

    // Randomize position
    vec2 seed = vec2(float(sporeID) / settings.spore_count, fract(float(sporeID) * float(0.17)));

    spore.position = vec4(
    random(seed) * float(settings.grid_size),
    random(seed + vec2(0.1, 0.2)) * float(settings.grid_size),
    random(seed + vec2(0.2, 0.3)) * float(settings.grid_size),
    0.0);

    // Randomize orientation (yaw and pitch)
    float randomYaw = random(seed + vec2(0.3, 0.4)) * 2.0 * 3.14159265359;   // Yaw in [0, 2π]
    float randomPitch = random(seed + vec2(0.4, 0.5)) * 3.14159265359; // Pitch in [0, π]

    // Compute rotation matrices
    mat3 yawRotation = mat3(
    vec3(cos(randomYaw), 0.0, -sin(randomYaw)),
    vec3(0.0, 1.0, 0.0),
    vec3(sin(randomYaw), 0.0, cos(randomYaw))
    );

    mat3 pitchRotation = mat3(
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, cos(randomPitch), sin(randomPitch)),
    vec3(0.0, -sin(randomPitch), cos(randomPitch))
    );

    spore.orientation = pitchRotation * yawRotation;

    // Write the updated spore back to the buffer
    spores[sporeID] = spore;
}
