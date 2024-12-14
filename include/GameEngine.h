#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>

// Abstract base class for game engines
class GameEngine {
public:
    GameEngine(int width, int height, std::string  title);
    virtual ~GameEngine();

    void run(); // Main game loop

    // Getters for Screen Dimensions
    [[nodiscard]] std::pair<int,int> getScreenSize() const;


    static std::string LoadShaderSource(const std::string& filepath);
    static std::pair<std::string, std::string> LoadCombinedShaderSource(const std::string& filepath);
    static GLuint CompileShader(const std::string& source, GLenum shader_type);

    static void CheckShaderCompilation(GLuint shader);
    static void CheckProgramLinking(GLuint program);

    // Error catcher helper function
    static void CheckGLError(const std::string& context);


protected:
    // Virtual methods to be implemented by derived classes
    virtual void renderingStart() = 0;
    virtual void start() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;

    // Utility methods for derived classes
    [[nodiscard]] int getScreenWidth() const;
    [[nodiscard]] int getScreenHeight() const;

    [[nodiscard]] float TimeSinceStart() const;
    [[nodiscard]] float DeltaTime() const;



private:
    // Core initialization
    void init();
    void initGLFW();

    // Window and context
    GLFWwindow* window;
    int width, height; // Private as it can change
    std::string title;

    // Timing
    float lastFrameTime;
    float deltaTime;
    float timeSinceStart;



    // Callback setup
    static void errorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};

#endif // GAMEENGINE_H
