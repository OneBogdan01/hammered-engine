#pragma once

#include <SDL3/SDL_events.h>

class Camera;
namespace hm
{
constexpr const char* WindowTitle {"Hammered Engine"};

namespace gfx
{

enum class GRAPHICS_API
{
  UNDEFINED,
  OPENGL,
  VULKAN
};

} // namespace gfx

// In charge of window creation and manipulation
class Device
{
 public:
  Device();
  ~Device();

  void EndFrame();
  // TODO move to editor
  void ChangeGraphicsBackend() const;

  // TODO remove, move to own camera system
  // static void processSDLEvent(SDL_Event& e);

  // getters
  SDL_Window& GetSDLWindow() const { return *m_pWindow; };
  glm::uvec2 GetWindowSize() const { return m_windowSize; };
  // setters
  void SetResizeRequest(bool resizeRequest)
  {
    m_bResizeRequested = resizeRequest;
  }
  void SetGraphicsAPI(gfx::GRAPHICS_API api);
  void SetViewportSize(const glm::uvec2& windowSize,
                       const glm::ivec2& windowPosition);

 private:
  void DestroyBackend();

  void Initialize();

  glm::uvec2 m_windowSize {1280, 720};
  void ResizeSwapchain();

  bool m_bShouldClose {false};
  bool m_bMinimized {false};
  bool m_bResizeRequested {false};
  bool m_bValidationLayer {false};
  gfx::GRAPHICS_API m_graphicsApi {gfx::GRAPHICS_API::UNDEFINED};
  SDL_Window* m_pWindow {nullptr};

  friend class Engine;
};
// TODO move to profiler or something
struct EngineStats
{
  float frametime;
  int triangle_count;
  int drawcall_count;
  float scene_update_time;
  float mesh_draw_time;
};
inline EngineStats stats;
} // namespace hm
