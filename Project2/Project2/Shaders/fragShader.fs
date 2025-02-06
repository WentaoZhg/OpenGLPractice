#version 330 core

in vec3 frag_normal;
in vec3 fragPos;
out vec4 fragColor;

void main()
{
    // Hard-coded light position in view space.
    // (Since the camera is at the origin in view space, this position is relative to it.)
    vec3 lightPos = vec3(0.0, 5.0, 5.0);

    // Material properties.
    vec3 ambientColor  = vec3(0.2, 0.2, 0.2);
    vec3 diffuseColor  = vec3(0.5, 0.5, 0.5);
    vec3 specularColor = vec3(1.0, 1.0, 1.0);
    float shininess    = 32.0;

    // Compute normalized surface normal.
    vec3 norm = normalize(frag_normal);

    // Compute light direction (from fragment to light).
    vec3 lightDir = normalize(lightPos - fragPos);

    // Compute view direction (from fragment to camera, which is at the origin in view space).
    vec3 viewDir = normalize(-fragPos);

    // Compute the half-vector between the light and the view direction.
    vec3 halfDir = normalize(lightDir + viewDir);

    // Ambient component.
    vec3 ambient = ambientColor;

    // Diffuse component.
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseColor * diff;

    // Specular component.
    float spec = 0.0;
    if(diff > 0.0)
        spec = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3 specular = specularColor * spec;

    // Combine all three components.
    vec3 finalColor = ambient + diffuse + specular;

    fragColor = vec4(finalColor, 1.0);
}
