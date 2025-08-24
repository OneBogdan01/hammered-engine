#pragma once

#include "core/ecs.hpp"
#include "core/input.hpp"

#include <SDL3/SDL_events.h>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

class Camera final : public hm::input::InputHandler, public hm::ecs::System
{
 public:
  explicit Camera(const std::string& name);
  ;

  glm::vec3 velocity;
  glm::vec3 position;
  // vertical rotation
  float pitch {0.f};
  // horizontal rotation
  float yaw {0.f};

  glm::mat4 getViewMatrix() const;
  glm::mat4 getRotationMatrix() const;

  void Update(f32) override;
  void Render() override;
  ~Camera() override;
  void HandleInput(SDL_Event* event) override;
};
