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
    : GameEngine(width, height, title), triangleVbo(0), triangleVao(0), shaderProgram(0), screenSizeLocation(0),
      voxelGrid{} {
}

MoldLabGame::~MoldLabGame() {
    if (triangleVbo) glDeleteBuffers(1, &triangleVbo);
    if (triangleVao) glDeleteVertexArrays(1, &triangleVao);

    std::cout << "Exiting..." << std::endl;
}



void MoldLabGame::start() {
    glGenBuffers(1, &triangleVbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Load and compile shaders
    auto [vertexShaderCode, fragmentShaderCode] = LoadCombinedShaderSource("shaders/quad_renderer.glsl");

    GLuint vertexShader = CompileShader(vertexShaderCode, GL_VERTEX_SHADER);
    CheckShaderCompilation(vertexShader);

    GLuint fragmentShader = CompileShader(fragmentShaderCode, GL_FRAGMENT_SHADER);
    CheckShaderCompilation(fragmentShader);

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    CheckProgramLinking(shaderProgram);

    // Get locations of shader attributes and uniforms
    screenSizeLocation = glGetUniformLocation(shaderProgram, "screenSize");
    GLint positionAttributeLocation = glGetAttribLocation(shaderProgram, "position");

    glGenVertexArrays(1, &triangleVao);
    glBindVertexArray(triangleVao);

    glGenBuffers(1, &triangleVbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
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
    auto [windowWidth, windowHeight] = getScreenSize();

    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen and depth buffer

    glUseProgram(shaderProgram);

    if (screenSizeLocation != -1) {
        glUniform2f(static_cast<GLint>(screenSizeLocation), static_cast<GLfloat>(windowWidth), static_cast<GLfloat>(windowHeight));
    }

    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}