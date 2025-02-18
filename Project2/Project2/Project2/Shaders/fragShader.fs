#version 330 core

in vec3 frag_normal;
in vec3 fragPos;
in vec2 fragTexCoord;
out vec4 fragColor;

uniform sampler2D diffuseTexture;

void main()
{
    // Hard-coded light position in view space.
    vec3 lightPos = vec3(0.0, 5.0, 5.0);
    
    // Material lighting properties.
    vec3 ambientColor  = vec3(0.2, 0.2, 0.2);
    vec3 diffuseColor  = vec3(0.5, 0.5, 0.5);
    vec3 specularColor = vec3(1.0, 1.0, 1.0);
    float shininess    = 32.0;
    
    vec3 norm = normalize(frag_normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir = normalize(-fragPos); // camera at origin in view space.
    vec3 halfDir = normalize(lightDir + viewDir);
    

    // Sample the diffuse texture.
    vec4 texColor = texture(diffuseTexture, fragTexCoord);

    vec3 ambient = ambientColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseColor * diff;
    float spec = (diff > 0.0) ? pow(max(dot(norm, halfDir), 0.0), shininess) : 0.0;
    vec3 specular = specularColor * spec;
    
    // Combine the lighting components with the texture.
    vec3 finalColor = (ambient + diffuse) * texColor.rgb + specular;
    fragColor = vec4(finalColor, 1.0);
}
