#include <iostream>
#include <linmath.h>
#include <cmath>
#include "MoldLabGame.h"
#include "MeshData.h"
#include "imgui.h"

const std::string USE_TRANSPARENCY_DEFINITION = "#define USE_TRANSPARENCY";
const std::string SIMULATION_SETTINGS_DEFINITION = "#define SIMULATION_SETTINGS";
const std::string SPORE_DEFINITION = "#define SPORE_STRUCT";
const std::string WRAP_GRID_DEFINITION = "#define WRAP_AROUND";


constexpr int GRID_TEXTURE_LOCATION = 0;
constexpr int SDF_TEXTURE_READ_LOCATION = 1;
constexpr int SDF_TEXTURE_WRITE_LOCATION = 2;

constexpr int SPORE_BUFFER_LOCATION = 0;
constexpr int SIMULATION_BUFFER_LOCATION = 1;

// ============================
// Constructor/Destructor
// ============================
void assignDefaultsToSimulationData(SimulationData& data) {
    data.spore_count = SimulationDefaults::SPORE_COUNT;
    data.grid_size = SimulationDefaults::GRID_SIZE;
    data.sdf_reduction = SimulationDefaults::SDF_REDUCTION_FACTOR;
    data.spore_speed = SimulationDefaults::SPORE_SPEED;
    data.decay_speed = SimulationDefaults::SPORE_DECAY;
    data.turn_speed = SimulationDefaults::SPORE_TURN_SPEED;
    data.sensor_distance = SimulationDefaults::SPORE_SENSOR_DISTANCE;
    data.sensor_angle = SimulationDefaults::SPORE_SENSOR_ANGLE;
}

MoldLabGame::MoldLabGame(const int width, const int height, const std::string &title)
    : GameEngine(width, height, title, false) {
    displayFramerate = true;

    addShaderDefinition(SIMULATION_SETTINGS_DEFINITION, "include/SimulationData.h");
    if (useTransparency) {
        addShaderDefinition(USE_TRANSPARENCY_DEFINITION, "");
    }
    if (wrapGrid) {
        addShaderDefinition(WRAP_GRID_DEFINITION, "");
    }
    addShaderDefinition(SPORE_DEFINITION, "include/Spore.h");

    // Set the simulation Settings to the Defaults
    assignDefaultsToSimulationData(simulationSettings);
}

MoldLabGame::~MoldLabGame() {
    if (triangleVbo)
        glDeleteBuffers(1, &triangleVbo);
    if (triangleVao)
        glDeleteVertexArrays(1, &triangleVao);
    if (sporesBuffer)
        glDeleteBuffers(1, &sporesBuffer);
    if (simulationSettingsBuffer)
        glDeleteBuffers(1, &simulationSettingsBuffer);
    if (voxelGridTexture)
        glDeleteTextures(1, &voxelGridTexture);
    if (sdfTexBuffer1)
        glDeleteTextures(1, &sdfTexBuffer1);
    if (sdfTexBuffer2)
        glDeleteTextures(1, &sdfTexBuffer2);

    std::cout << "Exiting..." << std::endl;
}

// ============================
// Initialization Helpers
// ============================
void MoldLabGame::initializeRenderShader(bool useTransparency) {
    if (!useTransparency) {
        addShaderDefinition(USE_TRANSPARENCY_DEFINITION, "");
    } else {
        removeShaderDefinition(USE_TRANSPARENCY_DEFINITION);
    }

    shaderProgram = CreateShaderProgram({
        {"shaders/renderer.glsl", GL_VERTEX_SHADER, true} // Combined vertex and fragment shaders
    });
}

void MoldLabGame::initializeMoveSporesShader(bool wrapAround) {
    if (!wrapAround) {
        addShaderDefinition(WRAP_GRID_DEFINITION, "");
    } else {
        removeShaderDefinition(WRAP_GRID_DEFINITION);
    }

    moveSporesShaderProgram = CreateShaderProgram({
        {"shaders/move_spores.glsl", GL_COMPUTE_SHADER, false}
    });
}

void MoldLabGame::initializeShaders() {
    initializeRenderShader(useTransparency);

    // Initialize the compute shaders
    drawSporesShaderProgram = CreateShaderProgram({
        {"shaders/draw_spores.glsl", GL_COMPUTE_SHADER, false}
    });

    initializeMoveSporesShader(wrapGrid);

    decaySporesShaderProgram = CreateShaderProgram({
        {"shaders/decay_spores.glsl", GL_COMPUTE_SHADER, false}
    });

    jumpFloodInitShaderProgram = CreateShaderProgram({
        {"shaders/jump_flood_init.glsl", GL_COMPUTE_SHADER, false}
    });

    jumpFloodStepShaderProgram = CreateShaderProgram({
        {"shaders/jump_flood_step.glsl", GL_COMPUTE_SHADER, false}
    });

    clearGridShaderProgram = CreateShaderProgram({
    {"shaders/clear_grid.glsl", GL_COMPUTE_SHADER, false}
    });

    randomizeSporesShaderProgram = CreateShaderProgram({
    {"shaders/randomize_spores.glsl", GL_COMPUTE_SHADER, false}
    });

    scaleSporesShaderProgram = CreateShaderProgram({
    {"shaders/scale_spores.glsl", GL_COMPUTE_SHADER, false}
    });
}


void MoldLabGame::initializeUniformVariables() {
    static int jfaStep = simulationSettings.grid_size;
    static int maxSporeSize = SimulationDefaults::SPORE_COUNT;

    jfaStepSV = ShaderVariable(jumpFloodStepShaderProgram, &jfaStep, "stepSize");
    maxSporeSizeSV = ShaderVariable(scaleSporesShaderProgram, &maxSporeSize, "maxSporeSize");
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
    glVertexAttribPointer(positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void *>(offsetof(Vertex, position)));

    glBindVertexArray(0); // Unbind VAO
}

void MoldLabGame::initializeVoxelGridBuffer() {
     int voxelGridSize = simulationSettings.grid_size;

    // ** Create Voxel Grid Texture **
    glGenTextures(1, &voxelGridTexture);
    glBindTexture(GL_TEXTURE_3D, voxelGridTexture);

    // Allocate storage for the 3D texture
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, voxelGridSize, voxelGridSize, voxelGridSize);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Bind the texture as an image unit for compute shader access
    glBindImageTexture(GRID_TEXTURE_LOCATION, voxelGridTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);

    glBindTexture(GL_TEXTURE_3D, 0); // Unbind the texture
}


void MoldLabGame::initializeSDFBuffer() {
     int reducedGridSize = simulationSettings.grid_size / simulationSettings.sdf_reduction;

    glGenTextures(1, &sdfTexBuffer1);
    glBindTexture(GL_TEXTURE_3D, sdfTexBuffer1);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, reducedGridSize, reducedGridSize, reducedGridSize);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);

    glGenTextures(1, &sdfTexBuffer2);
    glBindTexture(GL_TEXTURE_3D, sdfTexBuffer2);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, reducedGridSize, reducedGridSize, reducedGridSize);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, 0);
}


void uploadSettingsBuffer(GLuint simulationSettingsBuffer, const SimulationData &settings) {
    glGenBuffers(1, &simulationSettingsBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, simulationSettingsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SimulationData), &settings, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SIMULATION_BUFFER_LOCATION, simulationSettingsBuffer); // Binding index 2 for settings

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
}

void MoldLabGame::initializeSimulationBuffers() {
     GLsizeiptr sporesSize = sizeof(Spore) * simulationSettings.spore_count;


    glGenBuffers(1, &sporesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sporesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sporesSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPORE_BUFFER_LOCATION, sporesBuffer); // Binding index 1 for spores
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind

    // **Settings Buffer**
    uploadSettingsBuffer(simulationSettingsBuffer, simulationSettings);
}


// ============================
// Update Helpers
// ============================
void MoldLabGame::HandleCameraMovement(const float orbitRadius, const float deltaTime) {
    static float angle = 0.0f; // Current angle of rotation

    constexpr float rotationSpeed = 100.0f;

    // Update the angle
    angle += rotationSpeed * deltaTime;

     float gridCenter = (static_cast<float>(simulationSettings.grid_size) - 1.0f) * 0.5f; // Adjust for the centered cube positions
    set_vec4(simulationSettings.camera_focus, gridCenter, gridCenter, gridCenter, 0.0);

    // Adjust horizontal angle
    if (inputState.isLeftPressed) {
        horizontalAngle += rotationSpeed * deltaTime;
    }
    if (inputState.isRightPressed) {
        horizontalAngle -= rotationSpeed * deltaTime;
    }

    // Adjust vertical angle (clamped to avoid flipping)
    if (inputState.isUpPressed) {
        verticalAngle = std::min(verticalAngle + rotationSpeed * deltaTime, 89.0f);
    }
    if (inputState.isDownPressed) {
        verticalAngle = std::max(verticalAngle - rotationSpeed * deltaTime, -89.0f);
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
    const vec4 &focusPoint = simulationSettings.camera_focus;

    // Offset camera position by focus point
    set_vec4(simulationSettings.camera_position, focusPoint[0] + x, focusPoint[1] + y, focusPoint[2] + z, 0.0);
}

void MoldLabGame::clearGrid() const {
    const int gridSize = simulationSettings.grid_size;

    DispatchComputeShader(clearGridShaderProgram, gridSize, gridSize, gridSize);
}


void MoldLabGame::resetSporesAndGrid() const {
    clearGrid();
    DispatchComputeShader(randomizeSporesShaderProgram, simulationSettings.spore_count, 1, 1);
}


void MoldLabGame::DispatchComputeShaders() {
    int gridSize = simulationSettings.grid_size;

    uploadSettingsBuffer(simulationSettingsBuffer, simulationSettings);

    if (!gridSizeChanged) {
        DispatchComputeShader(decaySporesShaderProgram, gridSize, gridSize, gridSize);

        DispatchComputeShader(moveSporesShaderProgram, simulationSettings.spore_count, 1, 1);

        DispatchComputeShader(drawSporesShaderProgram, simulationSettings.spore_count, 1, 1);
    } else {
        glUseProgram(scaleSporesShaderProgram);
        maxSporeSizeSV.uploadToShader();

        DispatchComputeShader(scaleSporesShaderProgram, *maxSporeSizeSV.value, 1, 1);
        clearGrid();
    }

    gridSizeChanged = false;

    executeJFA();
}

void MoldLabGame::executeJFA() const {
    glUseProgram(jumpFloodInitShaderProgram);

    GLuint readTexture = sdfTexBuffer1;
    GLuint writeTexture = sdfTexBuffer2; // will be used later

    glBindImageTexture(SDF_TEXTURE_READ_LOCATION, readTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F); // set it to write ONLY for initialization


     int reducedGridSize = simulationSettings.grid_size / simulationSettings.sdf_reduction;
    DispatchComputeShader(jumpFloodInitShaderProgram, reducedGridSize, reducedGridSize, reducedGridSize);
    // inits the read, for later use
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


    glUseProgram(jumpFloodStepShaderProgram);

    int stepSize = reducedGridSize / 2;
    int iterations = 0;
    int testStopping = 1;

    while (stepSize >= 1 && testStopVale != 0) {
        iterations++;
        glBindImageTexture(SDF_TEXTURE_READ_LOCATION, readTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(SDF_TEXTURE_WRITE_LOCATION, writeTexture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);


        *jfaStepSV.value = stepSize;
        jfaStepSV.uploadToShader();


        DispatchComputeShader(jumpFloodStepShaderProgram, reducedGridSize, reducedGridSize, reducedGridSize);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        stepSize /= 2; // Halve step size
        std::swap(readTexture, writeTexture);

        if (iterations == testStopVale) {
            // break;
        }
    }

    glBindImageTexture(SDF_TEXTURE_READ_LOCATION, readTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    // set to read after last swap for rendering
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

    // initializeSpores();

    initializeSimulationBuffers();
}

void MoldLabGame::start() {
    inputManager.bindKeyState(GLFW_KEY_D, &inputState.isDPressed);
    inputManager.bindKeyState(GLFW_KEY_A, &inputState.isAPressed);
    inputManager.bindKeyState(GLFW_KEY_LEFT, &inputState.isLeftPressed);
    inputManager.bindKeyState(GLFW_KEY_RIGHT, &inputState.isRightPressed);
    inputManager.bindKeyState(GLFW_KEY_UP, &inputState.isUpPressed);
    inputManager.bindKeyState(GLFW_KEY_DOWN, &inputState.isDownPressed);


    resetSporesAndGrid();
}

void MoldLabGame::update(float deltaTime) {
    HandleCameraMovement(orbitRadius, deltaTime);

    simulationSettings.delta_time = deltaTime;

     float orbitDistanceChange = static_cast<float>(simulationSettings.grid_size) / 8.0f;

    if (inputState.isDPressed) {
        orbitRadius += orbitDistanceChange * deltaTime;
    } else if (inputState.isAPressed) {
        orbitRadius -= orbitDistanceChange * deltaTime;
    }

    DispatchComputeShaders();
}


void MoldLabGame::render() {
    // While using the
    glUseProgram(shaderProgram);

    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void MoldLabGame::renderUI() {
    ImGui::GetStyle().Alpha = 0.8f;

    // Add sliders for test values or other parameters
    ImGui::Begin("Simulation Settings"); // Begin a window
    ImGui::SliderInt("Spore Count", &simulationSettings.spore_count, 1, SimulationDefaults::SPORE_COUNT);
    ImGui::SliderInt("Test ing", &testStopVale, 0, 10);
    int previousGridSize = simulationSettings.grid_size;
    if (ImGui::SliderInt("Grid Size", &simulationSettings.grid_size, 10, SimulationDefaults::GRID_SIZE)) {
        // Ensure grid_size is divisible by sdf_reduction
        int reduction = simulationSettings.sdf_reduction;
        simulationSettings.grid_size = (simulationSettings.grid_size / reduction) * reduction;

        if (previousGridSize != simulationSettings.grid_size) {
            gridSizeChanged = true;
            float gridResizeFactor = static_cast<float>(simulationSettings.grid_size) / static_cast<float>(previousGridSize);
            simulationSettings.spore_speed *= gridResizeFactor;
            simulationSettings.sensor_distance *= gridResizeFactor;

            orbitRadius *= gridResizeFactor;

            simulationSettings.grid_resize_factor = gridResizeFactor;
        }
    }

    if  (simulationSettings.grid_size % simulationSettings.sdf_reduction != 0) {
        // ReSharper disable once CppDFAUnreachableCode
        std::cerr << "Warning: GRID_SIZE is not evenly divisible by SDF_REDUCTION_FACTOR!" << std::endl;
    }

    ImGui::SliderFloat("Spore Speed", &simulationSettings.spore_speed, 0.0f, static_cast<float>(simulationSettings.grid_size) / 2.0);
    ImGui::SliderFloat("Turn Speed", &simulationSettings.turn_speed, 0.0f, 5.0f);
    ImGui::SliderFloat("Decay Speed", &simulationSettings.decay_speed, 0.0f, 10.0f);
    ImGui::SliderFloat("Sensor Distance", &simulationSettings.sensor_distance, 0.0f, static_cast<float>(simulationSettings.grid_size) / 2.0);
    ImGui::SliderFloat("Sensor Angle", &simulationSettings.sensor_angle, 0.0f, M_PI);

    bool previousTransparentState = useTransparency; // Track the previous state
    if (ImGui::Checkbox("Use Transparency", &useTransparency)) {
        if (useTransparency != previousTransparentState) {
            initializeRenderShader(useTransparency);
        }
    }

    bool previousWrappingState = wrapGrid; // Track the previous state
    if (ImGui::Checkbox("Wrap Grid", &wrapGrid)) {
        if (wrapGrid != previousWrappingState) {
            initializeMoveSporesShader(wrapGrid);
        }
    }

    if (ImGui::Button("Randomize Spores")) {
        resetSporesAndGrid(); // Call the function when the button is pressed
    }

    ImGui::End(); // End the window
}
