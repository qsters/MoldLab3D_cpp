#type vertex
#version 330 core

uniform mat4 u_MVP;       // Model-View-Projection matrix
in vec3 a_Color;          // Input color attribute
in vec2 a_Position;       // Input position attribute
out vec3 v_Color;         // Output color passed to fragment shader

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 0.0, 1.0); // Transform position
    v_Color = a_Color;                                // Pass color to fragment shader
}

#type fragment
#version 330 core

in vec3 v_Color;          // Input color from vertex shader
out vec4 frag_Color;      // Output color to the framebuffer

void main()
{
    frag_Color = vec4(v_Color, 1.0); // Set the final fragment color
}
