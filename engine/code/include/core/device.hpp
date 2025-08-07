#pragma once

#include <string_view>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <SDL3/SDL_events.h>
#include <glm/glm.hpp>

class Camera;
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
  ~Device();
  void Render();
  void ChangeGraphicsBackend() const;
  void PreRender();
  void Initialize();
  static void DestroyBackend();
  void SetGraphicsAPI(gfx::GRAPHICS_API api);
  static void processSDLEvent(SDL_Event& e);
  Device();

  void SetViewportSize(const glm::uvec2& windowSize,
                       const glm::ivec2& windowPosition);

 private:
  friend class Engine;
  glm::uvec2 m_windowSize {1280, 720};
  // Should be called before any other ImGui set-up calls
  void InitImGui();
  // API specific initialization of ImGui
  void InitPlatformImGui();
  void resize_swapchain();

  bool m_shouldClose {false};
  bool m_shouldRender {false};

  gfx::GRAPHICS_API m_graphicsApi {gfx::GRAPHICS_API::OPENGL};
  bool resize_requested {false};
};

} // namespace hm
