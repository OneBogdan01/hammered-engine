#include "engine.hpp"

#include "core/ecs.hpp"
#include "core/fileio.hpp"

#include <SDL3/SDL_events.h>

#include "utility/console.hpp"

#include <backends/imgui_impl_sdl3.h>

using namespace hm;
using namespace hm::log;
using namespace hm::ecs;

Engine& Engine::Instance()
{
  static Engine engineInstance;
  return engineInstance;
}

void Engine::Init()
{
  m_device = new Device();
  m_ecs = new EntityComponentSystem();
}

void Engine::Run()
{
  while (m_device->m_shouldClose == false)
  {
    auto start = std::chrono::system_clock::now();
    // move to input class
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_EVENT_QUIT ||
          (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
      {
        m_device->m_shouldClose = true;
      }
      if (event.type == SDL_EVENT_WINDOW_MINIMIZED)
      {
        m_device->m_shouldRender = true;
      }
      if (event.type == SDL_EVENT_WINDOW_RESTORED)
      {
        m_device->m_shouldRender = false;
      }
      m_device->processSDLEvent(event);
      ImGui_ImplSDL3_ProcessEvent(&event); // Forward your event to backend
    }

    // TODO add delta time
    m_ecs->UpdateSystems(0.1f);
    m_ecs->RenderSystems();
    // do not draw if we are minimized
    if (m_device->m_shouldRender)
    {
      // throttle the speed to avoid the endless spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    // TODO move to a proper place
    if (m_device->resize_requested)
    {
      m_device->resize_swapchain();
    }

    m_device->PreRender();

    m_device->Render();
    auto end = std::chrono::system_clock::now();
    // convert to microseconds (integer), and then come back to miliseconds
    auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    stats.frametime = elapsed.count() / 1000.f;
  }
}
void Engine::Shutdown()
{
  delete m_device;
  delete m_ecs;
  Info("Engine is closed");
}
