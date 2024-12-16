#include "MoldLabGame.h"
#include <iostream>
#include <linmath.h>
#include <cmath>

static bool isDPressed = false;
static bool isAPressed = false;
static bool isLeftPressed = false;
static bool isRightPressed = false;
static bool isUpPressed = false;
static bool isDownPressed = false;


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
: GameEngine(width, height, title), voxelGrid{} {
    displayFramerate = true;
}

MoldLabGame::~MoldLabGame() {
    if (triangleVbo) glDeleteBuffers(1, &triangleVbo);
    if (triangleVao) glDeleteVertexArrays(1, &triangleVao);
    if (voxelGridBuffer) glDeleteBuffers(1, &voxelGridBuffer);
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

    static vec3 cameraPosition = {GRID_SIZE / 2.0, GRID_SIZE * 1.4, GRID_SIZE * 1.4};
    static vec3 focusPoint = {0.0f, 0.0f, 0.0f};
    static int gridSize = GRID_SIZE;
    static float testValue = 1.0f;

    cameraPositionSV = ShaderVariable(shaderProgram, &cameraPosition, "cameraPosition");
    focusPointSV = ShaderVariable(shaderProgram, &focusPoint, "focusPoint");
    gridSizeSV = ShaderVariable(shaderProgram, &gridSize, "gridSize");
    testValueSV = ShaderVariable(shaderProgram, &testValue, "testValue");


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
                bool placeCube = (std::rand() / (RAND_MAX + 1.0)) >= 0.75;

                if (placeCube) {
                    float random_number = std::rand() / (RAND_MAX + 1.0);
                    voxelGrid[x][y][z] = random_number;
                } else {
                    voxelGrid[x][y][z] = 0.0f;
                }
            }
        }
    }

    // ** Create Voxel Grid Buffer **
    glGenBuffers(1, &voxelGridBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelGridBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxelGrid), voxelGrid, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelGridBuffer); // Binding index 0
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind buffer
}


void MoldLabGame::start() {
    inputManager.bindKeyState(GLFW_KEY_D, &isDPressed);
    inputManager.bindKeyState(GLFW_KEY_A, &isAPressed);
    inputManager.bindKeyState(GLFW_KEY_LEFT, &isLeftPressed);
    inputManager.bindKeyState(GLFW_KEY_RIGHT, &isRightPressed);
    inputManager.bindKeyState(GLFW_KEY_UP, &isUpPressed);
    inputManager.bindKeyState(GLFW_KEY_DOWN, &isDownPressed);
}

void HandleCameraMovement(float horizontalAngle, float verticalAngle, float orbitRadius,
                          const ShaderVariable<vec3>& cameraPositionSV, const ShaderVariable<vec3>& focusPointSV) {
    // Convert angles to radians
    float azimuth = horizontalAngle * M_PI / 180.0f; // Azimuth in radians
    float altitude = verticalAngle * M_PI / 180.0f; // Altitude in radians

    // Compute camera position in spherical coordinates relative to the origin
    float x = orbitRadius * cos(altitude) * sin(azimuth);
    float y = orbitRadius * sin(altitude);
    float z = orbitRadius * cos(altitude) * cos(azimuth);

    // Get the focus point position
    const vec3& focusPoint = *focusPointSV.value;

    // Offset camera position by focus point
    set_vec3(*cameraPositionSV.value, focusPoint[0] + x, focusPoint[1] + y, focusPoint[2] + z);
}

void MoldLabGame::update(float deltaTime) {
    static const float ROTATION_SPEED = 35.0f; // Radians per second
    static float angle = 0.0f; // Current angle of rotation

    // Update the angle
    angle += ROTATION_SPEED * deltaTime;

    float gridCenter = (GRID_SIZE - 1.0f) * 0.5f; // Adjust for the centered cube positions
    set_vec3(*focusPointSV.value, gridCenter, gridCenter, gridCenter);

    // Adjust horizontal angle
    if (isLeftPressed) {
        horizontalAngle += ROTATION_SPEED * deltaTime;
    }
    if (isRightPressed) {
        horizontalAngle -= ROTATION_SPEED * deltaTime;
    }

    // Adjust vertical angle (clamped to avoid flipping)
    if (isUpPressed) {
        verticalAngle = std::min(verticalAngle + ROTATION_SPEED * deltaTime, 89.0f);
    }
    if (isDownPressed) {
        verticalAngle = std::max(verticalAngle - ROTATION_SPEED * deltaTime, -89.0f);
    }

    // Ensure angles wrap around properly
    if (horizontalAngle > 360.0f) horizontalAngle -= 360.0f;
    if (horizontalAngle < 0.0f) horizontalAngle += 360.0f;

    HandleCameraMovement(horizontalAngle, verticalAngle, GRID_SIZE * 1.3, cameraPositionSV, focusPointSV);

    float testScaler = 1.5f;
    // Adjust testValueSV based on key state
    float& testValue = *testValueSV.value; // Assume `testValueSV` is already initialized

    if (isDPressed) {
        testValue += deltaTime * testScaler; // Change rate is 1.0 units per second
        std::cout << "Test value: " << testValue << std::endl;
    }
    if (isAPressed) {
        testValue -= deltaTime * testScaler; // Same rate but in the opposite direction
        std::cout << "Test value: " << testValue << std::endl;
    }
}

void MoldLabGame::render() {
    // Handle window size and aspect ratio
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen and depth buffer

    glUseProgram(shaderProgram);

    gridSizeSV.uploadToShader();
    cameraPositionSV.uploadToShader();
    focusPointSV.uploadToShader();
    testValueSV.uploadToShader(true);
    
    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
