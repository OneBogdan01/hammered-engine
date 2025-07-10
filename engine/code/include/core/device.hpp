#pragma once

#include <string_view>
#include <glm/glm.hpp>
namespace hm
{
constexpr const char* WindowTitle {"Hammered Engine"};

namespace gfx
{

enum class GRAPHICS_API
{
  OPENGL,
  VULKAN
};

} // namespace gfx

class Device
{
 public:
  Device();
  ~Device();
  void Render();
  void ChangeGraphicsBackend() const;
  void PreRender();
  void Initialize();
  static void DestroyBackend();
  void SetGraphicsAPI(gfx::GRAPHICS_API api);

  void SetViewportSize(const glm::uvec2& windowSize,
                       const glm::ivec2& windowPosition);

 private:
  friend class Engine;
  glm::uvec2 m_windowSize {1280, 720};
  // Should be called before any other ImGui set-up calls
  void InitImGui();
  // API specific initialization of ImGui
  void InitPlatformImGui();

  bool m_shouldClose {false};
  bool m_shouldRender {false};

  gfx::GRAPHICS_API m_graphicsApi {gfx::GRAPHICS_API::OPENGL};
};

} // namespace hm
