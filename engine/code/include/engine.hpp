#pragma once

namespace hm::ecs
{
class EntityComponentSystem;
}
namespace hm
{

class Device;

class Engine
{
 public:
  static Engine& Instance();

  /// <summary>
  /// Initializes core modules
  /// </summary>
  void Init();
  /// <summary>
  /// Starts the update loop
  /// </summary>
  SDL_AppResult Run();
  // TODO move to input class
  SDL_AppResult  Input(SDL_Event*event);
  /// <summary>
  /// Cleans up resources and close window
  /// </summary>
  void Shutdown();

  Device& GetDevice() const { return *m_pDevice; }
  ecs::EntityComponentSystem& GetECS() { return *m_pEntityComponentSystem; };

 private:
  Device* m_pDevice {nullptr};
  ecs::EntityComponentSystem* m_pEntityComponentSystem {nullptr};
};
} // namespace hm
