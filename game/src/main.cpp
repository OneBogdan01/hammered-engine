#include "engine.hpp"

int main() {

  tale::WriteLine("Hello Game!");


  auto& engineInstance = tale::Engine::Instance();
  engineInstance.Init();
  engineInstance.Run();
  engineInstance.Quit();


}