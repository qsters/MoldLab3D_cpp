#version 430


// Simulation Settings
#define SIMULATION_SETTINGS

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(binding = 0, r32f) uniform image3D voxelData;

layout(std430, binding = 1) buffer SettingsBuffer {
    SimulationData settings;
};


void main() {
    // Get the 3D indices of the current work item
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint z = gl_GlobalInvocationID.z;

    // Ensure the indices are within the bounds of the grid
    if (x >= uint(settings.grid_size) || y >= uint(settings.grid_size) || z >= uint(settings.grid_size)) {
        return;
    }

    imageStore(voxelData, ivec3(x, y, z), vec4(0.0));
}
