#include "engine.hpp"
#include "core/fileio.hpp"
#include "core/renderer.hpp"
#include "external/imgui_impl.hpp"
#include "platform/opengl/imgui_impl_gl.hpp"
#include "platform/opengl/opengl_gl.hpp"
#include "platform/opengl/shader_gl.hpp"
hm::gpx::Renderer::Renderer(const std::string& name) : System(name) {}
hm::gpx::Renderer::~Renderer() {}
struct ComputeEffect
{
  std::string name;
  hm::Shader shader;
};

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
void hm::gpx::Renderer::Render()
{
  external::ImGuiStartFrame();
  // move to render
  ImGui::ShowDemoWindow(); // Show demo window! :)
  struct Constants
  {
    glm::vec4 data1 = glm::vec4(1.0, 0.0, 0.0, 1.0);
    glm::vec4 data2 = glm::vec4(0.0, 0.0, 1.0, 1.0);
    glm::vec4 data3 = glm::vec4(0.0, 0.0, 1.0, 1.0);
    glm::vec4 data4 = glm::vec4(1.0, 1.0, 0.0, 1.0);
  };

  // Sample values (you can hook these into ImGui like in your comments)
  static Constants constants {};
  static int currentBackgroundEffect = 0;
  static std::vector<ComputeEffect> backgroundEffects;

  if (ImGui::Begin("background"))
  {
    if (!backgroundEffects.empty())
    {
      ComputeEffect& selected = backgroundEffects[currentBackgroundEffect];

      ImGui::Text("Selected effect: %s", selected.name.c_str());
      ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0,
                       static_cast<int>(backgroundEffects.size()) - 1);
    }

    ImGui::InputFloat4("data1", (float*)&constants.data1);
    ImGui::InputFloat4("data2", (float*)&constants.data2);
    ImGui::InputFloat4("data3", (float*)&constants.data3);
    ImGui::InputFloat4("data4", (float*)&constants.data4);
  }
  ImGui::End();

  glClearColor(1, 0, 0, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // TODO make it so it defaults to glsl in case it is not supported

  static Shader triangle({io::GetPath("shaders/colored_triangle.vert.gl.spv"),
                          io::GetPath("shaders/colored_triangle.frag.gl.spv")});
  static Shader quadShader({io::GetPath("shaders/screen_quad.vert.gl.spv"),
                            io::GetPath("shaders/screen_quad.frag.gl.spv")});
  static ShaderPaths gradient;
  static ShaderPaths sky;

  static bool once {true};
  // Memory leak
  static GLuint vao {};
  static glm::mat4 camMatrix {glm::mat4(1.0f)};
  static unsigned int texture;
  static GLuint uboConstants;

  if (once)
  {
    glGenBuffers(1, &uboConstants);
    glBindBuffer(GL_UNIFORM_BUFFER, uboConstants);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::vec4) * 4, nullptr,
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1,
                     uboConstants); // Bind to binding = 1 in GLSL

    gradient.Compute = io::GetPath("shaders/gradient_color.comp.gl.spv");
    sky.Compute = io::GetPath("shaders/sky.comp.gl.spv");
    // texture
    //   Create texture for opengl operation
    //   -----------------------------------

    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    auto windowSize = Engine::Instance().GetDevice().GetWindowSize();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowSize.x, windowSize.y, 0,
                 GL_RGBA, GL_FLOAT, NULL);

    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    camMatrix = glm::scale(camMatrix, {1.0f, -1.f, 1.0f});

    glGenVertexArrays(1, &vao);

    static Shader gradientShader {gradient};
    static Shader skyShader {sky};
    static ComputeEffect grad {"Gradient", gradientShader};
    static ComputeEffect sk {"Sky", skyShader};
    backgroundEffects.reserve(2);
    backgroundEffects.push_back(grad);
    backgroundEffects.push_back(sk);
    // upload matrix
    once = false;
  }
  // Update UBO
  glBindBuffer(GL_UNIFORM_BUFFER, uboConstants);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Constants), &constants);
  // compute

  static GLint loc = glGetUniformLocation(triangle.m_programId, "uViewProj");
  backgroundEffects[currentBackgroundEffect].shader.Activate();
  auto windowSize = Engine::Instance().GetDevice().GetWindowSize();
  GLuint groupCountX = (windowSize.x + 16 - 1) / 16;
  GLuint groupCountY = (windowSize.y + 16 - 1) / 16;
  glDispatchCompute(groupCountX, groupCountY, 1);

  // make sure writing to image has finished before read
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  static GLint locQuad =
      glGetUniformLocation(quadShader.m_programId, "uViewProj");

  quadShader.Activate();
  glUniformMatrix4fv(locQuad, 1, GL_FALSE, glm::value_ptr(camMatrix));
  RenderQuad();
  triangle.Activate();
  glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(camMatrix));
  glBindVertexArray(vao);
  // Draw 3 vertices to form your triangle
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  external::ImGuiEndFrame();
}
void hm::gpx::Renderer::Update(f32 dt) {}
