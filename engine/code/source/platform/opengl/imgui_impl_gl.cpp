#include "external/imgui_impl.hpp"

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>

#include <platform/opengl/imgui_impl_gl.hpp>

void hm::external::ImGuiCleanUp()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void hm::external::ImGuiStartFrame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}
void hm::external::ImGuiEndFrame()
{
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void hm::external::ImGuiInitializeOpenGL(SDL_Window* window, SDL_GLContext ctx)
{
  ImGui_ImplSDL3_InitForOpenGL(window, ctx);
  ImGui_ImplOpenGL3_Init();
}
