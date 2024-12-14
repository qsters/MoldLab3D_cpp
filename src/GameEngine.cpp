#include "GameEngine.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility>


GameEngine::GameEngine(const int width, const int height, std::string  title)
    : window(nullptr), width(width), height(height), title(std::move(title)), lastFrameTime(0.0f), deltaTime(0.0f), timeSinceStart(0.0f) {

    init();
}

void GameEngine::init() {
    initGLFW();
}

GameEngine::~GameEngine() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void GameEngine::run() {
    start();
    renderingStart();

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentTime = static_cast<float>(glfwGetTime());
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        timeSinceStart += deltaTime;

        update(deltaTime);
        render();


        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

std::pair<int, int> GameEngine::getScreenSize() const {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    return {width, height};
}

int GameEngine::getScreenHeight() const {
    auto [width, height] = getScreenSize();
    return height;
}

int GameEngine::getScreenWidth() const {
    auto [width, height] = getScreenSize();
    return width;
}

std::string GameEngine::LoadShaderSource(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open shader file: " << filepath << std::endl;
        exit(EXIT_FAILURE);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::pair<std::string, std::string> GameEngine::LoadCombinedShaderSource(const std::string &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open shader file: " << filepath << std::endl;
        exit(EXIT_FAILURE);
    }

    std::stringstream vertex_shader_stream;
    std::stringstream fragment_shader_stream;

    std::string line;
    std::stringstream* current_stream = nullptr;

    while (std::getline(file, line)) {
        if (line.find("#type vertex") != std::string::npos) {
            current_stream = &vertex_shader_stream;
        } else if (line.find("#type fragment") != std::string::npos) {
            current_stream = &fragment_shader_stream;
        } else if (current_stream) {
            *current_stream << line << '\n';
        }
    }

    return { vertex_shader_stream.str(), fragment_shader_stream.str() };
}

GLuint GameEngine::CompileShader(const std::string& source, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
        exit(EXIT_FAILURE);
    }

    return shader;
}

void GameEngine::CheckShaderCompilation(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}

void GameEngine::CheckProgramLinking(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
}


void GameEngine::initGLFW() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, GameEngine::keyCallback);

    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1); // Enable V-Sync
}

void GameEngine::errorCallback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

void GameEngine::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
        glfwSetWindowShouldClose(window, GLFW_TRUE); // Close window on Escape key
    // TODO Finish implementing an actual input manager
}

void GameEngine::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Helper function for catching errors.
void GameEngine::CheckGLError(const std::string& context) {
    GLenum error = glGetError();
    while (error != GL_NO_ERROR) {
        std::string errorString;
        switch (error) {
            case GL_INVALID_ENUM: errorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW: errorString = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW: errorString = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY: errorString = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default: errorString = "UNKNOWN_ERROR"; break;
        }
        std::cerr << "OpenGL Error in " << context << ": " << errorString << " (" << error << ")" << std::endl;
        error = glGetError(); // Check for more errors
    }
}

float GameEngine::DeltaTime() const {
    return deltaTime;
}

float GameEngine::TimeSinceStart() const {
    return timeSinceStart;
}
