#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/renderer.hpp"
// NOLINT(cppcoreguidelines-macro-usage)
#define SDL_MAIN_USE_CALLBACKS 1

#include "core/input.hpp"

#include <SDL3/SDL_main.h>
// This could be moved to a config file or similar
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  auto &engineInstance = hm::Engine::Instance();

  engineInstance.Init();
  engineInstance.GetECS().CreateSystem<hm::gpx::Renderer>("Renderer");
  return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate(void *appstate)
{
  auto &engineInstance = hm::Engine::Instance();

  return engineInstance.Run();
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  return hm::Engine::Instance().GetInput().ProcessInput(event);
}
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  auto &engineInstance = hm::Engine::Instance();
  engineInstance.Shutdown();
}
