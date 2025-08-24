#pragma once
namespace hm::utility
{
class Command
{
 public:
  virtual ~Command() {}
  virtual void Execute() = 0;
};
} // namespace hm::utility
