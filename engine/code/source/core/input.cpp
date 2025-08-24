#include "core/input.hpp"

#include "camera.hpp"
#include "engine.hpp"
#include "core/device.hpp"
#include "utility/console.hpp"

#include <SDL3/SDL_init.h>
#include <backends/imgui_impl_sdl3.h>

SDL_AppResult hm::input::Input::ProcessInput(SDL_Event* event) const
{
  for (const auto& handler : m_inputHandlers)
  {
    handler->HandleInput(event);
  }
  auto& device = hm::Engine::Instance().GetDevice();
  if (event->type == SDL_EVENT_QUIT ||
      (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE))
  {
    device.SetCloseRequest(true);
  }
  if (event->type == SDL_EVENT_WINDOW_MINIMIZED)
  {
    device.SetMinimize(true);
  }
  else if (event->type == SDL_EVENT_WINDOW_RESTORED)
  {
    device.SetMinimize(false);
  }
  // m_pDevice->processSDLEvent(event);
  // disable imgui mouse input

  if (SDL_GetWindowRelativeMouseMode(
          &Engine::Instance().GetDevice().GetSDLWindow()) == false)
  {
    ImGui_ImplSDL3_ProcessEvent(event);
  }

  return SDL_APP_CONTINUE;
}

void hm::input::Input::AddInputHandler(InputHandler& inputHandler)
{
  m_inputHandlers.push_back(&inputHandler);
}
void hm::input::Input::RemoveInputHandler(const InputHandler& inputHandler)
{
  const auto iterator =
      std::find(m_inputHandlers.begin(), m_inputHandlers.end(), &inputHandler);

  if (iterator != m_inputHandlers.end())
  {
    m_inputHandlers.erase(iterator);
  }
  else
  {
    log::Error("Could not find  to remove from input handler");
  }
}
