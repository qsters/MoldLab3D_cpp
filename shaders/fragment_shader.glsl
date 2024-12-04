#version 330 core

in vec3 v_Color;          // Input color from vertex shader
out vec4 frag_Color;      // Output color to the framebuffer

void main()
{
    frag_Color = vec4(v_Color, 1.0); // Set the final fragment color
}
