#include "MoldLabGame.h"
#include <iostream>
#include <linmath.h>

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

MoldLabGame::MoldLabGame(int width, int height, const std::string& title)
    : GameEngine(width, height, title), triangle_vbo(0), triangle_vao(0), shaderProgram(0), mvp_uniform_location(0) {}

MoldLabGame::~MoldLabGame() {
    std::cout << "Exiting..." << std::endl;
}


void MoldLabGame::start() {
    glGenBuffers(1, &triangle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangle_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

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
    glVertexAttribPointer(position_attrib_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, position));

    // Configure color attribute
    glEnableVertexAttribArray(color_attrib_location);
    glVertexAttribPointer(color_attrib_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, color));
}

void MoldLabGame::update(float deltaTime) {

}

void MoldLabGame::render() {
    // Handle window size and aspect ratio
    auto [window_width, window_height] = getScreenSize();
    const float aspect_ratio = window_width / (float) window_height;


    glViewport(0, 0, window_width, window_height); // Update the viewport
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen

    // Model-View-Projection (MVP) matrix transformations
    mat4x4 model_matrix, projection_matrix, mvp_matrix;
    mat4x4_identity(model_matrix);
    mat4x4_rotate_Z(model_matrix, model_matrix, (float) glfwGetTime()); // Rotate over time
    mat4x4_ortho(projection_matrix, -aspect_ratio, aspect_ratio, -1.f, 1.f, 1.f, -1.f); // Orthographic projection
    mat4x4_mul(mvp_matrix, projection_matrix, model_matrix); // Combine matrices

    // Use the shader program and update the MVP uniform
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(mvp_uniform_location, 1, GL_FALSE, (const GLfloat*) &mvp_matrix);

    // Bind the VAO and draw the triangle
    glBindVertexArray(triangle_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
