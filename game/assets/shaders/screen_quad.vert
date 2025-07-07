#version 460 core

#ifndef VULKAN

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

layout (location = 0) out vec2 TexCoords;


layout(location = 0) uniform mat4 uViewProj;

void main()
{
    TexCoords = aTexCoords;
   gl_Position = uViewProj* vec4(aPos, 1.0);


}
#else
void main()
{

}
#endif