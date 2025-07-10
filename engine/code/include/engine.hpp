#pragma once
#include "core/device.hpp"
namespace hm
{

class Engine
{
 public:
  static Engine& Instance();
  /// <summary>
  /// Initializes core modules
  /// </summary>
  void Init();
  /// <summary>
  /// Starts the update loop
  /// </summary>
  void Run();
  /// <summary>
  /// Cleans up resources and close window
  /// </summary>
  void Shutdown();

  Device& GetDevice() const { return *m_device; }

 private:
  Device* m_device {nullptr};
};
} // namespace hm
