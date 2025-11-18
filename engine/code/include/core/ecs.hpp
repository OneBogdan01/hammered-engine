#pragma once
#include "utility/macros.hpp"

namespace hm
{
class Engine;
}
namespace hm::ecs
{
using Entity = entt::entity;
using Registry = entt::registry;
struct DeleteFlag
{
};
struct System

{
  System(const std::string& name);
  virtual ~System() = default;
  HM_NON_COPYABLE_NON_MOVABLE(System);

  virtual void Update(f32) = 0;
  virtual void Render() = 0;

  std::string m_name {};
  // Used to sort the systems
  u32 m_priority {};
};
class EntityComponentSystem
{
 public:
  EntityComponentSystem(const EntityComponentSystem&) = delete;
  EntityComponentSystem(const EntityComponentSystem&&) = delete;
  EntityComponentSystem& operator=(const EntityComponentSystem&) = delete;
  EntityComponentSystem& operator=(const EntityComponentSystem&&) = delete;
  // System
  // Updates all registered systems with delta time
  void UpdateSystems(f32);
  void RenderSystems();
  template<typename T, typename... Args>
  T& CreateSystem(Args&&... args);
  template<typename T>
  T& GetSystem();

  // Components
  template<typename T>
  T& AddComponent(Entity entity);
  template<typename T>
  T& GetComponent(Entity entity);
  template<typename T>
  bool HasComponent(Entity entity);
  //  Creates and returns the entity id
  Entity CreateEntity();
  // Adds a DeleteFlag component to the specified entity
  void QueueEntityDeletion(Entity entity);
  void DeleteEntity(Entity entity);
  // This will get all entities that have a  DeleteFlag component and
  // delete them
  void DeleteEntities();

  // Components
 private:
  Registry m_registry {};
  EntityComponentSystem() = default;
  ~EntityComponentSystem() = default;
  std::vector<std::unique_ptr<System>> m_systems {};
  friend class hm::Engine;
};
template<typename T, typename... Args>
T& EntityComponentSystem::CreateSystem(Args&&... args)
{
  T* system = new T(std::forward<Args>(args)...);
  m_systems.emplace_back(std::unique_ptr<System>(system));
  // sort based on priority?
  return *system;
}
template<typename T>
T& EntityComponentSystem::GetSystem()
{
  auto it = std::find_if(m_systems.begin(), m_systems.end(),
                         [](const auto& system)
                         {
                           return dynamic_cast<T*>(system.get()) != nullptr;
                         });

  SDL_assert(it != m_systems.end());
  return *dynamic_cast<T*>(it->get());
}

template<typename T>
T& EntityComponentSystem::AddComponent(Entity entity)
{
  return m_registry.emplace<T>(entity);
}
template<typename T>
T& EntityComponentSystem::GetComponent(Entity entity)
{
  return m_registry.get<T>(entity);
}
template<typename T>
bool EntityComponentSystem::HasComponent(Entity entity)
{
  return m_registry.try_get<T>(entity);
}

} // namespace hm::ecs
