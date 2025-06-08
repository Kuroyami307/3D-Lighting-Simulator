#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 inpNorm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 normal;
out vec3 vertexPosition;

void main()
{
    gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    
    // Transform the normal using the inverse transpose of model matrix
    //vec4 temp = normalize(model * vec4(inpNorm, 1.0f));
    //normal = vec3(temp.x, temp.y, temp.z);
    normal = inpNorm;
    vertexPosition = aPos;
}