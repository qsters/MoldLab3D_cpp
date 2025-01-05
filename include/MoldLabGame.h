#ifndef MOLDLABGAME_H
#define MOLDLABGAME_H

#include "GameEngine.h"
#include "ShaderVariable.h"
#include "SimulationData.h"
#include "Spore.h"

struct SimulationDefaults {
    static constexpr int GRID_SIZE = 500;
    static constexpr int SPORE_COUNT = 1'000'000;
    static constexpr float SPORE_SPEED = 10.0f;
    static constexpr float SPORE_DECAY = 0.33f;
    static constexpr float SPORE_SENSOR_DISTANCE = 10.0f;
    static constexpr float SPORE_SENSOR_ANGLE = M_PI / 2.0f;
    static constexpr float SPORE_TURN_SPEED = 1.0f;
    static constexpr float SPORE_ROTATION_SPEED = 1.0f;
    static constexpr int SDF_REDUCTION_FACTOR = 2;
};



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
    GLuint triangleVbo = 0, triangleVao = 0, voxelGridTexture = 0, simulationSettingsBuffer = 0, sporesBuffer = 0, sdfTexBuffer1 = 0, sdfTexBuffer2 = 0;
    GLuint shaderProgram = 0, drawSporesShaderProgram = 0, moveSporesShaderProgram = 0, decaySporesShaderProgram = 0, jumpFloodInitShaderProgram = 0, jumpFloodStepShaderProgram = 0, clearGridShaderProgram = 0, randomizeSporesShaderProgram = 0;
    ShaderVariable<int> jfaStepSV;

    SimulationData simulationSettings{};

    float horizontalAngle = 90.0f; // Rotation angle around the Y-axis
    float verticalAngle = 0.0f;   // Rotation angle around the X-axis
    float orbitRadius = SimulationDefaults::GRID_SIZE * 1.25;    // Distance from the origin
    float rotationSpeed = 1.0f;   // Speed of rotation

    bool useTransparency = true;
    bool wrapGrid = true;

    InputState inputState;

    // Initialization Functions
    void initializeRenderShader(bool useTransparency);
    void initializeMoveSporesShader(bool wrapAround);

    void initializeShaders();
    void initializeUniformVariables();
    void initializeVertexBuffers();
    void initializeVoxelGridBuffer();
    void initializeSDFBuffer();
    void initializeSimulationBuffers();

    // Update Helpers
    void HandleCameraMovement(float orbitRadius, float deltaTime);
    void DispatchComputeShaders() const;
    void executeJFA() const;
    void resetSporesAndGrid() const;
};

#endif // MOLDLABGAME_H
