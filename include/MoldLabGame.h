#ifndef MOLDLABGAME_H
#define MOLDLABGAME_H

#include <linmath.h>

#include "GameEngine.h"
#include "ShaderVariable.h"

constexpr int GRID_SIZE = 8;

class MoldLabGame : public GameEngine {
public:
    MoldLabGame(int width, int height, const std::string& title);
    ~MoldLabGame() override;

protected:
    void renderingStart() override;
    void start() override;
    void update(float deltaTime) override;
    void render() override;

private:
    GLuint triangleVbo = 0, triangleVao = 0, voxelGridBuffer = 0;
    GLuint shaderProgram = 0, growSporesShaderProgram = 0;
    ShaderVariable<vec3> cameraPositionSV, focusPointSV;
    ShaderVariable<int> gridSizeSV;
    ShaderVariable<float> testValueSV;

    float voxelGrid[GRID_SIZE][GRID_SIZE][GRID_SIZE];

    float horizontalAngle = 90.0f; // Rotation angle around the Y-axis
    float verticalAngle = 0.0f;   // Rotation angle around the X-axis
    float orbitRadius = 10.0f;    // Distance from the origin
    float rotationSpeed = 1.0f;   // Speed of rotation
};

#endif // MOLDLABGAME_H
