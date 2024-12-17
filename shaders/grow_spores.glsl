#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[];
};

uniform int gridSize;
uniform float time;

void main() {
    uvec3 threadID = gl_GlobalInvocationID;

    if (threadID.x >= uint(gridSize) || threadID.y >= uint(gridSize) || threadID.z >= uint(gridSize)) {
        return;
    }

    int index = int(threadID.x + gridSize * (threadID.y + gridSize * threadID.z));

    // Ensure index is within range
    if (index < 0 || index >= gridSize * gridSize * gridSize) {
        return;
    }

    float x = float(threadID.x) / float(gridSize);
    float y = float(threadID.y) / float(gridSize);
    float z = float(threadID.z) / float(gridSize);

    voxelData[index] = 0.5 + 0.5 * sin(5.0 * (x + y + z) + time);
}
