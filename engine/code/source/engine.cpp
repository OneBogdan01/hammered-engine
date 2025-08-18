#include "engine.hpp"

#include "core/ecs.hpp"
#include "core/input.hpp"
#include "camera.hpp"
#include "utility/console.hpp"


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
  m_pDevice = new Device();
  m_pInput = new input::Input();

  m_pEntityComponentSystem = new EntityComponentSystem();

  // TODO create camera based if it is the editor or not
  auto& camera =
      m_pEntityComponentSystem->CreateSystem<Camera>("Camera Editor");
  m_pInput->AddInputHandler(camera);
}

SDL_AppResult Engine::Run()
{
  if (m_pDevice->m_bShouldClose == true)
    return SDL_APP_SUCCESS;

  auto start = std::chrono::system_clock::now();

  // do not draw if we are minimized
  if (m_pDevice->m_bMinimized)
  {
    // throttle the speed to avoid the endless spinning
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return SDL_APP_CONTINUE;
  }
  // TODO move to a proper place
  if (m_pDevice->m_bResizeRequested)
  {
    m_pDevice->ResizeSwapchain();
  }

  // m_pDevice->PreRender();

  // m_pDevice->Render();
  // TODO add delta time
  m_pEntityComponentSystem->UpdateSystems(0.1f);
  m_pEntityComponentSystem->RenderSystems();
  m_pDevice->EndFrame();
  auto end = std::chrono::system_clock::now();
  // convert to microseconds (integer), and then come back to miliseconds
  auto elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  stats.frametime = elapsed.count() / 1000.f;
  return SDL_APP_CONTINUE;
}

void Engine::Shutdown()
{
  Info("Engine is freeing resources");
  delete m_pEntityComponentSystem;
  delete m_pInput;

  delete m_pDevice;
  Info("Engine is closed");
}
