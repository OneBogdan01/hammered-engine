#pragma once

#include <entt.hpp>
namespace hm
{
class Engine;
}
namespace hm::ecs
{
using Entity = entt::entity;
using Registry = entt::registry;
struct System

{
  System(const std::string& name);
  virtual ~System() = default;

  virtual void Update(f32) = 0;
  virtual void Render() = 0;

  std::string m_name {};
  // Used to sort the systems
  u32 m_priority {};
};
class EntityComponentSystem
{
 public:
  Registry registry {};

  EntityComponentSystem(const EntityComponentSystem&) = delete;
  EntityComponentSystem(const EntityComponentSystem&&) = delete;
  EntityComponentSystem& operator=(const EntityComponentSystem&) = delete;
  EntityComponentSystem& operator=(const EntityComponentSystem&&) = delete;

  template<typename T, typename... Args>
  T& CreateSystem(Args&&... args);
  void UpdateSystems(f32);
  void RenderSystems();

 private:
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

} // namespace hm::ecs
