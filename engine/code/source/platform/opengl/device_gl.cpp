#include "core/device.hpp"

#include "platform/opengl/device_gl.hpp"

#include "core/fileio.hpp"
#include "external/imgui_impl.hpp"
#include "platform/opengl/imgui_impl_gl.hpp"
#include "platform/opengl/opengl_gl.hpp"
#include "platform/opengl/shader_gl.hpp"

#include "utility/console.hpp"

#include <vector>

#include <glm/gtc/type_ptr.hpp>
namespace hm::internal
{
SDL_GLContext glContext {nullptr};
SDL_Window* window {nullptr};
} // namespace hm::internal

using namespace hm;
using namespace hm::internal;

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

  window = SDL_CreateWindow(WindowTitle, m_windowSize.x, m_windowSize.y,
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
      false)
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
  glViewport(windowPosition.x, windowPosition.y, windowSize.x, windowSize.y);
}
unsigned int quadVAO = 0;
unsigned int quadVBO;
void RenderQuad()
{
  if (quadVAO == 0)
  {
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)(3 * sizeof(float)));
  }
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}
// void Device::PreRender()
//{
//   ChangeGraphicsBackend();
// }
struct ComputeEffect
{
  std::string name;
  Shader shader;
};
  // void Device::Render()
  //{
  //  // move to render
  //  ImGui::ShowDemoWindow(); // Show demo window! :)
  //  struct Constants
  //  {
  //    glm::vec4 data1 = glm::vec4(1.0, 0.0, 0.0, 1.0);
  //    glm::vec4 data2 = glm::vec4(0.0, 0.0, 1.0, 1.0);
  //    glm::vec4 data3 = glm::vec4(0.0, 0.0, 1.0, 1.0);
  //    glm::vec4 data4 = glm::vec4(1.0, 1.0, 0.0, 1.0);
  //  };
  //
  //  // Sample values (you can hook these into ImGui like in your comments)
  //  static Constants constants {};
  //  static int currentBackgroundEffect = 0;
  //  static std::vector<ComputeEffect> backgroundEffects;
  //
  //  if (ImGui::Begin("background"))
  //  {
  //    if (!backgroundEffects.empty())
  //    {
  //      ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];
  //
  //      ImGui::Text("Selected effect: %s", selected.name.c_str());
  //      ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0,
  //                       static_cast<int>(backgroundEffects.size()) - 1);
  //    }
  //
  //    ImGui::InputFloat4("data1", (float*)&constants.data1);
  //    ImGui::InputFloat4("data2", (float*)&constants.data2);
  //    ImGui::InputFloat4("data3", (float*)&constants.data3);
  //    ImGui::InputFloat4("data4", (float*)&constants.data4);
  //  }
  //  ImGui::End();
  //
  //  glClearColor(1, 0, 0, 1.0f);
  //  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //
  //  // TODO make it so it defaults to glsl in case it is not supported
  //
  //  static Shader
  //  triangle({io::GetPath("shaders/colored_triangle.vert.gl.spv"),
  //                          io::GetPath("shaders/colored_triangle.frag.gl.spv")});
  //  static Shader quadShader({io::GetPath("shaders/screen_quad.vert.gl.spv"),
  //                            io::GetPath("shaders/screen_quad.frag.gl.spv")});
  //  static ShaderPaths gradient;
  //  static ShaderPaths sky;
  //
  //  static bool once {true};
  //  // Memory leak
  //  static GLuint vao {};
  //  static glm::mat4 camMatrix {glm::mat4(1.0f)};
  //  static unsigned int texture;
  //  static GLuint uboConstants;
  //
  //  if (once)
  //  {
  //    glGenBuffers(1, &uboConstants);
  //    glBindBuffer(GL_UNIFORM_BUFFER, uboConstants);
  //    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * 4, nullptr,
  //                 GL_DYNAMIC_DRAW);
  //    glBindBufferBase(GL_UNIFORM_BUFFER, 1,
  //                     uboConstants); // Bind to binding = 1 in GLSL
  //
  //    gradient.Compute = io::GetPath("shaders/gradient_color.comp.gl.spv");
  //    sky.Compute = io::GetPath("shaders/sky.comp.gl.spv");
  //    // texture
  //    //   Create texture for opengl operation
  //    //   -----------------------------------
  //
  //    glGenTextures(1, &texture);
  //    glActiveTexture(GL_TEXTURE0);
  //    glBindTexture(GL_TEXTURE_2D, texture);
  //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_windowSize.x,
  //    m_windowSize.y,
  //                 0, GL_RGBA, GL_FLOAT, NULL);
  //
  //    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE,
  //    GL_RGBA16F);
  //
  //    glActiveTexture(GL_TEXTURE0);
  //    glBindTexture(GL_TEXTURE_2D, texture);
  //
  //    camMatrix = glm::scale(camMatrix, {1.0f, -1.f, 1.0f});
  //
  //    glGenVertexArrays(1, &vao);
  //
  //    static Shader gradientShader {gradient};
  //    static Shader skyShader {sky};
  //    static ComputeEffect grad {"Gradient", gradientShader};
  //    static ComputeEffect sk {"Sky", skyShader};
  //    backgroundEffects.reserve(2);
  //    backgroundEffects.push_back(grad);
  //    backgroundEffects.push_back(sk);
  //    // upload matrix
  //    once = false;
  //  }
  //  // Update UBO
  //  glBindBuffer(GL_UNIFORM_BUFFER, uboConstants);
  //  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Constants), &constants);
  //  // compute
  //
  //  static GLint loc = glGetUniformLocation(triangle.m_programId,
  //  "uViewProj");
  //  backgroundEffects[currentBackgroundEffect].shader.Activate();
  //  GLuint groupCountX = (m_windowSize.x + 16 - 1) / 16;
  //  GLuint groupCountY = (m_windowSize.y + 16 - 1) / 16;
  //  glDispatchCompute(groupCountX, groupCountY, 1);
  //
  //  // make sure writing to image has finished before read
  //  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  //  static GLint locQuad =
  //      glGetUniformLocation(quadShader.m_programId, "uViewProj");
  //
  //  quadShader.Activate();
  //  glUniformMatrix4fv(locQuad, 1, GL_FALSE, glm::value_ptr(camMatrix));
  //  RenderQuad();
  //  triangle.Activate();
  //  glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(camMatrix));
  //  glBindVertexArray(vao);
  //  // Draw 3 vertices to form your triangle
  //  glDrawArrays(GL_TRIANGLES, 0, 3);
  //  glBindVertexArray(0);
  //
  //  // this stays in device
  //  SDL_GL_SwapWindow(window);
  //}

  SDL_GLContextState* device::GetGlContext() { return glContext; }

  SDL_Window* device::GetWindow() { return window; };
