#ifndef MOLDLABGAME_H
#define MOLDLABGAME_H

#include <linmath.h>

#include "GameEngine.h"

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
    GLuint triangleVbo, triangleVao;
    GLuint shaderProgram;
    GLuint screenSizeLocation, cameraPositionLocation;

    vec3 cameraPosition;

    static constexpr int GRID_SIZE = 1;

    float voxelGrid[GRID_SIZE][GRID_SIZE][GRID_SIZE];

};

#endif // MOLDLABGAME_H
