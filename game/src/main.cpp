#include "engine.hpp"
#include "fmt/format.h"
#include "utility/console.hpp"

int main()
{
  auto& engineInstance = hm::Engine::Instance();

  engineInstance.Instance();
  engineInstance.Init();
  engineInstance.Run();
  engineInstance.Shutdown();
}
