#include <iostream>
#include <linmath.h>
#include <cmath>
#include "MoldLabGame.h"
#include "MeshData.h"
#include "imgui.h"


// ============================
// Constructor/Destructor
// ============================

MoldLabGame::MoldLabGame(int width, int height, const std::string& title)
: GameEngine(width, height, title) {
    spores = new Spore[SPORE_COUNT]();


    displayFramerate = true;
}

MoldLabGame::~MoldLabGame() {
    if (triangleVbo) glDeleteBuffers(1, &triangleVbo);
    if (triangleVao) glDeleteVertexArrays(1, &triangleVao);
    if (voxelGridBuffer) glDeleteBuffers(1, &voxelGridBuffer);
    if (sporesBuffer) glDeleteBuffers(1, &sporesBuffer);
    if (simulationSettingsBuffer) glDeleteBuffers(1, &simulationSettingsBuffer);

    delete[] spores;

    std::cout << "Exiting..." << std::endl;
}

// ============================
// Initialization Helpers
// ============================
void MoldLabGame::initializeShaders() {
    // Load and compile shaders
    auto [vertexShaderCode, fragmentShaderCode] = LoadCombinedShaderSource("shaders/renderer.glsl");

    GLuint vertexShader = CompileShader(vertexShaderCode, GL_VERTEX_SHADER);

    GLuint fragmentShader = CompileShader(fragmentShaderCode, GL_FRAGMENT_SHADER);

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    CheckProgramLinking(shaderProgram);

    GLuint growsSporesShader = CompileShader(LoadShaderSource("shaders/draw_spores.glsl"), GL_COMPUTE_SHADER);

    drawSporesShaderProgram = glCreateProgram();
    glAttachShader(drawSporesShaderProgram, growsSporesShader);
    glLinkProgram(drawSporesShaderProgram);
    CheckProgramLinking(drawSporesShaderProgram);

    GLuint moveSporesShader = CompileShader(LoadShaderSource("shaders/move_spores.glsl"), GL_COMPUTE_SHADER);

    moveSporesShaderProgram = glCreateProgram();
    glAttachShader(moveSporesShaderProgram, moveSporesShader);
    glLinkProgram(moveSporesShaderProgram);
    CheckProgramLinking(moveSporesShaderProgram);

    GLuint decaySporesShader = CompileShader(LoadShaderSource("shaders/decay_spores.glsl"),GL_COMPUTE_SHADER);

    decaySporesShaderProgram = glCreateProgram();
    glAttachShader(decaySporesShaderProgram, decaySporesShader);
    glLinkProgram(decaySporesShaderProgram);
    CheckProgramLinking(decaySporesShaderProgram);

    GLuint jfaInitShader = CompileShader(LoadShaderSource("shaders/jump_flood_init.glsl"), GL_COMPUTE_SHADER);

    jumpFloodInitShaderProgram = glCreateProgram();
    glAttachShader(jumpFloodInitShaderProgram, jfaInitShader);
    glLinkProgram(jumpFloodInitShaderProgram);
    CheckProgramLinking(jumpFloodInitShaderProgram);

    GLuint jfaStepShader = CompileShader(LoadShaderSource("shaders/jump_flood_step.glsl"), GL_COMPUTE_SHADER);

    jumpFloodStepShaderProgram = glCreateProgram();
    glAttachShader(jumpFloodStepShaderProgram, jfaStepShader);
    glLinkProgram(jumpFloodStepShaderProgram);
    CheckProgramLinking(jumpFloodStepShaderProgram);
}

void MoldLabGame::initializeUniformVariables() {
    static vec3 cameraPosition = {GRID_SIZE / 2.0, GRID_SIZE * 1.4, GRID_SIZE * 1.4};
    static vec3 focusPoint = {0.0f, 0.0f, 0.0f};
    static int gridSize = GRID_SIZE;
    static int jfaStep = GRID_SIZE;
    static float deltaTimeStorage;

    cameraPositionSV = ShaderVariable(shaderProgram, &cameraPosition, "cameraPosition");
    focusPointSV = ShaderVariable(shaderProgram, &focusPoint, "focusPoint");
    gridSizeSV = ShaderVariable(shaderProgram, &gridSize, "gridSize");
    moveDeltaTimeSV = ShaderVariable(moveSporesShaderProgram, &deltaTimeStorage, "deltaTime");
    decayDeltaTimeSV = ShaderVariable(decaySporesShaderProgram, &deltaTimeStorage, "deltaTime");
    jfaStepSV = ShaderVariable(jumpFloodStepShaderProgram, &jfaStep, "stepSize");
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
    constexpr GLsizeiptr voxelGridSize = sizeof(float) * GRID_SIZE * GRID_SIZE * GRID_SIZE;

    // ** Create Voxel Grid Buffer **
    glGenBuffers(1, &voxelGridBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelGridBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, voxelGridSize, nullptr, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelGridBuffer); // Binding index 0
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind buffer
}

void MoldLabGame::initializeSDFBuffer() {
    int reducedGridSize = GRID_SIZE / SDF_REDUCTION_FACTOR;

    if (GRID_SIZE % SDF_REDUCTION_FACTOR != 0) {
        std::cerr << "Warning: GRID_SIZE is not evenly divisible by SDF_REDUCTION_FACTOR!" << std::endl;
    }

    glGenTextures(1, &sdfTexBuffer1);
    glBindTexture(GL_TEXTURE_3D, sdfTexBuffer1);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, reducedGridSize, reducedGridSize, reducedGridSize);

    glGenTextures(1, &sdfTexBuffer2);
    glBindTexture(GL_TEXTURE_3D, sdfTexBuffer2);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, reducedGridSize, reducedGridSize, reducedGridSize);

}


void uploadSettingsBuffer(GLuint simulationSettingsBuffer, SimulationSettings& settings) {
    glGenBuffers(1, &simulationSettingsBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, simulationSettingsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SimulationSettings), &settings, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, simulationSettingsBuffer); // Binding index 2 for settings

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
}

void MoldLabGame::initializeSimulationBuffers() {
    GLsizeiptr sporesSize = sizeof(Spore) * SPORE_COUNT;


    glGenBuffers(1, &sporesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sporesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sporesSize, spores, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sporesBuffer); // Binding index 1 for spores
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind

    // **Settings Buffer**
    uploadSettingsBuffer(simulationSettingsBuffer, simulationSettings);

    // Free the spores memory as it's no longer needed
    delete[] spores;
    spores = nullptr; // Set to nullptr for safety
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

void MoldLabGame::DispatchComputeShaders() {
    uploadSettingsBuffer(simulationSettingsBuffer, simulationSettings);

    glUseProgram(decaySporesShaderProgram);

    decayDeltaTimeSV.uploadToShader();

    DispatchComputeShader(decaySporesShaderProgram, GRID_SIZE, GRID_SIZE, GRID_SIZE);

    glUseProgram(moveSporesShaderProgram);

    moveDeltaTimeSV.uploadToShader();

    DispatchComputeShader(moveSporesShaderProgram, SPORE_COUNT, 1, 1);
    DispatchComputeShader(drawSporesShaderProgram, SPORE_COUNT, 1, 1);

    glUseProgram(jumpFloodInitShaderProgram);

    GLuint readTexture = sdfTexBuffer1;
    GLuint writeTexture = sdfTexBuffer2; // will be used later

    glBindImageTexture(0, readTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F); // set it to write only


    int reducedGridSize = GRID_SIZE / SDF_REDUCTION_FACTOR;
    DispatchComputeShader(jumpFloodInitShaderProgram, reducedGridSize, reducedGridSize, reducedGridSize); // inits the read, for later use
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);



    glUseProgram(jumpFloodStepShaderProgram);

    int stepSize = reducedGridSize / 2; // TODO: Make sure this is correct, may need to be doubled
    int iterations = 0;
    int testStopping = 1;

    while (stepSize >= 1) {
        iterations++;
        glBindImageTexture(0, readTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(1, writeTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);


        *jfaStepSV.value = stepSize;
        jfaStepSV.uploadToShader();


        DispatchComputeShader(jumpFloodStepShaderProgram, reducedGridSize, reducedGridSize , reducedGridSize);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        stepSize /= 2; // Halve step size
        std::swap(readTexture, writeTexture);

        if (iterations == testStopping) {
            // break;
        }
    }

    glBindImageTexture(0, readTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F); // set to read after last swap for rendering
}


// ============================
// Core Lifecycle Functions
// ============================
void MoldLabGame::renderingStart() {
    initializeShaders();

    initializeUniformVariables();

    initializeVertexBuffers();

    initializeVoxelGridBuffer();

    initializeSDFBuffer();


    simulationSettings.spore_count = SPORE_COUNT;
    simulationSettings.grid_size = GRID_SIZE;
    simulationSettings.sdf_reduction = SDF_REDUCTION_FACTOR;
    simulationSettings.spore_speed = SPORE_SPEED;
    simulationSettings.decay_speed = SPORE_DECAY;
    simulationSettings.turn_speed = SPORE_TURN_SPEED;
    simulationSettings.sensor_distance = SPORE_SENSOR_DISTANCE;

    srand(static_cast<unsigned int>(time(0)));

    for (int i = 0; i < SPORE_COUNT; i++) {
        Spore spore;

        // Random position between 0 and GRID_SIZE - 1
        spore.position[0] = static_cast<float>(rand() % GRID_SIZE);
        spore.position[1] = static_cast<float>(rand() % GRID_SIZE);
        spore.position[2] = static_cast<float>(rand() % GRID_SIZE);

        // Random direction vector (normalized)
        float dirX = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f; // Range [-1, 1]
        float dirY = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
        float dirZ = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;

        float magnitude = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        spore.direction[0] = dirX / magnitude;
        spore.direction[1] = dirY / magnitude;
        spore.direction[2] = dirZ / magnitude;

        spores[i] = spore;
    }


    initializeSimulationBuffers();
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

    *moveDeltaTimeSV.value = deltaTime;
    *decayDeltaTimeSV.value = deltaTime;

    DispatchComputeShaders();
}


void MoldLabGame::render() {
    // While using the
    glUseProgram(shaderProgram);

    gridSizeSV.uploadToShader();
    cameraPositionSV.uploadToShader();
    focusPointSV.uploadToShader();
    glUniform1i(glGetUniformLocation(shaderProgram, "sdfReductionFactor"), SDF_REDUCTION_FACTOR);


    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void MoldLabGame::renderUI() {
    ImGui::GetStyle().Alpha = 0.8f;

    // Add sliders for test values or other parameters
    ImGui::Begin("Simulation Settings"); // Begin a window
    ImGui::SliderFloat("Spore Speed", &simulationSettings.spore_speed, 0.0f, 50.0f);
    ImGui::SliderFloat("Turn Speed", &simulationSettings.turn_speed, 0.0f, 10.0f);
    ImGui::SliderFloat("Decay Speed", &simulationSettings.decay_speed, 0.0f, 10.0f);
    ImGui::SliderFloat("Sensor Distance", &simulationSettings.sensor_distance, 0.0f, GRID_SIZE / 4.0);
    ImGui::End(); // End the window
}
