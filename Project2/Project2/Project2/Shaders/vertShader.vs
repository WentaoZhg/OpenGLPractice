#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 mvp;
uniform mat4 view;  // Camera (view) transformation

out vec3 frag_normal;
out vec3 fragPos;      // Vertex position in view space.
out vec2 fragTexCoord; // Texture coordinate.

void main()
{
    vec4 posScaled = vec4(0.05 * pos, 1.0);
    vec4 posView = view * posScaled;
    fragPos = posView.xyz;
    fragTexCoord = texCoord;
    
    gl_Position = mvp * posScaled;
    
    // Transform normals using the inverse transpose of the view matrix.
    mat3 normalMatrix = transpose(inverse(mat3(view)));
    frag_normal = normalize(normalMatrix * normal);
}
