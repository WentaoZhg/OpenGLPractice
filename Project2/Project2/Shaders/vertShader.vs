#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform mat4 mvp;
uniform mat4 view;

out vec3 frag_normal;
out vec3 fragPos;

void main()
{
    // Scale the vertex position and compute its view-space position.
    vec4 posScaled = vec4(0.05 * pos, 1.0);
    vec4 posView = view * posScaled;
    fragPos = posView.xyz;
    
    // Compute the clip-space position.
    gl_Position = mvp * posScaled;

    // Compute the normal transformation matrix using the view matrix.
    mat3 normalMatrix = transpose(inverse(mat3(view)));
    frag_normal = normalize(normalMatrix * normal);
}