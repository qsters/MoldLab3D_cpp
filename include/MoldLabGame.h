#ifndef MOLDLABGAME_H
#define MOLDLABGAME_H

#include "GameEngine.h"
#include "ShaderVariable.h"
#include "SimulationData.h"
#include "Spore.h"

constexpr int GRID_SIZE = 500;
constexpr int SPORE_COUNT = 1000000;
constexpr float SPORE_SPEED = 10;
constexpr float SPORE_DECAY = 0.33;
constexpr float SPORE_SENSOR_DISTANCE = 10.0;
constexpr float SPORE_SENSOR_ANGLE = M_PI / 2.0;
constexpr float SPORE_TURN_SPEED = 1.0;
constexpr float SPORE_ROTATION_SPEED = 1.0;

constexpr int SDF_REDUCTION_FACTOR = 2;

constexpr float ROTATION_SPEED =  100.0f;

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

    static Spore CreateRandomSpore();

protected:
    // Core functions
    void renderingStart() override;
    void start() override;
    void update(float deltaTime) override;
    void render() override;
    void renderUI() override;

private:
    GLuint triangleVbo = 0, triangleVao = 0, voxelGridTexture = 0, simulationSettingsBuffer = 0, sporesBuffer = 0, sdfTexBuffer1 = 0, sdfTexBuffer2 = 0;
    GLuint shaderProgram = 0, drawSporesShaderProgram = 0, moveSporesShaderProgram = 0, decaySporesShaderProgram = 0, jumpFloodInitShaderProgram = 0, jumpFloodStepShaderProgram = 0, clearGridShaderProgram = 0;
    ShaderVariable<int> jfaStepSV;

    // Dynamically allocated to reduce Stack Memory, was causing issues at large values
    Spore* spores = nullptr;

    SimulationData simulationSettings{};

    float horizontalAngle = 90.0f; // Rotation angle around the Y-axis
    float verticalAngle = 0.0f;   // Rotation angle around the X-axis
    float orbitRadius = GRID_SIZE * 1.25;    // Distance from the origin
    float rotationSpeed = 1.0f;   // Speed of rotation

    bool useTransparency = false;

    InputState inputState;

    // Initialization Functions
    void initializeRenderShader(bool useTransparency);
    void initializeShaders();
    void initializeUniformVariables();
    void initializeVertexBuffers();
    void initializeVoxelGridBuffer();
    void initializeSDFBuffer();
    void initializeSimulationBuffers();
    void initializeSpores() const;

    // Update Helpers
    void HandleCameraMovement(float orbitRadius, float deltaTime);
    void DispatchComputeShaders() const;
    void executeJFA() const;

};

#endif // MOLDLABGAME_H
