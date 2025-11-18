#pragma once
#include "utility/macros.hpp"
namespace hm::utility
{
class Command
{
 public:
  virtual void Execute() = 0;
};
} // namespace hm::utility
