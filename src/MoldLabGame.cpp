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

void set_vec3(vec3 v, float x, float y, float z) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
}



MoldLabGame::MoldLabGame(int width, int height, const std::string& title)
: GameEngine(width, height, title), triangleVbo(0), triangleVao(0), shaderProgram(0), voxelGrid{} {

}

MoldLabGame::~MoldLabGame() {
    if (triangleVbo) glDeleteBuffers(1, &triangleVbo);
    if (triangleVao) glDeleteVertexArrays(1, &triangleVao);

    std::cout << "Exiting..." << std::endl;
}

void MoldLabGame::renderingStart() {
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

    static vec3 cameraPosition = {0.0f, 0.0f, 0.0f};
    static vec3 focusPoint = {0.0f, 0.0f, 0.0f};

    cameraPositionSV = ShaderVariable(shaderProgram, &cameraPosition, "cameraPosition");
    focusPointSV = ShaderVariable(shaderProgram, &focusPoint, "focusPoint");

 
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
}


void MoldLabGame::start() {

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
    static const float ROTATION_SPEED = 0.5f; // Radians per second
    static float angle = 0.0f; // Current angle of rotation

    // Update the angle
    angle += ROTATION_SPEED * deltaTime;

    vec3& cameraPosition = *cameraPositionSV.value;

    // Compute new camera position in a circular path around the origin
    float radius = 10.0f; // Distance from the origin
    cameraPosition[0] = radius * cos(angle); // X-coordinate
    cameraPosition[2] = radius * sin(angle); // Z-coordinate
    cameraPosition[1] = 2.0f; // Keep Y-coordinate fixed

    // Optional: Reset angle to prevent overflow
    if (angle > 2.0f * M_PI) {
        angle -= 2.0f * M_PI;
    }
}

void MoldLabGame::render() {
    // Handle window size and aspect ratio
    auto [windowWidth, windowHeight] = getScreenSize();

    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen and depth buffer

    glUseProgram(shaderProgram);


    cameraPositionSV.uploadToShader();
    focusPointSV.uploadToShader();
    
    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}