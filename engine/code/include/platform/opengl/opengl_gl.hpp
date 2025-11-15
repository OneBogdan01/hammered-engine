#pragma once

#include <glad/glad.h>
namespace hm
{
#ifdef DEBUG
// from
// https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions#detailed-messages-with-debug-output
void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                     GLsizei length, const GLchar* message,
                     const void* user_param);

#endif

} // namespace hm
