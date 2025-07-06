#version 460

layout(location = 0) out vec3 outColor;
#ifdef VULKAN
#define gl_VertexID gl_VertexIndex
#endif
#ifdef VULKAN
// Vulkan can push constants for viewProj
layout(push_constant) uniform constants
{
    mat4 uViewProj;
} PushConstants;
#else
// OpenGL uses uniform
layout(location = 0) uniform mat4 uViewProj;
#endif

void main() 
{
    const vec3 positions[3] = vec3[3](
        vec3(1.0, 1.0, 0.0),
        vec3(-1.0, 1.0, 0.0),
        vec3(0.0, -1.0, 0.0)
    );

    const vec3 colors[3] = vec3[3](
        vec3(1.0, 0.0, 0.0), // red
        vec3(0.0, 1.0, 0.0), // green
        vec3(0.0, 0.0, 1.0)  // blue
    );

//WHACKY
#ifdef VULKAN
    mat4 viewProj = mat4(1.0);
	gl_Position = vec4(positions[gl_VertexID], 1.0);
#else
    mat4 viewProj = uViewProj;
    gl_Position = viewProj * vec4(positions[gl_VertexID], 1.0);

#endif

    outColor = colors[gl_VertexID];
}
