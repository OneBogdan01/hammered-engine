#include "engine.hpp"
#include "fmt/format.h"
#include "utility/console.hpp"

int main()
{
  auto& engineInstance = tale::Engine::Instance();
  engineInstance.Init();
  tale::Engine another;
  tale::log::Info("{}", fmt::ptr(&engineInstance));
  engineInstance.Run();
  engineInstance.Quit();

  another;
}
