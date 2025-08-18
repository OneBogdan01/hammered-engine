#pragma once
#include "ecs.hpp"

namespace hm::gpx
{

class Renderer : public ecs::System
{
 public:
  ~Renderer() override;
  explicit Renderer(const std::string& name) : System(name) {}
  void Update(f32) override;

  void Render() override;
};
} // namespace hm::gpx
