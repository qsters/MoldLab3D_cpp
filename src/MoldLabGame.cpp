#include "MoldLabGame.h"
#include <iostream>
#include <linmath.h>
#include <cmath>

static bool isDPressed = false;
static bool isAPressed = false;


typedef struct Vertex {
    vec3 position; // 3D position of the vertex
} Vertex;

// Array of vertices defining a full-screen quad
static const Vertex quadVertices[6] = {
    {{-1.0f, -1.0f, 0.0f}}, // Bottom-left
    {{ 1.0f, -1.0f, 0.0f}}, // Bottom-right
    {{ 1.0f,  1.0f, 0.0f}}, // Top-right

    {{-1.0f, -1.0f, 0.0f}}, // Bottom-left
    {{ 1.0f,  1.0f, 0.0f}}, // Top-right
    {{-1.0f,  1.0f, 0.0f}}  // Top-left
};

void set_vec3(vec3 v, float x, float y, float z) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

MoldLabGame::MoldLabGame(int width, int height, const std::string& title)
: GameEngine(width, height, title), voxelGrid{} {
    displayFramerate = true;
}

MoldLabGame::~MoldLabGame() {
    if (triangleVbo) glDeleteBuffers(1, &triangleVbo);
    if (triangleVao) glDeleteVertexArrays(1, &triangleVao);
    if (voxelGridBuffer) glDeleteBuffers(1, &voxelGridBuffer);
    std::cout << "Exiting..." << std::endl;
}

void MoldLabGame::renderingStart() {
    glGenBuffers(1, &triangleVbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Load and compile shaders
    auto [vertexShaderCode, fragmentShaderCode] = LoadCombinedShaderSource("shaders/quad_renderer.glsl");

    GLuint vertexShader = CompileShader(vertexShaderCode, GL_VERTEX_SHADER);
    CheckShaderCompilation(vertexShader);

    GLuint fragmentShader = CompileShader(fragmentShaderCode, GL_FRAGMENT_SHADER);
    CheckShaderCompilation(fragmentShader);

    // Link shaders into a program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    CheckProgramLinking(shaderProgram);

    static vec3 cameraPosition = {0.0f, 0.0f, 0.0f};
    static vec3 focusPoint = {0.0f, 0.0f, 0.0f};
    static int gridSize = GRID_SIZE;
    static float testValue = 1.0f;

    cameraPositionSV = ShaderVariable(shaderProgram, &cameraPosition, "cameraPosition");
    focusPointSV = ShaderVariable(shaderProgram, &focusPoint, "focusPoint");
    gridSizeSV = ShaderVariable(shaderProgram, &gridSize, "gridSize");
    testValueSV = ShaderVariable(shaderProgram, &testValue, "testValue");


    GLint positionAttributeLocation = glGetAttribLocation(shaderProgram, "position");


    glGenVertexArrays(1, &triangleVao);
    glBindVertexArray(triangleVao);

    glGenBuffers(1, &triangleVbo);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Enable and set the position attribute
    glEnableVertexAttribArray(positionAttributeLocation);
    glVertexAttribPointer(positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

    glBindVertexArray(0); // Unbind VAO

    // **Initialize the voxel grid with 1's to stress test
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int z = 0; z < GRID_SIZE; z++) {
                float random_number = std::rand() / (RAND_MAX + 1.0);
                voxelGrid[x][y][z] = random_number;
            }
        }
    }

    // ** Create Voxel Grid Buffer **
    glGenBuffers(1, &voxelGridBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelGridBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(voxelGrid), voxelGrid, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelGridBuffer); // Binding index 0
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind buffer
}


void MoldLabGame::start() {
    inputManager.bindAction(GLFW_KEY_D, InputEventType::KeyPressed, [this]() {
        std::cout << "D key pressed" << std::endl;
        isDPressed = true;
    });

    inputManager.bindAction(GLFW_KEY_D, InputEventType::KeyReleased, [this]() {
        std::cout << "D key released" << std::endl;
        isDPressed = false;
    });

    inputManager.bindAction(GLFW_KEY_A, InputEventType::KeyPressed, [this]() {
        std::cout << "A key pressed" << std::endl;
        isAPressed = true;
    });

    inputManager.bindAction(GLFW_KEY_A, InputEventType::KeyReleased, [this]() {
        std::cout << "A key released" << std::endl;
        isAPressed = false;
    });
}

void MoldLabGame::update(float deltaTime) {
    static const float ROTATION_SPEED = 0.5f; // Radians per second
    static float angle = 0.0f; // Current angle of rotation

    // Update the angle
    angle += ROTATION_SPEED * deltaTime;

    vec3& focusPoint = *focusPointSV.value;

    float gridCenter = (GRID_SIZE - 1.0f) * 0.5f; // Adjust for the centered cube positions
    set_vec3(focusPoint, gridCenter, gridCenter, gridCenter);


    float orbitRadius = GRID_SIZE * 2.0f; // Adjust this as needed for a suitable orbit radius

    // Orbit speed (radians per second)
    float orbitSpeed = 1.0f;

    // Update the angle based on delta time
    static float orbitAngle = 0.0f;
    orbitAngle += orbitSpeed * deltaTime;

    // Wrap the angle to avoid overflow
    if (orbitAngle > 2.0f * M_PI) {
        orbitAngle -= 2.0f * M_PI;
    }

    // Compute new camera position
    float cameraX = gridCenter + orbitRadius * cos(orbitAngle); // Circular orbit in the XZ plane
    float cameraZ = gridCenter + orbitRadius * sin(orbitAngle);

    // Set the camera position, keeping the y-value fixed
    set_vec3(*cameraPositionSV.value, cameraX, gridCenter, cameraZ);

    float testScaler = 1.5f;
    // Adjust testValueSV based on key state
    float& testValue = *testValueSV.value; // Assume `testValueSV` is already initialized

    if (isDPressed) {
        testValue += deltaTime * testScaler; // Change rate is 1.0 units per second
        std::cout << "Test value: " << testValue << std::endl;
    }
    if (isAPressed) {
        testValue -= deltaTime * testScaler; // Same rate but in the opposite direction
        std::cout << "Test value: " << testValue << std::endl;
    }

    // // Compute new camera position in a circular path around the origin
    // float radius = 10.0f; // Distance from the origin
    // set_vec3(cameraPosition, radius * cos(angle), 2.0f, radius * sin(angle));
    // // Optional: Reset angle to prevent overflow
    // if (angle > 2.0f * M_PI) {
    //     angle -= 2.0f * M_PI;
    // }
}

void MoldLabGame::render() {
    // Handle window size and aspect ratio
    auto [windowWidth, windowHeight] = getScreenSize();

    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen and depth buffer

    glUseProgram(shaderProgram);

    gridSizeSV.uploadToShader();
    cameraPositionSV.uploadToShader();
    focusPointSV.uploadToShader();
    testValueSV.uploadToShader(true);
    
    // Draw the full-screen quad
    glBindVertexArray(triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
