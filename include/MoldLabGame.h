#ifndef MOLDLABGAME_H
#define MOLDLABGAME_H

#include "GameEngine.h"

class MoldLabGame : public GameEngine {
public:
    MoldLabGame(int width, int height, const std::string& title);
    ~MoldLabGame() override;

protected:
    void start() override;
    void update(float deltaTime) override;
    void render() override;

private:
    GLuint triangle_vbo, triangle_vao;
    GLuint shaderProgram;
    GLuint mvp_uniform_location;
};

#endif // MOLDLABGAME_H
