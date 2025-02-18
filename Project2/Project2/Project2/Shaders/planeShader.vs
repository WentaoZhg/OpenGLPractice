#version 330 core

// Vertex attributes.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 inputTexCoord;

// Uniform transformation matrix.
uniform mat4 mvp;

// Output texture coordinate to the fragment shader.
out vec2 TexCoord;

void main()
{
    gl_Position = mvp * vec4(pos, 1.0);
    TexCoord = inputTexCoord;
}
