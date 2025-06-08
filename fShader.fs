#version 330 core
in vec3 normal;
in vec3 vertexPosition;

out vec4 FragColor;

uniform vec3 ourColor;
uniform float lightIntensity;
uniform mat4 model;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

vec3 lightDirection;
vec3 viewDirection;
vec3 reflectedDirection;

void main()
{
    vec4 temp = vec4(normalize(lightPosition - vertexPosition), 0.0f);
    lightDirection = vec3(temp.x, temp.y, temp.z);

    temp = vec4(normalize(cameraPosition - vertexPosition), 0.0f);
    viewDirection = vec3(temp.x, temp.y, temp.z);

    reflectedDirection = normalize(reflect(-lightDirection, normal));

    //reflectedDirection = normalize(2 * dot(normal, lightDirection) * normal - lightDirection);

    float finalIntensity = 0.2 * lightIntensity;

    float dot_NL = dot(normal, lightDirection);
    float dot_RV = dot(reflectedDirection, viewDirection);

    if(dot_NL > 0)
    {
        finalIntensity += 0.8 * lightIntensity * dot_NL;
        if(dot_RV > 0)
            finalIntensity += 0.6 * lightIntensity * pow(dot_RV, 8);
    }

    FragColor = vec4(ourColor * finalIntensity, 1.0f);
    //FragColor = vec4(normal * 0.5 + 0.5, 1.0); // Should show gradient coloring
}