#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <linmath.h>

// Vertex definition
typedef struct Vertex {
    vec3 position; // 3D position of the vertex
} Vertex;

// Full-screen quad vertices (static const for reuse)
static const Vertex quadVertices[6] = {
    {{-1.0f, -1.0f, 0.0f}}, // Bottom-left
    {{ 1.0f, -1.0f, 0.0f}}, // Bottom-right
    {{ 1.0f,  1.0f, 0.0f}}, // Top-right

    {{-1.0f, -1.0f, 0.0f}}, // Bottom-left
    {{ 1.0f,  1.0f, 0.0f}}, // Top-right
    {{-1.0f,  1.0f, 0.0f}}  // Top-left
};

#endif // MESH_DATA_H
