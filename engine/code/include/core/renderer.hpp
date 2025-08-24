#pragma once
#include "ecs.hpp"

namespace hm::gpx
{

class Renderer : public ecs::System
{
 public:
  explicit Renderer(const std::string& name);
  ~Renderer() override;
  void Update(f32) override;

  void Render() override;
};
} // namespace hm::gpx
