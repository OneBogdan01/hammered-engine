#pragma once
#include "core/device.hpp"
#include "utility/logger.hpp"

#include <filesystem>
#include <SDL3/SDL_init.h>

namespace hm::io
{

std::string GetPath(const char* path);
void SaveGraphicsAPIToConfig(const std::string& api);
void RestartApplication();
gfx::GRAPHICS_API LoadGraphicsAPIFromConfig();
gfx::GRAPHICS_API LoadCurrentGraphicsAPI();

#ifdef GAME_GL_EXECUTABLE_NAME
inline constexpr std::string_view GlExe {GAME_GL_EXECUTABLE_NAME};
#else
inline constexpr std::string_view GlExe {};
#endif

#ifdef GAME_VK_EXECUTABLE_NAME
inline constexpr std::string_view VkExe {GAME_VK_EXECUTABLE_NAME};
#else
inline constexpr std::string_view VkExe {};
#endif

constexpr std::string_view AssetPath {"assets/"};
} // namespace hm::io
