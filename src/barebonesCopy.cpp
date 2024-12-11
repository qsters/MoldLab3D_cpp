#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

// Structure to represent a single vertex with position and color
typedef struct Vertex
{
    vec2 position;  // 2D position (x, y)
    vec3 color;     // RGB color (r, g, b)
} Vertex;

// Array of vertices defining a triangle
static const Vertex triangle_vertices[3] =
{
    { { -0.6f, -0.4f }, { 1.f, 0.f, 0.f } }, // Bottom-left, Red
    { {  0.6f, -0.4f }, { 0.f, 1.f, 0.f } }, // Bottom-right, Green
    { {   0.f,  0.6f }, { 0.f, 0.f, 1.f } }  // Top-center, Blue
};

// Function to load shader source code from a file
string load_shader_source(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Error: Could not open shader file: " << filepath << endl;
        exit(EXIT_FAILURE);
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to load and split shader source code from a combined file
std::pair<std::string, std::string> load_combined_shader_source(const std::string& filepath) {
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


// Function to compile a shader
GLuint compile_shader(const string& source, GLenum shader_type) {
    GLuint shader = glCreateShader(shader_type);
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        cerr << "Shader Compilation Error: " << infoLog << endl;
        exit(EXIT_FAILURE);
    }

    return shader;
}

// Error callback to print GLFW errors
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

// Key callback for handling key presses
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
        glfwSetWindowShouldClose(window, GLFW_TRUE); // Close window on Escape key
}

int main(void)
{
    glfwSetErrorCallback(error_callback);

    // Initialize GLFW
    if (!glfwInit())
        exit(EXIT_FAILURE);

    // Specify OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window and OpenGL context
    GLFWwindow* window = glfwCreateWindow(640, 480, "OpenGL Triangle", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSwapInterval(1); // Enable V-Sync

    // Main rendering loop
    while (!glfwWindowShouldClose(window))
    {
        cout << "Rendering!!" << endl;
        glfwSwapBuffers(window); // Display the frame
        glfwPollEvents();        // Handle user input
    }

    // Clean up resources
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
