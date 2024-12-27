#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>
#include "InputManager.h"


// Abstract base class for game engines
class GameEngine {
public:
    GameEngine(int width, int height, std::string  title, bool vSync);
    virtual ~GameEngine();

    void run(); // Main game loop

    // Getters for Screen Dimensions
    [[nodiscard]] std::pair<int,int> getScreenSize() const;


    static std::string LoadShaderSource(const std::string& filepath);
    static std::pair<std::string, std::string> LoadCombinedShaderSource(const std::string& filepath);
    GLuint CompileShader(const std::string& source, GLenum shader_type);

    GLuint CompileAndAttachShader(const std::string &source, GLenum shaderType, GLuint program);

    GLuint CreateShaderProgram(const std::vector<std::tuple<std::string, GLenum, bool>>& shaders);

    static void CheckProgramLinking(GLuint program);

    // Error catcher helper function
    static void CheckGLError(const std::string& context);

    void addShaderDefinition(const std::string& placeholder, const std::string& filePath);
    void removeShaderDefinition(const std::string &placeholder);

    bool GetVsyncStatus() const;
    void SetVsyncStatus(bool status);


protected:
    // Virtual methods to be implemented by derived classes
    virtual void renderingStart() = 0;
    virtual void start() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual void renderUI() = 0;

    // Utility methods for derived classes
    [[nodiscard]] int getScreenWidth() const;
    [[nodiscard]] int getScreenHeight() const;

    [[nodiscard]] int getMaxWorkGroupCountX() const;
    [[nodiscard]] int getMaxWorkGroupCountY() const;
    [[nodiscard]] int getMaxWorkGroupCountZ() const;

    [[nodiscard]] int getMaxWorkGroupSizeX() const;
    [[nodiscard]] int getMaxWorkGroupSizeY() const;
    [[nodiscard]] int getMaxWorkGroupSizeZ() const;

    [[nodiscard]] float TimeSinceStart() const;
    [[nodiscard]] float DeltaTime() const;

    void DispatchComputeShader(GLuint computeShaderProgram, int itemsX, int itemsY, int itemsZ) const;

    bool displayFramerate = false;
    InputManager inputManager;




private:
    // Core initialization
    void init();
    void initGLFW();
    void initImGui() const;


    void printFramerate(float& frameTimeAccumulator, int& frameCount) const;
    void ComputeShaderInitializationAndCheck();

    // Window and context
    GLFWwindow* window;
    int width, height; // Private as it can change
    std::string title;

    // Timing
    float lastFrameTime;
    float deltaTime;
    float timeSinceStart;

    int maxWorkGroupCountX{}, maxWorkGroupCountY{}, maxWorkGroupCountZ{};
    int maxWorkGroupSizeX{}, maxWorkGroupSizeY{}, maxWorkGroupSizeZ{};

    bool vSyncEnabled;


    // Callback setup
    static void errorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    std::unordered_map<std::string, std::string> shaderDefinitions;

};

#endif // GAMEENGINE_H
