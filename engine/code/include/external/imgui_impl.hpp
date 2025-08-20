
#pragma once

namespace hm::external
{
void ImGuiCleanUp();
// Generic initialization for ImGui, called before ImGuiInitializePlatform
void ImGuiInitialize();
// Initialized with specific graphics API context, ImGuiInitialize needs to be
// called before calling this

void ImGuiStartFrame();
void ImGuiEndFrame();

} // namespace hm::external
inline void hm::external::ImGuiInitialize()
{
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad;            // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // IF using Docking Branch}
}
