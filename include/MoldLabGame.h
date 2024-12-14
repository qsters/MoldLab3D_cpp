#ifndef MOLDLABGAME_H
#define MOLDLABGAME_H

#include <linmath.h>

#include "GameEngine.h"
#include "ShaderVariable.h"

constexpr int GRID_SIZE = 15;

class MoldLabGame : public GameEngine {
public:
    MoldLabGame(int width, int height, const std::string& title);
    ~MoldLabGame() override;

protected:
    void renderingStart() override;
    void start() override;
    void update(float deltaTime) override;
    void render() override;

    void onKeyCallback(int key, int scancode, int action, int mods) override;

private:
    GLuint triangleVbo, triangleVao;
    GLuint shaderProgram;
    ShaderVariable<vec3> cameraPositionSV, focusPointSV;
    ShaderVariable<int> gridSizeSV;
    ShaderVariable<float> testValueSV;

    float voxelGrid[GRID_SIZE][GRID_SIZE][GRID_SIZE];

};

#endif // MOLDLABGAME_H
