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
    float voxelData[]; // 1D array representing the high-resolution voxel grid
};

layout(std430, binding = 2) buffer SettingsBuffer {
    SimulationSettings settings;
};

// Make sure binding 3 is the current Read buffer when resetting it
layout(std430, binding = 3) buffer SDFGrid {
    vec4 sdfData[]; // 1D array for the reduced-resolution SDF grid
};


void main() {
    // Calculate 3D position in the reduced grid
    ivec3 reducedGridPos = ivec3(gl_GlobalInvocationID.xyz);

    int sdfReductionFactor = settings.sdf_reduction;

    // Compute 1D index for the reduced grid
    int reducedGridSize = settings.grid_size / sdfReductionFactor;
    int reducedIndex = reducedGridPos.x + reducedGridSize * (reducedGridPos.y + reducedGridSize * reducedGridPos.z);

    // Determine the corresponding high-resolution area to search
    ivec3 highGridStart = reducedGridPos * sdfReductionFactor;
    ivec3 highGridEnd = highGridStart + (sdfReductionFactor - 1);

    // Initialize the SDF cell as empty
    vec4 sdfEntry = vec4(-1.0, -1.0, -1.0, 1e6);



    // Search within the corresponding high-resolution area
    for (int z = highGridStart.z; z <= highGridEnd.z; ++z) {
        for (int y = highGridStart.y; y <= highGridEnd.y; ++y) {
            for (int x = highGridStart.x; x <= highGridEnd.x; ++x) {
                // Compute 1D index for the high-resolution voxel grid
                int highIndex = x + settings.grid_size * (y + settings.grid_size * z);

                // Check if the voxel is filled
                if (voxelData[highIndex] > 0.0) {

                    // Mark the reduced grid cell as having a value
                    sdfEntry = vec4(reducedGridPos * sdfReductionFactor, 0.0);
                    break; // Exit the loop early if any voxel is filled
                }
            }
            if (sdfEntry.w == 0.0) break; // Exit the outer loop early
        }
        if (sdfEntry.w == 0.0) break; // Exit the outer loop early
    }

    // Write the result to the reduced SDF grid
    sdfData[reducedIndex] = sdfEntry;
}
