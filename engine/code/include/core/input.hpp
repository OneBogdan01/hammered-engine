#pragma once
#include "ecs.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
namespace hm::utility
{
class InputCommand;
}

namespace hm::input
{
class InputHandler
{
 public:
  virtual ~InputHandler() = default;
  virtual void HandleInput(SDL_Event* event) = 0;
};
class Input
{
 public:
  // updated each frame with new SDL events
  SDL_AppResult ProcessInput(SDL_Event* event) const;

  void AddInputHandler(InputHandler& inputHandler);
  void RemoveInputHandler(const InputHandler& inputHandler);

 private:
  // vector of references for other handlers, do not free elements
  std::vector<InputHandler*> m_inputHandlers {};

  friend class ::hm::Engine;
};
} // namespace hm::input
