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
class System

{
 public:
  virtual ~System() = default;

 protected:
  virtual void Update(f32) = 0;
};
class EntityComponentSystem
{
 public:
  Registry registry;
  EntityComponentSystem() = delete;
  ~EntityComponentSystem() = delete;
  EntityComponentSystem(const EntityComponentSystem&) = delete;
  EntityComponentSystem(const EntityComponentSystem&&) = delete;
  EntityComponentSystem& operator=(const EntityComponentSystem&) = delete;
  EntityComponentSystem& operator=(const EntityComponentSystem&&) = delete;

 private:
  friend class hm::Engine;
};

} // namespace hm::ecs
