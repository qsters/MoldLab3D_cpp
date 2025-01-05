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

uniform int maxSporeSize;
uniform float gridResizeFactor; // Maybe a better name than this would be better?

void main() {
    uint sporeID = gl_GlobalInvocationID.x;

    // Check bounds
    if (sporeID >= maxSporeSize) {
        return;
    }

    Spore spore = spores[sporeID];

    // Scale the spore position based on the gridRatio
    spore.position.xyz *= gridResizeFactor;

    // Write the updated spore back to the buffer
    spores[sporeID] = spore;
}
