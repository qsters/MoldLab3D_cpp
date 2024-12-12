#include "MoldLabGame.h"
#include <iostream>
#include <linmath.h>
#include <cmath>

typedef struct Vertex {
    vec3 position; // 3D position of the vertex
} Vertex;

// Array of vertices defining a full-screen quad
static const Vertex quadVertices[6] = {
    {{-1.0f, -1.0f, 0.0f}}, // Bottom-left
    {{ 1.0f, -1.0f, 0.0f}}, // Bottom-right
    {{ 1.0f,  1.0f, 0.0f}}, // Top-right

    {{-1.0f, -1.0f, 0.0f}}, // Bottom-left
    {{ 1.0f,  1.0f, 0.0f}}, // Top-right
    {{-1.0f,  1.0f, 0.0f}}  // Top-left
};


MoldLabGame::MoldLabGame(int width, int height, const std::string& title)
    : GameEngine(width, height, title), triangle_vbo(0), triangle_vao(0), shaderProgram(0), screenSizeLocation(0),
      voxelGrid{} {
}

MoldLabGame::~MoldLabGame() {
    if (triangle_vbo) glDeleteBuffers(1, &triangle_vbo);
    if (triangle_vao) glDeleteVertexArrays(1, &triangle_vao);

    std::cout << "Exiting..." << std::endl;
}




void MoldLabGame::start() {
    glGenBuffers(1, &triangle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangle_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Load and compile shaders
    auto [vertex_shader_code, fragment_shader_code] = LoadCombinedShaderSource("shaders/quad_renderer.glsl");

    GLuint vertex_shader = CompileShader(vertex_shader_code, GL_VERTEX_SHADER);
    CheckShaderCompilation(vertex_shader);

    GLuint fragment_shader = CompileShader(fragment_shader_code, GL_FRAGMENT_SHADER);
    CheckShaderCompilation(fragment_shader);

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex_shader);
    glAttachShader(shaderProgram, fragment_shader);
    glLinkProgram(shaderProgram);

    CheckProgramLinking(shaderProgram);

    // Get locations of shader attributes and uniforms
    screenSizeLocation = glGetUniformLocation(shaderProgram, "screenSize");
    GLint positionAttributeLocation = glGetAttribLocation(shaderProgram, "position");

    glGenVertexArrays(1, &triangle_vao);
    glBindVertexArray(triangle_vao);

    glGenBuffers(1, &triangle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangle_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Enable and set the position attribute
    glEnableVertexAttribArray(positionAttributeLocation);
    glVertexAttribPointer(positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

    glBindVertexArray(0); // Unbind VAO

    // **Initialize the voxel grid with 1's to stress test
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                voxelGrid[x][y][z] = 1.0f;
            }
        }
    }
}

void MoldLabGame::update(float deltaTime) {

}

void MoldLabGame::render() {
    // Handle window size and aspect ratio
    auto [window_width, window_height] = getScreenSize();
    const float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);

    glViewport(0, 0, window_width, window_height); // Update the viewport
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen and depth buffer

    glUseProgram(shaderProgram);

    if (screenSizeLocation != -1) {
        glUniform2f(screenSizeLocation, static_cast<GLfloat>(window_width), static_cast<GLfloat>(window_height));
    }

    // Draw the full-screen quad
    glBindVertexArray(triangle_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}