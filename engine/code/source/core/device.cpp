#include "core/device.hpp"

#include "core/fileio.hpp"

#include <SDL3/SDL_init.h>

#include "utility/console.hpp"

#include <imgui.h>

using namespace hm;

void Device::SetGraphicsAPI(gfx::GRAPHICS_API api)
{
  m_graphicsApi = api;
}

Device::Device()
{
  SetGraphicsAPI(io::LoadCurrentGraphicsAPI());

  constexpr SDL_InitFlags flags {SDL_INIT_VIDEO};
  SDL_SetAppMetadata(WindowTitle, "0", "HammE");
  if (SDL_Init(flags) == false)
  {
    log::Error("SDL could not be initialized");
    return;
  }
  // set the validation layers to false if not in debug
#ifdef _DEBUG
  m_bValidationLayer = true;
#endif
  Initialize();
  // TODO check if init failed
  log::Info("Hammered Engine was initialized");
}

void Device::ChangeGraphicsBackend() const
{
  ImGui::Begin("Graphics Settings");

  static const char* apiOptions[] = {"OpenGL", "Vulkan"};
  static int selectedApiIndex =
      (m_graphicsApi == gfx::GRAPHICS_API::OPENGL) ? 0 : 1;

  if (ImGui::Combo("Graphics API", &selectedApiIndex, apiOptions,
                   IM_ARRAYSIZE(apiOptions)))
  {
    const gfx::GRAPHICS_API chosen = (selectedApiIndex == 0)
                                         ? gfx::GRAPHICS_API::OPENGL
                                         : gfx::GRAPHICS_API::VULKAN;

    if (chosen != m_graphicsApi)
    {
      io::SaveGraphicsAPIToConfig(
          (chosen == gfx::GRAPHICS_API::OPENGL) ? "OPENGL" : "VULKAN");

      // gracefully shut down the engine
      io::RestartApplication();
    }
  }

  ImGui::End();
}
