#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

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

layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[]; // 1D array representing the voxel grid
};

layout(std430, binding = 2) buffer SettingsBuffer {
    SimulationSettings settings;
};

layout(std430, binding = 3) buffer ReadSDFGrid {
    vec4 readSDFData[]; // 1D array for SDF
};

layout(std430, binding = 4) buffer WriteSDFGrid {
    vec4 writeSDFData[]; // 1D array for SDF
};


uniform int stepSize;
uniform int sdfReductionFactor;
void main() {
    // Calculate 3D grid position from global invocation ID
    ivec3 gridPos = ivec3(gl_GlobalInvocationID.xyz);

    int sdfReductionFactor = settings.sdf_reduction;
    int gridSize = settings.grid_size / sdfReductionFactor; // Use reduced grid size if applicable

    // Compute 1D index for the current cell
    int index = gridPos.x + gridSize * (gridPos.y + gridSize * gridPos.z);

    // Read the current value
    vec4 currentValue = readSDFData[index];

    // Initialize the output with the current value
    vec4 outputValue = currentValue;

    // Skip if this cell already has w == 0.0
    if (currentValue.w <= 0.0) {
        writeSDFData[index] = currentValue; // Pass the value through unchanged
        return;
    }

    // Define the relative neighbor positions based on stepSize
    ivec3 neighborOffsets[] = ivec3[](
    ivec3(-1, 0, 0),
    ivec3(1, 0, 0),
    ivec3(0, -1, 0),
    ivec3(0, 1, 0),
    ivec3(0, 0, -1),
    ivec3(0, 0, 1),
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
    ivec3(0, 1, 1)
    // Add more offsets if needed for diagonal neighbors
    );

    // Iterate through all neighbors
    for (int i = 0; i < neighborOffsets.length(); ++i) {
        ivec3 neighborPos = gridPos + neighborOffsets[i] * stepSize;
        neighborPos = clamp(neighborPos, ivec3(0), ivec3(gridSize - 1)); // Ensure within bounds

        int neighborIndex = neighborPos.x + gridSize * (neighborPos.y + gridSize * neighborPos.z);

        // Check the neighbor's value
        if (readSDFData[neighborIndex].w <= 0.0) {
            // Update the current cell's w to 0.0
            outputValue.w = 0.0;
            break; // Exit early if any neighbor has w == 0.0
        }
    }

    // Write the updated value to the write buffer
    writeSDFData[index] = outputValue;
}
