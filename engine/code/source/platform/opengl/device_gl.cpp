#include "core/device.hpp"

#include "platform/opengl/device_gl.hpp"

#include "core/fileio.hpp"
#include "external/imgui_impl.hpp"
#include "platform/opengl/imgui_impl_gl.hpp"
#include "platform/opengl/opengl_gl.hpp"

#include "utility/console.hpp"

namespace
{
SDL_GLContext glContext {nullptr};
SDL_Window* window {nullptr};
} // namespace

using namespace hm;

Device::~Device()
{
  DestroyBackend();
}
// TODO
void Device::ResizeSwapchain() {}

void Device::Initialize()
{
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window = SDL_CreateWindow(WindowTitle, static_cast<int32_t>(m_windowSize.x),
                            static_cast<int32_t>(m_windowSize.y),
                            SDL_INIT_VIDEO | SDL_WINDOW_OPENGL);
  if (window == nullptr)
  {
    log::Error("SDL could not create window {}", SDL_GetError());
    return;
  }

  glContext = SDL_GL_CreateContext(window);
  if (glContext == nullptr)
  {
    log::Error("SDL could not create a OpenGL context {}", SDL_GetError());
    return;
  }
  if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) ==
      0)
  {
    log::Error("Failed to initialize GLAD");
    return;
  }
  log::Info("GPU used: {}",
            reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

#ifdef DEBUG

  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Optional: to ensure callback is
                                         // called immediately
  glDebugMessageCallback(MessageCallback, nullptr);
  // Enable everything except notifications
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                        GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

  log::Info("OpenGL debug output enabled.");

#endif
  external::ImGuiInitialize();
  external::ImGuiInitializeOpenGL(window, glContext);
}

void Device::DestroyBackend()
{
  external::ImGuiCleanUp();
  SDL_GL_DestroyContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
void Device::SetViewportSize(const glm::uvec2& windowSize,
                             const glm::ivec2& windowPosition)
{
  glViewport(windowPosition.x, windowPosition.y, static_cast<i32>(windowSize.x),
             static_cast<i32>(windowSize.y));
}
// void Device::PreRender()
//{
//   ChangeGraphicsBackend();
// }

void Device::EndFrame()
{
  // this stays in device
  SDL_GL_SwapWindow(window);
}

SDL_GLContextState* device::GetGlContext()
{
  return glContext;
}

SDL_Window* device::GetWindow()
{
  return window;
};
