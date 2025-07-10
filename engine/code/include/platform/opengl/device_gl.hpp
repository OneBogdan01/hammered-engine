#pragma once

struct SDL_GLContextState;
struct SDL_Window;

namespace hm::device
{
SDL_GLContextState* GetGlContext();
SDL_Window* GetWindow();

} // namespace hm::device
