#include "core/ecs.hpp"
hm::ecs::System::System(const std::string& name) : m_name(name) {}
void hm::ecs::EntityComponentSystem::UpdateSystems(f32 dt)
{
  for (const auto& system : m_systems)
  {
    system->Update(dt);
  }
}
void hm::ecs::EntityComponentSystem::RenderSystems()
{
  for (const auto& system : m_systems)
  {
    system->Render();
  }
}
