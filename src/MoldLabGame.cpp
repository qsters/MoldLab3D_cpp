#include <iostream>
#include <linmath.h>
#include <cmath>
#include "MoldLabGame.h"
#include "MeshData.h"

// ============================
// Constructor/Destructor
// ============================


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

// ============================
// Initialization Helpers
// ============================
void MoldLabGame::initializeShaders() {
    // Load and compile shaders
    auto [vertexShaderCode, fragmentShaderCode] = MoldLabGame::LoadCombinedShaderSource("shaders/quad_renderer.glsl");

    GLuint vertexShader = CompileShader(vertexShaderCode, GL_VERTEX_SHADER);

    GLuint fragmentShader = CompileShader(fragmentShaderCode, GL_FRAGMENT_SHADER);

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    CheckProgramLinking(shaderProgram);

    std::string growSporesShaderRaw = LoadShaderSource("shaders/grow_spores.glsl");

    GLuint growsSporesShader = CompileShader(growSporesShaderRaw, GL_COMPUTE_SHADER);

    growSporesShaderProgram = glCreateProgram();
    glAttachShader(growSporesShaderProgram, growsSporesShader);
    glLinkProgram(growSporesShaderProgram);
    CheckProgramLinking(growSporesShaderProgram);
}

void MoldLabGame::initializeUniformVariables() {
    static vec3 cameraPosition = {GRID_SIZE / 2.0, GRID_SIZE * 1.4, GRID_SIZE * 1.4};
    static vec3 focusPoint = {0.0f, 0.0f, 0.0f};
    static int gridSize = GRID_SIZE;
    static float testValue = 1.0f;

    cameraPositionSV = ShaderVariable(shaderProgram, &cameraPosition, "cameraPosition");
    focusPointSV = ShaderVariable(shaderProgram, &focusPoint, "focusPoint");
    gridSizeSV = ShaderVariable(shaderProgram, &gridSize, "gridSize");
    testValueSV = ShaderVariable(shaderProgram, &testValue, "testValue");
}


void MoldLabGame::initializeVertexBuffers() {
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

void MoldLabGame::initializeVoxelGridBuffer() {
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


// ============================
// Update Helpers
// ============================
void MoldLabGame::HandleCameraMovement(float orbitRadius, float deltaTime) {
    static float angle = 0.0f; // Current angle of rotation

    // Update the angle
    angle += ROTATION_SPEED * deltaTime;

    float gridCenter = (GRID_SIZE - 1.0f) * 0.5f; // Adjust for the centered cube positions
    set_vec3(*focusPointSV.value, gridCenter, gridCenter, gridCenter);

    // Adjust horizontal angle
    if (inputState.isLeftPressed) {
        horizontalAngle += ROTATION_SPEED * deltaTime;
    }
    if (inputState.isRightPressed) {
        horizontalAngle -= ROTATION_SPEED * deltaTime;
    }

    // Adjust vertical angle (clamped to avoid flipping)
    if (inputState.isUpPressed) {
        verticalAngle = std::min(verticalAngle + ROTATION_SPEED * deltaTime, 89.0f);
    }
    if (inputState.isDownPressed) {
        verticalAngle = std::max(verticalAngle - ROTATION_SPEED * deltaTime, -89.0f);
    }

    // Ensure angles wrap around properly
    if (horizontalAngle > 360.0f) horizontalAngle -= 360.0f;
    if (horizontalAngle < 0.0f) horizontalAngle += 360.0f;
    // Convert angles to radians
    float azimuth = horizontalAngle * static_cast<float>(M_PI) / 180.0f; // Azimuth in radians
    float altitude = verticalAngle * static_cast<float>(M_PI) / 180.0f; // Altitude in radians

    // Compute camera position in spherical coordinates relative to the origin
    float x = orbitRadius * cos(altitude) * sin(azimuth);
    float y = orbitRadius * sin(altitude);
    float z = orbitRadius * cos(altitude) * cos(azimuth);

    // Get the focus point position
    const vec3& focusPoint = *focusPointSV.value;

    // Offset camera position by focus point
    set_vec3(*cameraPositionSV.value, focusPoint[0] + x, focusPoint[1] + y, focusPoint[2] + z);
}

void MoldLabGame::UpdateTestValue(float deltaTime) const {
    float testScaler = 1.5f;
    // Adjust testValueSV based on key state
    float& testValue = *testValueSV.value; // Assume `testValueSV` is already initialized

    if (inputState.isDPressed) {
        testValue += deltaTime * testScaler; // Change rate is 1.0 units per second
        std::cout << "Test value: " << testValue << std::endl;
    }
    if (inputState.isAPressed) {
        testValue -= deltaTime * testScaler; // Same rate but in the opposite direction
        std::cout << "Test value: " << testValue << std::endl;
    }
}




void MoldLabGame::DispatchComputeShaders() const {

    glUseProgram(growSporesShaderProgram);

    // Pass the grid size and time uniform values
    glUniform1i(glGetUniformLocation(growSporesShaderProgram, "gridSize"), GRID_SIZE);
    glUniform1f(glGetUniformLocation(growSporesShaderProgram, "time"), TimeSinceStart());

    // Dispatch the compute shader
    int groupCount = (GRID_SIZE + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE; // Ensure proper division for grid
    glDispatchCompute(groupCount, groupCount, groupCount);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL Error: " << err << std::endl;
    }

    // Ensure the compute shader writes finish before proceeding
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}



// ============================
// Core Lifecycle Functions
// ============================
void MoldLabGame::renderingStart() {
    ComputeShaderInitializationDebug();

    initializeShaders();

    initializeUniformVariables();

    initializeVertexBuffers();

    initializeVoxelGridBuffer();
}

void MoldLabGame::start() {
    inputManager.bindKeyState(GLFW_KEY_D, &inputState.isDPressed);
    inputManager.bindKeyState(GLFW_KEY_A, &inputState.isAPressed);
    inputManager.bindKeyState(GLFW_KEY_LEFT, &inputState.isLeftPressed);
    inputManager.bindKeyState(GLFW_KEY_RIGHT, &inputState.isRightPressed);
    inputManager.bindKeyState(GLFW_KEY_UP, &inputState.isUpPressed);
    inputManager.bindKeyState(GLFW_KEY_DOWN, &inputState.isDownPressed);

}

void MoldLabGame::update(float deltaTime) {
    HandleCameraMovement(GRID_SIZE * 1.3, deltaTime);

    UpdateTestValue(deltaTime);

    DispatchComputeShaders();
}


void MoldLabGame::render() {
    // While using the
    glUseProgram(shaderProgram);

    gridSizeSV.uploadToShader();
    cameraPositionSV.uploadToShader();
    focusPointSV.uploadToShader();
    testValueSV.uploadToShader(true);

    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}