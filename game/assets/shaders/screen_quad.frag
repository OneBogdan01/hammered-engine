#version 460 core
layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoords;

layout (binding = 0) uniform sampler2D tex;

void main()
{             
    vec3 texCol = texture(tex, TexCoords).rgb;      
    FragColor = vec4(texCol, 1.0);
}