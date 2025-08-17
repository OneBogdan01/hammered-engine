#pragma once
#include "ecs.hpp"

namespace hm::gpx
{

class Renderer : public ecs::System
{
 protected:
  void Update(f32) override;
};
} // namespace hm::gpx
