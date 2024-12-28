#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// Simulation Settings
#define SIMULATION_SETTINGS


layout(std430, binding = 1) buffer SettingsBuffer {
    SimulationData settings;
};

layout(binding = 0, r32f) uniform image3D voxelData;

// Using image3D for SDF data
layout(rgba32f, binding = 1) uniform writeonly image3D sdfData;


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

    highGridStart = clamp(highGridStart, ivec3(0), ivec3(settings.grid_size - 1));
    highGridEnd = clamp(highGridEnd, ivec3(0), ivec3(settings.grid_size - 1));


    // Initialize the SDF cell as empty
    vec4 sdfEntry = vec4(-1.0, -1.0, -1.0, 1e6);

    // Search within the corresponding high-resolution area
    for (int z = highGridStart.z; z <= highGridEnd.z; ++z) {
        for (int y = highGridStart.y; y <= highGridEnd.y; ++y) {
            for (int x = highGridStart.x; x <= highGridEnd.x; ++x) {
                // Compute 1D index for the high-resolution voxel grid
                int highIndex = x + settings.grid_size * (y + settings.grid_size * z);

                // Check if the voxel is filled
                if (imageLoad(voxelData, ivec3(x,y,z)).x > 0.0) {

                    // Mark the reduced grid cell as having a value
                    sdfEntry = vec4(reducedGridPos * sdfReductionFactor, 0.0);
                    break; // Exit the loop early if any voxel is filled
                }
            }
            if (sdfEntry.w == 0.0) break; // Exit the outer loop early
        }
        if (sdfEntry.w == 0.0) break; // Exit the outer loop early
    }

    // TODO: Remove, this is for testing
//    if (reducedGridPos == ivec3(9, 9, 9)) {
//        sdfEntry = vec4(reducedGridPos * sdfReductionFactor, 0.0); // Ensure (0, 0, 0) is defaulted if needed
//    }

    // Write the result to the reduced SDF grid
    imageStore(sdfData, reducedGridPos, sdfEntry);
}
