#include "platform/opengl/opengl_gl.hpp"

#include "utility/console.hpp"
#ifdef DEBUG
void hm::MessageCallback(GLenum source, GLenum type, GLuint id,
                           GLenum severity, [[maybe_unused]] GLsizei length,
                           const GLchar* message,
                           [[maybe_unused]] const void* user_param)
{
  const char* srcStr = "UNKNOWN";
  switch (source)
  {
    case GL_DEBUG_SOURCE_API:
      srcStr = "API";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      srcStr = "WINDOW SYSTEM";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      srcStr = "SHADER COMPILER";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      srcStr = "THIRD PARTY";
      break;
    case GL_DEBUG_SOURCE_APPLICATION:
      srcStr = "APPLICATION";
      break;
    case GL_DEBUG_SOURCE_OTHER:
      srcStr = "OTHER";
      break;
  }

  const char* typeStr = "UNKNOWN";
  switch (type)
  {
    case GL_DEBUG_TYPE_ERROR:
      typeStr = "ERROR";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      typeStr = "DEPRECATED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      typeStr = "UNDEFINED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      typeStr = "PORTABILITY";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      typeStr = "PERFORMANCE";
      break;
    case GL_DEBUG_TYPE_MARKER:
      typeStr = "MARKER";
      break;
    case GL_DEBUG_TYPE_OTHER:
      typeStr = "OTHER";
      break;
  }

  const char* severityStr = "UNKNOWN";
  switch (severity)
  {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      severityStr = "NOTIFICATION";
      break;
    case GL_DEBUG_SEVERITY_LOW:
      severityStr = "LOW";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      severityStr = "MEDIUM";
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      severityStr = "HIGH";
      break;
  }

  log::Error("{}, {}, {}, {} : {}", srcStr, typeStr, severityStr, id, message);
}

#endif
