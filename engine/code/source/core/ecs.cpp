#include "core/ecs.hpp"
hm::ecs::System::System(const std::string& name) : m_name(name) {}
hm::ecs::Entity hm::ecs::EntityComponentSystem::CreateEntity()
{
  return m_registry.create();
}
void hm::ecs::EntityComponentSystem::QueueEntityDeletion(const Entity entity)
{
  m_registry.emplace<DeleteFlag>(entity);
}
void hm::ecs::EntityComponentSystem::DeleteEntity(Entity entity)
{
  m_registry.destroy(entity);
}
void hm::ecs::EntityComponentSystem::DeleteEntities()
{
  const auto deleteView = m_registry.view<DeleteFlag>();
  m_registry.destroy(deleteView.begin(), deleteView.end());
}
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
