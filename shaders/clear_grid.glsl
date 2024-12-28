#version 430


// Simulation Settings
#define SIMULATION_SETTINGS

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[]; // 1D array representing the voxel grid
};

layout(std430, binding = 2) buffer SettingsBuffer {
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

    // Calculate the linear index of the voxel
    uint idx = z * settings.grid_size * settings.grid_size + y * settings.grid_size + x;

    // Reset voxel
    voxelData[idx] = 0.0;
}
