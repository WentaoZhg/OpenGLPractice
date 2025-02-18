#version 330 core

in vec2 TexCoord;
out vec4 fragColor;

uniform sampler2D renderedTexture;
uniform vec3 colorOffset;

void main()
{
    vec4 texColor = texture(renderedTexture, TexCoord);
    fragColor = texColor + vec4(colorOffset, 0.0);
}
