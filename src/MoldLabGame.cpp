#include "MoldLabGame.h"
#include <iostream>
#include <linmath.h>
#include <cmath>

typedef struct Vertex
{
    vec3 position;  // 2D position (x, y)
    vec3 color;     // RGB color (r, g, b)
} Vertex;


// Array of vertices defining a cube (12 triangles, 36 vertices)
static const Vertex vertices[36] =
{
    // Front face
    { { -0.5f, -0.5f,  0.5f }, { 1.f, 0.f, 0.f } }, // Bottom-left
    { {  0.5f, -0.5f,  0.5f }, { 0.f, 1.f, 0.f } }, // Bottom-right
    { {  0.5f,  0.5f,  0.5f }, { 0.f, 0.f, 1.f } }, // Top-right

    { { -0.5f, -0.5f,  0.5f }, { 1.f, 0.f, 0.f } }, // Bottom-left
    { {  0.5f,  0.5f,  0.5f }, { 0.f, 0.f, 1.f } }, // Top-right
    { { -0.5f,  0.5f,  0.5f }, { 1.f, 1.f, 0.f } }, // Top-left

    // Back face
    { { -0.5f, -0.5f, -0.5f }, { 1.f, 0.f, 1.f } }, // Bottom-left
    { {  0.5f, -0.5f, -0.5f }, { 0.f, 1.f, 1.f } }, // Bottom-right
    { {  0.5f,  0.5f, -0.5f }, { 1.f, 1.f, 1.f } }, // Top-right

    { { -0.5f, -0.5f, -0.5f }, { 1.f, 0.f, 1.f } }, // Bottom-left
    { {  0.5f,  0.5f, -0.5f }, { 1.f, 1.f, 1.f } }, // Top-right
    { { -0.5f,  0.5f, -0.5f }, { 0.f, 0.f, 1.f } }, // Top-left

    // Left face
    { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f, 0.f } }, // Bottom-left
    { { -0.5f, -0.5f,  0.5f }, { 1.f, 0.f, 0.f } }, // Bottom-right
    { { -0.5f,  0.5f,  0.5f }, { 1.f, 1.f, 0.f } }, // Top-right

    { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f, 0.f } }, // Bottom-left
    { { -0.5f,  0.5f,  0.5f }, { 1.f, 1.f, 0.f } }, // Top-right
    { { -0.5f,  0.5f, -0.5f }, { 1.f, 0.f, 1.f } }, // Top-left

    // Right face
    { {  0.5f, -0.5f, -0.5f }, { 0.f, 0.f, 1.f } }, // Bottom-left
    { {  0.5f, -0.5f,  0.5f }, { 0.f, 1.f, 0.f } }, // Bottom-right
    { {  0.5f,  0.5f,  0.5f }, { 1.f, 0.f, 0.f } }, // Top-right

    { {  0.5f, -0.5f, -0.5f }, { 0.f, 0.f, 1.f } }, // Bottom-left
    { {  0.5f,  0.5f,  0.5f }, { 1.f, 0.f, 0.f } }, // Top-right
    { {  0.5f,  0.5f, -0.5f }, { 1.f, 1.f, 1.f } }, // Top-left

    // Top face
    { { -0.5f,  0.5f, -0.5f }, { 0.f, 0.f, 1.f } }, // Bottom-left
    { {  0.5f,  0.5f, -0.5f }, { 1.f, 1.f, 0.f } }, // Bottom-right
    { {  0.5f,  0.5f,  0.5f }, { 1.f, 0.f, 0.f } }, // Top-right

    { { -0.5f,  0.5f, -0.5f }, { 0.f, 0.f, 1.f } }, // Bottom-left
    { {  0.5f,  0.5f,  0.5f }, { 1.f, 0.f, 0.f } }, // Top-right
    { { -0.5f,  0.5f,  0.5f }, { 1.f, 1.f, 0.f } }, // Top-left

    // Bottom face
    { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f, 1.f } }, // Bottom-left
    { {  0.5f, -0.5f, -0.5f }, { 0.f, 0.f, 1.f } }, // Bottom-right
    { {  0.5f, -0.5f,  0.5f }, { 0.f, 1.f, 0.f } }, // Top-right

    { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f, 1.f } }, // Bottom-left
    { {  0.5f, -0.5f,  0.5f }, { 0.f, 1.f, 0.f } }, // Top-right
    { { -0.5f, -0.5f,  0.5f }, { 1.f, 0.f, 0.f } }  // Top-left
};

MoldLabGame::MoldLabGame(int width, int height, const std::string& title)
    : GameEngine(width, height, title), triangle_vbo(0), triangle_vao(0), shaderProgram(0), mvp_uniform_location(0) {}

MoldLabGame::~MoldLabGame() {
    if (triangle_vbo) glDeleteBuffers(1, &triangle_vbo);
    if (triangle_vao) glDeleteVertexArrays(1, &triangle_vao);

    std::cout << "Exiting..." << std::endl;
}


void MoldLabGame::start() {
    glGenBuffers(1, &triangle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangle_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Load and compile shaders
    auto [vertex_shader_code, fragment_shader_code] = LoadCombinedShaderSource("shaders/simple_triangle.glsl");

    GLuint vertex_shader = CompileShader(vertex_shader_code, GL_VERTEX_SHADER);
    GLuint fragment_shader = CompileShader(fragment_shader_code, GL_FRAGMENT_SHADER);

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex_shader);
    glAttachShader(shaderProgram, fragment_shader);
    glLinkProgram(shaderProgram);


    // Get locations of shader attributes and uniforms
    mvp_uniform_location = glGetUniformLocation(shaderProgram, "u_MVP");
    const GLint position_attrib_location = glGetAttribLocation(shaderProgram, "a_Position");
    const GLint color_attrib_location = glGetAttribLocation(shaderProgram, "a_Color");

    // configure the Vertex Array Object (VAO)
    glGenVertexArrays(1, &triangle_vao);
    glBindVertexArray(triangle_vao);

    // Configure position attribute
    glEnableVertexAttribArray(position_attrib_location);
    glVertexAttribPointer(position_attrib_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

    // Configure color attribute
    glEnableVertexAttribArray(color_attrib_location);
    glVertexAttribPointer(color_attrib_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));
}

void MoldLabGame::update(float deltaTime) {

}

void MoldLabGame::render() {
    // Handle window size and aspect ratio
    auto [window_width, window_height] = getScreenSize();
    const float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);

    glViewport(0, 0, window_width, window_height); // Update the viewport
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen and depth buffer

    // Enable depth testing for proper 3D rendering
    glEnable(GL_DEPTH_TEST);

    // Model-View-Projection (MVP) matrix transformations
    mat4x4 model_matrix, view_matrix, projection_matrix, mvp_matrix;

    // Model transformation: no rotation, identity matrix
    mat4x4_identity(model_matrix);

    float oscillator = sin(TimeSinceStart());
    float oscillator2 = cos(TimeSinceStart());

    // View transformation: camera looking at the origin
    vec3 eye = {oscillator, oscillator2, 2.0f};  // Camera position
    vec3 center = {0.0f, 0.0f, 0.0f}; // Target position
    vec3 up = {0.0f, 1.0f, 0.0f};  // Up direction
    mat4x4_look_at(view_matrix, eye, center, up);

    // Perspective projection
    mat4x4_perspective(projection_matrix, 45.0f * (3.14159f / 180.0f), aspect_ratio, 0.1f, 100.0f);

    // Combine matrices: MVP = Projection * View * Model
    mat4x4_mul(mvp_matrix, view_matrix, model_matrix);
    mat4x4_mul(mvp_matrix, projection_matrix, mvp_matrix);

    // Use the shader program and update the MVP uniform
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(static_cast<GLint>(mvp_uniform_location), 1, GL_FALSE, reinterpret_cast<const GLfloat *>(mvp_matrix));

    // Bind the VAO and draw the triangle
    glBindVertexArray(triangle_vao);
    glDrawArrays(GL_TRIANGLES, 0,  sizeof(vertices) / sizeof(Vertex));
}
