#pragma once
#include "core/device.hpp"

#include "platform/vulkan/descriptors_vk.hpp"

namespace hm
{
namespace internal
{
struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void push_function(std::function<void()>&& function)
  {
    deletors.push_back(function);
  }

  void flush()
  {
    // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
    {
      (*it)(); // call functors
    }

    deletors.clear();
  }
};

struct FrameData
{
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
  VkSemaphore _swapchainSemaphore, _renderSemaphore;
  VkFence _renderFence;
  DeletionQueue _deletionQueue;
};
struct ComputePushConstants
{
  glm::vec4 data1;
  glm::vec4 data2;
  glm::vec4 data3;
  glm::vec4 data4;
};
struct ComputeEffect
{
  const char* name;

  VkPipeline pipeline;
  VkPipelineLayout layout;

  ComputePushConstants data;
};

constexpr uint32_t FRAME_OVERLAP = 2;
} // namespace internal
GPUMeshBuffers UploadMesh(std::span<uint32_t> indicies,
                          std::span<Vertex> vertices);
} // namespace hm
