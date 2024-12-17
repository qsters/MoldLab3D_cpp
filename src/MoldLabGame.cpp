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

void LoadAndCompileShaders(GLuint& shaderProgram, GLuint& growSporesShaderProgram) {
    // Load and compile shaders
    auto [vertexShaderCode, fragmentShaderCode] = MoldLabGame::LoadCombinedShaderSource("shaders/quad_renderer.glsl");

    GLuint vertexShader = MoldLabGame::CompileShader(vertexShaderCode, GL_VERTEX_SHADER);

    GLuint fragmentShader = MoldLabGame::CompileShader(fragmentShaderCode, GL_FRAGMENT_SHADER);

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    MoldLabGame::CheckProgramLinking(shaderProgram);

    std::string growSporesShaderRaw = MoldLabGame::LoadShaderSource("shaders/grow_spores.glsl");

    GLuint growsSporesShader = MoldLabGame::CompileShader(growSporesShaderRaw, GL_COMPUTE_SHADER);

    growSporesShaderProgram = glCreateProgram();
    glAttachShader(growSporesShaderProgram, growsSporesShader);
    glLinkProgram(growSporesShaderProgram);
    MoldLabGame::CheckProgramLinking(growSporesShaderProgram);
}

void SetupVertexBuffers(GLuint shaderProgram, GLuint& triangleVao, GLuint& triangleVbo) {
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

void SetupVoxelGridBuffer(float (&voxelGrid)[GRID_SIZE][GRID_SIZE][GRID_SIZE], GLuint& voxelGridBuffer) {
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

void ComputeShaderInitializationDebug() {
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;

    if (GLVersion.major >= 4 && GLVersion.minor >= 3) {
        std::cout << "Compute Shaders Supported!" << std::endl;

        // Check the maximum compute work group counts
        GLint workGroupCount[3];
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupCount[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupCount[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupCount[2]);

        std::cout << "Max Compute Work Group Count: "
                  << workGroupCount[0] << ", "
                  << workGroupCount[1] << ", "
                  << workGroupCount[2] << std::endl;

        // Check the maximum work group size
        GLint workGroupSize[3];
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSize[2]);

        std::cout << "Max Compute Work Group Size: "
                  << workGroupSize[0] << ", "
                  << workGroupSize[1] << ", "
                  << workGroupSize[2] << std::endl;
    } else {
        std::cerr << "Compute Shaders Not Supported!" << std::endl;
    }
}

void MoldLabGame::renderingStart() {
    ComputeShaderInitializationDebug();

    // EnableDebugOutput();
    glGenBuffers(1, &triangleVbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    LoadAndCompileShaders(shaderProgram, growSporesShaderProgram);

    static vec3 cameraPosition = {GRID_SIZE / 2.0, GRID_SIZE * 1.4, GRID_SIZE * 1.4};
    static vec3 focusPoint = {0.0f, 0.0f, 0.0f};
    static int gridSize = GRID_SIZE;
    static float testValue = 1.0f;

    cameraPositionSV = ShaderVariable(shaderProgram, &cameraPosition, "cameraPosition");
    focusPointSV = ShaderVariable(shaderProgram, &focusPoint, "focusPoint");
    gridSizeSV = ShaderVariable(shaderProgram, &gridSize, "gridSize");
    testValueSV = ShaderVariable(shaderProgram, &testValue, "testValue");

    SetupVertexBuffers(shaderProgram, triangleVao, triangleVbo);

    SetupVoxelGridBuffer(voxelGrid, voxelGridBuffer);
}


void MoldLabGame::start() {
    inputManager.bindKeyState(GLFW_KEY_D, &isDPressed);
    inputManager.bindKeyState(GLFW_KEY_A, &isAPressed);
    inputManager.bindKeyState(GLFW_KEY_LEFT, &isLeftPressed);
    inputManager.bindKeyState(GLFW_KEY_RIGHT, &isRightPressed);
    inputManager.bindKeyState(GLFW_KEY_UP, &isUpPressed);
    inputManager.bindKeyState(GLFW_KEY_DOWN, &isDownPressed);
}

void HandleCameraMovement(float &horizontalAngle, float &verticalAngle, float orbitRadius,
                          const ShaderVariable<vec3>& cameraPositionSV, const ShaderVariable<vec3>& focusPointSV, float deltaTime) {
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

void UpdateTestValue(const ShaderVariable<float>& testValueSV, float deltaTime) {
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

void RunComputeShader(GLuint& growSporesShaderProgram, int gridSize, float time) {

    glUseProgram(growSporesShaderProgram);

    // Pass the grid size and time uniform values
    glUniform1i(glGetUniformLocation(growSporesShaderProgram, "gridSize"), gridSize);
    glUniform1f(glGetUniformLocation(growSporesShaderProgram, "time"), time);

    // Dispatch the compute shader: assumes (8, 8, 8) local size
    int groupCount = (gridSize + 7) / 8; // Ensure proper division for grid
    glDispatchCompute(groupCount, groupCount, groupCount);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error: " << err << std::endl;
    }

    // Ensure the compute shader writes finish before proceeding
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


void MoldLabGame::update(float deltaTime) {
    HandleCameraMovement(horizontalAngle, verticalAngle, GRID_SIZE * 1.3, cameraPositionSV, focusPointSV, deltaTime);

    UpdateTestValue(testValueSV, deltaTime);

}

void MoldLabGame::render() {
    // Handle window size and aspect ratio
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen and depth buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelGridBuffer);

    RunComputeShader(growSporesShaderProgram, GRID_SIZE, TimeSinceStart());

    glUseProgram(shaderProgram);

    gridSizeSV.uploadToShader();
    cameraPositionSV.uploadToShader();
    focusPointSV.uploadToShader();
    testValueSV.uploadToShader(true);
    
    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
