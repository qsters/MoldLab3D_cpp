#type vertex
#version 430 core

uniform vec2 screenSize;
in vec3 position;

out vec2 uv;


void main() {
    gl_Position = vec4(position, 1.0); // Use quad's position directly
    uv = (position.xy + vec2(1.0)) * 0.5; // Normalize from [-1, 1] to [0, 1]
}

#type fragment
#version 430 core

in vec2 uv;
out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(uv, 0.0, 1.0); // Gradient color based on UV
}
