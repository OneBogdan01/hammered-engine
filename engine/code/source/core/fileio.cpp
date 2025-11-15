#include "core/fileio.hpp"

#include <fstream>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_filesystem.h>
namespace hm::io
{
std::string GetPath(const char* path)
{
  return std::string(AssetPath) + path;
}
void SaveGraphicsAPIToConfig(const std::string& api)
{
  const std::string configPath = io::GetPath("") + "config.ini";

  std::ofstream out(configPath);

  out << "graphics_api=" << api << "\n";
  out.close();

  log::Info("Saved graphics API to config: {}", api);
}
void RestartApplication()
{
  const char* base_path = SDL_GetBasePath();
  if (!base_path)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get base path");
    return;
  }

  const auto selectedAPI = LoadGraphicsAPIFromConfig();

  std::string exeName {};

  switch (selectedAPI)
  {
    case gfx::GRAPHICS_API::VULKAN:
      exeName = VkExe.empty() == false ? VkExe.data() : "";
      break;
    case gfx::GRAPHICS_API::OPENGL:
      exeName = GlExe.empty() == false ? GlExe.data() : "";
      break;
  }

  if (exeName.empty())
  {
    log::Error("This backend was not compiled");
    return;
  }
  std::string exe_path = std::string(base_path) + exeName;

  char cmd[4096];

#ifdef _WIN32
  snprintf(cmd, sizeof(cmd), "cmd /C \"timeout 1 && start \"\" \"%s\"\"",
           exe_path.c_str());
#elif defined(__APPLE__)
  snprintf(cmd, sizeof(cmd), "sleep 1 && open \"%s\"", exe_path.c_str());
#else
  snprintf(cmd, sizeof(cmd), "sleep 1 && \"%s\" &", exe_path.c_str());
#endif

  SDL_Log("Restarting application with command: %s", cmd);

  system(cmd);

  SDL_Quit();
  exit(0);
}
gfx::GRAPHICS_API LoadCurrentGraphicsAPI()
{
#ifdef GFX_USE_VULKAN

  return gfx::GRAPHICS_API::VULKAN;
#elif GFX_USE_OPENGL
  return gfx::GRAPHICS_API::OPENGL;
#endif
}
gfx::GRAPHICS_API LoadGraphicsAPIFromConfig()
{
  const std::string configPath = io::GetPath("") + "config.ini";

  std::ifstream in(configPath);
  if (!in)
  {
    return gfx::GRAPHICS_API::OPENGL; // fallback
  }

  std::string line;
  gfx::GRAPHICS_API result = gfx::GRAPHICS_API::OPENGL; // default

  while (std::getline(in, line))
  {
    if (line.find("graphics_api=") == 0)
    {
      const std::string value = line.substr(strlen("graphics_api="));
      if (value == "VULKAN")
        result = gfx::GRAPHICS_API::VULKAN;
      else
        result = gfx::GRAPHICS_API::OPENGL;
      break;
    }
  }

  in.close();

  return result;
}
} // namespace hm::io
