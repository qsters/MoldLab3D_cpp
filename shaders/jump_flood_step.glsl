#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// Simulation Settings
#DEFINE_SIMULATION_SETTINGS

layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[]; // 1D array representing the voxel grid
};

layout(std430, binding = 2) buffer SettingsBuffer {
    SimulationSettings settings;
};

layout(rgba32f, binding = 0) uniform readonly image3D readSDFData; // just updated to using textures instead of an array buffer
layout(rgba32f, binding = 1) uniform writeonly image3D writeSDFData;



uniform int stepSize;

void main() {
    // Calculate 3D grid position from global invocation ID
    ivec3 reducedGridPos = ivec3(gl_GlobalInvocationID.xyz);

    int sdfReductionFactor = settings.sdf_reduction;
    int gridSize = settings.grid_size / sdfReductionFactor; // Use reduced grid size if applicable

    vec3 worldGridPos = vec3(reducedGridPos * sdfReductionFactor);


    // Read the current value from the readSDFData texture
    vec4 currentValue = imageLoad(readSDFData, reducedGridPos);

    // Initialize the output with the current value
    vec4 outputValue = currentValue;

    // Skip if this cell already has w == 0.0
    if (currentValue.w <= 0.0) {
        imageStore(writeSDFData, reducedGridPos, currentValue); // Pass the value through unchanged
        return;
    }

    // Define the relative neighbor positions based on stepSize
    ivec3 neighborOffsets[] = ivec3[](
    // Direct neighbors (faces)
    ivec3(-1, 0, 0),
    ivec3(1, 0, 0),
    ivec3(0, -1, 0),
    ivec3(0, 1, 0),
    ivec3(0, 0, -1),
    ivec3(0, 0, 1),

    // Edge neighbors
    ivec3(-1, -1, 0),
    ivec3(-1, 1, 0),
    ivec3(1, -1, 0),
    ivec3(1, 1, 0),
    ivec3(-1, 0, -1),
    ivec3(-1, 0, 1),
    ivec3(1, 0, -1),
    ivec3(1, 0, 1),
    ivec3(0, -1, -1),
    ivec3(0, -1, 1),
    ivec3(0, 1, -1),
    ivec3(0, 1, 1),

    // Corner neighbors (diagonals)
    ivec3(-1, -1, -1),
    ivec3(-1, -1, 1),
    ivec3(-1, 1, -1),
    ivec3(-1, 1, 1),
    ivec3(1, -1, -1),
    ivec3(1, -1, 1),
    ivec3(1, 1, -1),
    ivec3(1, 1, 1)
    );


    // Iterate through all neighbors
    for (int i = 0; i < neighborOffsets.length(); ++i) {
        ivec3 neighborPos = reducedGridPos + neighborOffsets[i] * stepSize;
        neighborPos = clamp(neighborPos, ivec3(0), ivec3(gridSize - 1)); // Ensure within bounds

        // Load the neighbor's value from the readSDFData texture
        vec4 neighborValue = imageLoad(readSDFData, neighborPos);

        // Skip invalid neighbors
        if (neighborValue.w >= 1e6) {
            continue;
        }

        // Calculate the distance to the seed from this neighbor
        float distance = length(worldGridPos - neighborValue.xyz);

        // Update the current cell if the neighbor provides a closer seed
        if (distance < outputValue.w) {
            outputValue = vec4(neighborValue.xyz, distance);
        }
    }

    // Write the updated value to the writeSDFData texture
    imageStore(writeSDFData, reducedGridPos, outputValue);
}
