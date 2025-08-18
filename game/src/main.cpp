#include "engine.hpp"
#include "core/ecs.hpp"
#include "core/renderer.hpp"
#include "fmt/format.h"
#include "utility/console.hpp"

int main()
{
  auto& engineInstance = hm::Engine::Instance();

  engineInstance.Init();

  engineInstance.GetECS().CreateSystem<hm::gpx::Renderer>("Renderer");

  engineInstance.Run();
  engineInstance.Shutdown();
}
