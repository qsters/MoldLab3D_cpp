#ifndef MOLDLABGAME_H
#define MOLDLABGAME_H

#include <linmath.h>

#include "GameEngine.h"
#include "ShaderVariable.h"
#include "SimulationData.h"


constexpr int GRID_SIZE = 12;
constexpr int SPORE_COUNT = 1;
constexpr float SPORE_SPEED = 10;
constexpr float SPORE_DECAY = 0.33;
constexpr float SPORE_SENSOR_DISTANCE = 10.0;
constexpr float SPORE_TURN_SPEED = 5.0;

constexpr int SDF_REDUCTION_FACTOR = 2;

constexpr float ROTATION_SPEED =  35.0f;
constexpr int WORK_GROUP_SIZE = 8;

struct InputState {
    bool isDPressed = false;
    bool isAPressed = false;
    bool isLeftPressed = false;
    bool isRightPressed = false;
    bool isUpPressed = false;
    bool isDownPressed = false;
};


class MoldLabGame : public GameEngine {
public:
    MoldLabGame(int width, int height, const std::string& title);
    ~MoldLabGame() override;

protected:
    // Core functions
    void renderingStart() override;
    void start() override;
    void update(float deltaTime) override;
    void render() override;
    void renderUI() override;

private:
    GLuint triangleVbo = 0, triangleVao = 0, voxelGridBuffer = 0, simulationSettingsBuffer = 0, sporesBuffer = 0, sdfTexBuffer1 = 0, sdfTexBuffer2 = 0;
    GLuint shaderProgram = 0, drawSporesShaderProgram = 0, moveSporesShaderProgram = 0, decaySporesShaderProgram = 0, jumpFloodInitShaderProgram = 0, jumpFloodStepShaderProgram = 0;
    ShaderVariable<vec3> cameraPositionSV, focusPointSV;
    ShaderVariable<int> gridSizeSV, jfaStepSV;
    ShaderVariable<float> moveDeltaTimeSV, decayDeltaTimeSV;

    // Dynamically allocated to reduce Stack Memory, was causing issues at large values
    Spore* spores = nullptr;

    SimulationSettings simulationSettings;

    float horizontalAngle = 90.0f; // Rotation angle around the Y-axis
    float verticalAngle = 0.0f;   // Rotation angle around the X-axis
    float orbitRadius = 10.0f;    // Distance from the origin
    float rotationSpeed = 1.0f;   // Speed of rotation

    InputState inputState;

    // Initialization Functions
    void initializeShaders();
    void initializeUniformVariables();
    void initializeVertexBuffers();
    void initializeVoxelGridBuffer();
    void initializeSDFBuffer();
    void initializeSimulationBuffers();

    // Update Helpers
    void HandleCameraMovement(float orbitRadius, float deltaTime);
    void DispatchComputeShaders();

};

#endif // MOLDLABGAME_H
