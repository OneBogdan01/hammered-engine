#pragma once
#include "command.hpp"
namespace hm::utility
{
class InputCommand : public Command
{
 public:
  void Execute() override;
};
} // namespace hm::utility
