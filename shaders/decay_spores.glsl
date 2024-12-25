#version 430


// Simulation Settings
#DEFINE_SIMULATION_SETTINGS

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[]; // 1D array representing the voxel grid
};

layout(std430, binding = 2) buffer SettingsBuffer {
    SimulationSettings settings;
};

uniform float deltaTime;


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

    // Apply decay to the voxel value
    voxelData[idx] = max(0.0, voxelData[idx] - settings.decay_speed * deltaTime);
}
